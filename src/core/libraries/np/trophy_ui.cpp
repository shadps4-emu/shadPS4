// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <fstream>
#include <mutex>
#include <SDL3/SDL_init.h>
#include <cmrc/cmrc.hpp>
#include <fmt/format.h>
#include <imgui.h>
#include <imgui/imgui_std.h>
#include <queue>

#define MINIMP3_IMPLEMENTATION
#include <minimp3_ex.h>

#include "common/logging/formatter.h"
#include "common/path_util.h"
#include "common/stb.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/trophy_ui.h"
#include "imgui/renderer/font_stack.h"

CMRC_DECLARE(res);
namespace fs = std::filesystem;
using namespace ImGui;
namespace Libraries::Np::NpTrophy {

std::optional<TrophyUI> current_trophy_ui;
std::queue<TrophyInfo> trophy_queue;
std::mutex queueMtx;

// Decompresses DDS DXT5 (BC3) texture data to in-memory PNG bytes
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

    // FourCC 'DXT5' is 0x35545844
    if (fourcc != 0x35545844) {
        return data;
    }

    std::vector<u32> rgba(width * height);
    const u8* ptr = data.data() + 128;
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    if (data.size() < 128 + static_cast<size_t>(bw) * bh * 16) {
        return data;
    }

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            u8 a0 = ptr[0];
            u8 a1 = ptr[1];
            u8 alpha[8];
            alpha[0] = a0;
            alpha[1] = a1;
            if (a0 > a1) {
                for (int i = 1; i <= 6; ++i) {
                    alpha[i + 1] = ((7 - i) * a0 + i * a1) / 7;
                }
            } else {
                for (int i = 1; i <= 4; ++i) {
                    alpha[i + 1] = ((5 - i) * a0 + i * a1) / 5;
                }
                alpha[6] = 0;
                alpha[7] = 255;
            }

            u64 a_bits = 0;
            for (int i = 0; i < 6; ++i) {
                a_bits |= static_cast<u64>(ptr[2 + i]) << (8 * i);
            }

            u16 c0 = ptr[8] | (ptr[9] << 8);
            u16 c1 = ptr[10] | (ptr[11] << 8);

            u8 r0 = ((c0 >> 11) & 0x1F) * 255 / 31;
            u8 g0 = ((c0 >> 5) & 0x3F) * 255 / 63;
            u8 b0 = (c0 & 0x1F) * 255 / 31;

            u8 r1 = ((c1 >> 11) & 0x1F) * 255 / 31;
            u8 g1 = ((c1 >> 5) & 0x3F) * 255 / 63;
            u8 b1 = (c1 & 0x1F) * 255 / 31;

            u32 colors[4];
            colors[0] = (255 << 24) | (b0 << 16) | (g0 << 8) | r0;
            colors[1] = (255 << 24) | (b1 << 16) | (g1 << 8) | r1;
            colors[2] = (255 << 24) | (((2 * b0 + b1) / 3) << 16) | (((2 * g0 + g1) / 3) << 8) |
                        ((2 * r0 + r1) / 3);
            colors[3] = (255 << 24) | (((b0 + 2 * b1) / 3) << 16) | (((g0 + 2 * g1) / 3) << 8) |
                        ((r0 + 2 * r1) / 3);

            u32 c_bits = ptr[12] | (ptr[13] << 8) | (ptr[14] << 16) | (ptr[15] << 24);
            ptr += 16;

