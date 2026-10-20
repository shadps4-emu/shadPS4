// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Linux only needs root or CAP_NET_ADMIN, otherwise every test is skipped.

#include <gtest/gtest.h>

#ifdef __linux__

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <thread>

#include "core/net/p2p_tcp.h"

using namespace Core::Net;
using namespace Core::Net::P2P;
using namespace std::chrono_literals;

namespace {

constexpr const char* KernelIp = "10.77.0.1"; // the TUN interface address (kernel TCP)
constexpr const char* OurIp = "10.77.0.2";    // the address our TCP answers for

std::vector<u8> RandomBytes(size_t n, u32 seed) {
    std::mt19937 rng(seed);
    std::vector<u8> v(n);
    for (auto& b : v) {
        b = static_cast<u8>(rng());
    }
    return v;
}

u16 IpChecksum(const u8* header, size_t size) {
    u32 sum = 0;
    for (size_t i = 0; i < size; i += 2) {
        sum += (header[i] << 8) | header[i + 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return static_cast<u16>(~sum);
}

/// Kernel socket helpers with a generous timeout so a broken test fails instead of hanging.
int KernelSocket() {
    const int s = socket(AF_INET, SOCK_STREAM, 0);
    timeval tv{120, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    return s;
}

sockaddr_in Address(const char* ip, u16 port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

/// Our side of the network: owns TcpConnections keyed by (local port, remote port), accepts
/// SYNs on listening ports and resets everything else, like the P2P transport does.
class TunHost {
public:
    explicit TunHost(int tun) : tun_(tun) {
        inet_pton(AF_INET, OurIp, pseudo_.local.data());
        inet_pton(AF_INET, KernelIp, pseudo_.remote.data());
    }

    double loss = 0.0;
    std::map<std::pair<u16, u16>, std::unique_ptr<TcpConnection>> connections;
    std::map<u16, bool> listeners;

    TcpConnection& Connect(u16 local_port, u16 remote_port) {
        auto& c = connections[{local_port, remote_port}];
        c = std::make_unique<TcpConnection>(config_, pseudo_, local_port, remote_port, NextIss(),
                                            [this](std::span<const u8> s) { EmitToKernel(s); });
        c->Connect(Clock::now());
        return *c;
    }

    TcpConnection* First() {
        return connections.empty() ? nullptr : connections.begin()->second.get();
    }

    /// Runs the bridge until `step()` returns true or the time limit passes.
    bool Run(const std::function<bool()>& step, std::chrono::seconds limit) {
        const auto end = Clock::now() + limit;
        u8 buf[65536];
        while (Clock::now() < end) {
            if (step()) {
                return true;
            }
            auto next = Clock::now() + 5ms;
            for (auto& [_, c] : connections) {
                if (const auto d = c->NextDeadline(); d && *d < next) {
                    next = *d;
                }
            }
            const auto wait = std::chrono::ceil<std::chrono::milliseconds>(
                std::max<Clock::duration>(next - Clock::now(), 0ms));
            pollfd pfd{tun_, POLLIN, 0};
            poll(&pfd, 1, static_cast<int>(wait.count()));
            for (;;) {
                const auto n = read(tun_, buf, sizeof(buf));
                if (n <= 0) {
                    break;
                }
                OnPacket({buf, static_cast<size_t>(n)});
            }
            const auto now = Clock::now();
            for (auto& [_, c] : connections) {
                if (const auto d = c->NextDeadline(); d && *d <= now) {
                    c->OnTimer(now);
                }
            }
        }
        return false;
    }

    int checksum_failures = 0;

private:
    bool Drop() {
        return loss > 0 && std::uniform_real_distribution<double>(0, 1)(rng_) < loss;
    }

    u32 NextIss() {
        const u32 iss = next_iss_;
        next_iss_ += 0x01000193;
        return iss;
    }

    /// Wraps one of our TCP segments in IPv4 and hands it to the kernel.
    void EmitToKernel(std::span<const u8> segment) {
        if (Drop()) {
            return;
        }
        std::vector<u8> packet(20 + segment.size());
        packet[0] = 0x45;
        packet[2] = static_cast<u8>(packet.size() >> 8);
        packet[3] = static_cast<u8>(packet.size());
        packet[6] = 0x40; // don't fragment
        packet[8] = 64;
        packet[9] = IPPROTO_TCP;
        inet_pton(AF_INET, OurIp, packet.data() + 12);
        inet_pton(AF_INET, KernelIp, packet.data() + 16);
        const u16 checksum = IpChecksum(packet.data(), 20);
        packet[10] = static_cast<u8>(checksum >> 8);
        packet[11] = static_cast<u8>(checksum);
        std::copy(segment.begin(), segment.end(), packet.begin() + 20);
        [[maybe_unused]] const auto r = write(tun_, packet.data(), packet.size());
    }

    void OnPacket(std::span<const u8> packet) {
        if (packet.size() < 20 || (packet[0] >> 4) != 4 || packet[9] != IPPROTO_TCP) {
            return;
        }
        const size_t ihl = (packet[0] & 0x0f) * 4;
        const size_t total = (packet[2] << 8) | packet[3];
        if (total > packet.size() || ihl > total || Drop()) {
            return;
        }
        // Verifying the kernel's checksum against our pseudo-header also proves ours agree.
        const auto seg = ParseTcpSegment(packet.subspan(ihl, total - ihl), &pseudo_);
        if (!seg) {
            ++checksum_failures;
            return;
        }
        const auto key = std::pair{seg->dst_port, seg->src_port};
        if (const auto it = connections.find(key);
            it != connections.end() && it->second->State() != TcpState::Closed) {
            it->second->OnSegment(*seg, Clock::now());
            return;
        }
        if ((seg->flags & (TcpFlag::Syn | TcpFlag::Ack | TcpFlag::Rst)) == TcpFlag::Syn &&
            listeners.contains(seg->dst_port)) {
            auto& c = connections[key];
            c = std::make_unique<TcpConnection>(config_, pseudo_, seg->dst_port, seg->src_port,
                                                NextIss(),
                                                [this](std::span<const u8> s) { EmitToKernel(s); });
            c->AcceptSyn(*seg, Clock::now());
            return;
        }
        if (const auto rst = BuildResetFor(pseudo_, *seg)) {
            EmitToKernel(*rst);
        }
    }

    int tun_;
    TcpConfig config_;
    PseudoHeader pseudo_;
    std::mt19937 rng_{42};
    u32 next_iss_ = 0xfffffc00u; // exercise sequence wraparound
};

class NetP2PTcpKernelInterop : public ::testing::Test {
protected:
    void SetUp() override {
        tun_ = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
        if (tun_ < 0) {
            GTEST_SKIP() << "no /dev/net/tun access (needs root or CAP_NET_ADMIN)";
        }
        ifreq ifr{};
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
        std::strncpy(ifr.ifr_name, "shadps4tun0", IFNAMSIZ - 1);
        if (ioctl(tun_, TUNSETIFF, &ifr) < 0) {
            GTEST_SKIP() << "cannot create TUN device: " << std::strerror(errno);
        }
        const int s = socket(AF_INET, SOCK_DGRAM, 0);
        auto* addr = reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr);
        addr->sin_family = AF_INET;
        inet_pton(AF_INET, KernelIp, &addr->sin_addr);
        const bool ok = ioctl(s, SIOCSIFADDR, &ifr) == 0 &&
                        (inet_pton(AF_INET, "255.255.255.0", &addr->sin_addr),
                         ioctl(s, SIOCSIFNETMASK, &ifr) == 0) &&
                        ioctl(s, SIOCGIFFLAGS, &ifr) == 0 &&
                        (ifr.ifr_flags |= IFF_UP | IFF_RUNNING, ioctl(s, SIOCSIFFLAGS, &ifr) == 0);
        close(s);
        if (!ok) {
            GTEST_SKIP() << "cannot configure TUN device: " << std::strerror(errno);
        }
        std::this_thread::sleep_for(100ms);
        host_ = std::make_unique<TunHost>(tun_);
    }

    void TearDown() override {
        host_.reset();
        if (tun_ >= 0) {
            close(tun_); // the non-persistent interface disappears with its fd
        }
    }

    static bool Finished(const TcpConnection* c) {
        return c && (c->State() == TcpState::Closed || c->State() == TcpState::TimeWait);
    }

    /// The kernel connects to us; we echo; the kernel verifies and half-closes.
    void KernelClientToOurEcho(double loss, size_t bytes) {
        host_->loss = loss;
        host_->listeners[7000] = true;
        const auto data = RandomBytes(bytes, 1);
        std::atomic<bool> kernel_done{false};
        std::vector<u8> kernel_got;
        std::string kernel_error;

        std::thread kernel([&] {
            const int s = KernelSocket();
            const auto addr = Address(OurIp, 7000);
            if (connect(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
                kernel_error = std::string("connect: ") + std::strerror(errno);
                kernel_done = true;
                return;
            }
            std::thread writer([&] {
                for (size_t off = 0; off < data.size();) {
                    const auto n = send(s, data.data() + off, data.size() - off, 0);
                    if (n <= 0) {
                        break;
                    }
                    off += static_cast<size_t>(n);
                }
                shutdown(s, SHUT_WR);
            });
            u8 buf[16384];
            for (;;) {
                const auto n = recv(s, buf, sizeof(buf), 0);
                if (n < 0) {
                    kernel_error = std::string("recv: ") + std::strerror(errno);
                }
                if (n <= 0) {
                    break;
                }
                kernel_got.insert(kernel_got.end(), buf, buf + n);
            }
            writer.join();
            close(s);
            kernel_done = true;
        });

        // Our echo: send back whatever arrives, close after the kernel's FIN.
        std::vector<u8> pending;
        bool shut = false;
        host_->Run(
            [&] {
                if (auto* c = host_->First()) {
                    u8 buf[8192];
                    for (;;) {
                        const auto r = c->Recv(buf, false, Clock::now());
                        if (r.bytes == 0) {
                            break;
                        }
                        pending.insert(pending.end(), buf, buf + r.bytes);
                    }
                    while (!pending.empty()) {
                        const auto w = c->Send(pending, Clock::now());
                        if (w.bytes == 0) {
                            break;
                        }
                        pending.erase(pending.begin(), pending.begin() + w.bytes);
                    }
                    if (c->ReceivedFin() && pending.empty() && !shut) {
                        c->Shutdown(Clock::now());
                        shut = true;
                    }
                }
                return kernel_done.load();
            },
            300s);
        kernel.join();
        host_->Run([&] { return Finished(host_->First()); }, 20s); // final ACK exchange

        const auto* c = host_->First();
        EXPECT_TRUE(kernel_error.empty()) << kernel_error;
        EXPECT_TRUE(kernel_got == data)
            << "kernel received " << kernel_got.size() << " of " << data.size();
        ASSERT_TRUE(c);
        EXPECT_EQ(c->PendingError(), Error::Ok);
        EXPECT_TRUE(Finished(c)) << ToString(c->State());
        EXPECT_EQ(host_->checksum_failures, 0);
    }

    /// We connect to a kernel echo server, send, verify the echo and close first.
    void OurClientToKernelEcho(double loss, size_t bytes, int kernel_rcvbuf,
                               std::chrono::milliseconds kernel_stall) {
        host_->loss = loss;
        const int ls = socket(AF_INET, SOCK_STREAM, 0);
        int one = 1;
        setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (kernel_rcvbuf) {
            setsockopt(ls, SOL_SOCKET, SO_RCVBUF, &kernel_rcvbuf, sizeof(kernel_rcvbuf));
        }
        const auto addr = Address(KernelIp, 7001);
        ASSERT_EQ(bind(ls, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)), 0)
            << std::strerror(errno);
        listen(ls, 4);

        std::atomic<bool> kernel_done{false};
        std::thread kernel([&] {
            const int s = accept(ls, nullptr, nullptr);
            timeval tv{120, 0};
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            std::this_thread::sleep_for(kernel_stall); // let our sender hit a zero window
            u8 buf[16384];
            for (;;) {
                const auto n = recv(s, buf, sizeof(buf), 0);
                if (n <= 0) {
                    break;
                }
                for (ssize_t off = 0; off < n;) {
                    const auto w = send(s, buf + off, n - off, 0);
                    if (w <= 0) {
                        break;
                    }
                    off += w;
                }
            }
            close(s);
            kernel_done = true;
        });

        auto& c = host_->Connect(40001, 7001);
        const auto data = RandomBytes(bytes, 2);
        std::vector<u8> got;
        size_t off = 0;
        bool shut = false;
        host_->Run(
            [&] {
                while (off < data.size()) {
                    const auto w = c.Send(std::span(data).subspan(off), Clock::now());
                    if (w.bytes == 0) {
                        break;
                    }
                    off += w.bytes;
                }
                if (off == data.size() && !shut && c.State() == TcpState::Established) {
                    c.Shutdown(Clock::now());
                    shut = true;
                }
                u8 buf[8192];
                for (;;) {
                    const auto r = c.Recv(buf, false, Clock::now());
                    if (r.bytes == 0) {
                        break;
                    }
                    got.insert(got.end(), buf, buf + r.bytes);
                }
                return (c.ReceivedFin() && kernel_done && Finished(&c)) ||
                       c.PendingError() != Error::Ok;
            },
            300s);
        if (!kernel_done) {
            shutdown(ls, SHUT_RDWR);
            c.Abort();
        }
        kernel.join();
        close(ls);

        EXPECT_TRUE(got == data) << "received " << got.size() << " of " << data.size();
        EXPECT_EQ(c.PendingError(), Error::Ok);
        EXPECT_EQ(host_->checksum_failures, 0);
    }

    int tun_ = -1;
    std::unique_ptr<TunHost> host_;
};

TEST_F(NetP2PTcpKernelInterop, KernelClientToOurServer) {
    KernelClientToOurEcho(0.0, 1 << 20);
}

TEST_F(NetP2PTcpKernelInterop, KernelClientToOurServerWithLoss) {
    KernelClientToOurEcho(0.05, 256 << 10);
}

TEST_F(NetP2PTcpKernelInterop, OurClientToKernelServer) {
    OurClientToKernelEcho(0.0, 1 << 20, 0, 0ms);
}

TEST_F(NetP2PTcpKernelInterop, OurClientToKernelServerWithLoss) {
    OurClientToKernelEcho(0.05, 256 << 10, 0, 0ms);
}

TEST_F(NetP2PTcpKernelInterop, ZeroWindowProbing) {
    // The kernel reads nothing for 3 s with a tiny receive buffer: our persist timer has to
    // probe the closed window until it reopens.
    OurClientToKernelEcho(0.0, 256 << 10, 4096, 3000ms);
}

TEST_F(NetP2PTcpKernelInterop, OurConnectToClosedPortIsRefused) {
    auto& c = host_->Connect(40002, 7009); // nothing listens on 7009
    host_->Run([&] { return c.State() == TcpState::Closed; }, 20s);
    EXPECT_EQ(c.PendingError(), Error::ConnRefused);
}

TEST_F(NetP2PTcpKernelInterop, KernelConnectToOurClosedPortIsRefused) {
    std::atomic<int> result{0};
    std::atomic<bool> done{false};
    std::thread kernel([&] {
        const int s = KernelSocket();
        const auto addr = Address(OurIp, 7010);
        result =
            connect(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0 ? 0 : errno;
        close(s);
        done = true;
    });
    host_->Run([&] { return done.load(); }, 20s);
    kernel.join();
    EXPECT_EQ(result, ECONNREFUSED); // we answered the SYN with a RST
}

} // namespace

#else

TEST(NetP2PTcpKernelInterop, Unsupported) {
    GTEST_SKIP() << "the TUN interop tests run on Linux only";
}

#endif
