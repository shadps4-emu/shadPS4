// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/logging/log.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/sysutil.h"

namespace Libraries::SysUtil {

namespace {

constexpr s32 ORBIS_SYSUTIL_ERROR_INVALID_VALUE = 0x81670000;
constexpr u32 kNotificationParamMagic = 0xd59c4b6e;
constexpr size_t kShortMsgSlotSize = 0xb4;

void CopyBoundedString(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    const size_t len = strnlen(src, dst_size - 1);
    std::memcpy(dst, src, len);
    dst[len] = '\0';
}

Libraries::Kernel::OrbisNotificationRequest MakeBaseRequest(s32 type, s32 msg_id,
                                                            s32 target_id = -1) {
    Libraries::Kernel::OrbisNotificationRequest req{};
    req.type = type;
    req.msg_id = msg_id;
    req.target_id = target_id;
    return req;
}

s32 SendRequest(const Libraries::Kernel::OrbisNotificationRequest& req, s32 blocking = 0) {
    return Libraries::Kernel::sceKernelSendNotificationRequest(0, &req, sizeof(req), blocking);
}

} // namespace

s32 PS4_SYSV_ABI sceSysUtilSendNotificationRequest(const char* message, s32 blocking) {
    auto req = MakeBaseRequest(0, 0, -1);
    req.req_id = -1;
    CopyBoundedString(req.message, sizeof(req.message), message);
    return SendRequest(req, blocking);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotification(s32 messageType) {
    return SendRequest(MakeBaseRequest(1, messageType));
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserId(s32 messageType, s32 userId) {
    auto req = MakeBaseRequest(2, messageType);
    req.user_id = userId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithDeviceId(s32 messageType, u64 deviceId) {
    auto req = MakeBaseRequest(3, messageType);
    req.device_id = deviceId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser(s32 messageType,
                                                                           s32 userId,
                                                                           u64 deviceId) {
    auto req = MakeBaseRequest(4, messageType);
    req.user_id = userId;
    req.device_id = deviceId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithText(s32 messageType, const char* message) {
    auto req = MakeBaseRequest(5, messageType);
    CopyBoundedString(req.message, kShortMsgSlotSize, message);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithTextRelatedToUser(s32 messageType, s32 userId,
                                                                       const char* message) {
    auto req = MakeBaseRequest(6, messageType);
    req.user_id = userId;
    CopyBoundedString(req.message, kShortMsgSlotSize, message);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithErrorCode(s32 messageType, s32 errorCode) {
    auto req = MakeBaseRequest(7, messageType);
    req.error_num = errorCode;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppId(s32 messageType, s32 appId) {
    auto req = MakeBaseRequest(8, messageType);
    req.req_id = -1;
    req.app_id = appId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppName(s32 messageType, const char* appName) {
    auto req = MakeBaseRequest(9, messageType);
    CopyBoundedString(req.message, kShortMsgSlotSize, appName);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppInfo(s32 messageType, const char* param1,
                                                             const char* param2) {
    auto req = MakeBaseRequest(9, messageType);
    CopyBoundedString(req.message, kShortMsgSlotSize, param1);
    CopyBoundedString(req.message + kShortMsgSlotSize, kShortMsgSlotSize, param2);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppNameRelatedToUser(s32 messageType,
                                                                          s32 userId,
                                                                          const char* appName) {
    auto req = MakeBaseRequest(10, messageType);
    req.user_id = userId;
    CopyBoundedString(req.message, kShortMsgSlotSize, appName);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithParams(s32 messageType,
                                                            SysUtilNotificationParams params) {
    auto req = MakeBaseRequest(11, messageType);
    for (size_t i = 0; i < 3; ++i) {
        CopyBoundedString(req.message + i * kShortMsgSlotSize, kShortMsgSlotSize, params.param[i]);
    }
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserName(s32 messageType,
                                                              const char* userName) {
    auto req = MakeBaseRequest(12, messageType);
    CopyBoundedString(req.message, kShortMsgSlotSize, userName);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserNameInfo(s32 messageType,
                                                                  const char* userName,
                                                                  const char* info) {
    auto req = MakeBaseRequest(13, messageType);
    CopyBoundedString(req.message, kShortMsgSlotSize, userName);
    CopyBoundedString(req.message + kShortMsgSlotSize, kShortMsgSlotSize, info);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotification(s32 messageType, s32 targetId) {
    return SendRequest(MakeBaseRequest(14, messageType, targetId));
}

s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithDeviceId(s32 messageType,
                                                                        s32 targetId,
                                                                        u64 deviceId) {
    auto req = MakeBaseRequest(15, messageType, targetId);
    req.device_id = deviceId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithUserName(s32 messageType,
                                                                        s32 targetId,
                                                                        const char* userName) {
    auto req = MakeBaseRequest(16, messageType, targetId);
    CopyBoundedString(req.message, kShortMsgSlotSize, userName);
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithUserId(s32 messageType, s32 targetId,
                                                                      s32 userId) {
    auto req = MakeBaseRequest(17, messageType, targetId);
    req.user_id = userId;
    return SendRequest(req);
}

s32 PS4_SYSV_ABI sceSysUtilSendTrcCheckNotificationRequest(const char* message, s32 blocking) {
    LOG_DEBUG(Lib_SystemService, "sceSysUtilSendTrcCheckNotificationRequest: msg='{}', blocking={}",
              message ? message : "(null)", blocking);
    return 0;
}

s32 PS4_SYSV_ABI sceSysUtilSendNpDebugNotificationRequest(const char* message, s32 blocking) {
    LOG_DEBUG(Lib_SystemService, "sceSysUtilSendNpDebugNotificationRequest: msg='{}', blocking={}",
              message ? message : "(null)", blocking);
    return 0;
}

s32 PS4_SYSV_ABI sceSysUtilSendWebDebugNotificationRequest(const char* message, s32 blocking) {
    LOG_DEBUG(Lib_SystemService, "sceSysUtilSendWebDebugNotificationRequest: msg='{}', blocking={}",
              message ? message : "(null)", blocking);
    return 0;
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationParamInit(SysUtilSystemNotificationParam* param,
                                                           s32 type) {
    if (!param) {
        return ORBIS_SYSUTIL_ERROR_INVALID_VALUE;
    }

    std::memset(reinterpret_cast<u8*>(param) + offsetof(SysUtilSystemNotificationParam, target_id),
                0,
                sizeof(SysUtilSystemNotificationParam) -
                    offsetof(SysUtilSystemNotificationParam, target_id));
    param->type = type;
    param->priority = 0;
    param->unk08 = 0;
    param->msg_id = -1;
    param->target_id = -1;
    param->use_icon_image_uri = 0;

    if (type < 0 || type > 0xd) {
        return ORBIS_SYSUTIL_ERROR_INVALID_VALUE;
    }

    auto set_u32 = [&](size_t off, s32 val) { std::memcpy(param->data + off, &val, sizeof(val)); };
    auto set_u64 = [&](size_t off, u64 val) { std::memcpy(param->data + off, &val, sizeof(val)); };

    switch (type) {
    case 1:
    case 5:
    case 8:
        set_u32(0, -1);
        break;
    case 2:
        set_u64(0, static_cast<u64>(-1));
        break;
    case 3:
        set_u32(0, -1);
        set_u64(8, static_cast<u64>(-1));
        break;
    default:
        break;
    }

    param->magic = kNotificationParamMagic;
    return 0;
}

s32 PS4_SYSV_ABI sceSysUtilSendSystemNotification2(const SysUtilSystemNotificationParam* param) {
    if (!param || param->magic != kNotificationParamMagic || param->type < 0 || param->type > 0xd) {
        return ORBIS_SYSUTIL_ERROR_INVALID_VALUE;
    }

    Libraries::Kernel::OrbisNotificationRequest req{};
    req.priority = param->priority;
    req.msg_id = param->msg_id;
    req.target_id = param->target_id;
    req.unk28 = param->unk08;
    req.use_icon_image_uri = param->use_icon_image_uri;
    if (req.use_icon_image_uri != 0) {
        CopyBoundedString(req.icon_uri, sizeof(req.icon_uri), param->icon_uri);
    }

    auto read_s32 = [&](size_t off) {
        s32 val = 0;
        std::memcpy(&val, param->data + off, sizeof(val));
        return val;
    };
    auto read_u64 = [&](size_t off) {
        u64 val = 0;
        std::memcpy(&val, param->data + off, sizeof(val));
        return val;
    };
    auto read_str = [&](size_t off) { return reinterpret_cast<const char*>(param->data + off); };

    switch (param->type) {
    case 0:
        req.type = 1;
        break;
    case 1:
        req.type = 2;
        req.user_id = read_s32(0);
        break;
    case 2:
        req.type = 3;
        req.device_id = read_u64(0);
        break;
    case 3:
        req.type = 4;
        req.user_id = read_s32(0);
        req.device_id = read_u64(8);
        break;
    case 4:
        req.type = 5;
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(0));
        break;
    case 5:
        req.type = 6;
        req.user_id = read_s32(0);
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(4));
        break;
    case 6:
        req.type = 8;
        req.req_id = -1;
        req.app_id = read_s32(0);
        break;
    case 7:
        req.type = 9;
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(0));
        break;
    case 8:
        req.type = 10;
        req.user_id = read_s32(0);
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(4));
        break;
    case 9:
        req.type = 9;
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(0));
        CopyBoundedString(req.message + kShortMsgSlotSize, kShortMsgSlotSize,
                          read_str(kShortMsgSlotSize));
        break;
    case 10:
        req.type = 7;
        req.error_num = read_s32(0);
        break;
    case 11:
        req.type = 11;
        for (size_t i = 0; i < 3; ++i) {
            CopyBoundedString(req.message + i * kShortMsgSlotSize, kShortMsgSlotSize,
                              read_str(i * kShortMsgSlotSize));
        }
        break;
    case 12:
        req.type = 12;
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(0));
        break;
    case 13:
        req.type = 13;
        CopyBoundedString(req.message, kShortMsgSlotSize, read_str(0));
        CopyBoundedString(req.message + kShortMsgSlotSize, kShortMsgSlotSize,
                          read_str(kShortMsgSlotSize));
        break;
    default:
        return ORBIS_SYSUTIL_ERROR_INVALID_VALUE;
    }

    return SendRequest(req);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("jTNAhVZ-5Nc", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotification);
    LIB_FUNCTION("YbAfwT8fO7Q", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithUserId);
    LIB_FUNCTION("SxYUo6o6mjo", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithDeviceId);
    LIB_FUNCTION("ISeIA7PBwyc", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser);
    LIB_FUNCTION("Btgtlzfk9EQ", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithText);
    LIB_FUNCTION("HJyZ8CZGVoY", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithTextRelatedToUser);
    LIB_FUNCTION("iSlI1LKy7Bg", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithErrorCode);
    LIB_FUNCTION("KQxuDpbldZs", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithAppId);
    LIB_FUNCTION("N4AToF3po08", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithAppName);
    LIB_FUNCTION("3vHcGnZpiN8", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithAppInfo);
    LIB_FUNCTION("+RZFlwV4k3I", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithAppNameRelatedToUser);
    LIB_FUNCTION("AodT6gE5Qpk", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithParams);
    LIB_FUNCTION("DC2NSz1IDyk", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithUserName);
    LIB_FUNCTION("Kl9aRhwtXBM", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationWithUserNameInfo);
    LIB_FUNCTION("TnvxGzkFmbo", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendAddressingSystemNotification);
    LIB_FUNCTION("yKfATNkQZdc", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendAddressingSystemNotificationWithDeviceId);
    LIB_FUNCTION("VHGBWkTe0Jg", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendAddressingSystemNotificationWithUserName);
    LIB_FUNCTION("BgOJY0UvOQ4", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendAddressingSystemNotificationWithUserId);
    LIB_FUNCTION("AAk6w60xP3A", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendNotificationRequest);
    LIB_FUNCTION("-0FdPC78x7U", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendTrcCheckNotificationRequest);
    LIB_FUNCTION("Eu7QNB9x1r0", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendNpDebugNotificationRequest);
    LIB_FUNCTION("V+TVN7J5JbA", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendWebDebugNotificationRequest);
    LIB_FUNCTION("-+7YODtoVLM", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotificationParamInit);
    LIB_FUNCTION("f21DEbadT98", "libSceSysUtil", 1, "libSceSysUtil",
                 sceSysUtilSendSystemNotification2);
}

} // namespace Libraries::SysUtil
