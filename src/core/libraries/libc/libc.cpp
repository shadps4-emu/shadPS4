// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include "common/debug.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/libc/libc.h"
#include "core/libraries/libc/libc_cxa.h"
#include "core/libraries/libc/libc_math.h"
#include "core/libraries/libc/libc_stdio.h"
#include "core/libraries/libc/libc_stdlib.h"
#include "core/libraries/libc/libc_string.h"
#include "core/libraries/libs.h"

namespace Libraries::LibC {

constexpr bool log_file_libc = true; // disable it to disable logging
static u32 g_need_sceLibc = 1;

using cxa_destructor_func_t = void (*)(void*);

struct CxaDestructor {
    cxa_destructor_func_t destructor_func;
    void* destructor_object;
    void* module_id;
};

struct CContext {
    std::vector<CxaDestructor> cxa;
};

static PS4_SYSV_ABI int ps4___cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
    auto* cc = Common::Singleton<CContext>::Instance();
    CxaDestructor c{};
    c.destructor_func = func;
    c.destructor_object = arg;
    c.module_id = dso_handle;
    cc->cxa.push_back(c);
    return 0;
}

void PS4_SYSV_ABI ps4___cxa_finalize(void* d) {
    BREAKPOINT();
}

void PS4_SYSV_ABI ps4___cxa_pure_virtual() {
    BREAKPOINT();
}

static PS4_SYSV_ABI void ps4_init_env() {
    LOG_INFO(Lib_LibC, "called");
}

static PS4_SYSV_ABI void ps4_catchReturnFromMain(int status) {
    LOG_INFO(Lib_LibC, "returned = {}", status);
}

static PS4_SYSV_ABI void ps4__Assert() {
    LOG_INFO(Lib_LibC, "called");
    BREAKPOINT();
}

PS4_SYSV_ABI void ps4__ZdlPv(void* ptr) {
    std::free(ptr);
}

PS4_SYSV_ABI void ps4__ZSt11_Xbad_allocv() {
    LOG_INFO(Lib_LibC, "called");
    BREAKPOINT();
}

PS4_SYSV_ABI void ps4__ZSt14_Xlength_errorPKc() {
    LOG_INFO(Lib_LibC, "called");
    BREAKPOINT();
}

PS4_SYSV_ABI void* ps4__Znwm(u64 count) {
    if (count == 0) {
        LOG_INFO(Lib_LibC, "_Znwm count ={}", count);
        BREAKPOINT();
    }
    void* ptr = std::malloc(count);
    return ptr;
}

