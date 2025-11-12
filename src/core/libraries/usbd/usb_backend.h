//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>

#include <libusb.h>

#include "common/assert.h"
#include "common/types.h"

namespace Libraries::Usbd {

#if defined(_WIN32)
typedef CRITICAL_SECTION usbi_mutex_t;
#else
typedef pthread_mutex_t usbi_mutex_t;
#endif

struct list_head {
    struct list_head *prev, *next;
};

// Forward declared libusb structs
struct UsbDeviceHandle {
    usbi_mutex_t lock;
    unsigned long claimed_interfaces;
    struct list_head list;
    struct libusb_device* dev;
    int auto_detach_kernel_driver;
};

struct UsbDevice {
    volatile long refcnt;

    struct libusb_context* ctx;
    struct libusb_device* parent_dev;

    u8 bus_number;
    u8 port_number;
    u8 device_address;
    enum libusb_speed speed;

    struct list_head list;
    unsigned long session_data;

    struct libusb_device_descriptor device_descriptor;
    volatile long attached;
};

class UsbEmulatedImpl {
public:
    UsbEmulatedImpl() = default;

    virtual void LoadFigure(std::string file_name, u8 pad, u8 slot) = 0;
    virtual void RemoveFigure(u8 pad, u8 slot, bool full_remove) = 0;
    virtual void MoveFigure(u8 new_pad, u8 new_index, u8 old_pad, u8 old_index) = 0;
    virtual void TempRemoveFigure(u8 index) = 0;
    virtual void CancelRemoveFigure(u8 index) = 0;

protected:
    virtual ~UsbEmulatedImpl() = default;
};

class UsbBackend {
public:
    UsbBackend() = default;

    virtual s32 Init() = 0;
    virtual void Exit() = 0;

    virtual s64 GetDeviceList(libusb_device*** list) = 0;
    virtual void FreeDeviceList(libusb_device** list, s32 unref_devices) = 0;

    virtual s32 GetConfiguration(libusb_device_handle* dev, s32* config) = 0;
    virtual s32 GetDeviceDescriptor(libusb_device* dev, libusb_device_descriptor* desc) = 0;
    virtual s32 GetActiveConfigDescriptor(libusb_device* dev,
                                          libusb_config_descriptor** config) = 0;
    virtual s32 GetConfigDescriptor(libusb_device* dev, u8 config_index,
                                    libusb_config_descriptor** config) = 0;
    virtual void FreeConfigDescriptor(libusb_config_descriptor* config) = 0;

    virtual u8 GetBusNumber(libusb_device* dev) = 0;
    virtual u8 GetDeviceAddress(libusb_device* dev) = 0;
    virtual s32 GetMaxPacketSize(libusb_device* dev, u8 endpoint) = 0;

    virtual s32 OpenDevice(libusb_device* dev, libusb_device_handle** dev_handle) = 0;
    virtual void CloseDevice(libusb_device_handle* dev_handle) = 0;
    virtual libusb_device* GetDevice(libusb_device_handle* dev_handle) = 0;

    virtual s32 SetConfiguration(libusb_device_handle* dev_handle, s32 configuration) = 0;
    virtual s32 ClaimInterface(libusb_device_handle* dev_handle, s32 interface_number) = 0;
    virtual libusb_device_handle* OpenDeviceWithVidPid(u16 vendor_id, u16 product_id) = 0;
    virtual s32 ResetDevice(libusb_device_handle* dev_handle) = 0;

    virtual s32 KernelDriverActive(libusb_device_handle* dev_handle, s32 interface_number) = 0;
    virtual s32 DetachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) = 0;
    virtual s32 AttachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) = 0;

    virtual s32 ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType, u8 bRequest,
                                u16 wValue, u16 wIndex, u8* data, u16 wLength, u32 timeout) = 0;
    virtual s32 SubmitTransfer(libusb_transfer* transfer) = 0;

    virtual s32 TryLockEvents() = 0;
    virtual void LockEvents() = 0;
    virtual void UnlockEvents() = 0;
    virtual s32 EventHandlingOk() = 0;
    virtual s32 EventHandlerActive() = 0;
    virtual void LockEventWaiters() = 0;
    virtual void UnlockEventWaiters() = 0;
    virtual s32 WaitForEvent(timeval* tv) = 0;

    virtual s32 HandleEventsTimeout(timeval* tv) = 0;
    virtual s32 HandleEvents() = 0;
    virtual s32 HandleEventsLocked(timeval* tv) = 0;

    virtual s32 CheckConnected(libusb_device_handle* dev) = 0;
    virtual std::shared_ptr<UsbEmulatedImpl> GetImplRef() = 0;

