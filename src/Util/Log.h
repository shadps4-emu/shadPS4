#pragma once

#include <spdlog/spdlog.h>

namespace logging {

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define LOG_TRACE_IF(flag, ...) \
    if (flag) LOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG_IF(flag, ...) \
    if (flag) LOG_DEBUG(__VA_ARGS__)
#define LOG_INFO_IF(flag, ...) \
    if (flag) LOG_INFO(__VA_ARGS__)
#define LOG_WARN_IF(flag, ...) \
    if (flag) LOG_WARN(__VA_ARGS__)
#define LOG_ERROR_IF(flag, ...) \
    if (flag) LOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL_IF(flag, ...) \
    if (flag) LOG_CRITICAL(__VA_ARGS__)

int init(bool use_stdout);
}  // namespace logging

// copyright vita3k emu https://github.com/Vita3K/Vita3K/blob/master/vita3k/util/include/util/log.h
/*
    returns: A string with the input number formatted in hexadecimal
    Examples:
        * `12` returns: `"0xC"`
        * `1337` returns: `"0x539"`
        * `72742069` returns: `"0x455F4B5"`
*/
template <typename T>
std::string log_hex(T val) {
    using unsigned_type = typename std::make_unsigned<T>::type;
    std::stringstream ss;
    ss << "0x";
    ss << std::hex << static_cast<unsigned_type>(val);
    return ss.str();
}

/*
    returns: A string with the input number formatted in hexadecimal with padding of the inputted type size
    Examples:
        * `uint8_t 5` returns: `"0x05"`
        * `uint8_t 15` returns: `"0x0F"`
        * `uint8_t 255` returns: `"0xFF"`

        * `uint16_t 15` returns: `"0x000F"`
        * `uint16_t 1337` returns: `"0x0539"`
        * `uint16_t 65535` returns: `"0xFFFF"`


        * `uint32_t 15` returns: `"0x0000000F"`
        * `uint32_t 1337` returns: `"0x00000539"`
        * `uint32_t 65535` returns: `"0x0000FFFF"`
        * `uint32_t 134217728` returns: `"0x08000000"`
*/
template <typename T>
std::string log_hex_full(T val) {
    std::stringstream ss;
    ss << "0x";
    ss << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << val;
    return ss.str();
}
