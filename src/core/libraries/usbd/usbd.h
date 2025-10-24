// SPDX - FileCopyrightText : Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "emulated/dimensions.h"
#include "emulated/infinity.h"
#include "emulated/skylander.h"
#include "usb_backend.h"

extern "C" {
struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;
struct libusb_config_descriptor;
struct libusb_transfer;
struct libusb_control_setup;
struct timeval;
}

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Usbd {

extern std::shared_ptr<UsbBackend> usb_backend;

using SceUsbdDevice = libusb_device;
using SceUsbdDeviceHandle = libusb_device_handle;
using SceUsbdDeviceDescriptor = libusb_device_descriptor;
using SceUsbdConfigDescriptor = libusb_config_descriptor;
using SceUsbdTransfer = libusb_transfer;
using SceUsbdControlSetup = libusb_control_setup;
using SceUsbdTransferCallback = void PS4_SYSV_ABI (*)(SceUsbdTransfer* transfer);

using SkylandersPortalBackend = SkylanderBackend;
using InfinityBaseBackend = InfinityBackend;
using DimensionsToypadBackend = DimensionsBackend;

enum class SceUsbdSpeed : u32 {
    UNKNOWN = 0,
    LOW = 1,
    FULL = 2,
    HIGH = 3,
    SUPER = 4,
    SUPER_PLUS = 5
};

s32 PS4_SYSV_ABI sceUsbdInit();
void PS4_SYSV_ABI sceUsbdExit();

s64 PS4_SYSV_ABI sceUsbdGetDeviceList(SceUsbdDevice*** list);
void PS4_SYSV_ABI sceUsbdFreeDeviceList(SceUsbdDevice** list, s32 unref_devices);

SceUsbdDevice* PS4_SYSV_ABI sceUsbdRefDevice(SceUsbdDevice* device);
void PS4_SYSV_ABI sceUsbdUnrefDevice(SceUsbdDevice* device);

s32 PS4_SYSV_ABI sceUsbdGetConfiguration(SceUsbdDeviceHandle* dev_handle, s32* config);
s32 PS4_SYSV_ABI sceUsbdGetDeviceDescriptor(SceUsbdDevice* device, SceUsbdDeviceDescriptor* desc);
s32 PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor(SceUsbdDevice* device,
                                                  SceUsbdConfigDescriptor** config);
s32 PS4_SYSV_ABI sceUsbdGetConfigDescriptor(SceUsbdDevice* device, u8 config_index,
                                            SceUsbdConfigDescriptor** config);
s32 PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue(SceUsbdDevice* device, u8 bConfigurationValue,
                                                   SceUsbdConfigDescriptor** config);
void PS4_SYSV_ABI sceUsbdFreeConfigDescriptor(SceUsbdConfigDescriptor* config);

u8 PS4_SYSV_ABI sceUsbdGetBusNumber(SceUsbdDevice* device);
u8 PS4_SYSV_ABI sceUsbdGetDeviceAddress(SceUsbdDevice* device);

SceUsbdSpeed PS4_SYSV_ABI sceUsbdGetDeviceSpeed(SceUsbdDevice* device);
s32 PS4_SYSV_ABI sceUsbdGetMaxPacketSize(SceUsbdDevice* device, u8 endpoint);
s32 PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize(SceUsbdDevice* device, u8 endpoint);

s32 PS4_SYSV_ABI sceUsbdOpen(SceUsbdDevice* device, SceUsbdDeviceHandle** dev_handle);
void PS4_SYSV_ABI sceUsbdClose(SceUsbdDeviceHandle* dev_handle);
SceUsbdDevice* PS4_SYSV_ABI sceUsbdGetDevice(SceUsbdDeviceHandle* dev_handle);

s32 PS4_SYSV_ABI sceUsbdSetConfiguration(SceUsbdDeviceHandle* dev_handle, s32 config);
s32 PS4_SYSV_ABI sceUsbdClaimInterface(SceUsbdDeviceHandle* dev_handle, s32 interface_number);
s32 PS4_SYSV_ABI sceUsbdReleaseInterface(SceUsbdDeviceHandle* dev_handle, s32 interface_number);

SceUsbdDeviceHandle* PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid(u16 vendor_id, u16 product_id);

s32 PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting(SceUsbdDeviceHandle* dev_handle,
                                               int interface_number, int alternate_setting);
s32 PS4_SYSV_ABI sceUsbdClearHalt(SceUsbdDeviceHandle* dev_handle, u8 endpoint);
s32 PS4_SYSV_ABI sceUsbdResetDevice(SceUsbdDeviceHandle* dev_handle);

s32 PS4_SYSV_ABI sceUsbdKernelDriverActive(SceUsbdDeviceHandle* dev_handle, int interface_number);
s32 PS4_SYSV_ABI sceUsbdDetachKernelDriver(SceUsbdDeviceHandle* dev_handle, int interface_number);
s32 PS4_SYSV_ABI sceUsbdAttachKernelDriver(SceUsbdDeviceHandle* dev_handle, int interface_number);

u8* PS4_SYSV_ABI sceUsbdControlTransferGetData(SceUsbdTransfer* transfer);
SceUsbdControlSetup* PS4_SYSV_ABI sceUsbdControlTransferGetSetup(SceUsbdTransfer* transfer);

void PS4_SYSV_ABI sceUsbdFillControlSetup(u8* buf, u8 bmRequestType, u8 bRequest, u16 wValue,
                                          u16 wIndex, u16 wLength);

SceUsbdTransfer* PS4_SYSV_ABI sceUsbdAllocTransfer(int iso_packets);
s32 PS4_SYSV_ABI sceUsbdSubmitTransfer(SceUsbdTransfer* transfer);
s32 PS4_SYSV_ABI sceUsbdCancelTransfer(SceUsbdTransfer* transfer);
void PS4_SYSV_ABI sceUsbdFreeTransfer(SceUsbdTransfer* transfer);

void PS4_SYSV_ABI sceUsbdFillControlTransfer(SceUsbdTransfer* transfer,
                                             SceUsbdDeviceHandle* dev_handle, u8* buffer,
                                             SceUsbdTransferCallback callback, void* user_data,
                                             u32 timeout);
void PS4_SYSV_ABI sceUsbdFillBulkTransfer(SceUsbdTransfer* transfer,
                                          SceUsbdDeviceHandle* dev_handle, u8 endpoint, u8* buffer,
                                          s32 length, SceUsbdTransferCallback callback,
                                          void* user_data, u32 timeout);
void PS4_SYSV_ABI sceUsbdFillInterruptTransfer(SceUsbdTransfer* transfer,
                                               SceUsbdDeviceHandle* dev_handle, u8 endpoint,
                                               u8* buffer, s32 length,
                                               SceUsbdTransferCallback callback, void* user_data,
                                               u32 timeout);
void PS4_SYSV_ABI sceUsbdFillIsoTransfer(SceUsbdTransfer* transfer, SceUsbdDeviceHandle* dev_handle,
                                         u8 endpoint, u8* buffer, s32 length, s32 num_iso_packets,
                                         SceUsbdTransferCallback callback, void* userData,
                                         u32 timeout);

void PS4_SYSV_ABI sceUsbdSetIsoPacketLengths(SceUsbdTransfer* transfer, u32 length);
u8* PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer(SceUsbdTransfer* transfer, u32 packet);

s32 PS4_SYSV_ABI sceUsbdControlTransfer(SceUsbdDeviceHandle* dev_handle, u8 request_type,
                                        u8 bRequest, u16 wValue, u16 wIndex, u8* data, s32 wLength,
                                        u32 timeout);
s32 PS4_SYSV_ABI sceUsbdBulkTransfer(SceUsbdDeviceHandle* dev_handle, u8 endpoint, u8* data,
                                     s32 length, s32* actual_length, u32 timeout);
s32 PS4_SYSV_ABI sceUsbdInterruptTransfer(SceUsbdDeviceHandle* dev_handle, u8 endpoint, u8* data,
                                          s32 length, s32* actual_length, u32 timeout);

s32 PS4_SYSV_ABI sceUsbdGetDescriptor(SceUsbdDeviceHandle* dev_handle, u8 descType, u8 descIndex,
                                      u8* data, s32 length);
s32 PS4_SYSV_ABI sceUsbdGetStringDescriptor(SceUsbdDeviceHandle* dev_handle, u8 desc_index,
                                            u16 langid, u8* data, s32 length);
s32 PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii(SceUsbdDeviceHandle* dev_handle, u8 desc_index,
                                                 u8* data, s32 length);

s32 PS4_SYSV_ABI sceUsbdTryLockEvents();
void PS4_SYSV_ABI sceUsbdLockEvents();
void PS4_SYSV_ABI sceUsbdUnlockEvents();
s32 PS4_SYSV_ABI sceUsbdEventHandlingOk();
s32 PS4_SYSV_ABI sceUsbdEventHandlerActive();
void PS4_SYSV_ABI sceUsbdLockEventWaiters();
void PS4_SYSV_ABI sceUsbdUnlockEventWaiters();
s32 PS4_SYSV_ABI sceUsbdWaitForEvent(timeval* tv);

s32 PS4_SYSV_ABI sceUsbdHandleEventsTimeout(timeval* tv);
s32 PS4_SYSV_ABI sceUsbdHandleEvents();
s32 PS4_SYSV_ABI sceUsbdHandleEventsLocked(timeval* tv);

s32 PS4_SYSV_ABI sceUsbdCheckConnected(SceUsbdDeviceHandle* dev_handle);

int PS4_SYSV_ABI Func_65F6EF33E38FFF50();
int PS4_SYSV_ABI Func_97F056BAD90AADE7();
int PS4_SYSV_ABI Func_C55104A33B35B264();
int PS4_SYSV_ABI Func_D56B43060720B1E0();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Usbd