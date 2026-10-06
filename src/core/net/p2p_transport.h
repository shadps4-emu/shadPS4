// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// One UDP socket shared by P2P datagrams, P2P TCP streams and signaling.

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "core/net/p2p_tcp.h"

namespace Core::Net::P2P {

struct Endpoint {
    sockaddr_storage addr{};

    static Endpoint FromSockaddr(const sockaddr* sa, socklen_t len);
    static Endpoint IPv4(const char* dotted, u16 port);
    int Family() const {
        return addr.ss_family;
    }
    socklen_t Length() const;
    const sockaddr* Sockaddr() const {
        return reinterpret_cast<const sockaddr*>(&addr);
    }
    u16 Port() const; // host byte order
    bool operator<(const Endpoint& other) const;
    bool operator==(const Endpoint& other) const;
};

struct Protection {
    bool crypto = false;
    bool signature = false;
};

class Codec {
public:
    enum class Kind { Invalid, Stream, Datagram, Signaling };
    struct Decoded {
        Kind kind = Kind::Invalid;
        u16 src_vport = 0; // datagrams only
        u16 dst_vport = 0;
        std::vector<u8> payload;
    };

    virtual ~Codec() = default;
    virtual Decoded Decode(std::span<const u8> packet, const Endpoint& from) = 0;
    virtual std::vector<u8> EncodeStream(std::span<const u8> tcp_segment, const Endpoint& to,
                                         Protection protection) = 0;
    virtual std::vector<u8> EncodeDatagram(u16 src_vport, u16 dst_vport,
                                           std::span<const u8> payload, const Endpoint& to,
                                           Protection protection) = 0;
    virtual std::vector<u8> EncodeSignaling(std::span<const u8> data, const Endpoint& to) = 0;
    virtual PseudoHeader StreamPseudoHeader(const Endpoint& peer) = 0;
};

struct TransportConfig {
    TcpConfig tcp;
    size_t datagram_queue_bytes = 40 * 1024; // per vport, PS4's SO_RCVBUF
    size_t signaling_queue_packets = 256;    // per channel
    size_t signaling_channels = 1;
    std::function<size_t(std::span<const u8>)> classify_signaling;
    int max_backlog = 64;
    // TODO: NAT changes the addresses in the pseudo-header, so peer checksums may not
    // match. Off until tested.
    bool verify_stream_checksum = false;
};

class ReadinessFlag {
public:
    bool Valid() const {
        return wake_.Valid();
    }
    void Set(bool ready);
    Host::NativeSocket PollHandle() const {
        return wake_.PollHandle();
    }

private:
    Host::WakeHandle wake_;
    bool ready_ = false;
};

struct Readiness {
    ReadinessFlag readable; // data, EOF, error or accept
    ReadinessFlag writable; // space or error
};

struct IoResult {
    size_t bytes;
    Error error;
};

struct ConnectionKey {
    Endpoint peer;
    u16 peer_vport;
    u16 local_vport;
    bool operator<(const ConnectionKey& other) const;
};

class Transport;
struct DatagramBinding;
struct Connection;
struct ListenerState;

class DatagramSocket {
public:
    ~DatagramSocket();

    Error Bind(u16 vport); // 0 = any free vport
    Error SetPeer(const Endpoint& peer, u16 vport);
    void SetProtection(Protection protection);
    // to == nullptr sends to the SetPeer peer. extra is per-message protection.
    IoResult SendTo(std::span<const u8> data, const Endpoint* to, u16 to_vport,
                    Protection extra = {});
    IoResult RecvFrom(std::span<u8> out, bool peek, Endpoint* from, u16* from_vport);
    u32 Events() const; // Host::EventBits
    const Readiness& Handles() const {
        return *readiness_;
    }
    u16 BoundVport() const;
    u16 TransportPort() const;
    bool GetPeer(Endpoint* peer, u16* peer_vport) const;
    Protection GetProtection() const;
    void Close();

private:
    friend class Transport;
    explicit DatagramSocket(std::shared_ptr<Transport> transport);
    Error BindLocked(u16 vport);

    std::shared_ptr<Transport> transport_;
    std::shared_ptr<Readiness> readiness_;
    std::shared_ptr<DatagramBinding> binding_;
    Protection protection_;
    std::optional<std::pair<Endpoint, u16>> peer_;
};

class StreamSocket {
public:
    ~StreamSocket();

