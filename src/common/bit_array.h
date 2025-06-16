// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include "common/types.h"

#ifdef __AVX2__
#define BIT_ARRAY_USE_AVX
#include <immintrin.h>
#endif

namespace Common {

template <size_t N>
class BitArray {
    static_assert(N % 64 == 0, "BitArray size must be a multiple of 64 bits.");

    static constexpr size_t BITS_PER_WORD = 64;
    static constexpr size_t WORD_COUNT = N / BITS_PER_WORD;
    static constexpr size_t WORDS_PER_AVX = 4;
    static constexpr size_t AVX_WORD_COUNT = WORD_COUNT / WORDS_PER_AVX;

public:
    using Range = std::pair<size_t, size_t>;

    class Iterator {
    public:
        explicit Iterator(const BitArray& bit_array_, u64 start) : bit_array(bit_array_) {
            range = bit_array.FirstRangeFrom(start);
        }

        Iterator& operator++() {
            range = bit_array.FirstRangeFrom(range.second);
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return range == other.range;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        const Range& operator*() const {
            return range;
        }

        const Range* operator->() const {
            return &range;
        }

    private:
        const BitArray& bit_array;
        Range range;
    };

    using const_iterator = Iterator;
    using iterator_category = std::forward_iterator_tag;
    using value_type = Range;
    using difference_type = std::ptrdiff_t;
    using pointer = const Range*;
    using reference = const Range&;

    BitArray() = default;
    BitArray(const BitArray& other) = default;
    BitArray& operator=(const BitArray& other) = default;
    BitArray(BitArray&& other) noexcept = default;
    BitArray& operator=(BitArray&& other) noexcept = default;
    ~BitArray() = default;

