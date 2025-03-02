// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "usbd.h"

#include <fmt/format.h>
#include <libusb.h>

namespace Libraries::Usbd {

s32 libusb_to_orbis_error(int retVal) {
    if (retVal == LIBUSB_ERROR_OTHER)
        return 0x802400FF;
    if (retVal < 0) {
        LOG_ERROR(Lib_Usbd, "libusb returned: {}", libusb_error_name(retVal));
        return 0x80240000 - retVal;
    }

    return retVal;
}

libusb_context* g_libusb_context;

s32 PS4_SYSV_ABI sceUsbdInit() {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_init(&g_libusb_context));
}

void PS4_SYSV_ABI sceUsbdExit() {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_exit(g_libusb_context);
}

s64 PS4_SYSV_ABI sceUsbdGetDeviceList(SceUsbdDevice*** list) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_device_list(g_libusb_context, list));
}

void PS4_SYSV_ABI sceUsbdFreeDeviceList(SceUsbdDevice** list, s32 unref_devices) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_free_device_list(list, unref_devices);
}

SceUsbdDevice* PS4_SYSV_ABI sceUsbdRefDevice(SceUsbdDevice* device) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_ref_device(device);
}

void PS4_SYSV_ABI sceUsbdUnrefDevice(SceUsbdDevice* device) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_unref_device(device);
}

s32 PS4_SYSV_ABI sceUsbdGetConfiguration(SceUsbdDeviceHandle* dev_handle, s32* config) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_configuration(dev_handle, config));
}

s32 PS4_SYSV_ABI sceUsbdGetDeviceDescriptor(SceUsbdDevice* device, SceUsbdDeviceDescriptor* desc) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_device_descriptor(device, desc));
}

s32 PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor(SceUsbdDevice* device,
                                                  SceUsbdConfigDescriptor** config) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_active_config_descriptor(device, config));
}

s32 PS4_SYSV_ABI sceUsbdGetConfigDescriptor(SceUsbdDevice* device, u8 config_index,
                                            SceUsbdConfigDescriptor** config) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_config_descriptor(device, config_index, config));
}

s32 PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue(SceUsbdDevice* device, u8 bConfigurationValue,
                                                   SceUsbdConfigDescriptor** config) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_get_config_descriptor_by_value(device, bConfigurationValue, config));
}

void PS4_SYSV_ABI sceUsbdFreeConfigDescriptor(SceUsbdConfigDescriptor* config) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_free_config_descriptor(config);
}

u8 PS4_SYSV_ABI sceUsbdGetBusNumber(SceUsbdDevice* device) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_get_bus_number(device);
}

u8 PS4_SYSV_ABI sceUsbdGetDeviceAddress(SceUsbdDevice* device) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_get_device_address(device);
}

SceUsbdSpeed PS4_SYSV_ABI sceUsbdGetDeviceSpeed(SceUsbdDevice* device) {
    LOG_DEBUG(Lib_Usbd, "called");

    return static_cast<SceUsbdSpeed>(libusb_get_device_speed(device));
}

s32 PS4_SYSV_ABI sceUsbdGetMaxPacketSize(SceUsbdDevice* device, u8 endpoint) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_max_packet_size(device, endpoint));
}

s32 PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize(SceUsbdDevice* device, u8 endpoint) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_get_max_iso_packet_size(device, endpoint));
}

s32 PS4_SYSV_ABI sceUsbdOpen(SceUsbdDevice* device, SceUsbdDeviceHandle** dev_handle) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_open(device, dev_handle));
}

void PS4_SYSV_ABI sceUsbdClose(SceUsbdDeviceHandle* dev_handle) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_close(dev_handle);
}

SceUsbdDevice* PS4_SYSV_ABI sceUsbdGetDevice(SceUsbdDeviceHandle* dev_handle) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_get_device(dev_handle);
}

