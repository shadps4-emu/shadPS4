// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpMatching2 {

struct PeerInfo {
    u32 addr = 0;
    u16 port = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    s32 conn_id = 0;
    s32 status = 0;
    Libraries::Np::OrbisNpOnlineId online_id{};
    bool sent_established = false;
    u32 ping_us = 0;
};

struct MemberBinCache {
    OrbisNpMatching2AttributeId id = 0;
    u64 update_date = 0;
    std::vector<u8> data;
};

struct MemberCache {
    Libraries::Np::OrbisNpId np_id{};
    Libraries::Np::OrbisNpAccountId account_id = 0;
    Libraries::Np::OrbisNpPlatformType platform = Libraries::Np::OrbisNpPlatformType::None;
    u64 join_date = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    OrbisNpMatching2TeamId team_id = 0;
    OrbisNpMatching2RoomGroupId group_id = 0;
    OrbisNpMatching2NatType nat_type = 0;
    OrbisNpMatching2Flags flag_attr = 0;
    u32 addr = 0;
    u16 port = 0;
    std::map<OrbisNpMatching2AttributeId, MemberBinCache> bins;
};

struct RoomCache {
    u32 num_slots = 0;
    u64 mask_password = 0;
    OrbisNpMatching2SignalingType signaling_type = ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH;
    OrbisNpMatching2RoomMemberId signaling_main_member = 0;
    OrbisNpMatching2ServerId server_id = 0;
    OrbisNpMatching2WorldId world_id = 0;
    OrbisNpMatching2LobbyId lobby_id = 0;
    OrbisNpMatching2RoomId room_id = 0;
    u16 max_slot = 0;
    u16 public_slots = 0;
    u16 private_slots = 0;
    u16 open_public_slots = 0;
    u16 open_private_slots = 0;
    u64 passwd_slot_mask = 0;
    u64 joined_slot_mask = 0;
    OrbisNpMatching2Flags flags = 0;
    std::vector<OrbisNpMatching2RoomBinAttrInternal> bin_attrs_internal;
    std::vector<std::vector<u8>> bin_buffers;
    std::map<OrbisNpMatching2RoomGroupId, OrbisNpMatching2RoomGroup> groups;
    std::map<OrbisNpMatching2RoomMemberId, MemberCache> members;
    bool owner = false;
};

struct Matching2ContextCache {
    mutable std::recursive_mutex mutex;
    std::map<OrbisNpMatching2RoomMemberId, PeerInfo> peers;
    std::map<OrbisNpMatching2RoomId, RoomCache> rooms;

    void Reset() {
        peers.clear();
        rooms.clear();
    }
};

} // namespace Libraries::Np::NpMatching2
