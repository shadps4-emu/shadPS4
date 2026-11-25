// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>
#include <SDL3/SDL_init.h>
#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include <imgui/imgui_std.h>
#include <minimp3_ex.h>
#include <queue>

#include "common/logging/formatter.h"
#include "common/path_util.h"
#include "common/stb.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/trophy_ui.h"
#include "core/libraries/system/notification_ui.h"
#include "imgui/renderer/font_stack.h"

CMRC_DECLARE(res);
namespace fs = std::filesystem;
using namespace ImGui;

namespace Libraries::SystemNotification {

struct AvatarFetchState {
    std::mutex mutex;
    std::atomic<bool> done{false};
    std::vector<u8> avatar_png;
};

static std::mutex avatar_cache_mtx;
static std::unordered_map<std::string, std::vector<u8>> avatar_cache;

std::optional<SystemNotificationUI> current_notif_ui;
std::queue<NotificationInfo> notif_queue;
std::mutex queueMtx;
std::atomic<bool> notif_showing{false};

static void PopAndShowNextLocked();

static std::vector<u8> DecodeDdsToPng(const std::vector<u8>& data) {
    if (data.size() < 128 || data[0] != 'D' || data[1] != 'D' || data[2] != 'S' || data[3] != ' ') {
        return data;
    }
    u32 height = 0;
    u32 width = 0;
    u32 fourcc = 0;
    std::memcpy(&height, &data[12], sizeof(u32));
    std::memcpy(&width, &data[16], sizeof(u32));
    std::memcpy(&fourcc, &data[84], sizeof(u32));

    if (width == 0 || height == 0 || width > 4096 || height > 4096) {
        return data;
    }

    // FourCC 'DXT5'
    if (fourcc != 0x35545844) {
        return data;
    }

    const u32 bw = (width + 3) / 4;
    const u32 bh = (height + 3) / 4;
    if (data.size() < 128 + static_cast<size_t>(bw) * bh * 16) {
        return data;
    }

    const u8* src = data.data() + 128;
    std::vector<u8> rgba(width * height * 4);

    auto decode_bc3_block = [](const u8* block, u8* out_pixels, u32 stride, u32 bx, u32 by, u32 w,
                               u32 h) {
        const u8 a0 = block[0];
        const u8 a1 = block[1];
        u8 alphas[8]{};
        alphas[0] = a0;
        alphas[1] = a1;
        if (a0 > a1) {
            for (int i = 0; i < 6; ++i) {
                alphas[2 + i] = ((6 - i) * a0 + (1 + i) * a1) / 7;
            }
        } else {
            for (int i = 0; i < 4; ++i) {
                alphas[2 + i] = ((4 - i) * a0 + (1 + i) * a1) / 5;
            }
            alphas[6] = 0;
            alphas[7] = 255;
        }

        u64 a_bits = 0;
        for (int i = 0; i < 6; ++i) {
            a_bits |= static_cast<u64>(block[2 + i]) << (i * 8);
        }

        const u8* col_block = block + 8;
        const u16 c0_raw = col_block[0] | (col_block[1] << 8);
        const u16 c1_raw = col_block[2] | (col_block[3] << 8);

        auto rgb565_to_rgb888 = [](u16 c, u8& r, u8& g, u8& b) {
            r = ((c >> 11) & 0x1F) * 255 / 31;
            g = ((c >> 5) & 0x3F) * 255 / 63;
            b = (c & 0x1F) * 255 / 31;
        };

        u8 colors[4][3]{};
        rgb565_to_rgb888(c0_raw, colors[0][0], colors[0][1], colors[0][2]);
        rgb565_to_rgb888(c1_raw, colors[1][0], colors[1][1], colors[1][2]);

        for (int i = 0; i < 3; ++i) {
            colors[2][i] = (2 * colors[0][i] + colors[1][i]) / 3;
            colors[3][i] = (colors[0][i] + 2 * colors[1][i]) / 3;
        }

        const u32 c_bits =
            col_block[4] | (col_block[5] << 8) | (col_block[6] << 16) | (col_block[7] << 24);

        for (u32 y = 0; y < 4; ++y) {
            for (u32 x = 0; x < 4; ++x) {
                const u32 px = bx + x;
                const u32 py = by + y;
                if (px < w && py < h) {
                    const u32 idx = y * 4 + x;
                    const u32 a_idx = (a_bits >> (idx * 3)) & 0x7;
                    const u32 c_idx = (c_bits >> (idx * 2)) & 0x3;

                    u8* dst = out_pixels + (py * stride + px * 4);
                    dst[0] = colors[c_idx][0];
                    dst[1] = colors[c_idx][1];
                    dst[2] = colors[c_idx][2];
                    dst[3] = alphas[a_idx];
                }
            }
        }
    };

    u32 block_idx = 0;
    for (u32 by = 0; by < height; by += 4) {
        for (u32 bx = 0; bx < width; bx += 4) {
            decode_bc3_block(src + block_idx * 16, rgba.data(), width * 4, bx, by, width, height);
            block_idx++;
        }
    }

    std::vector<u8> png_bytes;
    auto write_cb = [](void* ctx, void* buf, int len) {
        auto* v = static_cast<std::vector<u8>*>(ctx);
        auto* p = static_cast<const u8*>(buf);
        v->insert(v->end(), p, p + len);
    };
    if (stbi_write_png_to_func(write_cb, &png_bytes, width, height, 4, rgba.data(), width * 4)) {
        return png_bytes;
    }
    return data;
}

static std::vector<u8> DownscalePngToMaxDim(std::vector<u8> data, int max_dim = 128) {
    data = DecodeDdsToPng(std::move(data));
    if (data.empty()) {
        return data;
    }

    int w = 0;
    int h = 0;
    stbi_uc* pixels =
        stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &w, &h, nullptr, 4);
    if (!pixels || w <= 0 || h <= 0) {
        if (pixels) {
            stbi_image_free(pixels);
        }
        return data;
    }

    if (w <= max_dim && h <= max_dim) {
        stbi_image_free(pixels);
        return data;
    }

    std::vector<u8> cur(pixels, pixels + static_cast<size_t>(w) * h * 4);
    stbi_image_free(pixels);

