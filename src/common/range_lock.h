// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <iterator>
#include <mutex>

namespace Common {

// From boost thread locking

template <typename Iterator>
struct RangeLockGuard {
    Iterator begin;
    Iterator end;

    RangeLockGuard(Iterator begin_, Iterator end_) : begin(begin_), end(end_) {
        LockRange(begin, end);
    }

    void release() {
        begin = end;
    }

    ~RangeLockGuard() {
        for (; begin != end; ++begin) {
            begin->unlock();
        }
    }
};

template <typename Iterator>
Iterator TryLockRange(Iterator begin, Iterator end) {
    using LockType = typename std::iterator_traits<Iterator>::value_type;

    if (begin == end) {
        return end;
    }

    std::unique_lock<LockType> guard(*begin, std::try_to_lock);
    if (!guard.owns_lock()) {
        return begin;
    }

    Iterator failed = TryLockRange(++begin, end);
    if (failed == end) {
        guard.release();
    }

    return failed;
}

template <typename Iterator>
void LockRange(Iterator begin, Iterator end) {
    using LockType = typename std::iterator_traits<Iterator>::value_type;

    if (begin == end) {
        return;
    }

    bool start_with_begin = true;
    Iterator second = begin;
    ++second;
    Iterator next = second;

    while (true) {
        std::unique_lock<LockType> begin_lock(*begin, std::defer_lock);
        if (start_with_begin) {
            begin_lock.lock();

            const Iterator failed_lock = TryLockRange(next, end);
            if (failed_lock == end) {
                begin_lock.release();
                return;
            }

            start_with_begin = false;
            next = failed_lock;
        } else {
            RangeLockGuard<Iterator> guard(next, end);

            if (begin_lock.try_lock()) {
                const Iterator failed_lock = TryLockRange(second, next);
                if (failed_lock == next) {
                    begin_lock.release();
                    guard.release();
                    return;
                }

                start_with_begin = false;
                next = failed_lock;
            } else {
                start_with_begin = true;
                next = second;
            }
        }
    }
}

} // namespace Common