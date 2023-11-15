#include "save_data.h"

#include <core/hle/error_codes.h>
#include <core/hle/libraries/libs.h>

#include "common/log.h"

namespace Core::Libraries::LibSaveData {

constexpr bool log_file_savedata = true;  // disable it to disable logging

s32 PS4_SYSV_ABI sceSaveDataMount2(const SceSaveDataMount2* mount, SceSaveDataMountResult* mountResult) {
    PRINT_DUMMY_FUNCTION_NAME();
    //Save Data are stored encrypted inside /user/home/user Id/title Id/savedata0/sce_sys/. 
    LOG_INFO_IF(log_file_savedata, "\t userId     = {}\n", mount->userId);
    LOG_INFO_IF(log_file_savedata, "\t dirname    = {}\n", mount->dirName->data);
    LOG_INFO_IF(log_file_savedata, "\t blocks     = {}\n", mount->blocks);
    LOG_INFO_IF(log_file_savedata, "\t mountMode  = {}\n", mount->mountMode);
    return SCE_OK;
}
void saveDataSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0z45PIH+SNI", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMount2);
}
}  // namespace Core::Libraries::LibSaveData