s32 PS4_SYSV_ABI sceUsbdSetConfiguration(SceUsbdDeviceHandle* dev_handle, s32 config) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_set_configuration(dev_handle, config));
}

s32 PS4_SYSV_ABI sceUsbdClaimInterface(SceUsbdDeviceHandle* dev_handle, s32 interface_number) {
    LOG_DEBUG(Lib_Usbd, "called");

    if (sceUsbdKernelDriverActive(dev_handle, interface_number)) {
        sceUsbdDetachKernelDriver(dev_handle, interface_number);
    }

    return libusb_to_orbis_error(libusb_claim_interface(dev_handle, interface_number));
}

s32 PS4_SYSV_ABI sceUsbdReleaseInterface(SceUsbdDeviceHandle* dev_handle, s32 interface_number) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_release_interface(dev_handle, interface_number));
}

SceUsbdDeviceHandle* PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid(u16 vendor_id, u16 product_id) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_open_device_with_vid_pid(g_libusb_context, vendor_id, product_id);
}

s32 PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting(SceUsbdDeviceHandle* dev_handle,
                                               int interface_number, int alternate_setting) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_set_interface_alt_setting(dev_handle, interface_number, alternate_setting));
}

s32 PS4_SYSV_ABI sceUsbdClearHalt(SceUsbdDeviceHandle* dev_handle, uint8_t endpoint) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_clear_halt(dev_handle, endpoint));
}

s32 PS4_SYSV_ABI sceUsbdResetDevice(SceUsbdDeviceHandle* dev_handle) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_reset_device(dev_handle));
}

s32 PS4_SYSV_ABI sceUsbdKernelDriverActive(SceUsbdDeviceHandle* dev_handle, int interface_number) {
    LOG_DEBUG(Lib_Usbd, "called");

#if defined(_WIN32)
    return 0;
#endif

    return libusb_to_orbis_error(libusb_kernel_driver_active(dev_handle, interface_number));
}

s32 PS4_SYSV_ABI sceUsbdDetachKernelDriver(SceUsbdDeviceHandle* dev_handle, int interface_number) {
    LOG_DEBUG(Lib_Usbd, "called");

#if defined(_WIN32)
    return 0;
#endif

    return libusb_to_orbis_error(libusb_detach_kernel_driver(dev_handle, interface_number));
}

s32 PS4_SYSV_ABI sceUsbdAttachKernelDriver(SceUsbdDeviceHandle* dev_handle, int interface_number) {
    LOG_DEBUG(Lib_Usbd, "called");

#if defined(_WIN32)
    return 0;
#endif

    return libusb_to_orbis_error(libusb_attach_kernel_driver(dev_handle, interface_number));
}

u8* PS4_SYSV_ABI sceUsbdControlTransferGetData(SceUsbdTransfer* transfer) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_control_transfer_get_data(transfer);
}

SceUsbdControlSetup* PS4_SYSV_ABI sceUsbdControlTransferGetSetup(SceUsbdTransfer* transfer) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_control_transfer_get_setup(transfer);
}

void PS4_SYSV_ABI sceUsbdFillControlSetup(u8* buf, u8 bmRequestType, u8 bRequest, u16 wValue,
                                          u16 wIndex, u16 wLength) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_fill_control_setup(buf, bmRequestType, bRequest, wValue, wIndex, wLength);
}

SceUsbdTransfer* PS4_SYSV_ABI sceUsbdAllocTransfer(int iso_packets) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_alloc_transfer(iso_packets);
}

s32 PS4_SYSV_ABI sceUsbdSubmitTransfer(SceUsbdTransfer* transfer) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_submit_transfer(transfer));
}

s32 PS4_SYSV_ABI sceUsbdCancelTransfer(SceUsbdTransfer* transfer) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_cancel_transfer(transfer));
}

void PS4_SYSV_ABI sceUsbdFreeTransfer(SceUsbdTransfer* transfer) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_free_transfer(transfer);
}