static constexpr u16 lowercaseTable[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B,
    0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B,
    0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073,
    0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, 0x0080, 0x0081, 0x0082, 0x0083,
    0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B,
    0x009C, 0x009D, 0x009E, 0x009F, 0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2, 0x00B3,
    0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB,
    0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0, 0x00E1, 0x00E2, 0x00E3,
    0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB,
    0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

const PS4_SYSV_ABI u16* ps4__Getptolower() {
    return &lowercaseTable[0];
}

static constexpr u16 uppercaseTable[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B,
    0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B,
    0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053,
    0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B,
    0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, 0x0080, 0x0081, 0x0082, 0x0083,
    0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B,
    0x009C, 0x009D, 0x009E, 0x009F, 0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2, 0x00B3,
    0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB,
    0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0, 0x00E1, 0x00E2, 0x00E3,
    0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB,
    0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

const PS4_SYSV_ABI u16* ps4__Getptoupper() {
    return &uppercaseTable[0];
}

namespace CharacterType {
enum : u16 {
    HexDigit = 0x1,  // '0'-'9', 'A'-'F', 'a'-'f'
    Uppercase = 0x2, // 'A'-'Z'
    Space = 0x4,
    Punctuation = 0x08,
    Lowercase = 0x10,    // 'a'-'z'
    DecimalDigit = 0x20, // '0'-'9'
    Control = 0x40,      // CR, FF, HT, NL, VT
    Control2 = 0x80,     // BEL, BS, etc
    ExtraSpace = 0x100,
    ExtraAlphabetic = 0x200,
    ExtraBlank = 0x400
};
}

static constexpr u16 characterTypeTable[256] = {
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control | CharacterType::Control2 | CharacterType::ExtraBlank,
    CharacterType::Control | CharacterType::Control2,
    CharacterType::Control | CharacterType::Control2,
    CharacterType::Control | CharacterType::Control2,
    CharacterType::Control | CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Control2,
    CharacterType::Space,                                  //
    CharacterType::Punctuation,                            // !
    CharacterType::Punctuation,                            // "
    CharacterType::Punctuation,                            // #
    CharacterType::Punctuation,                            // $
    CharacterType::Punctuation,                            // %
    CharacterType::Punctuation,                            // &
    CharacterType::Punctuation,                            // '
    CharacterType::Punctuation,                            // (
    CharacterType::Punctuation,                            // )
    CharacterType::Punctuation,                            // *
    CharacterType::Punctuation,                            // +
    CharacterType::Punctuation,                            // ,
    CharacterType::Punctuation,                            // -
    CharacterType::Punctuation,                            // .
    CharacterType::Punctuation,                            // /
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 0
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 1
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 2
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 3
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 4
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 5
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 6
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 7
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 8
    CharacterType::HexDigit | CharacterType::DecimalDigit, // 9
    CharacterType::Punctuation,                            // :
    CharacterType::Punctuation,                            // ;
    CharacterType::Punctuation,                            // <
    CharacterType::Punctuation,                            // =
    CharacterType::Punctuation,                            // >
    CharacterType::Punctuation,                            // ?
    CharacterType::Punctuation,                            // @
    CharacterType::HexDigit | CharacterType::Uppercase,    // A
    CharacterType::HexDigit | CharacterType::Uppercase,    // B
    CharacterType::HexDigit | CharacterType::Uppercase,    // C
    CharacterType::HexDigit | CharacterType::Uppercase,    // D
    CharacterType::HexDigit | CharacterType::Uppercase,    // E
    CharacterType::HexDigit | CharacterType::Uppercase,    // F
    CharacterType::Uppercase,                              // G
    CharacterType::Uppercase,                              // H
    CharacterType::Uppercase,                              // I
    CharacterType::Uppercase,                              // J
    CharacterType::Uppercase,                              // K
    CharacterType::Uppercase,                              // L
    CharacterType::Uppercase,                              // M
    CharacterType::Uppercase,                              // N
    CharacterType::Uppercase,                              // O
    CharacterType::Uppercase,                              // P
    CharacterType::Uppercase,                              // Q
    CharacterType::Uppercase,                              // R
    CharacterType::Uppercase,                              // S
    CharacterType::Uppercase,                              // T
    CharacterType::Uppercase,                              // U
    CharacterType::Uppercase,                              // V
    CharacterType::Uppercase,                              // W
    CharacterType::Uppercase,                              // X
    CharacterType::Uppercase,                              // Y
    CharacterType::Uppercase,                              // Z
    CharacterType::Punctuation,                            // [
    CharacterType::Punctuation,                            //
    CharacterType::Punctuation,                            // ]
    CharacterType::Punctuation,                            // ^
    CharacterType::Punctuation,                            // _
    CharacterType::Punctuation,                            // `
    CharacterType::HexDigit | CharacterType::Lowercase,    // a
    CharacterType::HexDigit | CharacterType::Lowercase,    // b
    CharacterType::HexDigit | CharacterType::Lowercase,    // c
    CharacterType::HexDigit | CharacterType::Lowercase,    // d
    CharacterType::HexDigit | CharacterType::Lowercase,    // e
    CharacterType::HexDigit | CharacterType::Lowercase,    // f
    CharacterType::Lowercase,                              // g
    CharacterType::Lowercase,                              // h
    CharacterType::Lowercase,                              // i
    CharacterType::Lowercase,                              // j
    CharacterType::Lowercase,                              // k
    CharacterType::Lowercase,                              // l
    CharacterType::Lowercase,                              // m
    CharacterType::Lowercase,                              // n
    CharacterType::Lowercase,                              // o
    CharacterType::Lowercase,                              // p
    CharacterType::Lowercase,                              // q
    CharacterType::Lowercase,                              // r
    CharacterType::Lowercase,                              // s
    CharacterType::Lowercase,                              // t
    CharacterType::Lowercase,                              // u
    CharacterType::Lowercase,                              // v
    CharacterType::Lowercase,                              // w
    CharacterType::Lowercase,                              // x
    CharacterType::Lowercase,                              // y
    CharacterType::Lowercase,                              // z
    CharacterType::Punctuation,                            // {
    CharacterType::Punctuation,                            // |
    CharacterType::Punctuation,                            // }
    CharacterType::Punctuation,                            // ~
    CharacterType::Control2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

const PS4_SYSV_ABI u16* ps4__Getpctype() {
    return &characterTypeTable[0];
}

void libcSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    // cxa functions
    LIB_FUNCTION("3GPpjQdAMTw", "libc", 1, "libc", 1, 1, ps4___cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libc", 1, "libc", 1, 1, ps4___cxa_guard_release);
    LIB_FUNCTION("2emaaluWzUw", "libc", 1, "libc", 1, 1, ps4___cxa_guard_abort);

    // stdlib functions
    LIB_FUNCTION("uMei1W9uyNo", "libc", 1, "libc", 1, 1, ps4_exit);
    LIB_FUNCTION("8G2LB+A3rzg", "libc", 1, "libc", 1, 1, ps4_atexit);
    LIB_FUNCTION("gQX+4GDQjpM", "libc", 1, "libc", 1, 1, ps4_malloc);
    LIB_FUNCTION("tIhsqj0qsFE", "libc", 1, "libc", 1, 1, ps4_free);
    LIB_FUNCTION("cpCOXWMgha0", "libc", 1, "libc", 1, 1, ps4_rand);
    LIB_FUNCTION("AEJdIVZTEmo", "libc", 1, "libc", 1, 1, ps4_qsort);

    // math functions
    LIB_FUNCTION("EH-x713A99c", "libc", 1, "libc", 1, 1, ps4_atan2f);
    LIB_FUNCTION("QI-x0SL8jhw", "libc", 1, "libc", 1, 1, ps4_acosf);
    LIB_FUNCTION("ZE6RNL+eLbk", "libc", 1, "libc", 1, 1, ps4_tanf);
    LIB_FUNCTION("GZWjF-YIFFk", "libc", 1, "libc", 1, 1, ps4_asinf);
    LIB_FUNCTION("9LCjpWyQ5Zc", "libc", 1, "libc", 1, 1, ps4_pow);
    LIB_FUNCTION("cCXjU72Z0Ow", "libc", 1, "libc", 1, 1, ps4__Sin);
    LIB_FUNCTION("ZtjspkJQ+vw", "libc", 1, "libc", 1, 1, ps4__Fsin);
    LIB_FUNCTION("dnaeGXbjP6E", "libc", 1, "libc", 1, 1, ps4_exp2);
    LIB_FUNCTION("1D0H2KNjshE", "libc", 1, "libc", 1, 1, ps4_powf);
    LIB_FUNCTION("DDHG1a6+3q0", "libc", 1, "libc", 1, 1, ps4_roundf);

    // string functions
    LIB_FUNCTION("Ovb2dSJOAuE", "libc", 1, "libc", 1, 1, ps4_strcmp);
    LIB_FUNCTION("j4ViWNHEgww", "libc", 1, "libc", 1, 1, ps4_strlen);
    LIB_FUNCTION("6sJWiWSRuqk", "libc", 1, "libc", 1, 1, ps4_strncpy);
    LIB_FUNCTION("+P6FRGH4LfA", "libc", 1, "libc", 1, 1, ps4_memmove);
    LIB_FUNCTION("kiZSXIWd9vg", "libc", 1, "libc", 1, 1, ps4_strcpy);
    LIB_FUNCTION("Ls4tzzhimqQ", "libc", 1, "libc", 1, 1, ps4_strcat);
    LIB_FUNCTION("DfivPArhucg", "libc", 1, "libc", 1, 1, ps4_memcmp);
    LIB_FUNCTION("Q3VBxCXhUHs", "libc", 1, "libc", 1, 1, ps4_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libc", 1, "libc", 1, 1, ps4_memset);
    LIB_FUNCTION("9yDWMxEFdJU", "libc", 1, "libc", 1, 1, ps4_strrchr);
    LIB_FUNCTION("aesyjrHVWy4", "libc", 1, "libc", 1, 1, ps4_strncmp);
    LIB_FUNCTION("g7zzzLDYGw0", "libc", 1, "libc", 1, 1, ps4_strdup);

    // stdio functions
    LIB_FUNCTION("xeYO4u7uyJ0", "libc", 1, "libc", 1, 1, ps4_fopen);
    // LIB_FUNCTION("hcuQgD53UxM", "libc", 1, "libc", 1, 1, ps4_printf);
    LIB_FUNCTION("Q2V+iqvjgC0", "libc", 1, "libc", 1, 1, ps4_vsnprintf);
    LIB_FUNCTION("YQ0navp+YIc", "libc", 1, "libc", 1, 1, ps4_puts);
    // LIB_FUNCTION("fffwELXNVFA", "libc", 1, "libc", 1, 1, ps4_fprintf);
    LIB_FUNCTION("QMFyLoqNxIg", "libc", 1, "libc", 1, 1, ps4_setvbuf);
    LIB_FUNCTION("uodLYyUip20", "libc", 1, "libc", 1, 1, ps4_fclose);
    LIB_FUNCTION("rQFVBXp-Cxg", "libc", 1, "libc", 1, 1, ps4_fseek);
    LIB_FUNCTION("SHlt7EhOtqA", "libc", 1, "libc", 1, 1, ps4_fgetpos);
    LIB_FUNCTION("lbB+UlZqVG0", "libc", 1, "libc", 1, 1, ps4_fread);
    LIB_FUNCTION("Qazy8LmXTvw", "libc", 1, "libc", 1, 1, ps4_ftell);

    // misc
    LIB_OBJ("P330P3dFF68", "libc", 1, "libc", 1, 1, &g_need_sceLibc);
    LIB_OBJ("2sWzhYqFH4E", "libc", 1, "libc", 1, 1, stdout);
    LIB_OBJ("H8AprKeZtNg", "libc", 1, "libc", 1, 1, stderr);
    LIB_FUNCTION("bzQExy189ZI", "libc", 1, "libc", 1, 1, ps4_init_env);
    LIB_FUNCTION("XKRegsFpEpk", "libc", 1, "libc", 1, 1, ps4_catchReturnFromMain);
    LIB_FUNCTION("-QgqOT5u2Vk", "libc", 1, "libc", 1, 1, ps4__Assert);
    LIB_FUNCTION("z+P+xCnWLBk", "libc", 1, "libc", 1, 1, ps4__ZdlPv);
    LIB_FUNCTION("eT2UsmTewbU", "libc", 1, "libc", 1, 1, ps4__ZSt11_Xbad_allocv);
    LIB_FUNCTION("tQIo+GIPklo", "libc", 1, "libc", 1, 1, ps4__ZSt14_Xlength_errorPKc);
    LIB_FUNCTION("fJnpuVVBbKk", "libc", 1, "libc", 1, 1, ps4__Znwm);
    LIB_FUNCTION("tsvEmnenz48", "libc", 1, "libc", 1, 1, ps4___cxa_atexit);
    LIB_FUNCTION("H2e8t5ScQGc", "libc", 1, "libc", 1, 1, ps4___cxa_finalize);
    LIB_FUNCTION("zr094EQ39Ww", "libc", 1, "libc", 1, 1, ps4___cxa_pure_virtual);
    LIB_FUNCTION("1uJgoVq3bQU", "libc", 1, "libc", 1, 1, ps4__Getptolower);
    LIB_FUNCTION("rcQCUr0EaRU", "libc", 1, "libc", 1, 1, ps4__Getptoupper);
    LIB_FUNCTION("sUP1hBaouOw", "libc", 1, "libc", 1, 1, ps4__Getpctype);
}

}; // namespace Libraries::LibC
