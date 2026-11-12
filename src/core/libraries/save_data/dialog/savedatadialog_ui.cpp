// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <fmt/chrono.h>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/elf_info.h"
#include "common/io_file.h"
#include "common/singleton.h"
#include "common/string_util.h"
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "core/libraries/save_data/save_db.h"
#include "core/libraries/save_data/save_instance.h"
#include "imgui/imgui_std.h"
#include "savedatadialog_ui.h"

using namespace ImGui;
using namespace Libraries::CommonDialog;
using Common::ElfInfo;

constexpr auto SAVE_ICON_SIZE = ImVec2{152.0f, 85.0f};
constexpr auto SAVE_ICON_PADDING = ImVec2{8.0f, 2.0f};

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};
constexpr auto FOOTER_HEIGHT = BUTTON_SIZE.y + 15.0f;
static constexpr float PROGRESS_BAR_WIDTH{0.8f};

static ::Core::FileSys::MntPoints* g_mnt =
    Common::Singleton<::Core::FileSys::MntPoints>::Instance();

static std::string SpaceSizeToString(size_t size) {
    std::string size_str;
    if (size > 1024 * 1024 * 1024) { // > 1GB
        size_str = fmt::format("{:.2f} GB", double(size / 1024 / 1024) / 1024.0f);
    } else if (size > 1024 * 1024) { // > 1MB
        size_str = fmt::format("{:.2f} MB", double(size / 1024) / 1024.0f);
    } else if (size > 1024) { // > 1KB
        size_str = fmt::format("{:.2f} KB", double(size) / 1024.0f);
    } else {
        size_str = fmt::format("{} B", size);
    }
    return size_str;
}