void PS4_SYSV_ABI sceUsbdFillControlTransfer(SceUsbdTransfer* transfer,
                                             SceUsbdDeviceHandle* dev_handle, u8* buffer,
                                             SceUsbdTransferCallback callback, void* user_data,
                                             u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_fill_control_transfer(transfer, dev_handle, buffer, callback, user_data, timeout);
}

void PS4_SYSV_ABI sceUsbdFillBulkTransfer(SceUsbdTransfer* transfer,
                                          SceUsbdDeviceHandle* dev_handle, uint8_t endpoint,
                                          u8* buffer, s32 length, SceUsbdTransferCallback callback,
                                          void* user_data, u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_fill_bulk_transfer(transfer, dev_handle, endpoint, buffer, length, callback, user_data,
                              timeout);
}

void PS4_SYSV_ABI sceUsbdFillInterruptTransfer(SceUsbdTransfer* transfer,
                                               SceUsbdDeviceHandle* dev_handle, uint8_t endpoint,
                                               u8* buffer, s32 length,
                                               SceUsbdTransferCallback callback, void* user_data,
                                               u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_fill_interrupt_transfer(transfer, dev_handle, endpoint, buffer, length, callback,
                                   user_data, timeout);
}

void PS4_SYSV_ABI sceUsbdFillIsoTransfer(SceUsbdTransfer* transfer, SceUsbdDeviceHandle* dev_handle,
                                         uint8_t endpoint, u8* buffer, s32 length,
                                         s32 num_iso_packets, SceUsbdTransferCallback callback,
                                         void* user_data, u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_fill_iso_transfer(transfer, dev_handle, endpoint, buffer, length, num_iso_packets,
                             callback, user_data, timeout);
}

void PS4_SYSV_ABI sceUsbdSetIsoPacketLengths(SceUsbdTransfer* transfer, u32 length) {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_set_iso_packet_lengths(transfer, length);
}

u8* PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer(SceUsbdTransfer* transfer, u32 packet) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_get_iso_packet_buffer(transfer, packet);
}

s32 PS4_SYSV_ABI sceUsbdControlTransfer(SceUsbdDeviceHandle* dev_handle, u8 request_type,
                                        u8 bRequest, u16 wValue, u16 wIndex, u8* data, s32 wLength,
                                        u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_control_transfer(dev_handle, request_type, bRequest, wValue,
                                                         wIndex, data, wLength, timeout));
}

s32 PS4_SYSV_ABI sceUsbdBulkTransfer(SceUsbdDeviceHandle* dev_handle, u8 endpoint, u8* data,
                                     s32 length, s32* actual_length, u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_bulk_transfer(dev_handle, endpoint, data, length, actual_length, timeout));
}

s32 PS4_SYSV_ABI sceUsbdInterruptTransfer(SceUsbdDeviceHandle* dev_handle, u8 endpoint, u8* data,
                                          s32 length, s32* actual_length, u32 timeout) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_interrupt_transfer(dev_handle, endpoint, data, length, actual_length, timeout));
}

s32 PS4_SYSV_ABI sceUsbdGetDescriptor(SceUsbdDeviceHandle* dev_handle, u8 descType, u8 descIndex,
                                      u8* data, s32 length) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_get_descriptor(dev_handle, descType, descIndex, data, length));
}

s32 PS4_SYSV_ABI sceUsbdGetStringDescriptor(SceUsbdDeviceHandle* dev_handle, u8 desc_index,
                                            u16 langid, u8* data, s32 length) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_get_string_descriptor(dev_handle, desc_index, langid, data, length));
}

s32 PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii(SceUsbdDeviceHandle* dev_handle, u8 desc_index,
                                                 u8* data, s32 length) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(
        libusb_get_string_descriptor_ascii(dev_handle, desc_index, data, length));
}

s32 PS4_SYSV_ABI sceUsbdTryLockEvents() {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_try_lock_events(g_libusb_context);
}

