// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "core/net/p2p_tcp.h"

namespace Core::Net::P2P {

using namespace TcpFlag;

namespace {

u16 Read16(const u8* p) {
    return static_cast<u16>((p[0] << 8) | p[1]);
}

u32 Read32(const u8* p) {
    return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) |
           (static_cast<u32>(p[2]) << 8) | p[3];
}

void Write16(u8* p, u16 v) {
    p[0] = static_cast<u8>(v >> 8);
    p[1] = static_cast<u8>(v);
}

void Write32(u8* p, u32 v) {
    p[0] = static_cast<u8>(v >> 24);
    p[1] = static_cast<u8>(v >> 16);
    p[2] = static_cast<u8>(v >> 8);
    p[3] = static_cast<u8>(v);
}

u64 SumWords(u64 sum, const u8* data, size_t size) {
    size_t i = 0;
    for (; i + 1 < size; i += 2) {
        sum += Read16(data + i);
    }
    if (i < size) {
        sum += static_cast<u64>(data[i]) << 8;
    }
    return sum;
}

} // namespace

const char* ToString(TcpState state) {
    switch (state) {
    case TcpState::Closed:
        return "Closed";
    case TcpState::SynSent:
        return "SynSent";
    case TcpState::SynReceived:
        return "SynReceived";
    case TcpState::Established:
        return "Established";
    case TcpState::FinWait1:
        return "FinWait1";
    case TcpState::FinWait2:
        return "FinWait2";
    case TcpState::CloseWait:
        return "CloseWait";
    case TcpState::Closing:
        return "Closing";
    case TcpState::LastAck:
        return "LastAck";
    case TcpState::TimeWait:
        return "TimeWait";
    }
    return "?";
}

u16 TcpChecksum(int family, const u8* src_addr, const u8* dst_addr, std::span<const u8> segment) {
    const size_t address_size = family == AF_INET6 ? 16 : 4;
    u64 sum = 0;
    sum = SumWords(sum, src_addr, address_size);
    sum = SumWords(sum, dst_addr, address_size);
    const u64 length = segment.size();
    sum += (length >> 16) + (length & 0xffff);
    sum += 6; // IPPROTO_TCP
    sum = SumWords(sum, segment.data(), segment.size());
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return static_cast<u16>(~sum);
}

std::optional<TcpSegment> ParseTcpSegment(std::span<const u8> bytes, const PseudoHeader* pseudo) {
    if (bytes.size() < 20) {
        return std::nullopt;
    }
    const u8* b = bytes.data();
    const size_t header_size = static_cast<size_t>(b[12] >> 4) * 4;
    if (header_size < 20 || header_size > bytes.size()) {
        return std::nullopt;
    }
    if (pseudo != nullptr &&
        TcpChecksum(pseudo->family, pseudo->remote.data(), pseudo->local.data(), bytes) != 0) {
        return std::nullopt;
    }
    TcpSegment seg{};
    seg.src_port = Read16(b);
    seg.dst_port = Read16(b + 2);
    seg.seq = Read32(b + 4);
    seg.ack = Read32(b + 8);
    seg.flags = b[13];
    seg.window = Read16(b + 14);
    for (size_t i = 20; i < header_size;) {
        const u8 kind = b[i];
        if (kind == 0) {
            break;
        }
        if (kind == 1) {
            ++i;
            continue;
        }
        if (i + 1 >= header_size) {
            break;
        }
        const u8 length = b[i + 1];
        if (length < 2 || i + length > header_size) {
            break;
        }
        if (kind == 2 && length == 4) {
            seg.mss = Read16(b + i + 2);
        }
        i += length;
    }
    seg.payload = bytes.subspan(header_size);
    return seg;
}

std::vector<u8> BuildTcpSegment(const PseudoHeader& pseudo, u16 src_port, u16 dst_port, u32 seq,
                                u32 ack, u8 flags, u16 window, std::optional<u16> mss,
                                std::span<const u8> payload) {
    const size_t header_size = mss ? 24 : 20;
    std::vector<u8> s(header_size + payload.size());
    Write16(s.data(), src_port);
    Write16(s.data() + 2, dst_port);
    Write32(s.data() + 4, seq);
    Write32(s.data() + 8, ack);
    s[12] = static_cast<u8>((header_size / 4) << 4);
    s[13] = flags;
    Write16(s.data() + 14, window);
    if (mss) {
        s[20] = 2;
        s[21] = 4;
        Write16(s.data() + 22, *mss);
    }
    std::copy(payload.begin(), payload.end(), s.begin() + header_size);
    Write16(s.data() + 16,
            TcpChecksum(pseudo.family, pseudo.local.data(), pseudo.remote.data(), s));
    return s;
}