    while (w > max_dim || h > max_dim) {
        const int nw = std::max(1, w / 2);
        const int nh = std::max(1, h / 2);
        std::vector<u8> next(static_cast<size_t>(nw) * nh * 4);
        for (int y = 0; y < nh; ++y) {
            const int sy0 = y * 2;
            const int sy1 = std::min(sy0 + 1, h - 1);
            for (int x = 0; x < nw; ++x) {
                const int sx0 = x * 2;
                const int sx1 = std::min(sx0 + 1, w - 1);
                const int sx[4] = {sx0, sx1, sx0, sx1};
                const int sy[4] = {sy0, sy0, sy1, sy1};
                u32 sum_r = 0;
                u32 sum_g = 0;
                u32 sum_b = 0;
                u32 sum_a = 0;
                for (int k = 0; k < 4; ++k) {
                    const size_t idx = (static_cast<size_t>(sy[k]) * w + sx[k]) * 4;
                    const u32 a = cur[idx + 3];
                    sum_r += cur[idx + 0] * a;
                    sum_g += cur[idx + 1] * a;
                    sum_b += cur[idx + 2] * a;
                    sum_a += a;
                }
                const size_t dst_idx = (static_cast<size_t>(y) * nw + x) * 4;
                if (sum_a > 0) {
                    next[dst_idx + 0] = static_cast<u8>((sum_r + sum_a / 2) / sum_a);
                    next[dst_idx + 1] = static_cast<u8>((sum_g + sum_a / 2) / sum_a);
                    next[dst_idx + 2] = static_cast<u8>((sum_b + sum_a / 2) / sum_a);
                    next[dst_idx + 3] = static_cast<u8>((sum_a + 2) / 4);
                } else {
                    std::memset(&next[dst_idx], 0, 4);
                }
            }
        }
        cur = std::move(next);
        w = nw;
        h = nh;
    }

    std::vector<u8> png_bytes;
    auto write_cb = [](void* ctx, void* buf, int len) {
        auto* v = static_cast<std::vector<u8>*>(ctx);
        auto* p = static_cast<const u8*>(buf);
        v->insert(v->end(), p, p + len);
    };
    if (stbi_write_png_to_func(write_cb, &png_bytes, w, h, 4, cur.data(), w * 4)) {
        return png_bytes;
    }
    return data;
}

static std::vector<u8> LoadAssetData(const std::vector<fs::path>& candidates,
                                     const std::string& embeddedResPath) {
    for (const auto& path : candidates) {
        std::error_code ec;
        if (fs::exists(path, ec)) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file) {
                const auto size = file.tellg();
                if (size > 0) {
                    file.seekg(0, std::ios::beg);
                    std::vector<u8> data(static_cast<size_t>(size));
                    if (file.read(reinterpret_cast<char*>(data.data()), size)) {
                        return DecodeDdsToPng(data);
                    }
                }
            }
        }
    }
    if (!embeddedResPath.empty()) {
        try {
            auto fs_cmrc = cmrc::res::get_filesystem();
            auto file = fs_cmrc.open(embeddedResPath);
            std::vector<u8> data(file.begin(), file.end());
            return DecodeDdsToPng(data);
        } catch (...) {
        }
    }
    return {};
}

static std::vector<fs::path> GetTextureCandidatePaths(const fs::path& customDir,
                                                      const std::vector<std::string>& baseNames) {
    std::vector<fs::path> candidates;
    static constexpr std::array extensions = {"_4k.png", "_4k.dds", ".png", ".dds"};
    const std::vector<fs::path> search_dirs = {
        customDir,
        customDir / "textures",
        customDir / "Sce.Vsh.ShellUI.Notification" / "textures",
        customDir / "notification" / "textures",
    };

    for (const auto& base : baseNames) {
        for (const auto& dir : search_dirs) {
            for (const auto* ext : extensions) {
                candidates.push_back(dir / (base + ext));
            }
        }
    }
    return candidates;
}

static void DrawNinePatch(ImDrawList* draw_list, ImTextureID tex_id, ImVec2 dst_min, ImVec2 dst_max,
                          float tex_w, float tex_h, float scale, ImU32 col, bool is_right_side,
                          bool src_left_shadow = false) {
    if (!draw_list || !tex_id || tex_w <= 0.0f || tex_h <= 0.0f) {
        return;
    }

    const float ref_scale = tex_h / 56.0f;
    const float src_m_l = src_left_shadow ? 32.0f : 102.0f;
    const float src_m_r = src_left_shadow ? 102.0f : 32.0f;
    const float m_l = src_m_l * ref_scale;
    const float m_t = 18.0f * ref_scale;
    const float m_r = src_m_r * ref_scale;
    const float m_b = 34.0f * ref_scale;

    const bool flip_h = (is_right_side != src_left_shadow);
    float dst_m_l = (flip_h ? src_m_r : src_m_l) * scale;
    float dst_m_r = (flip_h ? src_m_l : src_m_r) * scale;
    float dst_m_t = 18.0f * scale;
    float dst_m_b = 34.0f * scale;

    const float dst_w = dst_max.x - dst_min.x;
    const float dst_h = dst_max.y - dst_min.y;

    if (dst_m_l + dst_m_r > dst_w) {
        const float f = dst_w / (dst_m_l + dst_m_r);
        dst_m_l *= f;
        dst_m_r *= f;
    }
    if (dst_m_t + dst_m_b > dst_h) {
        const float f = dst_h / (dst_m_t + dst_m_b);
        dst_m_t *= f;
        dst_m_b *= f;
    }

    const float dx[4] = {dst_min.x, dst_min.x + dst_m_l, dst_max.x - dst_m_r, dst_max.x};
    const float dy[4] = {dst_min.y, dst_min.y + dst_m_t, dst_max.y - dst_m_b, dst_max.y};

    float u[4] = {0.0f, m_l / tex_w, (tex_w - m_r) / tex_w, 1.0f};
    const float v[4] = {0.0f, m_t / tex_h, (tex_h - m_b) / tex_h, 1.0f};

    if (flip_h) {
        std::swap(u[0], u[3]);
        std::swap(u[1], u[2]);
    }

    for (int row = 0; row < 3; ++row) {
        for (int col_idx = 0; col_idx < 3; ++col_idx) {
            const ImVec2 p_min(dx[col_idx], dy[row]);
            const ImVec2 p_max(dx[col_idx + 1], dy[row + 1]);
            if (p_max.x <= p_min.x || p_max.y <= p_min.y) {
                continue;
            }
            draw_list->AddImage(tex_id, p_min, p_max, ImVec2(u[col_idx], v[row]),
                                ImVec2(u[col_idx + 1], v[row + 1]), col);
        }
    }
}

static std::string TruncateWithEllipsis(ImFont* font, float font_size, float max_w,
                                        std::string text) {
    if (!font || font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text.c_str()).x <= max_w) {
        return text;
    }
    while (!text.empty() &&
           font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, (text + "...").c_str()).x > max_w) {
        while (!text.empty() && (static_cast<u8>(text.back()) & 0xC0) == 0x80) {
            text.pop_back();
        }
        if (!text.empty()) {
            text.pop_back();
        }
    }
    return text + "...";
}

static std::pair<std::string, std::string> SplitTwoLines(ImFont* font, float font_size, float max_w,
                                                         const std::string& text) {
    if (!font || font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text.c_str()).x <= max_w) {
        return {text, {}};
    }
    size_t cut = 0;
    size_t last_space = std::string::npos;
    for (size_t i = 0; i < text.size();) {
        const u8 b = static_cast<u8>(text[i]);
        size_t next = i + ((b >= 0xF0) ? 4 : (b >= 0xE0) ? 3 : (b >= 0xC0) ? 2 : 1);
        next = std::min(next, text.size());
        if (font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text.c_str(), text.c_str() + next).x >
            max_w) {
            break;
        }
        if (text[i] == ' ') {
            last_space = i;
        }
        cut = next;
        i = next;
    }
    if (cut == 0) {
        return {TruncateWithEllipsis(font, font_size, max_w, text), {}};
    }
    if (last_space != std::string::npos && last_space > 0) {
        return {text.substr(0, last_space), text.substr(last_space + 1)};
    }
    return {text.substr(0, cut), text.substr(cut)};
}

