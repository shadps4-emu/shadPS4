// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xxhash.h>
#include "common/assert.h"
#include "common/config.h"
#include "core/virtual_memory.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"
#include "video_core/texture_cache/tile_manager.h"

#ifndef _WIN64
#include <signal.h>
#include <sys/mman.h>

#define PAGE_NOACCESS PROT_NONE
#define PAGE_READWRITE (PROT_READ | PROT_WRITE)
#define PAGE_READONLY PROT_READ
#else
#include <windows.h>

void mprotect(void* addr, size_t len, int prot) {
    DWORD old_prot{};
    BOOL result = VirtualProtect(addr, len, prot, &old_prot);
    ASSERT_MSG(result != 0, "Region protection failed");
}

#endif

namespace VideoCore {

static TextureCache* g_texture_cache = nullptr;

#ifndef _WIN64
void GuestFaultSignalHandler(int sig, siginfo_t* info, void* raw_context) {
    ucontext_t* ctx = reinterpret_cast<ucontext_t*>(raw_context);
    const VAddr address = reinterpret_cast<VAddr>(info->si_addr);

#ifdef __APPLE__
    const u32 err = ctx->uc_mcontext->__es.__err;
#else
    const greg_t err = ctx->uc_mcontext.gregs[REG_ERR];
#endif

    if (err & 0x2) {
        g_texture_cache->OnCpuWrite(address);
    } else {
        // Read not supported!
        UNREACHABLE();
    }
}
#else
LONG WINAPI GuestFaultSignalHandler(EXCEPTION_POINTERS* pExp) noexcept {
    const u32 ec = pExp->ExceptionRecord->ExceptionCode;
    if (ec == EXCEPTION_ACCESS_VIOLATION) {
        const auto info = pExp->ExceptionRecord->ExceptionInformation;
        if (info[0] == 1) { // Write violation
            g_texture_cache->OnCpuWrite(info[1]);
            return EXCEPTION_CONTINUE_EXECUTION;
        } /* else {
            UNREACHABLE();
        }*/
    }
    return EXCEPTION_CONTINUE_SEARCH; // pass further
}
#endif

static constexpr u64 StreamBufferSize = 512_MB;
static constexpr u64 PageShift = 12;

TextureCache::TextureCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_)
    : instance{instance_}, scheduler{scheduler_},
      staging{instance, scheduler, vk::BufferUsageFlagBits::eTransferSrc, StreamBufferSize,
              Vulkan::BufferType::Upload},
      tile_manager{instance, scheduler} {

#ifndef _WIN64
#ifdef __APPLE__
    // Read-only memory write results in SIGBUS on Apple.
    static constexpr int SignalType = SIGBUS;
#else
    static constexpr int SignalType = SIGSEGV;
#endif

    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SignalType);

    using HandlerType = decltype(sigaction::sa_sigaction);

    struct sigaction guest_access_fault {};
    guest_access_fault.sa_flags = SA_SIGINFO | SA_ONSTACK;
    guest_access_fault.sa_sigaction = &GuestFaultSignalHandler;
    guest_access_fault.sa_mask = signal_mask;
    sigaction(SignalType, &guest_access_fault, nullptr);
#else
    veh_handle = AddVectoredExceptionHandler(0, GuestFaultSignalHandler);
    ASSERT_MSG(veh_handle, "Failed to register an exception handler");
#endif
    g_texture_cache = this;

    ImageInfo info;
    info.pixel_format = vk::Format::eR8G8B8A8Unorm;
    info.type = vk::ImageType::e2D;
    const ImageId null_id = slot_images.insert(instance, scheduler, info);
    ASSERT(null_id.index == 0);

    ImageViewInfo view_info;
    void(slot_image_views.insert(instance, view_info, slot_images[null_id], null_id));
}

TextureCache::~TextureCache() {
#if _WIN64
    RemoveVectoredExceptionHandler(veh_handle);
#endif
}

