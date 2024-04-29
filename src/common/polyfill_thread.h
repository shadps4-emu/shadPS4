// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

//
// TODO: remove this file when jthread is supported by all compilation targets
//

#pragma once

#include <version>

#ifdef __cpp_lib_jthread

#include <chrono>
#include <condition_variable>
#include <stop_token>
#include <thread>
#include <utility>

namespace Common {

template <typename Condvar, typename Lock, typename Pred>
void CondvarWait(Condvar& cv, std::unique_lock<Lock>& lk, std::stop_token token, Pred&& pred) {
    cv.wait(lk, token, std::forward<Pred>(pred));
}

template <typename Rep, typename Period>
bool StoppableTimedWait(std::stop_token token, const std::chrono::duration<Rep, Period>& rel_time) {
    std::condition_variable_any cv;
    std::mutex m;

    // Perform the timed wait.
    std::unique_lock lk{m};
    return !cv.wait_for(lk, token, rel_time, [&] { return token.stop_requested(); });
}

} // namespace Common

#else

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>

namespace std {
namespace polyfill {

using stop_state_callback = size_t;

class stop_state {
public:
    stop_state() = default;
    ~stop_state() = default;

    bool request_stop() {
        unique_lock lk{m_lock};

        if (m_stop_requested) {
            // Already set, nothing to do.
            return false;
        }

        // Mark stop requested.
        m_stop_requested = true;

        while (!m_callbacks.empty()) {
            // Get an iterator to the first element.
            const auto it = m_callbacks.begin();

            // Move the callback function out of the map.
            function<void()> f;
            swap(it->second, f);

            // Erase the now-empty map element.
            m_callbacks.erase(it);

            // Run the callback.
            if (f) {
                f();
            }
        }

        return true;
    }

    bool stop_requested() const {
        unique_lock lk{m_lock};
        return m_stop_requested;
    }

    stop_state_callback insert_callback(function<void()> f) {
        unique_lock lk{m_lock};

        if (m_stop_requested) {
            // Stop already requested. Don't insert anything,
            // just run the callback synchronously.
            if (f) {
                f();
            }
            return 0;
        }

        // Insert the callback.
        stop_state_callback ret = ++m_next_callback;
        m_callbacks.emplace(ret, std::move(f));
        return ret;
    }

    void remove_callback(stop_state_callback cb) {
        unique_lock lk{m_lock};
        m_callbacks.erase(cb);
    }

private:
    mutable recursive_mutex m_lock;
    map<stop_state_callback, function<void()>> m_callbacks;
    stop_state_callback m_next_callback{0};
    bool m_stop_requested{false};
};

} // namespace polyfill

class stop_token;
class stop_source;
struct nostopstate_t {
    explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};

template <class Callback>
class stop_callback;

class stop_token {
public:
    stop_token() noexcept = default;

    stop_token(const stop_token&) noexcept = default;
    stop_token(stop_token&&) noexcept = default;
    stop_token& operator=(const stop_token&) noexcept = default;
    stop_token& operator=(stop_token&&) noexcept = default;
    ~stop_token() = default;

    void swap(stop_token& other) noexcept {
        m_stop_state.swap(other.m_stop_state);
    }

    [[nodiscard]] bool stop_requested() const noexcept {
        return m_stop_state && m_stop_state->stop_requested();
    }
    [[nodiscard]] bool stop_possible() const noexcept {
        return m_stop_state != nullptr;
    }

private:
    friend class stop_source;
    template <typename Callback>
    friend class stop_callback;
    stop_token(shared_ptr<polyfill::stop_state> stop_state) : m_stop_state(std::move(stop_state)) {}

private:
    shared_ptr<polyfill::stop_state> m_stop_state;
};

class stop_source {
public:
    stop_source() : m_stop_state(make_shared<polyfill::stop_state>()) {}
    explicit stop_source(nostopstate_t) noexcept {}

    stop_source(const stop_source&) noexcept = default;
    stop_source(stop_source&&) noexcept = default;
    stop_source& operator=(const stop_source&) noexcept = default;
    stop_source& operator=(stop_source&&) noexcept = default;
    ~stop_source() = default;
    void swap(stop_source& other) noexcept {
        m_stop_state.swap(other.m_stop_state);
    }

    [[nodiscard]] stop_token get_token() const noexcept {
        return stop_token(m_stop_state);
    }
    [[nodiscard]] bool stop_possible() const noexcept {
        return m_stop_state != nullptr;
    }
    [[nodiscard]] bool stop_requested() const noexcept {
        return m_stop_state && m_stop_state->stop_requested();
    }
    bool request_stop() noexcept {
        return m_stop_state && m_stop_state->request_stop();
    }

private:
    friend class jthread;
    explicit stop_source(shared_ptr<polyfill::stop_state> stop_state)
        : m_stop_state(std::move(stop_state)) {}

private:
    shared_ptr<polyfill::stop_state> m_stop_state;
};

template <typename Callback>
class stop_callback {
    static_assert(is_nothrow_destructible_v<Callback>);
    static_assert(is_invocable_v<Callback>);

public:
    using callback_type = Callback;