    BitArray(const BitArray& other, size_t start, size_t end) {
        if (start >= end || end > N) {
            return;
        }
        const size_t first_word = start / BITS_PER_WORD;
        const size_t last_word = (end - 1) / BITS_PER_WORD;
        const size_t start_bit = start % BITS_PER_WORD;
        const size_t end_bit = (end - 1) % BITS_PER_WORD;
        const u64 start_mask = ~((1ULL << start_bit) - 1);
        const u64 end_mask = end_bit == BITS_PER_WORD - 1 ? ~0ULL : (1ULL << (end_bit + 1)) - 1;
        if (first_word == last_word) {
            data[first_word] = other.data[first_word] & (start_mask & end_mask);
        } else {
            data[first_word] = other.data[first_word] & start_mask;
            size_t i = first_word + 1;
#ifdef BIT_ARRAY_USE_AVX
            for (; i + WORDS_PER_AVX <= last_word; i += WORDS_PER_AVX) {
                const __m256i current =
                    _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&other.data[i]));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(&data[i]), current);
            }
#endif
            for (; i < last_word; ++i) {
                data[i] = other.data[i];
            }
            data[last_word] = other.data[last_word] & end_mask;
        }
    }

    BitArray(const BitArray& other, const Range& range)
        : BitArray(other, range.first, range.second) {}

    const_iterator begin() const {
        return Iterator(*this, 0);
    }
    const_iterator end() const {
        return Iterator(*this, N);
    }

    inline constexpr void Set(size_t idx) {
        data[idx / BITS_PER_WORD] |= (1ULL << (idx % BITS_PER_WORD));
    }

    inline constexpr void Unset(size_t idx) {
        data[idx / BITS_PER_WORD] &= ~(1ULL << (idx % BITS_PER_WORD));
    }

    inline constexpr bool Get(size_t idx) const {
        return (data[idx / BITS_PER_WORD] & (1ULL << (idx % BITS_PER_WORD))) != 0;
    }

    inline void SetRange(size_t start, size_t end) {
        if (start >= end || end > N) {
            return;
        }
        const size_t first_word = start / BITS_PER_WORD;
        const size_t last_word = (end - 1) / BITS_PER_WORD;
        const size_t start_bit = start % BITS_PER_WORD;
        const size_t end_bit = (end - 1) % BITS_PER_WORD;
        const u64 start_mask = ~((1ULL << start_bit) - 1);
        const u64 end_mask = end_bit == BITS_PER_WORD - 1 ? ~0ULL : (1ULL << (end_bit + 1)) - 1;
        if (first_word == last_word) {
            data[first_word] |= start_mask & end_mask;
        } else {
            data[first_word] |= start_mask;
            size_t i = first_word + 1;
#ifdef BIT_ARRAY_USE_AVX
            const __m256i value = _mm256_set1_epi64x(-1);
            for (; i + WORDS_PER_AVX <= last_word; i += WORDS_PER_AVX) {
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(&data[i]), value);
            }
#endif
            for (; i < last_word; ++i) {
                data[i] = ~0ULL;
            }
            data[last_word] |= end_mask;
        }
    }

    inline void UnsetRange(size_t start, size_t end) {
        if (start >= end || end > N) {
            return;
        }
        size_t first_word = start / BITS_PER_WORD;
        const size_t last_word = (end - 1) / BITS_PER_WORD;
        const size_t start_bit = start % BITS_PER_WORD;
        const size_t end_bit = (end - 1) % BITS_PER_WORD;
        const u64 start_mask = (1ULL << start_bit) - 1;
        const u64 end_mask = end_bit == BITS_PER_WORD - 1 ? 0ULL : ~((1ULL << (end_bit + 1)) - 1);
        if (first_word == last_word) {
            data[first_word] &= start_mask | end_mask;
        } else {
            data[first_word] &= start_mask;
            size_t i = first_word + 1;
#ifdef BIT_ARRAY_USE_AVX
            const __m256i value = _mm256_setzero_si256();
            for (; i + WORDS_PER_AVX <= last_word; i += WORDS_PER_AVX) {
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(&data[i]), value);
            }
#endif
            for (; i < last_word; ++i) {
                data[i] = 0ULL;
            }
            data[last_word] &= end_mask;
        }
    }

    inline constexpr void SetRange(const Range& range) {
        SetRange(range.first, range.second);
    }

    inline constexpr void UnsetRange(const Range& range) {
        UnsetRange(range.first, range.second);
    }

    inline constexpr void Clear() {
        data.fill(0);
    }

    inline constexpr void Fill() {
        data.fill(~0ULL);
    }

    inline constexpr bool None() const {
        u64 result = 0;
        for (const auto& word : data) {
            result |= word;
        }
        return result == 0;
    }

    inline constexpr bool Any() const {
        return !None();
    }

    Range FirstRangeFrom(size_t start) const {
        if (start >= N) {
            return {N, N};
        }
        const auto find_end_bit = [&](size_t word) {
#ifdef BIT_ARRAY_USE_AVX
            const __m256i all_one = _mm256_set1_epi64x(-1);
            for (; word + WORDS_PER_AVX <= WORD_COUNT; word += WORDS_PER_AVX) {
                const __m256i current =
                    _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[word]));
                const __m256i cmp = _mm256_cmpeq_epi64(current, all_one);
                if (_mm256_movemask_epi8(cmp) != 0xFFFFFFFF) {
                    break;
                }
            }
#endif
            for (; word < WORD_COUNT; ++word) {
                if (data[word] != ~0ULL) {
                    return (word * BITS_PER_WORD) + std::countr_one(data[word]);
                }
            }
            return N;
        };

        const auto word_bits = [&](size_t index, u64 word) {
            const int empty_bits = std::countr_zero(word);
            const int ones_count = std::countr_one(word >> empty_bits);
            const size_t start_bit = index * BITS_PER_WORD + empty_bits;
            if (ones_count + empty_bits < BITS_PER_WORD) {
                return Range{start_bit, start_bit + ones_count};
            }
            return Range{start_bit, find_end_bit(index + 1)};
        };

        const size_t start_word = start / BITS_PER_WORD;
        const size_t start_bit = start % BITS_PER_WORD;
        const u64 masked_first = data[start_word] & (~((1ULL << start_bit) - 1));
        if (masked_first) {
            return word_bits(start_word, masked_first);
        }

        size_t word = start_word + 1;
#ifdef BIT_ARRAY_USE_AVX
        for (; word + WORDS_PER_AVX <= WORD_COUNT; word += WORDS_PER_AVX) {
            const __m256i current =
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[word]));
            if (!_mm256_testz_si256(current, current)) {
                break;
            }
        }