struct CategoryIconSpec {
    NotificationIcon icon;
    std::initializer_list<const char*> names;
    bool gray_bg = false;
    bool no_bg = false;
};

static const CategoryIconSpec* FindCategoryIconSpec(NotificationIcon iconType) {
    static constexpr std::array<CategoryIconSpec, 8> kSpecs = {{
        {NotificationIcon::Error, {"tex_icon_ban", "icon_ban"}},
        {NotificationIcon::Capture, {"tex_icon_capture"}, false, true},
        {NotificationIcon::Download, {"tex_default_icon_download"}},
        {NotificationIcon::Upload, {"tex_default_icon_upload_16_9"}},
        {NotificationIcon::Party, {"tex_icon_party"}},
        {NotificationIcon::User, {"tex_default_icon_friend", "tex_default_avatar"}, false, true},
        {NotificationIcon::Device, {"tex_device_controller"}, true, false},
        {NotificationIcon::App, {"tex_default_icon_game"}},
    }};
    for (const auto& spec : kSpecs) {
        if (spec.icon == iconType) {
            return &spec;
        }
    }
    return nullptr;
}

static fs::path GetDiskAvatarCachePath(const std::string& target) {
    if (target.empty()) {
        return {};
    }
    std::string safe;
    safe.reserve(target.size());
    for (const char ch : target) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if ((uch >= 'a' && uch <= 'z') || (uch >= 'A' && uch <= 'Z') ||
            (uch >= '0' && uch <= '9') || ch == '-' || ch == '_') {
            safe.push_back(ch);
        } else {
            safe.push_back('_');
        }
    }
    return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "avatar_cache" /
           (safe + ".png");
}

static std::shared_ptr<AvatarFetchState> StartAvatarFetch(s32 userId, const std::string& target,
                                                          bool allow_self_fallback,
                                                          ImGui::RefCountedTexture& out_tex,
                                                          bool& out_applied_cached) {
    out_applied_cached = false;
    if (target.empty()) {
        return nullptr;
    }
    std::vector<u8> cached_png;
    {
        std::lock_guard lock(avatar_cache_mtx);
        if (auto it = avatar_cache.find(target); it != avatar_cache.end()) {
            cached_png = it->second;
        }
    }
    if (cached_png.empty()) {
        const fs::path disk_path = GetDiskAvatarCachePath(target);
        if (!disk_path.empty()) {
            cached_png = LoadAssetData({disk_path}, "");
            if (!cached_png.empty()) {
                std::lock_guard lock(avatar_cache_mtx);
                avatar_cache[target] = cached_png;
            }
        }
    }
    if (!cached_png.empty()) {
        out_tex = RefCountedTexture::DecodePngTexture(std::move(cached_png));
        out_applied_cached = true;
    }
    auto fetch = std::make_shared<AvatarFetchState>();
    std::thread([fetch, userId, target, allow_self_fallback] {
        auto& np = Libraries::Np::NpHandler::GetInstance();
        const s32 uid = (userId >= 0) ? userId : 1;
        auto card = np.FetchProfileCard(uid, target);
        if (!card.ok && allow_self_fallback && userId >= 0) {
            if (const u64 acc = np.GetAccountId(uid); acc != 0) {
                card = np.FetchProfileCard(uid, std::to_string(acc));
            }
        }
        if (card.ok && !card.avatar_png.empty()) {
            auto scaled = DownscalePngToMaxDim(std::move(card.avatar_png), 128);
            bool changed = true;
            {
                std::lock_guard lock(avatar_cache_mtx);
                if (auto it = avatar_cache.find(target);
                    it != avatar_cache.end() && it->second == scaled) {
                    changed = false;
                } else {
                    avatar_cache[target] = scaled;
                }
            }
            if (changed) {
                const fs::path disk_path = GetDiskAvatarCachePath(target);
                if (!disk_path.empty()) {
                    std::error_code ec;
                    fs::create_directories(disk_path.parent_path(), ec);
                    std::ofstream out(disk_path, std::ios::binary);
                    if (out) {
                        out.write(reinterpret_cast<const char*>(scaled.data()),
                                  static_cast<std::streamsize>(scaled.size()));
                    }
                }
                std::lock_guard lock(fetch->mutex);
                fetch->avatar_png = std::move(scaled);
            }
        }
        fetch->done.store(true);
    }).detach();
    return fetch;
}

