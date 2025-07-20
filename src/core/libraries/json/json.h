// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Json {

s32 PS4_SYSV_ABI _ZN3sce4Json11Initializer10initializeEPKNS0_13InitParameterE();
s32 PS4_SYSV_ABI _ZN3sce4Json11Initializer9terminateEv();
s32 PS4_SYSV_ABI _ZN3sce4Json11InitializerC1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json11InitializerC2Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json11InitializerD1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json11InitializerD2Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json11s_initparamE();
s32 PS4_SYSV_ABI _ZN3sce4Json4FreeEPv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value10referArrayEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value10referValueEm();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value10referValueERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value10s_nullboolE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value10s_nullrealE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value11referObjectEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value11referStringEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value11s_nullarrayE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value12referBooleanEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value12referIntegerEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value12s_nullobjectE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value12s_nullstringE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value13referUIntegerEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value13s_nullintegerE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value14s_nulluintegerE();
s32 PS4_SYSV_ABI
_ZN3sce4Json5Value18serialize_internalERSbIcSt11char_traitsIcENS0_8StlAllocIcEEEPFiS7_PvES8_PS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setEb();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setEd();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setEl();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setEm();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setENS0_9ValueTypeE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setERKS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI
_ZN3sce4Json5Value3setERKSt3mapISbIcSt11char_traitsIcENS0_8StlAllocIcEEES1_St4lessIS7_ENS5_ISt4pairIS7_S1_EEEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value3setERKSt4listIS1_NS0_8StlAllocIS1_EEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value4swapERS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value5clearEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value9referRealEv();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value9serializeERSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI
_ZN3sce4Json5Value9serializeERSbIcSt11char_traitsIcENS0_8StlAllocIcEEEPFiS7_PvES8_();
s32 PS4_SYSV_ABI _ZN3sce4Json5Value9setParentEPKS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueaSERKS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1Eb();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1Ed();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1El();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1Em();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1ENS0_9ValueTypeE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1ERKS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1ERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI
_ZN3sce4Json5ValueC1ERKSt3mapISbIcSt11char_traitsIcENS0_8StlAllocIcEEES1_St4lessIS7_ENS5_ISt4pairIS7_S1_EEEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1ERKSt4listIS1_NS0_8StlAllocIS1_EEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2Eb();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2Ed();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2El();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2Em();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2ENS0_9ValueTypeE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2ERKS1_();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2ERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI
_ZN3sce4Json5ValueC2ERKSt3mapISbIcSt11char_traitsIcENS0_8StlAllocIcEEES1_St4lessIS7_ENS5_ISt4pairIS7_S1_EEEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2ERKSt4listIS1_NS0_8StlAllocIS1_EEE();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueC2Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueD1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json5ValueD2Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json6MallocEm();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser10parseArrayERNS0_5ValueERNS0_11InputStreamEPS2_();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser10parseValueERNS0_5ValueERNS0_11InputStreamEPS2_();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser11parseNumberERNS0_5ValueERNS0_11InputStreamEPS2_();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser11parseObjectERNS0_5ValueERNS0_11InputStreamEPS2_();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser11parseStringERNS0_5ValueERNS0_11InputStreamEPS2_();
s32 PS4_SYSV_ABI
_ZN3sce4Json6Parser11parseStringERSbIcSt11char_traitsIcENS0_8StlAllocIcEEERNS0_11InputStreamE();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser12parseQuadHexERNS0_11InputStreamE();
s32 PS4_SYSV_ABI
_ZN3sce4Json6Parser14parseCodePointERSbIcSt11char_traitsIcENS0_8StlAllocIcEEERNS0_11InputStreamE();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser5parseERNS0_5ValueEPFiRcPvES5_();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser5parseERNS0_5ValueEPKc();
s32 PS4_SYSV_ABI _ZN3sce4Json6Parser5parseERNS0_5ValueEPKcm();
s32 PS4_SYSV_ABI _ZN3sce4Json9RootParamC1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json9RootParamC2Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json9RootParamD1Ev();
s32 PS4_SYSV_ABI _ZN3sce4Json9RootParamD2Ev();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value10getBooleanEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value10getIntegerEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value11getUIntegerEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value5countEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value7getRealEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value7getRootEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value7getTypeEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value8getArrayEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value8getValueEm();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value8getValueERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value8toStringERSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value9getObjectEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5Value9getStringEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5ValuecvbEv();
s32 PS4_SYSV_ABI _ZNK3sce4Json5ValueixEm();
s32 PS4_SYSV_ABI _ZNK3sce4Json5ValueixEPKc();
s32 PS4_SYSV_ABI _ZNK3sce4Json5ValueixERKSbIcSt11char_traitsIcENS0_8StlAllocIcEEE();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Json