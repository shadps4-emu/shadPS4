// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"

namespace Libraries::Np::NpWebApi2 {

// Forward declarations to prevent compile issues
class PushEventHandle;
class PushEventFilter;
class PushEventCallback;
class PushEventPushContext;
class PushEventPushContextCallback;

class PushEventHandle {
public:
    PushEventHandle(s32 new_id) : id(new_id) {}

private:
    s32 id{};
    u32 timeout{};
    s32 user_count{};
    bool aborted{};
    bool deleting{};
};

class PushEventFilter {};

class PushEventCallback {};

class PushEventPushContext {};

class PushEventPushContextCallback {};

}; // namespace Libraries::Np::NpWebApi2