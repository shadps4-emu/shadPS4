// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The STREAM_P2P TCP state machine: wire format, and two connections over a simulated link with
// loss, duplication, reordering and jitter on a virtual clock (so the tests are fast and
// deterministic).

#include <cstdlib>
#include <deque>
#include <random>
#include <string>
#include <queue>

#include <gtest/gtest.h>

#include "core/net/p2p_tcp.h"

using namespace Core::Net;
using namespace Core::Net::P2P;
using namespace std::chrono_literals;

namespace {

PseudoHeader Pseudo(u8 local, u8 remote) {
    PseudoHeader p;
    p.family = AF_INET;
    p.local = {10, 0, 0, local};
    p.remote = {10, 0, 0, remote};
    return p;
}

std::vector<u8> RandomBytes(size_t n, u64 seed) {
    std::mt19937_64 rng(seed);
    std::vector<u8> v(n);
    for (auto& b : v) {
        b = static_cast<u8>(rng());
    }
    return v;
}

// ---------------------------------------------------------------------------------------------
// Wire format
// ---------------------------------------------------------------------------------------------

TEST(NetP2PTcpWire, SequenceArithmeticWraps) {
    EXPECT_TRUE(SeqLt(0xFFFFFFF0u, 0x10u));
    EXPECT_TRUE(SeqGt(0x10u, 0xFFFFFFF0u));
    EXPECT_TRUE(SeqLe(5u, 5u));
    EXPECT_FALSE(SeqLt(5u, 5u));
}

TEST(NetP2PTcpWire, BuildParseRoundTripWithChecksum) {
    const u8 payload[] = {1, 2, 3, 4, 5};
    const auto seg = BuildTcpSegment(Pseudo(1, 2), 1234, 80, 1000, 2000,
                                     TcpFlag::Ack | TcpFlag::Psh, 4096, u16{1200}, payload);
    const auto receiver = Pseudo(2, 1);
    const auto parsed = ParseTcpSegment(seg, &receiver);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->src_port, 1234);
    EXPECT_EQ(parsed->dst_port, 80);
    EXPECT_EQ(parsed->seq, 1000u);
    EXPECT_EQ(parsed->ack, 2000u);
    EXPECT_EQ(parsed->flags, TcpFlag::Ack | TcpFlag::Psh);
    EXPECT_EQ(parsed->window, 4096);
    EXPECT_EQ(parsed->mss, 1200);
    EXPECT_EQ(std::vector<u8>(parsed->payload.begin(), parsed->payload.end()),
              std::vector<u8>(std::begin(payload), std::end(payload)));

    // A flipped bit, or the wrong pseudo-header, fails verification.
    auto corrupt = seg;
    corrupt.back() ^= 1;
    EXPECT_FALSE(ParseTcpSegment(corrupt, &receiver));
    const auto wrong = Pseudo(2, 9);
    EXPECT_FALSE(ParseTcpSegment(seg, &wrong));
    EXPECT_TRUE(ParseTcpSegment(corrupt)); // no pseudo-header: no verification
}

TEST(NetP2PTcpWire, KnownChecksum) {
    // SYN 10.0.0.1:1234 -> 10.0.0.2:80, seq 1, window 1024, no options; checksum computed with
    // an independent implementation (RFC 1071 one's complement sum, Python).
    const auto seg =
        BuildTcpSegment(Pseudo(1, 2), 1234, 80, 1, 0, TcpFlag::Syn, 1024, std::nullopt, {});
    EXPECT_EQ((seg[16] << 8) | seg[17], 0x92BD);
}

TEST(NetP2PTcpWire, SkipsUnknownOptions) {
    // SYN carrying MSS, SACK-permitted, timestamps, NOP and window scale, as Linux sends.
    std::vector<u8> seg(40, 0);
    seg[12] = 10 << 4;
    seg[13] = TcpFlag::Syn;
    const u8 options[] = {2, 4, 0x05, 0xB4, 4, 2, 8, 10, 0, 0, 0, 1, 0, 0, 0, 0, 1, 3, 3, 7};
    std::copy(std::begin(options), std::end(options), seg.begin() + 20);
    const auto parsed = ParseTcpSegment(seg);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->mss, 1460);
    EXPECT_TRUE(parsed->payload.empty());
}

