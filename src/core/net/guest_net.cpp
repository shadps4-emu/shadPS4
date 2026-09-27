// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "core/net/guest_net.h"
#include "core/net/p2p_transport.h"

namespace Core::Net {

using Host::Error;

namespace {

// A blocking call interrupted by abort or close fails with EINTR
constexpr Error AbortedError = Error::Intr;

struct GuestEpoll;

// Which abort flag a blocking call answers to.
enum class Side { Recv = 0, Send = 1 };

// One blocking call in progress. Lives on the caller's stack while it is registered.
struct Waiter {
    const Host::WakeHandle* wake;
    Side side;
    bool aborted = false; // guarded by the owner's wait_mutex
};

// One wake handle per guest thread, created on its first blocking call. Blocking calls wait on
// {socket, this handle}; abort and close signal the handles of the calls they interrupt.
const Host::WakeHandle* ThreadWake() {
    thread_local Host::WakeHandle wake;
    return wake.Valid() ? &wake : nullptr;
}

struct GuestSocket {
    GuestSocket(Host::NativeSocket native_, int type_, u32 generation_)
        : native(native_), type(type_), generation(generation_) {}
    GuestSocket(std::shared_ptr<P2P::DatagramSocket> dgram, u32 generation_)
        : native(Host::InvalidSocket), type(SOCK_DGRAM), generation(generation_),
          p2p_dgram(std::move(dgram)) {}
    GuestSocket(std::shared_ptr<P2P::StreamSocket> stream, u32 generation_)
        : native(Host::InvalidSocket), type(SOCK_STREAM), generation(generation_),
          p2p_stream(std::move(stream)) {}
    ~GuestSocket() {
        // Runs when the last in-flight call drops its reference. P2P streams close gracefully
        // (FIN, TIME-WAIT) inside the transport after the guest has let go.
        if (p2p_stream) {
            p2p_stream->Close();
        }
        if (p2p_dgram) {
            p2p_dgram->Close();
        }
        if (native != Host::InvalidSocket && type == SOCK_STREAM) {
            Host::PrepareClose(native, !nonblocking);
        }
        Host::CloseSocket(native);
    }
    GuestSocket(const GuestSocket&) = delete;
    GuestSocket& operator=(const GuestSocket&) = delete;

    bool IsP2P() const {
        return p2p_dgram || p2p_stream;
    }
    const P2P::Readiness* P2PHandles() const {
        return p2p_dgram ? &p2p_dgram->Handles() : p2p_stream ? &p2p_stream->Handles() : nullptr;
    }
    u32 P2PEvents() const {
        return p2p_dgram ? p2p_dgram->Events() : p2p_stream ? p2p_stream->Events() : 0;
    }

    const Host::NativeSocket native; // InvalidSocket for P2P sockets
    const int type;
    const u32 generation; // distinguishes reused ids inside epoll tags
    const std::shared_ptr<P2P::DatagramSocket> p2p_dgram;
    const std::shared_ptr<P2P::StreamSocket> p2p_stream;

    std::atomic<bool> nonblocking{false};
    std::atomic<bool> reuse_addr{false};
    std::atomic<bool> reuse_port{false};
    std::atomic<s64> rcv_timeout_us{0}; // 0 = infinite (BSD semantics)
    std::atomic<s64> snd_timeout_us{0};
    std::atomic<s64> connect_timeout_us{0}; // SO_CONNECTTIMEO, 0 = infinite
    std::atomic<s64> accept_timeout_us{0};  // SO_ACCEPTTIMEO, 0 = infinite
    std::atomic<bool> connected{false};     // connect succeeded or started, or accepted
    std::atomic<bool> listening{false};

    std::mutex attr_mutex;
    SocketAttributes attributes; // guarded by attr_mutex

    // Blocking calls in progress, and aborts preserved for the next one (see SocketAbort).
    std::mutex wait_mutex;
    std::vector<Waiter*> waiters;        // guarded by wait_mutex
    std::array<bool, 2> pending_abort{}; // guarded by wait_mutex, indexed by Side
    bool send_again = false;             // guarded by wait_mutex: SND_PRESERVATION_AGAIN hit a send
    bool closing = false;                // guarded by wait_mutex; set once by close

    // Lock order: GuestEpoll::mutex before reg_mutex.
    std::mutex reg_mutex;
    bool closed = false;                           // guarded by reg_mutex
    std::vector<std::weak_ptr<GuestEpoll>> epolls; // guarded by reg_mutex
};

struct GuestEpoll {
    struct Registration {
        u32 generation;
        u64 data;
        u32 events;
        bool p2p;
        bool has_writable;     // P2P: writable handle registered (EvOut requested)
        bool external;         // an ExternalObject: reported once, then its handle is removed
        bool armed;            // external: its handle is still in the host epoll
        bool reported = false; // external: its completion has been reported
    };

    Host::HostEpoll host;
    std::mutex mutex;
    std::unordered_map<s32, Registration> regs; // guarded by mutex, keyed by socket id

    // Waits in progress and abort state, guarded by mutex. Abort wakes waits through the host
    // epoll's wake handle, which is drained once every flagged wait has left.
    std::vector<Waiter*> waiters;
    int flagged_waiters = 0;
    bool pending_abort = false;
    bool destroyed = false;
    /// Sockets aborted while this epoll was being waited on, as (id, generation): reported
    /// once as EvHup. Delivered through the wake handle, like an abort.
    std::vector<std::pair<s32, u32>> pending_hups;

    /// The wake handle may be drained: no flagged wait and no hangup left to deliver.
    bool WakeIdle() const {
        return flagged_waiters == 0 && pending_hups.empty() && !destroyed;
    }
};

// All net objects, keyed by id (see IdAllocator in guest_net.h).
// An id whose object the library keeps (a resolver). Core::Net only knows when it finishes,
// so an epoll can report that (ORBIS_NET_EPOLLDESCID).
struct ExternalObject {
    explicit ExternalObject(u32 generation_) : generation(generation_) {}

    const u32 generation;
    P2P::ReadinessFlag done; // set once by SignalExternal
    std::atomic<bool> signaled{false};
    std::atomic<bool> hangup{false}; // finished by an abort: reported with EvHup

    std::mutex reg_mutex; // guards the two below, like GuestSocket::reg_mutex
    bool closed = false;
    std::vector<std::weak_ptr<GuestEpoll>> epolls;
};

class ObjectTable {
public:
    using Object = std::variant<std::shared_ptr<GuestSocket>, std::shared_ptr<GuestEpoll>,
                                std::shared_ptr<ExternalObject>>;

    // Returns the new id, or fails with EMFILE (socket limit) or ENFILE (no id left).
    NetResult Insert(Object object) {
        // Sockets and resolvers count towards the limit, epolls do not.
        const bool counted = !std::holds_alternative<std::shared_ptr<GuestEpoll>>(object);
        {
            std::scoped_lock lock{mutex_};
            if (counted && counted_ >= MaxSockets) {
                return NetResult::Fail(Error::MFile);
            }
            counted_ += counted ? 1 : 0; // reserve the slot while the id is allocated
        }
        const s32 id = Allocate(); // hooks run without our lock
        std::scoped_lock lock{mutex_};
        if (id < 0) {
            counted_ -= counted ? 1 : 0;
            return NetResult::Fail(Error::NFile);
        }
        objects_[id] = {std::move(object), counted};
        return NetResult::Ok(id);
    }

    template <typename T>
    std::shared_ptr<T> Get(s32 id) const {
        std::scoped_lock lock{mutex_};
        const auto it = objects_.find(id);
        if (it == objects_.end()) {
            return nullptr;
        }
        const auto* object = std::get_if<std::shared_ptr<T>>(&it->second.object);
        return object ? *object : nullptr;
    }

