// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/ssl.h"

namespace Libraries::Ssl {

int PS4_SYSV_ABI CA_MGMT_allocCertDistinguishedName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_certDistinguishedNameCompare() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_convertKeyBlobToPKCS8Key() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_convertKeyDER() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_convertKeyPEM() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_convertPKCS8KeyToKeyBlob() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_convertProtectedPKCS8KeyToKeyBlob() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_decodeCertificate() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_enumAltName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_enumCrl() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractAllCertDistinguishedName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractBasicConstraint() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractCertASN1Name() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractCertTimes() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractKeyBlobEx() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractKeyBlobTypeEx() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractPublicKeyInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractSerialNum() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_extractSignature() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_free() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_freeCertDistinguishedName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_freeCertDistinguishedNameOnStack() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_freeCertificate() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_freeKeyBlob() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_freeSearchDetails() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_getCertSignAlgoType() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_keyBlobToDER() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_keyBlobToPEM() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_makeKeyBlobEx() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_rawVerifyOID() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_reorderChain() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_returnCertificatePrints() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_verifyCertWithKeyBlob() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CA_MGMT_verifySignature() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_checkCertificateIssuer() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_checkCertificateIssuer2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_checkCertificateIssuerSerialNumber() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_CompSubjectAltNames() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_CompSubjectAltNames2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_CompSubjectAltNamesExact() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_CompSubjectCommonName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_CompSubjectCommonName2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_ComputeBufferHash() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_decryptRSASignature() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_decryptRSASignatureBuffer() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_enumerateAltName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_enumerateAltName2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_enumerateCRL() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_enumerateCRL2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_enumerateCRLAux() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractAllDistinguishedNames() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractDistinguishedNames() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractDistinguishedNames2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractDistinguishedNamesFromName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractRSAKey() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractSerialNum() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractSerialNum2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractValidityTime() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_extractValidityTime2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertExtension() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateExtensions() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateExtensions2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateIssuerSerialNumber() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateIssuerSerialNumber2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateKeyUsage() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateKeyUsage2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateSubject() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertificateSubject2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getCertSignAlgoType() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_GetCertTime() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getNumberOfChild() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getRSASignatureAlgo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getSignatureItem() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getSubjectCommonName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_getSubjectCommonName2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_isRootCertificate() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_isRootCertificate2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_rawVerifyOID() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_rawVerifyOID2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_setKeyFromSubjectPublicKeyInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_setKeyFromSubjectPublicKeyInfoCert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addCertAuthority() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addIdentity() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addIdentityNakedKey() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addIdentityPSK() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addIdentityWithCertificateChain() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_addTrustPoint() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_createStore() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findCertBySubject() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findIdentityByTypeFirst() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findIdentityByTypeNext() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findIdentityCertChainFirst() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findIdentityCertChainNext() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_findPskByIdentity() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_releaseStore() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_traversePskListHead() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_STORE_traversePskListNext() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_validateCertificate() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_validateCertificateWithConf() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_VerifyCertificatePolicies() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_VerifyCertificatePolicies2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_verifySignature() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_VerifyValidityTime() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_VerifyValidityTime2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CERT_VerifyValidityTimeWithConf() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CRYPTO_initAsymmetricKey() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI CRYPTO_uninitAsymmetricKey() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI GC_createInstanceIDs() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI getCertSigAlgo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI MOCANA_freeMocana() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI MOCANA_initMocana() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI RSA_verifySignature() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslCheckRecvPending() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslClose() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslConnect() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslCreateSslConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslDeleteSslConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslDisableOption() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslDisableOptionInternal() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslDisableOptionInternalInsecure() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslEnableOption() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslEnableOptionInternal() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslFreeCaCerts() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslFreeCaList() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslFreeSslCertName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetCaCerts() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetCaList() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetIssuerName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetMemoryPoolStats() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetNameEntryCount() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetNameEntryInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetNanoSSLModuleId() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetNotAfter() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetNotBefore() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetSerialNumber() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetSslError() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslGetSubjectName() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslInit(std::size_t poolSize) {
    LOG_ERROR(Lib_Ssl, "(DUMMY) called poolSize = {}", poolSize);
    // return a value >1
    static int id = 0;
    return ++id;
}

int PS4_SYSV_ABI sceSslLoadCert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslLoadRootCACert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslRecv() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslReuseConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslSend() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslSetMinSslVersion() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslSetSslVersion() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslSetVerifyCallback() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslShowMemoryStat() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslTerm() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSslUnloadCert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_acceptConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_acceptConnectionCommon() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_assignCertificateStore() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_acceptConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_closeConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_connect() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_connectCommon() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_getRecvBuffer() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_getSendBuffer() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_init() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_initServer() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_recvMessage() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_recvMessage2() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_sendMessage() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_sendMessagePending() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ASYNC_start() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_closeConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_connect() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_connectWithCfgParam() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_enableCiphers() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_findConnectionInstance() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getCipherInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getClientRandom() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getClientSessionInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getCookie() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getNextSessionId() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getServerRandom() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSessionCache() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSessionFlags() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSessionInfo() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSessionStatus() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSocketId() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_getSSLTLSVersion() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_init() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_initiateRehandshake() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_initServerCert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_ioctl() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_isSessionSSL() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_lockSessionCacheMutex() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_lookupAlert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_negotiateConnection() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_recv() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_recvPending() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_releaseTables() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_retrieveServerNameList() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_rngFun() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_send() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_sendAlert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_sendPending() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_setCookie() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_setServerCert() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_setServerNameList() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_setSessionFlags() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_shutdown() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_sslSettings() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI SSL_validateCertParam() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI VLONG_freeVlongQueue() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_22E76E60BC0587D7() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_28F8791A771D39C7() {
    LOG_ERROR(Lib_Ssl, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Pgt0gg14ewU", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_allocCertDistinguishedName);
    LIB_FUNCTION("wJ5jCpkCv-c", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_certDistinguishedNameCompare);
    LIB_FUNCTION("Vc2tb-mWu78", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_convertKeyBlobToPKCS8Key);
    LIB_FUNCTION("IizpdlgPdpU", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_convertKeyDER);
    LIB_FUNCTION("Y-5sBnpVclY", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_convertKeyPEM);
    LIB_FUNCTION("jb6LuBv9weg", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_convertPKCS8KeyToKeyBlob);
    LIB_FUNCTION("ExsvtKwhWoM", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_convertProtectedPKCS8KeyToKeyBlob);
    LIB_FUNCTION("AvoadUUK03A", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_decodeCertificate);
    LIB_FUNCTION("S0DCFBqmhQY", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_enumAltName);
    LIB_FUNCTION("Xt+SprLPiVQ", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_enumCrl);
    LIB_FUNCTION("4HzS6Vkd-uU", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_extractAllCertDistinguishedName);
    LIB_FUNCTION("W80mmhRKtH8", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractBasicConstraint);
    LIB_FUNCTION("7+F9pr5g26Q", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractCertASN1Name);
    LIB_FUNCTION("KsvuhF--f6k", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractCertTimes);
    LIB_FUNCTION("Md+HYkCBZB4", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractKeyBlobEx);
    LIB_FUNCTION("rFiChDgHkGQ", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractKeyBlobTypeEx);
    LIB_FUNCTION("9bKYzKP6kYU", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractPublicKeyInfo);
    LIB_FUNCTION("xXCqbDBx6mA", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractSerialNum);
    LIB_FUNCTION("xakUpzS9qv0", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_extractSignature);
    LIB_FUNCTION("m7EXDQRv7NU", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_free);
    LIB_FUNCTION("64t1HKepy1Q", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_freeCertDistinguishedName);
    LIB_FUNCTION("d7AAqdK2IDo", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CA_MGMT_freeCertDistinguishedNameOnStack);
    LIB_FUNCTION("PysF6pUcK-o", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_freeCertificate);
    LIB_FUNCTION("ipLIammTj2Q", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_freeKeyBlob);
    LIB_FUNCTION("C05CUtDViqU", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_freeSearchDetails);
    LIB_FUNCTION("tq511UiaNlE", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_getCertSignAlgoType);
    LIB_FUNCTION("1e46hRscIE8", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_keyBlobToDER);
    LIB_FUNCTION("5U2j47T1l70", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_keyBlobToPEM);
    LIB_FUNCTION("+oCOy8+4at8", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_makeKeyBlobEx);
    LIB_FUNCTION("YMbRl6PNq5U", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_rawVerifyOID);
    LIB_FUNCTION("O+JTn8Dwan8", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_reorderChain);
    LIB_FUNCTION("he6CvWiX3iM", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_returnCertificatePrints);
    LIB_FUNCTION("w5ZBRGN1lzY", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_verifyCertWithKeyBlob);
    LIB_FUNCTION("5e5rj-coUv8", "libSceSsl", 1, "libSceSsl", 1, 1, CA_MGMT_verifySignature);
    LIB_FUNCTION("6nH53ruuckc", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_checkCertificateIssuer);
    LIB_FUNCTION("MB3EExhoaJQ", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_checkCertificateIssuer2);
    LIB_FUNCTION("sDUV9VsqJD8", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_checkCertificateIssuerSerialNumber);
    LIB_FUNCTION("FXCfp5CwcPk", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_CompSubjectAltNames);
    LIB_FUNCTION("szJ8gsZdoHE", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_CompSubjectAltNames2);
    LIB_FUNCTION("1aewkTBcGEY", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_CompSubjectAltNamesExact);
    LIB_FUNCTION("gdWmmelQC1k", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_CompSubjectCommonName);
    LIB_FUNCTION("6Z-n6acrhTs", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_CompSubjectCommonName2);
    LIB_FUNCTION("p12OhhUCGEE", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_ComputeBufferHash);
    LIB_FUNCTION("5G+Z9vXPWYU", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_decryptRSASignature);
    LIB_FUNCTION("WZCBPnvf0fw", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_decryptRSASignatureBuffer);
    LIB_FUNCTION("AvjnXHAa7G0", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_enumerateAltName);
    LIB_FUNCTION("goUd71Bv0lk", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_enumerateAltName2);
    LIB_FUNCTION("tf3dP8kVauc", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_enumerateCRL);
    LIB_FUNCTION("noRFMfbcI-g", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_enumerateCRL2);
    LIB_FUNCTION("Xy4cdu44Xr0", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_enumerateCRLAux);
    LIB_FUNCTION("2FPKT8OxHxo", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_extractAllDistinguishedNames);
    LIB_FUNCTION("xyd+kSAhtSw", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractDistinguishedNames);
    LIB_FUNCTION("BQIv6mcPFRM", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractDistinguishedNames2);
    LIB_FUNCTION("nxcdqUGDgW8", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_extractDistinguishedNamesFromName);
    LIB_FUNCTION("u82YRvIENeo", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractRSAKey);
    LIB_FUNCTION("HBWarJFXoCM", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractSerialNum);
    LIB_FUNCTION("8Lemumnt1-8", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractSerialNum2);
    LIB_FUNCTION("JhanUiHOg-M", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractValidityTime);
    LIB_FUNCTION("6ocfVwswH-E", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_extractValidityTime2);
    LIB_FUNCTION("8FqgR3V7gHs", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertExtension);
    LIB_FUNCTION("sRIARmcXPHE", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateExtensions);
    LIB_FUNCTION("ABAA2f3PM8k", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateExtensions2);
    LIB_FUNCTION("CATkBsr20tY", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_getCertificateIssuerSerialNumber);
    LIB_FUNCTION("JpnKObUJsxQ", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_getCertificateIssuerSerialNumber2);
    LIB_FUNCTION("jp75ki1UzRU", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateKeyUsage);
    LIB_FUNCTION("prSVrFdvQiU", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateKeyUsage2);
    LIB_FUNCTION("8+UPqcEgsYg", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateSubject);
    LIB_FUNCTION("X-rqVhPnKJI", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertificateSubject2);
    LIB_FUNCTION("Pt3o1t+hh1g", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getCertSignAlgoType);
    LIB_FUNCTION("oNJNApmHV+M", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_GetCertTime);
    LIB_FUNCTION("GCPUCV9k1Mg", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getNumberOfChild);
    LIB_FUNCTION("lCB1AE4xSkE", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getRSASignatureAlgo);
    LIB_FUNCTION("+7U74Zy7gKg", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getSignatureItem);
    LIB_FUNCTION("hOABTkhp6NM", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getSubjectCommonName);
    LIB_FUNCTION("3CECWZfBTVg", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_getSubjectCommonName2);
    LIB_FUNCTION("OP-VhFdtkmo", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_isRootCertificate);
    LIB_FUNCTION("0iwGE4M4DU8", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_isRootCertificate2);
    LIB_FUNCTION("pWg3+mTkoTI", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_rawVerifyOID);
    LIB_FUNCTION("HofoEUZ5mOM", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_rawVerifyOID2);
    LIB_FUNCTION("w2lGr-89zLc", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_setKeyFromSubjectPublicKeyInfo);
    LIB_FUNCTION("OeGeb9Njons", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_setKeyFromSubjectPublicKeyInfoCert);
    LIB_FUNCTION("N+UDju8zxtE", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_addCertAuthority);
    LIB_FUNCTION("pIZfvPaYmrs", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_addIdentity);
    LIB_FUNCTION("D6QBgLq-nlc", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_addIdentityNakedKey);
    LIB_FUNCTION("uAHc6pgeFaQ", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_addIdentityPSK);
    LIB_FUNCTION("xdxuhUkYalI", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_STORE_addIdentityWithCertificateChain);
    LIB_FUNCTION("OcZJcxANLfw", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_addTrustPoint);
    LIB_FUNCTION("gu0eRZMqTu8", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_createStore);
    LIB_FUNCTION("s1tJ1zBkky4", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_findCertBySubject);
    LIB_FUNCTION("4aXDehFZLDA", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_STORE_findIdentityByTypeFirst);
    LIB_FUNCTION("K-g87UhrYQ8", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_STORE_findIdentityByTypeNext);
    LIB_FUNCTION("ULOVCAVPJE4", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_STORE_findIdentityCertChainFirst);
    LIB_FUNCTION("uS9P+bSWOC0", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_STORE_findIdentityCertChainNext);
    LIB_FUNCTION("k3RI-YRkW-M", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_findPskByIdentity);
    LIB_FUNCTION("AloU5nLupdU", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_releaseStore);
    LIB_FUNCTION("gAHkf68L6+0", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_traversePskListHead);
    LIB_FUNCTION("w2CtqF+x7og", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_STORE_traversePskListNext);
    LIB_FUNCTION("GTSbNvpE1fQ", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_validateCertificate);
    LIB_FUNCTION("j6Wk8AtmVQM", "libSceSsl", 1, "libSceSsl", 1, 1,
                 CERT_validateCertificateWithConf);
    LIB_FUNCTION("wdl-XapuxKU", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_VerifyCertificatePolicies);
    LIB_FUNCTION("BQah1z-QS-w", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_VerifyCertificatePolicies2);
    LIB_FUNCTION("GPRMLcwyslw", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_verifySignature);
    LIB_FUNCTION("CAgB8oEGwsY", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_VerifyValidityTime);
    LIB_FUNCTION("3wferxuMV6Y", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_VerifyValidityTime2);
    LIB_FUNCTION("UO2a3+5CCCs", "libSceSsl", 1, "libSceSsl", 1, 1, CERT_VerifyValidityTimeWithConf);
    LIB_FUNCTION("PRWr3-ytpdg", "libSceSsl", 1, "libSceSsl", 1, 1, CRYPTO_initAsymmetricKey);
    LIB_FUNCTION("cW7VCIMCh9A", "libSceSsl", 1, "libSceSsl", 1, 1, CRYPTO_uninitAsymmetricKey);
    LIB_FUNCTION("u+brAYVFGUs", "libSceSsl", 1, "libSceSsl", 1, 1, GC_createInstanceIDs);
    LIB_FUNCTION("pOmcRglskbI", "libSceSsl", 1, "libSceSsl", 1, 1, getCertSigAlgo);
    LIB_FUNCTION("uBqy-2-dQ-A", "libSceSsl", 1, "libSceSsl", 1, 1, MOCANA_freeMocana);
    LIB_FUNCTION("U3NHH12yORo", "libSceSsl", 1, "libSceSsl", 1, 1, MOCANA_initMocana);
    LIB_FUNCTION("pBwtarKd7eg", "libSceSsl", 1, "libSceSsl", 1, 1, RSA_verifySignature);
    LIB_FUNCTION("1VM0h1JrUfA", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslCheckRecvPending);
    LIB_FUNCTION("viRXSHZYd0c", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslClose);
    LIB_FUNCTION("zXvd6iNyfgc", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslConnect);
    LIB_FUNCTION("P14ATpXc4J8", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslCreateSslConnection);
    LIB_FUNCTION("hwrHV6Pprk4", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslDeleteSslConnection);
    LIB_FUNCTION("iLKz4+ukLqk", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslDisableOption);
    LIB_FUNCTION("-WqxBRAUVM4", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslDisableOptionInternal);
    LIB_FUNCTION("w1+L-27nYas", "libSceSsl", 1, "libSceSsl", 1, 1,
                 sceSslDisableOptionInternalInsecure);
    LIB_FUNCTION("m-zPyAsIpco", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslEnableOption);
    LIB_FUNCTION("g-zCwUKstEQ", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslEnableOptionInternal);
    LIB_FUNCTION("qIvLs0gYxi0", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslFreeCaCerts);
    LIB_FUNCTION("+DzXseDVkeI", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslFreeCaList);
    LIB_FUNCTION("RwXD8grHZHM", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslFreeSslCertName);
    LIB_FUNCTION("TDfQqO-gMbY", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetCaCerts);
    LIB_FUNCTION("qOn+wm28wmA", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetCaList);
    LIB_FUNCTION("7whYpYfHP74", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetIssuerName);
    LIB_FUNCTION("-PoIzr3PEk0", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetMemoryPoolStats);
    LIB_FUNCTION("R1ePzopYPYM", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetNameEntryCount);
    LIB_FUNCTION("7RBSTKGrmDA", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetNameEntryInfo);
    LIB_FUNCTION("AzUipl-DpIw", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetNanoSSLModuleId);
    LIB_FUNCTION("xHpt6+2pGYk", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetNotAfter);
    LIB_FUNCTION("Eo0S65Jy28Q", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetNotBefore);
    LIB_FUNCTION("DOwXL+FQMEY", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetSerialNumber);
    LIB_FUNCTION("0XcZknp7-Wc", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetSslError);
    LIB_FUNCTION("dQReuBX9sD8", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslGetSubjectName);
    LIB_FUNCTION("hdpVEUDFW3s", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslInit);
    LIB_FUNCTION("Ab7+DH+gYyM", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslLoadCert);
    LIB_FUNCTION("3-643mGVFJo", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslLoadRootCACert);
    LIB_FUNCTION("hi0veU3L2pU", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslRecv);
    LIB_FUNCTION("50R2xYaYZwE", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslReuseConnection);
    LIB_FUNCTION("p5bM5PPufFY", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslSend);
    LIB_FUNCTION("QWSxBzf6lAg", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslSetMinSslVersion);
    LIB_FUNCTION("bKaEtQnoUuQ", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslSetSslVersion);
    LIB_FUNCTION("E4a-ahM57QQ", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslSetVerifyCallback);
    LIB_FUNCTION("lnHFrZV5zAY", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslShowMemoryStat);
    LIB_FUNCTION("0K1yQ6Lv-Yc", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslTerm);
    LIB_FUNCTION("UQ+3Qu7v3cA", "libSceSsl", 1, "libSceSsl", 1, 1, sceSslUnloadCert);
    LIB_FUNCTION("26lYor6xrR4", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_acceptConnection);
    LIB_FUNCTION("iHBiYOSciqY", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_acceptConnectionCommon);
    LIB_FUNCTION("budJurAYNHc", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_assignCertificateStore);
    LIB_FUNCTION("dCRcdgdoIEI", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_acceptConnection);
    LIB_FUNCTION("KI5jhdvg2S8", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_closeConnection);
    LIB_FUNCTION("hk+NcQTQlqI", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_connect);
    LIB_FUNCTION("rKD5kXcvN0E", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_connectCommon);
    LIB_FUNCTION("Fxq5MuWRkSw", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_getRecvBuffer);
    LIB_FUNCTION("vCpt1jyL6C4", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_getSendBuffer);
    LIB_FUNCTION("wZp1hBtjV1I", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_init);
    LIB_FUNCTION("P+O-4XCIODs", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_initServer);
    LIB_FUNCTION("GfDzwBDRl3M", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_recvMessage);
    LIB_FUNCTION("oM5w6Fb4TWM", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_recvMessage2);
    LIB_FUNCTION("dim5NDlc7Vs", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_sendMessage);
    LIB_FUNCTION("Qq0o-+hobOI", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_sendMessagePending);
    LIB_FUNCTION("y+ZFCsZYNME", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ASYNC_start);
    LIB_FUNCTION("5g9cNS3IFCk", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_closeConnection);
    LIB_FUNCTION("i9AvJK-l5Jk", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_connect);
    LIB_FUNCTION("mgs+n71u35Y", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_connectWithCfgParam);
    LIB_FUNCTION("4hPwsDmVKZc", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_enableCiphers);
    LIB_FUNCTION("yUd2ukhZLJI", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_findConnectionInstance);
    LIB_FUNCTION("J7LWSdYo0Zg", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getCipherInfo);
    LIB_FUNCTION("kRb0lquIrj0", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getClientRandom);
    LIB_FUNCTION("sSD8SHia8Zc", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getClientSessionInfo);
    LIB_FUNCTION("eT7n5lcEYCc", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getCookie);
    LIB_FUNCTION("2Irwf6Oqt4E", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getNextSessionId);
    LIB_FUNCTION("s9qIeprVILk", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getServerRandom);
    LIB_FUNCTION("NRoSvM1VPm8", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSessionCache);
    LIB_FUNCTION("dHosoPLXaMw", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSessionFlags);
    LIB_FUNCTION("7QgvTqUGFlU", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSessionInfo);
    LIB_FUNCTION("ufoBDuHGOlM", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSessionStatus);
    LIB_FUNCTION("EAoybreRrGU", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSocketId);
    LIB_FUNCTION("ElUzZAXIvY0", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_getSSLTLSVersion);
    LIB_FUNCTION("Wi9eDU54UCU", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_init);
    LIB_FUNCTION("BSqmh5B4KTg", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_initiateRehandshake);
    LIB_FUNCTION("xIFe7m4wqX4", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_initServerCert);
    LIB_FUNCTION("zlMZOG3VDYg", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_ioctl);
    LIB_FUNCTION("fje5RYUa+2g", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_isSessionSSL);
    LIB_FUNCTION("IKENWUUd8bk", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_lockSessionCacheMutex);
    LIB_FUNCTION("n6-12LafAeA", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_lookupAlert);
    LIB_FUNCTION("H4Z3ShBNjSA", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_negotiateConnection);
    LIB_FUNCTION("9PTAJclcW50", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_recv);
    LIB_FUNCTION("NrZz0ZgQrao", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_recvPending);
    LIB_FUNCTION("SHInb+l58Bs", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_releaseTables);
    LIB_FUNCTION("f0MBRCQeOEg", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_retrieveServerNameList);
    LIB_FUNCTION("6J0PLGaYl0Y", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_rngFun);
    LIB_FUNCTION("MoaZ6-hDS-k", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_send);
    LIB_FUNCTION("H02lfd0hCG0", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_sendAlert);
    LIB_FUNCTION("nXlhepw9ztI", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_sendPending);
    LIB_FUNCTION("Bf0pzkQc6CU", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_setCookie);
    LIB_FUNCTION("dSP1n53RtVw", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_setServerCert);
    LIB_FUNCTION("kNIvrkD-XJk", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_setServerNameList);
    LIB_FUNCTION("pbTq-nEsN1w", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_setSessionFlags);
    LIB_FUNCTION("-UDxVMs9h9M", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_shutdown);
    LIB_FUNCTION("nH9FVvfZhCs", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_sslSettings);
    LIB_FUNCTION("2Bd7UoCRhQ8", "libSceSsl", 1, "libSceSsl", 1, 1, SSL_validateCertParam);
    LIB_FUNCTION("wcVuyTUr5ys", "libSceSsl", 1, "libSceSsl", 1, 1, VLONG_freeVlongQueue);
    LIB_FUNCTION("IuduYLwFh9c", "libSceSsl", 1, "libSceSsl", 1, 1, Func_22E76E60BC0587D7);
    LIB_FUNCTION("KPh5GncdOcc", "libSceSsl", 1, "libSceSsl", 1, 1, Func_28F8791A771D39C7);
};

} // namespace Libraries::Ssl
