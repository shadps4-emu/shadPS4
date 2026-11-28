// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/linker.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SysUtil {

struct SysUtilNotificationParams {
    char param[3][0xb4];
};
static_assert(sizeof(SysUtilNotificationParams) == 0x21c);

struct SysUtilSystemNotificationParam {
    s32 type;
    s32 priority;
    s32 unk08;
    s32 msg_id;
    s32 target_id;
    u8 use_icon_image_uri;
    char icon_uri[0x803];
    u8 data[0x220];
    u32 magic;
};
static_assert(sizeof(SysUtilSystemNotificationParam) == 0xa3c);
static_assert(offsetof(SysUtilSystemNotificationParam, magic) == 0xa38);

s32 PS4_SYSV_ABI sceSysUtilSendNotificationRequest(const char* message, s32 blocking);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotification(s32 messageType);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserId(s32 messageType, s32 userId);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithDeviceId(s32 messageType, u64 deviceId);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithDeviceIdRelatedToUser(s32 messageType,
                                                                           s32 userId,
                                                                           u64 deviceId);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithText(s32 messageType, const char* message);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithTextRelatedToUser(s32 messageType, s32 userId,
                                                                       const char* message);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithErrorCode(s32 messageType, s32 errorCode);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppId(s32 messageType, s32 appId);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppName(s32 messageType, const char* appName);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppInfo(s32 messageType, const char* param1,
                                                             const char* param2);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithAppNameRelatedToUser(s32 messageType,
                                                                          s32 userId,
                                                                          const char* appName);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithParams(s32 messageType,
                                                            SysUtilNotificationParams params);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserName(s32 messageType,
                                                              const char* userName);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationWithUserNameInfo(s32 messageType,
                                                                  const char* userName,
                                                                  const char* info);
s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotification(s32 messageType, s32 targetId);
s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithDeviceId(s32 messageType,
                                                                        s32 targetId, u64 deviceId);
s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithUserName(s32 messageType,
                                                                        s32 targetId,
                                                                        const char* userName);
s32 PS4_SYSV_ABI sceSysUtilSendAddressingSystemNotificationWithUserId(s32 messageType, s32 targetId,
                                                                      s32 userId);
s32 PS4_SYSV_ABI sceSysUtilSendTrcCheckNotificationRequest(const char* message, s32 blocking);
s32 PS4_SYSV_ABI sceSysUtilSendNpDebugNotificationRequest(const char* message, s32 blocking);
s32 PS4_SYSV_ABI sceSysUtilSendWebDebugNotificationRequest(const char* message, s32 blocking);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotificationParamInit(SysUtilSystemNotificationParam* param,
                                                           s32 type);
s32 PS4_SYSV_ABI sceSysUtilSendSystemNotification2(const SysUtilSystemNotificationParam* param);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::SysUtil