std::optional<std::vector<u8>> BuildResetFor(const PseudoHeader& pseudo, const TcpSegment& seg) {
    if (seg.flags & Rst) {
        return std::nullopt;
    }
    if (seg.flags & Ack) {
        return BuildTcpSegment(pseudo, seg.dst_port, seg.src_port, seg.ack, 0, Rst, 0, std::nullopt,
                               {});
    }
    const u32 length = static_cast<u32>(seg.payload.size()) + ((seg.flags & Syn) ? 1 : 0) +
                       ((seg.flags & Fin) ? 1 : 0);
    return BuildTcpSegment(pseudo, seg.dst_port, seg.src_port, 0, seg.seq + length, Rst | Ack, 0,
                           std::nullopt, {});
}

TcpConnection::TcpConnection(const TcpConfig& config, const PseudoHeader& pseudo, u16 local_port,
                             u16 remote_port, u32 iss, Emit emit)
    : config_(config), pseudo_(pseudo), local_port_(local_port), remote_port_(remote_port),
      emit_(std::move(emit)), iss_(iss), snd_una_(iss), snd_nxt_(iss), snd_max_(iss),
      buf_seq_(iss + 1), rto_(config.initial_rto) {}

bool TcpConnection::Synchronized() const {
    switch (state_) {
    case TcpState::Closed:
    case TcpState::SynSent:
    case TcpState::SynReceived:
        return false;
    default:
        return true;
    }
}

u16 TcpConnection::ReceiveWindow() const {
    const size_t used = recv_buf_.size();
    if (used >= config_.recv_buffer) {
        return 0;
    }
    return static_cast<u16>(std::min<size_t>(config_.recv_buffer - used, 65535));
}

u32 TcpConnection::FinSeq() const {
    return buf_seq_ + static_cast<u32>(send_buf_.size());
}

bool TcpConnection::FinSent() const {
    return fin_queued_ && SeqGt(snd_max_, FinSeq());
}

bool TcpConnection::FinAcked() const {
    return fin_queued_ && SeqGt(snd_una_, FinSeq());
}

size_t TcpConnection::SendSpace() const {
    return send_buf_.size() >= config_.send_buffer ? 0 : config_.send_buffer - send_buf_.size();
}

bool TcpConnection::Readable() const {
    return !recv_buf_.empty() || fin_received_ || error_ != Error::Ok || state_ == TcpState::Closed;
}

bool TcpConnection::Writable() const {
    if (error_ != Error::Ok || state_ == TcpState::Closed) {
        return true; // wake waiters so they see the error
    }
    return (state_ == TcpState::Established || state_ == TcpState::CloseWait) && !fin_queued_ &&
           SendSpace() > 0;
}

void TcpConnection::EmitSegment(u32 seq, u8 flags, std::span<const u8> payload, bool with_mss) {
    const u16 window = ReceiveWindow();
    const u32 ack = (flags & Ack) ? rcv_nxt_ : 0;
    auto segment =
        BuildTcpSegment(pseudo_, local_port_, remote_port_, seq, ack, flags, window,
                        with_mss ? std::optional<u16>{config_.mss} : std::nullopt, payload);
    if (flags & Ack) {
        last_advertised_window_ = window;
        unacked_segments_ = 0;
        delayed_ack_deadline_.reset();
    }
    emit_(segment);
}

void TcpConnection::SendAck() {
    // Not snd_nxt_, which is rewound after a timeout. BSD drops ACKs with an old seq.
    EmitSegment(snd_max_, Ack, {});
}

void TcpConnection::EmitReset(u32 seq) {
    emit_(BuildTcpSegment(pseudo_, local_port_, remote_port_, seq, 0, Rst, 0, std::nullopt, {}));
}

void TcpConnection::SendData(u32 seq, u32 len) {
    std::vector<u8> payload(len);
    const auto begin = send_buf_.begin() + static_cast<std::ptrdiff_t>(seq - buf_seq_);
    std::copy(begin, begin + len, payload.begin());
    const bool last = seq + len == FinSeq();
    EmitSegment(seq, Ack | (last ? Psh : 0), payload);
}

