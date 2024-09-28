// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "common/types.h"

class Splash {
public:
    struct ImageInfo {
        u32 width;
        u32 height;
        u32 num_channels;

        u32 GetSizeBytes() const {
            return width * height * 4; // we always forcing rgba8 for simplicity
        }
    };

    Splash() = default;
    ~Splash() = default;

    bool Open(const std::filesystem::path& filepath);
    [[nodiscard]] bool IsLoaded() const {
        return img_data.size();
    }

    const auto& GetImageData() const {
        return img_data;
    }

    ImageInfo GetImageInfo() const {
        return img_info;
    }

private:
    ImageInfo img_info{};
    std::vector<u8> img_data{};
};
