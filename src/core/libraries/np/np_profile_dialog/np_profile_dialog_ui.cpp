// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <cmath>
#include <thread>
#include <utility>

#include <imgui.h>
#include <imgui/imgui_std.h>
#include "np_profile_dialog_ui.h"

using namespace ImGui;
using namespace Libraries::CommonDialog;

namespace Libraries::Np::NpProfileDialog {

// PS4 color palette
static constexpr ImVec4 COL_OVERLAY = {0.00f, 0.00f, 0.00f, 0.65f};
static constexpr ImVec4 COL_HEADER_TOP = {0.00f, 0.34f, 0.62f, 1.00f};
static constexpr ImVec4 COL_HEADER_BOT = {0.00f, 0.22f, 0.42f, 1.00f};
static constexpr ImVec4 COL_CARD_BG = {0.11f, 0.11f, 0.12f, 1.00f};
static constexpr ImVec4 COL_SEPARATOR = {0.22f, 0.22f, 0.24f, 1.00f};
static constexpr ImVec4 COL_AVATAR_BG = {0.18f, 0.18f, 0.20f, 1.00f};
static constexpr ImVec4 COL_AVATAR_OUTLINE = {0.00f, 0.44f, 0.75f, 1.00f};
static constexpr ImVec4 COL_TEXT_PRIMARY = {1.00f, 1.00f, 1.00f, 1.00f};
static constexpr ImVec4 COL_TEXT_SECONDARY = {0.65f, 0.65f, 0.68f, 1.00f};
static constexpr ImVec4 COL_BUTTON_NORMAL = {0.18f, 0.18f, 0.20f, 1.00f};
static constexpr ImVec4 COL_BUTTON_HOVERED = {0.00f, 0.44f, 0.75f, 1.00f};
static constexpr ImVec4 COL_BUTTON_ACTIVE = {0.00f, 0.33f, 0.60f, 1.00f};
static constexpr ImVec4 COL_BUTTON_BORDER = {0.32f, 0.32f, 0.35f, 1.00f};

static constexpr float CARD_WIDTH = 480.0f;
static constexpr float CARD_HEIGHT = 330.0f;
static constexpr float HEADER_HEIGHT = 44.0f;
static constexpr float FOOTER_HEIGHT = 56.0f;
static constexpr float AVATAR_RADIUS = 44.0f;
static constexpr float AVATAR_REL_X = CARD_WIDTH * 0.5f;
static constexpr float AVATAR_REL_Y = HEADER_HEIGHT + AVATAR_RADIUS + 16.0f;
static constexpr float FADE_SPEED = 8.0f;
static constexpr ImVec2 CLOSE_BUTTON_SIZE = {110.0f, 34.0f};

static ImU32 AlphaScale(ImVec4 col, float alpha) {
    col.w *= alpha;
    return GetColorU32(col);
}

static void DrawCardShadow(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha) {
    for (int i = 0; i < 6; ++i) {
        const float spread = (float)(i + 1) * 3.5f;
        const float a = alpha * (0.28f - (float)i * 0.04f);
        if (a <= 0.0f)
            break;
        dl->AddRectFilled({min.x - spread, min.y + spread}, {max.x + spread, max.y + spread * 1.5f},
                          IM_COL32(0, 0, 0, (int)(a * 255.0f)), 8.0f + spread);
    }
}

static void DrawAvatarPlaceholder(ImDrawList* dl, ImVec2 center, float radius, float alpha) {
    const ImU32 bg_col = AlphaScale(COL_AVATAR_BG, alpha);
    const ImU32 outline_col = AlphaScale(COL_AVATAR_OUTLINE, alpha);
    const ImU32 silhouette = IM_COL32(90, 90, 96, (int)(alpha * 255.0f));

    dl->AddCircleFilled(center, radius, bg_col, 64);

    dl->AddCircleFilled({center.x, center.y - radius * 0.18f}, radius * 0.32f, silhouette, 32);

    dl->AddCircleFilled({center.x, center.y + radius * 0.45f}, radius * 0.55f, silhouette, 48);

    // Outline drawn last — sits on top of silhouette
    dl->AddCircle(center, radius, outline_col, 64, 2.5f);
}

static const char* GetModeTitle(OrbisNpProfileDialogMode mode) {
    switch (mode) {
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL:
        return "Profile";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST:
        return "Friend Request";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST:
        return "Block List";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT:
        return "Report";
    default:
        return "Profile";
    }
}

static const char* GetModeSubtitle(OrbisNpProfileDialogMode mode) {
    switch (mode) {
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL:
        return "Player Profile";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST:
        return "Send Friend Request";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST:
        return "Add to Block List";
    case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT:
        return "Report Player";
    default:
        return "Player Profile";
    }
}

NpProfileDialogUi::NpProfileDialogUi(NpProfileDialogState* state, Status* status,
                                     OrbisNpProfileDialogResult* result)
    : state(state), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        open_alpha = 0.0f;
        AddLayer(this);
    }
}