TEST(NetP2PTcpWire, RejectsMalformed) {
    std::vector<u8> seg(20, 0);
    seg[12] = 4 << 4; // data offset below 20 bytes
    EXPECT_FALSE(ParseTcpSegment(seg));
    seg[12] = 15 << 4; // data offset beyond the segment
    EXPECT_FALSE(ParseTcpSegment(seg));
    EXPECT_FALSE(ParseTcpSegment(std::vector<u8>(19, 0)));
}

TEST(NetP2PTcpWire, ResetForUnknownPort) {
    const auto p = Pseudo(1, 2);
    const u8 data[10]{};
    // With ACK: RST takes its sequence number from the ACK.
    TcpSegment with_ack{5000, 80, 100, 777, TcpFlag::Ack, 100, std::nullopt, data};
    auto rst = ParseTcpSegment(*BuildResetFor(p, with_ack));
    EXPECT_EQ(rst->flags, TcpFlag::Rst);
    EXPECT_EQ(rst->seq, 777u);
    EXPECT_EQ(rst->src_port, 80);
    EXPECT_EQ(rst->dst_port, 5000);
    // Without ACK (a SYN): RST|ACK acknowledging what the segment occupied.
    TcpSegment syn{5000, 80, 100, 0, TcpFlag::Syn, 100, std::nullopt, {}};
    rst = ParseTcpSegment(*BuildResetFor(p, syn));
    EXPECT_EQ(rst->flags, TcpFlag::Rst | TcpFlag::Ack);
    EXPECT_EQ(rst->ack, 101u);
    // Never answer a RST with a RST.
    TcpSegment reset{5000, 80, 100, 0, TcpFlag::Rst, 0, std::nullopt, {}};
    EXPECT_FALSE(BuildResetFor(p, reset));
}

// ---------------------------------------------------------------------------------------------
// Simulated link
// ---------------------------------------------------------------------------------------------

struct LinkParams {
    double loss = 0.0;
    double duplicate = 0.0;
    std::chrono::milliseconds delay{20};
    std::chrono::milliseconds jitter{0}; // jitter larger than packet spacing reorders
};

class Sim {
public:
    Sim(u64 seed, LinkParams link) : rng_(seed), link_(link) {}

    Clock::time_point now{};

    void Transmit(int to, std::span<const u8> bytes) {
        if (std::getenv("SHADPS4_NET_SIMLOG")) { // keep a trace, printed on failure
            const auto seg = ParseTcpSegment(bytes);
            char line[160];
            std::snprintf(line, sizeof(line), "%8.3f %s flags=%02x seq=%u ack=%u len=%zu win=%u",
                          std::chrono::duration<double>(now.time_since_epoch()).count(),
                          to ? "A->B" : "B->A", seg->flags, seg->seq, seg->ack, seg->payload.size(),
                          seg->window);
            log_.push_back(line);
            if (log_.size() > 60) {
                log_.pop_front();
            }
        }
        std::uniform_real_distribution<double> u(0, 1);
        const int copies = u(rng_) < link_.loss ? 0 : (u(rng_) < link_.duplicate ? 2 : 1);
        for (int i = 0; i < copies; ++i) {
            const auto jitter = link_.jitter.count()
                                    ? std::chrono::milliseconds(rng_() % link_.jitter.count())
                                    : 0ms;
            wire_.push({now + link_.delay + jitter, to, {bytes.begin(), bytes.end()}, order_++});
        }
    }

    std::optional<Clock::time_point> NextDelivery() const {
        return wire_.empty() ? std::nullopt : std::optional{wire_.top().deliver_at};
    }

    /// Pops the next packet due by `now`.
    std::optional<std::pair<int, std::vector<u8>>> Pop() {
        if (wire_.empty() || wire_.top().deliver_at > now) {
            return std::nullopt;
        }
        auto p = wire_.top();
        wire_.pop();
        return std::pair{p.to, std::move(p.bytes)};
    }

