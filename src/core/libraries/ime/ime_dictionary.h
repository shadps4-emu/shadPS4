// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "core/libraries/ime/ime_kb_language.h"

namespace Libraries::Ime {

class ImeDictionary {
public:
    static std::shared_ptr<const ImeDictionary> Load(const std::filesystem::path& directory,
                                                     ImeKbVariant variant);
    static std::shared_ptr<const ImeDictionary> LoadApplication(const std::filesystem::path& path);
    std::vector<std::u16string> Find(std::u16string_view reading) const;
    bool Empty() const;

private:
    std::map<std::u16string, std::vector<std::u16string>, std::less<>> words;

    void Read(const std::filesystem::path& path);
    void Add(std::u16string reading, std::u16string text);
};

} // namespace Libraries::Ime