NpProfileDialogUi::~NpProfileDialogUi() {
    Finish(Result::USER_CANCELED);
}

NpProfileDialogUi::NpProfileDialogUi(NpProfileDialogUi&& other) noexcept
    : Layer(other), state(other.state), status(other.status), result(other.result),
      open_alpha(other.open_alpha) {
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
    other.open_alpha = 0.0f;
}

NpProfileDialogUi& NpProfileDialogUi::operator=(NpProfileDialogUi other) {
    using std::swap;
    swap(state, other.state);
    swap(status, other.status);
    swap(result, other.result);
    swap(open_alpha, other.open_alpha);
    if (status && *status == Status::RUNNING) {
        first_render = true;
        open_alpha = 0.0f;
        AddLayer(this);
    }
    return *this;
}

void NpProfileDialogUi::Finish(Result user_action) {
    if (result) {
        result->result = 0; // ORBIS_OK
        result->userAction = user_action;
    }
    if (status) {
        *status = Status::FINISHED;
    }
    state = nullptr;
    status = nullptr;
    result = nullptr;
    RemoveLayer(this);
}

void NpProfileDialogUi::Draw() {
    if (status == nullptr || *status != Status::RUNNING) {
        return;
    }

    const auto& io = GetIO();
    open_alpha = std::fmin(open_alpha + io.DeltaTime * FADE_SPEED, 1.0f);

    const float card_x = (io.DisplaySize.x - CARD_WIDTH) * 0.5f;
    const float card_y = (io.DisplaySize.y - CARD_HEIGHT) * 0.5f;
    const ImVec2 card_min = {card_x, card_y};
    const ImVec2 card_max = {card_x + CARD_WIDTH, card_y + CARD_HEIGHT};

    SetNextWindowPos({0, 0});
    SetNextWindowSize(io.DisplaySize);
    PushStyleColor(ImGuiCol_WindowBg, AlphaScale(COL_OVERLAY, open_alpha));
    PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    Begin("##NpProfileDialogOverlay", nullptr,
          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
              ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing);
    End();
    PopStyleVar();
    PopStyleColor();

    SetNextWindowPos(card_min);
    SetNextWindowSize({CARD_WIDTH, CARD_HEIGHT});
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    KeepNavHighlight();

    PushStyleColor(ImGuiCol_WindowBg, AlphaScale(COL_CARD_BG, open_alpha));
    PushStyleColor(ImGuiCol_Border, AlphaScale(COL_SEPARATOR, open_alpha));
    PushStyleColor(ImGuiCol_Text, AlphaScale(COL_TEXT_PRIMARY, open_alpha));
    PushStyleColor(ImGuiCol_Button, AlphaScale(COL_BUTTON_NORMAL, open_alpha));
    PushStyleColor(ImGuiCol_ButtonHovered, AlphaScale(COL_BUTTON_HOVERED, open_alpha));
    PushStyleColor(ImGuiCol_ButtonActive, AlphaScale(COL_BUTTON_ACTIVE, open_alpha));
    PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (Begin("##NpProfileDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoMove)) {

        ImDrawList* dl = GetWindowDrawList();
        const ImVec2 win_pos = GetWindowPos();

        DrawCardShadow(dl, card_min, card_max, open_alpha);

        const ImVec2 hdr_min = win_pos;
        const ImVec2 hdr_max = {win_pos.x + CARD_WIDTH, win_pos.y + HEADER_HEIGHT};
        dl->AddRectFilledMultiColor(hdr_min, hdr_max, AlphaScale(COL_HEADER_TOP, open_alpha),
                                    AlphaScale(COL_HEADER_TOP, open_alpha),
                                    AlphaScale(COL_HEADER_BOT, open_alpha),
                                    AlphaScale(COL_HEADER_BOT, open_alpha));
        dl->AddRectFilled({hdr_min.x, hdr_max.y - 6.0f}, hdr_max,
                          AlphaScale(COL_HEADER_BOT, open_alpha));

        dl->AddLine({hdr_min.x + 6.0f, hdr_min.y + 1.0f}, {hdr_max.x - 6.0f, hdr_min.y + 1.0f},
                    IM_COL32(255, 255, 255, (int)(open_alpha * 40.0f)), 1.0f);

        // Footer separator
        const float footer_top_screen = win_pos.y + CARD_HEIGHT - FOOTER_HEIGHT;
        dl->AddLine({win_pos.x, footer_top_screen}, {win_pos.x + CARD_WIDTH, footer_top_screen},
                    AlphaScale(COL_SEPARATOR, open_alpha), 1.0f);

        // Avatar
        const ImVec2 avatar_screen = {win_pos.x + AVATAR_REL_X, win_pos.y + AVATAR_REL_Y};
        DrawAvatarPlaceholder(dl, avatar_screen, AVATAR_RADIUS, open_alpha);

        // Header title
        const char* title = GetModeTitle(state->mode);
        const ImVec2 title_sz = CalcTextSize(title);
        SetCursorPos({(CARD_WIDTH - title_sz.x) * 0.5f, (HEADER_HEIGHT - title_sz.y) * 0.5f});
        TextUnformatted(title);

        // Online ID / Account ID
        char id_buf[32]{};
        const char* id_str = state->onlineId.c_str();
        if (state->hasAccountId) {
            snprintf(id_buf, sizeof(id_buf), "%" PRIu64, state->accountId);
            id_str = id_buf;
        }
        const float id_y = AVATAR_REL_Y + AVATAR_RADIUS + 14.0f;
        const ImVec2 id_sz = CalcTextSize(id_str);
        SetCursorPos({(CARD_WIDTH - id_sz.x) * 0.5f, id_y});
        TextUnformatted(id_str);

        // Mode subtitle
        PushStyleColor(ImGuiCol_Text, AlphaScale(COL_TEXT_SECONDARY, open_alpha));
        const char* subtitle = GetModeSubtitle(state->mode);
        const ImVec2 sub_sz = CalcTextSize(subtitle);
        SetCursorPos({(CARD_WIDTH - sub_sz.x) * 0.5f, id_y + id_sz.y + 5.0f});
        TextUnformatted(subtitle);
        PopStyleColor();

        const float footer_y_local = CARD_HEIGHT - FOOTER_HEIGHT;

        PushStyleColor(ImGuiCol_Border, AlphaScale(COL_BUTTON_BORDER, open_alpha));
        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        SetCursorPos({CARD_WIDTH - CLOSE_BUTTON_SIZE.x - 16.0f,
                      footer_y_local + (FOOTER_HEIGHT - CLOSE_BUTTON_SIZE.y) * 0.5f});
        if (Button("Close##btn", CLOSE_BUTTON_SIZE)) {
            Finish(Result::USER_CANCELED);
        }
        PopStyleVar();
        PopStyleColor();

        if (first_render) {
            SetItemCurrentNavFocus();
        }
    }
    End();

    PopStyleVar(4);
    PopStyleColor(6);

    first_render = false;
}

} // namespace Libraries::Np::NpProfileDialog