SystemNotificationUI::SystemNotificationUI(const std::filesystem::path& iconPath,
                                           std::vector<u8> iconData, const std::string& title,
                                           const std::string& message, NotificationIcon iconType,
                                           NotificationSound soundType, s32 userId,
                                           const std::string& userName, BatteryStatus batteryStatus,
                                           const std::string& targetUser)
    : notif_title(title), notif_message(message), notif_user_name(userName),
      battery_status(batteryStatus), notif_user_id(userId), has_user_panel(!userName.empty()) {

    side = EmulatorSettings.GetTrophyNotificationSide();
    notif_timer = EmulatorSettings.GetTrophyNotificationDuration();

    const auto CustomTrophy_Dir = Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy);

    if (has_user_panel) {
        if (auto d = LoadAssetData(
                GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification_bk"}), "");
            !d.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(d));
            is_white_bg = false;
            bg_left_shadow = false;
        } else if (auto w = LoadAssetData(
                       GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification"}), "");
                   !w.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(w));
            is_white_bg = true;
            bg_left_shadow = false;
        }
    } else {
        if (auto d = LoadAssetData(GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_bk"}), "");
            !d.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(d));
            is_white_bg = false;
            bg_left_shadow = true;
        } else if (auto dn = LoadAssetData(
                       GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification_bk"}), "");
                   !dn.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(dn));
            is_white_bg = false;
            bg_left_shadow = false;
        } else if (auto w =
                       LoadAssetData(GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg"}), "");
                   !w.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(w));
            is_white_bg = true;
            bg_left_shadow = true;
        } else if (auto wn = LoadAssetData(
                       GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification"}), "");
                   !wn.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(std::move(wn));
            is_white_bg = true;
            bg_left_shadow = false;
        }
    }

    if (battery_status != BatteryStatus::None) {
        static constexpr std::array<std::pair<const char*, const char*>, 5> kBatteryNames = {{
            {"tex_device_battery_0", "tex_param_battery_0"},
            {"tex_device_battery_1", "tex_param_battery_1"},
            {"tex_device_battery_2", "tex_param_battery_2"},
            {"tex_device_battery_3", "tex_param_battery_3"},
            {"tex_device_battery_nocharge", "tex_param_battery_nocharge"},
        }};
        for (size_t i = 0; i < kBatteryNames.size(); ++i) {
            auto data =
                LoadAssetData(GetTextureCandidatePaths(CustomTrophy_Dir, {kBatteryNames[i].first,
                                                                          kBatteryNames[i].second}),
                              "");
            if (!data.empty()) {
                battery_icons[i] = RefCountedTexture::DecodePngTexture(std::move(data));
            }
        }
    }

    if (has_user_panel) {
        auto avatar_candidates = GetTextureCandidatePaths(
            CustomTrophy_Dir, {"tex_default_avatar", "tex_default_icon_friend"});
        if (auto avatar_data = LoadAssetData(avatar_candidates, ""); !avatar_data.empty()) {
            user_avatar = RefCountedTexture::DecodePngTexture(std::move(avatar_data));
        }
        bool applied_cached = false;
        avatar_fetch = StartAvatarFetch(userId, notif_user_name, true, user_avatar, applied_cached);
        has_fetched_user_avatar = applied_cached;
    }

    bool has_notif_icon = false;
    bool has_custom_uri_icon = false;
    if (!iconData.empty()) {
        auto decoded = DownscalePngToMaxDim(std::move(iconData), 128);
        if (!decoded.empty()) {
            notif_icon = RefCountedTexture::DecodePngTexture(std::move(decoded));
            has_notif_icon = true;
            has_custom_uri_icon = true;
            custom_icon_bg = true;
        }
    } else if (!iconPath.empty() && fs::exists(iconPath)) {
        auto file_data = LoadAssetData({iconPath}, "");
        if (!file_data.empty()) {
            file_data = DownscalePngToMaxDim(std::move(file_data), 128);
            notif_icon = RefCountedTexture::DecodePngTexture(std::move(file_data));
        } else {
            notif_icon = RefCountedTexture::DecodePngFile(iconPath);
        }
        has_notif_icon = true;
        has_custom_uri_icon = true;
        custom_icon_bg = true;
    }

    if (!has_notif_icon) {
        if (const auto* spec = FindCategoryIconSpec(iconType)) {
            std::vector<std::string> names(spec->names.begin(), spec->names.end());
            auto cat_data = LoadAssetData(GetTextureCandidatePaths(CustomTrophy_Dir, names), "");
            if (!cat_data.empty()) {
                notif_icon = RefCountedTexture::DecodePngTexture(std::move(cat_data));
                has_notif_icon = true;
                if (is_white_bg && !spec->no_bg) {
                    icon_bg_color =
                        spec->gray_bg ? IM_COL32(86, 86, 86, 255) : IM_COL32(0, 55, 145, 255);
                }
            }
        }
    }

    if (!has_custom_uri_icon && iconType == NotificationIcon::User && !targetUser.empty()) {
        bool applied_cached = false;
        body_avatar_fetch = StartAvatarFetch(userId, targetUser, false, notif_icon, applied_cached);
        if (applied_cached) {
            icon_bg_color = 0;
            custom_icon_bg = false;
            has_body_avatar = true;
        }
    }

    if (!has_notif_icon) {
        auto sys_candidates = GetTextureCandidatePaths(
            CustomTrophy_Dir, {"tex_icon_system", "icon_system", "tex_system_notification",
                               "tex_default_icon_notification"});
        auto sys_icon_data = LoadAssetData(sys_candidates, "");
        if (!sys_icon_data.empty()) {
            notif_icon = RefCountedTexture::DecodePngTexture(std::move(sys_icon_data));
            icon_bg_color = is_white_bg ? IM_COL32(0, 55, 145, 255) : 0;
        } else {
            auto embedded_data = LoadAssetData({}, "src/resources/shadps4.png");
            if (!embedded_data.empty()) {
                notif_icon = RefCountedTexture::DecodePngTexture(std::move(embedded_data));
                icon_bg_color = 0;
            }
        }
    }

    AddLayer(this);

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            LOG_ERROR(Lib_SystemService, "Unable to init SDL Audio for notification sound: {}",
                      SDL_GetError());
            return;
        }
    }

    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    std::string targetName;
    if (EmulatorSettings.GetAudioBackend() == 0 &&
        EmulatorSettings.GetSDLMainOutputDevice() != "Default Device") {
        targetName = EmulatorSettings.GetSDLMainOutputDevice();
    } else if (EmulatorSettings.GetAudioBackend() == 1 &&
               EmulatorSettings.GetOpenALMainOutputDevice() != "Default Device") {
        targetName = EmulatorSettings.GetOpenALMainOutputDevice();
    }

    if (!targetName.empty()) {
        int count = 0;
        if (SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count)) {
            for (int i = 0; i < count; ++i) {
                const char* rawName = SDL_GetAudioDeviceName(devices[i]);
                if (!rawName) {
                    continue;
                }
                std::string_view name(rawName);
                if (EmulatorSettings.GetAudioBackend() == 1 && name.size() > 15) {
                    name.remove_prefix(15);
                }
                if (name == targetName) {
                    const SDL_AudioDeviceID customDev = SDL_OpenAudioDevice(devices[i], nullptr);
                    if (customDev != 0) {
                        if (audioDevice != 0) {
                            SDL_CloseAudioDevice(audioDevice);
                        }
                        audioDevice = customDev;
                    }
                    break;
                }
            }
            SDL_free(devices);
        }
    }

    if (audioDevice == 0) {
        LOG_ERROR(Lib_SystemService,
                  "Unable to open audio device for notification sound playback: {}",
                  SDL_GetError());
        return;
    }

    static constexpr std::array<const char*, 5> party_enter_sounds = {
        "snd_msg_party_enter",  "se_msg_party_enter", "snd_msg_system_msg",
        "snd_msg_notification", "snd_msg_trophy",
    };
    static constexpr std::array<const char*, 5> party_quit_sounds = {
        "snd_msg_party_quit",   "se_msg_party_quit", "snd_msg_system_msg",
        "snd_msg_notification", "snd_msg_trophy",
    };
    static constexpr std::array<const char*, 5> psbutton_sounds = {
        "snd_msg_ng_psbutton",  "se_msg_ng_psbutton", "snd_msg_system_msg",
        "snd_msg_notification", "snd_msg_trophy",
    };
    static constexpr std::array<const char*, 5> capture_sounds = {
        "snd_ss_captured",      "se_ss_captured", "snd_msg_system_msg",
        "snd_msg_notification", "snd_msg_trophy",
    };
    static constexpr std::array<const char*, 5> app_sounds = {
        "snd_msg_notification", "se_msg_notification", "notification",
        "snd_msg_system_msg",   "snd_msg_trophy",
    };
    static constexpr std::array<const char*, 5> sys_sounds = {
        "snd_msg_system_msg",   "se_msg_system_msg", "system_msg",
        "snd_msg_notification", "snd_msg_trophy",
    };

    const auto& sound_names = [&]() -> const std::array<const char*, 5>& {
        switch (soundType) {
        case NotificationSound::PartyEnter:
            return party_enter_sounds;
        case NotificationSound::PartyQuit:
            return party_quit_sounds;
        case NotificationSound::PsButton:
            return psbutton_sounds;
        case NotificationSound::Capture:
            return capture_sounds;
        case NotificationSound::Notification:
            return app_sounds;
        case NotificationSound::System:
        default:
            return sys_sounds;
        }
    }();
    static constexpr std::array audio_extensions = {".wav", ".mp3"};

    for (const auto* name : sound_names) {
        for (const auto* ext : audio_extensions) {
            const auto path = CustomTrophy_Dir / (std::string(name) + ext);
            if (fs::exists(path)) {
                auto data = LoadAssetData({path}, "");
                if (!data.empty()) {
                    if (std::string_view(ext) == ".wav") {
                        PlayWav(data);
                    } else {
                        PlayMp3(data);
                    }
                    return;
                }
            }
        }
    }
}