protected:
    virtual ~UsbBackend() = default;
};

class UsbRealBackend : public UsbBackend {
public:
    s32 Init() override {
        return libusb_init(&g_libusb_context);
    }
    void Exit() override {
        libusb_exit(g_libusb_context);
    }

    s64 GetDeviceList(libusb_device*** list) override {
        return libusb_get_device_list(g_libusb_context, list);
    }
    void FreeDeviceList(libusb_device** list, s32 unref_devices) override {
        libusb_free_device_list(list, unref_devices);
    }

    s32 GetConfiguration(libusb_device_handle* dev, s32* config) override {
        return libusb_get_configuration(dev, config);
    }
    s32 GetDeviceDescriptor(libusb_device* dev, libusb_device_descriptor* desc) override {
        return libusb_get_device_descriptor(dev, desc);
    }
    s32 GetActiveConfigDescriptor(libusb_device* dev, libusb_config_descriptor** config) override {
        return libusb_get_active_config_descriptor(dev, config);
    }
    s32 GetConfigDescriptor(libusb_device* dev, u8 config_index,
                            libusb_config_descriptor** config) override {
        return libusb_get_config_descriptor(dev, config_index, config);
    }
    void FreeConfigDescriptor(libusb_config_descriptor* config) override {
        libusb_free_config_descriptor(config);
    }

    u8 GetBusNumber(libusb_device* dev) override {
        return libusb_get_bus_number(dev);
    }
    u8 GetDeviceAddress(libusb_device* dev) override {
        return libusb_get_device_address(dev);
    }
    s32 GetMaxPacketSize(libusb_device* dev, u8 endpoint) override {
        return libusb_get_max_packet_size(dev, endpoint);
    }

    s32 OpenDevice(libusb_device* dev, libusb_device_handle** dev_handle) override {
        return libusb_open(dev, dev_handle);
    }
    void CloseDevice(libusb_device_handle* dev_handle) override {
        libusb_close(dev_handle);
    }
    libusb_device* GetDevice(libusb_device_handle* dev_handle) override {
        return libusb_get_device(dev_handle);
    }

    s32 SetConfiguration(libusb_device_handle* dev_handle, s32 configuration) override {
        return libusb_set_configuration(dev_handle, configuration);
    }
    s32 ClaimInterface(libusb_device_handle* dev_handle, s32 interface_number) override {
        return libusb_claim_interface(dev_handle, interface_number);
    }
    libusb_device_handle* OpenDeviceWithVidPid(u16 vendor_id, u16 product_id) override {
        return libusb_open_device_with_vid_pid(g_libusb_context, vendor_id, product_id);
    }
    s32 ResetDevice(libusb_device_handle* dev_handle) override {
        return libusb_reset_device(dev_handle);
    }

    s32 KernelDriverActive(libusb_device_handle* dev_handle, s32 interface_number) override {
#if defined(_WIN32) || defined(__APPLE__)
        return s_has_removed_driver ? 0 : 1;
#else
        return libusb_kernel_driver_active(dev_handle, interface_number);
#endif
    }
    s32 DetachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) override {
#if defined(_WIN32) || defined(__APPLE__)
        s_has_removed_driver = true;
        return LIBUSB_SUCCESS;
#else
        return libusb_detach_kernel_driver(dev_handle, interface_number);
#endif
    }
    s32 AttachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) override {
#if defined(_WIN32) || defined(__APPLE__)
        s_has_removed_driver = false;
        return LIBUSB_SUCCESS;
#else
        return libusb_attach_kernel_driver(dev_handle, interface_number);
#endif
    }

    s32 ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType, u8 bRequest, u16 wValue,
                        u16 wIndex, u8* data, u16 wLength, u32 timeout) override {
        return libusb_control_transfer(dev_handle, bmRequestType, bRequest, wValue, wIndex, data,
                                       wLength, timeout);
    }
    s32 SubmitTransfer(libusb_transfer* transfer) override {
        return libusb_submit_transfer(transfer);
    }

    s32 TryLockEvents() override {
        return libusb_try_lock_events(g_libusb_context);
    }
    void LockEvents() override {
        return libusb_lock_events(g_libusb_context);
    }
    void UnlockEvents() override {
        return libusb_unlock_events(g_libusb_context);
    }
    s32 EventHandlingOk() override {
        return libusb_event_handling_ok(g_libusb_context);
    }
    s32 EventHandlerActive() override {
        return libusb_event_handler_active(g_libusb_context);
    }
    void LockEventWaiters() override {
        return libusb_lock_event_waiters(g_libusb_context);
    }
    void UnlockEventWaiters() override {
        return libusb_unlock_event_waiters(g_libusb_context);
    }
    s32 WaitForEvent(timeval* tv) override {
        return libusb_wait_for_event(g_libusb_context, tv);
    }

    s32 HandleEventsTimeout(timeval* tv) override {
        return libusb_handle_events_timeout(g_libusb_context, tv);
    }
    s32 HandleEvents() override {
        return libusb_handle_events(g_libusb_context);
    }
    s32 HandleEventsLocked(timeval* tv) override {
        return libusb_handle_events_locked(g_libusb_context, tv);
    }

    s32 CheckConnected(libusb_device_handle* dev) override {
        // There's no libusb version of this function.
        // Simulate by querying data.

        int config;
        return libusb_get_configuration(dev, &config);
    }

    std::shared_ptr<UsbEmulatedImpl> GetImplRef() override {
        return nullptr;
    }