    // Removes `id` if it holds a T.
    template <typename T>
    std::shared_ptr<T> Remove(s32 id) {
        std::shared_ptr<T> removed;
        {
            std::scoped_lock lock{mutex_};
            const auto it = objects_.find(id);
            if (it == objects_.end()) {
                return nullptr;
            }
            auto* object = std::get_if<std::shared_ptr<T>>(&it->second.object);
            if (object == nullptr) {
                return nullptr;
            }
            removed = std::move(*object);
            counted_ -= it->second.counted ? 1 : 0;
            objects_.erase(it);
        }
        Release(id);
        return removed;
    }

    std::optional<NetObjectKind> Kind(s32 id) const {
        std::scoped_lock lock{mutex_};
        const auto it = objects_.find(id);
        if (it == objects_.end()) {
            return std::nullopt;
        }
        switch (it->second.object.index()) {
        case 0:
            return NetObjectKind::Socket;
        case 1:
            return NetObjectKind::Epoll;
        default:
            return NetObjectKind::External;
        }
    }

    /// The ids holding a T, in ascending order.
    template <typename T>
    std::vector<s32> Ids() const {
        std::vector<s32> ids;
        std::scoped_lock lock{mutex_};
        for (const auto& [id, entry] : objects_) {
            if (std::holds_alternative<std::shared_ptr<T>>(entry.object)) {
                ids.push_back(id);
            }
        }
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    void SetAllocator(IdAllocator allocator) {
        std::scoped_lock lock{alloc_mutex_};
        allocator_ = std::move(allocator);
    }

private:
    struct Entry {
        Object object;
        bool counted;
    };

    s32 Allocate() {
        std::unique_lock lock{alloc_mutex_};
        if (allocator_.allocate) {
            const auto allocate = allocator_.allocate;
            lock.unlock();
            return allocate();
        }
        // Default: lowest free id from 1, like a BSD descriptor table.
        auto it = std::find(used_.begin() + 1, used_.end(), false);
        if (it == used_.end()) {
            used_.push_back(false);
            it = used_.end() - 1;
        }
        *it = true;
        return static_cast<s32>(it - used_.begin());
    }

    void Release(s32 id) {
        std::unique_lock lock{alloc_mutex_};
        if (allocator_.release) {
            const auto release = allocator_.release;
            lock.unlock();
            release(id);
        } else if (static_cast<size_t>(id) < used_.size()) {
            used_[static_cast<size_t>(id)] = false;
        }
    }

    mutable std::mutex mutex_;
    std::unordered_map<s32, Entry> objects_; // guarded by mutex_
    int counted_ = 0;                        // guarded by mutex_

    std::mutex alloc_mutex_;
    IdAllocator allocator_;        // guarded by alloc_mutex_
    std::vector<bool> used_{true}; // default allocator; id 0 is never handed out
};

ObjectTable g_objects;
std::atomic<u32> g_next_generation{1};

std::mutex g_p2p_mutex;
std::shared_ptr<P2P::Transport> g_p2p_transports[2]; // [0] = IPv4, [1] = IPv6

// Epoll tag: generation in the high 32 bits, socket id in the low 31, and bit 31 marking a
// P2P socket's writable handle. Ids are positive, so a tag never equals Host::ReservedTag.
constexpr u64 WritableHandleBit = 1ull << 31;

u64 MakeTag(s32 id, u32 generation, bool writable_handle = false) {
    return (static_cast<u64>(generation) << 32) | static_cast<u32>(id) |
           (writable_handle ? WritableHandleBit : 0);
}

NetResult FromError(Error e) {
    return e == Error::Ok ? NetResult::Ok() : NetResult::Fail(e);
}

// What to poll when waiting for `interest` on `s`: the host socket itself, or for P2P
// sockets the matching readiness handle (which becomes readable when the condition holds).
std::pair<Host::NativeSocket, u32> WaitTarget(const GuestSocket& s, u32 interest) {
    if (const auto* handles = s.P2PHandles()) {
        const auto& flag = (interest & Host::Writable) ? handles->writable : handles->readable;
        return {flag.PollHandle(), Host::Readable};
    }
    return {s.native, interest};
}

// Takes a preserved abort for `side`, if there is one.
bool ConsumePendingAbort(GuestSocket& s, Side side) {
    std::scoped_lock lock{s.wait_mutex};
    return std::exchange(s.pending_abort[static_cast<size_t>(side)], false);
}

// Registers a blocking call with its socket for as long as it runs, so abort and close can
// interrupt it. Entry() consumes a preserved abort.
class WaitScope {
public:
    WaitScope(GuestSocket& s, Side side) : s_(s) {
        waiter_.wake = ThreadWake();
        waiter_.side = side;
        if (waiter_.wake == nullptr) {
            entry_error_ = Error::NoBufs;
            return;
        }
        std::scoped_lock lock{s_.wait_mutex};
        waiter_.wake->Drain(); // a signal meant for this thread's previous call
        if (s_.closing) {
            entry_error_ = Error::BadF;
        } else if (std::exchange(s_.pending_abort[static_cast<size_t>(side)], false)) {
            entry_error_ = AbortedError;
        } else {
            s_.waiters.push_back(&waiter_);
            registered_ = true;
        }
    }
    ~WaitScope() {
        if (registered_) {
            std::scoped_lock lock{s_.wait_mutex};
            std::erase(s_.waiters, &waiter_);
        }
    }
    WaitScope(const WaitScope&) = delete;
    WaitScope& operator=(const WaitScope&) = delete;

