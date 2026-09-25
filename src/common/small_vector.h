// SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

template <class T, std::size_t N>
class SmallVector {
    static_assert(N > 0, "SmallVector inline capacity must be > 0");
    static constexpr bool triv_reloc =
        std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;

    constexpr SmallVector() noexcept : m_data(m_inline.buf), m_size(0), m_capacity(N) {}

    constexpr explicit SmallVector(size_type n) : SmallVector() {
        reserve(n);
        for (size_type i = 0; i < n; ++i) {
            std::construct_at(m_data + i);
        }
        m_size = n;
    }

    constexpr SmallVector(const SmallVector& o) : SmallVector() {
        reserve(o.m_size);
        copy_construct(m_data, o.m_data, o.m_size);
        m_size = o.m_size;
    }
    constexpr SmallVector(SmallVector&& o) noexcept : SmallVector() {
        if (o.is_inline()) {
            reloc_construct(m_data, o.m_data, o.m_size);
            m_size = o.m_size;
            o.m_size = 0;
        } else {
            m_data = o.m_data;
            m_size = o.m_size;
            m_capacity = o.m_capacity;
            o.m_data = o.m_inline.buf;
            o.m_size = 0;
            o.m_capacity = N;
        }
    }
    constexpr SmallVector& operator=(const SmallVector& o) {
        if (this == &o) {
            return *this;
        }
        clear();
        reserve(o.m_size);
        copy_construct(m_data, o.m_data, o.m_size);
        m_size = o.m_size;
        return *this;
    }
    constexpr SmallVector& operator=(SmallVector&& o) noexcept {
        if (this == &o)
            return *this;
        reset();
        if (o.is_inline()) {
            reloc_construct(m_data, o.m_data, o.m_size);
            m_size = o.m_size;
            o.m_size = 0;
        } else {
            m_data = o.m_data;
            m_size = o.m_size;
            m_capacity = o.m_capacity;
            o.m_data = o.m_inline.buf;
            o.m_size = 0;
            o.m_capacity = N;
        }
        return *this;
    }
    constexpr ~SmallVector() {
        reset();
    }

    bool operator==(const SmallVector& other) const {
        if (m_size != other.m_size) {
            return false;
        }
        for (size_type i = 0; i < m_size; ++i) {
            if (!(m_data[i] == other.m_data[i])) {
                return false;
            }
        }
        return true;
    }

    constexpr reference operator[](size_type i) noexcept {
        return m_data[i];
    }
    constexpr const_reference operator[](size_type i) const noexcept {
        return m_data[i];
    }
    constexpr reference front() noexcept {
        return m_data[0];
    }
    constexpr const_reference front() const noexcept {
        return m_data[0];
    }
    constexpr reference back() noexcept {
        return m_data[m_size - 1];
    }
    constexpr const_reference back() const noexcept {
        return m_data[m_size - 1];
    }
    constexpr pointer data() noexcept {
        return m_data;
    }
    constexpr const_pointer data() const noexcept {
        return m_data;
    }

    constexpr iterator begin() noexcept {
        return m_data;
    }
    constexpr iterator end() noexcept {
        return m_data + m_size;
    }
    constexpr const_iterator begin() const noexcept {
        return m_data;
    }
    constexpr const_iterator end() const noexcept {
        return m_data + m_size;
    }
    constexpr const_iterator cbegin() const noexcept {
        return m_data;
    }
    constexpr const_iterator cend() const noexcept {
        return m_data + m_size;
    }

    constexpr bool empty() const noexcept {
        return m_size == 0;
    }
    constexpr size_type size() const noexcept {
        return m_size;
    }
    constexpr size_type capacity() const noexcept {
        return m_capacity;
    }
    static constexpr size_type inlinecapacity() noexcept {
        return N;
    }
    constexpr bool is_inline() const noexcept {
        return m_data == m_inline.buf;
    }

    constexpr void reserve(size_type n) {
        if (n > m_capacity) {
            grow_to(n);
        }
    }

    constexpr void clear() noexcept {
        std::destroy(m_data, m_data + m_size);
        m_size = 0;
    }