            for (int y = 0; y < 4 && (by * 4 + y) < height; ++y) {
                for (int x = 0; x < 4 && (bx * 4 + x) < width; ++x) {
                    int idx = y * 4 + x;
                    u8 a_idx = (a_bits >> (idx * 3)) & 0x07;
                    u8 c_idx = (c_bits >> (idx * 2)) & 0x03;

                    u32 col = colors[c_idx] & 0x00FFFFFF;
                    u32 a_val = static_cast<u32>(alpha[a_idx]) << 24;
                    rgba[(by * 4 + y) * width + (bx * 4 + x)] = col | a_val;
                }
            }
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

static std::vector<u8> LoadAssetData(const std::vector<fs::path>& candidatePaths,
                                     const std::string& embeddedResPath) {
    for (const auto& path : candidatePaths) {
        std::error_code ec;
        if (fs::exists(path, ec)) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file) {
                const auto size = file.tellg();
                if (size > 0) {
                    file.seekg(0, std::ios::beg);
                    std::vector<u8> raw(static_cast<size_t>(size));
                    if (file.read(reinterpret_cast<char*>(raw.data()), size)) {
                        return DecodeDdsToPng(raw);
                    }
                }
            }
        }
    }
    if (!embeddedResPath.empty()) {
        try {
            auto resource = cmrc::res::get_filesystem();
            auto file = resource.open(embeddedResPath);
            return std::vector<u8>(file.begin(), file.end());
        } catch (...) {
        }
    }
    return {};
}

static std::vector<fs::path> GetTextureCandidatePaths(const fs::path& custom_dir,
                                                      const std::vector<std::string>& base_names) {
    std::vector<fs::path> candidates;
    const std::vector<std::string> extensions = {"_4k.png", "_4k.dds", ".png", ".dds"};
    const std::vector<fs::path> search_dirs = {
        custom_dir,
        custom_dir / "textures",
        custom_dir / "Sce.Vsh.ShellUI.Notification" / "textures",
        custom_dir / "Sce.Vsh.ShellUI.Trophy" / "textures",
    };

    for (const auto& base : base_names) {
        for (const auto& dir : search_dirs) {
            for (const auto& ext : extensions) {
                candidates.push_back(dir / (base + ext));
            }
        }
    }
    return candidates;
}