void TcpConnection::ArmRetransmit(Clock::time_point now) {
    if (!retransmit_deadline_) {
        retransmit_deadline_ = now + rto_;
    }
}

void TcpConnection::TrySend(Clock::time_point now) {
    switch (state_) {
    case TcpState::Established:
    case TcpState::CloseWait:
    case TcpState::FinWait1:
    case TcpState::Closing:
    case TcpState::LastAck:
        break;
    default:
        return;
    }
    for (;;) {
        if (FinSent() && snd_nxt_ == FinSeq() + 1) {
            return; // FIN in flight
        }
        const u32 offset = snd_nxt_ - buf_seq_;
        const u32 available =
            offset < send_buf_.size() ? static_cast<u32>(send_buf_.size()) - offset : 0;
        const u32 flight = snd_nxt_ - snd_una_;
        const u32 window = std::min(snd_wnd_, cwnd_);
        const u32 usable = window > flight ? window - flight : 0;
        const u32 len = std::min({available, usable, static_cast<u32>(snd_mss_)});

        if (len == 0) {
            if (available == 0 && fin_queued_ && snd_nxt_ == FinSeq()) {
                EmitSegment(snd_nxt_, Fin | Ack, {});
                ++snd_nxt_;
                if (SeqGt(snd_nxt_, snd_max_)) {
                    snd_max_ = snd_nxt_;
                }
                ArmRetransmit(now);
            } else if (available > 0 && snd_wnd_ == 0 && snd_max_ == snd_una_ &&
                       !persist_deadline_) {
                persist_interval_ = rto_;
                persist_deadline_ = now + persist_interval_;
            }
            return;
        }

        SendData(snd_nxt_, len);
        if (!rtt_active_ && snd_nxt_ == snd_max_) {
            rtt_active_ = true; // don't time retransmits
            rtt_seq_ = snd_nxt_;
            rtt_start_ = now;
        }
        snd_nxt_ += len;
        if (SeqGt(snd_nxt_, snd_max_)) {
            snd_max_ = snd_nxt_;
        }
        ArmRetransmit(now);
    }
}

void TcpConnection::RetransmitFirst() {
    ++retransmissions_;
    rtt_active_ = false;
    const u32 offset = snd_una_ - buf_seq_;
    if (offset < send_buf_.size()) {
        const u32 len = std::min<u32>(static_cast<u32>(send_buf_.size()) - offset, snd_mss_);
        SendData(snd_una_, len);
    } else if (FinSent()) {
        EmitSegment(FinSeq(), Fin | Ack, {});
    }
}

void TcpConnection::Connect(Clock::time_point now) {
    state_ = TcpState::SynSent;
    snd_nxt_ = snd_max_ = iss_ + 1;
    EmitSegment(iss_, Syn, {}, true);
    rtt_active_ = true;
    rtt_seq_ = iss_;
    rtt_start_ = now;
    ArmRetransmit(now);
}

void TcpConnection::AcceptSyn(const TcpSegment& syn, Clock::time_point now) {
    irs_ = syn.seq;
    rcv_nxt_ = irs_ + 1;
    snd_mss_ = std::min<u16>(syn.mss.value_or(536), config_.mss);
    cwnd_ = std::min<u32>(4u * snd_mss_, std::max<u32>(2u * snd_mss_, 4380));
    snd_wnd_ = syn.window;
    snd_wl1_ = syn.seq;
    snd_wl2_ = iss_;
    state_ = TcpState::SynReceived;
    snd_nxt_ = snd_max_ = iss_ + 1;
    EmitSegment(iss_, Syn | Ack, {}, true);
    rtt_active_ = true;
    rtt_seq_ = iss_;
    rtt_start_ = now;
    ArmRetransmit(now);
}