SystemNotificationUI::~SystemNotificationUI() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    if (EmulatorSettings.GetAudioBackend() != 0) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    Finish();
}

void SystemNotificationUI::Finish() {
    RemoveLayer(this);
}

void SystemNotificationUI::Draw() {
    const auto& io = ImGui::GetIO();

    const float display_w = io.DisplaySize.x;
    const float display_h = io.DisplaySize.y;
    if (display_w <= 0.0f || display_h <= 0.0f) {
        return;
    }

    constexpr float ref_w = 1920.0f;
    constexpr float ref_h = 1080.0f;
    constexpr float target_aspect = ref_w / ref_h;
    const float current_aspect = display_w / display_h;

    float vp_y = 0.0f;
    float vp_h = display_h;
    if (current_aspect <= target_aspect) {
        vp_h = display_w / target_aspect;
        vp_y = (display_h - vp_h) * 0.5f;
    }

    const float base_scale = vp_h / ref_h;
    const float scale = (base_scale < 1.0f) ? (0.60f + 0.40f * base_scale) : base_scale;

    if (avatar_fetch && avatar_fetch->done.exchange(false)) {
        std::vector<u8> png;
        {
            std::lock_guard lock(avatar_fetch->mutex);
            png = std::move(avatar_fetch->avatar_png);
        }
        if (!png.empty()) {
            user_avatar = RefCountedTexture::DecodePngTexture(std::move(png));
            has_fetched_user_avatar = true;
        }
        avatar_fetch.reset();
    }
    if (body_avatar_fetch && body_avatar_fetch->done.exchange(false)) {
        std::vector<u8> png;
        {
            std::lock_guard lock(body_avatar_fetch->mutex);
            png = std::move(body_avatar_fetch->avatar_png);
        }
        if (!png.empty()) {
            notif_icon = RefCountedTexture::DecodePngTexture(std::move(png));
            icon_bg_color = 0;
            custom_icon_bg = false;
            has_body_avatar = true;
        }
        body_avatar_fetch.reset();
    }

    ImFont* title_font = ImGui::FontStack::GetTrophyTitleFont();
    ImFont* desc_font = ImGui::FontStack::GetTrophyDescFont();
    if (!title_font) {
        title_font = ImGui::GetFont();
    }
    if (!desc_font) {
        desc_font = ImGui::GetFont();
    }

    const bool is_device_handover = (battery_status != BatteryStatus::None);
    const bool is_icon_only =
        (!is_device_handover && !has_user_panel && notif_title.empty() && notif_message.empty());
    const bool is_single_line = !is_icon_only && (notif_title.empty() || notif_message.empty());
    const float header_h = has_user_panel ? 56.0f : 0.0f;
    const float body_h = is_device_handover ? 70.0f : (has_user_panel ? 92.0f : 100.0f);
    const float hdr_font_size = 22.0f * scale;
    const float title_font_size = 30.0f * scale;
    const float desc_font_size = 24.0f * scale;
    const float avatar_size = 34.0f * scale;
    const float right_pad = 48.0f * scale;

    float needed_w = 0.0f;
    if (has_user_panel) {
        needed_w = std::max(
            needed_w,
            102.0f * scale + avatar_size + 14.0f * scale +
                title_font->CalcTextSizeA(hdr_font_size, FLT_MAX, 0.0f, notif_user_name.c_str()).x +
                right_pad);
    }
    if (is_device_handover) {
        needed_w = std::max(needed_w, 102.0f * scale + avatar_size + 14.0f * scale + 96.0f * scale +
                                          right_pad);
    } else if (is_icon_only) {
        needed_w = 272.0f * scale;
    } else if (is_single_line) {
        const std::string& single_text = notif_title.empty() ? notif_message : notif_title;
        const float max_single_text_w = (664.0f - 189.0f) * scale - right_pad;
        const float w_at_title =
            desc_font->CalcTextSizeA(title_font_size, FLT_MAX, 0.0f, single_text.c_str()).x;
        if (w_at_title <= max_single_text_w) {
            needed_w = std::max(needed_w, 189.0f * scale + w_at_title + right_pad);
        } else {
            auto [l1, l2] =
                SplitTwoLines(desc_font, desc_font_size, max_single_text_w, single_text);
            const float w1 = desc_font->CalcTextSizeA(desc_font_size, FLT_MAX, 0.0f, l1.c_str()).x;
            const float w2 = desc_font->CalcTextSizeA(desc_font_size, FLT_MAX, 0.0f, l2.c_str()).x;
            needed_w = std::max(needed_w, 189.0f * scale + std::max(w1, w2) + right_pad);
        }
    } else {
        const float tw =
            title_font->CalcTextSizeA(title_font_size, FLT_MAX, 0.0f, notif_title.c_str()).x;
        const float mw =
            desc_font->CalcTextSizeA(desc_font_size, FLT_MAX, 0.0f, notif_message.c_str()).x;
        needed_w = std::max(needed_w, 189.0f * scale + std::max(tw, mw) + right_pad);
    }

    const float min_card_w = is_icon_only ? (240.0f * scale) : (320.0f * scale);
    const float card_width = std::clamp(needed_w, min_card_w, 664.0f * scale);
    const float card_height = (4.0f + header_h + body_h + 21.0f) * scale;
    const ImVec2 window_size{card_width, card_height};

    elapsed_time += io.DeltaTime;
    notif_timer -= io.DeltaTime;

    const float in_progress = std::min(elapsed_time / 0.4f, 1.0f);
    const float in_ease = 1.0f - std::pow(1.0f - in_progress, 3.0f);

    float out_ease = 1.0f;
    if (notif_timer <= 0.4f) {
        const float out_p = std::clamp((0.4f - static_cast<float>(notif_timer)) / 0.4f, 0.0f, 1.0f);
        out_ease = 1.0f - (out_p * out_p);
    }

    float final_pos_x, start_x;
    float final_pos_y, start_y;

    const bool is_right_side = (side == "right");
    const float retail_y = vp_y + 110.0f * scale;

    if (side == "top") {
        start_x = (display_w - window_size.x) * 0.5f;
        start_y = -window_size.y;
        final_pos_x = start_x;
        final_pos_y = retail_y;
    } else if (is_right_side) {
        start_x = display_w;
        start_y = retail_y;
        final_pos_x = display_w - window_size.x;
        final_pos_y = retail_y;
    } else if (side == "bottom") {
        start_x = -window_size.x;
        final_pos_y = display_h - window_size.y - (vp_y + 40.0f * scale);
        start_y = final_pos_y;
        final_pos_x = 0.0f;
    } else {
        start_x = -window_size.x;
        start_y = retail_y;
        final_pos_x = 0.0f;
        final_pos_y = retail_y;
    }

    ImVec2 current_pos;
    if (notif_timer <= 0.4f) {
        current_pos.x = ImLerp(start_x, final_pos_x, out_ease);
        current_pos.y = ImLerp(start_y, final_pos_y, out_ease);
    } else {
        current_pos.x = ImLerp(start_x, final_pos_x, in_ease);
        current_pos.y = ImLerp(start_y, final_pos_y, in_ease);
    }

    const float alpha = std::clamp(out_ease, 0.0f, 1.0f);

    ImGui::SetNextWindowPos(current_pos);
    ImGui::SetNextWindowSize(window_size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::Begin("##SystemNotificationWindow", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 p1(p0.x + window_size.x, p0.y + window_size.y);
        const float solid_left_x = is_right_side ? (p0.x + 20.0f * scale) : p0.x;
        const float solid_right_x = is_right_side ? p1.x : (p1.x - 20.0f * scale);
        const float solid_top_y = p0.y + 4.0f * scale;
        const float card_center_y = p0.y + (4.0f + header_h + body_h * 0.5f) * scale;

        if (notification_bg_icon) {
            const auto bg_tex = notification_bg_icon.GetTexture();
            DrawNinePatch(draw_list, bg_tex.im_id, p0, p1, static_cast<float>(bg_tex.width),
                          static_cast<float>(bg_tex.height), scale,
                          IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)), is_right_side,
                          bg_left_shadow);
        } else {
            const float top_y = solid_top_y;
            const float bot_y = p1.y - 21.0f * scale;
            const float right_x = is_right_side ? p1.x : (p0.x + window_size.x - 30.0f * scale);
            const float left_x = is_right_side ? (p1.x - window_size.x + 30.0f * scale) : p0.x;
            const float rounding = 16.0f * scale;
            const ImDrawFlags flags =
                is_right_side ? ImDrawFlags_RoundCornersLeft : ImDrawFlags_RoundCornersRight;

            const ImU32 bg_color = is_white_bg
                                       ? IM_COL32(245, 245, 248, static_cast<int>(250 * alpha))
                                       : IM_COL32(18, 18, 20, static_cast<int>(245 * alpha));
            draw_list->AddRectFilled(ImVec2(left_x, top_y), ImVec2(right_x, bot_y), bg_color,
                                     rounding, flags);

            const ImU32 border_col = is_white_bg
                                         ? IM_COL32(210, 210, 215, static_cast<int>(230 * alpha))
                                         : IM_COL32(65, 68, 72, static_cast<int>(230 * alpha));
            draw_list->AddRect(ImVec2(left_x, top_y), ImVec2(right_x, bot_y), border_col, rounding,
                               flags, 1.5f * scale);
        }

        const float icon_size = 64.0f * scale;
        float icon_x;
        float text_start_x;
        float text_max_w;

        if (is_right_side) {
            icon_x = p0.x + window_size.x - 102.0f * scale - icon_size;
            text_start_x = p0.x + right_pad;
            text_max_w = (icon_x - 16.0f * scale) - text_start_x;
        } else {
            icon_x = p0.x + 102.0f * scale;
            text_start_x = p0.x + 189.0f * scale;
            text_max_w = (p0.x + window_size.x - right_pad) - text_start_x;
        }

        const float avatar_x = is_right_side ? (solid_right_x - 102.0f * scale - avatar_size)
                                             : (p0.x + 102.0f * scale);

        if (has_user_panel) {
            const float panel_h = 54.0f * scale;
            const float sep_h = 2.0f * scale;
            if (is_white_bg) {
                draw_list->AddRectFilled(ImVec2(solid_left_x, solid_top_y),
                                         ImVec2(solid_right_x, solid_top_y + panel_h),
                                         IM_COL32(120, 120, 120, static_cast<int>(255 * alpha)));
            }
            draw_list->AddRectFilled(ImVec2(solid_left_x, solid_top_y + panel_h),
                                     ImVec2(solid_right_x, solid_top_y + panel_h + sep_h),
                                     IM_COL32(0, 120, 200, static_cast<int>(255 * alpha)));

            const float avatar_y = solid_top_y + (panel_h - avatar_size) * 0.5f;
            if (user_avatar) {
                const auto av_tex = user_avatar.GetTexture();
                if (has_fetched_user_avatar) {
                    draw_list->AddImageRounded(
                        av_tex.im_id, ImVec2(avatar_x, avatar_y),
                        ImVec2(avatar_x + avatar_size, avatar_y + avatar_size), ImVec2(0.0f, 0.0f),
                        ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)),
                        avatar_size * 0.5f);
                } else {
                    draw_list->AddImage(av_tex.im_id, ImVec2(avatar_x, avatar_y),
                                        ImVec2(avatar_x + avatar_size, avatar_y + avatar_size),
                                        ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                        IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
                }
            } else {
                draw_list->AddRectFilled(ImVec2(avatar_x, avatar_y),
                                         ImVec2(avatar_x + avatar_size, avatar_y + avatar_size),
                                         IM_COL32(86, 86, 86, static_cast<int>(255 * alpha)));
            }

            const float hdr_y = solid_top_y + (panel_h - hdr_font_size) * 0.5f;
            const ImU32 hdr_col = IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));
            if (is_right_side) {
                const float max_w = (avatar_x - 14.0f * scale) - text_start_x;
                const std::string disp_user =
                    TruncateWithEllipsis(title_font, hdr_font_size, max_w, notif_user_name);
                const float tw =
                    title_font->CalcTextSizeA(hdr_font_size, FLT_MAX, 0.0f, disp_user.c_str()).x;
                draw_list->AddText(title_font, hdr_font_size,
                                   ImVec2(avatar_x - 14.0f * scale - tw, hdr_y), hdr_col,
                                   disp_user.c_str());
            } else {
                const float uname_x = avatar_x + avatar_size + 14.0f * scale;
                const float max_w = (solid_right_x - 24.0f * scale) - uname_x;
                const std::string disp_user =
                    TruncateWithEllipsis(title_font, hdr_font_size, max_w, notif_user_name);
                draw_list->AddText(title_font, hdr_font_size, ImVec2(uname_x, hdr_y), hdr_col,
                                   disp_user.c_str());
            }
        }

        if (is_device_handover) {
            const float dev_w = 37.0f * scale;
            const float gap = 6.0f * scale;
            const float bat_w = 53.0f * scale;
            const float bat_h = 34.0f * scale;
            const float dev_x = is_right_side ? (avatar_x - 14.0f * scale - (dev_w + gap + bat_w))
                                              : (avatar_x + avatar_size + 14.0f * scale);
            const float dev_y = card_center_y - dev_w * 0.5f;
            const ImU32 tint = is_white_bg ? IM_COL32(45, 45, 50, static_cast<int>(255 * alpha))
                                           : IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));
            if (notif_icon) {
                const auto dev_tex = notif_icon.GetTexture();
                draw_list->AddImage(dev_tex.im_id, ImVec2(dev_x, dev_y),
                                    ImVec2(dev_x + dev_w, dev_y + dev_w), ImVec2(0.0f, 0.0f),
                                    ImVec2(1.0f, 1.0f), tint);
            }
            size_t bat_idx = 4;
            bool show_bat = true;
            switch (battery_status) {
            case BatteryStatus::Level0:
                bat_idx = 0;
                break;
            case BatteryStatus::LowBlinking:
                bat_idx = 0;
                show_bat = (static_cast<int>(elapsed_time / 0.5f) % 2) == 0;
                break;
            case BatteryStatus::Level1:
                bat_idx = 1;
                break;
            case BatteryStatus::Level2:
                bat_idx = 2;
                break;
            case BatteryStatus::Level3:
                bat_idx = 3;
                break;
            case BatteryStatus::Charging:
                bat_idx = static_cast<size_t>(elapsed_time / 0.45f) % 4;
                break;
            case BatteryStatus::NoCharge:
            default:
                bat_idx = 4;
                break;
            }
            if (show_bat && battery_icons[bat_idx]) {
                const auto bat_tex = battery_icons[bat_idx].GetTexture();
                float draw_bat_w = bat_w;
                float draw_bat_h = bat_h;
                if (bat_tex.width > 0 && bat_tex.height > 0 &&
                    bat_tex.height * 10 == bat_tex.width * 7) {
                    draw_bat_h = 30.0f * scale;
                    draw_bat_w = 43.0f * scale;
                }
                const float bat_x = dev_x + dev_w + gap;
                const float bat_y = card_center_y - draw_bat_h * 0.5f;
                draw_list->AddImage(bat_tex.im_id, ImVec2(bat_x, bat_y),
                                    ImVec2(bat_x + draw_bat_w, bat_y + draw_bat_h),
                                    ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
            }
        } else {
            const float icon_y = card_center_y - icon_size * 0.5f;

            if (notif_icon) {
                const auto icon_tex = notif_icon.GetTexture();
                u32 resolved_bg = icon_bg_color;
                if (resolved_bg == 0 && custom_icon_bg && is_white_bg && icon_tex.width > 0 &&
                    icon_tex.width != icon_tex.height) {
                    resolved_bg = IM_COL32(86, 86, 86, 255);
                }
                if (resolved_bg != 0) {
                    const ImU32 bg_col =
                        (resolved_bg & 0x00FFFFFFu) | (static_cast<u32>(255 * alpha) << 24);
                    draw_list->AddRectFilled(ImVec2(icon_x, icon_y),
                                             ImVec2(icon_x + icon_size, icon_y + icon_size),
                                             bg_col);
                }
                float draw_w = icon_size;
                float draw_h = icon_size;
                if (icon_tex.width > 0 && icon_tex.height > 0 &&
                    icon_tex.width != icon_tex.height) {
                    const float aspect =
                        static_cast<float>(icon_tex.width) / static_cast<float>(icon_tex.height);
                    if (aspect > 1.0f) {
                        draw_h = icon_size / aspect;
                    } else {
                        draw_w = icon_size * aspect;
                    }
                }
                const float draw_x = icon_x + (icon_size - draw_w) * 0.5f;
                const float draw_y = icon_y + (icon_size - draw_h) * 0.5f;
                if (has_body_avatar) {
                    draw_list->AddImageRounded(
                        icon_tex.im_id, ImVec2(draw_x, draw_y),
                        ImVec2(draw_x + draw_w, draw_y + draw_h), ImVec2(0.0f, 0.0f),
                        ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)),
                        std::min(draw_w, draw_h) * 0.5f);
                } else {
                    draw_list->AddImage(icon_tex.im_id, ImVec2(draw_x, draw_y),
                                        ImVec2(draw_x + draw_w, draw_y + draw_h),
                                        ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                        IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
                }
            } else {
                draw_list->AddRectFilled(
                    ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_size, icon_y + icon_size),
                    IM_COL32(30, 30, 35, static_cast<int>(255 * alpha)), 4.0f * scale);
                draw_list->AddRect(
                    ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_size, icon_y + icon_size),
                    IM_COL32(80, 80, 90, static_cast<int>(180 * alpha)), 4.0f * scale);
            }

            if (!is_icon_only) {
                const ImU32 header_col =
                    is_white_bg ? IM_COL32(25, 25, 30, static_cast<int>(255 * alpha))
                                : IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));
                const bool is_error_sub =
                    notif_message.starts_with("[Error]") || notif_message.starts_with("[Failed]");
                const ImU32 sub_col =
                    is_error_sub
                        ? IM_COL32(255, 0, 0, static_cast<int>(255 * alpha))
                        : (is_white_bg ? IM_COL32(85, 90, 100, static_cast<int>(235 * alpha))
                                       : IM_COL32(215, 220, 230, static_cast<int>(245 * alpha)));

                if (is_single_line) {
                    const std::string& single_text =
                        notif_title.empty() ? notif_message : notif_title;
                    if (desc_font
                            ->CalcTextSizeA(title_font_size, FLT_MAX, 0.0f, single_text.c_str())
                            .x <= text_max_w) {
                        const float single_y = card_center_y - title_font_size * 0.5f;
                        draw_list->AddText(desc_font, title_font_size,
                                           ImVec2(text_start_x, single_y), header_col,
                                           single_text.c_str());
                    } else {
                        auto [line1, line2] =
                            SplitTwoLines(desc_font, desc_font_size, text_max_w, single_text);
                        if (line2.empty()) {
                            const float single_y = card_center_y - desc_font_size * 0.5f;
                            draw_list->AddText(desc_font, desc_font_size,
                                               ImVec2(text_start_x, single_y), header_col,
                                               line1.c_str());
                        } else {
                            const float block_top = card_center_y - 27.0f * scale;
                            draw_list->AddText(desc_font, desc_font_size,
                                               ImVec2(text_start_x, block_top), header_col,
                                               line1.c_str());
                            const std::string line2_disp =
                                TruncateWithEllipsis(desc_font, desc_font_size, text_max_w, line2);
                            draw_list->AddText(desc_font, desc_font_size,
                                               ImVec2(text_start_x, block_top + 30.0f * scale),
                                               header_col, line2_disp.c_str());
                        }
                    }
                } else {
                    const float line1_y = p0.y + (4.0f + header_h + 18.0f) * scale;
                    const std::string display_title =
                        TruncateWithEllipsis(title_font, title_font_size, text_max_w, notif_title);
                    draw_list->AddText(title_font, title_font_size, ImVec2(text_start_x, line1_y),
                                       header_col, display_title.c_str());

                    const float line2_y = p0.y + (4.0f + header_h + 57.0f) * scale;
                    const std::string display_name =
                        TruncateWithEllipsis(desc_font, desc_font_size, text_max_w, notif_message);
                    draw_list->AddText(desc_font, desc_font_size, ImVec2(text_start_x, line2_y),
                                       sub_col, display_name.c_str());
                }
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);

    if (notif_timer <= 0) {
        bool notify_trophy_queue = false;
        {
            std::lock_guard<std::mutex> lock(queueMtx);
            if (!notif_queue.empty() && !Libraries::Np::NpTrophy::IsTrophyShowing()) {
                PopAndShowNextLocked();
            } else {
                current_notif_ui.reset();
                notif_showing.store(false);
                notify_trophy_queue = true;
            }
        }
        if (notify_trophy_queue) {
            Libraries::Np::NpTrophy::ShowNextQueuedTrophy();
        }
    }
}

