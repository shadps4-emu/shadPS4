// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <string>
#include <vector>
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

namespace Libraries::Np::NpCommerce {

// OrbisNpCommerceDialogMode
enum class CommerceMode : s32 {
    CATEGORY = 0,
    PRODUCT = 1,
    PRODUCT_CODE = 2,
    CHECKOUT = 3,
    DOWNLOADLIST = 4,
    PLUS = 5,
};

// OrbisNpCommerceDialogResult result values
enum class CommerceResult : s32 {
    OK = 0,
    USER_CANCELED = 1,
    PURCHASED = 2,
};

struct CommerceDialogState {
    std::vector<std::string> targets; // product / category labels
    std::string username;             // resolved from userId, may be empty
    CommerceMode mode{CommerceMode::CATEGORY};
    s32 user_id{-1};
    u32 service_label{0};
    u64 features{0};          // ORBIS_NP_PLUS_FEATURE_* (PLUS mode only)
    void* user_data{nullptr}; // echoed back into the result struct
};

class CommerceDialogUi final : public ImGui::Layer {
    bool first_render{false};
    CommerceDialogState* state{};
    CommonDialog::Status* status{};
    s32* result{};

public:
    explicit CommerceDialogUi(CommerceDialogState* state = nullptr,
                              CommonDialog::Status* status = nullptr, s32* result = nullptr);
    ~CommerceDialogUi() override;
    CommerceDialogUi(const CommerceDialogUi&) = delete;
    CommerceDialogUi& operator=(const CommerceDialogUi&) = delete;
    CommerceDialogUi(CommerceDialogUi&& other) noexcept;
    CommerceDialogUi& operator=(CommerceDialogUi&& other) noexcept;

    void Finish(CommerceResult r);
    void Draw() override;
};

// OrbisNpCommercePsStoreIconPos
enum class PsStoreIconPos : s32 {
    CENTER = 0, // screen lower center
    LEFT = 1,   // screen lower left
    RIGHT = 2,  // screen lower right
};

// OrbisNpCommercePsStoreIconLayout
enum class PsStoreIconLayout : s32 {
    DEFAULT = 0,          // full screen (100%)
    FOLLOW_SAFE_AREA = 1, // follow display safe area (90%-100%)
    FIXED_90 = 2,         // fixed 90% of screen
};

class PsStoreIconLayer final : public ImGui::Layer {
    std::atomic<bool> shown{false};
    std::atomic<PsStoreIconPos> pos{PsStoreIconPos::CENTER};
    std::atomic<PsStoreIconLayout> layout{PsStoreIconLayout::DEFAULT};

public:
    static PsStoreIconLayer& Instance();

    void Show(PsStoreIconPos p);
    void Hide();
    void SetLayout(PsStoreIconLayout l);

    void Draw() override;
};

} // namespace Libraries::Np::NpCommerce