void TcpConnection::HandleSynSent(const TcpSegment& seg, Clock::time_point now) {
    const bool ack_ok = SeqGt(seg.ack, iss_) && SeqLe(seg.ack, snd_max_);
    if ((seg.flags & Ack) && !ack_ok) {
        if (!(seg.flags & Rst)) {
            EmitReset(seg.ack);
        }
        return;
    }
    if (seg.flags & Rst) {
        if (seg.flags & Ack) {
            Terminate(Error::ConnRefused);
        }
        return;
    }
    if (!(seg.flags & Syn)) {
        return;
    }
    irs_ = seg.seq;
    rcv_nxt_ = irs_ + 1;
    snd_mss_ = std::min<u16>(seg.mss.value_or(536), config_.mss);
    cwnd_ = std::min<u32>(4u * snd_mss_, std::max<u32>(2u * snd_mss_, 4380));
    snd_wnd_ = seg.window;
    snd_wl1_ = seg.seq;
    snd_wl2_ = seg.ack;
    if (seg.flags & Ack) {
        snd_una_ = seg.ack;
        if (rtt_active_ && SeqGt(seg.ack, rtt_seq_)) {
            SampleRtt(now - rtt_start_);
            rtt_active_ = false;
        }
        retries_ = 0;
        retransmit_deadline_.reset();
        state_ = TcpState::Established;
        SendAck();
        TrySend(now);
    } else {
        state_ = TcpState::SynReceived;
        EmitSegment(iss_, Syn | Ack, {}, true);
    }
}

bool TcpConnection::Acceptable(u32 seq, u32 len) const {
    const u32 window = ReceiveWindow();
    const auto in_window = [&](u32 s) { return SeqLe(rcv_nxt_, s) && SeqLt(s, rcv_nxt_ + window); };
    if (len == 0) {
        return window == 0 ? seq == rcv_nxt_ : in_window(seq);
    }
    if (window == 0) {
        return false;
    }
    return in_window(seq) || in_window(seq + len - 1);
}

void TcpConnection::OnSegment(const TcpSegment& seg, Clock::time_point now) {
    switch (state_) {
    case TcpState::Closed:
        return;
    case TcpState::SynSent:
        HandleSynSent(seg, now);
        return;
    case TcpState::TimeWait:
        if ((seg.flags & Fin) && !(seg.flags & Rst)) {
            SendAck();
            time_wait_deadline_ = now + config_.time_wait;
        }
        return;
    default:
        break;
    }

    // Peer resent its SYN, so our SYN-ACK was lost.
    if (state_ == TcpState::SynReceived && (seg.flags & (Syn | Ack | Rst)) == Syn &&
        seg.seq == irs_) {
        ++retransmissions_;
        EmitSegment(iss_, Syn | Ack, {}, true);
        return;
    }

    const u32 length = static_cast<u32>(seg.payload.size()) + ((seg.flags & Syn) ? 1 : 0) +
                       ((seg.flags & Fin) ? 1 : 0);
    bool ack_only = false;
    if (!Acceptable(seg.seq, length)) {
        if (ReceiveWindow() == 0 && seg.seq == rcv_nxt_ && (seg.flags & Ack) &&
            !(seg.flags & (Rst | Syn))) {
            ack_only = true;
        } else {
            if (!(seg.flags & Rst)) {
                SendAck();
            }
            return;
        }
    }

    if (seg.flags & Rst) {
        if (seg.seq == rcv_nxt_) {
            const bool closing = state_ == TcpState::Closing || state_ == TcpState::LastAck;
            Terminate(closing                           ? Error::Ok
                      : state_ == TcpState::SynReceived ? Error::ConnRefused
                                                        : Error::ConnReset);
        } else {
            SendAck();
        }
        return;
    }
    if (seg.flags & Syn) {
        SendAck();
        return;
    }
    if (!(seg.flags & Ack)) {
        return;
    }

    if (state_ == TcpState::SynReceived) {
        if (!(SeqGt(seg.ack, snd_una_) && SeqLe(seg.ack, snd_max_))) {
            EmitReset(seg.ack);
            return;
        }
        state_ = TcpState::Established;
        snd_wnd_ = seg.window;
        snd_wl1_ = seg.seq;
        snd_wl2_ = seg.ack;
    }

    if (!ProcessAck(seg, now)) {
        return;
    }
    if (ack_only) {
        SendAck(); // window still closed
        return;
    }
    ProcessData(seg, now);
    TrySend(now);
}

