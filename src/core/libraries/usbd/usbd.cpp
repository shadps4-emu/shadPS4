// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "usbd.h"

namespace Libraries::Usbd {

int PS4_SYSV_ABI sceUsbdAllocTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdAttachKernelDriver() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdBulkTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdCancelTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdCheckConnected() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdClaimInterface() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdClearHalt() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdClose() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdControlTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdControlTransferGetData() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdControlTransferGetSetup() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdDetachKernelDriver() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdEventHandlerActive() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdEventHandlingOk() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdExit() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillBulkTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillControlSetup() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillControlTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillInterruptTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillIsoTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFreeConfigDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFreeDeviceList() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFreeTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetBusNumber() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetConfigDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetConfiguration() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDeviceAddress() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDeviceDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDeviceList() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDeviceSpeed() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetMaxPacketSize() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetStringDescriptor() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdHandleEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdHandleEventsLocked() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdHandleEventsTimeout() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdInit() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return 0x80240005; // Skip
}

int PS4_SYSV_ABI sceUsbdInterruptTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdKernelDriverActive() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdLockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdLockEventWaiters() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdOpen() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdRefDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdReleaseInterface() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdResetDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdSetConfiguration() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdSetIsoPacketLengths() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdSubmitTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdTryLockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdUnlockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdUnlockEventWaiters() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdUnrefDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdWaitForEvent() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_65F6EF33E38FFF50() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_97F056BAD90AADE7() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C55104A33B35B264() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D56B43060720B1E0() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
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