    template <typename C>
        requires constructible_from<Callback, C>
    explicit stop_callback(const stop_token& st,
                           C&& cb) noexcept(is_nothrow_constructible_v<Callback, C>)
        : m_stop_state(st.m_stop_state) {
        if (m_stop_state) {
            m_callback = m_stop_state->insert_callback(std::move(cb));
        }
    }
    template <typename C>
        requires constructible_from<Callback, C>
    explicit stop_callback(stop_token&& st,
                           C&& cb) noexcept(is_nothrow_constructible_v<Callback, C>)
        : m_stop_state(std::move(st.m_stop_state)) {
        if (m_stop_state) {
            m_callback = m_stop_state->insert_callback(std::move(cb));
        }
    }
    ~stop_callback() {
        if (m_stop_state && m_callback) {
            m_stop_state->remove_callback(m_callback);
        }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback(stop_callback&&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;
    stop_callback& operator=(stop_callback&&) = delete;

private:
    shared_ptr<polyfill::stop_state> m_stop_state;
    polyfill::stop_state_callback m_callback;
};

template <typename Callback>
stop_callback(stop_token, Callback) -> stop_callback<Callback>;

class jthread {
public:
    using id = thread::id;
    using native_handle_type = thread::native_handle_type;

    jthread() noexcept = default;

    template <typename F, typename... Args,
              typename = enable_if_t<!is_same_v<remove_cvref_t<F>, jthread>>>
    explicit jthread(F&& f, Args&&... args)
        : m_stop_state(make_shared<polyfill::stop_state>()),
          m_thread(make_thread(std::forward<F>(f), std::forward<Args>(args)...)) {}

    ~jthread() {
        if (joinable()) {
            request_stop();
            join();
        }
    }

    jthread(const jthread&) = delete;
    jthread(jthread&&) noexcept = default;
    jthread& operator=(const jthread&) = delete;

    jthread& operator=(jthread&& other) noexcept {
        m_thread.swap(other.m_thread);
        m_stop_state.swap(other.m_stop_state);
        return *this;
    }

    void swap(jthread& other) noexcept {
        m_thread.swap(other.m_thread);
        m_stop_state.swap(other.m_stop_state);
    }
    [[nodiscard]] bool joinable() const noexcept {
        return m_thread.joinable();
    }
    void join() {
        m_thread.join();
    }
    void detach() {
        m_thread.detach();
        m_stop_state.reset();
    }

    [[nodiscard]] id get_id() const noexcept {
        return m_thread.get_id();
    }
    [[nodiscard]] native_handle_type native_handle() {
        return m_thread.native_handle();
    }
    [[nodiscard]] stop_source get_stop_source() noexcept {
        return stop_source(m_stop_state);
    }
    [[nodiscard]] stop_token get_stop_token() const noexcept {
        return stop_source(m_stop_state).get_token();
    }
    bool request_stop() noexcept {
        return get_stop_source().request_stop();
    }
    [[nodiscard]] static unsigned int hardware_concurrency() noexcept {
        return thread::hardware_concurrency();
    }

private:
    template <typename F, typename... Args>
    thread make_thread(F&& f, Args&&... args) {
        if constexpr (is_invocable_v<decay_t<F>, stop_token, decay_t<Args>...>) {
            return thread(std::forward<F>(f), get_stop_token(), std::forward<Args>(args)...);
        } else {
            return thread(std::forward<F>(f), std::forward<Args>(args)...);
        }
    }

    shared_ptr<polyfill::stop_state> m_stop_state;
    thread m_thread;
};

} // namespace std

namespace Common {

template <typename Condvar, typename Lock, typename Pred>
void CondvarWait(Condvar& cv, std::unique_lock<Lock>& lk, std::stop_token token, Pred pred) {
    if (token.stop_requested()) {
        return;
    }

    std::stop_callback callback(token, [&] {
        { std::scoped_lock lk2{*lk.mutex()}; }
        cv.notify_all();
    });

    cv.wait(lk, [&] { return pred() || token.stop_requested(); });
}

template <typename Rep, typename Period>
bool StoppableTimedWait(std::stop_token token, const std::chrono::duration<Rep, Period>& rel_time) {
    if (token.stop_requested()) {
        return false;
    }

    bool stop_requested = false;
    std::condition_variable cv;
    std::mutex m;

    std::stop_callback cb(token, [&] {
        // Wake up the waiting thread.
        {
            std::scoped_lock lk{m};
            stop_requested = true;
        }
        cv.notify_one();
    });

    // Perform the timed wait.
    std::unique_lock lk{m};
    return !cv.wait_for(lk, rel_time, [&] { return stop_requested; });
}

} // namespace Common

#endif
