#pragma once

using s08 = char;
using s16 = short;
using s32 = int;
using s64 = long long;

using u08 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

#define PS4_SYSV_ABI __attribute__((sysv_abi))


// UDLs for memory size values
constexpr u64 operator""_KB(u64 x) { return 1024ULL * x; }
constexpr u64 operator""_MB(u64 x) { return 1024_KB * x; }
constexpr u64 operator""_GB(u64 x) { return 1024_MB * x; }