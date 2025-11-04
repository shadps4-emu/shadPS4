// SPDX-FileCopyrightText: Copyright 2014-2018 Marco Paland (info@paland.com)
// SPDX-License-Identifier: MIT

///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2018, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources.
//        Use this instead of bloated standard/newlib printf.
//        These routines are thread safe and reentrant!
//
///////////////////////////////////////////////////////////////////////////////
// Vita3K emulator project
// Copyright (C) 2023 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// copied from Vita3k project at 6/10/2023 (latest update 30/06/2023)
// modifications for adapting va_args parameters

#pragma once

#include <stdio.h>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "common/va_ctx.h"

namespace Libraries::LibcInternal {
// ntoa conversion buffer size, this must be big enough to hold
// one converted numeric number including padded zeros (dynamically created on stack)
// 32 byte is a good default
#define PRINTF_NTOA_BUFFER_SIZE 32U

// ftoa conversion buffer size, this must be big enough to hold
// one converted float number including padded zeros (dynamically created on stack)
// 32 byte is a good default
#define PRINTF_FTOA_BUFFER_SIZE 32U

// define this to support floating point (%f)
#define PRINTF_SUPPORT_FLOAT

// define this to support long long types (%llu or %p)
#define PRINTF_SUPPORT_LONG_LONG

// define this to support the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
#define PRINTF_SUPPORT_PTRDIFF_T

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD (1U << 0U)
#define FLAGS_LEFT (1U << 1U)
#define FLAGS_PLUS (1U << 2U)
#define FLAGS_SPACE (1U << 3U)
#define FLAGS_HASH (1U << 4U)
#define FLAGS_UPPERCASE (1U << 5U)
#define FLAGS_CHAR (1U << 6U)
#define FLAGS_SHORT (1U << 7U)
#define FLAGS_LONG (1U << 8U)
#define FLAGS_LONG_LONG (1U << 9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_WIDTH (1U << 11U)

// output function type
typedef void (*out_fct_type)(char character, void* buffer, size_t idx, size_t maxlen);

// wrapper (used as buffer) for output function type
typedef struct {
    void (*fct)(char character, void* arg);
    void* arg;
} out_fct_wrap_type;

// internal buffer output
static inline void _out_buffer(char character, void* buffer, size_t idx, size_t maxlen) {
    if (idx < maxlen) {
        ((char*)buffer)[idx] = character;
    }
}

// internal null output
static inline void _out_null(char character, void* buffer, size_t idx, size_t maxlen) {
    (void)character;
    (void)buffer;
    (void)idx;
    (void)maxlen;
}

// internal output function wrapper
static inline void _out_fct(char character, void* buffer, size_t idx, size_t maxlen) {
    (void)idx;
    (void)maxlen;
    // buffer is the output fct pointer
    ((out_fct_wrap_type*)buffer)->fct(character, ((out_fct_wrap_type*)buffer)->arg);
}

// internal strlen
// \return The length of the string (excluding the terminating 0)
static inline unsigned int _strlen(const char* str) {
    const char* s;
    for (s = str; *s; ++s)
        ;
    return (unsigned int)(s - str);
}

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline bool _is_digit(char ch) {
    return (ch >= '0') && (ch <= '9');
}

// internal ASCII string to unsigned int conversion
static inline unsigned int _atoi(const char** str) {
    unsigned int i = 0U;
    while (_is_digit(**str)) {
        i = i * 10U + (unsigned int)(*((*str)++) - '0');
    }
    return i;
}

// internal itoa format
static inline size_t _ntoa_format(out_fct_type out, char* buffer, size_t idx, size_t maxlen,
                                  char* buf, size_t len, bool negative, unsigned int base,
                                  unsigned int prec, unsigned int width, unsigned int flags) {
    const size_t start_idx = idx;

    // pad leading zeros
    while (!(flags & FLAGS_LEFT) && (len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
        buf[len++] = '0';
    }
    while (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD) && (len < width) &&
           (len < PRINTF_NTOA_BUFFER_SIZE)) {
        buf[len++] = '0';
    }

    // handle hash
    if (flags & FLAGS_HASH) {
        if (((len == prec) || (len == width)) && (len > 0U)) {
            len--;
            if ((base == 16U) && (len > 0U)) {
                len--;
            }
        }
        if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            buf[len++] = 'x';
        }
        if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            buf[len++] = 'X';
        }
        if (len < PRINTF_NTOA_BUFFER_SIZE) {
            buf[len++] = '0';
        }
    }

    // handle sign
    if ((len == width) && (negative || (flags & FLAGS_PLUS) || (flags & FLAGS_SPACE))) {
        len--;
    }
    if (len < PRINTF_NTOA_BUFFER_SIZE) {
        if (negative) {
            buf[len++] = '-';
        } else if (flags & FLAGS_PLUS) {
            buf[len++] = '+'; // ignore the space if the '+' exists
        } else if (flags & FLAGS_SPACE) {
            buf[len++] = ' ';
        }
    }

    // pad spaces up to given width
    if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
        for (size_t i = len; i < width; i++) {
            out(' ', buffer, idx++, maxlen);
        }
    }

    // reverse string
    for (size_t i = 0U; i < len; i++) {
        out(buf[len - i - 1U], buffer, idx++, maxlen);
    }

    // append pad spaces up to given width
    if (flags & FLAGS_LEFT) {
        while (idx - start_idx < width) {
            out(' ', buffer, idx++, maxlen);
        }
    }

    return idx;
}