    Error Entry() const {
        return entry_error_;
    }
    bool Interrupted() {
        std::scoped_lock lock{s_.wait_mutex};
        return waiter_.aborted;
    }
    const Host::WakeHandle& Wake() const {
        return *waiter_.wake;
    }

private:
    GuestSocket& s_;
    Waiter waiter_{};
    Error entry_error_ = Error::Ok;
    bool registered_ = false;
};

// Wakes every call blocked on `s` (wait_mutex held). Returns which sides had one.
std::array<bool, 2> InterruptWaiters(GuestSocket& s) {
    std::array<bool, 2> blocked{};
    for (Waiter* w : s.waiters) {
        w->aborted = true;
        w->wake->Signal();
        blocked[static_cast<size_t>(w->side)] = true;
    }
    return blocked;
}

// Core of every blocking guest call: try the non-blocking op; on WouldBlock, wait for
// readiness, deadline or abort, then retry.
template <typename Op>
NetResult RunBlocking(GuestSocket& s, Side side, u32 interest, Host::Deadline deadline,
                      bool blocking, Op&& op) {
    if (!blocking) {
        // A preserved abort is for the next call of its kind, blocking or not (Net Library
        // Reference, sceNetSocketAbort).
        if (ConsumePendingAbort(s, side)) {
            return NetResult::Fail(AbortedError);
        }
        return op();
    }
    WaitScope scope(s, side);
    if (const Error e = scope.Entry(); e != Error::Ok) {
        return NetResult::Fail(e);
    }
    const auto [handle, handle_interest] = WaitTarget(s, interest);
    for (;;) {
        if (scope.Interrupted()) {
            return NetResult::Fail(AbortedError);
        }
        const NetResult r = op();
        if (r.error != Error::WouldBlock) {
            return r;
        }
        switch (Host::WaitOne(handle, handle_interest, scope.Wake(), deadline)) {
        case Host::WaitResult::Ready:
        case Host::WaitResult::Woken: // top of the loop reports the abort
            continue;
        case Host::WaitResult::TimedOut:
            // BSD: an expired SO_RCVTIMEO/SO_SNDTIMEO is EAGAIN, not ETIMEDOUT.
            return NetResult::Fail(Error::WouldBlock);
        case Host::WaitResult::Failed:
            return NetResult::Fail(Host::LastError());
        }
    }
}

Host::Deadline SocketDeadline(const std::atomic<s64>& timeout_us) {
    return Host::DeadlineFromSocketTimeout(std::chrono::microseconds{timeout_us.load()});
}

// Blocking stream sends complete fully (BSD); `send_some` performs one non-blocking attempt.
template <typename SendSome>
NetResult SendAll(GuestSocket& s, size_t len, bool blocking, bool stream, SendSome&& send_some) {
    const auto deadline = SocketDeadline(s.snd_timeout_us); // one deadline for the whole call
    size_t sent = 0;
    for (;;) {
        const NetResult r = RunBlocking(s, Side::Send, Host::Writable, deadline, blocking,
                                        [&] { return send_some(sent); });
        if (r.error != Error::Ok) {
            if (r.error == AbortedError) {
                // SND_PRESERVATION_AGAIN: after an aborted send returned a partial count, the
                // next send fails with EINTR once.
                std::scoped_lock lock{s.wait_mutex};
                if (std::exchange(s.send_again, false) && sent > 0) {
                    s.pending_abort[static_cast<size_t>(Side::Send)] = true;
                }
            }
            // BSD returns the partial count if a blocking send is cut short after progress.
            return sent > 0 ? NetResult::Ok(static_cast<s64>(sent)) : r;
        }
        sent += static_cast<size_t>(r.value);
        if (!blocking || !stream || sent >= len) {
            return NetResult::Ok(static_cast<s64>(sent));
        }
    }
}

// MSG_WAITALL on a blocking stream: receive until `len` bytes, FIN or an error; data already
// received wins over the error (Net Library Reference, sceNetRecv). `recv_some(got)` is one
// (blocking) receive into the rest of the buffer.
template <typename RecvSome>
NetResult ReceiveAll(size_t len, bool waitall, RecvSome&& recv_some) {
    if (!waitall) {
        return recv_some(0);
    }
    size_t got = 0;
    while (got < len) {
        const NetResult r = recv_some(got);
        if (r.error != Error::Ok) {
            return got > 0 ? NetResult::Ok(static_cast<s64>(got)) : r;
        }
        if (r.value == 0) {
            break; // FIN
        }
        got += static_cast<size_t>(r.value);
    }
    return NetResult::Ok(static_cast<s64>(got));
}

// --- Epoll registration helpers (GuestEpoll::mutex held) -------------------------------------

Error HostRegister(GuestEpoll& ep, s32 id, GuestSocket& s, u32 events, u32 flags,
                   GuestEpoll::Registration& reg) {
    if (!s.IsP2P()) {
        return ep.host.Add(s.native, MakeTag(id, s.generation), events, flags);
    }
    if (flags != 0) {
        return Error::Inval; // TODO: one-shot not emulated for P2P yet (no guest asks)
    }
    // The readable handle is always registered: like epoll, errors and hangups are reported
    // even when not requested, and the readable flag is set for those too.
    const auto* handles = s.P2PHandles();
    if (const Error e =
            ep.host.Add(handles->readable.PollHandle(), MakeTag(id, s.generation), Host::EvIn, 0);
        e != Error::Ok) {
        return e;
    }
    if (events & Host::EvOut) {
        if (const Error e = ep.host.Add(handles->writable.PollHandle(),
                                        MakeTag(id, s.generation, true), Host::EvIn, 0);
            e != Error::Ok) {
            ep.host.Remove(handles->readable.PollHandle());
            return e;
        }
        reg.has_writable = true;
    }
    return Error::Ok;
}

Error HostModify(GuestEpoll& ep, s32 id, GuestSocket& s, u32 events, u32 flags,
                 GuestEpoll::Registration& reg) {
    if (!s.IsP2P()) {
        return ep.host.Modify(s.native, MakeTag(id, s.generation), events, flags);
    }
    if (flags != 0) {
        return Error::Inval;
    }
    const auto* handles = s.P2PHandles();
    const bool want_writable = (events & Host::EvOut) != 0;
    if (want_writable && !reg.has_writable) {
        if (const Error e = ep.host.Add(handles->writable.PollHandle(),
                                        MakeTag(id, s.generation, true), Host::EvIn, 0);
            e != Error::Ok) {
            return e;
        }
    } else if (!want_writable && reg.has_writable) {
        ep.host.Remove(handles->writable.PollHandle());
    }
    reg.has_writable = want_writable;
    return Error::Ok;
}

Error HostUnregister(GuestEpoll& ep, GuestSocket& s, const GuestEpoll::Registration& reg) {
    if (!s.IsP2P()) {
        return ep.host.Remove(s.native);
    }
    const auto* handles = s.P2PHandles();
    if (reg.has_writable) {
        ep.host.Remove(handles->writable.PollHandle());
    }
    return ep.host.Remove(handles->readable.PollHandle());
}

std::shared_ptr<P2P::Transport> P2PTransportFor(int family) {
    std::scoped_lock lock{g_p2p_mutex};
    return g_p2p_transports[family == AF_INET6 ? 1 : 0];
}

} // namespace

void SetIdAllocator(IdAllocator allocator) {
    g_objects.SetAllocator(std::move(allocator));
}

std::optional<NetObjectKind> GetObjectKind(s32 id) {
    return g_objects.Kind(id);
}

NetResult CloseObject(s32 id) {
    switch (g_objects.Kind(id).value_or(NetObjectKind::Socket)) {
    case NetObjectKind::Socket:
        return SocketClose(id); // EBADF when the id is unknown
    case NetObjectKind::Epoll:
        return EpollDestroy(id);
    case NetObjectKind::External:
        break; // released by its owner, which knows what else to clean up
    }
    return NetResult::Fail(Error::BadF);
}

NetResult ReserveExternalId() {
    auto object = std::make_shared<ExternalObject>(g_next_generation.fetch_add(1));
    if (!object->done.Valid()) {
        return NetResult::Fail(Error::NoBufs);
    }
    return g_objects.Insert(std::move(object));
}

NetResult SignalExternal(s32 id, bool hangup) {
    const auto x = g_objects.Get<ExternalObject>(id);
    if (!x) {
        return NetResult::Fail(Error::BadF);
    }
    x->hangup = hangup;
    x->signaled = true;
    x->done.Set(true);
    return NetResult::Ok();
}

NetResult ReleaseExternalId(s32 id) {
    const auto x = g_objects.Remove<ExternalObject>(id);
    if (!x) {
        return NetResult::Fail(Error::BadF);
    }
    std::vector<std::weak_ptr<GuestEpoll>> epolls;
    {
        std::scoped_lock lock{x->reg_mutex};
        x->closed = true;
        epolls.swap(x->epolls);
    }
    for (const auto& weak : epolls) {
        const auto ep = weak.lock();
        if (!ep) {
            continue;
        }
        std::scoped_lock lock{ep->mutex};
        const auto it = ep->regs.find(id);
        if (it != ep->regs.end() && it->second.generation == x->generation) {
            if (it->second.armed) {
                ep->host.Remove(x->done.PollHandle());
            }
            ep->regs.erase(it);
        }
    }
    return NetResult::Ok();
}

std::vector<s32> ListSockets() {
    return g_objects.Ids<GuestSocket>();
}

s32 ToOrbisReturn(NetResult result) {
    if (result.error == Error::Ok) {
        return static_cast<s32>(result.value);
    }
    // TODO: store static_cast<s32>(result.error) in the guest net errno (what sceNetErrnoLoc
    // returns) via the existing helper, and reuse the existing ORBIS_NET_ERROR_* constants.
    return static_cast<s32>(0x80410100u | static_cast<u32>(result.error));
}

// Sockets
namespace {

bool IsLimitedBroadcast(const sockaddr* addr, socklen_t len) {
    if (addr == nullptr || addr->sa_family != AF_INET || len < sizeof(sockaddr_in)) {
        return false;
    }
    return reinterpret_cast<const sockaddr_in*>(addr)->sin_addr.s_addr == htonl(INADDR_BROADCAST);
}

bool BroadcastAllowed(Host::NativeSocket s) {
    int on = 0;
    socklen_t len = sizeof(on);
    // Windows may write a 1-byte BOOL; `on` starts at 0, so either size reads right.
    return getsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&on), &len) == 0 &&
           on != 0;
}

} // namespace

