// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <cstdint>
#include <new>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "ime_dialog.h"
#include "ime_dialog_ui.h"

static constexpr std::array<float, 2> MAX_X_POSITIONS = {3840.0f, 1920.0f};
static constexpr std::array<float, 2> MAX_Y_POSITIONS = {2160.0f, 1080.0f};

namespace Libraries::ImeDialog {

static OrbisImeDialogStatus g_ime_dlg_status = OrbisImeDialogStatus::None;
static OrbisImeDialogResult g_ime_dlg_result{};
static ImeDialogState g_ime_dlg_state{};
static ImeDialogUi g_ime_dlg_ui;
static OrbisImeDialogParam g_ime_dlg_param{};
static OrbisImeParamExtended g_ime_dlg_extended{};
static bool g_ime_dlg_has_extended = false;
static bool g_ime_dlg_result_committed = false;
static bool g_ime_dlg_sw_version_cached = false;
static u32 g_ime_dlg_sw_version_hex = 0;
static OrbisImeExtKeyboardFilter g_ime_dlg_ext_keyboard_filter = nullptr;
static Libraries::UserService::OrbisUserServiceUserId g_ime_dlg_ext_keyboard_filter_user_id =
    Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
static bool g_ime_dlg_ext_keyboard_filter_registered = false;
static bool g_ime_dlg_ext_keyboard_filter_active = false;
static u32 g_ime_dlg_resource_id = 0;

static Error ValidateImeDialogParam(const OrbisImeDialogParam* param,
                                    const OrbisImeParamExtended* extended, bool internal);
static Error SetupDialogState(OrbisImeDialogParam* param, OrbisImeParamExtended* extended,
                              bool internal);
static bool IsValidDialogUserId(s32 user_id, bool internal);
static Error ComputeImeDialogPanelSize(const OrbisImeDialogParam* param, u32* width, u32* height,
                                       bool log);
static Error ComputeImeDialogPanelSizeExtended(const OrbisImeDialogParam* param,
                                               const OrbisImeParamExtended* extended, u32* width,
                                               u32* height, bool log);

struct ImeDialogClientStub {
    bool connected = false;
    bool started = false;
    u32 user_id = static_cast<u32>(Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID);
    u32 resource_id = 0;
    int dialog_state = 0;
};

static ImeDialogClientStub* g_ime_dlg_client = nullptr;

static void DestroyImeDialogClient() {
    if (g_ime_dlg_client) {
        delete g_ime_dlg_client;
        g_ime_dlg_client = nullptr;
    }
}

static ImeDialogClientStub* CreateImeDialogClient() {
    auto* client = new (std::nothrow) ImeDialogClientStub{};
    return client;
}

static void InitImeDialogClient(ImeDialogClientStub* client) {
    if (!client) {
        return;
    }
    *client = {};
    client->user_id = static_cast<u32>(Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID);
}

static int ImeDialogClientConnect(ImeDialogClientStub* client, u32 client_type, u32 resource_id,
                                  Libraries::UserService::OrbisUserServiceUserId user_id,
                                  bool internal) {
    (void)client_type;
    if (!client) {
        return static_cast<int>(Error::INTERNAL);
    }
    if (!IsValidDialogUserId(user_id, internal)) {
        return static_cast<int>(Error::INVALID_USER_ID);
    }
    client->connected = true;
    client->resource_id = resource_id;
    client->user_id = static_cast<u32>(user_id);
    return 0;
}

static int ImeDialogClientStart(ImeDialogClientStub* client, const OrbisImeDialogParam* param,
                                const OrbisImeParamExtended* extended, u32 user_flags, s64 user_id,
                                s32 unk1, s32 unk2, bool internal) {
    (void)param;
    (void)extended;
    (void)user_flags;
    (void)user_id;
    (void)unk1;
    (void)unk2;
    (void)internal;
    if (!client || !client->connected) {
        return static_cast<int>(Error::NOT_ACTIVE);
    }
    client->started = true;
    return 0;
}

static int ImeDialogClientStartFallback(ImeDialogClientStub* client,
                                        const OrbisImeDialogParam* param,
                                        const OrbisImeParamExtended* extended, u32 user_flags,
                                        s64 user_id) {
    return ImeDialogClientStart(client, param, extended, user_flags, user_id, 0, -1, false);
}

static void ImeDialogClientShutdown(ImeDialogClientStub* client) {
    if (!client) {
        return;
    }
    client->started = false;
    client->connected = false;
    client->dialog_state = 0;
}

static Error ImeDialogClientDisconnect(ImeDialogClientStub* client) {
    if (!client || !client->connected) {
        return Error::CONNECTION_FAILED;
    }
    ImeDialogClientShutdown(client);
    return Error::OK;
}

static int RegisterExtKeyboardFilter(Libraries::UserService::OrbisUserServiceUserId user_id,
                                     OrbisImeExtKeyboardFilter filter) {
    (void)user_id;
    (void)filter;
    return ORBIS_OK;
}

static void SetExtKeyboardFilterActive(bool active) {
    g_ime_dlg_ext_keyboard_filter_active = active;
}

static void NotifyExtKeyboardFilterState(bool active) {
    if (!g_ime_dlg_ext_keyboard_filter || !g_ime_dlg_ext_keyboard_filter_registered) {
        g_ime_dlg_ext_keyboard_filter_active = false;
        return;
    }
    SetExtKeyboardFilterActive(active);
}

static void ComputeUserFlags(Libraries::UserService::OrbisUserServiceUserId user_id, u32* flags,
                             s64* out_user) {
    u32 local_flags = 0x11;
    s64 local_user = 0;
    const s32 uid = user_id;
    if ((uid + 1) > 1 && uid != 0xff) {
        if (uid == 0xfe) {
            local_flags = 0;
            local_user = 0;
        } else {
            local_flags = 1;
            local_user = uid;
        }
    }
    if (flags) {
        *flags = local_flags;
    }
    if (out_user) {
        *out_user = local_user;
    }
}

static int InitDialogInternalWithClient(OrbisImeDialogParam* param, OrbisImeParamExtended* extended,
                                        u32 user_flags, s64 user_value, u32 resource_id, u32 unk1,
                                        u32 unk2, bool connect_returns_connection_failed) {
    if (g_ime_dlg_client != nullptr || g_ime_dlg_status != OrbisImeDialogStatus::None) {
        return static_cast<int>(Error::BUSY);
    }
    if (!param) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }

    const Error validate_ret = ValidateImeDialogParam(param, extended, true);
    if (validate_ret != Error::OK) {
        return static_cast<int>(validate_ret);
    }

    g_ime_dlg_resource_id = resource_id;
    g_ime_dlg_client = CreateImeDialogClient();
    if (!g_ime_dlg_client) {
        return static_cast<int>(Error::NO_MEMORY);
    }
    InitImeDialogClient(g_ime_dlg_client);

