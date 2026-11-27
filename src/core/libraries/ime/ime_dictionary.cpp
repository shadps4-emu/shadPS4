// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_dictionary.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <mutex>
#include <set>
#include <span>
#include <zlib.h>

namespace Libraries::Ime {
namespace {

class DictionaryData {
public:
    explicit DictionaryData(std::span<const u8> bytes) : bytes(bytes) {}

    bool Contains(std::size_t offset, std::size_t count) const {
        return offset <= bytes.size() && count <= bytes.size() - offset;
    }

    u8 Byte(std::size_t offset) const {
        return Contains(offset, 1) ? bytes[offset] : 0;
    }

    u32 Integer(std::size_t offset, std::size_t count = 4) const {
        if (!Contains(offset, count)) {
            return 0;
        }
        u32 value = 0;
        for (std::size_t i = 0; i < count; ++i) {
            value = (value << 8) | bytes[offset + i];
        }
        return value;
    }

    u32 Bits(std::size_t offset, u32 start, u32 count) const {
        if (count > 32 || !Contains(offset + start / 8, (start % 8 + count + 7) / 8)) {
            return 0;
        }
        u32 value = 0;
        for (u32 bit = start; bit < start + count; ++bit) {
            value = (value << 1) | ((bytes[offset + bit / 8] >> (7 - bit % 8)) & 1);
        }
        return value;
    }

    std::u16string Text(std::size_t offset, std::size_t units) const {
        if (units > 55 || !Contains(offset, units * 2)) {
            return {};
        }
        std::u16string text;
        text.reserve(units);
        for (std::size_t i = 0; i < units; ++i) {
            const auto character = static_cast<char16_t>(Integer(offset + i * 2, 2));
            if (character == 0) {
                return {};
            }
            text.push_back(character);
        }
        return text;
    }

private:
    std::span<const u8> bytes;
};

std::filesystem::path InputMethodDirectory(ImeKbVariant variant) {
    switch (variant) {
    case ImeKbVariant::Japanese:
    case ImeKbVariant::Japanese_Gojuon:
        return "JA/32";
    case ImeKbVariant::Chinese_CN:
        return "ZH/CN/pinyin/GB18030";
    case ImeKbVariant::Chinese_TW:
        return "ZH/TW/pinyin";
    case ImeKbVariant::Chinese_HK:
        return "ZH/HK/jyutping";
    case ImeKbVariant::Chinese_TW_Zhuyin:
        return "ZH/TW/bopomofo";
    case ImeKbVariant::Chinese_TW_Cangjie:
        return "ZH/TW/cangjie";
    default:
        return {};
    }
}

void Katakana(std::u16string& text) {
    for (auto& character : text) {
        if (character >= u'ぁ' && character <= u'ゖ') {
            character += 0x60;
        }
    }
}

} // namespace

std::shared_ptr<const ImeDictionary> ImeDictionary::Load(const std::filesystem::path& directory,
                                                         ImeKbVariant variant) {
    auto method = InputMethodDirectory(variant);
    if (method.empty()) {
        return {};
    }
    auto path = directory / method;
    std::error_code error;
    if (method == "JA/32" && !std::filesystem::is_directory(path, error)) {
        path = directory / "JA";
    }
    static std::mutex mutex;
    static std::map<std::filesystem::path, std::shared_ptr<const ImeDictionary>> dictionaries;
    const std::scoped_lock lock(mutex);
    if (const auto found = dictionaries.find(path); found != dictionaries.end()) {
        return found->second;
    }
    auto dictionary = std::make_shared<ImeDictionary>();
    for (const auto* name : {"njtan.a", "njubase1.a", "njubase2.a", "njubase3.a", "njcangjie.a"}) {
        dictionary->Read(path / name);
    }
    dictionaries.emplace(path, dictionary);
    return dictionary;
}

std::shared_ptr<const ImeDictionary> ImeDictionary::LoadApplication(
    const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }
    const auto size = file.tellg();
    if (size < 24 || size > 200 * 1024) {
        return {};
    }
    std::vector<u8> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        return {};
    }
    const DictionaryData data(bytes);
    const auto integer = [&](std::size_t offset) {
        return data.Byte(offset) | (u32{data.Byte(offset + 1)} << 8) |
               (u32{data.Byte(offset + 2)} << 16) | (u32{data.Byte(offset + 3)} << 24);
    };
    const auto count = integer(12);
    const auto records = integer(16);
    const auto strings = integer(20);
    if (integer(0) != 0x40454353 || integer(4) != bytes.size() - 24 || records < 24 ||
        count > (bytes.size() - 24) / 8 || strings != records + u64{count} * 8 ||
        !data.Contains(records, u64{count} * 8) || strings > bytes.size() ||
        ((bytes.size() - strings) & 1) != 0 ||
        integer(8) != crc32(0, bytes.data() + 12, static_cast<uInt>(bytes.size() - 12))) {
        return {};
    }
    const auto text = [&](u32 packed) {
        std::u16string value;
        const auto length = packed >> 24;
        const auto offset = packed & 0xffffff;
        if ((offset & 1) != 0 || !data.Contains(u64{strings} + offset, length * 2)) {
            return value;
        }
        for (u32 i = 0; i < length; ++i) {
            const auto position = strings + offset + i * 2;
            const auto character =
                static_cast<char16_t>(data.Byte(position) | (u16{data.Byte(position + 1)} << 8));
            if (character == 0) {
                return std::u16string{};
            }
            value.push_back(character);
        }
        return value;
    };
    auto dictionary = std::make_shared<ImeDictionary>();
    std::vector<bool> seen(count);
    for (std::size_t group = 24; group < records;) {
        if (!data.Contains(group, 4) || group + 4 > records) {
            return {};
        }
        const auto first = integer(group) & 0xffff;
        const auto last = integer(group) >> 16;
        if (last < first || u64{group} + 4 + (last - first + 1ULL) * 4 > records) {
            return {};
        }
        group += 4;
        for (u32 character = first; character <= last; ++character, group += 4) {
            const auto entry = integer(group);
            if (entry == 0xffffffff) {
                continue;
            }
            const auto entries = entry & 0xffff;
            const auto begin = entry >> 16;
            if (entries > count || begin > count - entries) {
                return {};
            }
            for (u32 i = begin; i < begin + entries; ++i) {
                if (seen[i]) {
                    return {};
                }
                seen[i] = true;
                const auto key = integer(records + i * 8ULL);
                const auto value = integer(records + i * 8ULL + 4);
                auto reading = text(key);
                if (reading.size() != (key >> 24) || character == 0) {
                    return {};
                }
                reading.insert(reading.begin(), static_cast<char16_t>(character));
                auto word = text(value);
                if (word.size() != (value >> 24)) {
                    return {};
                }
                if (word.empty()) {
                    word = reading;
                }
                const auto valid_utf16 = [](const std::u16string& text) {
                    for (std::size_t j = 0; j < text.size(); ++j) {
                        if (text[j] >= 0xd800 && text[j] <= 0xdbff) {
                            if (++j == text.size() || text[j] < 0xdc00 || text[j] > 0xdfff) {
                                return false;
                            }
                        } else if (text[j] >= 0xdc00 && text[j] <= 0xdfff) {
                            return false;
                        }
                    }
                    return true;
                };
                if (!valid_utf16(reading) || !valid_utf16(word)) {
                    return {};
                }
                dictionary->Add(std::move(reading), std::move(word));
            }
        }
    }
    return std::ranges::all_of(seen, [](bool value) { return value; }) ? dictionary : nullptr;
}