void PS4_SYSV_ABI sceUsbdLockEvents() {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_lock_events(g_libusb_context);
}

void PS4_SYSV_ABI sceUsbdUnlockEvents() {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_unlock_events(g_libusb_context);
}

s32 PS4_SYSV_ABI sceUsbdEventHandlingOk() {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_event_handling_ok(g_libusb_context);
}

s32 PS4_SYSV_ABI sceUsbdEventHandlerActive() {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_event_handler_active(g_libusb_context);
}

void PS4_SYSV_ABI sceUsbdLockEventWaiters() {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_lock_event_waiters(g_libusb_context);
}

void PS4_SYSV_ABI sceUsbdUnlockEventWaiters() {
    LOG_DEBUG(Lib_Usbd, "called");

    libusb_unlock_event_waiters(g_libusb_context);
}

s32 PS4_SYSV_ABI sceUsbdWaitForEvent(timeval* tv) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_wait_for_event(g_libusb_context, tv));
}

s32 PS4_SYSV_ABI sceUsbdHandleEventsTimeout(timeval* tv) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_handle_events_timeout(g_libusb_context, tv));
}

s32 PS4_SYSV_ABI sceUsbdHandleEvents() {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_handle_events(g_libusb_context));
}

s32 PS4_SYSV_ABI sceUsbdHandleEventsLocked(timeval* tv) {
    LOG_DEBUG(Lib_Usbd, "called");

    return libusb_to_orbis_error(libusb_handle_events_locked(g_libusb_context, tv));
}

s32 PS4_SYSV_ABI sceUsbdCheckConnected(SceUsbdDeviceHandle* dev_handle) {
    LOG_DEBUG(Lib_Usbd, "called");

    // There's no libusb version of this function.
    // Simulate by querying data.

    int config;
    int r = libusb_get_configuration(dev_handle, &config);

    return libusb_to_orbis_error(r);
}