    const int connect_ret =
        ImeDialogClientConnect(g_ime_dlg_client, 2, g_ime_dlg_resource_id, param->user_id, true);
    if (connect_ret != 0) {
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        if (connect_returns_connection_failed) {
            return static_cast<int>(Error::CONNECTION_FAILED);
        }
        return connect_ret;
    }

    const int start_ret =
        ImeDialogClientStart(g_ime_dlg_client, param, extended, user_flags, user_value,
                             static_cast<s32>(unk1), static_cast<s32>(unk2), true);
    if (start_ret != 0) {
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        return start_ret;
    }

    const Error setup_ret = SetupDialogState(param, extended, true);
    if (setup_ret != Error::OK) {
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        return static_cast<int>(setup_ret);
    }

    return static_cast<int>(Error::OK);
}

static void CommitDialogResultIfNeeded() {
    if (g_ime_dlg_result_committed) {
        return;
    }
    if (g_ime_dlg_status != OrbisImeDialogStatus::Finished) {
        return;
    }

    g_ime_dlg_state.CallTextFilter();
    const bool restore_original = (g_ime_dlg_result.endstatus != OrbisImeDialogEndStatus::Ok);
    g_ime_dlg_state.CopyTextToOrbisBuffer(restore_original);
    g_ime_dlg_result_committed = true;
    LOG_DEBUG(Lib_ImeDialog, "Committed result (restore_original={})", restore_original);
}

static bool UseOver2kCoordinates(const OrbisImeDialogParam& param) {
    return True(param.option & OrbisImeOption::USE_OVER_2K_COORDINATES);
}

static float ToInternalCoord(float value, const OrbisImeDialogParam& param) {
    return UseOver2kCoordinates(param) ? value : (value * 2.0f);
}

static float FromInternalCoord(float value, const OrbisImeDialogParam& param) {
    return UseOver2kCoordinates(param) ? value : std::round(value * 0.5f);
}

static float ClampInternalX(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value >= 3840.0f) {
        return 3839.0f;
    }
    return value;
}

static float ClampInternalY(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value >= 2160.0f) {
        return 2159.0f;
    }
    return value;
}

static u32 GetCompiledSdkVersion() {
    if (!g_ime_dlg_sw_version_cached) {
        Libraries::Kernel::SwVersionStruct sw{};
        sw.struct_size = sizeof(sw);
        if (Libraries::Kernel::sceKernelGetSystemSwVersion(&sw) == ORBIS_OK) {
            g_ime_dlg_sw_version_hex = sw.hex_representation;
        }
        g_ime_dlg_sw_version_cached = true;
    }

    s32 ver = 0;
    if (Libraries::Kernel::sceKernelGetCompiledSdkVersion(&ver) != ORBIS_OK) {
        return 0;
    }
    return static_cast<u32>(ver);
}

static u32 GetImeDialogOptionMask() {
    const u32 sdk = GetCompiledSdkVersion();
    u32 uVar8 = 0x80068ff;
    if (sdk > 0x14fffff) {
        uVar8 = 0x69ff;
    }
    u32 uVar5 = uVar8 & 0x80061ff;
    if (sdk > 0x174ffff) {
        uVar5 = uVar8;
    }
    uVar8 = uVar5 & 0x80049ff;
    if (sdk > 0x34fffff) {
        uVar8 = uVar5;
    }
    uVar5 = uVar8 & 0x80029ff;
    if (sdk > 0x3ffffff) {
        uVar5 = uVar8;
    }
    return uVar5;
}

static u64 GetImeDialogLanguageMask() {
    const u32 sdk = GetCompiledSdkVersion();
    u64 uVar3 = (sdk > 0x1ffffff) ? (0x1000000ULL + 0x3fe1fffffULL) : 0x3fe1fffffULL;
    u64 uVar7 = uVar3 & 0x3fd1fffffULL;
    if (sdk > 0x24fffff) {
        uVar7 = uVar3;
    }
    u64 uVar8 = uVar7 & 0x2031fffffULL;
    if (sdk > 0x4ffffff) {
        uVar8 = uVar7;
    }
    uVar7 = uVar8 & 0x1ff1fffffULL;
    if (sdk > 0xfffffff) {
        uVar7 = uVar8;
    }
    return uVar7;
}

static u32 GetImeDialogOptionMaskInternal() {
    const u32 sdk = GetCompiledSdkVersion();
    u32 mask = (sdk > 0x14fffff) ? 0xf7f06fff : 0xff706eff;

    // SDK < 0x1700000 masks some bits, otherwise ORs 0x08600000
    u32 tmp = (sdk < 0x1700000) ? (mask & 0xffe06bff) : (mask | 0x08600000);
    u32 tmp2 = (sdk > 0x174ffff) ? tmp : (tmp & 0xfff067ff);
    u32 tmp3 = (sdk > 0x34fffff) ? tmp2 : (tmp2 & 0xfff04fff);
    u32 tmp4 = (sdk > 0x3ffffff) ? tmp3 : (tmp3 & 0xfff02fff);
    return tmp4;
}

static u64 GetImeDialogLanguageMaskInternal() {
    const u32 sdk = GetCompiledSdkVersion();
    const u64 base = (sdk > 0x1ffffff ? 0x1000000ULL : 0ULL) + 0x303fe1fffffULL;
    u64 mask = (sdk > 0x24fffff) ? base : (base & 0x303fd1fffffULL);
    mask = (sdk > 0x4ffffff) ? mask : (mask & 0x302031fffffULL);
    mask = (sdk > 0xfffffff) ? mask : (mask & 0x301ff1fffffULL);
    return mask;
}

static bool IsValidDialogExtOption_2a70(u32 option, u32 sdk) {
    u32 mask = (sdk < 0x1560000) ? 0x41df : 0x4fdf;
    if ((option & 0x4080) == 0x4000) {
        return false;
    }
    u32 allow = (sdk > 0x5ffffff) ? mask : (mask & 0x0fdf);
    return (~allow & option) == 0;
}

static bool IsValidDialogExtOption_00bd0(u32 option, u32 sdk) {
    u32 mask = (sdk < 0x1560000) ? 0x71df : 0x7fdf;
    u32 allow = (sdk > 0x24fffff) ? mask : (mask & 0x4fdf);
    if (((option & 0x3000) == 0x2000) || ((option & 0x4080) == 0x4000)) {
        return false;
    }
    allow = (sdk > 0x5ffffff) ? allow : (allow & 0x3fdf);
    return (~allow & option) == 0;
}

static bool IsValidDialogExtOption(u32 option) {
    const u32 sdk = GetCompiledSdkVersion();
    if (sdk < 0x1500000) {
        return (option & 0xffffff20U) == 0;
    }
    if (sdk < 0x2500000) {
        return IsValidDialogExtOption_2a70(option, sdk);
    }
    return IsValidDialogExtOption_00bd0(option, sdk);
}

