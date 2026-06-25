// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <imgui.h>
#include "common/logging/log.h"
#include "core/libraries/np/np_commerce/np_commerce_ui.h"
#include "imgui/imgui_std.h"

using namespace ImGui;
using namespace Libraries::CommonDialog;
using namespace Libraries::Np::NpCommerce;

static constexpr ImVec2 BUTTON_SIZE{160.0f, 30.0f};

static const char* ModeTitle(CommerceMode m) {
    switch (m) {
    case CommerceMode::CATEGORY:
        return "shadNet Store";
    case CommerceMode::PRODUCT:
        return "Product";
    case CommerceMode::PRODUCT_CODE:
        return "Redeem Code";
    case CommerceMode::CHECKOUT:
        return "Purchase Confirmation";
    case CommerceMode::DOWNLOADLIST:
        return "My Downloads";
    case CommerceMode::PLUS:
        return "shadNet Plus";
    default:
        return "shadNet Store";
    }
}

static const char* ModeActionLabel(CommerceMode m) {
    switch (m) {
    case CommerceMode::CATEGORY:
    case CommerceMode::PRODUCT:
    case CommerceMode::CHECKOUT:
        return "Simulate purchase";
    case CommerceMode::PRODUCT_CODE:
        return "Redeem code";
    case CommerceMode::PLUS:
        return "Join SN Plus";
    case CommerceMode::DOWNLOADLIST:
    default:
        return nullptr;
    }
}

static const char* ModeEmptyText(CommerceMode m) {
    switch (m) {
    case CommerceMode::CATEGORY:
        return "Top store category.";
    case CommerceMode::PRODUCT_CODE:
        return "Enter a promotion code.";
    case CommerceMode::DOWNLOADLIST:
        return "All downloadable content.";
    case CommerceMode::PLUS:
        return "Join shadNet Plus.";
    default:
        return "No product specified.";
    }
}

static const char* ModeDescription(CommerceMode m) {
    switch (m) {
    case CommerceMode::CATEGORY:
        return "Browsing a store category.";
    case CommerceMode::PRODUCT:
        return "Viewing a product.";
    case CommerceMode::PRODUCT_CODE:
        return "Redeeming a promotion code.";
    case CommerceMode::CHECKOUT:
        return "Confirming a purchase.";
    case CommerceMode::DOWNLOADLIST:
        return "Viewing owned downloads.";
    case CommerceMode::PLUS:
        return "Joining shadNet Plus.";
    default:
        return "";
    }
}

static std::string DescribeFeatures(u64 features) {
    if (features == 0) {
        return "none";
    }
    std::string out;
    if (features & 0x1) { // ORBIS_NP_PLUS_FEATURE_REALTIME_MULTIPLAY
        out += "Realtime Multiplay";
    }
    if (out.empty()) {
        return "unknown";
    }
    return out;
}

// Decomposed CHECKOUT/DOWNLOADLIST element "[serviceLabel:]product[-sku]".
struct ParsedTarget {
    std::string service;
    std::string product;
    std::string sku;
};

static ParsedTarget ParseTarget(const std::string& t) {
    ParsedTarget out;
    std::string s = t;
    if (const auto colon = s.find(':'); colon != std::string::npos) {
        out.service = s.substr(0, colon);
        s = s.substr(colon + 1);
    }
    if (const auto dash = s.find('-'); dash != std::string::npos) {
        out.product = s.substr(0, dash);
        out.sku = s.substr(dash + 1);
    } else {
        out.product = s;
    }
    return out;
}

CommerceDialogUi::CommerceDialogUi(CommerceDialogState* state, Status* status, s32* result)
    : state(state), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

CommerceDialogUi::~CommerceDialogUi() {
    Finish(CommerceResult::USER_CANCELED);
}

CommerceDialogUi::CommerceDialogUi(CommerceDialogUi&& other) noexcept
    : Layer(other), first_render(other.first_render), state(other.state), status(other.status),
      result(other.result) {
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
}

CommerceDialogUi& CommerceDialogUi::operator=(CommerceDialogUi&& other) noexcept {
    if (this != &other) {
        first_render = other.first_render;
        state = other.state;
        status = other.status;
        result = other.result;
        other.state = nullptr;
        other.status = nullptr;
        other.result = nullptr;
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
    }
    return *this;
}

void CommerceDialogUi::Finish(CommerceResult r) {
    if (result) {
        *result = static_cast<s32>(r);
    }
    if (status) {
        *status = Status::FINISHED;
    }
    state = nullptr;
    status = nullptr;
    result = nullptr;
    RemoveLayer(this);
}