#endif
        for (; word < WORD_COUNT; ++word) {
            if (data[word] != 0) {
                return word_bits(word, data[word]);
            }
        }
        return {N, N};
    }

    inline constexpr Range FirstRange() const {
        return FirstRangeFrom(0);
    }

    Range LastRangeFrom(size_t end) const {
        if (end == 0) {
            return {0, 0};
        }
        if (end > N) {
            end = N;
        }
        const auto find_start_bit = [&](size_t word) {
#ifdef BIT_ARRAY_USE_AVX
            const __m256i all_zero = _mm256_setzero_si256();
            for (; word >= WORDS_PER_AVX; word -= WORDS_PER_AVX) {
                const __m256i current = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(&data[word - WORDS_PER_AVX]));
                const __m256i cmp = _mm256_cmpeq_epi64(current, all_zero);
                if (_mm256_movemask_epi8(cmp) != 0xFFFFFFFF) {
                    break;
                }
            }
#endif
            for (; word > 0; --word) {
                if (data[word - 1] != ~0ULL) {
                    return word * BITS_PER_WORD - std::countl_one(data[word - 1]);
                }
            }
            return size_t(0);
        };
        const auto word_bits = [&](size_t index, u64 word) {
            const int empty_bits = std::countl_zero(word);
            const int ones_count = std::countl_one(word << empty_bits);
            const size_t end_bit = index * BITS_PER_WORD - empty_bits;
            if (empty_bits + ones_count < BITS_PER_WORD) {
                return Range{end_bit - ones_count, end_bit};
            }
            return Range{find_start_bit(index - 1), end_bit};
        };
        const size_t end_word = ((end - 1) / BITS_PER_WORD) + 1;
        const size_t end_bit = (end - 1) % BITS_PER_WORD;
        u64 masked_last = data[end_word - 1];
        if (end_bit < BITS_PER_WORD - 1) {
            masked_last &= (1ULL << (end_bit + 1)) - 1;
        }
        if (masked_last) {
            return word_bits(end_word, masked_last);
        }
        size_t word = end_word - 1;
#ifdef BIT_ARRAY_USE_AVX
        for (; word >= WORDS_PER_AVX; word -= WORDS_PER_AVX) {
            const __m256i current =
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[word - WORDS_PER_AVX]));
            if (!_mm256_testz_si256(current, current)) {
                break;
            }
        }
#endif
        for (; word > 0; --word) {
            if (data[word - 1] != 0) {
                return word_bits(word, data[word - 1]);
            }
        }
        return {0, 0};
    }

    inline constexpr Range LastRange() const {
        return LastRangeFrom(N);
    }

    inline constexpr size_t Size() const {
        return N;
    }

    inline constexpr BitArray& operator|=(const BitArray& other) {
        for (size_t i = 0; i < WORD_COUNT; ++i) {
            data[i] |= other.data[i];
        }
        return *this;
    }

    inline constexpr BitArray& operator&=(const BitArray& other) {
        for (size_t i = 0; i < WORD_COUNT; ++i) {
            data[i] &= other.data[i];
        }
        return *this;
    }

    inline constexpr BitArray& operator^=(const BitArray& other) {
        for (size_t i = 0; i < WORD_COUNT; ++i) {
            data[i] ^= other.data[i];
        }
        return *this;
    }

    inline constexpr BitArray& operator~() {
        for (size_t i = 0; i < WORD_COUNT; ++i) {
            data[i] = ~data[i];
        }
        return *this;
    }

    inline constexpr BitArray operator|(const BitArray& other) const {
        BitArray result = *this;
        result |= other;
        return result;
    }

    inline constexpr BitArray operator&(const BitArray& other) const {
        BitArray result = *this;
        result &= other;
        return result;
    }

    inline constexpr BitArray operator^(const BitArray& other) const {
        BitArray result = *this;
        result ^= other;
        return result;
    }

    inline constexpr BitArray operator~() const {
        BitArray result = *this;
        result = ~result;
        return result;
    }

    inline constexpr bool operator==(const BitArray& other) const {
        u64 result = 0;
        for (size_t i = 0; i < WORD_COUNT; ++i) {
            result |= data[i] ^ other.data[i];
        }
        return result == 0;
    }

    inline constexpr bool operator!=(const BitArray& other) const {
        return !(*this == other);
    }

private:
    std::array<u64, WORD_COUNT> data{};
};

} // namespace Common