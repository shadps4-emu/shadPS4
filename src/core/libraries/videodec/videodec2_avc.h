// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Videodec2 {

struct OrbisVideodec2AvcPictureInfo {
    u64 thisSize;

    bool isValid;

    u64 ptsData;
    u64 dtsData;
    u64 attachedData;

    u8 idrPictureflag;

    u8 profile_idc;
    u8 level_idc;
    u32 pic_width_in_mbs_minus1;
    u32 pic_height_in_map_units_minus1;
    u8 frame_mbs_only_flag;

    u8 frame_cropping_flag;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;

    u8 aspect_ratio_info_present_flag;
    u8 aspect_ratio_idc;
    u16 sar_width;
    u16 sar_height;

    u8 video_signal_type_present_flag;
    u8 video_format;
    u8 video_full_range_flag;
    u8 colour_description_present_flag;
    u8 colour_primaries;
    u8 transfer_characteristics;
    u8 matrix_coefficients;

    u8 timing_info_present_flag;
    u32 num_units_in_tick;
    u32 time_scale;
    u8 fixed_frame_rate_flag;

    u8 bitstream_restriction_flag;
    u8 max_dec_frame_buffering;

    u8 pic_struct_present_flag;
    u8 pic_struct;
    u8 field_pic_flag;
    u8 bottom_field_flag;

    u8 sequenceParameterSetPresentFlag;
    u8 pictureParameterSetPresentFlag;
    u8 auDelimiterPresentFlag;
    u8 endOfSequencePresentFlag;
    u8 endOfStreamPresentFlag;
    u8 fillerDataPresentFlag;
    u8 pictureTimingSeiPresentFlag;
    u8 bufferingPeriodSeiPresentFlag;

    u8 constraint_set0_flag;
    u8 constraint_set1_flag;
    u8 constraint_set2_flag;
    u8 constraint_set3_flag;
    u8 constraint_set4_flag;
    u8 constraint_set5_flag;
};
static_assert(sizeof(OrbisVideodec2AvcPictureInfo) == 0x78);

// An older version of the OrbisVideodec2AvcPictureInfo struct
// Keeping this is needed for compatiblity with older games.
struct OrbisVideodec2LegacyAvcPictureInfo {
    u64 thisSize;

    bool isValid;

    u64 ptsData;
    u64 dtsData;
    u64 attachedData;

    u8 idrPictureflag;

    u8 profile_idc;
    u8 level_idc;
    u32 pic_width_in_mbs_minus1;
    u32 pic_height_in_map_units_minus1;
    u8 frame_mbs_only_flag;

    u8 frame_cropping_flag;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;

    u8 aspect_ratio_info_present_flag;
    u8 aspect_ratio_idc;
    u16 sar_width;
    u16 sar_height;

    u8 video_signal_type_present_flag;
    u8 video_format;
    u8 video_full_range_flag;
    u8 colour_description_present_flag;
    u8 colour_primaries;
    u8 transfer_characteristics;
    u8 matrix_coefficients;

    u8 timing_info_present_flag;
    u32 num_units_in_tick;
    u32 time_scale;
    u8 fixed_frame_rate_flag;

    u8 bitstream_restriction_flag;
    u8 max_dec_frame_buffering;

    u8 pic_struct_present_flag;
    u8 pic_struct;
    u8 field_pic_flag;
    u8 bottom_field_flag;
};
static_assert(sizeof(OrbisVideodec2LegacyAvcPictureInfo) == 0x68);

} // namespace Libraries::Videodec2