// internal itoa for 'long' type
static inline size_t _ntoa_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen,
                                unsigned long value, bool negative, unsigned long base,
                                unsigned int prec, unsigned int width, unsigned int flags) {
    char buf[PRINTF_NTOA_BUFFER_SIZE];
    size_t len = 0U;

    // write if precision != 0 and value is != 0
    if (!(flags & FLAGS_PRECISION) || value) {
        do {
            const char digit = (char)(value % base);
            buf[len++] =
                digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
            value /= base;
        } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec,
                        width, flags);
}

// internal itoa for 'long long' type
#if defined(PRINTF_SUPPORT_LONG_LONG)
static inline size_t _ntoa_long_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen,
                                     unsigned long long value, bool negative,
                                     unsigned long long base, unsigned int prec, unsigned int width,
                                     unsigned int flags) {
    char buf[PRINTF_NTOA_BUFFER_SIZE];
    size_t len = 0U;

    // write if precision != 0 and value is != 0
    if (!(flags & FLAGS_PRECISION) || value) {
        do {
            const char digit = (char)(value % base);
            buf[len++] =
                digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
            value /= base;
        } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec,
                        width, flags);
}
#endif // PRINTF_SUPPORT_LONG_LONG

#if defined(PRINTF_SUPPORT_FLOAT)
static inline size_t _ftoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value,
                           unsigned int prec, unsigned int width, unsigned int flags) {
    char buf[PRINTF_FTOA_BUFFER_SIZE];
    size_t len = 0U;
    double diff = 0.0;

    // if input is larger than thres_max, revert to exponential
    const double thres_max = (double)0x7FFFFFFF;

    // powers of 10
    static const double pow10[] = {1,      10,      100,      1000,      10000,
                                   100000, 1000000, 10000000, 100000000, 1000000000};

    // test for negative
    bool negative = false;
    if (value < 0) {
        negative = true;
        value = 0 - value;
    }

    // set default precision to 6, if not set explicitly
    if (!(flags & FLAGS_PRECISION)) {
        prec = 6U;
    }
    // limit precision to 9, cause a prec >= 10 can lead to overflow errors
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U)) {
        buf[len++] = '0';
        prec--;
    }

    int whole = (int)value;
    double tmp = (value - whole) * pow10[prec];
    unsigned long frac = (unsigned long)tmp;
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        // handle rollover, e.g. case 0.99 with prec 1 is 1.0
        if (frac >= pow10[prec]) {
            frac = 0;
            ++whole;
        }
    } else if ((diff == 0.5) && ((frac == 0U) || (frac & 1U))) {
        // if halfway, round up if odd, OR if last digit is 0
        ++frac;
    }

    // TBD: for very large numbers switch back to native sprintf for exponentials. Anyone want to
    // write code to replace this? Normal printf behavior is to print EVERY whole number digit which
    // can be 100s of characters overflowing your buffers == bad
    if (value > thres_max) {
        return 0U;
    }

    if (prec == 0U) {
        diff = value - (double)whole;
        if (diff > 0.5) {
            // greater than 0.5, round up, e.g. 1.6 -> 2
            ++whole;
        } else if ((diff == 0.5) && (whole & 1)) {
            // exactly 0.5 and ODD, then round up
            // 1.5 -> 2, but 2.5 -> 2
            ++whole;
        }
    } else {
        unsigned int count = prec;
        // now do fractional part, as an unsigned number
        while (len < PRINTF_FTOA_BUFFER_SIZE) {
            --count;
            buf[len++] = (char)(48U + (frac % 10U));
            if (!(frac /= 10U)) {
                break;
            }
        }
        // add extra 0s
        while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U)) {
            buf[len++] = '0';
        }
        if (len < PRINTF_FTOA_BUFFER_SIZE) {
            // add decimal
            buf[len++] = '.';
        }
    }

    // do whole part, number is reversed
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
        buf[len++] = (char)(48 + (whole % 10));
        if (!(whole /= 10)) {
            break;
        }
    }

    // pad leading zeros
    while (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD) && (len < width) &&
           (len < PRINTF_FTOA_BUFFER_SIZE)) {
        buf[len++] = '0';
    }

    // handle sign
    if ((len == width) && (negative || (flags & FLAGS_PLUS) || (flags & FLAGS_SPACE))) {
        len--;
    }
    if (len < PRINTF_FTOA_BUFFER_SIZE) {
        if (negative) {
            buf[len++] = '-';
        } else if (flags & FLAGS_PLUS) {
            buf[len++] = '+'; // ignore the space if the '+' exists
        } else if (flags & FLAGS_SPACE) {
            buf[len++] = ' ';
        }
    }

    // pad spaces up to given width
    if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
        for (size_t i = len; i < width; i++) {
            out(' ', buffer, idx++, maxlen);
        }
    }

    // reverse string
    for (size_t i = 0U; i < len; i++) {
        out(buf[len - i - 1U], buffer, idx++, maxlen);
    }

    // append pad spaces up to given width
    if (flags & FLAGS_LEFT) {
        while (idx < width) {
            out(' ', buffer, idx++, maxlen);
        }
    }

    return idx;
}
#endif // PRINTF_SUPPORT_FLOAT