void TextureCache::OnCpuWrite(VAddr address) {
    std::unique_lock lock{m_page_table};
    ForEachImageInRegion(address, 1 << PageShift, [&](ImageId image_id, Image& image) {
        // Ensure image is reuploaded when accessed again.
        image.flags |= ImageFlagBits::CpuModified;
        // Untrack image, so the range is unprotected and the guest can write freely.
        UntrackImage(image, image_id);
    });
}

ImageId TextureCache::FindImage(const ImageInfo& info, bool refresh_on_create) {
    std::unique_lock lock{m_page_table};
    boost::container::small_vector<ImageId, 2> image_ids;
    ForEachImageInRegion(
        info.guest_address, info.guest_size_bytes, [&](ImageId image_id, Image& image) {
            // Address and width must match.
            if (image.cpu_addr != info.guest_address || image.info.size.width != info.size.width) {
                return;
            }
            if (info.IsDepthStencil() != image.info.IsDepthStencil() &&
                info.pixel_format != vk::Format::eR32Sfloat) {
                return;
            }
            image_ids.push_back(image_id);
        });

    ASSERT_MSG(image_ids.size() <= 1, "Overlapping images not allowed!");

    ImageId image_id{};
    if (image_ids.empty()) {
        image_id = slot_images.insert(instance, scheduler, info);
        RegisterImage(image_id);
    } else {
        image_id = image_ids[0];
    }

    RegisterMeta(info, image_id);

    Image& image = slot_images[image_id];
    if (True(image.flags & ImageFlagBits::CpuModified) && refresh_on_create) {
        RefreshImage(image);
        TrackImage(image, image_id);
    }

    return image_id;
}

ImageView& TextureCache::RegisterImageView(ImageId image_id, const ImageViewInfo& view_info) {
    Image& image = slot_images[image_id];
    if (const ImageViewId view_id = image.FindView(view_info); view_id) {
        return slot_image_views[view_id];
    }

    // All tiled images are created with storage usage flag. This makes set of formats (e.g. sRGB)
    // impossible to use. However, during view creation, if an image isn't used as storage we can
    // temporary remove its storage bit.
    std::optional<vk::ImageUsageFlags> usage_override;
    if (!image.info.usage.storage) {
        usage_override = image.usage & ~vk::ImageUsageFlagBits::eStorage;
    }

    const ImageViewId view_id =
        slot_image_views.insert(instance, view_info, image, image_id, usage_override);
    image.image_view_infos.emplace_back(view_info);
    image.image_view_ids.emplace_back(view_id);
    return slot_image_views[view_id];
}

ImageView& TextureCache::FindTexture(const AmdGpu::Image& desc, bool is_storage) {
    const ImageInfo info{desc};
    const ImageId image_id = FindImage(info);
    Image& image = slot_images[image_id];
    auto& usage = image.info.usage;

    if (is_storage) {
        image.Transit(vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderWrite);
        usage.storage = true;
    } else {
        const auto new_layout = image.info.IsDepthStencil()
                                    ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                    : vk::ImageLayout::eShaderReadOnlyOptimal;
        image.Transit(new_layout, vk::AccessFlagBits::eShaderRead);
        usage.texture = true;
    }

    const ImageViewInfo view_info{desc, is_storage};
    return RegisterImageView(image_id, view_info);
}

ImageView& TextureCache::FindRenderTarget(const AmdGpu::Liverpool::ColorBuffer& buffer,
                                          const AmdGpu::Liverpool::CbDbExtent& hint) {
    const ImageInfo info{buffer, hint};
    const ImageId image_id = FindImage(info);
    Image& image = slot_images[image_id];
    image.flags &= ~ImageFlagBits::CpuModified;

    image.Transit(vk::ImageLayout::eColorAttachmentOptimal,
                  vk::AccessFlagBits::eColorAttachmentWrite |
                      vk::AccessFlagBits::eColorAttachmentRead);

    image.info.usage.render_target = true;

    ImageViewInfo view_info{buffer, !!image.info.usage.vo_buffer};
    return RegisterImageView(image_id, view_info);
}

