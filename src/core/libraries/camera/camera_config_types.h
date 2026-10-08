// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

constexpr s32 ORBIS_CAMERA_MAX_DEVICE_NUM = 2;

enum OrbisCameraResolution {
    ORBIS_CAMERA_RESOLUTION_1280X800 = 0x0,
    ORBIS_CAMERA_RESOLUTION_640X400 = 0x1,
    ORBIS_CAMERA_RESOLUTION_320X200 = 0x2,
    ORBIS_CAMERA_RESOLUTION_160X100 = 0x3,
    ORBIS_CAMERA_RESOLUTION_320X192 = 0x4,
    ORBIS_CAMERA_RESOLUTION_SPECIFIED_WIDTH_HEIGHT,
    ORBIS_CAMERA_RESOLUTION_UNKNOWN = 0xFF,
};

enum OrbisCameraFramerate {
    ORBIS_CAMERA_FRAMERATE_UNKNOWN = 0,
    ORBIS_CAMERA_FRAMERATE_7_5 = 7,
    ORBIS_CAMERA_FRAMERATE_15 = 15,
    ORBIS_CAMERA_FRAMERATE_30 = 30,
    ORBIS_CAMERA_FRAMERATE_60 = 60,
    ORBIS_CAMERA_FRAMERATE_120 = 120,
    ORBIS_CAMERA_FRAMERATE_240 = 240,
};

enum OrbisCameraBaseFormat {
    ORBIS_CAMERA_FORMAT_YUV422 = 0x0,
    ORBIS_CAMERA_FORMAT_RAW16,
    ORBIS_CAMERA_FORMAT_RAW8,
    ORBIS_CAMERA_FORMAT_NO_USE = 0x10,
    ORBIS_CAMERA_FORMAT_UNKNOWN = 0xFF,
};

enum OrbisCameraScaleFormat {
    ORBIS_CAMERA_SCALE_FORMAT_YUV422 = 0x0,
    ORBIS_CAMERA_SCALE_FORMAT_Y16 = 0x3,
    ORBIS_CAMERA_SCALE_FORMAT_Y8,
    ORBIS_CAMERA_SCALE_FORMAT_NO_USE = 0x10,
    ORBIS_CAMERA_SCALE_FORMAT_UNKNOWN = 0xFF,
};

struct OrbisCameraFormat {
    OrbisCameraBaseFormat formatLevel0;
    OrbisCameraScaleFormat formatLevel1;
    OrbisCameraScaleFormat formatLevel2;
    OrbisCameraScaleFormat formatLevel3;
};

struct OrbisCameraConfigExtention {
    OrbisCameraFormat format;
    OrbisCameraResolution resolution;
    OrbisCameraFramerate framerate;
    u32 width;
    u32 height;
    u32 reserved1;
    void* pBaseOption;
};

constexpr OrbisCameraConfigExtention camera_config_types[5][ORBIS_CAMERA_MAX_DEVICE_NUM]{
    {
        // type 1
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_RAW16,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
    },
    {
        // type 2
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
    },
    {
        // type 3
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_Y8,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
    },
    {
        // type 4
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_RAW16,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_RAW16,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
    },
    {
        // type 5
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
        {
            .format =
                {
                    .formatLevel0 = ORBIS_CAMERA_FORMAT_RAW16,
                    .formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                    .formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_YUV422,
                },
            .framerate = ORBIS_CAMERA_FRAMERATE_60,
        },
    }};

enum OrbisCameraConfigType {
    ORBIS_CAMERA_CONFIG_TYPE1 = 0x01,
    ORBIS_CAMERA_CONFIG_TYPE2 = 0x02,
    ORBIS_CAMERA_CONFIG_TYPE3 = 0x03,
    ORBIS_CAMERA_CONFIG_TYPE4 = 0x04,
    ORBIS_CAMERA_CONFIG_TYPE5 = 0x05,
    ORBIS_CAMERA_CONFIG_EXTENTION = 0x10,
};