    std::string Log() const {
        std::string out;
        for (const auto& line : log_) {
            out += "    " + line + "\n";
        }
        return out;
    }

private:
    struct Packet {
        Clock::time_point deliver_at;
        int to; // 0 = A, 1 = B
        std::vector<u8> bytes;
        u64 order;
        bool operator>(const Packet& o) const {
            return deliver_at != o.deliver_at ? deliver_at > o.deliver_at : order > o.order;
        }
    };

    std::mt19937_64 rng_;
    LinkParams link_;
    std::priority_queue<Packet, std::vector<Packet>, std::greater<>> wire_;
    u64 order_ = 0;
    std::deque<std::string> log_;
};

struct TransferResult {
    std::vector<u8> a_sent, b_sent, a_got, b_got;
    TcpState a_state, b_state;
    Error a_error, b_error;
    bool b_created;
};

TransferResult RunTransfer(u64 seed, LinkParams link, size_t bytes, Clock::duration reader_stall,
                           std::string* log) {
    Sim sim(seed, link);
    TcpConfig config;
    const auto a_pseudo = Pseudo(1, 2);
    const auto b_pseudo = Pseudo(2, 1);
    std::optional<TcpConnection> b;
    // A's initial sequence number sits just below 2^32 so the transfer crosses the wrap.
    TcpConnection a(config, a_pseudo, 5000, 6000, 0xfffff000u,
                    [&](std::span<const u8> s) { sim.Transmit(1, s); });
    a.Connect(sim.now);

    TransferResult r;
    r.a_sent = RandomBytes(bytes, seed * 2 + 1);
    r.b_sent = RandomBytes(bytes, seed * 2 + 2);
    size_t a_off = 0, b_off = 0;
    bool a_shut = false, b_shut = false;
    const auto start = sim.now;

    auto pump = [&](TcpConnection& c, const std::vector<u8>& out, size_t& off, std::vector<u8>& in,
                    bool& shut, bool may_read) {
        while (off < out.size()) {
            const auto w = c.Send(std::span(out).subspan(off), sim.now);
            if (w.bytes == 0) {
                break;
            }
            off += w.bytes;
        }
        if (off == out.size() && !shut &&
            (c.State() == TcpState::Established || c.State() == TcpState::CloseWait)) {
            c.Shutdown(sim.now);
            shut = true;
        }
        if (may_read) {
            u8 buf[4096];
            for (;;) {
                const auto got = c.Recv(buf, false, sim.now);
                if (got.bytes == 0) {
                    break;
                }
                in.insert(in.end(), buf, buf + got.bytes);
            }
        }
    };
    const auto done = [](const TcpConnection& c) {
        return c.State() == TcpState::Closed || c.State() == TcpState::TimeWait;
    };

    while (sim.now < start + 600s) {
        pump(a, r.a_sent, a_off, r.a_got, a_shut, true);
        if (b) {
            pump(*b, r.b_sent, b_off, r.b_got, b_shut, sim.now - start >= reader_stall);
        }
        if (done(a) && b && done(*b) && r.a_got.size() == bytes && r.b_got.size() == bytes) {
            break;
        }

        // Advance the virtual clock to the next packet or timer.
        std::optional<Clock::time_point> next;
        const auto consider = [&](std::optional<Clock::time_point> t) {
            if (t && (!next || *t < *next)) {
                next = t;
            }
        };
        consider(sim.NextDelivery());
        consider(a.NextDeadline());
        if (b) {
            consider(b->NextDeadline());
            if (sim.now - start < reader_stall) {
                consider(start + reader_stall);
            }
        }
        if (!next) {
            break;
        }
        sim.now = std::max(sim.now, *next);

        while (auto packet = sim.Pop()) {
            const auto [to, bytes_in] = std::move(*packet);
            const auto& pseudo = to == 0 ? a_pseudo : b_pseudo;
            const auto seg = ParseTcpSegment(bytes_in, &pseudo);
            if (!seg) {
                ADD_FAILURE() << "segment failed checksum verification";
                continue;
            }
            TcpConnection* target = to == 0 ? &a : (b ? &*b : nullptr);
            if (target && target->State() == TcpState::Closed) {
                if (auto rst = BuildResetFor(pseudo, *seg)) {
                    sim.Transmit(to ^ 1, *rst);
                }
                continue;
            }
            if (target) {
                target->OnSegment(*seg, sim.now);
            } else if ((seg->flags & (TcpFlag::Syn | TcpFlag::Ack)) == TcpFlag::Syn) {
                b.emplace(config, b_pseudo, 6000, 5000, 0x12345678u,
                          [&](std::span<const u8> s) { sim.Transmit(0, s); });
                b->AcceptSyn(*seg, sim.now);
            }
        }
        a.OnTimer(sim.now);
        if (b) {
            b->OnTimer(sim.now);
        }
    }

    r.a_state = a.State();
    r.a_error = a.PendingError();
    r.b_created = b.has_value();
    r.b_state = b ? b->State() : TcpState::Closed;
    r.b_error = b ? b->PendingError() : Error::Ok;
    if (log) {
        *log = sim.Log();
    }
    return r;
}