NetResult SocketCreate(int family, int type, int protocol) {
    // A protocol that does not go with the type: EPROTONOSUPPORT, as FreeBSD's socreate()
    // says. Hosts disagree (macOS answers EPROTOTYPE), so this is decided here.
    if ((family == AF_INET || family == AF_INET6) && protocol != 0 &&
        ((type == SOCK_STREAM && protocol != IPPROTO_TCP) ||
         (type == SOCK_DGRAM && protocol != IPPROTO_UDP))) {
        return NetResult::Fail(Error::ProtoNoSupport);
    }
    Error e;
    const Host::NativeSocket native = Host::CreateSocket(family, type, protocol, &e);
    if (native == Host::InvalidSocket) {
        return NetResult::Fail(e);
    }
    auto s = std::make_shared<GuestSocket>(native, type, g_next_generation.fetch_add(1));
    return g_objects.Insert(std::move(s));
}

NetResult SocketCreatePair(int family, int type, int protocol, s32 ids[2]) {
    Host::NativeSocket natives[2];
    if (const Error e = Host::CreateSocketPair(family, type, protocol, natives); e != Error::Ok) {
        return NetResult::Fail(e);
    }
    auto first = std::make_shared<GuestSocket>(natives[0], type, g_next_generation.fetch_add(1));
    auto second = std::make_shared<GuestSocket>(natives[1], type, g_next_generation.fetch_add(1));
    const NetResult a = g_objects.Insert(std::move(first));
    if (a.error != Error::Ok) {
        return a; // `second` closes its socket on the way out
    }
    const NetResult b = g_objects.Insert(std::move(second));
    if (b.error != Error::Ok) {
        SocketClose(static_cast<s32>(a.value));
        return b;
    }
    ids[0] = static_cast<s32>(a.value);
    ids[1] = static_cast<s32>(b.value);
    return NetResult::Ok();
}

NetResult SocketBind(s32 id, const sockaddr* addr, socklen_t len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketBind
    }
    if (const Error e = Host::PrepareBind(s->native, s->reuse_addr, s->reuse_port);
        e != Error::Ok) {
        return NetResult::Fail(e);
    }
    return FromError(Host::Bind(s->native, addr, len));
}

NetResult SocketListen(s32 id, int backlog) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->p2p_dgram) {
        return NetResult::Fail(Error::OpNotSupp); // "socket type cannot accept connections"
    }
    const Error e =
        s->p2p_stream ? s->p2p_stream->Listen(backlog) : Host::Listen(s->native, backlog);
    if (e == Error::Ok) {
        s->listening = true;
    }
    return FromError(e);
}

NetResult SocketConnect(s32 id, const sockaddr* addr, socklen_t len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketConnect
    }
    if (s->listening) {
        return NetResult::Fail(Error::OpNotSupp); // "connection attempted to a listening socket"
    }
    if (s->nonblocking) {
        if (ConsumePendingAbort(*s, Side::Send)) {
            return NetResult::Fail(AbortedError);
        }
        const Error e = Host::Connect(s->native, addr, len);
        if (e == Error::Ok || e == Error::InProgress) {
            s->connected = true;
        }
        return FromError(e);
    }
    // Blocking connect: registered first, so a preserved abort stops it before it starts.
    WaitScope scope(*s, Side::Send);
    if (const Error e = scope.Entry(); e != Error::Ok) {
        return NetResult::Fail(e);
    }
    const Error started = Host::Connect(s->native, addr, len);
    if (started == Error::Ok || started == Error::InProgress) {
        s->connected = true;
    }
    if (started != Error::InProgress) {
        return FromError(started);
    }
    // Wait for writability, then read the verdict from SO_ERROR. The 500 ms slices guard
    // against old WSAPoll builds that never signal a failed connect. SO_CONNECTTIMEO bounds
    // the wait: EWOULDBLOCK when it passes, the connect going on in the background.
    using namespace std::chrono_literals;
    const auto deadline = SocketDeadline(s->connect_timeout_us);
    for (;;) {
        if (scope.Interrupted()) {
            return NetResult::Fail(AbortedError);
        }
        auto slice = Host::Clock::now() + 500ms;
        if (deadline && *deadline < slice) {
            if (Host::Clock::now() >= *deadline) {
                return NetResult::Fail(Error::WouldBlock);
            }
            slice = *deadline;
        }
        const auto w = Host::WaitOne(s->native, Host::Writable, scope.Wake(), slice);
        if (w == Host::WaitResult::Failed) {
            return NetResult::Fail(Host::LastError());
        }
        if (w == Host::WaitResult::Woken) {
            continue;
        }
        if (const Error pending = Host::PendingSocketError(s->native); pending != Error::Ok) {
            return NetResult::Fail(pending);
        }
        if (w == Host::WaitResult::Ready) {
            return NetResult::Ok();
        }
        // Slice expired with no verdict yet: keep waiting.
    }
}

NetResult SocketAccept(s32 id, sockaddr* addr, socklen_t* len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketAccept
    }
    Host::NativeSocket client = Host::InvalidSocket;
    // SO_ACCEPTTIMEO bounds the wait (EWOULDBLOCK), not SO_RCVTIMEO.
    const NetResult r = RunBlocking(
        *s, Side::Recv, Host::Readable, SocketDeadline(s->accept_timeout_us), !s->nonblocking, [&] {
            Error e;
            client = Host::Accept(s->native, addr, len, &e);
            return client == Host::InvalidSocket ? NetResult::Fail(e) : NetResult::Ok();
        });
    if (r.error != Error::Ok) {
        return r;
    }
    auto c = std::make_shared<GuestSocket>(client, s->type, g_next_generation.fetch_add(1));
    // FreeBSD gives the accepted socket the listener's O_NONBLOCK (unlike Linux), timeouts and
    // socket-level flags (sonewconn copies so_options and the buffers' sb_timeo).
    c->nonblocking = s->nonblocking.load();
    c->reuse_addr = s->reuse_addr.load();
    c->reuse_port = s->reuse_port.load();
    c->rcv_timeout_us = s->rcv_timeout_us.load();
    c->snd_timeout_us = s->snd_timeout_us.load();
    c->connect_timeout_us = s->connect_timeout_us.load();
    c->accept_timeout_us = s->accept_timeout_us.load();
    c->connected = true;
    {
        std::scoped_lock lock{s->attr_mutex};
        c->attributes = s->attributes;
    }
    return g_objects.Insert(std::move(c));
}

NetResult SocketSendTo(s32 id, const void* buf, size_t len, int host_flags, bool dontwait,
                       const sockaddr* addr, socklen_t addr_len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketSendTo
    }
    if (s->type != SOCK_STREAM) {
        // UDP and RAW never wait on the PS4: with the host buffer full the datagram is
        // dropped, and ENOBUFS is not reported. A preserved
        // send abort still applies (sceNetSocketAbort).
        if (ConsumePendingAbort(*s, Side::Send)) {
            return NetResult::Fail(kAbortedError);
        }
        if (IsLimitedBroadcast(addr, addr_len) && !BroadcastAllowed(s->native)) {
            // FreeBSD's ip_output: EACCES without SO_BROADCAST. Hosts without a route for
            // 255.255.255.255 (macOS, for one) fail earlier with EHOSTUNREACH instead.
            return NetResult::Fail(Error::Acces);
        }
        const auto io = Host::SendTo(s->native, buf, len, host_flags, addr, addr_len);
        if (io.error == Error::WouldBlock || io.error == Error::NoBufs) {
            return NetResult::Ok(static_cast<s64>(len));
        }
        if (io.error == Error::NotConn && addr == nullptr) {
            // No destination on an unconnected datagram socket: EDESTADDRREQ as on BSD and
            // Linux,Windows says ENOTCONN.
            return NetResult::Fail(Error::DestAddrReq);
        }
        return io.error == Error::Ok ? NetResult::Ok(io.value) : NetResult::Fail(io.error);
    }
    const auto* bytes = static_cast<const u8*>(buf);
    return SendAll(*s, len, !dontwait && !s->nonblocking, s->type == SOCK_STREAM, [&](size_t sent) {
        const auto io =
            Host::SendTo(s->native, bytes + sent, len - sent, host_flags, addr, addr_len);
        return NetResult{io.value, io.error};
    });
}