protected:
    libusb_context* g_libusb_context = nullptr;
    bool s_has_removed_driver = false;
};

class UsbEmulatedBackend : public UsbRealBackend {
public:
    s64 GetDeviceList(libusb_device*** list) override {
        auto** fake = static_cast<libusb_device**>(calloc(2, sizeof(libusb_device*)));
        fake[0] = GetDevice(nullptr);
        fake[1] = nullptr;
        *list = fake;

        return 1;
    }
    void FreeDeviceList(libusb_device** list, s32 unref_devices) override {
        if (!list) {
            return;
        }

        if (unref_devices) {
            int i = 0;
            libusb_device* dev;

            while ((dev = list[i++]) != nullptr) {
                free(dev);
            }
        }
        free(list);
    }

    s32 GetConfiguration(libusb_device_handle* dev, s32* config) override {
        config = nullptr;
        return LIBUSB_SUCCESS;
    }

    s32 GetDeviceDescriptor(libusb_device* dev, libusb_device_descriptor* desc) override {
        std::memcpy(desc, FillDeviceDescriptor(), sizeof(libusb_device_descriptor));
        return LIBUSB_SUCCESS;
    }
    s32 GetActiveConfigDescriptor(libusb_device* dev, libusb_config_descriptor** config) override {
        const auto endpoint_descs = FillEndpointDescriptorPair();
        const auto interface_desc = FillInterfaceDescriptor(endpoint_descs);

        const auto interface = static_cast<libusb_interface*>(calloc(1, sizeof(libusb_interface)));
        interface->altsetting = interface_desc;
        interface->num_altsetting = 1;

        const auto new_config = FillConfigDescriptor(interface);

        ASSERT(endpoint_descs && interface_desc && new_config);
        *config = new_config;
        return LIBUSB_SUCCESS;
    }
    s32 GetConfigDescriptor(libusb_device* dev, u8 config_index,
                            libusb_config_descriptor** config) override {
        if (config_index >= 1) {
            return LIBUSB_ERROR_NOT_FOUND;
        }
        return GetActiveConfigDescriptor(dev, config);
    }
    void FreeConfigDescriptor(libusb_config_descriptor* config) override {
        // Member variable reference, don't free
    }

    u8 GetBusNumber(libusb_device* dev) override {
        return 0;
    }
    u8 GetDeviceAddress(libusb_device* dev) override {
        return 0;
    }
    s32 GetMaxPacketSize(libusb_device* dev, u8 endpoint) override {
        libusb_device_descriptor* desc = nullptr;

        int r = GetDeviceDescriptor(dev, desc);
        if (r < LIBUSB_SUCCESS) {
            return LIBUSB_ERROR_OTHER;
        }
        return desc->bMaxPacketSize0;
    }

