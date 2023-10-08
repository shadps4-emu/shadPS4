#pragma once
#include "Core/PS4/Loader/SymbolsResolver.h"

namespace Emulator::HLE::Libraries::LibPad {

struct ScePadData {
    u32 buttons;
    u08 left_stick_x;
    u08 left_stick_y;
    u08 right_stick_x;
    u08 right_stick_y;
    u08 analog_buttons_l2;
    u08 analog_buttons_r2;
    u08 padding[2];
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    float angular_velocity_x;
    float angular_velocity_y;
    float angular_velocity_z;
    u08 touch_data_touch_num;
    u08 touch_data_reserve[3];
    u32 touch_data_reserve1;
    u16 touch_data_touch0_x;
    u16 touch_data_touch0_y;
    u08 touch_data_touch0_id;
    u08 touch_data_touch0_reserve[3];
    u16 touch_data_touch1_x;
    u16 touch_data_touch1_y;
    u08 touch_data_touch1_id;
    u08 touch_data_touch1_reserve[3];
    bool connected;
    u64 timestamp;
    u32 extension_unit_data_extension_unit_id;
    u08 extension_unit_data_reserve[1];
    u08 extension_unit_data_data_length;
    u08 extension_unit_data_data[10];
    u08 connected_count;
    u08 reserve[2];
    u08 device_unique_data_len;
    u08 device_unique_data[12];
};
// hle functions
int PS4_SYSV_ABI scePadInit();

void libPad_Register(SymbolsResolver* sym);
};  // namespace Emulator::HLE::Libraries::LibPad