    constexpr void push_back(const T& v) {
        if (m_size == m_capacity) {
            grow_to(m_capacity + m_capacity / 2 + 1);
        }
        std::construct_at(m_data + m_size, v);
        ++m_size;
    }
    constexpr void push_back(T&& v) {
        if (m_size == m_capacity) {
            grow_to(m_capacity + m_capacity / 2 + 1);
        }
        std::construct_at(m_data + m_size, std::move(v));
        ++m_size;
    }
    template <class... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (m_size == m_capacity) {
            grow_to(m_capacity + m_capacity / 2 + 1);
        }
        T* p = std::construct_at(m_data + m_size, std::forward<Args>(args)...);
        ++m_size;
        return *p;
    }
    constexpr void pop_back() noexcept {
        --m_size;
        std::destroy_at(m_data + m_size);
    }

    constexpr void resize(size_type n) {
        if (n < m_size) {
            std::destroy(m_data + n, m_data + m_size);
        } else if (n > m_size) {
            reserve(n);
            for (size_type i = m_size; i < n; ++i) {
                std::construct_at(m_data + i);
            }
        }
        m_size = n;
    }

    constexpr iterator insert(const_iterator pos, const T* first, const T* last) {
        const size_type idx = static_cast<size_type>(pos - m_data);
        const size_type cnt = static_cast<size_type>(last - first);
        if (cnt == 0) {
            return m_data + idx;
        }
        if (m_size + cnt > m_capacity) {
            grow_to(m_size + cnt);
        }
        open_gap(idx, cnt);
        for (size_type i = 0; i < cnt; ++i) {
            std::construct_at(m_data + idx + i, first[i]);
        }
        m_size += cnt;
        return m_data + idx;
    }
    constexpr iterator insert(const_iterator pos, const T& v) {
        return insert(pos, &v, &v + 1);
    }

    constexpr iterator erase(const_iterator first, const_iterator last) {
        const size_type i = static_cast<size_type>(first - m_data);
        const size_type j = static_cast<size_type>(last - m_data);
        if (i == j) {
            return m_data + i;
        }
        const size_type tail = m_size - j;
        std::destroy(m_data + i, m_data + j);
        if (triv_reloc && !std::is_constant_evaluated()) {
            if (tail) {
                __builtin_memmove(m_data + i, m_data + j, tail * sizeof(T));
            }
        } else {
            for (size_type k = 0; k < tail; ++k) {
                std::construct_at(m_data + i + k, std::move(m_data[j + k]));
                std::destroy_at(m_data + j + k);
            }
        }
        m_size -= (j - i);
        return m_data + i;
    }
    constexpr iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

private:
    static constexpr void copy_construct(T* dst, const T* src, size_type n) {
        if (triv_reloc && !std::is_constant_evaluated()) {
            if (n) {
                __builtin_memcpy(dst, src, n * sizeof(T));
            }
        } else
            for (size_type i = 0; i < n; ++i) {
                std::construct_at(dst + i, src[i]);
            }
    }
    static constexpr void reloc_construct(T* dst, T* src, size_type n) {
        if (triv_reloc && !std::is_constant_evaluated()) {
            if (n) {
                __builtin_memcpy(dst, src, n * sizeof(T));
            }
        } else
            for (size_type i = 0; i < n; ++i) {
                std::construct_at(dst + i, std::move(src[i]));
                std::destroy_at(src + i);
            }
    }
    constexpr void reset() noexcept {
        std::destroy(m_data, m_data + m_size);
        if (!is_inline()) {
            std::allocator<T>{}.deallocate(m_data, m_capacity);
        }
        m_data = m_inline.buf;
        m_size = 0;
        m_capacity = N;
    }
    constexpr void grow_to(size_type new_cap) {
        if (new_cap < N) {
            new_cap = N;
        }
        T* nb = std::allocator<T>{}.allocate(new_cap);
        reloc_construct(nb, m_data, m_size);
        if (!is_inline()) {
            std::allocator<T>{}.deallocate(m_data, m_capacity);
        }
        m_data = nb;
        m_capacity = new_cap;
    }
    constexpr void open_gap(size_type idx, size_type cnt) {
        const size_type tail = m_size - idx;
        if (triv_reloc && !std::is_constant_evaluated()) {
            if (tail) {
                __builtin_memmove(m_data + idx + cnt, m_data + idx, tail * sizeof(T));
            }
        } else {
            for (size_type k = tail; k-- > 0;) {
                std::construct_at(m_data + idx + cnt + k, std::move(m_data[idx + k]));
                std::destroy_at(m_data + idx + k);
            }
        }
    }

    union Storage {
        constexpr Storage() : dummy{} {}
        constexpr ~Storage() {}
        char dummy;
        T buf[N];
    };

    T* m_data;
    size_type m_size;
    size_type m_capacity;
    Storage m_inline;
};