void CommerceDialogUi::Draw() {
    if (status == nullptr || *status != Status::RUNNING || state == nullptr) {
        return;
    }
    const auto& io = GetIO();
    const ImVec2 window_size{std::min(io.DisplaySize.x, 520.0f),
                             std::min(io.DisplaySize.y, 380.0f)};

    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    KeepNavHighlight();
    if (Begin("CommerceDialog##NpCommerceDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        std::string title = ModeTitle(state->mode);
        if (state->targets.size() == 1 &&
            (state->mode == CommerceMode::PRODUCT || state->mode == CommerceMode::CATEGORY)) {
            title += ": " + state->targets.front();
        }
        {
            const float tw = CalcTextSize(title.c_str()).x;
            SetCursorPosX(std::max(10.0f, (GetWindowSize().x - tw) * 0.5f));
            TextUnformatted(title.c_str());
        }
        Separator();

        TextWrapped("%s", ModeDescription(state->mode));
        Spacing();

        // Calling user.
        if (!state->username.empty()) {
            Text("User: %s (id %d)", state->username.c_str(), state->user_id);
        } else {
            Text("User id: %d", state->user_id);
        }
        // Service label is meaningful for category/product/checkout/downloadlist.
        if (state->mode != CommerceMode::PRODUCT_CODE && state->mode != CommerceMode::PLUS) {
            Text("Service label: %u", state->service_label);
        }
        // PS Plus feature only applies in PLUS mode.
        if (state->mode == CommerceMode::PLUS) {
            Text("PS Plus feature: %s", DescribeFeatures(state->features).c_str());
        }

        Spacing();
        if (state->targets.empty()) {
            TextWrapped("%s", ModeEmptyText(state->mode));
        } else {
            const bool checkout_form =
                state->mode == CommerceMode::CHECKOUT || state->mode == CommerceMode::DOWNLOADLIST;
            TextDisabled(state->targets.size() == 1 ? "Target:" : "Targets:");
            for (const auto& t : state->targets) {
                if (checkout_form) {
                    const ParsedTarget pt = ParseTarget(t);
                    if (!pt.sku.empty() || !pt.service.empty()) {
                        BulletText("Product %s", pt.product.c_str());
                        if (!pt.sku.empty()) {
                            Indent();
                            Text("SKU: %s", pt.sku.c_str());
                            Unindent();
                        }
                        if (!pt.service.empty()) {
                            Indent();
                            Text("Service: %s", pt.service.c_str());
                            Unindent();
                        }
                        continue;
                    }
                }
                BulletText("%s", t.c_str());
            }
        }

        Spacing();

        const char* action = ModeActionLabel(state->mode);
        const auto ws = GetWindowSize();
        const float n = action ? 2.0f : 1.0f;
        SetCursorPos({ws.x / 2.0f - (BUTTON_SIZE.x * n + 10.0f * (n - 1.0f)) / 2.0f,
                      ws.y - 10.0f - BUTTON_SIZE.y});
        BeginGroup();
        if (action) {
            if (Button(action, BUTTON_SIZE)) {
                LOG_INFO(Lib_NpCommerce, "CommerceDialog: action '{}' mode={} target='{}'", action,
                         static_cast<s32>(state->mode),
                         state->targets.empty() ? "" : state->targets.front());
                Finish(CommerceResult::PURCHASED);
            }
            SameLine();
        }
        if (Button("Close", BUTTON_SIZE)) {
            Finish(CommerceResult::USER_CANCELED);
        }
        // Default focus on Close so a stray confirm press dismisses, never buys.
        if (first_render) {
            SetItemCurrentNavFocus();
        }
        EndGroup();
    }
    End();

    first_render = false;
}

static float LayoutScale(PsStoreIconLayout l) {
    switch (l) {
    case PsStoreIconLayout::FIXED_90:
        return 0.90f;
    case PsStoreIconLayout::FOLLOW_SAFE_AREA:
        return 0.95f;
    case PsStoreIconLayout::DEFAULT:
    default:
        return 1.0f;
    }
}

PsStoreIconLayer& PsStoreIconLayer::Instance() {
    static PsStoreIconLayer instance;
    return instance;
}

void PsStoreIconLayer::Show(PsStoreIconPos p) {
    pos.store(p);
    if (!shown.exchange(true)) {
        AddLayer(this);
    }
}

void PsStoreIconLayer::Hide() {
    if (shown.exchange(false)) {
        RemoveLayer(this);
    }
}

void PsStoreIconLayer::SetLayout(PsStoreIconLayout l) {
    layout.store(l);
}

void PsStoreIconLayer::Draw() {
    if (!shown.load()) {
        return;
    }
    const auto& io = GetIO();
    const ImVec2 ds = io.DisplaySize;
    const float scale = LayoutScale(layout.load());
    const float inset_x = ds.x * (1.0f - scale) * 0.5f;
    const float inset_y = ds.y * (1.0f - scale) * 0.5f;

    constexpr ImVec2 icon{52.0f, 52.0f};
    constexpr float pad = 16.0f;

    float x;
    switch (pos.load()) {
    case PsStoreIconPos::LEFT:
        x = inset_x + pad;
        break;
    case PsStoreIconPos::RIGHT:
        x = ds.x - inset_x - icon.x - pad;
        break;
    case PsStoreIconPos::CENTER:
    default:
        x = (ds.x - icon.x) * 0.5f;
        break;
    }
    const float y = ds.y - inset_y - icon.y - pad;

    SetNextWindowPos({x, y});
    SetNextWindowSize(icon);
    if (Begin("##PsStoreIcon", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
        auto* dl = GetWindowDrawList();
        const ImVec2 p0 = GetWindowPos();
        const ImVec2 p1{p0.x + icon.x, p0.y + icon.y};
        dl->AddRectFilled(p0, p1, IM_COL32(0, 55, 145, 230), 10.0f);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 210), 10.0f, 0, 2.0f);
        const char* label = "SN";
        const ImVec2 ts = CalcTextSize(label);
        dl->AddText({p0.x + (icon.x - ts.x) * 0.5f, p0.y + (icon.y - ts.y) * 0.5f},
                    IM_COL32(255, 255, 255, 255), label);
    }
    End();
}