static bool IsValidDialogUserId(s32 user_id, bool internal) {
    const u32 sdk = GetCompiledSdkVersion();
    const u32 uid_u = static_cast<u32>(user_id);

    if (sdk < 0x1500000) {
        if ((uid_u + 1U) < 2U || (uid_u - 0xfeU) < 2U) {
            return true;
        }
        // We cannot query registered users (stubbed), allow non-negative ids.
        return user_id >= 0;
    }

    if (!internal) {
        if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID ||
            user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
            return false;
        }
        if (user_id == 0xfe) {
            return true;
        }
    } else {
        if ((uid_u + 1U) < 2U || (uid_u - 0xfeU) < 2U) {
            return true;
        }
    }

    return user_id >= 0;
}

static bool IsAllZero(const s8* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (data[i] != 0) {
            return false;
        }
    }
    return true;
}

static Error ValidateImeDialogParam(const OrbisImeDialogParam* param,
                                    const OrbisImeParamExtended* extended, bool internal) {
    if (!param) {
        return Error::INVALID_ADDRESS;
    }

    const u32 type = static_cast<u32>(param->type);
    if (!internal) {
        if (type > static_cast<u32>(OrbisImeType::Number)) {
            return Error::INVALID_TYPE;
        }
        const u32 option_mask = GetImeDialogOptionMask();
        if ((static_cast<u32>(param->option) & (option_mask ^ 0xfffffdffU)) != 0) {
            return Error::INVALID_OPTION;
        }
        const u64 lang_mask = GetImeDialogLanguageMask();
        if ((~lang_mask & static_cast<u64>(param->supported_languages)) != 0) {
            return Error::INVALID_SUPPORTED_LANGUAGES;
        }
    } else {
        if (type > static_cast<u32>(OrbisImeType::Number) && type != 0x100 && type != 0x101) {
            return Error::INVALID_TYPE;
        }
        const u32 option_mask = GetImeDialogOptionMaskInternal();
        if ((~option_mask & static_cast<u32>(param->option)) != 0) {
            return Error::INVALID_OPTION;
        }
        const u64 lang_mask = GetImeDialogLanguageMaskInternal();
        if ((~lang_mask & static_cast<u64>(param->supported_languages)) != 0) {
            return Error::INVALID_SUPPORTED_LANGUAGES;
        }
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (sdk < 0x1500000) {
        if (param->posx < 0.0f || param->posx >= 1920.0f) {
            return Error::INVALID_POSX;
        }
        if (param->posy < 0.0f || param->posy >= 1080.0f) {
            return Error::INVALID_POSY;
        }
    } else {
        if (param->posx < 0.0f ||
            param->posx >=
                MAX_X_POSITIONS[False(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES)]) {
            return Error::INVALID_POSX;
        }
        if (param->posy < 0.0f ||
            param->posy >=
                MAX_Y_POSITIONS[False(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES)]) {
            return Error::INVALID_POSY;
        }
    }

    if (static_cast<u32>(param->horizontal_alignment) >= 3) {
        return Error::INVALID_HORIZONTALIGNMENT;
    }
    if (static_cast<u32>(param->vertical_alignment) >= 3) {
        return Error::INVALID_VERTICALALIGNMENT;
    }

    const u32 option = static_cast<u32>(param->option);
    if ((option & 0x5U) == 0x5U) {
        return Error::INVALID_PARAM;
    }
    if ((option & 0x4U) != 0) {
        if (!(type > static_cast<u32>(OrbisImeType::Mail) ||
              type == static_cast<u32>(OrbisImeType::BasicLatin))) {
            return Error::INVALID_PARAM;
        }
    }
    if ((option & 0x1U) != 0) {
        if (!(type <= static_cast<u32>(OrbisImeType::BasicLatin) || type >= 0x100)) {
            return Error::INVALID_PARAM;
        }
    }

    if (!IsValidDialogUserId(param->user_id, internal)) {
        return Error::INVALID_USER_ID;
    }
    if (!internal && sdk < 0x1500000 &&
        param->user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return Error::INVALID_USER_ID;
    }

    if (!IsAllZero(param->reserved, sizeof(param->reserved))) {
        return Error::INVALID_RESERVED;
    }

    if (param->input_text_buffer == nullptr) {
        return Error::INVALID_INPUT_TEXT_BUFFER;
    }

    if (extended) {
        if (static_cast<u32>(extended->priority) > 3) {
            return Error::INVALID_EXTENDED;
        }
        if (!IsValidDialogExtOption(static_cast<u32>(extended->option))) {
            return Error::INVALID_EXTENDED;
        }
        if (!IsAllZero(extended->reserved, sizeof(extended->reserved))) {
            return Error::INVALID_EXTENDED;
        }
        if (sdk < 0x1560000) {
            if (extended->ext_keyboard_filter != nullptr) {
                return Error::INVALID_EXTENDED;
            }
            if (static_cast<u32>(extended->disable_device) != 0) {
                return Error::INVALID_EXTENDED;
            }
            if (extended->ext_keyboard_mode != 0) {
                return Error::INVALID_EXTENDED;
            }
        } else {
            if ((extended->ext_keyboard_mode & 0xe3fffffcU) != 0) {
                return Error::INVALID_EXTENDED;
            }
        }
        if (static_cast<u32>(extended->disable_device) > 7) {
            return Error::INVALID_EXTENDED;
        }
    }

    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogAbort() {
    LOG_INFO(Lib_ImeDialog, "Abort called (status={}, client_state={})",
             static_cast<u32>(g_ime_dlg_status),
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    g_ime_dlg_status = OrbisImeDialogStatus::Finished;
    g_ime_dlg_result.endstatus = OrbisImeDialogEndStatus::Aborted;
    CommitDialogResultIfNeeded();
    if (g_ime_dlg_client) {
        g_ime_dlg_client->dialog_state = 5;
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogForceClose() {
    LOG_INFO(Lib_ImeDialog, "ForceClose called (status={}, client_state={})",
             static_cast<u32>(g_ime_dlg_status),
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    const Error disconnect_ret = ImeDialogClientDisconnect(g_ime_dlg_client);

    if (g_ime_dlg_ext_keyboard_filter_active) {
        NotifyExtKeyboardFilterState(false);
    }
    g_ime_dlg_status = OrbisImeDialogStatus::None;
    g_ime_dlg_ui = ImeDialogUi();
    g_ime_dlg_state = ImeDialogState();
    g_ime_dlg_param = {};
    g_ime_dlg_extended = {};
    g_ime_dlg_has_extended = false;
    g_ime_dlg_result_committed = false;
    g_ime_dlg_ext_keyboard_filter = nullptr;
    g_ime_dlg_ext_keyboard_filter_user_id =
        Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    g_ime_dlg_ext_keyboard_filter_registered = false;
    g_ime_dlg_ext_keyboard_filter_active = false;
    g_ime_dlg_resource_id = 0;
    DestroyImeDialogClient();

    return disconnect_ret;
}

Error PS4_SYSV_ABI sceImeDialogForTestFunction() {
    return Error::INTERNAL;
}

int PS4_SYSV_ABI sceImeDialogGetCurrentStarState(s64 param_1) {
    LOG_INFO(Lib_ImeDialog, "GetCurrentStarState called (client_state={}, addr=0x{:X})",
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1, static_cast<u32>(param_1));
    if (!g_ime_dlg_client) {
        return static_cast<int>(Error::DIALOG_NOT_IN_USE);
    }
    if (param_1 == 0) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }
    if (g_ime_dlg_client->dialog_state == 4 || g_ime_dlg_client->dialog_state == 5) {
        return static_cast<int>(Error::IME_SUSPENDING);
    }
    auto* out_state = reinterpret_cast<u32*>(param_1);
    *out_state = 0;
    LOG_DEBUG(Lib_ImeDialog, "GetCurrentStarState -> 0");
    return static_cast<int>(Error::OK);
}

int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm(OrbisImePositionAndForm* posForm) {
    LOG_INFO(Lib_ImeDialog, "GetPanelPositionAndForm called (client_state={})",
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        return static_cast<int>(Error::DIALOG_NOT_IN_USE);
    }
    if (!posForm) {
        return static_cast<int>(Error::INVALID_ADDRESS);
    }
    *posForm = {};
    posForm->type = OrbisImePanelType::Dialog;
    posForm->posx = FromInternalCoord(g_ime_dlg_param.posx, g_ime_dlg_param);
    posForm->posy = FromInternalCoord(g_ime_dlg_param.posy, g_ime_dlg_param);
    posForm->horizontal_alignment = g_ime_dlg_param.horizontal_alignment;
    posForm->vertical_alignment = g_ime_dlg_param.vertical_alignment;

    OrbisImeDialogParam size_param = g_ime_dlg_param;
    size_param.option =
        static_cast<OrbisImeOption>(static_cast<u32>(size_param.option) |
                                    static_cast<u32>(OrbisImeOption::USE_OVER_2K_COORDINATES));
    u32 width = 0;
    u32 height = 0;
    if (g_ime_dlg_has_extended) {
        (void)ComputeImeDialogPanelSizeExtended(&size_param, &g_ime_dlg_extended, &width, &height,
                                                false);
    } else {
        (void)ComputeImeDialogPanelSize(&size_param, &width, &height, false);
    }

    if (!UseOver2kCoordinates(g_ime_dlg_param)) {
        posForm->width =
            static_cast<u32>(FromInternalCoord(static_cast<float>(width), g_ime_dlg_param));
        posForm->height =
            static_cast<u32>(FromInternalCoord(static_cast<float>(height), g_ime_dlg_param));
    } else {
        posForm->width = width;
        posForm->height = height;
    }
    LOG_INFO(Lib_ImeDialog, "GetPanelPositionAndForm: type={}, pos=({}, {}), size={}x{}",
             static_cast<u32>(posForm->type), posForm->posx, posForm->posy, posForm->width,
             posForm->height);
    return static_cast<int>(Error::OK);
}

Error PS4_SYSV_ABI sceImeDialogGetPanelSize(const OrbisImeDialogParam* param, u32* width,
                                            u32* height) {
    return ComputeImeDialogPanelSize(param, width, height, true);
}

static Error ComputeImeDialogPanelSize(const OrbisImeDialogParam* param, u32* width, u32* height,
                                       bool log) {
    if (log) {
        LOG_INFO(Lib_ImeDialog, "called");
    }

    if (!param) {
        return Error::INVALID_ADDRESS;
    }
    if (!width || !height) {
        return Error::INVALID_ADDRESS;
    }
    if (static_cast<u32>(param->type) > static_cast<u32>(OrbisImeType::Number)) {
        LOG_ERROR(Lib_ImeDialog, "Unknown OrbisImeType: {}", static_cast<u32>(param->type));
        return Error::INVALID_TYPE;
    }

    u32 option = static_cast<u32>(param->option);
    const u32 option_mask = GetImeDialogOptionMask();
    const u32 invalid_bits = option & (option_mask ^ 0xfffffdffU);
    if (invalid_bits != 0) {
        if (log) {
            LOG_ERROR(Lib_ImeDialog, "Invalid option: {:032b}", option);
            return Error::INVALID_OPTION;
        }
        LOG_DEBUG(Lib_ImeDialog, "Masking invalid option bits: {:032b}", invalid_bits);
        const u32 over2k_bit = static_cast<u32>(OrbisImeOption::USE_OVER_2K_COORDINATES);
        const u32 preserve_over2k = option & over2k_bit;
        option = (option & ~invalid_bits) | preserve_over2k;
    }

    const u64 lang_mask = GetImeDialogLanguageMask();
    if ((~lang_mask & static_cast<u64>(param->supported_languages)) != 0) {
        if (log) {
            LOG_ERROR(Lib_ImeDialog, "Invalid supported_languages: {:064b}",
                      static_cast<u64>(param->supported_languages));
            return Error::INVALID_SUPPORTED_LANGUAGES;
        }
        LOG_DEBUG(Lib_ImeDialog, "Masking invalid supported_languages bits");
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (param->type == OrbisImeType::Number) {
        *width = 0x172;
        u32 h = 0x1d6;
        if (sdk > 0x16fffff) {
            h = 0x20a - (sdk < 0x2000000 ? 1U : 0U);
        }
        *height = h;
    } else {
        u32 opt = option;
        if (param->type != OrbisImeType::BasicLatin) {
            if ((opt & 0xc0000004) != 4) {
                *width = 0x319;
                *height = (opt & 1) ? 0x274 : 0x210;
                goto done;
            }
        }
        *width = 0x319;
        if ((opt & 1) == 0) {
            *height = (sdk > 0x16fffff) ? 0x210 : 0x1dc;
        } else {
            *height = (sdk > 0x16fffff) ? 0x274 : 0x240;
        }
    }
done:
    if ((option & 0x4000) != 0) {
        *width <<= 1;
        *height <<= 1;
    }
    if (log) {
        LOG_DEBUG(Lib_ImeDialog, "PanelSize: type={}, option=0x{:X}, sdk=0x{:X}, size={}x{}",
                  static_cast<u32>(param->type), option, sdk, *width, *height);
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended(const OrbisImeDialogParam* param,
                                                    const OrbisImeParamExtended* extended,
                                                    u32* width, u32* height) {
    return ComputeImeDialogPanelSizeExtended(param, extended, width, height, true);
}

static Error ComputeImeDialogPanelSizeExtended(const OrbisImeDialogParam* param,
                                               const OrbisImeParamExtended* extended, u32* width,
                                               u32* height, bool log) {
    if (!param || !width || !height) {
        return Error::INVALID_ADDRESS;
    }

    if (static_cast<u32>(param->type) > static_cast<u32>(OrbisImeType::Number)) {
        return Error::INVALID_TYPE;
    }
    u32 option = static_cast<u32>(param->option);
    const u32 option_mask = GetImeDialogOptionMask();
    const u32 invalid_bits = option & (option_mask ^ 0xfffffdffU);
    if (invalid_bits != 0) {
        if (log) {
            return Error::INVALID_OPTION;
        }
        const u32 over2k_bit = static_cast<u32>(OrbisImeOption::USE_OVER_2K_COORDINATES);
        const u32 preserve_over2k = option & over2k_bit;
        option = (option & ~invalid_bits) | preserve_over2k;
    }
    const u64 lang_mask = GetImeDialogLanguageMask();
    if ((~lang_mask & static_cast<u64>(param->supported_languages)) != 0) {
        if (log) {
            return Error::INVALID_SUPPORTED_LANGUAGES;
        }
    }

    if (!extended) {
        return sceImeDialogGetPanelSize(param, width, height);
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (sdk > 0x16fffff) {
        if (!IsValidDialogExtOption(static_cast<u32>(extended->option))) {
            return Error::INVALID_EXTENDED;
        }
    }

    bool accessibility = false;
    const u32 ext_opt = static_cast<u32>(extended->option);
    if ((ext_opt & static_cast<u32>(OrbisImeExtOption::ENABLE_ACCESSIBILITY)) != 0) {
        if ((ext_opt & static_cast<u32>(OrbisImeExtOption::ACCESSIBILITY_PANEL_FORCED)) != 0) {
            accessibility = true;
        } else {
            // Emulate system accessibility setting (LLE FUN_01005a50 path).
            accessibility = Config::getImeAccessibilityEnabled();
        }
    }

    if (accessibility) {
        *width = 0x780;
        *height = 0x438;
        goto done;
    }

    const bool hide_keypanel_for_ext =
        (param->option & OrbisImeOption::EXT_KEYBOARD) != OrbisImeOption::DEFAULT &&
        (extended->option & OrbisImeExtOption::HIDE_KEYPANEL_IF_EXT_KEYBOARD) !=
            OrbisImeExtOption::DEFAULT;

    const u32 type = static_cast<u32>(param->type);
    const bool multiline = (option & 1U) != 0;

    auto use_short_url_mail_height = [&](u32 ime_type) {
        // LLE: (SVar1 & ~BASIC_LATIN) == (URL | bVar15), with bVar15 default true.
        // When bVar15 is true, this condition is false for Url/Mail; false enables short heights.
        const bool bVar15 = !Config::getImeUrlMailShortPanel();
        const u32 masked_type = ime_type & ~static_cast<u32>(OrbisImeType::BasicLatin);
        const u32 url_or_mail = static_cast<u32>(OrbisImeType::Url) | (bVar15 ? 1U : 0U);
        return masked_type == url_or_mail;
    };

    if (type == static_cast<u32>(OrbisImeType::Number)) {
        *width = 0x172;
        if (!hide_keypanel_for_ext) {
            if (sdk > 0x16fffff) {
                *height = (sdk < 0x2000000) ? 0x209 : 0x20a;
            } else {
                *height = 0x1d6;
            }
        } else {
            if (sdk < 0x1700000) {
                *height = 0x1d6;
            } else {
                *height = (sdk < 0x2000000) ? 0x65 : 0x66;
            }
        }
    } else {
        const bool basic_latin_branch =
            (type == static_cast<u32>(OrbisImeType::BasicLatin)) || ((option & 0xc0000004U) == 4U);

        if (basic_latin_branch) {
            *width = 0x319;
            if (!multiline) {
                if (!hide_keypanel_for_ext) {
                    *height = (sdk > 0x16fffff) ? 0x210 : 0x1dc;
                } else {
                    *height = (sdk < 0x1700000) ? 0x66 : 0x67;
                }
            } else {
                if (!hide_keypanel_for_ext) {
                    *height = (sdk > 0x16fffff) ? 0x274 : 0x240;
                } else {
                    *height = (sdk < 0x1700000) ? 0x10a : 0xcb;
                }
            }
        } else {
            *width = 0x319;
            if (!multiline) {
                if (hide_keypanel_for_ext) {
                    const bool short_url_mail = use_short_url_mail_height(type);
                    if (short_url_mail) {
                        *height = (sdk < 0x1700000) ? 0x66 : 0x67;
                    } else {
                        *height = (sdk < 0x1700000) ? 0xa6 : 0xa8;
                    }
                } else {
                    *height = 0x210;
                }
            } else {
                if (hide_keypanel_for_ext) {
                    const bool short_url_mail = use_short_url_mail_height(type);
                    if (short_url_mail) {
                        *height = (sdk < 0x1700000) ? 0xca : 0xcb;
                    } else {
                        *height = (sdk < 0x1700000) ? 0x10a : 0x10c;
                    }
                } else {
                    *height = 0x274;
                }
            }
        }
    }

done:
    if ((param->option & OrbisImeOption::USE_OVER_2K_COORDINATES) != OrbisImeOption::DEFAULT) {
        *width <<= 1;
        *height <<= 1;
    }

    if (log) {
        LOG_DEBUG(Lib_ImeDialog, "PanelSizeExt: type={}, option=0x{:X}, sdk=0x{:X}, size={}x{}",
                  static_cast<u32>(param->type), option, sdk, *width, *height);
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogGetResult(OrbisImeDialogResult* result) {
    LOG_INFO(Lib_ImeDialog, "GetResult called (status={}, client_state={})",
             static_cast<u32>(g_ime_dlg_status),
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        LOG_INFO(Lib_ImeDialog, "IME dialog is not running");
        return Error::DIALOG_NOT_IN_USE;
    }

    if (result == nullptr) {
        LOG_INFO(Lib_ImeDialog, "called with result (NULL)");
        return Error::INVALID_ADDRESS;
    }

    for (size_t i = 0; i < sizeof(result->reserved); ++i) {
        if (result->reserved[i] != 0) {
            LOG_INFO(Lib_ImeDialog, "result->reserved not zeroed");
            return Error::INVALID_RESERVED;
        }
    }

    if (g_ime_dlg_client && g_ime_dlg_client->dialog_state == 4) {
        return Error::DIALOG_NOT_FINISHED;
    }

    if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
        return Error::DIALOG_NOT_FINISHED;
    }

    if (g_ime_dlg_status == OrbisImeDialogStatus::Finished && g_ime_dlg_client) {
        g_ime_dlg_client->dialog_state = 5;
    }

    CommitDialogResultIfNeeded();
    result->endstatus = g_ime_dlg_result.endstatus;
    LOG_INFO(Lib_ImeDialog, "GetResult -> endstatus={}", static_cast<u32>(result->endstatus));
    return Error::OK;
}

OrbisImeDialogStatus PS4_SYSV_ABI sceImeDialogGetStatus() {
    LOG_INFO(Lib_ImeDialog, "GetStatus called (status={}, client_state={})",
             static_cast<u32>(g_ime_dlg_status),
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        LOG_DEBUG(Lib_ImeDialog, "GetStatus -> None (no client)");
        return OrbisImeDialogStatus::None;
    }

    if (g_ime_dlg_client->dialog_state == 5) {
        g_ime_dlg_status = OrbisImeDialogStatus::Finished;
    }

    if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
        g_ime_dlg_state.CallTextFilter();
    } else if (g_ime_dlg_status == OrbisImeDialogStatus::Finished) {
        CommitDialogResultIfNeeded();
    }

    if (g_ime_dlg_client) {
        if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
            g_ime_dlg_client->dialog_state = 4;
        } else if (g_ime_dlg_status == OrbisImeDialogStatus::Finished) {
            g_ime_dlg_client->dialog_state = 5;
        }
    }

    if (g_ime_dlg_status == OrbisImeDialogStatus::Running && g_ime_dlg_ext_keyboard_filter &&
        g_ime_dlg_ext_keyboard_filter_registered && !g_ime_dlg_ext_keyboard_filter_active) {
        NotifyExtKeyboardFilterState(true);
    } else if (g_ime_dlg_status != OrbisImeDialogStatus::Running &&
               g_ime_dlg_ext_keyboard_filter_active) {
        NotifyExtKeyboardFilterState(false);
    }

    LOG_DEBUG(Lib_ImeDialog, "GetStatus -> {}", static_cast<u32>(g_ime_dlg_status));
    return g_ime_dlg_status;
}

static Error SetupDialogState(OrbisImeDialogParam* param, OrbisImeParamExtended* extended,
                              bool internal) {
    if (!param) {
        return Error::INVALID_ADDRESS;
    }

    g_ime_dlg_resource_id = 0;
    g_ime_dlg_ext_keyboard_filter = nullptr;
    g_ime_dlg_ext_keyboard_filter_user_id =
        Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    g_ime_dlg_ext_keyboard_filter_registered = false;

    if (param->max_text_length == 0 || param->max_text_length > ORBIS_IME_MAX_TEXT_LENGTH) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid max_text_length={}",
                  param->max_text_length);
        return Error::INVALID_MAX_TEXT_LENGTH;
    }

    g_ime_dlg_result = {};
    g_ime_dlg_result_committed = false;

    OrbisImeDialogParam local_param = *param;
    const u32 sdk = GetCompiledSdkVersion();
    if (!internal && sdk < 0x1500000) {
        local_param.option =
            static_cast<OrbisImeOption>(static_cast<u32>(local_param.option) & 0x26ffffffU);
    }
    local_param.posx = ClampInternalX(ToInternalCoord(local_param.posx, local_param));
    local_param.posy = ClampInternalY(ToInternalCoord(local_param.posy, local_param));
    g_ime_dlg_param = local_param;

    OrbisImeParamExtended* ext_ptr = nullptr;
    if (extended) {
        g_ime_dlg_extended = *extended;
        g_ime_dlg_has_extended = true;
        ext_ptr = &g_ime_dlg_extended;
        if (extended->ext_keyboard_filter) {
            g_ime_dlg_ext_keyboard_filter = extended->ext_keyboard_filter;
            g_ime_dlg_ext_keyboard_filter_user_id = param->user_id;
        }
    } else {
        g_ime_dlg_extended = {};
        g_ime_dlg_has_extended = false;
    }

    g_ime_dlg_state = ImeDialogState(&g_ime_dlg_param, ext_ptr);
    g_ime_dlg_status = OrbisImeDialogStatus::Running;
    if (g_ime_dlg_client) {
        g_ime_dlg_client->dialog_state = 4;
    }
    g_ime_dlg_ui = ImeDialogUi(&g_ime_dlg_state, &g_ime_dlg_status, &g_ime_dlg_result);

    LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: successful, status now=Running");
    return Error::OK;
}

static Error InitDialogCommon(OrbisImeDialogParam* param, OrbisImeParamExtended* extended,
                              bool internal) {
    if (param == nullptr) {
        LOG_ERROR(Lib_ImeDialog, "param is null");
        return Error::INVALID_ADDRESS;
    }
    if (g_ime_dlg_client != nullptr || g_ime_dlg_status != OrbisImeDialogStatus::None) {
        LOG_ERROR(Lib_ImeDialog, "busy (status={})", (u32)g_ime_dlg_status);
        return Error::BUSY;
    }

    const Error vret = ValidateImeDialogParam(param, extended, internal);
    if (vret != Error::OK) {
        return vret;
    }

    return SetupDialogState(param, extended, internal);
}

Error PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, "Init called, param={}, extended={}", static_cast<void*>(param),
             static_cast<void*>(extended));

    if (g_ime_dlg_client != nullptr || g_ime_dlg_status != OrbisImeDialogStatus::None) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: busy");
        return Error::BUSY;
    }

    if (param != nullptr) {
        LOG_DEBUG(Lib_ImeDialog, "param->user_id: {}", static_cast<u32>(param->user_id));
        LOG_DEBUG(Lib_ImeDialog, "param->type: {}", static_cast<u32>(param->type));
        LOG_DEBUG(Lib_ImeDialog, "param->supported_languages: {:064b}",
                  static_cast<u64>(param->supported_languages));
        LOG_DEBUG(Lib_ImeDialog, "param->enter_label: {}", static_cast<u32>(param->enter_label));
        LOG_DEBUG(Lib_ImeDialog, "param->input_method: {}", static_cast<u32>(param->input_method));
        LOG_DEBUG(Lib_ImeDialog, "param->filter: {}", (void*)param->filter);
        LOG_DEBUG(Lib_ImeDialog, "param->option: {:032b}", static_cast<u32>(param->option));
        LOG_DEBUG(Lib_ImeDialog,
                  "  opt flags: multiline={}, password={}, ext_kbd={}, fixed_pos={}, over2k={}",
                  True(param->option & OrbisImeOption::MULTILINE),
                  True(param->option & OrbisImeOption::PASSWORD),
                  True(param->option & OrbisImeOption::EXT_KEYBOARD),
                  True(param->option & OrbisImeOption::FIXED_POSITION),
                  True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES));
        LOG_DEBUG(Lib_ImeDialog, "param->max_text_length: {}", param->max_text_length);
        LOG_DEBUG(Lib_ImeDialog, "param->input_text_buffer: {}", (void*)param->input_text_buffer);
        LOG_DEBUG(Lib_ImeDialog, "param->posx: {}", param->posx);
        LOG_DEBUG(Lib_ImeDialog, "param->posy: {}", param->posy);
        LOG_DEBUG(Lib_ImeDialog, "param->horizontal_alignment: {}",
                  static_cast<u32>(param->horizontal_alignment));
        LOG_DEBUG(Lib_ImeDialog, "param->vertical_alignment: {}",
                  static_cast<u32>(param->vertical_alignment));
        LOG_DEBUG(Lib_ImeDialog, "param->placeholder: {}",
                  param->placeholder ? "<non-null>" : "NULL");
        LOG_DEBUG(Lib_ImeDialog, "param.title: {}", param->title ? "<non-null>" : "NULL");
    }

    if (extended) {
        LOG_DEBUG(Lib_ImeDialog, "extended->option: {:032b}", static_cast<u32>(extended->option));
        LOG_DEBUG(Lib_ImeDialog, "extended->color_base: {{{},{},{},{}}}", extended->color_base.r,
                  extended->color_base.g, extended->color_base.b, extended->color_base.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_line: {{{},{},{},{}}}", extended->color_line.r,
                  extended->color_line.g, extended->color_line.b, extended->color_line.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_text_field: {{{},{},{},{}}}",
                  extended->color_text_field.r, extended->color_text_field.g,
                  extended->color_text_field.b, extended->color_text_field.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_preedit: {{{},{},{},{}}}",
                  extended->color_preedit.r, extended->color_preedit.g, extended->color_preedit.b,
                  extended->color_preedit.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_default: {{{},{},{},{}}}",
                  extended->color_button_default.r, extended->color_button_default.g,
                  extended->color_button_default.b, extended->color_button_default.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_function: {{{},{},{},{}}}",
                  extended->color_button_function.r, extended->color_button_function.g,
                  extended->color_button_function.b, extended->color_button_function.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_symbol: {{{},{},{},{}}}",
                  extended->color_button_symbol.r, extended->color_button_symbol.g,
                  extended->color_button_symbol.b, extended->color_button_symbol.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_text: {{{},{},{},{}}}", extended->color_text.r,
                  extended->color_text.g, extended->color_text.b, extended->color_text.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_special: {{{},{},{},{}}}",
                  extended->color_special.r, extended->color_special.g, extended->color_special.b,
                  extended->color_special.a);

        LOG_DEBUG(Lib_ImeDialog, "extended->priority: {:032b}",
                  static_cast<u32>(extended->priority));
        LOG_DEBUG(Lib_ImeDialog, "extended->disable_device: {:032b}",
                  static_cast<u32>(extended->disable_device));
        LOG_DEBUG(Lib_ImeDialog, "extended->ext_keyboard_mode: {:032b}",
                  static_cast<u32>(extended->ext_keyboard_mode));
        LOG_DEBUG(Lib_ImeDialog, "extended->additional_dictionary_path: {}",
                  extended->additional_dictionary_path ? extended->additional_dictionary_path
                                                       : "NULL");
    } else {
        LOG_DEBUG(Lib_ImeDialog, "extended: NULL");
    }

    const u32 sdk = GetCompiledSdkVersion();
    if (!param) {
        return Error::INVALID_ADDRESS;
    }

    const bool legacy_sdk = sdk < 0x1500000;
    const Error validate_ret = ValidateImeDialogParam(param, extended, legacy_sdk);
    if (validate_ret != Error::OK) {
        return validate_ret;
    }

    OrbisImeDialogParam local_param{};
    OrbisImeDialogParam* param_ptr = param;
    if (legacy_sdk) {
        local_param = *param;
        local_param.option =
            static_cast<OrbisImeOption>(static_cast<u32>(local_param.option) & 0x26ffffffU);
        param_ptr = &local_param;
    }

    u32 user_flags = 0x11;
    s64 user_value = 0;
    ComputeUserFlags(param->user_id, &user_flags, &user_value);

    g_ime_dlg_resource_id = 0;
    g_ime_dlg_client = CreateImeDialogClient();
    if (!g_ime_dlg_client) {
        return Error::NO_MEMORY;
    }
    InitImeDialogClient(g_ime_dlg_client);

    constexpr u32 kImeDialogServiceNotActive = 0x80bc07baU;
    constexpr u32 kImeDialogServiceRetry = 0x80bc07b1U;

    const int connect_ret =
        ImeDialogClientConnect(g_ime_dlg_client, 2, g_ime_dlg_resource_id, param->user_id, false);
    if ((static_cast<u32>(connect_ret) & 0xfffffffeU) == kImeDialogServiceNotActive) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: connect returned NOT_ACTIVE (0x{:X})",
                 static_cast<u32>(connect_ret));
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        return Error::NOT_ACTIVE;
    }
    if (connect_ret == static_cast<int>(Error::INVALID_USER_ID)) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: connect invalid user id");
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        return Error::INVALID_USER_ID;
    }
    if (connect_ret == static_cast<int>(Error::NOT_ACTIVE) ||
        static_cast<u32>(connect_ret) == kImeDialogServiceRetry) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: connect fallback (0x{:X})",
                 static_cast<u32>(connect_ret));
        const int fallback_ret = ImeDialogClientStartFallback(g_ime_dlg_client, param_ptr, extended,
                                                              user_flags, user_value);
        if (fallback_ret < 0) {
            ImeDialogClientShutdown(g_ime_dlg_client);
            DestroyImeDialogClient();
            return Error::NOT_ACTIVE;
        }
        const Error setup_ret = SetupDialogState(param_ptr, extended, false);
        if (setup_ret != Error::OK) {
            ImeDialogClientShutdown(g_ime_dlg_client);
            DestroyImeDialogClient();
            return setup_ret;
        }
        return static_cast<Error>(fallback_ret);
    }
    if (connect_ret != 0) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: connect failed (0x{:X})",
                 static_cast<u32>(connect_ret));
        ImeDialogClientShutdown(g_ime_dlg_client);
        DestroyImeDialogClient();
        return Error::CONNECTION_FAILED;
    }

    int start_ret = ImeDialogClientStart(g_ime_dlg_client, param_ptr, extended, user_flags,
                                         user_value, 0, -1, legacy_sdk);
    if (start_ret == 0) {
        const Error setup_ret = SetupDialogState(param_ptr, extended, false);
        if (setup_ret != Error::OK) {
            ImeDialogClientShutdown(g_ime_dlg_client);
            DestroyImeDialogClient();
            return setup_ret;
        }

        if (extended && extended->ext_keyboard_filter) {
            const int reg_ret =
                RegisterExtKeyboardFilter(param->user_id, extended->ext_keyboard_filter);
            if (reg_ret >= 0) {
                g_ime_dlg_ext_keyboard_filter_registered = true;
                LOG_DEBUG(Lib_ImeDialog, "ext_keyboard_filter registered (user_id={})",
                          static_cast<u32>(g_ime_dlg_ext_keyboard_filter_user_id));
            } else {
                g_ime_dlg_ext_keyboard_filter = nullptr;
                g_ime_dlg_ext_keyboard_filter_user_id =
                    Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
            }
        }

        return Error::OK;
    }

    if (static_cast<u32>(start_ret) == kImeDialogServiceRetry) {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: start fallback (0x{:X})",
                 static_cast<u32>(start_ret));
        const int fallback_ret = ImeDialogClientStartFallback(g_ime_dlg_client, param_ptr, extended,
                                                              user_flags, user_value);
        if (fallback_ret >= 0) {
            const Error setup_ret = SetupDialogState(param_ptr, extended, false);
            if (setup_ret != Error::OK) {
                ImeDialogClientShutdown(g_ime_dlg_client);
                DestroyImeDialogClient();
                return setup_ret;
            }
            return static_cast<Error>(fallback_ret);
        }
        start_ret = static_cast<int>(Error::NOT_ACTIVE);
    } else {
        LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: start failed (0x{:X})",
                 static_cast<u32>(start_ret));
        ImeDialogClientShutdown(g_ime_dlg_client);
    }

    DestroyImeDialogClient();
    return static_cast<Error>(start_ret);
}