TrophyUI::TrophyUI(const std::filesystem::path& iconPath, const std::string& title,
                   const std::string& message, const std::string_view& rarity)
    : trophy_title(title), trophy_name(message), trophy_type(rarity) {

    side = EmulatorSettings.GetTrophyNotificationSide();
    trophy_timer = EmulatorSettings.GetTrophyNotificationDuration();

    const auto CustomTrophy_Dir = Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy);

    if (fs::exists(iconPath)) {
        trophy_icon = RefCountedTexture::DecodePngFile(iconPath);
    }

    if (true) {
        std::string base_name = "bronze";
        if (trophy_type == "P")
            base_name = "platinum";
        else if (trophy_type == "G")
            base_name = "gold";
        else if (trophy_type == "S")
            base_name = "silver";

        auto cup_candidates =
            GetTextureCandidatePaths(CustomTrophy_Dir, {
                                                           "tex_" + base_name + "_s",
                                                           base_name + "_s",
                                                           "tex_" + base_name,
                                                           base_name,
                                                       });
        auto cup_data = LoadAssetData(cup_candidates, "src/resources/" + base_name + ".png");
        if (!cup_data.empty()) {
            trophy_type_icon = RefCountedTexture::DecodePngTexture(cup_data);
        }
    } else {
        if (!trophy_icon) {
            auto sys_icon_candidates =
                GetTextureCandidatePaths(CustomTrophy_Dir, {
                                                               "tex_icon_system",
                                                               "icon_system",
                                                               "tex_system_notification",
                                                               "tex_default_icon_notification",
                                                           });
            auto sys_icon_data = LoadAssetData(sys_icon_candidates, "src/resources/shadps4.png");
            if (!sys_icon_data.empty()) {
                trophy_icon = RefCountedTexture::DecodePngTexture(sys_icon_data);
            }
        }
    }

    auto default_trophy_candidates =
        GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_default_trophy"});
    auto def_data = LoadAssetData(default_trophy_candidates, "");
    if (!def_data.empty()) {
        default_trophy_icon = RefCountedTexture::DecodePngTexture(def_data);
    }

    auto ps_candidates = GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_ps_button", "ps_button"});
    auto ps_data = LoadAssetData(ps_candidates, "src/resources/shadps4.png");
    if (!ps_data.empty()) {
        ps_button_icon = RefCountedTexture::DecodePngTexture(ps_data);
    }

    auto ps_appear_candidates =
        GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_ps_button_appear", "ps_button_appear"});
    auto ps_appear_data = LoadAssetData(ps_appear_candidates, "");
    if (!ps_appear_data.empty()) {
        ps_button_appear_icon = RefCountedTexture::DecodePngTexture(ps_appear_data);
    }

    auto bg_dark_candidates =
        GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification_bk"});
    auto bg_dark_data = LoadAssetData(bg_dark_candidates, "");
    if (!bg_dark_data.empty()) {
        notification_bg_icon = RefCountedTexture::DecodePngTexture(bg_dark_data);
        is_white_bg = false;
    } else {
        auto bg_white_candidates =
            GetTextureCandidatePaths(CustomTrophy_Dir, {"tex_bg_notification"});
        auto bg_white_data = LoadAssetData(bg_white_candidates, "");
        if (!bg_white_data.empty()) {
            notification_bg_icon = RefCountedTexture::DecodePngTexture(bg_white_data);
            is_white_bg = true;
        }
    }

    AddLayer(this);

    if (SDL_WasInit(SDL_INIT_AUDIO) != 0) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            LOG_ERROR(Lib_NpTrophy, "Unable to init SDL Audio for trophy sound: {}",
                      SDL_GetError());
            return;
        }
    }

    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    // user selected Sdl Backend, use same device as Sdl main Device
    if (EmulatorSettings.GetAudioBackend() == 0) {
        if (EmulatorSettings.GetSDLMainOutputDevice() != "Default Device") {
            int count;
            SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

            for (int i = 0; i < count; i++) {
                std::string name = SDL_GetAudioDeviceName(devices[i]);
                if (name == EmulatorSettings.GetSDLMainOutputDevice()) {
                    audioDevice = SDL_OpenAudioDevice(devices[i], NULL);
                }
            }
        }

        // user selected OpenAl Backend, use same device as OpenAl main Device
    } else if (EmulatorSettings.GetAudioBackend() == 1) {
        if (EmulatorSettings.GetOpenALMainOutputDevice() != "Default Device") {
            int count;
            SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

            for (int i = 0; i < count; i++) {
                std::string name = SDL_GetAudioDeviceName(devices[i]);
                name.erase(0, 15);
                if (name == EmulatorSettings.GetOpenALMainOutputDevice()) {
                    audioDevice = SDL_OpenAudioDevice(devices[i], NULL);
                }
            }
        }
    }

    if (audioDevice == 0) {
        LOG_ERROR(Lib_NpTrophy, "Unable to open audio device for trophy sound playback: {}",
                  SDL_GetError());
        return;
    }

    static constexpr std::array trophy_sound_names = {
        "snd_msg_trophy",
        "trophy",
    };
    static constexpr std::array audio_extensions = {".wav", ".mp3"};
    const auto sound_names = std::span<const char* const>(trophy_sound_names);

    bool audio_played = false;
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
                    audio_played = true;
                    break;
                }
            }
        }
        if (audio_played) {
            break;
        }
    }

    if (!audio_played) {
        auto fallback_wav = LoadAssetData({}, "src/resources/trophy.wav");
        if (!fallback_wav.empty()) {
            PlayWav(fallback_wav);
        }
    }
}

TrophyUI::~TrophyUI() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
    }

    // if emulator is not using sdl audio backend
    if (EmulatorSettings.GetAudioBackend() != 0) {
        SDL_CloseAudioDevice(audioDevice);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    Finish();
}

void TrophyUI::Finish() {
    RemoveLayer(this);
}

