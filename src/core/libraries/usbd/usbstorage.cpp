// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "usbstorage.h"

namespace Libraries::UsbStorage {

// PS4 libSceUsbStorage / auto_mounter error codes (module 0x80f4)
constexpr s32 ORBIS_AUTO_MOUNTER_ERROR_INVAL = static_cast<s32>(0x80f40002);
constexpr s32 ORBIS_AUTO_MOUNTER_ERROR_NOENT = static_cast<s32>(0x80f40004);
constexpr s32 ORBIS_AUTO_MOUNTER_ERROR_NOTSUP = static_cast<s32>(0x80f40005);

// Flags for OrbisUsbStorageDeviceInfo additional_features
constexpr u64 ORBIS_USB_STORAGE_FEATURE_READ_ONLY = 1ull << 4;
constexpr u64 ORBIS_USB_STORAGE_FEATURE_FAT = 1ull << 6;
constexpr u64 ORBIS_USB_STORAGE_FEATURE_EXFAT = 1ull << 7;

struct OrbisUsbStorageDeviceInfo {
    u32 device_id;           // +0x00
    u8 pad0[12];             // +0x04
    u16 vendor_id;           // +0x10
    u16 product_id;          // +0x12
    u16 bcd_device;          // +0x14
    char vendor[256];        // +0x16
    u8 pad1[2];              // +0x116
    u64 vendor_len;          // +0x118
    char product[256];       // +0x120
    u64 product_len;         // +0x220
    char serial_number[256]; // +0x228
    u64 serial_number_len;   // +0x328
    u64 additional_features; // +0x330
};
static_assert(offsetof(OrbisUsbStorageDeviceInfo, vendor) == 0x16);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, vendor_len) == 0x118);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, product) == 0x120);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, product_len) == 0x220);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, serial_number) == 0x228);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, serial_number_len) == 0x328);
static_assert(offsetof(OrbisUsbStorageDeviceInfo, additional_features) == 0x330);
static_assert(sizeof(OrbisUsbStorageDeviceInfo) == 0x338);

#pragma pack(push, 1)
struct OrbisUsbStorageDirent {
    u32 d_fileno;
    u16 d_reclen;
    u8 d_type;
    u8 d_namlen;
    char d_name[256];
};
#pragma pack(pop)
static_assert(offsetof(OrbisUsbStorageDirent, d_fileno) == 0);
static_assert(offsetof(OrbisUsbStorageDirent, d_reclen) == 4);
static_assert(offsetof(OrbisUsbStorageDirent, d_type) == 6);
static_assert(offsetof(OrbisUsbStorageDirent, d_namlen) == 7);
static_assert(offsetof(OrbisUsbStorageDirent, d_name) == 8);

struct UsbDirentItem {
    std::string name;
    bool is_directory;
};

struct UsbGetdentsState {
    std::vector<UsbDirentItem> items;
    size_t current_index = 0;
};

static std::mutex g_usb_getdents_mutex;
static std::unordered_map<s32, UsbGetdentsState> g_usb_getdents_table;
static s32 g_usb_next_fd = 1;

std::filesystem::path GetUsbMountPath(u32 device_id) {
    return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "mnt" /
           ("usb" + std::to_string(device_id));
}

