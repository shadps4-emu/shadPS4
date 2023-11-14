#include "save_data.h"

#include <core/hle/error_codes.h>
#include <core/hle/libraries/libs.h>

#include "common/log.h"

namespace Core::Libraries::LibSaveData {
s32 sceSaveDataMount2(const SceSaveDataMount2* mount, SceSaveDataMountResult* mountResult) {
    PRINT_DUMMY_FUNCTION_NAME();
    return SCE_OK;
}
void saveDataSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0z45PIH+SNI", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMount2);
}
}  // namespace Core::Libraries::LibSaveData