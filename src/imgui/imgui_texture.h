// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <imgui.h>

namespace ImGui {

namespace Core::TextureManager {
struct Inner;
} // namespace Core::TextureManager

class RefCountedTexture {
    Core::TextureManager::Inner* inner;

    explicit RefCountedTexture(Core::TextureManager::Inner* inner);

public:
    struct Image {
        ImTextureID im_id;
        u32 width;
        u32 height;
    };

    static RefCountedTexture DecodePngTexture(std::vector<u8> data);

    static RefCountedTexture DecodePngFile(std::filesystem::path path);

    RefCountedTexture();

    RefCountedTexture(const RefCountedTexture& other);
    RefCountedTexture(RefCountedTexture&& other) noexcept;
    RefCountedTexture& operator=(const RefCountedTexture& other);
    RefCountedTexture& operator=(RefCountedTexture&& other) noexcept;

    virtual ~RefCountedTexture();

    [[nodiscard]] Image GetTexture() const;

    explicit(false) operator bool() const;
};

}; // namespace ImGui