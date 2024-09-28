// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>

#include "common/assert.h"
#include "common/io_file.h"
#include "splash.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "externals/stb_image.h"

bool Splash::Open(const std::filesystem::path& filepath) {
    ASSERT_MSG(filepath.stem().string() != "png", "Unexpected file format passed");

    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }

    std::vector<u8> png_file{};
    const auto png_size = file.GetSize();
    png_file.resize(png_size);
    file.Seek(0);
    file.Read(png_file);

    auto* img_mem = stbi_load_from_memory(png_file.data(), png_file.size(),
                                          reinterpret_cast<int*>(&img_info.width),
                                          reinterpret_cast<int*>(&img_info.height),
                                          reinterpret_cast<int*>(&img_info.num_channels), 4);
    if (!img_mem) {
        return false;
    }

    const auto img_size = img_info.GetSizeBytes();
    img_data.resize(img_size);
    std::memcpy(img_data.data(), img_mem, img_size);
    stbi_image_free(img_mem);
    return true;
}