NetResult SocketRecvFrom(s32 id, void* buf, size_t len, int host_flags, bool dontwait,
                         sockaddr* addr, socklen_t* addr_len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketRecvFrom
    }
    const bool blocking = !dontwait && !s->nonblocking;
    // MSG_WAITALL is done here: the host socket is non-blocking, where it means nothing.
    const bool waitall = (host_flags & MSG_WAITALL) != 0;
    host_flags &= ~MSG_WAITALL;
    const auto deadline = SocketDeadline(s->rcv_timeout_us);
    auto* bytes = static_cast<u8*>(buf);
    return ReceiveAll(len, blocking && waitall && s->type == SOCK_STREAM, [&](size_t got) {
        return RunBlocking(*s, Side::Recv, Host::Readable, deadline, blocking, [&] {
            const auto io =
                Host::RecvFrom(s->native, bytes + got, len - got, host_flags,
                               got == 0 ? addr : nullptr, got == 0 ? addr_len : nullptr);
            return NetResult{io.value, io.error};
        });
    });
}

NetResult SocketShutdown(s32 id, int how) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->p2p_stream) {
        return FromError(s->p2p_stream->Shutdown(how));
    }
    if (s->p2p_dgram) {
        return NetResult::Ok(); // nothing to shut down on a datagram socket
    }
    return FromError(Host::ShutdownSocket(s->native, how));
}

NetResult SocketGetName(s32 id, sockaddr* addr, socklen_t* len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // DISCUSS: P2P getsockname (address + vport)
    }
    return FromError(getsockname(s->native, addr, len) == 0 ? Error::Ok : Host::LastError());
}

NetResult SocketGetPeerName(s32 id, sockaddr* addr, socklen_t* len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::Inval); // use P2PSocketGetPeerName
    }
    return FromError(getpeername(s->native, addr, len) == 0 ? Error::Ok : Host::LastError());
}

NetResult SocketSetHostOption(s32 id, int level, int name, const void* value, socklen_t len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::NoProtoOpt);
    }
    return FromError(setsockopt(s->native, level, name, static_cast<const char*>(value), len) == 0
                         ? Error::Ok
                         : Host::LastError());
}

NetResult SocketGetHostOption(s32 id, int level, int name, void* value, socklen_t* len) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    if (s->IsP2P()) {
        return NetResult::Fail(Error::NoProtoOpt);
    }
    return FromError(getsockopt(s->native, level, name, static_cast<char*>(value), len) == 0
                         ? Error::Ok
                         : Host::LastError());
}

NetResult SocketGetInfo(s32 id, SocketInfo* info) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    info->type = s->type;
    info->p2p = s->IsP2P();
    info->nonblocking = s->nonblocking;
    info->reuse_addr = s->reuse_addr;
    info->rcv_timeout_us = s->rcv_timeout_us;
    info->snd_timeout_us = s->snd_timeout_us;
    info->connect_timeout_us = s->connect_timeout_us;
    info->accept_timeout_us = s->accept_timeout_us;
    info->connected = s->connected;
    info->listening = s->listening;
    if (s->p2p_stream) {
        const Error e = s->p2p_stream->ConnectResult();
        info->pending_error =
            e == Error::InProgress || e == Error::NotConn ? Error::Ok : e; // DISCUSS: not cleared
    } else if (s->p2p_dgram) {
        info->pending_error = Error::Ok;
    } else {
        info->pending_error = Host::PendingSocketError(s->native);
    }
    return NetResult::Ok();
}

NetResult SocketGetAttributes(s32 id, SocketAttributes* out) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    std::scoped_lock lock{s->attr_mutex};
    *out = s->attributes;
    return NetResult::Ok();
}

NetResult SocketUpdateAttributes(s32 id, const std::function<void(SocketAttributes&)>& update) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    std::scoped_lock lock{s->attr_mutex};
    update(s->attributes);
    return NetResult::Ok();
}

NetResult SocketSetNonBlocking(s32 id, bool enable) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->nonblocking = enable; // guest state only: the host socket stays non-blocking
    return NetResult::Ok();
}

NetResult SocketSetReuseAddr(s32 id, bool enable) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->reuse_addr = enable;
    return NetResult::Ok();
}

NetResult SocketSetReusePort(s32 id, bool enable) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->reuse_port = enable;
    return NetResult::Ok();
}

NetResult SocketSetRecvTimeout(s32 id, std::chrono::microseconds value) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->rcv_timeout_us = value.count(); // never set on the host socket
    return NetResult::Ok();
}

NetResult SocketSetConnectTimeout(s32 id, std::chrono::microseconds value) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->connect_timeout_us = std::max<s64>(value.count(), 0);
    return NetResult::Ok();
}

NetResult SocketSetAcceptTimeout(s32 id, std::chrono::microseconds value) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->accept_timeout_us = std::max<s64>(value.count(), 0);
    return NetResult::Ok();
}

NetResult SocketSetSendTimeout(s32 id, std::chrono::microseconds value) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    s->snd_timeout_us = value.count();
    return NetResult::Ok();
}

NetResult SocketAbort(s32 id, u32 flags) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    // A socket waited on through an epoll is reported there as EvHup (Net Library Reference,
    // sceNetEpollWait). Lock order: GuestEpoll::mutex before the socket's locks, so the
    // registrations are copied first.
    bool hangup = false;
    {
        std::vector<std::weak_ptr<GuestEpoll>> epolls;
        {
            std::scoped_lock lock{s->reg_mutex};
            epolls = s->epolls;
        }
        for (const auto& weak : epolls) {
            const auto ep = weak.lock();
            if (!ep) {
                continue;
            }
            std::scoped_lock lock{ep->mutex};
            const auto it = ep->regs.find(id);
            if (ep->destroyed || ep->waiters.empty() || it == ep->regs.end() ||
                it->second.generation != s->generation) {
                continue;
            }
            ep->pending_hups.emplace_back(id, s->generation);
            ep->host.Wake();
            hangup = true;
        }
    }

    // Queued data is left alone: abort only interrupts waits.
    std::scoped_lock lock{s->wait_mutex};
    const auto blocked = InterruptWaiters(*s);
    bool preserved = false;
    const bool send_again = (flags & kSocketAbortPreserveSendAgain) != 0;
    if ((flags & kSocketAbortPreserveRecv) && !blocked[static_cast<size_t>(Side::Recv)]) {
        s->pending_abort[static_cast<size_t>(Side::Recv)] = true;
        preserved = true;
    }
    if ((flags & kSocketAbortPreserveSend) || send_again) {
        if (!blocked[static_cast<size_t>(Side::Send)]) {
            s->pending_abort[static_cast<size_t>(Side::Send)] = true;
            preserved = true;
        } else if (send_again) {
            s->send_again = true; // SendAll decides once it knows whether data went out
        }
    }
    // Nothing interrupted and nothing kept for later: ENOTBLK, "abort process called while
    // target is not in wait state" (Net Library Reference, error codes).
    if (!blocked[0] && !blocked[1] && !preserved && !hangup) {
        return NetResult::Fail(Error::NotBlk);
    }
    return NetResult::Ok();
}