bool TcpConnection::ProcessAck(const TcpSegment& seg, Clock::time_point now) {
    if (SeqGt(seg.ack, snd_max_)) {
        SendAck(); // acks unsent data
        return false;
    }
    if (SeqGt(seg.ack, snd_una_)) {
        OnNewAck(seg.ack, now);
    } else if (seg.ack == snd_una_ && seg.payload.empty() && !(seg.flags & (Syn | Fin)) &&
               seg.window == snd_wnd_ && snd_max_ != snd_una_) {
        OnDuplicateAck();
    }

    if (SeqLt(snd_wl1_, seg.seq) || (snd_wl1_ == seg.seq && SeqLe(snd_wl2_, seg.ack))) {
        snd_wnd_ = seg.window;
        snd_wl1_ = seg.seq;
        snd_wl2_ = seg.ack;
        if (snd_wnd_ > 0) {
            persist_deadline_.reset();
        }
    }

    if (FinAcked()) {
        switch (state_) {
        case TcpState::FinWait1:
            state_ = TcpState::FinWait2;
            break;
        case TcpState::Closing:
            EnterTimeWait(now);
            return false;
        case TcpState::LastAck:
            state_ = TcpState::Closed;
            ClearTimers();
            return false;
        default:
            break;
        }
    }
    return true;
}

void TcpConnection::OnNewAck(u32 ack, Clock::time_point now) {
    const u32 acked = ack - snd_una_;
    const u32 data_acked = SeqGt(ack, buf_seq_)
                               ? std::min<u32>(ack - buf_seq_, static_cast<u32>(send_buf_.size()))
                               : 0;
    send_buf_.erase(send_buf_.begin(), send_buf_.begin() + data_acked);
    buf_seq_ += data_acked;
    snd_una_ = ack;
    if (SeqLt(snd_nxt_, snd_una_)) {
        snd_nxt_ = snd_una_;
    }

    if (rtt_active_ && SeqGt(ack, rtt_seq_)) {
        SampleRtt(now - rtt_start_);
        rtt_active_ = false;
    }
    retries_ = 0;

    if (in_recovery_) {
        if (SeqGe(ack, recover_)) {
            in_recovery_ = false;
            cwnd_ = ssthresh_;
        } else {
            // NewReno partial ACK
            RetransmitFirst();
            cwnd_ = cwnd_ > acked ? cwnd_ - acked + snd_mss_ : snd_mss_;
        }
    } else if (cwnd_ < ssthresh_) {
        cwnd_ += std::min<u32>(acked, snd_mss_); // slow start
    } else {
        cwnd_ += std::max<u32>(1, snd_mss_ * snd_mss_ / std::max<u32>(cwnd_, 1));
    }
    cwnd_ = std::min<u32>(cwnd_, 1u << 30);
    dupacks_ = 0;

    retransmit_deadline_.reset();
    if (snd_una_ != snd_max_) {
        retransmit_deadline_ = now + rto_;
    }
}

void TcpConnection::OnDuplicateAck() {
    if (in_recovery_) {
        cwnd_ += snd_mss_;
        return;
    }
    if (++dupacks_ == 3) {
        const u32 flight = snd_max_ - snd_una_;
        ssthresh_ = std::max<u32>(flight / 2, 2u * snd_mss_);
        recover_ = snd_max_;
        in_recovery_ = true;
        RetransmitFirst();
        cwnd_ = ssthresh_ + 3u * snd_mss_;
    }
}

void TcpConnection::ProcessData(const TcpSegment& seg, Clock::time_point now) {
    if (state_ != TcpState::Established && state_ != TcpState::FinWait1 &&
        state_ != TcpState::FinWait2) {
        // Peer already sent FIN, this is a retransmission.
        if (seg.flags & Fin) {
            SendAck();
        }
        return;
    }

    u32 seq = seg.seq;
    std::span<const u8> data = seg.payload;
    if (seg.flags & Fin) {
        peer_fin_seq_ = seg.seq + static_cast<u32>(seg.payload.size());
    }
    if (SeqLt(seq, rcv_nxt_)) {
        const u32 skip = rcv_nxt_ - seq;
        data = skip >= data.size() ? std::span<const u8>{} : data.subspan(skip);
        seq = rcv_nxt_;
    }
    const u32 window = ReceiveWindow();
    const u32 room = seq - rcv_nxt_ < window ? rcv_nxt_ + window - seq : 0;
    if (data.size() > room) {
        data = data.first(room);
    }

    bool ack_now = false;
    if (!data.empty()) {
        if (seq == rcv_nxt_) {
            recv_buf_.insert(recv_buf_.end(), data.begin(), data.end());
            rcv_nxt_ += static_cast<u32>(data.size());
            rcv_offset_ += data.size();
            if (!out_of_order_.empty()) {
                PullOutOfOrder();
                ack_now = true; // filled a hole
            }
            ++unacked_segments_;
        } else {
            const u64 offset = rcv_offset_ + (seq - rcv_nxt_);
            if (out_of_order_bytes_ + data.size() <= config_.recv_buffer) {
                auto& slot = out_of_order_[offset];
                if (slot.size() < data.size()) {
                    out_of_order_bytes_ += data.size() - slot.size();
                    slot.assign(data.begin(), data.end());
                }
            }
            ack_now = true; // dup ACK for fast retransmit
        }
    }

    if (peer_fin_seq_ && *peer_fin_seq_ == rcv_nxt_ && !fin_received_) {
        ++rcv_nxt_;
        fin_received_ = true;
        ack_now = true;
        switch (state_) {
        case TcpState::Established:
            state_ = TcpState::CloseWait;
            break;
        case TcpState::FinWait1:
            if (FinAcked()) {
                EnterTimeWait(now);
            } else {
                state_ = TcpState::Closing;
            }
            break;
        case TcpState::FinWait2:
            EnterTimeWait(now);
            break;
        default:
            break;
        }
    }

    if (ack_now || unacked_segments_ >= 2) {
        SendAck();
    } else if (unacked_segments_ > 0 && !delayed_ack_deadline_) {
        delayed_ack_deadline_ = now + config_.delayed_ack;
    }
}