int PS4_SYSV_ABI sceImeDialogInitInternal(OrbisImeDialogParam* param,
                                          OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, "InitInternal called, param={}, extended={}", static_cast<void*>(param),
             static_cast<void*>(extended));
    u32 user_flags = 0x11;
    s64 user_value = 0;
    if (param) {
        ComputeUserFlags(param->user_id, &user_flags, &user_value);
    }
    return InitDialogInternalWithClient(param, extended, user_flags, user_value, 0, 0, 0xffffffff,
                                        true);
}

int PS4_SYSV_ABI sceImeDialogInitInternal2(int* param_1, u32* param_2, u32 param_3, u64 param_4) {
    LOG_INFO(Lib_ImeDialog, "InitInternal2 called (param={}, extended={}, flags=0x{:X})",
             static_cast<void*>(param_1), static_cast<void*>(param_2), param_3);
    auto* param = reinterpret_cast<OrbisImeDialogParam*>(param_1);
    auto* extended = reinterpret_cast<OrbisImeParamExtended*>(param_2);
    return InitDialogInternalWithClient(param, extended, param_3, static_cast<s64>(param_4), 0, 0,
                                        0xffffffff, false);
}

int PS4_SYSV_ABI sceImeDialogInitInternal3(int* param_1, u32* param_2, u32 param_3, u64 param_4,
                                           u32 param_5, u32 param_6) {
    LOG_INFO(Lib_ImeDialog,
             "InitInternal3 called (param={}, extended={}, flags=0x{:X}, resource_id=0x{:X})",
             static_cast<void*>(param_1), static_cast<void*>(param_2), param_3, param_5);
    auto* param = reinterpret_cast<OrbisImeDialogParam*>(param_1);
    auto* extended = reinterpret_cast<OrbisImeParamExtended*>(param_2);
    return InitDialogInternalWithClient(param, extended, param_3, static_cast<s64>(param_4),
                                        param_5, param_5, param_6, false);
}