static void DrawNinePatch(ImDrawList* draw_list, ImTextureID tex_id, ImVec2 dst_min, ImVec2 dst_max,
                          float tex_w, float tex_h, float scale, ImU32 col, bool flip_h = false) {
    if (!draw_list || !tex_id || tex_w <= 0.0f || tex_h <= 0.0f) {
        return;
    }

    const float ref_scale = tex_h / 56.0f;
    float m_l = 102.0f * ref_scale;
    float m_t = 18.0f * ref_scale;
    float m_r = 32.0f * ref_scale;
    float m_b = 34.0f * ref_scale;

    float dst_m_l = (flip_h ? 32.0f : 102.0f) * scale;
    float dst_m_r = (flip_h ? 102.0f : 32.0f) * scale;
    float dst_m_t = 18.0f * scale;
    float dst_m_b = 34.0f * scale;

    float dst_w = dst_max.x - dst_min.x;
    float dst_h = dst_max.y - dst_min.y;

    if (dst_m_l + dst_m_r > dst_w) {
        float f = dst_w / (dst_m_l + dst_m_r);
        dst_m_l *= f;
        dst_m_r *= f;
    }
    if (dst_m_t + dst_m_b > dst_h) {
        float f = dst_h / (dst_m_t + dst_m_b);
        dst_m_t *= f;
        dst_m_b *= f;
    }

    float dx[4] = {dst_min.x, dst_min.x + dst_m_l, dst_max.x - dst_m_r, dst_max.x};
    float dy[4] = {dst_min.y, dst_min.y + dst_m_t, dst_max.y - dst_m_b, dst_max.y};

    float u0 = 0.0f;
    float u1 = m_l / tex_w;
    float u2 = (tex_w - m_r) / tex_w;
    float u3 = 1.0f;

    float v0 = 0.0f;
    float v1 = m_t / tex_h;
    float v2 = (tex_h - m_b) / tex_h;
    float v3 = 1.0f;

    float u[4];
    if (flip_h) {
        u[0] = u3;
        u[1] = u2;
        u[2] = u1;
        u[3] = u0;
    } else {
        u[0] = u0;
        u[1] = u1;
        u[2] = u2;
        u[3] = u3;
    }
    float v[4] = {v0, v1, v2, v3};

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            ImVec2 p_min(dx[x], dy[y]);
            ImVec2 p_max(dx[x + 1], dy[y + 1]);
            if (p_max.x <= p_min.x || p_max.y <= p_min.y)
                continue;

            ImVec2 uv_min(u[x], v[y]);
            ImVec2 uv_max(u[x + 1], v[y + 1]);

            draw_list->AddImage(tex_id, p_min, p_max, uv_min, uv_max, col);
        }
    }
}

