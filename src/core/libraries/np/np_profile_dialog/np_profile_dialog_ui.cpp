// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <cinttypes>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui/imgui_std.h>
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "imgui/imgui_texture.h"
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
static constexpr ImVec4 COL_TEXT_GOOD = {0.45f, 0.80f, 0.50f, 1.00f};
static constexpr ImVec4 COL_TEXT_BAD = {0.95f, 0.45f, 0.40f, 1.00f};

static constexpr float CARD_WIDTH = 480.0f;
static constexpr float CARD_HEIGHT = 350.0f;
static constexpr float HEADER_HEIGHT = 44.0f;
static constexpr float FOOTER_HEIGHT = 56.0f;
static constexpr float AVATAR_RADIUS = 44.0f;
static constexpr float AVATAR_REL_X = CARD_WIDTH * 0.5f;
static constexpr float AVATAR_REL_Y = HEADER_HEIGHT + AVATAR_RADIUS + 16.0f;
static constexpr float FADE_SPEED = 8.0f;
static constexpr ImVec2 CLOSE_BUTTON_SIZE = {110.0f, 34.0f};
static constexpr ImVec2 ACTION_BUTTON_SIZE = {210.0f, 34.0f};

using NpHandler = Libraries::Np::NpHandler;

struct ReportItem {
    s32 bit;
    const char* label;
    const char* name;
};
static constexpr std::array<ReportItem, 4> REPORT_ITEMS = {{
    {ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ONLINE_ID, "Online ID", "onlineId"},
    {ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_NAME, "Name", "name"},
    {ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_PICTURE, "Picture", "picture"},
    {ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ABOUT_ME, "About Me", "aboutMe"},
}};

struct ProfileFetch {
    std::mutex mutex;
    NpHandler::ProfileCard card;
    std::atomic<bool> done{false};
};
struct ReportTask {
    std::atomic<int> state{0}; // 0 running, 1 sent, 2 failed
};

struct DialogRuntime {
    std::shared_ptr<ProfileFetch> fetch;
    bool loaded{false};
    NpHandler::ProfileCard card;
    ImGui::RefCountedTexture avatar;

    bool action_done{false};
    std::string message;
    bool message_bad{false};

    std::array<bool, REPORT_ITEMS.size()> report_picked{};
    std::shared_ptr<ReportTask> report;
};

static bool ProfileHasItem(const NpHandler::ProfileCard& card, s32 bit) {
    switch (bit) {
    case ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ONLINE_ID:
        return true;
    case ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_PICTURE:
        return card.custom_picture;
    default:
        return false;
    }
}

static const char* RelationLabel(const std::string& relation) {
    if (relation == "me")
        return "This is you";
    if (relation == "friend")
        return "Friend";
    if (relation == "requesting friend")
        return "Friend request sent";
    if (relation == "requested friend")
        return "Sent you a friend request";
    if (relation == "blocked")
        return "Blocked";
    if (relation == "friend of friends")
        return "Friend of a friend";
    return "";
}

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