void SystemNotificationUI::PlayMp3(std::span<const u8> mp3Data) {
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    std::vector<short> pcm(MINIMP3_MAX_SAMPLES_PER_FRAME);
    mp3dec_init(&mp3d);

    SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, 44100};
    bool specInfoSet = false;

    stream = SDL_CreateAudioStream(&spec, &spec);
    if (!stream || !SDL_BindAudioStream(audioDevice, stream)) {
        return;
    }

    SDL_SetAudioStreamGain(stream,
                           static_cast<float>(EmulatorSettings.GetVolumeSlider() * 0.01f * 1.2f));
    const unsigned char* buffer_ptr = mp3Data.data();
    size_t remaining_size = mp3Data.size();

    while (remaining_size > 0) {
        const int samples =
            mp3dec_decode_frame(&mp3d, buffer_ptr, remaining_size, pcm.data(), &info);
        if (samples > 0) {
            if (!specInfoSet && info.hz > 0 && info.channels > 0) {
                spec = {SDL_AUDIO_S16, info.channels, info.hz};
                SDL_SetAudioStreamFormat(stream, &spec, &spec);
                specInfoSet = true;
            }
            SDL_PutAudioStreamData(stream, pcm.data(), samples * info.channels * sizeof(short));
            buffer_ptr += info.frame_bytes;
            remaining_size -= info.frame_bytes;
        } else {
            break;
        }
    }
    SDL_ResumeAudioDevice(audioDevice);
}