void TrophyUI::Draw() {
    const auto& io = GetIO();

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
    // In 1080p -> 1.0 (exact retail match). In 1440p/4K -> crisp proportional high-DPI.
    // In smaller windowed modes (< 1080p), prevent micro-UI by smoothly scaling down with a
    // comfortable floor:
    const float scale = (base_scale < 1.0f) ? (0.60f + 0.40f * base_scale) : base_scale;

    const float card_width = 563.0f * scale;
    const float card_height = 125.0f * scale;

    const ImVec2 window_size{
        card_width,
        card_height,
    };

    elapsed_time += io.DeltaTime;
    trophy_timer -= io.DeltaTime;

    float in_progress = std::min(elapsed_time / 0.4f, 1.0f);
    float in_ease = 1.0f - std::pow(1.0f - in_progress, 3.0f);

    float out_ease = 1.0f;
    if (trophy_timer <= 0.4f) {
        float out_p = std::clamp((0.4f - static_cast<float>(trophy_timer)) / 0.4f, 0.0f, 1.0f);
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
    if (trophy_timer <= 0.4f) {
        current_pos.x = ImLerp(start_x, final_pos_x, out_ease);
        current_pos.y = ImLerp(start_y, final_pos_y, out_ease);
    } else {
        current_pos.x = ImLerp(start_x, final_pos_x, in_ease);
        current_pos.y = ImLerp(start_y, final_pos_y, in_ease);
    }

    float alpha = std::clamp(out_ease, 0.0f, 1.0f);

    SetNextWindowPos(current_pos);
    SetNextWindowSize(window_size);
    PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (Begin("##TrophyNotification", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
                  ImGuiWindowFlags_NoNav)) {

        ImDrawList* draw_list = GetWindowDrawList();
        ImVec2 p0 = GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + window_size.x, p0.y + window_size.y);

        if (notification_bg_icon) {
            auto bg_tex = notification_bg_icon.GetTexture();
            DrawNinePatch(draw_list, bg_tex.im_id, p0, p1, static_cast<float>(bg_tex.width),
                          static_cast<float>(bg_tex.height), scale,
                          IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)), is_right_side);
        } else {
            float top_y = p0.y + 4.0f * scale;
            float bot_y = p0.y + 104.0f * scale;
            float right_x = is_right_side ? p1.x : (p0.x + window_size.x - 30.0f * scale);
            float left_x = is_right_side ? (p1.x - window_size.x + 30.0f * scale) : p0.x;
            float rounding = 16.0f * scale;
            ImDrawFlags flags =
                is_right_side ? ImDrawFlags_RoundCornersLeft : ImDrawFlags_RoundCornersRight;
            draw_list->AddRectFilled(ImVec2(left_x, top_y), ImVec2(right_x, bot_y),
                                     IM_COL32(18, 18, 20, static_cast<int>(245 * alpha)), rounding,
                                     flags);
            draw_list->AddRect(ImVec2(left_x, top_y), ImVec2(right_x, bot_y),
                               IM_COL32(65, 68, 72, static_cast<int>(230 * alpha)), rounding, flags,
                               1.5f * scale);
        }

        float card_center_y = p0.y + 53.5f * scale;

        float icon_size = 64.0f * scale;
        float icon_x;
        float text_start_x;
        float text_max_w;

        // Retail Sce.Vsh.ShellUI.Trophy / Notification layout_handover_msg_panel:
        // Inner x = 102, Icon x = 0 (abs x = 102), Text x = 87 (abs x = 189), PS button x = 477
        if (is_right_side) {
            icon_x = p0.x + window_size.x - 102.0f * scale - icon_size;
            text_start_x = p0.x + 120.0f * scale;
            text_max_w = (icon_x - 16.0f * scale) - text_start_x;
        } else {
            icon_x = p0.x + 102.0f * scale;
            text_start_x = p0.x + 189.0f * scale;
            text_max_w = 266.0f * scale;
        }

        float icon_y = card_center_y - icon_size * 0.5f;

        if (trophy_icon) {
            draw_list->AddImage(trophy_icon.GetTexture().im_id, ImVec2(icon_x, icon_y),
                                ImVec2(icon_x + icon_size, icon_y + icon_size), ImVec2(0.0f, 0.0f),
                                ImVec2(1.0f, 1.0f),
                                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
        } else if (trophy_type_icon) {
            draw_list->AddImage(trophy_type_icon.GetTexture().im_id, ImVec2(icon_x, icon_y),
                                ImVec2(icon_x + icon_size, icon_y + icon_size), ImVec2(0.0f, 0.0f),
                                ImVec2(1.0f, 1.0f),
                                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
        } else if (default_trophy_icon) {
            draw_list->AddImage(default_trophy_icon.GetTexture().im_id, ImVec2(icon_x, icon_y),
                                ImVec2(icon_x + icon_size, icon_y + icon_size), ImVec2(0.0f, 0.0f),
                                ImVec2(1.0f, 1.0f),
                                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
        } else {
            draw_list->AddRectFilled(
                ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_size, icon_y + icon_size),
                IM_COL32(30, 30, 35, static_cast<int>(255 * alpha)), 4.0f * scale);
            draw_list->AddRect(ImVec2(icon_x, icon_y),
                               ImVec2(icon_x + icon_size, icon_y + icon_size),
                               IM_COL32(80, 80, 90, static_cast<int>(180 * alpha)), 4.0f * scale);
        }

        const char* header_str = trophy_title.c_str();

        ImFont* title_font = ImGui::FontStack::GetTrophyTitleFont();
        ImFont* desc_font = ImGui::FontStack::GetTrophyDescFont();
        if (!title_font)
            title_font = ImGui::GetFont();
        if (!desc_font)
            desc_font = ImGui::GetFont();

        float title_font_size = 32.0f * scale;
        float desc_font_size = 24.0f * scale;

        std::string display_name = trophy_name;
        if (desc_font->CalcTextSizeA(desc_font_size, FLT_MAX, 0.0f, display_name.c_str()).x >
            text_max_w) {
            while (!display_name.empty() && desc_font
                                                    ->CalcTextSizeA(desc_font_size, FLT_MAX, 0.0f,
                                                                    (display_name + "...").c_str())
                                                    .x > text_max_w) {
                display_name.pop_back();
            }
            display_name += "...";
        }

        ImU32 header_col = is_white_bg ? IM_COL32(25, 25, 30, static_cast<int>(255 * alpha))
                                       : IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));
        ImU32 sub_col = is_white_bg ? IM_COL32(85, 90, 100, static_cast<int>(235 * alpha))
                                    : IM_COL32(215, 220, 230, static_cast<int>(245 * alpha));

        const bool is_single_line = trophy_title.empty() || trophy_name.empty();
        if (is_single_line) {
            const std::string& single_text = trophy_title.empty() ? trophy_name : trophy_title;
            float font_size = 32.0f * scale;
            float single_y = card_center_y - font_size * 0.5f;

            std::string truncated = single_text;
            if (desc_font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, truncated.c_str()).x >
                text_max_w) {
                while (
                    !truncated.empty() &&
                    desc_font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, (truncated + "...").c_str())
                            .x > text_max_w) {
                    truncated.pop_back();
                }
                truncated += "...";
            }

            draw_list->AddText(desc_font, font_size, ImVec2(text_start_x, single_y), header_col,
                               truncated.c_str());
        } else {
            float line1_y = p0.y + 22.0f * scale;
            float line2_y = p0.y + 61.0f * scale;

            std::string truncated_title = trophy_title;
            if (title_font->CalcTextSizeA(title_font_size, FLT_MAX, 0.0f, truncated_title.c_str())
                    .x > text_max_w) {
                while (!truncated_title.empty() &&
                       title_font
                               ->CalcTextSizeA(title_font_size, FLT_MAX, 0.0f,
                                               (truncated_title + "...").c_str())
                               .x > text_max_w) {
                    truncated_title.pop_back();
                }
                truncated_title += "...";
            }

            draw_list->AddText(title_font, title_font_size, ImVec2(text_start_x, line1_y),
                               header_col, truncated_title.c_str());

            float text_name_x = text_start_x;
            if (trophy_type_icon) {
                float small_cup_size = 24.0f * scale;
                float small_cup_gap = 8.0f * scale;
                float cup_y = p0.y + 58.0f * scale;
                draw_list->AddImage(trophy_type_icon.GetTexture().im_id,
                                    ImVec2(text_start_x, cup_y),
                                    ImVec2(text_start_x + small_cup_size, cup_y + small_cup_size),
                                    ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                    IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
                text_name_x += small_cup_size + small_cup_gap;
            }

            draw_list->AddText(desc_font, desc_font_size, ImVec2(text_name_x, line2_y), sub_col,
                               display_name.c_str());
        }

        if (true) {
            float ps_center_x =
                is_right_side ? (p0.x + (563.0f - 478.0f) * scale) : (p0.x + 478.0f * scale);
            float ps_size = 48.0f * scale;
            float ps_x = ps_center_x - ps_size * 0.5f;
            float ps_y = card_center_y - ps_size * 0.5f;

            float flash_start = 0.15f;
            float flash_end = 0.85f;
            if (elapsed_time >= flash_start && elapsed_time <= flash_end) {
                float ft = (elapsed_time - flash_start) / (flash_end - flash_start);

                float appear_scale = 0.85f + 0.50f * std::sin(ft * 1.57079f);
                float appear_size = 72.0f * scale * appear_scale;
                float appear_x = ps_center_x - appear_size * 0.5f;
                float appear_y = card_center_y - appear_size * 0.5f;

                float flash_alpha = (ft < 0.25f) ? (ft / 0.25f) : (1.0f - (ft - 0.25f) / 0.75f);
                flash_alpha = std::clamp(flash_alpha, 0.0f, 1.0f);
                float flash_curve = flash_alpha * (2.0f - flash_alpha);
                int appear_col_a = static_cast<int>(255.0f * flash_curve * alpha);

                if (appear_col_a > 0) {
                    if (ps_button_appear_icon) {
                        draw_list->AddImage(ps_button_appear_icon.GetTexture().im_id,
                                            ImVec2(appear_x, appear_y),
                                            ImVec2(appear_x + appear_size, appear_y + appear_size),
                                            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                            IM_COL32(255, 255, 255, appear_col_a));
                    } else {
                        draw_list->AddCircleFilled(
                            ImVec2(ps_center_x, card_center_y), appear_size * 0.5f,
                            IM_COL32(0, 150, 255, static_cast<int>(140 * flash_curve * alpha)));
                    }
                }
            }

            if (ps_button_icon) {
                draw_list->AddImage(ps_button_icon.GetTexture().im_id, ImVec2(ps_x, ps_y),
                                    ImVec2(ps_x + ps_size, ps_y + ps_size), ImVec2(0.0f, 0.0f),
                                    ImVec2(1.0f, 1.0f),
                                    IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
            } else {
                draw_list->AddCircleFilled(ImVec2(ps_center_x, card_center_y), 15.0f * scale,
                                           IM_COL32(35, 35, 40, static_cast<int>(240 * alpha)));
                draw_list->AddCircle(ImVec2(ps_center_x, card_center_y), 15.0f * scale,
                                     IM_COL32(200, 200, 210, static_cast<int>(220 * alpha)), 0,
                                     1.5f * scale);
            }
        }
    }
    End();

    PopStyleColor(2);
    PopStyleVar(4);

    if (trophy_timer <= 0) {
        std::lock_guard<std::mutex> lock(queueMtx);
        if (!trophy_queue.empty()) {
            TrophyInfo next_trophy = trophy_queue.front();
            trophy_queue.pop();
            current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.title,
                                      next_trophy.trophy_name, next_trophy.trophy_type);
        } else {
            current_trophy_ui.reset();
        }
    }
}