int PS4_SYSV_ABI Func_65F6EF33E38FFF50() {
    LOG_ERROR(Lib_Usbd, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_97F056BAD90AADE7() {
    LOG_ERROR(Lib_Usbd, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C55104A33B35B264() {
    LOG_ERROR(Lib_Usbd, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D56B43060720B1E0() {
    LOG_ERROR(Lib_Usbd, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceUsbd(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0ktE1PhzGFU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdAllocTransfer);
    LIB_FUNCTION("BKMEGvfCPyU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdAttachKernelDriver);
    LIB_FUNCTION("fotb7DzeHYw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdBulkTransfer);
    LIB_FUNCTION("-KNh1VFIzlM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdCancelTransfer);
    LIB_FUNCTION("MlW6deWfPp0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdCheckConnected);
    LIB_FUNCTION("AE+mHBHneyk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClaimInterface);
    LIB_FUNCTION("3tPPMo4QRdY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClearHalt);
    LIB_FUNCTION("HarYYlaFGJY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClose);
    LIB_FUNCTION("RRKFcKQ1Ka4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdControlTransfer);
    LIB_FUNCTION("XUWtxI31YEY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdControlTransferGetData);
    LIB_FUNCTION("SEdQo8CFmus", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdControlTransferGetSetup);
    LIB_FUNCTION("Y5go+ha6eDs", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdDetachKernelDriver);
    LIB_FUNCTION("Vw8Hg1CN028", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdEventHandlerActive);
    LIB_FUNCTION("e7gp1xhu6RI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdEventHandlingOk);
    LIB_FUNCTION("Fq6+0Fm55xU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdExit);
    LIB_FUNCTION("oHCade-0qQ0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillBulkTransfer);
    LIB_FUNCTION("8KrqbaaPkE0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillControlSetup);
    LIB_FUNCTION("7VGfMerK6m0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillControlTransfer);
    LIB_FUNCTION("t3J5pXxhJlI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillInterruptTransfer);
    LIB_FUNCTION("xqmkjHCEOSY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillIsoTransfer);
    LIB_FUNCTION("Hvd3S--n25w", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeConfigDescriptor);
    LIB_FUNCTION("EQ6SCLMqzkM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeDeviceList);
    LIB_FUNCTION("-sgi7EeLSO8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeTransfer);
    LIB_FUNCTION("S1o1C6yOt5g", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetActiveConfigDescriptor);
    LIB_FUNCTION("t7WE9mb1TB8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetBusNumber);
    LIB_FUNCTION("Dkm5qe8j3XE", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetConfigDescriptor);
    LIB_FUNCTION("GQsAVJuy8gM", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetConfigDescriptorByValue);
    LIB_FUNCTION("L7FoTZp3bZs", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetConfiguration);
    LIB_FUNCTION("-JBoEtvTxvA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDescriptor);
    LIB_FUNCTION("rsl9KQ-agyA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDevice);
    LIB_FUNCTION("GjlCrU4GcIY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceAddress);
    LIB_FUNCTION("bhomgbiQgeo", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceDescriptor);
    LIB_FUNCTION("8qB9Ar4P5nc", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceList);
    LIB_FUNCTION("e1UWb8cWPJM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceSpeed);
    LIB_FUNCTION("vokkJ0aDf54", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetIsoPacketBuffer);
    LIB_FUNCTION("nuIRlpbxauM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetMaxIsoPacketSize);
    LIB_FUNCTION("YJ0cMAlLuxQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetMaxPacketSize);
    LIB_FUNCTION("g2oYm1DitDg", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetStringDescriptor);
    LIB_FUNCTION("t4gUfGsjk+g", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetStringDescriptorAscii);
    LIB_FUNCTION("EkqGLxWC-S0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEvents);
    LIB_FUNCTION("rt-WeUGibfg", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEventsLocked);
    LIB_FUNCTION("+wU6CGuZcWk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEventsTimeout);
    LIB_FUNCTION("TOhg7P6kTH4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdInit);
    LIB_FUNCTION("rxi1nCOKWc8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdInterruptTransfer);
    LIB_FUNCTION("RLf56F-WjKQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdKernelDriverActive);
    LIB_FUNCTION("u9yKks02-rA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdLockEvents);
    LIB_FUNCTION("AeGaY8JrAV4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdLockEventWaiters);
    LIB_FUNCTION("VJ6oMq-Di2U", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdOpen);
    LIB_FUNCTION("vrQXYRo1Gwk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdOpenDeviceWithVidPid);
    LIB_FUNCTION("U1t1SoJvV-A", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdRefDevice);
    LIB_FUNCTION("REfUTmTchMw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdReleaseInterface);
    LIB_FUNCTION("hvMn0QJXj5g", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdResetDevice);
    LIB_FUNCTION("FhU9oYrbXoA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetConfiguration);
    LIB_FUNCTION("DVCQW9o+ki0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetInterfaceAltSetting);
    LIB_FUNCTION("dJxro8Nzcjk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetIsoPacketLengths);
    LIB_FUNCTION("L0EHgZZNVas", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSubmitTransfer);
    LIB_FUNCTION("TcXVGc-LPbQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdTryLockEvents);
    LIB_FUNCTION("RA2D9rFH-Uw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnlockEvents);
    LIB_FUNCTION("1DkGvUQYFKI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnlockEventWaiters);
    LIB_FUNCTION("OULgIo1zAsA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnrefDevice);
    LIB_FUNCTION("ys2e9VRBPrY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdWaitForEvent);
    LIB_FUNCTION("ZfbvM+OP-1A", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_65F6EF33E38FFF50);
    LIB_FUNCTION("l-BWutkKrec", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_97F056BAD90AADE7);
    LIB_FUNCTION("xVEEozs1smQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_C55104A33B35B264);
    LIB_FUNCTION("1WtDBgcgseA", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_D56B43060720B1E0);
};

} // namespace Libraries::Usbd