void ImeDictionary::Add(std::u16string reading, std::u16string text) {
    reading.erase(0, reading.find_first_not_of(u"\x01\x02\x03"));
    if (reading.empty() || text.empty()) {
        return;
    }
    auto& candidates = words[std::move(reading)];
    if (std::ranges::find(candidates, text) == candidates.end()) {
        candidates.push_back(std::move(text));
    }
}

void ImeDictionary::Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return;
    }
    const auto size = file.tellg();
    if (size < 0x54 || size > 32 * 1024 * 1024) {
        return;
    }
    std::vector<u8> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        return;
    }
    const DictionaryData data(bytes);
    if (data.Integer(0) != 0x4e4a4443 || data.Integer(4) != 0x20000 ||
        static_cast<u64>(data.Integer(12)) + data.Integer(16) + 28 != bytes.size() ||
        data.Integer(bytes.size() - 4) != 0x4e4a4443) {
        return;
    }
    const auto type = data.Integer(8);
    if (type == 0x50000) {
        const auto records = data.Integer(0x20);
        const auto strings = data.Integer(0x24);
        const auto count = data.Integer(0x50);
        if (count > bytes.size() / 10 || !data.Contains(records, count * 10ULL)) {
            return;
        }
        for (u32 i = 0; i < count; ++i) {
            const std::size_t record = records + i * 10ULL;
            const auto key = data.Integer(record);
            const auto value = data.Integer(record + 4);
            auto reading = data.Text(strings + (key & 0xffffffULL), (key >> 25) & 63);
            auto text = data.Text(strings + (value & 0xffffffULL), (value >> 25) & 63);
            if (((value >> 25) & 63) == 0) {
                text = reading;
                if (data.Byte(record + 4) & 0x80) {
                    Katakana(text);
                }
            }
            Add(std::move(reading), std::move(text));
        }
        return;
    }
    if (type != 2 && type != 4) {
        return;
    }
    const auto alphabet = data.Integer(0x42);
    const auto alphabet_count = data.Byte(0x46);
    const auto alphabet_size = data.Byte(0x47);
    const auto nodes = data.Integer(0x48);
    const auto stems = data.Integer(0x2b);
    const auto child_bits = data.Byte(0x4d);
    const auto stem_bits = data.Byte(0x4c);
    const auto flag_bits = data.Byte(0x33) + ((data.Byte(0x1c) & 3) != 0);
    const auto frequency_bits = data.Byte(0x32);
    const auto candidate_bits = data.Byte(0x2f);
    const auto reading_bits = data.Byte(0x35);
    const auto stem_header_bits =
        1 + flag_bits + frequency_bits + data.Byte(0x30) + data.Byte(0x31);
    if (nodes >= stems || stems >= bytes.size() || child_bits > 32 || stem_bits > 32 ||
        candidate_bits > 16 || reading_bits > 16 || stem_header_bits > 64 || alphabet_size != 2 ||
        !data.Contains(alphabet, alphabet_count * 2)) {
        return;
    }
    std::set<std::size_t> visited;
    std::function<void(std::size_t, const std::u16string&)> walk;
    walk = [&](std::size_t node, const std::u16string& prefix) {
        while (node < stems && prefix.size() < 55 && visited.insert(node).second) {
            const auto flags = data.Byte(node);
            const u32 count = (flags & 16) ? (flags & 15) + 2 : 1;
            u32 bit = (flags & 16) ? 8 : 4;
            const auto child = (flags & 64) ? data.Bits(node, bit, child_bits) : 0;
            bit += (flags & 64) ? child_bits : 0;
            const auto record = (flags & 32) ? data.Bits(node, bit, stem_bits) : 0;
            bit += (flags & 32) ? stem_bits : 0;
            auto reading = prefix;
            for (u32 i = 0; i < count; ++i) {
                const auto index = data.Bits(node, bit, 8);
                bit += 8;
                if (index == 0 || index > alphabet_count) {
                    return;
                }
                reading.push_back(
                    static_cast<char16_t>(data.Integer(alphabet + (index - 1) * 2, 2)));
            }
            if (flags & 32) {
                std::size_t position = stems + record;
                while (data.Contains(position, 2)) {
                    const bool last = (data.Byte(position) & 128) != 0;
                    const auto length = data.Bits(position, stem_header_bits, candidate_bits);
                    const auto extra =
                        last && (data.Byte(0x1c) & 128)
                            ? data.Bits(position, stem_header_bits + candidate_bits, reading_bits)
                            : 0;
                    const auto header = (stem_header_bits + candidate_bits +
                                         (last && (data.Byte(0x1c) & 128) ? reading_bits : 0) + 7) /
                                        8;
                    if (length % 2 != 0 || !data.Contains(position, header + length + extra)) {
                        break;
                    }
                    Add(reading, length ? data.Text(position + header, length / 2) : reading);
                    position += header + length + extra;
                    if (last || header + length + extra == 0) {
                        break;
                    }
                }
            }
            if (child && child < stems - node) {
                walk(node + child, reading);
            }
            if (flags & 128) {
                break;
            }
            node += (bit + 7) / 8;
        }
    };
    walk(nodes, {});
}

std::vector<std::u16string> ImeDictionary::Find(std::u16string_view reading) const {
    if (reading.empty()) {
        return {};
    }
    if (const auto found = words.find(reading); found != words.end()) {
        return found->second;
    }
    std::vector<std::u16string> result;
    for (auto it = words.lower_bound(reading);
         it != words.end() && it->first.starts_with(reading) && result.size() < 32; ++it) {
        for (const auto& word : it->second) {
            if (std::ranges::find(result, word) == result.end()) {
                result.push_back(word);
                if (result.size() == 32) {
                    break;
                }
            }
        }
    }
    return result;
}

bool ImeDictionary::Empty() const {
    return words.empty();
}

} // namespace Libraries::Ime