void TrophyUI::PlayMp3(std::span<const u8> mp3Data) {
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    std::vector<short> pcm(MINIMP3_MAX_SAMPLES_PER_FRAME);
    mp3dec_init(&mp3d);

    // always s16 when decoded by minimp3, channels/frequency changed later on as necessary
    SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, 44100};
    bool specInfoSet = false;

    stream = SDL_CreateAudioStream(&spec, &spec);
    SDL_BindAudioStream(audioDevice, stream);

    // make this louder than game stream
    SDL_SetAudioStreamGain(stream,
                           static_cast<float>(EmulatorSettings.GetVolumeSlider() * 0.01f * 1.2f));
    const unsigned char* buffer_ptr = mp3Data.data();
    size_t remaining_size = mp3Data.size();

    while (remaining_size > 0) {
        int samples = mp3dec_decode_frame(&mp3d, buffer_ptr, remaining_size, pcm.data(), &info);
        if (samples > 0) {
            if (!specInfoSet && info.hz > 0 && info.channels > 0) {
                spec = {SDL_AUDIO_S16, info.channels, info.hz};
                SDL_SetAudioStreamFormat(stream, &spec, &spec);
                specInfoSet = true;
            }

            SDL_PutAudioStreamData(stream, pcm.data(), samples * 2 * sizeof(short));
            buffer_ptr += info.frame_bytes;
            remaining_size -= info.frame_bytes;
        } else {
            break;
        }
    }
}

