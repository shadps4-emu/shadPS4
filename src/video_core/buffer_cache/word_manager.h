// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include "common/div_ceil.h"
#include "common/types.h"
#include "video_core/page_manager.h"

namespace VideoCore {

constexpr u64 PAGES_PER_WORD = 64;
constexpr u64 BYTES_PER_PAGE = 4_KB;
constexpr u64 BYTES_PER_WORD = PAGES_PER_WORD * BYTES_PER_PAGE;

enum class Type {
    CPU,
    GPU,
    Untracked,
};

/// Vector tracking modified pages tightly packed with small vector optimization
template <size_t stack_words = 1>
struct WordsArray {
    /// Returns the pointer to the words state
    [[nodiscard]] const u64* Pointer(bool is_short) const noexcept {
        return is_short ? stack.data() : heap;
    }

    /// Returns the pointer to the words state
    [[nodiscard]] u64* Pointer(bool is_short) noexcept {
        return is_short ? stack.data() : heap;
    }

    std::array<u64, stack_words> stack{}; ///< Small buffers storage
    u64* heap;                            ///< Not-small buffers pointer to the storage
};

template <size_t stack_words = 1>
struct Words {
    explicit Words() = default;
    explicit Words(u64 size_bytes_) : size_bytes{size_bytes_} {
        num_words = Common::DivCeil(size_bytes, BYTES_PER_WORD);
        if (IsShort()) {
            cpu.stack.fill(~u64{0});
            gpu.stack.fill(0);
            untracked.stack.fill(~u64{0});
        } else {
            // Share allocation between CPU and GPU pages and set their default values
            u64* const alloc = new u64[num_words * 3];
            cpu.heap = alloc;
            gpu.heap = alloc + num_words;
            untracked.heap = alloc + num_words * 2;
            std::fill_n(cpu.heap, num_words, ~u64{0});
            std::fill_n(gpu.heap, num_words, 0);
            std::fill_n(untracked.heap, num_words, ~u64{0});
        }
        // Clean up tailing bits
        const u64 last_word_size = size_bytes % BYTES_PER_WORD;
        const u64 last_local_page = Common::DivCeil(last_word_size, BYTES_PER_PAGE);
        const u64 shift = (PAGES_PER_WORD - last_local_page) % PAGES_PER_WORD;
        const u64 last_word = (~u64{0} << shift) >> shift;
        cpu.Pointer(IsShort())[NumWords() - 1] = last_word;
        untracked.Pointer(IsShort())[NumWords() - 1] = last_word;
    }

    ~Words() {
        Release();
    }

    Words& operator=(Words&& rhs) noexcept {
        Release();
        size_bytes = rhs.size_bytes;
        num_words = rhs.num_words;
        cpu = rhs.cpu;
        gpu = rhs.gpu;
        untracked = rhs.untracked;
        rhs.cpu.heap = nullptr;
        return *this;
    }

    Words(Words&& rhs) noexcept
        : size_bytes{rhs.size_bytes}, num_words{rhs.num_words}, cpu{rhs.cpu}, gpu{rhs.gpu},
          untracked{rhs.untracked} {
        rhs.cpu.heap = nullptr;
    }

    Words& operator=(const Words&) = delete;
    Words(const Words&) = delete;

    /// Returns true when the buffer fits in the small vector optimization
    [[nodiscard]] bool IsShort() const noexcept {
        return num_words <= stack_words;
    }

    /// Returns the number of words of the buffer
    [[nodiscard]] size_t NumWords() const noexcept {
        return num_words;
    }

    /// Release buffer resources
    void Release() {
        if (!IsShort()) {
            // CPU written words is the base for the heap allocation
            delete[] cpu.heap;
        }
    }

    template <Type type>
    std::span<u64> Span() noexcept {
        if constexpr (type == Type::CPU) {
            return std::span<u64>(cpu.Pointer(IsShort()), num_words);
        } else if constexpr (type == Type::GPU) {
            return std::span<u64>(gpu.Pointer(IsShort()), num_words);
        } else if constexpr (type == Type::Untracked) {
            return std::span<u64>(untracked.Pointer(IsShort()), num_words);
        }
    }

    template <Type type>
    std::span<const u64> Span() const noexcept {
        if constexpr (type == Type::CPU) {
            return std::span<const u64>(cpu.Pointer(IsShort()), num_words);
        } else if constexpr (type == Type::GPU) {
            return std::span<const u64>(gpu.Pointer(IsShort()), num_words);
        } else if constexpr (type == Type::Untracked) {
            return std::span<const u64>(untracked.Pointer(IsShort()), num_words);
        }
    }