NetResult SocketClose(s32 id) {
    const auto s = g_objects.Remove<GuestSocket>(id);
    if (!s) {
        return NetResult::Fail(Error::BadF);
    }
    // Wake every thread still blocked on this socket. The socket itself is closed by
    // ~GuestSocket once the last of them returns, never while in use.
    // DISCUSS: real BSD keeps a recv blocked across a concurrent close; waking is deliberate.
    {
        std::scoped_lock lock{s->wait_mutex};
        s->closing = true;
        InterruptWaiters(*s);
    }

    std::vector<std::weak_ptr<GuestEpoll>> epolls;
    {
        std::scoped_lock lock{s->reg_mutex};
        s->closed = true; // EpollControl(Add) checks this, so no registration can slip in later
        epolls.swap(s->epolls);
    }
    for (const auto& weak : epolls) {
        const auto ep = weak.lock();
        if (!ep) {
            continue;
        }
        std::scoped_lock lock{ep->mutex};
        const auto it = ep->regs.find(id);
        if (it != ep->regs.end() && it->second.generation == s->generation) {
            HostUnregister(*ep, *s, it->second);
            ep->regs.erase(it);
        }
    }
    return NetResult::Ok();
}

// P2P sockets
void SetP2PTransport(int family, std::shared_ptr<P2P::Transport> transport) {
    std::scoped_lock lock{g_p2p_mutex};
    g_p2p_transports[family == AF_INET6 ? 1 : 0] = std::move(transport);
}

NetResult P2PSocketCreate(int family, bool stream) {
    const auto transport = P2PTransportFor(family);
    if (!transport) {
        return NetResult::Fail(Error::NetDown); // TODO: firmware error when P2P is not up
    }
    const u32 generation = g_next_generation.fetch_add(1);
    auto s = stream ? std::make_shared<GuestSocket>(transport->CreateStream(), generation)
                    : std::make_shared<GuestSocket>(transport->CreateDatagram(), generation);
    const auto* handles = s->P2PHandles();
    if (!handles->readable.Valid() || !handles->writable.Valid()) {
        return NetResult::Fail(Error::NoBufs);
    }
    return g_objects.Insert(std::move(s));
}

NetResult P2PSocketSetProtection(s32 id, bool crypto, bool signature) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    const P2P::Protection protection{crypto, signature};
    if (s->p2p_stream) {
        s->p2p_stream->SetProtection(protection);
    } else {
        s->p2p_dgram->SetProtection(protection);
    }
    return NetResult::Ok();
}

NetResult P2PSocketGetProtection(s32 id, bool* crypto, bool* signature) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    const auto protection =
        s->p2p_stream ? s->p2p_stream->GetProtection() : s->p2p_dgram->GetProtection();
    *crypto = protection.crypto;
    *signature = protection.signature;
    return NetResult::Ok();
}

NetResult P2PSocketBind(s32 id, u16 vport) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    return FromError(s->p2p_stream ? s->p2p_stream->Bind(vport) : s->p2p_dgram->Bind(vport));
}

NetResult P2PSocketConnect(s32 id, const P2P::Endpoint& peer, u16 peer_vport) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    if (s->p2p_dgram) {
        const Error e = s->p2p_dgram->SetPeer(peer, peer_vport);
        if (e == Error::Ok) {
            s->connected = true;
        }
        return FromError(e);
    }
    if (s->listening) {
        return NetResult::Fail(Error::OpNotSupp);
    }
    if (s->nonblocking) {
        if (ConsumePendingAbort(*s, Side::Send)) {
            return NetResult::Fail(AbortedError);
        }
        const Error e = s->p2p_stream->Connect(peer, peer_vport);
        if (e == Error::Ok || e == Error::InProgress) {
            s->connected = true;
        }
        return FromError(e);
    }
    // Blocking connect: registered first, so a preserved abort stops it before it starts.
    WaitScope scope(*s, Side::Send);
    if (const Error e = scope.Entry(); e != Error::Ok) {
        return NetResult::Fail(e);
    }
    const Error started = s->p2p_stream->Connect(peer, peer_vport);
    if (started == Error::Ok || started == Error::InProgress) {
        s->connected = true;
    }
    if (started != Error::InProgress) {
        return FromError(started);
    }
    // The writable flag rises on establishment and on failure. An interrupted or timed-out
    // connect (SO_CONNECTTIMEO: EWOULDBLOCK) keeps going in the background, as on BSD.
    const auto handle = s->P2PHandles()->writable.PollHandle();
    const auto deadline = SocketDeadline(s->connect_timeout_us);
    for (;;) {
        if (scope.Interrupted()) {
            return NetResult::Fail(AbortedError);
        }
        if (const Error result = s->p2p_stream->ConnectResult(); result != Error::InProgress) {
            return FromError(result);
        }
        switch (Host::WaitOne(handle, Host::Readable, scope.Wake(), deadline)) {
        case Host::WaitResult::Failed:
            return NetResult::Fail(Host::LastError());
        case Host::WaitResult::TimedOut:
            return NetResult::Fail(Error::WouldBlock);
        default:
            break;
        }
    }
}

NetResult P2PSocketAccept(s32 id, P2P::Endpoint* peer, u16* peer_vport) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    if (s->p2p_dgram) {
        return NetResult::Fail(Error::OpNotSupp);
    }
    std::shared_ptr<P2P::StreamSocket> child;
    const NetResult r = RunBlocking(*s, Side::Recv, Host::Readable,
                                    SocketDeadline(s->accept_timeout_us), !s->nonblocking, [&] {
                                        Error e;
                                        child = s->p2p_stream->Accept(peer, peer_vport, &e);
                                        return child ? NetResult::Ok() : NetResult::Fail(e);
                                    });
    if (r.error != Error::Ok) {
        return r;
    }
    auto c = std::make_shared<GuestSocket>(std::move(child), g_next_generation.fetch_add(1));
    c->nonblocking = s->nonblocking.load();
    c->connected = true;
    {
        std::scoped_lock lock{s->attr_mutex};
        c->attributes = s->attributes;
    }
    return g_objects.Insert(std::move(c));
}

NetResult P2PSocketSendTo(s32 id, const void* buf, size_t len, bool dontwait,
                          const P2P::Endpoint* peer, u16 peer_vport, P2PSendProtection protection) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    const auto* bytes = static_cast<const u8*>(buf);
    return SendAll(
        *s, len, !dontwait && !s->nonblocking, s->p2p_stream != nullptr, [&](size_t sent) {
            const std::span<const u8> rest{bytes + sent, len - sent};
            const auto io = s->p2p_stream
                                ? s->p2p_stream->Send(rest)
                                : s->p2p_dgram->SendTo(rest, peer, peer_vport,
                                                       {protection.crypto, protection.signature});
            return io.error == Error::Ok ? NetResult::Ok(static_cast<s64>(io.bytes))
                                         : NetResult::Fail(io.error);
        });
}

NetResult P2PSocketRecvFrom(s32 id, void* buf, size_t len, bool peek, bool dontwait,
                            P2P::Endpoint* from, u16* from_vport, bool waitall) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    const bool blocking = !dontwait && !s->nonblocking;
    const auto deadline = SocketDeadline(s->rcv_timeout_us);
    auto* bytes = static_cast<u8*>(buf);
    const bool loop = blocking && waitall && !peek && s->p2p_stream != nullptr;
    return ReceiveAll(len, loop, [&](size_t got) {
        const std::span<u8> out{bytes + got, len - got};
        return RunBlocking(*s, Side::Recv, Host::Readable, deadline, blocking, [&] {
            P2P::IoResult io;
            if (s->p2p_stream) {
                io = s->p2p_stream->Recv(out, peek);
                if (io.error == Error::Ok && from && from_vport) {
                    s->p2p_stream->GetPeer(from, from_vport);
                }
            } else {
                io = s->p2p_dgram->RecvFrom(out, peek, from, from_vport);
            }
            return io.error == Error::Ok ? NetResult::Ok(static_cast<s64>(io.bytes))
                                         : NetResult::Fail(io.error);
        });
    });
}

NetResult P2PSocketGetName(s32 id, u16* udp_port, u16* vport) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    *vport = s->p2p_stream ? s->p2p_stream->BoundVport() : s->p2p_dgram->BoundVport();
    *udp_port = s->p2p_stream ? s->p2p_stream->TransportPort() : s->p2p_dgram->TransportPort();
    return NetResult::Ok();
}