void TcpConnection::PullOutOfOrder() {
    while (!out_of_order_.empty()) {
        auto it = out_of_order_.begin();
        if (it->first > rcv_offset_) {
            return;
        }
        const u64 end = it->first + it->second.size();
        if (end > rcv_offset_) {
            const auto skip = static_cast<std::ptrdiff_t>(rcv_offset_ - it->first);
            recv_buf_.insert(recv_buf_.end(), it->second.begin() + skip, it->second.end());
            const u32 added = static_cast<u32>(end - rcv_offset_);
            rcv_nxt_ += added;
            rcv_offset_ += added;
        }
        out_of_order_bytes_ -= it->second.size();
        out_of_order_.erase(it);
    }
}

std::optional<Clock::time_point> TcpConnection::NextDeadline() const {
    std::optional<Clock::time_point> next;
    for (const auto& deadline :
         {retransmit_deadline_, persist_deadline_, delayed_ack_deadline_, time_wait_deadline_}) {
        if (deadline && (!next || *deadline < *next)) {
            next = deadline;
        }
    }
    return next;
}

void TcpConnection::OnTimer(Clock::time_point now) {
    if (time_wait_deadline_ && now >= *time_wait_deadline_) {
        state_ = TcpState::Closed;
        ClearTimers();
        return;
    }
    if (delayed_ack_deadline_ && now >= *delayed_ack_deadline_) {
        SendAck();
    }
    if (retransmit_deadline_ && now >= *retransmit_deadline_) {
        retransmit_deadline_.reset();
        OnRetransmitTimeout(now);
    }
    if (persist_deadline_ && now >= *persist_deadline_) {
        EmitSegment(snd_una_ - 1, Ack, {});
        persist_interval_ = std::min<Clock::duration>(persist_interval_ * 2, config_.max_rto);
        persist_deadline_ = now + persist_interval_;
    }
}

void TcpConnection::OnRetransmitTimeout(Clock::time_point now) {
    const bool handshake = state_ == TcpState::SynSent || state_ == TcpState::SynReceived;
    if (++retries_ > (handshake ? config_.syn_retries : config_.data_retries)) {
        if (Synchronized()) {
            EmitReset(snd_max_);
        }
        Terminate(Error::TimedOut);
        return;
    }
    ++retransmissions_;
    rtt_active_ = false;
    rto_ = std::min<Clock::duration>(rto_ * 2, config_.max_rto);

    if (state_ == TcpState::SynSent) {
        EmitSegment(iss_, Syn, {}, true);
    } else if (state_ == TcpState::SynReceived) {
        EmitSegment(iss_, Syn | Ack, {}, true);
    } else {
        const u32 flight = snd_max_ - snd_una_;
        ssthresh_ = std::max<u32>(flight / 2, 2u * snd_mss_);
        cwnd_ = snd_mss_;
        in_recovery_ = false;
        dupacks_ = 0;
        snd_nxt_ = snd_una_;
        TrySend(now);
    }
    retransmit_deadline_ = now + rto_;
}