static s32 PS4_SYSV_ABI sceUsbStorageInit(void* attr) {
    LOG_DEBUG(Lib_Usbd, "called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageTerm() {
    LOG_DEBUG(Lib_Usbd, "called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageRegisterCallback(s32 event_type, void* callback, void* arg) {
    if (!callback) {
        LOG_ERROR(Lib_Usbd, "callback is nullptr");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    if (event_type != 1 && event_type != 2) {
        LOG_ERROR(Lib_Usbd, "event_type {} is invalid", event_type);
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: event_type = {}, callback = {}, arg = {}", event_type,
              callback, arg);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageUnregisterCallback(s32 event_type, void* callback) {
    if (!callback) {
        LOG_ERROR(Lib_Usbd, "callback is nullptr");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    if (event_type != 1 && event_type != 2) {
        LOG_ERROR(Lib_Usbd, "event_type {} is invalid", event_type);
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: event_type = {}, callback = {}", event_type, callback);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageRegisterCallbackForMapAvailable(u32 device_id,
                                                                     const char* directory,
                                                                     void* callback, void* arg) {
    if (!directory || !callback) {
        LOG_ERROR(Lib_Usbd, "directory or callback is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: device_id = {}, directory = {}, callback = {}, arg = {}",
              device_id, directory, callback, arg);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageUnregisterCallbackForMapAvailable(u32 device_id,
                                                                       const char* directory,
                                                                       void* callback) {
    if (!directory || !callback) {
        LOG_ERROR(Lib_Usbd, "directory or callback is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: device_id = {}, directory = {}, callback = {}",
              device_id, directory, callback);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageRegisterCallbackForMapUnavailable(u32 device_id,
                                                                       const char* directory,
                                                                       void* callback, void* arg) {
    if (!directory || !callback) {
        LOG_ERROR(Lib_Usbd, "directory or callback is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: device_id = {}, directory = {}, callback = {}, arg = {}",
              device_id, directory, callback, arg);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageUnregisterCallbackForMapUnavailable(u32 device_id,
                                                                         const char* directory,
                                                                         void* callback) {
    if (!directory || !callback) {
        LOG_ERROR(Lib_Usbd, "directory or callback is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_ERROR(Lib_Usbd, "(STUBBED) called: device_id = {}, directory = {}, callback = {}",
              device_id, directory, callback);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageGetDeviceList(u32* device_ids, u32* count) {
    if (!device_ids || !count) {
        LOG_ERROR(Lib_Usbd, "device_ids or count pointer is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }

    const auto usb_path = GetUsbMountPath(0);
    std::error_code ec;
    const bool usb_exists = std::filesystem::exists(usb_path, ec);

    if (!usb_exists) {
        *count = 0;
        LOG_DEBUG(Lib_Usbd, "no USB devices found");
        return ORBIS_OK;
    }

    device_ids[0] = 0;
    *count = 1;
    LOG_INFO(Lib_Usbd, "found 1 USB device (id = 0)");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageGetDeviceInfo(u32 device_id, OrbisUsbStorageDeviceInfo* info) {
    if (!info) {
        LOG_ERROR(Lib_Usbd, "info pointer is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    if (device_id != 0) {
        LOG_ERROR(Lib_Usbd, "invalid device_id {}", device_id);
        return ORBIS_AUTO_MOUNTER_ERROR_NOENT;
    }

    const auto usb_path = GetUsbMountPath(device_id);
    std::error_code ec;
    if (!std::filesystem::exists(usb_path, ec)) {
        LOG_ERROR(Lib_Usbd, "USB device {} mount path does not exist", device_id);
        return ORBIS_AUTO_MOUNTER_ERROR_NOENT;
    }

    std::memset(info, 0, sizeof(OrbisUsbStorageDeviceInfo));
    info->device_id = device_id;
    info->vendor_id = 0x054c;
    info->product_id = 0x0268;
    info->bcd_device = 0x0100;

    std::strncpy(info->vendor, "shadPS4", sizeof(info->vendor) - 1);
    info->vendor_len = std::strlen(info->vendor);

    std::strncpy(info->product, "Virtual USB Storage", sizeof(info->product) - 1);
    info->product_len = std::strlen(info->product);

    std::strncpy(info->serial_number, "0123456789ABCDEF", sizeof(info->serial_number) - 1);
    info->serial_number_len = std::strlen(info->serial_number);

    info->additional_features = ORBIS_USB_STORAGE_FEATURE_EXFAT;

    LOG_INFO(Lib_Usbd, "device_id = {}, vendor = '{}', product = '{}', serial = '{}'", device_id,
             info->vendor, info->product, info->serial_number);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageIsExist(u32 device_id) {
    if (device_id != 0) {
        return 0;
    }
    std::error_code ec;
    return std::filesystem::exists(GetUsbMountPath(device_id), ec) ? 1 : 0;
}

static s32 PS4_SYSV_ABI sceUsbStorageRequestMapWSB(u32 device_id, const char* subpath, s32 flags,
                                                   s32 unk, char* out_mount_path, s32* out_status) {
    if (device_id != 0) {
        LOG_ERROR(Lib_Usbd, "invalid device_id {}", device_id);
        return ORBIS_AUTO_MOUNTER_ERROR_NOENT;
    }

    std::string mount_path = "/mnt/usb" + std::to_string(device_id);
    if (subpath && subpath[0] != '\0') {
        std::string_view sp(subpath);
        if (sp.starts_with("/mnt/usb")) {
            mount_path = std::string(sp);
        } else if (sp.starts_with("/")) {
            mount_path += std::string(sp);
        } else {
            mount_path += "/" + std::string(sp);
        }
    }

    if (out_mount_path) {
        std::strncpy(out_mount_path, mount_path.c_str(), 127);
        out_mount_path[127] = '\0';
    }
    if (out_status) {
        *out_status = 0;
    }

    LOG_INFO(Lib_Usbd, "mapped device_id {} to '{}'", device_id, mount_path);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageRequestMap(u32 device_id, const char* subpath, s32 flags,
                                                s32 unk, char* out_mount_path, s32* out_status) {
    return sceUsbStorageRequestMapWSB(device_id, subpath, flags, unk, out_mount_path, out_status);
}

static s32 PS4_SYSV_ABI sceUsbStorageRequestUnmap(u32 device_id, const char* directory) {
    if (!directory) {
        LOG_ERROR(Lib_Usbd, "directory is null");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    LOG_INFO(Lib_Usbd, "unmapped device_id {}, directory '{}'", device_id, directory);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageGetdentsOpen(u32 device_id, s32* out_fd, u64* out_bufsize) {
    if (!out_fd || !out_bufsize) {
        LOG_ERROR(Lib_Usbd, "null pointer parameter");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }
    if (device_id != 0) {
        LOG_ERROR(Lib_Usbd, "invalid device_id {}", device_id);
        return ORBIS_AUTO_MOUNTER_ERROR_NOENT;
    }

    const auto mount_usb_dir = GetUsbMountPath(device_id);
    std::error_code ec;
    if (!std::filesystem::exists(mount_usb_dir, ec)) {
        std::filesystem::create_directories(mount_usb_dir, ec);
    }

    UsbGetdentsState state;
    for (const auto& entry : std::filesystem::directory_iterator(mount_usb_dir, ec)) {
        const auto filename = entry.path().filename().string();
        if (filename.empty() || filename[0] == '.') {
            continue;
        }
        const bool is_dir = entry.is_directory(ec);
        state.items.push_back({filename, is_dir});
    }

    std::scoped_lock lock(g_usb_getdents_mutex);
    const s32 fd = g_usb_next_fd++;
    const size_t count = state.items.size();
    g_usb_getdents_table[fd] = std::move(state);

    *out_fd = fd;
    *out_bufsize = 0x8000; // 32 KB buffer

    LOG_INFO(Lib_Usbd, "opened getdents fd = {} for device {}, found {} entries", fd, device_id,
             count);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageGetdentsRead(s32 fd, void* buffer, u64 bufsize,
                                                  u64* out_bytes_read) {
    if (!buffer || !out_bytes_read) {
        LOG_ERROR(Lib_Usbd, "null buffer or out_bytes_read");
        return ORBIS_AUTO_MOUNTER_ERROR_INVAL;
    }

    std::scoped_lock lock(g_usb_getdents_mutex);
    auto it = g_usb_getdents_table.find(fd);
    if (it == g_usb_getdents_table.end()) {
        LOG_ERROR(Lib_Usbd, "invalid fd {}", fd);
        *out_bytes_read = 0;
        return ORBIS_AUTO_MOUNTER_ERROR_NOTSUP;
    }

    auto& state = it->second;
    u64 bytes_written = 0;

    while (state.current_index < state.items.size()) {
        const auto& item = state.items[state.current_index];
        const u16 namlen = static_cast<u16>(std::min<size_t>(item.name.size(), 255));
        const u16 reclen = static_cast<u16>(Common::AlignUp<size_t>(8 + namlen + 1, 8));

        if (bytes_written + reclen > bufsize) {
            break;
        }

        auto* entry =
            reinterpret_cast<OrbisUsbStorageDirent*>(static_cast<u8*>(buffer) + bytes_written);
        std::memset(entry, 0, reclen);
        entry->d_fileno = static_cast<u32>(state.current_index + 1);
        entry->d_reclen = reclen;
        entry->d_type = item.is_directory ? 4 : 8; // 4 = DT_DIR, 8 = DT_REG
        entry->d_namlen = static_cast<u8>(namlen);
        std::memcpy(entry->d_name, item.name.data(), namlen);
        entry->d_name[namlen] = '\0';

        LOG_TRACE(Lib_Usbd, "entry: '{}', type = {}", item.name, entry->d_type);

        bytes_written += reclen;
        state.current_index++;
    }

    *out_bytes_read = bytes_written;
    LOG_DEBUG(Lib_Usbd, "fd = {}, returned {} bytes ({}/{} entries)", fd, bytes_written,
              state.current_index, state.items.size());
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceUsbStorageGetdentsClose(s32 fd) {
    std::scoped_lock lock(g_usb_getdents_mutex);
    g_usb_getdents_table.erase(fd);
    LOG_DEBUG(Lib_Usbd, "closed fd = {}", fd);
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("BDDZwF5kuTc", "libSceUsbStorage", 1, "libSceUsbStorage", sceUsbStorageInit);
    LIB_FUNCTION("Wp8zHTocS5E", "libSceUsbStorage", 1, "libSceUsbStorage", sceUsbStorageTerm);
    LIB_FUNCTION("vFkdkzJgSpw", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageRegisterCallback);
    LIB_FUNCTION("+Ib-MHNUf80", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageUnregisterCallback);
    LIB_FUNCTION("IDYJZSeBgDs", "libSceUsbStorage", 1, "libSceUsbStorage", sceUsbStorageRequestMap);
    LIB_FUNCTION("rx7EcAS2ARk", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageRequestMapWSB);
    LIB_FUNCTION("rx7EcAS2ARk", "libSceUsbStorageAux", 1, "libSceUsbStorage",
                 sceUsbStorageRequestMapWSB);
    LIB_FUNCTION("fl3roYs7F9U", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageRequestUnmap);
    LIB_FUNCTION("mryrNITeYvI", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageGetDeviceList);
    LIB_FUNCTION("-GvBqz54ssU", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageGetDeviceInfo);
    LIB_FUNCTION("tO8DvyElInw", "libSceUsbStorage", 1, "libSceUsbStorage", sceUsbStorageIsExist);
    LIB_FUNCTION("0rG6xtn7N5Q", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageRegisterCallbackForMapAvailable);
    LIB_FUNCTION("C3ETNYXsht4", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageUnregisterCallbackForMapAvailable);
    LIB_FUNCTION("8mpZuu7xfbM", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageRegisterCallbackForMapUnavailable);
    LIB_FUNCTION("4YMBk1lfUm0", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageUnregisterCallbackForMapUnavailable);
    LIB_FUNCTION("2LrlpFmBTC8", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsOpen);
    LIB_FUNCTION("2LrlpFmBTC8", "libSceUsbStorageAux", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsOpen);
    LIB_FUNCTION("OeJdPEmLYX4", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsRead);
    LIB_FUNCTION("OeJdPEmLYX4", "libSceUsbStorageAux", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsRead);
    LIB_FUNCTION("w1mZOJCxvhQ", "libSceUsbStorage", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsClose);
    LIB_FUNCTION("w1mZOJCxvhQ", "libSceUsbStorageAux", 1, "libSceUsbStorage",
                 sceUsbStorageGetdentsClose);
}

} // namespace Libraries::UsbStorage
