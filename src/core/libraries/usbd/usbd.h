// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Usbd {

int PS4_SYSV_ABI sceUsbdAllocTransfer();
int PS4_SYSV_ABI sceUsbdAttachKernelDriver();
int PS4_SYSV_ABI sceUsbdBulkTransfer();
int PS4_SYSV_ABI sceUsbdCancelTransfer();
int PS4_SYSV_ABI sceUsbdCheckConnected();
int PS4_SYSV_ABI sceUsbdClaimInterface();
int PS4_SYSV_ABI sceUsbdClearHalt();
int PS4_SYSV_ABI sceUsbdClose();
int PS4_SYSV_ABI sceUsbdControlTransfer();
int PS4_SYSV_ABI sceUsbdControlTransferGetData();
int PS4_SYSV_ABI sceUsbdControlTransferGetSetup();
int PS4_SYSV_ABI sceUsbdDetachKernelDriver();
int PS4_SYSV_ABI sceUsbdEventHandlerActive();
int PS4_SYSV_ABI sceUsbdEventHandlingOk();
int PS4_SYSV_ABI sceUsbdExit();
int PS4_SYSV_ABI sceUsbdFillBulkTransfer();
int PS4_SYSV_ABI sceUsbdFillControlSetup();
int PS4_SYSV_ABI sceUsbdFillControlTransfer();
int PS4_SYSV_ABI sceUsbdFillInterruptTransfer();
int PS4_SYSV_ABI sceUsbdFillIsoTransfer();
int PS4_SYSV_ABI sceUsbdFreeConfigDescriptor();
int PS4_SYSV_ABI sceUsbdFreeDeviceList();
int PS4_SYSV_ABI sceUsbdFreeTransfer();
int PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor();
int PS4_SYSV_ABI sceUsbdGetBusNumber();
int PS4_SYSV_ABI sceUsbdGetConfigDescriptor();
int PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue();
int PS4_SYSV_ABI sceUsbdGetConfiguration();
int PS4_SYSV_ABI sceUsbdGetDescriptor();
int PS4_SYSV_ABI sceUsbdGetDevice();
int PS4_SYSV_ABI sceUsbdGetDeviceAddress();
int PS4_SYSV_ABI sceUsbdGetDeviceDescriptor();
int PS4_SYSV_ABI sceUsbdGetDeviceList();
int PS4_SYSV_ABI sceUsbdGetDeviceSpeed();
int PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer();
int PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize();
int PS4_SYSV_ABI sceUsbdGetMaxPacketSize();
int PS4_SYSV_ABI sceUsbdGetStringDescriptor();
int PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii();
int PS4_SYSV_ABI sceUsbdHandleEvents();
int PS4_SYSV_ABI sceUsbdHandleEventsLocked();
int PS4_SYSV_ABI sceUsbdHandleEventsTimeout();
int PS4_SYSV_ABI sceUsbdInit();
int PS4_SYSV_ABI sceUsbdInterruptTransfer();
int PS4_SYSV_ABI sceUsbdKernelDriverActive();
int PS4_SYSV_ABI sceUsbdLockEvents();
int PS4_SYSV_ABI sceUsbdLockEventWaiters();
int PS4_SYSV_ABI sceUsbdOpen();
int PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid();
int PS4_SYSV_ABI sceUsbdRefDevice();
int PS4_SYSV_ABI sceUsbdReleaseInterface();
int PS4_SYSV_ABI sceUsbdResetDevice();
int PS4_SYSV_ABI sceUsbdSetConfiguration();
int PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting();
int PS4_SYSV_ABI sceUsbdSetIsoPacketLengths();
int PS4_SYSV_ABI sceUsbdSubmitTransfer();
int PS4_SYSV_ABI sceUsbdTryLockEvents();
int PS4_SYSV_ABI sceUsbdUnlockEvents();
int PS4_SYSV_ABI sceUsbdUnlockEventWaiters();
int PS4_SYSV_ABI sceUsbdUnrefDevice();
int PS4_SYSV_ABI sceUsbdWaitForEvent();
int PS4_SYSV_ABI Func_65F6EF33E38FFF50();
int PS4_SYSV_ABI Func_97F056BAD90AADE7();
int PS4_SYSV_ABI Func_C55104A33B35B264();
int PS4_SYSV_ABI Func_D56B43060720B1E0();

void RegisterlibSceUsbd(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Usbd