ImageView& TextureCache::FindDepthTarget(const AmdGpu::Liverpool::DepthBuffer& buffer,
                                         u32 num_slices, VAddr htile_address,
                                         const AmdGpu::Liverpool::CbDbExtent& hint,
                                         bool write_enabled) {
    const ImageInfo info{buffer, num_slices, htile_address, hint};
    const ImageId image_id = FindImage(info, false);
    Image& image = slot_images[image_id];
    image.flags &= ~ImageFlagBits::CpuModified;

    const auto new_layout = write_enabled ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                          : vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    image.Transit(new_layout, vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                                  vk::AccessFlagBits::eDepthStencilAttachmentRead);

    image.info.usage.depth_target = true;

    ImageViewInfo view_info;
    view_info.format = info.pixel_format;
    return RegisterImageView(image_id, view_info);
}

void TextureCache::RefreshImage(Image& image) {
    // Mark image as validated.
    image.flags &= ~ImageFlagBits::CpuModified;

    {
        if (!tile_manager.TryDetile(image)) {
            // Upload data to the staging buffer.
            const auto offset = staging.Copy(image.cpu_addr, image.info.guest_size_bytes, 4);
            // Copy to the image.
            image.Upload(staging.Handle(), offset);
        }

        image.Transit(vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead);
        return;
    }

    ASSERT(image.info.resources.levels == image.info.mips_layout.size());
    const u8* image_data = reinterpret_cast<const u8*>(image.cpu_addr);
    for (u32 m = 0; m < image.info.resources.levels; m++) {
        const u32 width = std::max(image.info.size.width >> m, 1u);
        const u32 height = std::max(image.info.size.height >> m, 1u);
        const u32 depth = image.info.is_volume ? std::max(image.info.size.depth >> m, 1u) : 1u;
        const u32 map_size = image.info.mips_layout[m].second * image.info.resources.layers;

        // Upload data to the staging buffer.
        const auto [data, offset, _] = staging.Map(map_size, 16);
        if (image.info.is_tiled) {
            ConvertTileToLinear(data, image_data, width, height, Config::isNeoMode());
        } else {
            std::memcpy(data,
                        image_data + image.info.mips_layout[m].first * image.info.resources.layers,
                        map_size);
        }
        staging.Commit(map_size);

        // Copy to the image.
        const vk::BufferImageCopy image_copy = {
            .bufferOffset = offset,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = m,
                .baseArrayLayer = 0,
                .layerCount = u32(image.info.resources.layers),
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, depth},
        };

        scheduler.EndRendering();

        const auto cmdbuf = scheduler.CommandBuffer();
        image.Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);

        cmdbuf.copyBufferToImage(staging.Handle(), image.image,
                                 vk::ImageLayout::eTransferDstOptimal, image_copy);

        image.Transit(vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead);
    }
}

vk::Sampler TextureCache::GetSampler(const AmdGpu::Sampler& sampler) {
    const u64 hash = XXH3_64bits(&sampler, sizeof(sampler));
    const auto [it, new_sampler] = samplers.try_emplace(hash, instance, sampler);
    return it->second.Handle();
}

void TextureCache::RegisterImage(ImageId image_id) {
    Image& image = slot_images[image_id];
    ASSERT_MSG(False(image.flags & ImageFlagBits::Registered),
               "Trying to register an already registered image");
    image.flags |= ImageFlagBits::Registered;
    ForEachPage(image.cpu_addr, image.info.guest_size_bytes,
                [this, image_id](u64 page) { page_table[page].push_back(image_id); });
}