void SystemNotificationUI::PlayWav(std::span<const u8> wavData) {
    SDL_AudioSpec spec;
    Uint8* audioBuf = nullptr;
    Uint32 audioLen = 0;

    SDL_IOStream* io = SDL_IOFromConstMem(wavData.data(), wavData.size());
    if (!SDL_LoadWAV_IO(io, true, &spec, &audioBuf, &audioLen)) {
        LOG_ERROR(Lib_SystemService, "Unable to load notification wave file data: {}",
                  SDL_GetError());
        return;
    }

    stream = SDL_CreateAudioStream(&spec, &spec);
    if (!stream || !SDL_BindAudioStream(audioDevice, stream)) {
        SDL_free(audioBuf);
        return;
    }

    SDL_SetAudioStreamGain(stream,
                           static_cast<float>(EmulatorSettings.GetVolumeSlider() * 0.01f * 1.2f));
    SDL_PutAudioStreamData(stream, audioBuf, audioLen);
    SDL_free(audioBuf);
    SDL_ResumeAudioDevice(audioDevice);
}

bool IsNotificationShowing() {
    return notif_showing.load();
}

static void PopAndShowNextLocked() {
    NotificationInfo next = std::move(notif_queue.front());
    notif_queue.pop();
    notif_showing.store(true);
    current_notif_ui.emplace(next.icon_path, std::move(next.icon_data), next.title, next.message,
                             next.icon_type, next.sound_type, next.user_id, next.user_name,
                             next.battery_status, next.target_user);
}