namespace Libraries::SaveData::Dialog {

static u32 GetFirmwareVerSafe() {
    return ElfInfo::Instance().IsInitialized() ? ElfInfo::Instance().FirmwareVer()
                                               : ElfInfo::FW_1150;
}

static std::string GetDefaultTitleIdSafe() {
    std::string tid = "CUSA00000";
    if (ElfInfo::Instance().IsInitialized()) {
        tid = ElfInfo::Instance().GameSerial();
    }
    if (auto* psf = Common::Singleton<PSF>::Instance()) {
        if (auto install_dir = psf->GetString("INSTALL_DIR_SAVEDATA");
            install_dir && !install_dir->empty()) {
            tid = std::string(*install_dir);
        }
    }
    return tid;
}

static std::string FixedCharToString(const char* buf, size_t cap) {
    return std::string(buf, strnlen(buf, cap));
}

void SaveDialogResult::CopyTo(OrbisSaveDataDialogResult& result) const {
    result.mode = this->mode;
    result.result = this->result;
    result.buttonId = this->button_id;
    if (result.dirName != nullptr) {
        std::memset(result.dirName, 0, sizeof(OrbisSaveDataDirName));
        std::memcpy(result.dirName->data, this->dir_name.data(),
                    std::min(this->dir_name.size(), sizeof(result.dirName->data) - 1));
    }
    if (result.param != nullptr && this->param.GetString("MAINTITLE").has_value()) {
        std::memset(result.param, 0, sizeof(OrbisSaveDataParam));
        TruncateUtf8(*this->param.GetString("MAINTITLE"), result.param->title,
                     sizeof(result.param->title));
        if (auto v = this->param.GetString("SUBTITLE")) {
            TruncateUtf8(*v, result.param->subTitle, sizeof(result.param->subTitle));
        }
        if (auto v = this->param.GetString("DETAIL")) {
            TruncateUtf8(*v, result.param->detail, sizeof(result.param->detail));
        }
        if (auto v = this->param.GetInteger("SAVEDATA_LIST_PARAM")) {
            result.param->userParam = static_cast<u32>(*v);
        }
        if (auto v = this->param.GetString("MTIME")) {
            result.param->mtime = Iso8601ToUtcTime(*v);
        }
    }
    result.userData = this->user_data;
}

SaveDialogState::SaveDialogState(const OrbisSaveDataDialogParam& param) {
    this->mode = param.mode;
    this->type = param.dispType;
    this->user_data = param.userData;
    if (param.optionParam != nullptr) {
        this->enable_back = {param.optionParam->back == OrbisSaveDataDialogOptionBack::ENABLE};
    }

    const std::string game_serial = GetDefaultTitleIdSafe();
    const auto item = param.items;
    if (item == nullptr) {
        this->title_id = game_serial;
    } else {
        this->user_id = item->userId;
        if (item->titleId == nullptr) {
            this->title_id = game_serial;
        } else {
            this->title_id = FixedCharToString(item->titleId->data, sizeof(item->titleId->data));
        }

        if (item->dirName != nullptr) {
            for (u32 i = 0; i < item->dirNameNum; i++) {
                const std::string dir_name =
                    FixedCharToString(item->dirName[i].data, sizeof(item->dirName[i].data));
                if (dir_name.empty()) {
                    continue;
                }

                const auto dir_path = GetSaveDirPath(user_id, title_id, dir_name);
                const auto param_sfo_path = dir_path / "sce_sys" / "param.sfo";

                auto db_entry_opt = SaveDb::GetEntry(user_id, title_id, dir_name);
                const bool has_db = db_entry_opt.has_value();
                const SaveDbEntry db_entry = db_entry_opt.value_or(SaveDbEntry{});
                const bool has_sfo = std::filesystem::exists(param_sfo_path);
                if (!has_sfo && !has_db) {
                    continue;
                }

                PSF param_sfo;
                const bool sfo_ok = has_sfo && param_sfo.Open(param_sfo_path);
                const bool is_corrupted = (has_db && db_entry.is_broken) || !sfo_ok;

                if (!sfo_ok && has_db) {
                    param_sfo.AddString("MAINTITLE", db_entry.main_title, true);
                    param_sfo.AddString("SUBTITLE", db_entry.sub_title, true);
                    param_sfo.AddString("DETAIL", db_entry.detail, true);
                    param_sfo.AddInteger("SAVEDATA_LIST_PARAM",
                                         static_cast<s32>(db_entry.user_param), true);
                    if (db_entry.mtime != 0) {
                        param_sfo.AddString("MTIME", UtcTimeToIso8601(db_entry.mtime), true);
                    }
                }

                const auto last_write_tp = param_sfo.GetLastWrite();
                const auto last_write = std::chrono::system_clock::to_time_t(last_write_tp);
                std::string date_str;
                if (const auto* tm = std::localtime(&last_write)) {
                    date_str = fmt::format("{:%d %b, %Y %R}", *tm);
                }

                const size_t size =
                    std::filesystem::exists(dir_path)
                        ? Common::FS::GetDirectorySize(dir_path)
                        : static_cast<size_t>(db_entry.blocks * OrbisSaveDataBlockSize);
                const std::string size_str = SpaceSizeToString(size);

                const auto meta_icon = GetMetaIconPath(user_id, title_id, dir_name);
                const auto save_icon = dir_path / "sce_sys" / "icon0.png";
                const auto& icon_path = std::filesystem::exists(meta_icon) ? meta_icon : save_icon;
                RefCountedTexture icon;
                if (std::filesystem::exists(icon_path)) {
                    icon = RefCountedTexture::DecodePngFile(icon_path);
                }

                this->save_list.emplace_back(Item{
                    .dir_name = dir_name,
                    .icon = icon,
                    .title = std::string{param_sfo.GetString("MAINTITLE").value_or("Unknown")},
                    .subtitle = std::string{param_sfo.GetString("SUBTITLE").value_or("")},
                    .details = std::string{param_sfo.GetString("DETAIL").value_or("")},
                    .date = date_str,
                    .size = size_str,
                    .last_write = last_write_tp,
                    .pfo = param_sfo,
                    .is_corrupted = is_corrupted,
                });
            }
        }

        if (type == OrbisSaveDataDialogType::SAVE && item->newItem != nullptr) {
            RefCountedTexture icon;
            std::string title{"New Save"};

            const auto new_item = item->newItem;
            if (new_item->iconBuf && new_item->iconSize) {
                const auto* buf = static_cast<const u8*>(new_item->iconBuf);
                icon = RefCountedTexture::DecodePngTexture({buf, buf + new_item->iconSize});
            } else if (auto bytes = g_mnt->ReadFile("/app0/sce_sys/save_data.png")) {
                icon = RefCountedTexture::DecodePngTexture(std::move(*bytes));
            }
            if (new_item->title != nullptr) {
                title = std::string{new_item->title};
            }

            this->new_item = Item{
                .dir_name = "",
                .icon = icon,
                .title = title,
            };
        }

        if (item->focusPos != OrbisSaveDataDialogFocusPos::DIRNAME) {
            this->focus_pos = item->focusPos;
        } else if (item->focusPosDirName != nullptr) {
            this->focus_pos =
                FixedCharToString(item->focusPosDirName->data, sizeof(item->focusPosDirName->data));
        }
        this->style = item->itemStyle;
    }

    switch (mode) {
    case OrbisSaveDataDialogMode::USER_MSG: {
        this->state = UserState{param};
    } break;
    case OrbisSaveDataDialogMode::SYSTEM_MSG:
        this->state = SystemState{*this, param};
        break;
    case OrbisSaveDataDialogMode::ERROR_CODE: {
        this->state = ErrorCodeState{param};
    } break;
    case OrbisSaveDataDialogMode::PROGRESS_BAR: {
        this->state = ProgressBarState{*this, param};
    } break;
    default:
        break;
    }
}

SaveDialogState::UserState::UserState(const OrbisSaveDataDialogParam& param) {
    auto& user = *param.userMsgParam;
    this->type = user.buttonType;
    this->msg_type = user.msgType;
    this->msg = user.msg != nullptr ? std::string{user.msg} : std::string{};
}

SaveDialogState::SystemState::SystemState(const SaveDialogState& state,
                                          const OrbisSaveDataDialogParam& param) {
#define M(save, load, del)                                                                         \
    if (type == OrbisSaveDataDialogType::SAVE)                                                     \
        this->msg = save;                                                                          \
    else if (type == OrbisSaveDataDialogType::LOAD)                                                \
        this->msg = load;                                                                          \
    else if (type == OrbisSaveDataDialogType::DELETE)                                              \
        this->msg = del;                                                                           \
    else                                                                                           \
        this->msg = ""

    auto type = param.dispType;
    auto& sys = *param.sysMsgParam;
    switch (sys.msgType) {
    case OrbisSaveDataDialogSystemMessageType::NODATA: {
        return_cancel = true;
        this->msg = "There is no saved data";
    } break;
    case OrbisSaveDataDialogSystemMessageType::CONFIRM:
        show_no = true;
        M("Do you want to save?", "Do you want to load this saved data?",
          "Do you want to delete this saved data?");
        break;
    case OrbisSaveDataDialogSystemMessageType::OVERWRITE:
        show_no = true;
        M("Do you want to overwrite the existing saved data?", "##UNKNOWN##", "##UNKNOWN##");
        break;
    case OrbisSaveDataDialogSystemMessageType::NOSPACE:
        return_cancel = true;
        M(fmt::format(
              "There is not enough space to save the data. To continue {} free space is required.",
              SpaceSizeToString(sys.value * OrbisSaveDataBlockSize)),
          "##UNKNOWN##", "##UNKNOWN##");
        break;
    case OrbisSaveDataDialogSystemMessageType::PROGRESS:
        hide_ok = true;
        show_cancel = state.enable_back.value_or(false);
        M("Saving...", "Loading...", "Deleting...");
        break;
    case OrbisSaveDataDialogSystemMessageType::FILE_CORRUPTED:
        return_cancel = true;
        this->msg = "The saved data is corrupted.";
        break;
    case OrbisSaveDataDialogSystemMessageType::FINISHED:
        return_cancel = true;
        M("Saved successfully.", "Loading complete.", "Deletion complete.");
        break;
    case OrbisSaveDataDialogSystemMessageType::NOSPACE_CONTINUABLE:
        return_cancel = true;
        M(fmt::format("There is not enough space to save the data. {} free space is required.",
                      SpaceSizeToString(sys.value * OrbisSaveDataBlockSize)),
          "##UNKNOWN##", "##UNKNOWN##");
        break;
    case OrbisSaveDataDialogSystemMessageType::CORRUPTED_AND_DELETED: {
        show_cancel = state.enable_back.value_or(true);
        const char* msg1 = "The saved data is corrupted and will be deleted.";
        M(msg1, msg1, "##UNKNOWN##");
    } break;
    case OrbisSaveDataDialogSystemMessageType::CORRUPTED_AND_CREATED: {
        show_cancel = state.enable_back.value_or(true);
        const char* msg2 = "The saved data is corrupted. This saved data will be deleted and a new "
                           "one will be created.";
        M(msg2, msg2, "##UNKNOWN##");
    } break;
    case OrbisSaveDataDialogSystemMessageType::CORRUPTED_AND_RESTORE: {
        show_cancel = state.enable_back.value_or(true);
        const char* msg3 =
            "The saved data is corrupted. The data that was backed up by the system will be "
            "restored.";
        M(msg3, msg3, "##UNKNOWN##");
    } break;
    case OrbisSaveDataDialogSystemMessageType::TOTAL_SIZE_EXCEEDED:
        M("Cannot create more saved data", "##UNKNOWN##", "##UNKNOWN##");
        break;
    default:
        msg = fmt::format("Unknown message type: {}", magic_enum::enum_name(sys.msgType));
        break;
    }

#undef M
}

SaveDialogState::ErrorCodeState::ErrorCodeState(const OrbisSaveDataDialogParam& param) {
    auto& err = *param.errorCodeParam;
    constexpr auto NOT_FOUND = 0x809F0008;
    constexpr auto BROKEN = 0x809F000F;
    switch (err.errorCode) {
    case NOT_FOUND:
        this->error_msg = "There is not saved data.";
        break;
    case BROKEN:
        this->error_msg = "The data is corrupted.";
        break;
    default:
        this->error_msg = fmt::format("An error has occurred. ({:X})", err.errorCode);
        break;
    }
}
SaveDialogState::ProgressBarState::ProgressBarState(const SaveDialogState& state,
                                                    const OrbisSaveDataDialogParam& param) {
    this->progress = 0;

    auto& bar = *param.progressBarParam;

    if (bar.msg != nullptr) {
        this->msg = std::string{bar.msg};
    } else {
        switch (bar.sysMsgType) {
        case OrbisSaveDataDialogProgressSystemMessageType::INVALID:
            this->msg = "";
            break;
        case OrbisSaveDataDialogProgressSystemMessageType::PROGRESS:
            switch (state.type) {
            case OrbisSaveDataDialogType::SAVE:
                this->msg = "Saving...";
                break;
            case OrbisSaveDataDialogType::LOAD:
                this->msg = "Loading...";
                break;
            case OrbisSaveDataDialogType::DELETE:
                this->msg = "Deleting...";
                break;
            }
            break;
        case OrbisSaveDataDialogProgressSystemMessageType::RESTORE:
            this->msg = "Restoring saved data...";
            break;
        }
    }
}

SaveDialogUi::SaveDialogUi(SaveDialogState* state, Status* status, SaveDialogResult* result)
    : state(state), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

SaveDialogUi::~SaveDialogUi() {
    Finish(OrbisSaveDataDialogButtonId::INVALID);
}

SaveDialogUi::SaveDialogUi(SaveDialogUi&& other) noexcept
    : Layer(other), state(other.state), status(other.status), result(other.result) {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

SaveDialogUi& SaveDialogUi::operator=(SaveDialogUi&& other) noexcept {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    using std::swap;
    state = other.state;
    other.state = nullptr;
    status = other.status;
    other.status = nullptr;
    result = other.result;
    other.result = nullptr;
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
    return *this;
}

void SaveDialogUi::Finish(OrbisSaveDataDialogButtonId buttonId, Result r) {
    std::unique_lock lock(draw_mutex);
    if (result && state) {
        result->mode = this->state->mode;
        result->result = r;
        result->button_id = buttonId;
        result->user_data = this->state->user_data;
        if (state->mode != OrbisSaveDataDialogMode::LIST && !state->save_list.empty()) {
            result->dir_name = state->save_list.front().dir_name;
        }
    }
    if (status) {
        *status = Status::FINISHED;
    }
    RemoveLayer(this);
}

void SaveDialogUi::Draw() {
    std::unique_lock lock{draw_mutex};

    if (status == nullptr || *status != Status::RUNNING || state == nullptr) {
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;

    ImVec2 window_size;

    if (state->GetMode() == OrbisSaveDataDialogMode::LIST) {
        window_size = ImVec2{
            std::min(io.DisplaySize.x - 200.0f, 1100.0f),
            std::min(io.DisplaySize.y - 100.0f, 700.0f),
        };
    } else {
        window_size = ImVec2{
            std::min(io.DisplaySize.x, 600.0f),
            std::min(io.DisplaySize.y, 300.0f),
        };
    }

    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    if (Begin("Save Data Dialog##SaveDataDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();

        Separator();
        // Draw title bigger
        SetWindowFontScale(1.7f);
        switch (state->type) {
        case OrbisSaveDataDialogType::SAVE:
            TextUnformatted("Save");
            break;
        case OrbisSaveDataDialogType::LOAD:
            TextUnformatted("Load");
            break;
        case OrbisSaveDataDialogType::DELETE:
            TextUnformatted("Delete");
            break;
        }
        SetWindowFontScale(1.0f);
        Separator();

        BeginGroup();
        switch (state->GetMode()) {
        case OrbisSaveDataDialogMode::LIST:
            DrawList();
            break;
        case OrbisSaveDataDialogMode::USER_MSG:
            DrawUser();
            break;
        case OrbisSaveDataDialogMode::SYSTEM_MSG:
            DrawSystemMessage();
            break;
        case OrbisSaveDataDialogMode::ERROR_CODE:
            DrawErrorCode();
            break;
        case OrbisSaveDataDialogMode::PROGRESS_BAR:
            DrawProgressBar();
            break;
        default:
            TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "!!!Unknown dialog mode!!!");
        }
        EndGroup();
    }
    End();

    first_render = false;
    if (*status == Status::FINISHED) {
        if (state) {
            *state = SaveDialogState{};
        }
        state = nullptr;
        status = nullptr;
        result = nullptr;
    }
}

void SaveDialogUi::DrawItem(int _id, const SaveDialogState::Item& item, bool clickable) {
    constexpr auto text_spacing = 0.95f;

    auto& ctx = *GetCurrentContext();
    auto& window = *ctx.CurrentWindow;

    auto content_region_avail = GetContentRegionAvail();
    const auto outer_pos = window.DC.CursorPos;
    const auto pos = outer_pos + SAVE_ICON_PADDING;

    const ImVec2 size = {content_region_avail.x - SAVE_ICON_PADDING.x,
                         SAVE_ICON_SIZE.y + SAVE_ICON_PADDING.y};
    const ImRect bb{outer_pos, outer_pos + size + SAVE_ICON_PADDING};

    const ImGuiID id = GetID(_id);

    ItemSize(size + ImVec2{0.0f, SAVE_ICON_PADDING.y * 2.0f});
    if (!ItemAdd(bb, id)) {
        return;
    }

    window.DrawList->AddRectFilled(bb.Min + SAVE_ICON_PADDING, bb.Max - SAVE_ICON_PADDING,
                                   GetColorU32(ImVec4{0.3f}));

    bool hovered = false;
    if (clickable) {
        bool held;
        bool pressed = ButtonBehavior(bb, id, &hovered, &held);
        if (pressed) {
            result->dir_name = item.dir_name;
            result->param = item.pfo;
            Finish(OrbisSaveDataDialogButtonId::INVALID);
        }
        RenderNavHighlight(bb, id);
    }

    if (item.icon) {
        auto texture = item.icon.GetTexture();
        window.DrawList->AddImage(texture.im_id, pos, pos + SAVE_ICON_SIZE);
    } else {
        // placeholder
        window.DrawList->AddRectFilled(pos, pos + SAVE_ICON_SIZE, GetColorU32(ImVec4{0.7f}));
    }

    auto pos_x = SAVE_ICON_SIZE.x + 5.0f;
    auto pos_y = 2.0f;

    if (!item.title.empty()) {
        const char* begin = &item.title.front();
        const char* end = &item.title.back() + 1;
        SetWindowFontScale(1.5f);
        RenderText(pos + ImVec2{pos_x, pos_y}, begin, end, false);
        pos_y += ctx.FontSize * text_spacing;
    }
    SetWindowFontScale(1.1f);

    if (item.is_corrupted) {
        pos_y -= ctx.FontSize * text_spacing * 0.3f;
        const auto bright = (int)std::abs(std::sin(ctx.Time) * 0.15f * 255.0f);
        PushStyleColor(ImGuiCol_Text, IM_COL32(bright + 216, bright, bright, 0xFF));
        RenderText(pos + ImVec2{pos_x, pos_y}, "Corrupted", nullptr, false);
        PopStyleColor();
        pos_y += ctx.FontSize * text_spacing * 0.8f;
    }

    if (state->style == OrbisSaveDataDialogItemStyle::TITLE_SUBTITLE_DATESIZE) {
        if (!item.subtitle.empty()) {
            const char* begin = &item.subtitle.front();
            const char* end = &item.subtitle.back() + 1;
            RenderText(pos + ImVec2{pos_x, pos_y}, begin, end, false);
        }
        pos_y += ctx.FontSize * text_spacing;
    }

    {
        float width = 0.0f;
        if (!item.date.empty()) {
            const char* d_begin = &item.date.front();
            const char* d_end = &item.date.back() + 1;
            width = CalcTextSize(d_begin, d_end).x + 15.0f;
            RenderText(pos + ImVec2{pos_x, pos_y}, d_begin, d_end, false);
        }
        if (!item.size.empty()) {
            const char* s_begin = &item.size.front();
            const char* s_end = &item.size.back() + 1;
            RenderText(pos + ImVec2{pos_x + width, pos_y}, s_begin, s_end, false);
        }
        pos_y += ctx.FontSize * text_spacing;
    }

    if (state->style == OrbisSaveDataDialogItemStyle::TITLE_DATASIZE_SUBTITLE &&
        !item.subtitle.empty()) {
        const char* begin = &item.subtitle.front();
        const char* end = &item.subtitle.back() + 1;
        RenderText(pos + ImVec2{pos_x, pos_y}, begin, end, false);
    }

    SetWindowFontScale(1.0f);

    if (hovered) {
        window.DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), 0.0f, 0, 2.0f);
    }
}

void SaveDialogUi::DrawList() {
    auto availableSize = GetContentRegionAvail();

    constexpr auto footerHeight = 30.0f;
    availableSize.y -= footerHeight + 1.0f;

    BeginChild("##ScrollingRegion", availableSize, ImGuiChildFlags_NavFlattened);
    int i = 0;
    if (state->new_item.has_value()) {
        DrawItem(i++, state->new_item.value());
    }
    for (const auto& item : state->save_list) {
        DrawItem(i++, item);
    }
    if (first_render) { // Make the initial focus
        if (std::holds_alternative<OrbisSaveDataDialogFocusPos>(state->focus_pos)) {
            auto pos = std::get<OrbisSaveDataDialogFocusPos>(state->focus_pos);
            if (pos == OrbisSaveDataDialogFocusPos::LISTHEAD ||
                pos == OrbisSaveDataDialogFocusPos::DATAHEAD) {
                SetItemCurrentNavFocus(GetID(0));
            } else if (pos == OrbisSaveDataDialogFocusPos::LISTTAIL ||
                       pos == OrbisSaveDataDialogFocusPos::DATATAIL) {
                SetItemCurrentNavFocus(GetID(std::max(i - 1, 0)));
            } else { // Date
                int idx = 0;
                int max_idx = 0;
                bool is_min = pos == OrbisSaveDataDialogFocusPos::DATAOLDEST;
                std::chrono::system_clock::time_point max_write{};
                if (state->new_item.has_value()) {
                    idx++;
                }
                for (const auto& item : state->save_list) {
                    if (item.last_write > max_write ^ is_min) {
                        max_write = item.last_write;
                        max_idx = idx;
                    }
                    idx++;
                }
                SetItemCurrentNavFocus(GetID(max_idx));
            }
        } else if (std::holds_alternative<std::string>(state->focus_pos)) {
            auto dir_name = std::get<std::string>(state->focus_pos);
            if (dir_name.empty()) {
                SetItemCurrentNavFocus(GetID(0));
            } else {
                int idx = 0;
                if (state->new_item.has_value()) {
                    if (dir_name == state->new_item->dir_name) {
                        SetItemCurrentNavFocus(GetID(idx));
                    }
                    idx++;
                }
                for (const auto& item : state->save_list) {
                    if (item.dir_name == dir_name) {
                        SetItemCurrentNavFocus(GetID(idx));
                        break;
                    }
                    idx++;
                }
            }
        }
    }
    EndChild();

    Separator();
    if (state->enable_back.value_or(true)) {
        constexpr auto back = "Back";
        constexpr float pad = 7.0f;
        const auto txt_size = CalcTextSize(back);
        const auto button_size = ImVec2{
            std::max(txt_size.x, 100.0f) + pad * 2.0f,
            footerHeight - pad,
        };
        SetCursorPosX(GetContentRegionAvail().x - button_size.x);
        if (Button(back, button_size)) {
            result->dir_name.clear();
            Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
        }
        if (IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
            SetItemCurrentNavFocus();
        }
    }
}

void SaveDialogUi::DrawUser() {
    const auto& user_state = state->GetState<SaveDialogState::UserState>();
    const auto btn_type = user_state.type;

    const auto ws = GetWindowSize();

    if (!state->save_list.empty()) {
        DrawItem(0, state->save_list.front(), false);
    } else if (state->new_item) {
        DrawItem(0, *state->new_item, false);
    }

    auto has_btn = btn_type != OrbisSaveDataDialogButtonType::NONE;
    ImVec2 btn_space;
    if (has_btn) {
        btn_space = ImVec2{0.0f, FOOTER_HEIGHT};
    }

    const auto& msg = user_state.msg;
    if (!msg.empty()) {
        const char* begin = &msg.front();
        const char* end = &msg.back() + 1;
        if (user_state.msg_type == OrbisSaveDataDialogUserMessageType::ERROR) {
            PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            // Maybe make the text bold?
        }
        DrawCenteredText(begin, end, GetContentRegionAvail() - btn_space);
        if (user_state.msg_type == OrbisSaveDataDialogUserMessageType::ERROR) {
            PopStyleColor();
        }
    }

    if (has_btn) {
        int count = 1;
        if (btn_type == OrbisSaveDataDialogButtonType::YESNO ||
            btn_type == OrbisSaveDataDialogButtonType::OKCANCEL) {
            ++count;
        }

        SetCursorPos({
            ws.x / 2.0f - BUTTON_SIZE.x / 2.0f * static_cast<float>(count),
            ws.y - FOOTER_HEIGHT + 5.0f,
        });

        BeginGroup();
        if (btn_type == OrbisSaveDataDialogButtonType::YESNO) {
            if (Button("Yes", BUTTON_SIZE)) {
                Finish(OrbisSaveDataDialogButtonId::YES);
            }
            SameLine();
            if (Button("No", BUTTON_SIZE)) {
                if (GetFirmwareVerSafe() < ElfInfo::FW_450) {
                    Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
                } else {
                    Finish(OrbisSaveDataDialogButtonId::NO);
                }
            }
            if (first_render || IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                SetItemCurrentNavFocus();
            }
        } else {
            if (Button("OK", BUTTON_SIZE)) {
                if (btn_type == OrbisSaveDataDialogButtonType::OK &&
                    GetFirmwareVerSafe() < ElfInfo::FW_450) {
                    Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
                } else {
                    Finish(OrbisSaveDataDialogButtonId::OK);
                }
            }
            if (first_render) {
                SetItemCurrentNavFocus();
            }
            if (btn_type == OrbisSaveDataDialogButtonType::OKCANCEL) {
                SameLine();
                if (Button("Cancel", BUTTON_SIZE)) {
                    Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
                }
                if (IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                    SetItemCurrentNavFocus();
                }
            }
        }
        EndGroup();
    }
}

void SaveDialogUi::DrawSystemMessage() {
    const auto& sys_state = state->GetState<SaveDialogState::SystemState>();

    if (!state->save_list.empty()) {
        DrawItem(0, state->save_list.front(), false);
    } else if (state->new_item) {
        DrawItem(0, *state->new_item, false);
    }

    const auto ws = GetWindowSize();
    const auto& msg = sys_state.msg;
    if (!msg.empty()) {
        const char* begin = &msg.front();
        const char* end = &msg.back() + 1;
        DrawCenteredText(begin, end, GetContentRegionAvail() - ImVec2{0.0f, FOOTER_HEIGHT});
    }
    int count = 1;
    if (sys_state.hide_ok) {
        --count;
    }
    if (sys_state.show_no || sys_state.show_cancel) {
        ++count;
    }

    SetCursorPos({
        ws.x / 2.0f - BUTTON_SIZE.x / 2.0f * static_cast<float>(count),
        ws.y - FOOTER_HEIGHT + 5.0f,
    });
    BeginGroup();
    if (Button(sys_state.show_no ? "Yes" : "OK", BUTTON_SIZE)) {
        if (sys_state.return_cancel && GetFirmwareVerSafe() < ElfInfo::FW_450) {
            Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
        } else {
            Finish(OrbisSaveDataDialogButtonId::YES);
        }
    }
    SameLine();
    if (sys_state.show_no) {
        if (Button("No", BUTTON_SIZE)) {
            if (GetFirmwareVerSafe() < ElfInfo::FW_450) {
                Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
            } else {
                Finish(OrbisSaveDataDialogButtonId::NO);
            }
        }
    } else if (sys_state.show_cancel) {
        if (Button("Cancel", BUTTON_SIZE)) {
            Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
        }
    }
    if (first_render || IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
        SetItemCurrentNavFocus();
    }
    EndGroup();
}

void SaveDialogUi::DrawErrorCode() {
    const auto& err_state = state->GetState<SaveDialogState::ErrorCodeState>();

    if (!state->save_list.empty()) {
        DrawItem(0, state->save_list.front(), false);
    } else if (state->new_item) {
        DrawItem(0, *state->new_item, false);
    }

    const auto ws = GetWindowSize();
    const auto& msg = err_state.error_msg;
    if (!msg.empty()) {
        const char* begin = &msg.front();
        const char* end = &msg.back() + 1;
        DrawCenteredText(begin, end, GetContentRegionAvail() - ImVec2{0.0f, FOOTER_HEIGHT});
    }

    SetCursorPos({
        ws.x / 2.0f - BUTTON_SIZE.x / 2.0f,
        ws.y - FOOTER_HEIGHT + 5.0f,
    });
    if (Button("OK", BUTTON_SIZE)) {
        if (GetFirmwareVerSafe() < ElfInfo::FW_450) {
            Finish(OrbisSaveDataDialogButtonId::INVALID, Result::USER_CANCELED);
        } else {
            Finish(OrbisSaveDataDialogButtonId::OK);
        }
    }
    if (first_render) {
        SetItemCurrentNavFocus();
    }
}

void SaveDialogUi::DrawProgressBar() {
    const auto& bar_state = state->GetState<SaveDialogState::ProgressBarState>();

    const auto ws = GetWindowSize();

    if (!state->save_list.empty()) {
        DrawItem(0, state->save_list.front(), false);
    } else if (state->new_item) {
        DrawItem(0, *state->new_item, false);
    }

    const auto& msg = bar_state.msg;
    if (!msg.empty()) {
        const char* begin = &msg.front();
        const char* end = &msg.back() + 1;
        DrawCenteredText(begin, end, GetContentRegionAvail() - ImVec2{0.0f, FOOTER_HEIGHT});
    }

    SetCursorPos({
        ws.x * ((1 - PROGRESS_BAR_WIDTH) / 2.0f),
        ws.y - FOOTER_HEIGHT + 5.0f,
    });

    ProgressBar(static_cast<float>(bar_state.progress) / 100.0f,
                {PROGRESS_BAR_WIDTH * ws.x, BUTTON_SIZE.y});
}
}; // namespace Libraries::SaveData::Dialog