struct Scenario {
    const char* name;
    LinkParams link;
    Clock::duration reader_stall;
};

class NetP2PTcpSim : public ::testing::TestWithParam<Scenario> {};

TEST_P(NetP2PTcpSim, BidirectionalTransferAndClose) {
    constexpr size_t kBytes = 300 * 1024;
    for (u64 seed = 1; seed <= 20; ++seed) {
        std::string log;
        const auto r = RunTransfer(seed, GetParam().link, kBytes, GetParam().reader_stall, &log);
        ASSERT_TRUE(r.b_created) << "seed " << seed;
        ASSERT_TRUE(r.a_got == r.b_sent && r.b_got == r.a_sent)
            << "seed " << seed << ": a got " << r.a_got.size() << ", b got " << r.b_got.size()
            << " of " << kBytes << "; a " << ToString(r.a_state) << " b " << ToString(r.b_state)
            << "\n"
            << log << "(set SHADPS4_NET_SIMLOG=1 for a packet trace)";
        EXPECT_EQ(r.a_error, Error::Ok) << "seed " << seed;
        EXPECT_EQ(r.b_error, Error::Ok) << "seed " << seed;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Links, NetP2PTcpSim,
    ::testing::Values(
        Scenario{"Clean", {}, 0s}, Scenario{"Loss5", {.loss = 0.05}, 0s},
        Scenario{"Loss20", {.loss = 0.20}, 0s},
        Scenario{"ReorderDuplicate", {.duplicate = 0.05, .jitter = 30ms}, 0s},
        Scenario{"Loss10ReorderDuplicate", {.loss = 0.10, .duplicate = 0.05, .jitter = 30ms}, 0s},
        Scenario{"ZeroWindow", {.loss = 0.02}, 3s}),
    [](const auto& info) { return std::string(info.param.name); });

TEST(NetP2PTcp, ConnectTimesOutOnDeadLink) {
    Clock::time_point now{};
    TcpConfig config;
    int syns = 0;
    TcpConnection a(config, Pseudo(1, 2), 5000, 6000, 1, [&](std::span<const u8>) { ++syns; });
    a.Connect(now);
    while (const auto next = a.NextDeadline()) {
        now = *next;
        a.OnTimer(now);
    }
    EXPECT_EQ(a.State(), TcpState::Closed);
    EXPECT_EQ(a.PendingError(), Error::TimedOut);
    EXPECT_EQ(syns, 1 + config.syn_retries);
    EXPECT_GE(std::chrono::duration<double>(now.time_since_epoch()).count(), 60.0);
}

TEST(NetP2PTcp, AppIoBeforeAndAfterConnection) {
    Clock::time_point now{};
    TcpConnection a(TcpConfig{}, Pseudo(1, 2), 5000, 6000, 1, [](std::span<const u8>) {});
    a.Connect(now);
    const u8 byte = 1;
    u8 buf[4];
    EXPECT_EQ(a.Send({&byte, 1}, now).error, Error::WouldBlock); // still connecting
    EXPECT_EQ(a.Recv(buf, false, now).error, Error::WouldBlock);
    a.Abort();
    EXPECT_EQ(a.State(), TcpState::Closed);
    EXPECT_EQ(a.Send({&byte, 1}, now).error, Error::NotConn);
}

} // namespace
