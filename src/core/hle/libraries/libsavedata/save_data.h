#pragma once

#include <core/hle/libraries/libuserservice/libuserservice.h>

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSaveData {

constexpr int SCE_SAVE_DATA_TITLE_ID_DATA_SIZE = 10;        // Save data title ID size
constexpr int SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE = 32;      // Maximum size for a save data directory name
constexpr int SCE_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE = 16;  // Maximum size for a mount point name
constexpr int SCE_SAVE_DATA_FINGERPRINT_DATA_SIZE = 65;     // Fingerprint size
constexpr int SCE_SAVE_DATA_TITLE_MAXSIZE = 128;            // Maximum size for a save data title name (NULL-terminated, UTF-8)
constexpr int SCE_SAVE_DATA_SUBTITLE_MAXSIZE = 128;         // Maximum size for a save data subtitle name (NULL-terminated, UTF-8)
constexpr int SCE_SAVE_DATA_DETAIL_MAXSIZE = 1024;          // Maximum size for save data detailed information (NULL-terminated, UTF-8)

struct SceSaveDataDirName {
    char data[SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE];
};

struct SceSaveDataMountPoint {
    char data[SCE_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE];
};
struct SceSaveDataMount2 {
    Core::Libraries::LibUserService::SceUserServiceUserId userId;
    s32 padding;
    const SceSaveDataDirName* dirName;
    u64 blocks;
    u32 mountMode;
    u08 reserved[32];
    s32 padding2;
};

struct SceSaveDataMountResult {
    SceSaveDataMountPoint mountPoint;
    u64 requiredBlocks;
    u32 unused;
    u32 mountStatus;
    u08 reserved[28];
    s32 padding;
};

s32 sceSaveDataMount2(const SceSaveDataMount2* mount, SceSaveDataMountResult* mountResult);

void saveDataSymbolsRegister(Loader::SymbolsResolver* sym);

}  // namespace Core::Libraries::LibSaveData