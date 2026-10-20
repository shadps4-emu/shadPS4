// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include "core/net/host_net.h"

namespace Core::Net::P2P {

using Clock = std::chrono::steady_clock;
using Host::Error;

// Sequence-number comparisons modulo 2^32.
constexpr bool SeqLt(u32 a, u32 b) {
    return static_cast<s32>(a - b) < 0;
}
constexpr bool SeqLe(u32 a, u32 b) {
    return static_cast<s32>(a - b) <= 0;
}
constexpr bool SeqGt(u32 a, u32 b) {
    return SeqLt(b, a);
}
constexpr bool SeqGe(u32 a, u32 b) {
    return SeqLe(b, a);
}

namespace TcpFlag {
constexpr u8 Fin = 0x01;
constexpr u8 Syn = 0x02;
constexpr u8 Rst = 0x04;
constexpr u8 Psh = 0x08;
constexpr u8 Ack = 0x10;
} // namespace TcpFlag

struct PseudoHeader {
    int family = AF_INET;
    std::array<u8, 16> local{}; // IPv4 uses first 4 bytes
    std::array<u8, 16> remote{};
};

struct TcpSegment {
    u16 src_port; // host byte order
    u16 dst_port;
    u32 seq;
    u32 ack;
    u8 flags;
    u16 window;
    std::optional<u16> mss;
    std::span<const u8> payload;
};

u16 TcpChecksum(int family, const u8* src_addr, const u8* dst_addr, std::span<const u8> segment);
std::optional<TcpSegment> ParseTcpSegment(std::span<const u8> bytes,
                                          const PseudoHeader* pseudo = nullptr);

std::vector<u8> BuildTcpSegment(const PseudoHeader& pseudo, u16 src_port, u16 dst_port, u32 seq,
                                u32 ack, u8 flags, u16 window, std::optional<u16> mss,
                                std::span<const u8> payload);
std::optional<std::vector<u8>> BuildResetFor(const PseudoHeader& pseudo, const TcpSegment& seg);

struct TcpConfig {
    // PS4 defaults for TCP over UDPP2P
    u32 recv_buffer = 65535;
    u32 send_buffer = 32 * 1024;
    u16 mss = 1200; // fits one P2P datagram after overhead
    std::chrono::milliseconds initial_rto{1000};
    std::chrono::milliseconds min_rto{200};
    std::chrono::milliseconds max_rto{60000};
    std::chrono::milliseconds delayed_ack{40};
    std::chrono::milliseconds time_wait{2000}; // TODO: FreeBSD 2MSL is 60 s
    int syn_retries = 6;
    int data_retries = 12;
};

enum class TcpState {
    Closed,
    SynSent,
    SynReceived,
    Established,
    FinWait1,
    FinWait2,
    CloseWait,
    Closing,
    LastAck,
    TimeWait,
};

const char* ToString(TcpState state);

class TcpConnection {
public:
    using Emit = std::function<void(std::span<const u8> segment)>;

    struct IoResult {
        size_t bytes;
        Error error;
    };

    TcpConnection(const TcpConfig& config, const PseudoHeader& pseudo, u16 local_port,
                  u16 remote_port, u32 iss, Emit emit);

    void Connect(Clock::time_point now);
    void AcceptSyn(const TcpSegment& syn, Clock::time_point now);
    void OnSegment(const TcpSegment& seg, Clock::time_point now);
    void OnTimer(Clock::time_point now);
    std::optional<Clock::time_point> NextDeadline() const;

    IoResult Send(std::span<const u8> data, Clock::time_point now);
    IoResult Recv(std::span<u8> out, bool peek, Clock::time_point now);
    void Shutdown(Clock::time_point now);
    void Close(Clock::time_point now);
    void Abort();

    TcpState State() const {
        return state_;
    }
    Error PendingError() const {
        return error_;
    }
    bool Readable() const;
    bool Writable() const;
    bool ReceivedFin() const {
        return fin_received_;
    }
    size_t SendSpace() const;
    size_t BytesToRead() const {
        return recv_buf_.size();
    }
    u16 LocalPort() const {
        return local_port_;
    }
    u16 RemotePort() const {
        return remote_port_;
    }

    u32 Retransmissions() const {
        return retransmissions_;
    }

private:
    bool Synchronized() const;
    u16 ReceiveWindow() const;
    bool FinSent() const;
    bool FinAcked() const;
    u32 FinSeq() const;

    void EmitSegment(u32 seq, u8 flags, std::span<const u8> payload, bool with_mss = false);
    void SendAck();
    void EmitReset(u32 seq);
    void SendData(u32 seq, u32 len);
    void TrySend(Clock::time_point now);
    void RetransmitFirst();

    bool Acceptable(u32 seq, u32 len) const;
    void HandleSynSent(const TcpSegment& seg, Clock::time_point now);
    bool ProcessAck(const TcpSegment& seg, Clock::time_point now);
    void OnNewAck(u32 ack, Clock::time_point now);
    void OnDuplicateAck();
    void ProcessData(const TcpSegment& seg, Clock::time_point now);
    void PullOutOfOrder();
    void MaybeSendWindowUpdate();

    void OnRetransmitTimeout(Clock::time_point now);
    void SampleRtt(Clock::duration rtt);
    void ArmRetransmit(Clock::time_point now);
    void EnterTimeWait(Clock::time_point now);
    void Terminate(Error error);
    void ClearTimers();

    const TcpConfig config_;
    const PseudoHeader pseudo_;
    const u16 local_port_;
    const u16 remote_port_;
    Emit emit_;

    TcpState state_ = TcpState::Closed;
    Error error_ = Error::Ok;

    u32 iss_;
    u32 snd_una_;
    u32 snd_nxt_;
    u32 snd_max_;
    u32 buf_seq_;
    u32 snd_wnd_ = 0;
    u32 snd_wl1_ = 0;
    u32 snd_wl2_ = 0;
    u16 snd_mss_ = 536;
    std::deque<u8> send_buf_;
    bool fin_queued_ = false;

    u32 cwnd_ = 0;
    u32 ssthresh_ = 0x7fffffff;
    int dupacks_ = 0;
    bool in_recovery_ = false;
    u32 recover_ = 0;
    Clock::duration srtt_{};
    Clock::duration rttvar_{};
    Clock::duration rto_;
    bool have_rtt_ = false;
    bool rtt_active_ = false;
    u32 rtt_seq_ = 0;
    Clock::time_point rtt_start_{};
    int retries_ = 0;
    Clock::duration persist_interval_{};

    std::optional<Clock::time_point> retransmit_deadline_;
    std::optional<Clock::time_point> persist_deadline_;
    std::optional<Clock::time_point> delayed_ack_deadline_;
    std::optional<Clock::time_point> time_wait_deadline_;
    u32 irs_ = 0;
    u32 rcv_nxt_ = 0;
    u64 rcv_offset_ = 0; // stream offset of rcv_nxt_
    std::deque<u8> recv_buf_;
    std::map<u64, std::vector<u8>> out_of_order_;
    size_t out_of_order_bytes_ = 0;
    std::optional<u32> peer_fin_seq_;
    bool fin_received_ = false;
    int unacked_segments_ = 0;
    u16 last_advertised_window_ = 0;

    u32 retransmissions_ = 0;
};

} // namespace Core::Net::P2P