    u64 size_bytes = 0;
    size_t num_words = 0;
    WordsArray<stack_words> cpu;
    WordsArray<stack_words> gpu;
    WordsArray<stack_words> untracked;
};

template <size_t stack_words = 1>
class WordManager {
public:
    explicit WordManager(PageManager* tracker_, VAddr cpu_addr_, u64 size_bytes)
        : tracker{tracker_}, cpu_addr{cpu_addr_}, words{size_bytes} {}

    explicit WordManager() = default;

    void SetCpuAddress(VAddr new_cpu_addr) {
        cpu_addr = new_cpu_addr;
    }

    VAddr GetCpuAddr() const {
        return cpu_addr;
    }

    static u64 ExtractBits(u64 word, size_t page_start, size_t page_end) {
        constexpr size_t number_bits = sizeof(u64) * 8;
        const size_t limit_page_end = number_bits - std::min(page_end, number_bits);
        u64 bits = (word >> page_start) << page_start;
        bits = (bits << limit_page_end) >> limit_page_end;
        return bits;
    }

    static std::pair<size_t, size_t> GetWordPage(VAddr address) {
        const size_t converted_address = static_cast<size_t>(address);
        const size_t word_number = converted_address / BYTES_PER_WORD;
        const size_t amount_pages = converted_address % BYTES_PER_WORD;
        return std::make_pair(word_number, amount_pages / BYTES_PER_PAGE);
    }

    template <typename Func>
    void IterateWords(size_t offset, size_t size, Func&& func) const {
        using FuncReturn = std::invoke_result_t<Func, std::size_t, u64>;
        static constexpr bool BOOL_BREAK = std::is_same_v<FuncReturn, bool>;
        const size_t start = static_cast<size_t>(std::max<s64>(static_cast<s64>(offset), 0LL));
        const size_t end = static_cast<size_t>(std::max<s64>(static_cast<s64>(offset + size), 0LL));
        if (start >= SizeBytes() || end <= start) {
            return;
        }
        auto [start_word, start_page] = GetWordPage(start);
        auto [end_word, end_page] = GetWordPage(end + BYTES_PER_PAGE - 1ULL);
        const size_t num_words = NumWords();
        start_word = std::min(start_word, num_words);
        end_word = std::min(end_word, num_words);
        const size_t diff = end_word - start_word;
        end_word += (end_page + PAGES_PER_WORD - 1ULL) / PAGES_PER_WORD;
        end_word = std::min(end_word, num_words);
        end_page += diff * PAGES_PER_WORD;
        constexpr u64 base_mask{~0ULL};
        for (size_t word_index = start_word; word_index < end_word; word_index++) {
            const u64 mask = ExtractBits(base_mask, start_page, end_page);
            start_page = 0;
            end_page -= PAGES_PER_WORD;
            if constexpr (BOOL_BREAK) {
                if (func(word_index, mask)) {
                    return;
                }
            } else {
                func(word_index, mask);
            }
        }
    }

    template <typename Func>
    void IteratePages(u64 mask, Func&& func) const {
        size_t offset = 0;
        while (mask != 0) {
            const size_t empty_bits = std::countr_zero(mask);
            offset += empty_bits;
            mask = mask >> empty_bits;

            const size_t continuous_bits = std::countr_one(mask);
            func(offset, continuous_bits);
            mask = continuous_bits < PAGES_PER_WORD ? (mask >> continuous_bits) : 0;
            offset += continuous_bits;
        }
    }

    /**
     * Change the state of a range of pages
     *
     * @param dirty_addr    Base address to mark or unmark as modified
     * @param size          Size in bytes to mark or unmark as modified
     */
    template <Type type, bool enable>
    void ChangeRegionState(u64 dirty_addr, u64 size) noexcept(type == Type::GPU) {
        std::span<u64> state_words = words.template Span<type>();
        [[maybe_unused]] std::span<u64> untracked_words = words.template Span<Type::Untracked>();
        IterateWords(dirty_addr - cpu_addr, size, [&](size_t index, u64 mask) {
            if constexpr (type == Type::CPU) {
                NotifyPageTracker<!enable>(index, untracked_words[index], mask);
            }
            if constexpr (enable) {
                state_words[index] |= mask;
                if constexpr (type == Type::CPU) {
                    untracked_words[index] |= mask;
                }
            } else {
                state_words[index] &= ~mask;
                if constexpr (type == Type::CPU) {
                    untracked_words[index] &= ~mask;
                }
            }
        });
    }