NetResult P2PSocketGetPeerName(s32 id, P2P::Endpoint* peer, u16* peer_vport) {
    const auto s = g_objects.Get<GuestSocket>(id);
    if (!s || !s->IsP2P()) {
        return NetResult::Fail(s ? Error::Inval : Error::BadF);
    }
    const bool connected = s->p2p_stream ? s->p2p_stream->GetPeer(peer, peer_vport)
                                         : s->p2p_dgram->GetPeer(peer, peer_vport);
    return connected ? NetResult::Ok() : NetResult::Fail(Error::NotConn);
}

// Epoll
NetResult EpollCreate() {
    auto ep = std::make_shared<GuestEpoll>();
    if (!ep->host.Valid()) {
        return NetResult::Fail(Error::NoBufs);
    }
    return g_objects.Insert(std::move(ep));
}

namespace {

/// EpollControl for an ExternalObject: its done flag is watched until reported once.
NetResult ExternalEpollControl(GuestEpoll& ep, const std::shared_ptr<GuestEpoll>& ep_ref, s32 id,
                               ExternalObject& x, EpollOp op, u32 events, u64 data) {
    std::scoped_lock ep_lock{ep.mutex};
    std::scoped_lock x_lock{x.reg_mutex};
    if (x.closed) {
        return NetResult::Fail(Error::BadF);
    }
    const auto it = ep.regs.find(id);
    const bool exists = it != ep.regs.end();
    switch (op) {
    case EpollOp::Add: {
        if (exists) {
            return NetResult::Fail(Error::Exist);
        }
        // Only a registration with EvIn waits for the lookup (Net Library Reference,
        // SceNetEpollEvent); EvOut means nothing for a resolver.
        const bool arm = (events & Host::EvIn) != 0;
        if (arm) {
            const auto tag = MakeTag(id, x.generation);
            if (const Error e = ep.host.Add(x.done.PollHandle(), tag, Host::EvIn, 0);
                e != Error::Ok) {
                return NetResult::Fail(e);
            }
        }
        ep.regs.emplace(
            id, GuestEpoll::Registration{x.generation, data, events, false, false, true, arm});
        x.epolls.push_back(ep_ref);
        return NetResult::Ok();
    }
    case EpollOp::Modify: {
        if (!exists) {
            return NetResult::Fail(Error::NoEnt);
        }
        auto& reg = it->second;
        const bool want = (events & Host::EvIn) != 0 && !reg.reported;
        if (want && !reg.armed) {
            const auto tag = MakeTag(id, x.generation);
            if (const Error e = ep.host.Add(x.done.PollHandle(), tag, Host::EvIn, 0);
                e != Error::Ok) {
                return NetResult::Fail(e);
            }
            reg.armed = true;
        } else if (!want && reg.armed) {
            ep.host.Remove(x.done.PollHandle());
            reg.armed = false;
        }
        reg.data = data;
        reg.events = events;
        return NetResult::Ok();
    }
    case EpollOp::Delete:
        if (!exists) {
            return NetResult::Fail(Error::NoEnt);
        }
        if (it->second.armed) {
            ep.host.Remove(x.done.PollHandle());
        }
        ep.regs.erase(it);
        std::erase_if(x.epolls, [&](const std::weak_ptr<GuestEpoll>& w) {
            const auto p = w.lock();
            return !p || p == ep_ref;
        });
        return NetResult::Ok();
    }
    return NetResult::Fail(Error::Inval);
}

} // namespace

// EpollControl on an epoll object, which need not be in the object table (SocketSelect).
static NetResult EpollControlOn(const std::shared_ptr<GuestEpoll>& ep, EpollOp op, s32 socket_id,
                                u32 events, u32 flags, u64 data) {
    if (ep) {
        if (const auto x = g_objects.Get<ExternalObject>(socket_id)) {
            return ExternalEpollControl(*ep, ep, socket_id, *x, op, events, data);
        }
    }
    const auto s = g_objects.Get<GuestSocket>(socket_id);
    if (!ep || !s) {
        return NetResult::Fail(Error::BadF);
    }

    std::scoped_lock ep_lock{ep->mutex};
    std::scoped_lock s_lock{s->reg_mutex};
    if (s->closed) {
        return NetResult::Fail(Error::BadF);
    }
    const auto it = ep->regs.find(socket_id);
    const bool exists = it != ep->regs.end();

    switch (op) {
    case EpollOp::Add: {
        if (exists) {
            return NetResult::Fail(Error::Exist);
        }
        GuestEpoll::Registration reg{s->generation, data, events, s->IsP2P(), false, false, false};
        const Error e = HostRegister(*ep, socket_id, *s, events, flags, reg);
        if (e == Error::Ok) {
            ep->regs.emplace(socket_id, reg);
            s->epolls.push_back(ep);
        }
        return FromError(e);
    }
    case EpollOp::Modify: {
        if (!exists) {
            return NetResult::Fail(Error::NoEnt);
        }
        const Error e = HostModify(*ep, socket_id, *s, events, flags, it->second);
        if (e == Error::Ok) {
            it->second.data = data;
            it->second.events = events;
        }
        return FromError(e);
    }
    case EpollOp::Delete: {
        if (!exists) {
            return NetResult::Fail(Error::NoEnt);
        }
        const Error e = HostUnregister(*ep, *s, it->second);
        ep->regs.erase(it);
        std::erase_if(s->epolls, [&](const std::weak_ptr<GuestEpoll>& w) {
            const auto p = w.lock();
            return !p || p == ep;
        });
        return FromError(e);
    }
    }
    return NetResult::Fail(Error::Inval);
}

NetResult EpollControl(s32 eid, EpollOp op, s32 socket_id, u32 events, u32 flags, u64 data) {
    return EpollControlOn(g_objects.Get<GuestEpoll>(eid), op, socket_id, events, flags, data);
}