void ShowNextQueuedNotification() {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (!notif_queue.empty() && !current_notif_ui.has_value() &&
        !Libraries::Np::NpTrophy::IsTrophyShowing()) {
        PopAndShowNextLocked();
    }
}

void AddNotificationToQueue(const std::string& message, const std::string& title,
                            const std::filesystem::path& iconPath, std::vector<u8> iconData,
                            NotificationIcon iconType, NotificationSound soundType, s32 userId,
                            const std::string& userName, BatteryStatus batteryStatus,
                            const std::string& targetUser) {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (EmulatorSettings.IsTrophyPopupDisabled()) {
        return;
    }

    std::string final_title = title;
    std::string final_msg = message;

    if (final_title.empty() && !final_msg.empty()) {
        const auto nl_pos = final_msg.find('\n');
        if (nl_pos != std::string::npos) {
            final_title = final_msg.substr(0, nl_pos);
            final_msg = final_msg.substr(nl_pos + 1);
        }
    }

    NotificationInfo new_notif;
    new_notif.icon_path = iconPath;
    new_notif.icon_data = std::move(iconData);
    new_notif.title = std::move(final_title);
    new_notif.message = std::move(final_msg);
    new_notif.icon_type = iconType;
    new_notif.sound_type = soundType;
    new_notif.user_id = userId;
    new_notif.user_name = userName;
    new_notif.battery_status = batteryStatus;
    new_notif.target_user = targetUser;
    notif_queue.push(std::move(new_notif));

    if (!current_notif_ui.has_value() && !Libraries::Np::NpTrophy::IsTrophyShowing()) {
        PopAndShowNextLocked();
    }
}

void AddDeviceHandoverNotification(s32 userId, const std::string& userName,
                                   BatteryStatus batteryStatus) {
    const std::string resolved_user = userName.empty() ? std::string("User") : userName;
    AddNotificationToQueue("", "", {}, {}, NotificationIcon::Device, NotificationSound::PsButton,
                           userId, resolved_user, batteryStatus);
}

void UpdateDeviceBatteryStatus(s32 userId, BatteryStatus batteryStatus) {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (current_notif_ui.has_value() && current_notif_ui->notif_user_id == userId &&
        current_notif_ui->battery_status != BatteryStatus::None) {
        current_notif_ui->battery_status = batteryStatus;
    }
    const size_t qlen = notif_queue.size();
    for (size_t i = 0; i < qlen; ++i) {
        NotificationInfo item = std::move(notif_queue.front());
        notif_queue.pop();
        if (item.user_id == userId && item.battery_status != BatteryStatus::None) {
            item.battery_status = batteryStatus;
        }
        notif_queue.push(std::move(item));
    }
}

} // namespace Libraries::SystemNotification