    void SetProtection(Protection protection);
    Error Bind(u16 vport);
    Error Listen(int backlog);
    Error Connect(const Endpoint& peer, u16 peer_vport);
    Error ConnectResult() const;
    std::shared_ptr<StreamSocket> Accept(Endpoint* peer, u16* peer_vport, Error* error);
    IoResult Send(std::span<const u8> data);
    IoResult Recv(std::span<u8> out, bool peek);
    Error Shutdown(int how); // 0 = read, 1 = write, 2 = both
    void Close();
    void Abort();

    u32 Events() const;
    const Readiness& Handles() const {
        return *readiness_;
    }
    std::optional<TcpState> State() const;
    bool GetPeer(Endpoint* peer, u16* peer_vport) const;
    u16 BoundVport() const;
    u16 TransportPort() const;
    Protection GetProtection() const;

private:
    friend class Transport;
    StreamSocket(std::shared_ptr<Transport> transport, std::shared_ptr<Readiness> readiness);
    Error BindLocked(u16 vport); // transport mutex held

    std::shared_ptr<Transport> transport_;
    std::shared_ptr<Readiness> readiness_;
    Protection protection_;
    u16 bound_vport_ = 0;
    bool owns_vport_ = false;
    bool read_shutdown_ = false;
    bool closed_ = false;
    std::shared_ptr<ListenerState> listener_;
    std::shared_ptr<Connection> connection_;
};

class Transport : public std::enable_shared_from_this<Transport> {
public:
    static std::shared_ptr<Transport> Create(int family, const Endpoint& bind_address,
                                             std::unique_ptr<Codec> codec,
                                             const TransportConfig& config, Error* error);
    ~Transport();

    u16 BoundPort() const {
        return bound_port_;
    }
    int Family() const {
        return family_;
    }

    std::shared_ptr<DatagramSocket> CreateDatagram();
    std::shared_ptr<StreamSocket> CreateStream();

    Error SendSignaling(std::span<const u8> data, const Endpoint& to);
    bool RecvSignaling(size_t channel, std::vector<u8>* data, Endpoint* from);

    void SetOutgoingLossForTesting(double probability);

private:
    friend class DatagramSocket;
    friend class StreamSocket;

    Transport(int family, std::unique_ptr<Codec> codec, const TransportConfig& config);

    void ThreadMain();
    void HandlePacket(std::span<const u8> packet, const Endpoint& from, Clock::time_point now);
    void HandleStreamSegment(std::span<const u8> bytes, const Endpoint& from,
                             Clock::time_point now);
    void HandleDatagram(Codec::Decoded& decoded, const Endpoint& from);
    void RunTimers(Clock::time_point now);
    std::optional<Clock::time_point> EarliestDeadline() const;

    std::shared_ptr<Connection> NewConnection(const ConnectionKey& key, Protection protection,
                                              std::shared_ptr<Readiness> readiness);
    void Update(const std::shared_ptr<Connection>& connection);
    void Reschedule(const Connection& connection);
    Error SendPacket(std::span<const u8> packet, const Endpoint& to);
    u16 AllocateVport(bool stream);
    u32 NewIss();

    const int family_;
    std::unique_ptr<Codec> codec_;
    const TransportConfig config_;
    Host::NativeSocket socket_ = Host::InvalidSocket;
    u16 bound_port_ = 0;

    mutable std::mutex mutex_;
    std::map<u16, std::shared_ptr<DatagramBinding>> datagram_bindings_;
    std::map<u16, std::shared_ptr<ListenerState>> listeners_;
    std::map<ConnectionKey, std::shared_ptr<Connection>> connections_;
    std::map<u16, int> stream_vports_in_use_;
    std::vector<std::deque<std::pair<std::vector<u8>, Endpoint>>> signaling_;
    u16 next_stream_port_ = 49152;    // 49152-65535
    u16 next_datagram_vport_ = 32768; // 32768-49999
    u64 iss_state_;
    double loss_for_testing_ = 0.0;
    u64 loss_rng_ = 0x9e3779b97f4a7c15ull;

    std::thread thread_;
    std::atomic<bool> stop_{false};
    Host::WakeHandle kick_;
    Clock::time_point sleeping_until_ = Clock::time_point::min();
    std::vector<u8> receive_buffer_;
};

} // namespace Core::Net::P2P