void TrophyUI::PlayWav(std::span<const u8> wavData) {
    SDL_AudioSpec spec;
    Uint8* audioBuf = nullptr;
    Uint32 audioLen = 0;

    SDL_IOStream* io = SDL_IOFromConstMem(wavData.data(), wavData.size());
    if (!SDL_LoadWAV_IO(io, true, &spec, &audioBuf, &audioLen)) {
        LOG_ERROR(Lib_NpTrophy, "Unable to load trophy wave file data: {}", SDL_GetError());
        return;
    }

    stream = SDL_CreateAudioStream(&spec, &spec);
    SDL_BindAudioStream(audioDevice, stream);

    // make this louder than game stream
    SDL_SetAudioStreamGain(stream,
                           static_cast<float>(EmulatorSettings.GetVolumeSlider() * 0.01f * 1.2f));
    SDL_PutAudioStreamData(stream, audioBuf, audioLen);
    SDL_free(audioBuf);
}

static std::string_view GetTrophyEarnedText(s32 lang) {
    switch (lang) {
    case 0:
        return "トロフィーを獲得しました！";
    case 2:
        return "Trophée obtenu !";
    case 3:
        return "¡Trofeo ganado!";
    case 4:
        return "Trophäe erhalten!";
    case 5:
        return "Trofeo guadagnato!";
    case 6:
        return "Trofee gewonnen.";
    case 7:
        return "Troféu conquistado!";
    case 8:
        return "Приз получен!";
    case 9:
        return "트로피를 획득했습니다!";
    case 10:
        return "已獲得獎盃！";
    case 11:
        return "已获得奖杯！";
    case 12:
        return "Olet ansainnut trophyn!";
    case 13:
        return "Trophy vunnen!";
    case 14:
        return "Trophy er opnået!";
    case 15:
        return "Du har oppnådd et trophy!";
    case 16:
        return "Zdobyto trofeum!";
    case 17:
        return "Troféu conquistado!";
    case 19:
        return "Kupa kazanıldı!";
    case 20:
        return "¡Ganaste un trofeo!";
    case 21:
        return "تم ربح جائزة!";
    case 22:
        return "Trophée obtenu!";
    case 23:
        return "Trofej byla získána!";
    case 24:
        return "Elnyert trófea!";
    case 25:
        return "Κερδίσατε το τρόπαιο!";
    case 26:
        return "Trofeu câștigat.";
    case 27:
        return "ได้รับถ้วยรางวัล!";
    case 28:
        return "Đã giành được danh hiệu!";
    case 29:
        return "Trofi diperoleh!";
    case 30:
        return "Приз отримано!";
    default:
        return "Trophy earned!";
    }
}

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                      const std::string_view& rarity) {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (EmulatorSettings.IsTrophyPopupDisabled()) {
        return;
    }

    TrophyInfo new_trophy;
    new_trophy.trophy_icon_path = trophyIconPath;
    new_trophy.title = std::string(GetTrophyEarnedText(EmulatorSettings.GetConsoleLanguage()));
    new_trophy.trophy_name = trophyName;
    new_trophy.trophy_type = rarity;
    trophy_queue.push(new_trophy);

    if (!current_trophy_ui.has_value()) {
#ifdef ENABLE_QT_GUI
        BackgroundMusicPlayer::getInstance().stopMusic();
#endif
        TrophyInfo next_trophy = trophy_queue.front();
        trophy_queue.pop();
        current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.title,
                                  next_trophy.trophy_name, next_trophy.trophy_type);
    }
}

} // namespace Libraries::Np::NpTrophy