// internal vsnprintf
static inline int _vsnprintf(out_fct_type out, char* buffer, const char* format,
                             Common::VaList* va_list) {
    unsigned int flags, width, precision, n;
    size_t idx = 0U;
    auto maxlen = static_cast<size_t>(-1);

    if (!buffer) {
        // use null output function
        out = _out_null;
    }

    while (*format) {
        // format specifier?  %[flags][width][.precision][length]
        if (*format != '%') {
            // no
            out(*format, buffer, idx++, maxlen);
            format++;
            continue;
        } else {
            // yes, evaluate it
            format++;
        }

        // evaluate flags
        flags = 0U;
        do {
            switch (*format) {
            case '0':
                flags |= FLAGS_ZEROPAD;
                format++;
                n = 1U;
                break;
            case '-':
                flags |= FLAGS_LEFT;
                format++;
                n = 1U;
                break;
            case '+':
                flags |= FLAGS_PLUS;
                format++;
                n = 1U;
                break;
            case ' ':
                flags |= FLAGS_SPACE;
                format++;
                n = 1U;
                break;
            case '#':
                flags |= FLAGS_HASH;
                format++;
                n = 1U;
                break;
            default:
                n = 0U;
                break;
            }
        } while (n);

        // evaluate width field
        width = 0U;
        if (_is_digit(*format)) {
            width = _atoi(&format);
        } else if (*format == '*') {
            const int w = vaArgInteger(va_list); // const int w = va.next<int>(cpu, mem);

            if (w < 0) {
                flags |= FLAGS_LEFT; // reverse padding
                width = (unsigned int)-w;
            } else {
                width = (unsigned int)w;
            }
            format++;
        }

        // evaluate precision field
        precision = 0U;
        if (*format == '.') {
            flags |= FLAGS_PRECISION;
            format++;
            if (_is_digit(*format)) {
                precision = _atoi(&format);
            } else if (*format == '*') {
                precision =
                    vaArgInteger(va_list); // precision = (unsigned int)va.next<int>(cpu, mem);
                format++;
            }
        }

        // evaluate length field
        switch (*format) {
        case 'l':
            flags |= FLAGS_LONG;
            format++;
            if (*format == 'l') {
                flags |= FLAGS_LONG_LONG;
                format++;
            }
            break;
        case 'h':
            flags |= FLAGS_SHORT;
            format++;
            if (*format == 'h') {
                flags |= FLAGS_CHAR;
                format++;
            }
            break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
        case 't':
            flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
#endif
        case 'j':
            flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
        case 'z':
            flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
        default:
            break;
        }

        // evaluate specifier
        switch (*format) {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'b': {
            // set the base
            unsigned int base;
            if (*format == 'x' || *format == 'X') {
                base = 16U;
            } else if (*format == 'o') {
                base = 8U;
            } else if (*format == 'b') {
                base = 2U;
                flags &= ~FLAGS_HASH; // no hash for bin format
            } else {
                base = 10U;
                flags &= ~FLAGS_HASH; // no hash for dec format
            }
            // uppercase
            if (*format == 'X') {
                flags |= FLAGS_UPPERCASE;
            }

            // no plus or space flag for u, x, X, o, b
            if ((*format != 'i') && (*format != 'd')) {
                flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
            }

            // convert the integer
            if ((*format == 'i') || (*format == 'd')) {
                // signed
                if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
                    auto value = vaArgLongLong(
                        va_list); // const long long value = va.next<long long>(cpu, mem);
                    idx = _ntoa_long_long(out, buffer, idx, maxlen,
                                          (unsigned long long)(value > 0 ? value : 0 - value),
                                          value < 0, base, precision, width, flags);
#endif
                } else if (flags & FLAGS_LONG) {
                    auto value = vaArgLong(va_list); // const long value = va.next<long>(cpu, mem);
                    idx = _ntoa_long(out, buffer, idx, maxlen,
                                     (unsigned long)(value > 0 ? value : 0 - value), value < 0,
                                     base, precision, width, flags);
                } else {
                    // const int value = (flags & FLAGS_CHAR) ? (char)va.next<int>(cpu, mem) :
                    // (flags & FLAGS_SHORT) ? (short int)va.next<int>(cpu, mem): va.next<int>(cpu,
                    // mem);
                    const int value =
                        (flags & FLAGS_CHAR)    ? static_cast<char>(vaArgInteger(va_list))
                        : (flags & FLAGS_SHORT) ? static_cast<int16_t>(vaArgInteger(va_list))
                                                : vaArgInteger(va_list);
                    idx = _ntoa_long(out, buffer, idx, maxlen,
                                     (unsigned int)(value > 0 ? value : 0 - value), value < 0, base,
                                     precision, width, flags);
                }
            } else {
                // unsigned
                if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
                    // idx = _ntoa_long_long(out, buffer, idx, maxlen, va.next<unsigned long
                    // long>(cpu, mem), false, base, precision, width, flags);
                    idx = _ntoa_long_long(out, buffer, idx, maxlen,
                                          static_cast<u64>(vaArgLongLong(va_list)), false, base,
                                          precision, width, flags);
#endif
                } else if (flags & FLAGS_LONG) {
                    // idx = _ntoa_long(out, buffer, idx, maxlen, va.next<unsigned long>(cpu, mem),
                    // false, base, precision, width, flags);
                    idx = _ntoa_long(out, buffer, idx, maxlen, static_cast<u32>(vaArgLong(va_list)),
                                     false, base, precision, width, flags);
                } else {
                    // const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned
                    // char)va.next<unsigned int>(cpu, mem) : (flags & FLAGS_SHORT) ?
                    //     (unsigned short int)va.next<unsigned int>(cpu, mem) : va.next<unsigned
                    //     int>(cpu, mem);
                    const unsigned int value =
                        (flags & FLAGS_CHAR)    ? static_cast<u8>(vaArgInteger(va_list))
                        : (flags & FLAGS_SHORT) ? static_cast<u16>(vaArgInteger(va_list))
                                                : static_cast<u32>(vaArgInteger(va_list));
                    idx = _ntoa_long(out, buffer, idx, maxlen, value, false, base, precision, width,
                                     flags);
                }
            }
            format++;
            break;
        }
#if defined(PRINTF_SUPPORT_FLOAT)
        case 'f':
        case 'F':
            // idx = _ftoa(out, buffer, idx, maxlen, va.next<double>(cpu, mem), precision, width,
            // flags);
            idx = _ftoa(out, buffer, idx, maxlen, vaArgDouble(va_list), precision, width, flags);
            format++;
            break;
#endif // PRINTF_SUPPORT_FLOAT
        case 'c': {
            unsigned int l = 1U;
            // pre padding
            if (!(flags & FLAGS_LEFT)) {
                while (l++ < width) {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            // char output
            // out((char)va.next<int>(cpu, mem), buffer, idx++, maxlen);
            out(static_cast<char>(vaArgInteger(va_list)), buffer, idx++, maxlen);
            // post padding
            if (flags & FLAGS_LEFT) {
                while (l++ < width) {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            format++;
            break;
        }

        case 's': {
            const char* p = vaArgPtr<const char>(
                va_list); // const char *p = va.next<Ptr<char>>(cpu, mem).get(mem);
            p = p != nullptr ? p : "(null)";
            unsigned int l = _strlen(p);
            // pre padding
            if (flags & FLAGS_PRECISION) {
                l = (l < precision ? l : precision);
            }
            if (!(flags & FLAGS_LEFT)) {
                while (l++ < width) {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            // string output
            while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
                out(*(p++), buffer, idx++, maxlen);
            }
            // post padding
            if (flags & FLAGS_LEFT) {
                while (l++ < width) {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            format++;
            break;
        }

        case 'p': {
            width = sizeof(void*) * 2U;
            flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
            if (is_ll) {
                // idx = _ntoa_long_long(out, buffer, idx, maxlen,
                // (uintptr_t)va.next<Ptr<void>>(cpu, mem).address(), false, 16U, precision, width,
                // flags);
                idx = _ntoa_long_long(out, buffer, idx, maxlen,
                                      reinterpret_cast<uintptr_t>(vaArgPtr<void>(va_list)), false,
                                      16U, precision, width, flags);
            } else {
#endif
                // idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned
                // long)((uintptr_t)va.next<Ptr<void>>(cpu, mem).address()), false, 16U, precision,
                // width, flags);
                idx = _ntoa_long(
                    out, buffer, idx, maxlen,
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vaArgPtr<void>(va_list))),
                    false, 16U, precision, width, flags);
#if defined(PRINTF_SUPPORT_LONG_LONG)
            }
#endif
            format++;
            break;
        }

        case '%':
            out('%', buffer, idx++, maxlen);
            format++;
            break;

        default:
            out(*format, buffer, idx++, maxlen);
            format++;
            break;
        }
    }

    // termination
    out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

    // return written chars without terminating \0
    return (int)idx;
}

static int printf_ctx(Common::VaCtx* ctx) {
    const char* format = vaArgPtr<const char>(&ctx->va_list);
    char buffer[256];
    int result = _vsnprintf(_out_buffer, buffer, format, &ctx->va_list);
    printf("%s", buffer);
    return result;
}

static int fprintf_ctx(Common::VaCtx* ctx, char* buf) {
    const char* format = vaArgPtr<const char>(&ctx->va_list);
    char buffer[256];
    int result = _vsnprintf(_out_buffer, buffer, format, &ctx->va_list);
    std::strcpy(buf, buffer);
    return result;
}

static int vsnprintf_ctx(char* s, size_t n, const char* format, Common::VaList* arg) {
    std::vector<char> buffer(n);
    int result = _vsnprintf(_out_buffer, buffer.data(), format, arg);
    std::strcpy(s, buffer.data());
    return result;
}

static int snprintf_ctx(char* s, size_t n, Common::VaCtx* ctx) {
    const char* format = vaArgPtr<const char>(&ctx->va_list);
    std::vector<char> buffer(n);
    int result = _vsnprintf(_out_buffer, buffer.data(), format, &ctx->va_list);
    std::strcpy(s, buffer.data());
    return result;
}

} // namespace Libraries::LibcInternal