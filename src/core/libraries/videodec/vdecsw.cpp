// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/vdecsw.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::Vdecsw {

s32 PS4_SYSV_ABI sceVdecswQueryComputeMemoryInfo(OrbisVdecswComputeMemoryInfo* compute_mem_info) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVdecswAllocateComputeQueue(const OrbisVdecswComputeConfigInfo* compute_cfg_info_in,
                              const OrbisVdecswComputeMemoryInfo* compute_mem_info_in,
                              OrbisVdecswComputeQueue* compute_queue_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswReleaseComputeQueue(OrbisVdecswComputeQueue compute_queue_in) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVdecswQueryDecoderMemoryInfo(const OrbisVdecswDecoderConfigInfo* decoder_config_info_in,
                                OrbisVdecswDecoderMemoryInfo* decoder_memory_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswCreateDecoder(const OrbisVdecswDecoderConfigInfo* decoder_config_info_in,
                                        const OrbisVdecswDecoderMemoryInfo* decoder_memory_info_in,
                                        OrbisVdecswDecoder* decoder_instance_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswDeleteDecoder(OrbisVdecswDecoder decoder_instance_in) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswResetDecoder(OrbisVdecswDecoder decoder_instance_in) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswSetDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                         const OrbisVdecswInputData* input_data_in) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswSyncDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                          OrbisVdecswInputResult* input_result_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswTrySyncDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                             OrbisVdecswInputResult* input_result_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswSetDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                          OrbisVdecswFrameBuffer* frame_buffer_in_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswSyncDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                           OrbisVdecswOutputInfo* output_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswTrySyncDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                              OrbisVdecswOutputInfo* output_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswFinalizeDecodeSequence(OrbisVdecswDecoder decoder_instance_in) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswGetPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                         void* p_1st_picture_info_out,
                                         void* p_2nd_picture_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswGetAvcPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                            OrbisVdecswAvcPictureInfo* p_1st_picture_info_out,
                                            OrbisVdecswAvcPictureInfo* p_2nd_picture_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswGetHevcPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                             OrbisVdecswHevcPictureInfo* picture_info_out) {
    LOG_ERROR(Lib_Vdecsw, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hIgrg5h4V6s", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswAllocateComputeQueue);
    LIB_FUNCTION("+L5ArV1tPGA", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswCreateDecoder);
    LIB_FUNCTION("ecUtPX+dBYk", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswDeleteDecoder);
    LIB_FUNCTION("5Y6nZqIZvBg", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswFinalizeDecodeSequence);
    LIB_FUNCTION("ihNT-uuEAr4", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetAvcPictureInfo);
    LIB_FUNCTION("PzF+L5zXoyg", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetHevcPictureInfo);
    LIB_FUNCTION("FzECy3Wxxas", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetPictureInfo);
    LIB_FUNCTION("0moTubWCsTM", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswQueryComputeMemoryInfo);
    LIB_FUNCTION("A+2M7EivuOU", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswQueryDecoderMemoryInfo);
    LIB_FUNCTION("fX-zOOefbbs", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswReleaseComputeQueue);
    LIB_FUNCTION("veb-YBrOqo0", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswResetDecoder);
    LIB_FUNCTION("aqMiF0AgUYI", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSetDecodeInput);
    LIB_FUNCTION("rgtMCOpyBSc", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSetDecodeOutput);
    LIB_FUNCTION("AAMM-Q1X0g0", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSyncDecodeInput);
    LIB_FUNCTION("tWiSgXov8GM", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSyncDecodeOutput);
    LIB_FUNCTION("l4sQYy5wPkc", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswTrySyncDecodeInput);
    LIB_FUNCTION("kMBw37oH8nI", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswTrySyncDecodeOutput);
}

} // namespace Libraries::Vdecsw