void TcpConnection::SampleRtt(Clock::duration rtt) {
    if (!have_rtt_) {
        srtt_ = rtt;
        rttvar_ = rtt / 2;
        have_rtt_ = true;
    } else {
        const auto delta = srtt_ > rtt ? srtt_ - rtt : rtt - srtt_;
        rttvar_ = (3 * rttvar_ + delta) / 4;
        srtt_ = (7 * srtt_ + rtt) / 8;
    }
    const auto variance = std::max<Clock::duration>(4 * rttvar_, std::chrono::milliseconds(10));
    rto_ = std::clamp<Clock::duration>(srtt_ + variance, config_.min_rto, config_.max_rto);
}

void TcpConnection::EnterTimeWait(Clock::time_point now) {
    ClearTimers();
    state_ = TcpState::TimeWait;
    time_wait_deadline_ = now + config_.time_wait;
}

void TcpConnection::Terminate(Error error) {
    error_ = error;
    state_ = TcpState::Closed;
    ClearTimers();
    send_buf_.clear();
    out_of_order_.clear();
    out_of_order_bytes_ = 0;
    // Received data stays readable, like BSD.
}

void TcpConnection::ClearTimers() {
    retransmit_deadline_.reset();
    persist_deadline_.reset();
    delayed_ack_deadline_.reset();
    time_wait_deadline_.reset();
}

TcpConnection::IoResult TcpConnection::Send(std::span<const u8> data, Clock::time_point now) {
    if (error_ != Error::Ok) {
        return {0, error_};
    }
    switch (state_) {
    case TcpState::SynSent:
    case TcpState::SynReceived:
        return {0, Error::WouldBlock};
    case TcpState::Established:
    case TcpState::CloseWait:
        break;
    default:
        return {0, fin_queued_ ? Error::Pipe : Error::NotConn};
    }
    if (fin_queued_) {
        return {0, Error::Pipe};
    }
    const size_t n = std::min(SendSpace(), data.size());
    if (n == 0) {
        return {0, data.empty() ? Error::Ok : Error::WouldBlock};
    }
    send_buf_.insert(send_buf_.end(), data.begin(), data.begin() + n);
    TrySend(now);
    return {n, Error::Ok};
}

TcpConnection::IoResult TcpConnection::Recv(std::span<u8> out, bool peek, Clock::time_point) {
    if (!recv_buf_.empty()) {
        const size_t n = std::min(out.size(), recv_buf_.size());
        std::copy_n(recv_buf_.begin(), n, out.begin());
        if (!peek) {
            recv_buf_.erase(recv_buf_.begin(), recv_buf_.begin() + n);
            MaybeSendWindowUpdate();
        }
        return {n, Error::Ok};
    }
    if (fin_received_) {
        return {0, Error::Ok}; // EOF
    }
    if (error_ != Error::Ok) {
        return {0, error_};
    }
    if (state_ == TcpState::Closed) {
        return {0, Error::NotConn};
    }
    return {0, Error::WouldBlock};
}

void TcpConnection::MaybeSendWindowUpdate() {
    if (!Synchronized() || fin_received_) {
        return;
    }
    // Only update once the window opened enough to avoid silly window syndrome.
    const u32 threshold = std::min<u32>(config_.recv_buffer / 2, 2u * snd_mss_);
    if (static_cast<s32>(ReceiveWindow()) - static_cast<s32>(last_advertised_window_) >=
        static_cast<s32>(threshold)) {
        SendAck();
    }
}

void TcpConnection::Shutdown(Clock::time_point now) {
    if (fin_queued_) {
        return;
    }
    switch (state_) {
    case TcpState::SynSent:
        Terminate(Error::Ok);
        return;
    case TcpState::SynReceived:
        Abort(); // not accepted yet
        return;
    case TcpState::Established:
        state_ = TcpState::FinWait1;
        break;
    case TcpState::CloseWait:
        state_ = TcpState::LastAck;
        break;
    default:
        return;
    }
    fin_queued_ = true;
    TrySend(now);
}

void TcpConnection::Close(Clock::time_point now) {
    if (state_ == TcpState::SynSent || (Synchronized() && !recv_buf_.empty())) {
        Abort(); // BSD resets on close with unread data
        return;
    }
    Shutdown(now);
}

void TcpConnection::Abort() {
    if (state_ != TcpState::Closed && state_ != TcpState::SynSent && state_ != TcpState::TimeWait) {
        EmitReset(snd_max_);
    }
    Terminate(Error::Ok);
}

} // namespace Core::Net::P2P