    /**
     * Loop over each page in the given range, turn off those bits and notify the tracker if
     * needed. Call the given function on each turned off range.
     *
     * @param query_cpu_range Base CPU address to loop over
     * @param size            Size in bytes of the CPU range to loop over
     * @param func            Function to call for each turned off region
     */
    template <Type type, bool clear, typename Func>
    void ForEachModifiedRange(VAddr query_cpu_range, s64 size, Func&& func) {
        static_assert(type != Type::Untracked);

        std::span<u64> state_words = words.template Span<type>();
        [[maybe_unused]] std::span<u64> untracked_words = words.template Span<Type::Untracked>();
        const size_t offset = query_cpu_range - cpu_addr;
        bool pending = false;
        size_t pending_offset{};
        size_t pending_pointer{};
        const auto release = [&]() {
            func(cpu_addr + pending_offset * BYTES_PER_PAGE,
                 (pending_pointer - pending_offset) * BYTES_PER_PAGE);
        };
        IterateWords(offset, size, [&](size_t index, u64 mask) {
            if constexpr (type == Type::GPU) {
                mask &= ~untracked_words[index];
            }
            const u64 word = state_words[index] & mask;
            if constexpr (clear) {
                if constexpr (type == Type::CPU) {
                    NotifyPageTracker<true>(index, untracked_words[index], mask);
                }
                state_words[index] &= ~mask;
                if constexpr (type == Type::CPU) {
                    untracked_words[index] &= ~mask;
                }
            }
            const size_t base_offset = index * PAGES_PER_WORD;
            IteratePages(word, [&](size_t pages_offset, size_t pages_size) {
                const auto reset = [&]() {
                    pending_offset = base_offset + pages_offset;
                    pending_pointer = base_offset + pages_offset + pages_size;
                };
                if (!pending) {
                    reset();
                    pending = true;
                    return;
                }
                if (pending_pointer == base_offset + pages_offset) {
                    pending_pointer += pages_size;
                    return;
                }
                release();
                reset();
            });
        });
        if (pending) {
            release();
        }
    }

    /**
     * Returns true when a region has been modified
     *
     * @param offset Offset in bytes from the start of the buffer
     * @param size   Size in bytes of the region to query for modifications
     */
    template <Type type>
    [[nodiscard]] bool IsRegionModified(u64 offset, u64 size) const noexcept {
        static_assert(type != Type::Untracked);

        const std::span<const u64> state_words = words.template Span<type>();
        [[maybe_unused]] const std::span<const u64> untracked_words =
            words.template Span<Type::Untracked>();
        bool result = false;
        IterateWords(offset, size, [&](size_t index, u64 mask) {
            if constexpr (type == Type::GPU) {
                mask &= ~untracked_words[index];
            }
            const u64 word = state_words[index] & mask;
            if (word != 0) {
                result = true;
                return true;
            }
            return false;
        });
        return result;
    }

    /// Returns the number of words of the manager
    [[nodiscard]] size_t NumWords() const noexcept {
        return words.NumWords();
    }

    /// Returns the size in bytes of the manager
    [[nodiscard]] u64 SizeBytes() const noexcept {
        return words.size_bytes;
    }

    /// Returns true when the buffer fits in the small vector optimization
    [[nodiscard]] bool IsShort() const noexcept {
        return words.IsShort();
    }

private:
    template <Type type>
    u64* Array() noexcept {
        if constexpr (type == Type::CPU) {
            return words.cpu.Pointer(IsShort());
        } else if constexpr (type == Type::GPU) {
            return words.gpu.Pointer(IsShort());
        } else if constexpr (type == Type::Untracked) {
            return words.untracked.Pointer(IsShort());
        }
    }

    template <Type type>
    const u64* Array() const noexcept {
        if constexpr (type == Type::CPU) {
            return words.cpu.Pointer(IsShort());
        } else if constexpr (type == Type::GPU) {
            return words.gpu.Pointer(IsShort());
        } else if constexpr (type == Type::Untracked) {
            return words.untracked.Pointer(IsShort());
        }
    }

    /**
     * Notify tracker about changes in the CPU tracking state of a word in the buffer
     *
     * @param word_index   Index to the word to notify to the tracker
     * @param current_bits Current state of the word
     * @param new_bits     New state of the word
     *
     * @tparam add_to_tracker True when the tracker should start tracking the new pages
     */
    template <bool add_to_tracker>
    void NotifyPageTracker(u64 word_index, u64 current_bits, u64 new_bits) const {
        u64 changed_bits = (add_to_tracker ? current_bits : ~current_bits) & new_bits;
        VAddr addr = cpu_addr + word_index * BYTES_PER_WORD;
        IteratePages(changed_bits, [&](size_t offset, size_t size) {
            tracker->UpdatePagesCachedCount(addr + offset * BYTES_PER_PAGE, size * BYTES_PER_PAGE,
                                            add_to_tracker ? 1 : -1);
        });
    }

    PageManager* tracker;
    VAddr cpu_addr = 0;
    Words<stack_words> words;
};

} // namespace VideoCore