void TextureCache::RegisterMeta(const ImageInfo& info, ImageId image_id) {
    Image& image = slot_images[image_id];

    if (image.flags & ImageFlagBits::MetaRegistered) {
        return;
    }

    bool registered = true;
    // Current resource tracking implementation allows us to detect usage of meta only in the last
    // moment, so we likely will miss its first clear. To avoid this and make first frame, where
    // the meta is encountered, looks correct we set its state to "cleared" at registrations time.
    if (info.usage.render_target) {
        if (info.meta_info.cmask_addr) {
            surface_metas.emplace(
                info.meta_info.cmask_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::CMask, .is_cleared = true});
            image.info.meta_info.cmask_addr = info.meta_info.cmask_addr;
        }

        if (info.meta_info.fmask_addr) {
            surface_metas.emplace(
                info.meta_info.fmask_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::FMask, .is_cleared = true});
            image.info.meta_info.fmask_addr = info.meta_info.fmask_addr;
        }
    } else if (info.usage.depth_target) {
        if (info.meta_info.htile_addr) {
            surface_metas.emplace(
                info.meta_info.htile_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::HTile, .is_cleared = true});
            image.info.meta_info.htile_addr = info.meta_info.htile_addr;
        }
    } else {
        registered = false;
    }

    if (registered) {
        image.flags |= ImageFlagBits::MetaRegistered;
    }
}

void TextureCache::UnregisterImage(ImageId image_id) {
    Image& image = slot_images[image_id];
    ASSERT_MSG(True(image.flags & ImageFlagBits::Registered),
               "Trying to unregister an already registered image");
    image.flags &= ~ImageFlagBits::Registered;
    ForEachPage(image.cpu_addr, image.info.guest_size_bytes, [this, image_id](u64 page) {
        const auto page_it = page_table.find(page);
        if (page_it == page_table.end()) {
            ASSERT_MSG(false, "Unregistering unregistered page=0x{:x}", page << PageShift);
            return;
        }
        auto& image_ids = page_it.value();
        const auto vector_it = std::ranges::find(image_ids, image_id);
        if (vector_it == image_ids.end()) {
            ASSERT_MSG(false, "Unregistering unregistered image in page=0x{:x}", page << PageShift);
            return;
        }
        image_ids.erase(vector_it);
    });
    slot_images.erase(image_id);
}

void TextureCache::TrackImage(Image& image, ImageId image_id) {
    if (True(image.flags & ImageFlagBits::Tracked)) {
        return;
    }
    image.flags |= ImageFlagBits::Tracked;
    UpdatePagesCachedCount(image.cpu_addr, image.info.guest_size_bytes, 1);
}

void TextureCache::UntrackImage(Image& image, ImageId image_id) {
    if (False(image.flags & ImageFlagBits::Tracked)) {
        return;
    }
    image.flags &= ~ImageFlagBits::Tracked;
    UpdatePagesCachedCount(image.cpu_addr, image.info.guest_size_bytes, -1);
}

void TextureCache::UpdatePagesCachedCount(VAddr addr, u64 size, s32 delta) {
    std::scoped_lock lk{mutex};
    const u64 num_pages = ((addr + size - 1) >> PageShift) - (addr >> PageShift) + 1;
    const u64 page_start = addr >> PageShift;
    const u64 page_end = page_start + num_pages;

    const auto pages_interval =
        decltype(cached_pages)::interval_type::right_open(page_start, page_end);
    if (delta > 0) {
        cached_pages.add({pages_interval, delta});
    }

    const auto& range = cached_pages.equal_range(pages_interval);
    for (const auto& [range, count] : boost::make_iterator_range(range)) {
        const auto interval = range & pages_interval;
        const VAddr interval_start_addr = boost::icl::first(interval) << PageShift;
        const VAddr interval_end_addr = boost::icl::last_next(interval) << PageShift;
        const u32 interval_size = interval_end_addr - interval_start_addr;
        void* addr = reinterpret_cast<void*>(interval_start_addr);
        if (delta > 0 && count == delta) {
            mprotect(addr, interval_size, PAGE_READONLY);
        } else if (delta < 0 && count == -delta) {
            mprotect(addr, interval_size, PAGE_READWRITE);
        } else {
            ASSERT(count >= 0);
        }
    }

    if (delta < 0) {
        cached_pages.add({pages_interval, delta});
    }
}

} // namespace VideoCore
