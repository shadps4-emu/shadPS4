// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h" 

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ssl2 {

int PS4_SYSV_ABI CA_MGMT_extractKeyBlobEx();
int PS4_SYSV_ABI CA_MGMT_extractPublicKeyInfo();
int PS4_SYSV_ABI CA_MGMT_freeKeyBlob();
int PS4_SYSV_ABI CRYPTO_initAsymmetricKey();
int PS4_SYSV_ABI CRYPTO_uninitAsymmetricKey();
int PS4_SYSV_ABI RSA_verifySignature();
int PS4_SYSV_ABI sceSslCheckRecvPending();
int PS4_SYSV_ABI sceSslClose();
int PS4_SYSV_ABI sceSslConnect();
int PS4_SYSV_ABI sceSslCreateConnection();
int PS4_SYSV_ABI sceSslCreateSslConnection();
int PS4_SYSV_ABI sceSslDeleteConnection();
int PS4_SYSV_ABI sceSslDeleteSslConnection();
int PS4_SYSV_ABI sceSslDisableOption();
int PS4_SYSV_ABI sceSslDisableOptionInternal();
int PS4_SYSV_ABI sceSslDisableOptionInternalInsecure();
int PS4_SYSV_ABI sceSslDisableVerifyOption();
int PS4_SYSV_ABI sceSslEnableOption();
int PS4_SYSV_ABI sceSslEnableOptionInternal();
int PS4_SYSV_ABI sceSslEnableVerifyOption();
int PS4_SYSV_ABI sceSslFreeCaCerts();
int PS4_SYSV_ABI sceSslFreeCaList();
int PS4_SYSV_ABI sceSslFreeSslCertName();
int PS4_SYSV_ABI sceSslGetAlpnSelected();
int PS4_SYSV_ABI sceSslGetCaCerts();
int PS4_SYSV_ABI sceSslGetCaList();
int PS4_SYSV_ABI sceSslGetFingerprint();
int PS4_SYSV_ABI sceSslGetIssuerName();
int PS4_SYSV_ABI sceSslGetMemoryPoolStats();
int PS4_SYSV_ABI sceSslGetNameEntryCount();
int PS4_SYSV_ABI sceSslGetNameEntryInfo();
int PS4_SYSV_ABI sceSslGetNanoSSLModuleId();
int PS4_SYSV_ABI sceSslGetNotAfter();
int PS4_SYSV_ABI sceSslGetNotBefore();
int PS4_SYSV_ABI sceSslGetPeerCert();
int PS4_SYSV_ABI sceSslGetPem();
int PS4_SYSV_ABI sceSslGetSerialNumber();
int PS4_SYSV_ABI sceSslGetSslError();
int PS4_SYSV_ABI sceSslGetSubjectName();
int PS4_SYSV_ABI sceSslInit();
int PS4_SYSV_ABI sceSslLoadCert();
int PS4_SYSV_ABI sceSslLoadRootCACert();
int PS4_SYSV_ABI sceSslRead();
int PS4_SYSV_ABI sceSslRecv();
int PS4_SYSV_ABI sceSslReuseConnection();
int PS4_SYSV_ABI sceSslSend();
int PS4_SYSV_ABI sceSslSetAlpn();
int PS4_SYSV_ABI sceSslSetMinSslVersion();
int PS4_SYSV_ABI sceSslSetSslVersion();
int PS4_SYSV_ABI sceSslSetVerifyCallback();
int PS4_SYSV_ABI sceSslTerm();
int PS4_SYSV_ABI sceSslUnloadCert();
int PS4_SYSV_ABI sceSslWrite();
int PS4_SYSV_ABI VLONG_freeVlongQueue();
int PS4_SYSV_ABI Func_22E76E60BC0587D7();
int PS4_SYSV_ABI Func_28F8791A771D39C7();

void RegisterlibSceSsl2(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ssl2