// The avatar image, clipped to the same circle as the placeholder.
static void DrawAvatarImage(ImDrawList* dl, ImVec2 center, float radius, float alpha,
                            ImTextureID texture) {
    dl->AddCircleFilled(center, radius, AlphaScale(COL_AVATAR_BG, alpha), 64);
    dl->AddImageRounded(texture, {center.x - radius, center.y - radius},
                        {center.x + radius, center.y + radius}, {0.0f, 0.0f}, {1.0f, 1.0f},
                        IM_COL32(255, 255, 255, (int)(alpha * 255.0f)), radius);
    dl->AddCircle(center, radius, AlphaScale(COL_AVATAR_OUTLINE, alpha), 64, 2.5f);
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
      open_alpha(other.open_alpha), rt(std::move(other.rt)) {
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
    swap(rt, other.rt);
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

    if (!rt) {
        rt = std::make_unique<DialogRuntime>();
        std::string target = state->onlineId;
        if (state->hasAccountId) {
            target = std::to_string(state->accountId);
        }
        auto fetch = std::make_shared<ProfileFetch>();
        rt->fetch = fetch;
        const s32 user_id = state->userId;
        std::thread([fetch, user_id, target = std::move(target)] {
            NpHandler::ProfileCard card =
                NpHandler::GetInstance().FetchProfileCard(user_id, target);
            {
                std::scoped_lock lk{fetch->mutex};
                fetch->card = std::move(card);
            }
            fetch->done = true;
        }).detach();
    }
    if (rt->fetch && rt->fetch->done) {
        {
            std::scoped_lock lk{rt->fetch->mutex};
            rt->card = std::move(rt->fetch->card);
        }
        rt->fetch.reset();
        rt->loaded = true;
        if (!rt->card.avatar_png.empty()) {
            rt->avatar = ImGui::RefCountedTexture::DecodePngTexture(std::move(rt->card.avatar_png));
        }
        if (!rt->card.ok) {
            rt->message = "This player's profile could not be loaded.";
            rt->message_bad = true;
        } else if (state->mode ==
                   OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT) {
            int available = 0;
            for (size_t i = 0; i < REPORT_ITEMS.size(); ++i) {
                available += (state->griefReportItems & REPORT_ITEMS[i].bit) &&
                             ProfileHasItem(rt->card, REPORT_ITEMS[i].bit);
            }
            if (available == 0) {
                rt->message = "There is nothing in this player's profile to report.";
                rt->message_bad = true;
            } else if (available == 1) {
                for (size_t i = 0; i < REPORT_ITEMS.size(); ++i) {
                    rt->report_picked[i] = (state->griefReportItems & REPORT_ITEMS[i].bit) &&
                                           ProfileHasItem(rt->card, REPORT_ITEMS[i].bit);
                }
            }
        }
    }
    if (rt->report && rt->report->state != 0 && !rt->action_done) {
        rt->action_done = true;
        const bool sent = rt->report->state == 1;
        rt->message = sent ? "Report sent. Thank you." : "The report could not be sent.";
        rt->message_bad = !sent;
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
        if (rt->avatar) {
            DrawAvatarImage(dl, avatar_screen, AVATAR_RADIUS, open_alpha,
                            rt->avatar.GetTexture().im_id);
        } else {
            DrawAvatarPlaceholder(dl, avatar_screen, AVATAR_RADIUS, open_alpha);
        }

        // Header title
        const char* title = GetModeTitle(state->mode);
        const ImVec2 title_sz = CalcTextSize(title);
        SetCursorPos({(CARD_WIDTH - title_sz.x) * 0.5f, (HEADER_HEIGHT - title_sz.y) * 0.5f});
        TextUnformatted(title);

        // Online ID / Account ID
        char id_buf[32]{};
        const char* id_str = state->onlineId.c_str();
        if (rt->card.ok) {
            id_str = rt->card.online_id.c_str(); // the name, once the profile has loaded
        } else if (state->hasAccountId) {
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

        DrawBody(id_y + id_sz.y + sub_sz.y + 16.0f);
        if (status != nullptr) {
            DrawFooter();
        }
    }
    End();

    PopStyleVar(4);
    PopStyleColor(6);

    first_render = false;
}

void NpProfileDialogUi::DrawBody(float top) {
    const auto centered = [&](const char* text, float y, ImVec4 color) {
        PushStyleColor(ImGuiCol_Text, AlphaScale(color, open_alpha));
        const ImVec2 sz = CalcTextSize(text);
        SetCursorPos({(CARD_WIDTH - sz.x) * 0.5f, y});
        TextUnformatted(text);
        PopStyleColor();
    };
    const float message_y = CARD_HEIGHT - FOOTER_HEIGHT - 26.0f;

    if (!rt->loaded) {
        centered("Loading profile...", top, COL_TEXT_SECONDARY);
        return;
    }

    if (rt->card.ok &&
        state->mode != OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT) {
        if (const char* rel = RelationLabel(rt->card.relation); *rel != '\0') {
            centered(rel, top, COL_TEXT_SECONDARY);
        }
    }

    if (rt->card.ok &&
        state->mode == OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT &&
        !rt->action_done && !rt->report) {
        int shown = 0;
        for (size_t i = 0; i < REPORT_ITEMS.size(); ++i) {
            const ReportItem& item = REPORT_ITEMS[i];
            if (!(state->griefReportItems & item.bit) || !ProfileHasItem(rt->card, item.bit)) {
                continue;
            }
            const float col_x = shown % 2 == 0 ? 96.0f : CARD_WIDTH * 0.5f + 24.0f;
            SetCursorPos({col_x, top + (float)(shown / 2) * 28.0f});
            Checkbox(item.label, &rt->report_picked[i]);
            ++shown;
        }
    }

    if (!rt->message.empty()) {
        centered(rt->message.c_str(), message_y, rt->message_bad ? COL_TEXT_BAD : COL_TEXT_GOOD);
    }
}

void NpProfileDialogUi::DrawFooter() {
    const float footer_y_local = CARD_HEIGHT - FOOTER_HEIGHT;
    const float button_y = footer_y_local + (FOOTER_HEIGHT - CLOSE_BUTTON_SIZE.y) * 0.5f;

    // The action for this mode, if there is one to take.
    const char* action = nullptr;
    bool action_enabled = false;
    const auto& card = rt->card;
    if (rt->loaded && card.ok && !rt->action_done) {
        switch (state->mode) {
        case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST:
            if (card.relation == "requested friend") {
                action = "Accept Friend Request";
                action_enabled = true;
            } else if (card.relation == "me" || card.relation == "friend" ||
                       card.relation == "requesting friend" || card.relation == "blocked") {
                if (rt->message.empty()) {
                    rt->message = card.relation == "me"        ? "You cannot add yourself."
                                  : card.relation == "friend"  ? "You are already friends."
                                  : card.relation == "blocked" ? "Unblock this player first."
                                                               : "Your friend request is pending.";
                    rt->message_bad = false;
                }
            } else {
                action = "Send Friend Request";
                action_enabled = true;
            }
            break;
        case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST:
            if (card.relation == "me" || card.relation == "blocked") {
                if (rt->message.empty()) {
                    rt->message = card.relation == "me" ? "You cannot block yourself."
                                                        : "This player is already blocked.";
                    rt->message_bad = false;
                }
            } else {
                action = "Block";
                action_enabled = true;
                if (rt->message.empty()) {
                    rt->message = "They will no longer be your friend or able to contact you.";
                    rt->message_bad = false;
                }
            }
            break;
        case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT: {
            if (!rt->report && !rt->message_bad) {
                action = "Send Report";
                for (const bool picked : rt->report_picked) {
                    action_enabled |= picked;
                }
            }
            break;
        }
        default:
            break;
        }
    }

    PushStyleColor(ImGuiCol_Border, AlphaScale(COL_BUTTON_BORDER, open_alpha));
    PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    if (action != nullptr) {
        SetCursorPos({16.0f, button_y});
        BeginDisabled(!action_enabled);
        if (Button(action, ACTION_BUTTON_SIZE)) {
            const s32 user_id = state->userId;
            auto& np = NpHandler::GetInstance();
            switch (state->mode) {
            case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST: {
                const bool accepting = card.relation == "requested friend";
                const s32 rc = np.SendFriendRequest(user_id, card.online_id);
                rt->action_done = true;
                rt->message_bad = rc != ORBIS_OK;
                rt->message = rc != ORBIS_OK ? "You are not signed in to shadNet."
                              : accepting    ? "You are now friends."
                                             : "Friend request sent.";
                break;
            }
            case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST: {
                const s32 rc = np.BlockUser(user_id, card.online_id);
                rt->action_done = true;
                rt->message_bad = rc != ORBIS_OK;
                rt->message =
                    rc != ORBIS_OK ? "You are not signed in to shadNet." : "Player blocked.";
                break;
            }
            case OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT: {
                std::vector<std::string> items;
                for (size_t i = 0; i < REPORT_ITEMS.size(); ++i) {
                    if (rt->report_picked[i]) {
                        items.emplace_back(REPORT_ITEMS[i].name);
                    }
                }
                auto task = std::make_shared<ReportTask>();
                rt->report = task;
                rt->message = "Sending report...";
                rt->message_bad = false;
                std::thread([task, user_id, account_id = card.account_id,
                             items = std::move(items)] {
                    task->state =
                        NpHandler::GetInstance().SendGriefReport(user_id, account_id, items) ? 1
                                                                                             : 2;
                }).detach();
                break;
            }
            default:
                break;
            }
        }
        EndDisabled();
    }

    SetCursorPos({CARD_WIDTH - CLOSE_BUTTON_SIZE.x - 16.0f, button_y});
    if (Button("Close##btn", CLOSE_BUTTON_SIZE)) {
        Finish(Result::USER_CANCELED);
    }
    if (first_render) {
        SetItemCurrentNavFocus();
    }
    PopStyleVar();
    PopStyleColor();
}

} // namespace Libraries::Np::NpProfileDialog