    s32 OpenDevice(libusb_device* dev, libusb_device_handle** dev_handle) override {
        auto* _dev_handle = static_cast<UsbDeviceHandle*>(calloc(1, sizeof(libusb_device_handle*)));
        if (!_dev_handle) {
            return LIBUSB_ERROR_NO_MEM;
        }
        _dev_handle->dev = dev;
        *dev_handle = reinterpret_cast<libusb_device_handle*>(_dev_handle);
        return LIBUSB_SUCCESS;
    }
    void CloseDevice(libusb_device_handle* dev_handle) override {
        LOG_WARNING(Lib_Usbd, "Guest decided to close device, might be an implementation issue");
        free(dev_handle);
    }
    libusb_device* GetDevice(libusb_device_handle* dev_handle) override {
        const auto desc = FillDeviceDescriptor();
        ASSERT(desc);

        const auto fake = static_cast<UsbDevice*>(calloc(1, sizeof(UsbDevice)));
        fake->bus_number = 0;
        fake->port_number = 0;
        fake->device_address = 0;
        fake->device_descriptor = *desc;
        fake->ctx = g_libusb_context;
        return reinterpret_cast<libusb_device*>(fake);
    }

    s32 SetConfiguration(libusb_device_handle* dev_handle, s32 configuration) override {
        return LIBUSB_SUCCESS;
    }
    s32 ClaimInterface(libusb_device_handle* dev_handle, s32 interface_number) override {
        return LIBUSB_SUCCESS;
    }
    libusb_device_handle* OpenDeviceWithVidPid(u16 vendor_id, u16 product_id) override {
        libusb_device_handle* dev_handle = nullptr;
        OpenDevice(GetDevice(nullptr), &dev_handle);
        return dev_handle;
    }
    s32 ResetDevice(libusb_device_handle* dev_handle) override {
        LOG_WARNING(Lib_Usbd, "Guest decided to reset device, might be an implementation issue");
        return LIBUSB_SUCCESS;
    }

    s32 KernelDriverActive(libusb_device_handle* dev_handle, s32 interface_number) override {
        return s_has_removed_driver ? 0 : 1;
    }
    s32 DetachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) override {
        s_has_removed_driver = true;
        return LIBUSB_SUCCESS;
    }
    s32 AttachKernelDriver(libusb_device_handle* dev_handle, s32 interface_number) override {
        s_has_removed_driver = false;
        return LIBUSB_SUCCESS;
    }

    s32 ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType, u8 bRequest, u16 wValue,
                        u16 wIndex, u8* data, u16 wLength, u32 timeout) override {
        LOG_WARNING(Lib_Usbd, "Backend has not handled control transfers");
        return LIBUSB_ERROR_PIPE;
    }
    s32 SubmitTransfer(libusb_transfer* transfer) override {
        ASSERT(transfer->type == LIBUSB_TRANSFER_TYPE_INTERRUPT);

        // if we have no other flying transfers, start the list with this one
        if (flight_list.empty()) {
            flight_list.push_front(transfer);
            return LIBUSB_SUCCESS;
        }

        // if we have infinite timeout, append to end of list
        if (transfer->timeout == 0) {
            flight_list.push_back(transfer);
            return LIBUSB_SUCCESS;
        }

        // otherwise, find appropriate place in list
        for (auto it = flight_list.begin(); it != flight_list.end(); ++it) {
            if ((*it)->timeout > transfer->timeout) {
                flight_list.insert(it, transfer);
                return LIBUSB_SUCCESS;
            }
        }

        // otherwise we need to be inserted at the end
        flight_list.push_back(transfer);
        return LIBUSB_SUCCESS;
    }

    s32 HandleEventsTimeout(timeval* tv) override {
        return HandleEvents();
    }
    s32 HandleEvents() override {
        if (!flight_list.empty()) {
            const auto transfer = flight_list.front();

            const u8 flags = transfer->flags;
            transfer->status = HandleAsyncTransfer(transfer);
            transfer->actual_length = transfer->length;
            if (transfer->callback) {
                transfer->callback(transfer);
            }
            if (flags & LIBUSB_TRANSFER_FREE_TRANSFER) {
                libusb_free_transfer(transfer);
            }
            flight_list.pop_front();
        }

        return LIBUSB_SUCCESS;
    }

    s32 CheckConnected(libusb_device_handle* dev) override {
        return LIBUSB_SUCCESS;
    }

protected:
    virtual libusb_endpoint_descriptor* FillEndpointDescriptorPair() = 0;
    virtual libusb_interface_descriptor* FillInterfaceDescriptor(
        libusb_endpoint_descriptor* descs) = 0;
    virtual libusb_config_descriptor* FillConfigDescriptor(libusb_interface* inter) = 0;
    virtual libusb_device_descriptor* FillDeviceDescriptor() = 0;

    virtual libusb_transfer_status HandleAsyncTransfer(libusb_transfer* transfer) = 0;

    std::list<libusb_transfer*> flight_list;
};

} // namespace Libraries::Usbd
