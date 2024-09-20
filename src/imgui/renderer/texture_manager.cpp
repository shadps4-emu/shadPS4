// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <utility>

#include <externals/stb_image.h>

#include "common/assert.h"
#include "common/io_file.h"
#include "common/polyfill_thread.h"
#include "imgui_impl_vulkan.h"
#include "texture_manager.h"

namespace ImGui {

namespace Core::TextureManager {
struct Inner {
    std::atomic_int count = 0;
    ImTextureID texture_id = nullptr;
    u32 width = 0;
    u32 height = 0;

    Vulkan::UploadTextureData upload_data;

    ~Inner();
};
} // namespace Core::TextureManager

using namespace Core::TextureManager;

RefCountedTexture::RefCountedTexture(Inner* inner) : inner(inner) {
    ++inner->count;
}

RefCountedTexture RefCountedTexture::DecodePngTexture(std::vector<u8> data) {
    const auto core = new Inner;
    Core::TextureManager::DecodePngTexture(std::move(data), core);
    return RefCountedTexture(core);
}

RefCountedTexture RefCountedTexture::DecodePngFile(std::filesystem::path path) {
    const auto core = new Inner;
    Core::TextureManager::DecodePngFile(std::move(path), core);
    return RefCountedTexture(core);
}

RefCountedTexture::RefCountedTexture() : inner(nullptr) {}

RefCountedTexture::RefCountedTexture(const RefCountedTexture& other) : inner(other.inner) {
    if (inner != nullptr) {
        ++inner->count;
    }
}

RefCountedTexture::RefCountedTexture(RefCountedTexture&& other) noexcept : inner(other.inner) {
    other.inner = nullptr;
}

RefCountedTexture& RefCountedTexture::operator=(const RefCountedTexture& other) {
    if (this == &other)
        return *this;
    inner = other.inner;
    if (inner != nullptr) {
        ++inner->count;
    }
    return *this;
}

RefCountedTexture& RefCountedTexture::operator=(RefCountedTexture&& other) noexcept {
    if (this == &other)
        return *this;
    std::swap(inner, other.inner);
    return *this;
}

RefCountedTexture::~RefCountedTexture() {
    if (inner != nullptr) {
        if (inner->count.fetch_sub(1) == 1) {
            delete inner;
        }
    }
}
RefCountedTexture::Image RefCountedTexture::GetTexture() const {
    if (inner == nullptr) {
        return {};
    }
    return Image{
        .im_id = inner->texture_id,
        .width = inner->width,
        .height = inner->height,
    };
}
RefCountedTexture::operator bool() const {
    return inner != nullptr && inner->texture_id != nullptr;
}

struct Job {
    Inner* core;
    std::vector<u8> data;
    std::filesystem::path path;
};

struct UploadJob {
    Inner* core = nullptr;
    Vulkan::UploadTextureData data;
    int tick = 0; // Used to skip the first frame when destroying to await the current frame to draw
};

static bool g_is_worker_running = false;
static std::jthread g_worker_thread;
static std::condition_variable g_worker_cv;

static std::mutex g_job_list_mtx;
static std::deque<Job> g_job_list;

static std::mutex g_upload_mtx;
static std::deque<UploadJob> g_upload_list;

namespace Core::TextureManager {

Inner::~Inner() {
    if (upload_data.descriptor_set != nullptr) {
        std::unique_lock lk{g_upload_mtx};
        g_upload_list.emplace_back(UploadJob{
            .data = this->upload_data,
            .tick = 2,
        });
    }
}

void WorkerLoop() {
    std::mutex mtx;
    while (g_is_worker_running) {
        std::unique_lock lk{mtx};
        g_worker_cv.wait(lk);
        if (!g_is_worker_running) {
            break;
        }
        while (true) {
            g_job_list_mtx.lock();
            if (g_job_list.empty()) {
                g_job_list_mtx.unlock();
                break;
            }
            auto [core, png_raw, path] = std::move(g_job_list.front());
            g_job_list.pop_front();
            g_job_list_mtx.unlock();

            if (!path.empty()) { // Decode PNG from file
                Common::FS::IOFile file(path, Common::FS::FileAccessMode::Read);
                if (!file.IsOpen()) {
                    LOG_ERROR(ImGui, "Failed to open PNG file: {}", path.string());
                    continue;
                }
                png_raw.resize(file.GetSize());
                file.Seek(0);
                file.ReadRaw<u8>(png_raw.data(), png_raw.size());
                file.Close();
            }

            int width, height;
            const stbi_uc* pixels =
                stbi_load_from_memory(png_raw.data(), png_raw.size(), &width, &height, nullptr, 4);

            auto texture = Vulkan::UploadTexture(pixels, vk::Format::eR8G8B8A8Unorm, width, height,
                                                 width * height * 4 * sizeof(stbi_uc));

            core->upload_data = texture;
            core->width = width;
            core->height = height;

            std::unique_lock upload_lk{g_upload_mtx};
            g_upload_list.emplace_back(UploadJob{
                .core = core,
            });
        }
    }
}

void StartWorker() {
    ASSERT(!g_is_worker_running);
    g_worker_thread = std::jthread(WorkerLoop);
    g_is_worker_running = true;
}

void StopWorker() {
    ASSERT(g_is_worker_running);
    g_is_worker_running = false;
    g_worker_cv.notify_one();
}

void DecodePngTexture(std::vector<u8> data, Inner* core) {
    ++core->count;
    Job job{
        .core = core,
        .data = std::move(data),
    };
    std::unique_lock lk{g_job_list_mtx};
    g_job_list.push_back(std::move(job));
    g_worker_cv.notify_one();
}

void DecodePngFile(std::filesystem::path path, Inner* core) {
    ++core->count;
    Job job{
        .core = core,
        .path = std::move(path),
    };
    std::unique_lock lk{g_job_list_mtx};
    g_job_list.push_back(std::move(job));
    g_worker_cv.notify_one();
}

void Submit() {
    UploadJob upload;
    {
        std::unique_lock lk{g_upload_mtx};
        if (g_upload_list.empty()) {
            return;
        }
        // Upload one texture at a time to avoid slow down
        upload = g_upload_list.front();
        g_upload_list.pop_front();
        if (upload.tick > 0) {
            --upload.tick;
            g_upload_list.emplace_back(upload);
            return;
        }
    }
    if (upload.core != nullptr) {
        upload.core->upload_data.Upload();
        upload.core->texture_id = upload.core->upload_data.descriptor_set;
        if (upload.core->count.fetch_sub(1) == 1) {
            delete upload.core;
        }
    } else {
        upload.data.Destroy();
    }
}
} // namespace Core::TextureManager

} // namespace ImGui