int PS4_SYSV_ABI sceImeDialogSetPanelPosition(s32 posx, s32 posy) {
    LOG_INFO(Lib_ImeDialog, "SetPanelPosition called (client_state={}, pos=({}, {}))",
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1, posx, posy);
    if (!g_ime_dlg_client) {
        return static_cast<int>(Error::DIALOG_NOT_IN_USE);
    }
    if (g_ime_dlg_status == OrbisImeDialogStatus::Running ||
        g_ime_dlg_status == OrbisImeDialogStatus::Finished || g_ime_dlg_client->dialog_state == 4 ||
        g_ime_dlg_client->dialog_state == 5) {
        return static_cast<int>(Error::IME_SUSPENDING);
    }
    g_ime_dlg_param.posx = ClampInternalX(ToInternalCoord(static_cast<f32>(posx), g_ime_dlg_param));
    g_ime_dlg_param.posy = ClampInternalY(ToInternalCoord(static_cast<f32>(posy), g_ime_dlg_param));
    LOG_DEBUG(Lib_ImeDialog, "SetPanelPosition: pos=({}, {})", g_ime_dlg_param.posx,
              g_ime_dlg_param.posy);
    return static_cast<int>(Error::OK);
}

Error PS4_SYSV_ABI sceImeDialogTerm() {
    LOG_INFO(Lib_ImeDialog, "Term called (status={}, client_state={})",
             static_cast<u32>(g_ime_dlg_status),
             g_ime_dlg_client ? g_ime_dlg_client->dialog_state : -1);
    if (!g_ime_dlg_client) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    LOG_DEBUG(Lib_ImeDialog, "Term: status={}, endstatus={}", static_cast<u32>(g_ime_dlg_status),
              static_cast<u32>(g_ime_dlg_result.endstatus));
    CommitDialogResultIfNeeded();
    (void)ImeDialogClientDisconnect(g_ime_dlg_client);
    if (g_ime_dlg_ext_keyboard_filter_active) {
        NotifyExtKeyboardFilterState(false);
    }
    g_ime_dlg_status = OrbisImeDialogStatus::None;
    g_ime_dlg_ui = ImeDialogUi();
    g_ime_dlg_state = ImeDialogState();
    g_ime_dlg_param = {};
    g_ime_dlg_extended = {};
    g_ime_dlg_has_extended = false;
    g_ime_dlg_result_committed = false;
    g_ime_dlg_ext_keyboard_filter = nullptr;
    g_ime_dlg_ext_keyboard_filter_user_id =
        Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    g_ime_dlg_ext_keyboard_filter_registered = false;
    g_ime_dlg_ext_keyboard_filter_active = false;
    g_ime_dlg_resource_id = 0;
    DestroyImeDialogClient();

    return Error::OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oBmw4xrmfKs", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogAbort);
    LIB_FUNCTION("bX4H+sxPI-o", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogForceClose);
    LIB_FUNCTION("UFcyYDf+e88", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogForTestFunction);
    LIB_FUNCTION("fy6ntM25pEc", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetCurrentStarState);
    LIB_FUNCTION("8jqzzPioYl8", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetPanelPositionAndForm);
    LIB_FUNCTION("wqsJvRXwl58", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetPanelSize);
    LIB_FUNCTION("CRD+jSErEJQ", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetPanelSizeExtended);
    LIB_FUNCTION("x01jxu+vxlc", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetResult);
    LIB_FUNCTION("IADmD4tScBY", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetStatus);
    LIB_FUNCTION("NUeBrN7hzf0", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInit);
    LIB_FUNCTION("KR6QDasuKco", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal);
    LIB_FUNCTION("oe92cnJQ9HE", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal2);
    LIB_FUNCTION("IoKIpNf9EK0", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal3);
    LIB_FUNCTION("-2WqB87KKGg", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogSetPanelPosition);
    LIB_FUNCTION("gyTyVn+bXMw", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogTerm);
};

} // namespace Libraries::ImeDialog