/// EpollWait on an epoll object, which need not be in the object table (SocketSelect).
static NetResult EpollWaitOn(const std::shared_ptr<GuestEpoll>& ep, std::span<GuestEpollEvent> out,
                             s64 timeout_us) {
    if (!ep) {
        return NetResult::Fail(Error::BadF);
    }
    if (out.empty()) {
        return NetResult::Fail(Error::Inval);
    }
    const Host::Deadline deadline =
        timeout_us < 0 ? Host::Deadline{}
                       : Host::Clock::now() + std::chrono::microseconds{timeout_us};
    std::array<Host::EpollEvent, 64> buf;
    const auto host_out = std::span{buf}.first(std::min(out.size(), buf.size()));

    // Register the wait so EpollAbort can interrupt it (waits are woken through the host
    // epoll's wake handle, drained once every interrupted wait has left).
    Waiter me{};
    {
        std::scoped_lock lock{ep->mutex};
        if (ep->destroyed) {
            return NetResult::Fail(Error::BadF);
        }
        if (std::exchange(ep->pending_abort, false)) {
            return NetResult::Fail(AbortedError);
        }
        ep->waiters.push_back(&me);
    }
    struct Unregister {
        GuestEpoll& ep;
        Waiter& me;
        ~Unregister() {
            std::scoped_lock lock{ep.mutex};
            std::erase(ep.waiters, &me);
            if (me.aborted && --ep.flagged_waiters == 0 && ep.WakeIdle()) {
                ep.host.DrainWake();
            }
        }
    } unregister{*ep, me};
    // Adds (ep->mutex held) the hangups of sockets aborted during a wait, merged with any
    // other event of the same socket.
    const auto add_hangups = [&](size_t n) {
        auto& hups = ep->pending_hups;
        size_t kept = 0;
        for (const auto& [id, generation] : hups) {
            const auto it = ep->regs.find(id);
            if (it == ep->regs.end() || it->second.generation != generation) {
                continue; // deleted or closed since: dropped
            }
            const auto existing =
                std::find_if(out.begin(), out.begin() + n, [&](auto& e) { return e.ident == id; });
            if (existing != out.begin() + n) {
                existing->events |= Host::EvHup;
            } else if (n < out.size()) {
                out[n++] = {Host::EvHup, id, it->second.data};
            } else {
                hups[kept++] = {id, generation}; // no room: for the next wait
            }
        }
        hups.resize(kept);
        if (ep->WakeIdle()) {
            ep->host.DrainWake();
        }
        return n;
    };

    for (;;) {
        {
            std::scoped_lock lock{ep->mutex};
            if (me.aborted) {
                return NetResult::Fail(AbortedError);
            }
            if (!ep->pending_hups.empty()) {
                if (const size_t n = add_hangups(0); n > 0) {
                    return NetResult::Ok(static_cast<s64>(n));
                }
            }
        }
        const auto r = ep->host.Wait(host_out, deadline);
        if (r.error != Error::Ok) {
            return NetResult::Fail(r.error);
        }
        // A wake meant for another wait on the same epoll is ignored; events in the same
        // batch are still reported.
        size_t n = 0;
        {
            std::scoped_lock lock{ep->mutex};
            for (s32 i = 0; i < r.count; ++i) {
                const auto socket_id = static_cast<s32>(host_out[i].tag & 0x7fffffffu);
                const auto generation = static_cast<u32>(host_out[i].tag >> 32);
                const auto it = ep->regs.find(socket_id);
                // Skip events for sockets deleted or closed between the host wait and now.
                if (it == ep->regs.end() || it->second.generation != generation) {
                    continue;
                }
                u32 events = host_out[i].events;
                if (it->second.external) {
                    // Finished: reported once, then no longer watched (the flag stays set).
                    const auto x = g_objects.Get<ExternalObject>(socket_id);
                    if (!x || x->generation != generation || !it->second.armed) {
                        continue;
                    }
                    ep->host.Remove(x->done.PollHandle());
                    it->second.armed = false;
                    it->second.reported = true;
                    // SCE_NET_EPOLLIN comes with SCE_NET_EPOLLDESCID; an aborted lookup also
                    // reports SCE_NET_EPOLLHUP.
                    events = Host::EvIn | Host::EvDescId | (x->hangup ? Host::EvHup : 0u);
                } else if (it->second.p2p) {
                    // The handle only says "something changed": compute the real state.
                    // Errors and hangups are always reported, as with epoll.
                    const auto s = g_objects.Get<GuestSocket>(socket_id);
                    if (!s || s->generation != generation) {
                        continue;
                    }
                    events = s->P2PEvents() & (it->second.events | Host::EvErr | Host::EvHup);
                    if (events == 0) {
                        continue; // already consumed by another thread
                    }
                }
                // A P2P socket can fire through both of its handles: report it once.
                const auto existing = std::find_if(out.begin(), out.begin() + n,
                                                   [&](auto& e) { return e.ident == socket_id; });
                if (existing != out.begin() + n) {
                    existing->events |= events;
                } else {
                    out[n++] = {events, socket_id, it->second.data};
                }
            }
            if (!ep->pending_hups.empty()) {
                n = add_hangups(n);
            }
        }
        if (n > 0) {
            return NetResult::Ok(static_cast<s64>(n));
        }
        if (r.count == 0 && !r.woken) {
            return NetResult::Ok(0); // the deadline passed
        }
        // Woken, or every event was stale: check for abort and wait out the remaining time.
    }
}

NetResult EpollWait(s32 eid, std::span<GuestEpollEvent> out, s64 timeout_us) {
    return EpollWaitOn(g_objects.Get<GuestEpoll>(eid), out, timeout_us);
}

namespace {

/// Flags every wait on `ep` and wakes them (ep.mutex held). Returns whether there were any.
bool InterruptEpollWaiters(GuestEpoll& ep) {
    for (Waiter* w : ep.waiters) {
        if (!w->aborted) {
            w->aborted = true;
            ++ep.flagged_waiters;
        }
    }
    if (ep.waiters.empty()) {
        return false;
    }
    ep.host.Wake();
    return true;
}

} // namespace

NetResult EpollAbort(s32 eid, u32 flags) {
    const auto ep = g_objects.Get<GuestEpoll>(eid);
    if (!ep) {
        return NetResult::Fail(Error::BadF);
    }
    std::scoped_lock lock{ep->mutex};
    if (InterruptEpollWaiters(*ep)) {
        return NetResult::Ok();
    }
    if (flags & kEpollAbortPreserve) {
        ep->pending_abort = true;
        return NetResult::Ok();
    }
    return NetResult::Fail(Error::NotBlk); // nothing waiting, as for SocketAbort
}

NetResult EpollDestroy(s32 eid) {
    const auto ep = g_objects.Remove<GuestEpoll>(eid);
    if (!ep) {
        return NetResult::Fail(Error::BadF);
    }
    // Release any waiter; the host instance is freed when the last one returns. Sockets only
    // hold weak references, which simply expire.
    std::scoped_lock lock{ep->mutex};
    ep->destroyed = true;
    InterruptEpollWaiters(*ep);
    return NetResult::Ok();
}

// select()
NetResult SocketSelect(std::span<SelectEntry> entries, s64 timeout_us) {
    for (auto& e : entries) {
        e.ready = 0;
    }
    if (entries.empty()) {
        return NetResult::Ok(0); // nothing to wait for: the caller sleeps if it wants to
    }
    // A private epoll, never in the object table: it takes no id, and each socket is removed
    // from it again before returning.
    const auto ep = std::make_shared<GuestEpoll>();
    if (!ep->host.Valid()) {
        return NetResult::Fail(Error::NoBufs);
    }
    std::vector<size_t> registered;
    const auto unregister_all = [&] {
        for (const size_t i : registered) {
            EpollControlOn(ep, EpollOp::Delete, entries[i].id, 0, 0, 0);
        }
        registered.clear();
    };
    for (size_t i = 0; i < entries.size(); ++i) {
        if (!g_objects.Get<GuestSocket>(entries[i].id)) {
            unregister_all();
            return NetResult::Fail(Error::BadF);
        }
        // The same socket may be listed once only; merge duplicates into the first entry.
        const auto first =
            std::find_if(entries.begin(), entries.begin() + i,
                         [&](const SelectEntry& e) { return e.id == entries[i].id; });
        if (first != entries.begin() + i) {
            continue;
        }
        u32 interest = 0;
        for (size_t j = i; j < entries.size(); ++j) {
            if (entries[j].id == entries[i].id) {
                interest |= entries[j].interest;
            }
        }
        const auto r = EpollControlOn(ep, EpollOp::Add, entries[i].id, interest, 0, i);
        if (r.error != Error::Ok) {
            unregister_all();
            return r;
        }
        registered.push_back(i);
    }

    // The first wait honours the timeout; the following ones only collect whatever else is
    // ready (a wait reports at most 64 sockets), each reported socket leaving the epoll.
    std::vector<GuestEpollEvent> out(registered.size());
    std::vector<size_t> still;
    s64 wait = timeout_us;
    for (;;) {
        const auto r = EpollWaitOn(ep, out, wait);
        if (r.error != Error::Ok) {
            unregister_all();
            return r;
        }
        if (r.value == 0) {
            break;
        }
        for (s64 k = 0; k < r.value; ++k) {
            const s32 id = out[k].ident;
            // Like BSD: an error or hangup makes the socket both readable and writable (the
            // next call returns the error without blocking).
            u32 ready = out[k].events;
            if (ready & (Host::EvErr | Host::EvHup)) {
                ready |= Host::EvIn | Host::EvOut;
            }
            for (auto& e : entries) {
                if (e.id == id) {
                    e.ready = ready & e.interest;
                }
            }
            EpollControlOn(ep, EpollOp::Delete, id, 0, 0, 0);
            std::erase_if(registered, [&](size_t i) { return entries[i].id == id; });
        }
        if (registered.empty()) {
            break;
        }
        wait = 0;
    }
    unregister_all();
    s64 count = 0;
    for (const auto& e : entries) {
        count += std::popcount(e.ready);
    }
    return NetResult::Ok(count);
}

} // namespace Core::Net
