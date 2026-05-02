// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ampr/ampr.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <unordered_map>

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"

namespace Libraries::Ampr {
namespace {

constexpr u64 CommandBufferHeaderSize = 0x28;
constexpr u64 CommandBufferSelfOffset = 0x00;
constexpr u64 CommandBufferDataOffset = 0x08;
constexpr u64 CommandBufferSizeOffset = 0x10;
constexpr u64 CommandBufferAux0Offset = 0x18;
constexpr u64 CommandBufferAux1Offset = 0x20;
constexpr u64 ReadFileRecordSize = 0x30;

struct CommandBufferState {
    VAddr buffer = 0;
    u64 size = 0;
    u64 write_offset = 0;
};

std::mutex g_command_buffer_mutex;
std::unordered_map<VAddr, CommandBufferState> g_command_buffers;

bool IsValidGuestRange(VAddr addr, u64 size) {
    if (size == 0) {
        return true;
    }
    return addr != 0 && Core::Memory::Instance()->IsValidMapping(addr, size);
}

template <typename T>
bool WriteGuest(VAddr addr, const T& value) {
    if (!IsValidGuestRange(addr, sizeof(T))) {
        return false;
    }
    std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(T));
    return true;
}

template <typename T>
bool ReadGuest(VAddr addr, T& value) {
    if (!IsValidGuestRange(addr, sizeof(T))) {
        return false;
    }
    std::memcpy(&value, reinterpret_cast<const void*>(addr), sizeof(T));
    return true;
}

bool WriteVisibleCommandBufferPointers(VAddr command_buffer, VAddr buffer, u64 size) {
    return WriteGuest(command_buffer + CommandBufferSelfOffset, command_buffer) &&
           WriteGuest(command_buffer + CommandBufferDataOffset, buffer) &&
           WriteGuest(command_buffer + CommandBufferSizeOffset, size);
}

bool WriteCommandBufferPointers(VAddr command_buffer, VAddr buffer, u64 size, u64 write_offset = 0) {
    if (!WriteVisibleCommandBufferPointers(command_buffer, buffer, size)) {
        return false;
    }

    std::scoped_lock lock{g_command_buffer_mutex};
    auto& state = g_command_buffers[command_buffer];
    state.buffer = buffer;
    state.size = size;
    state.write_offset = write_offset;
    return true;
}

bool InitializeCommandBuffer(VAddr command_buffer, VAddr buffer, u64 size, u64 aux0, u64 aux1,
                             bool clear) {
    if (clear) {
        if (!IsValidGuestRange(command_buffer, CommandBufferHeaderSize)) {
            return false;
        }
        std::memset(reinterpret_cast<void*>(command_buffer), 0, CommandBufferHeaderSize);
    }

    return WriteGuest(command_buffer + CommandBufferSelfOffset, command_buffer) &&
           WriteGuest(command_buffer + CommandBufferAux0Offset, aux0) &&
           WriteGuest(command_buffer + CommandBufferAux1Offset, aux1) &&
           WriteCommandBufferPointers(command_buffer, buffer, size);
}

bool TryGetCommandBufferState(VAddr command_buffer, CommandBufferState& out) {
    {
        std::scoped_lock lock{g_command_buffer_mutex};
        const auto it = g_command_buffers.find(command_buffer);
        if (it != g_command_buffers.end()) {
            out = it->second;
            return true;
        }
    }

    VAddr buffer = 0;
    u64 size = 0;
    if (!ReadGuest(command_buffer + CommandBufferDataOffset, buffer) ||
        !ReadGuest(command_buffer + CommandBufferSizeOffset, size)) {
        return false;
    }

    out.buffer = buffer;
    out.size = size;
    out.write_offset = 0;
    std::scoped_lock lock{g_command_buffer_mutex};
    g_command_buffers[command_buffer] = out;
    return true;
}

s32 ReadHostFileToGuest(const std::filesystem::path& host_path, u64 file_offset, VAddr destination,
                        u64 size, u64& bytes_read) {
    bytes_read = 0;
    if (size == 0) {
        return ORBIS_OK;
    }
    if (file_offset > static_cast<u64>(std::numeric_limits<s64>::max()) ||
        !IsValidGuestRange(destination, size)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    std::ifstream stream{host_path, std::ios::binary};
    if (!stream) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    stream.seekg(0, std::ios::end);
    const auto file_size = stream.tellg();
    if (file_size <= 0 || file_offset >= static_cast<u64>(file_size)) {
        return ORBIS_OK;
    }
    stream.seekg(static_cast<std::streamoff>(file_offset), std::ios::beg);

    std::array<char, 64 * 1024> buffer{};
    while (bytes_read < size && stream) {
        const auto request =
            static_cast<std::streamsize>(std::min<u64>(buffer.size(), size - bytes_read));
        stream.read(buffer.data(), request);
        const auto read = stream.gcount();
        if (read <= 0) {
            break;
        }
        std::memcpy(reinterpret_cast<void*>(destination + bytes_read), buffer.data(),
                    static_cast<std::size_t>(read));
        bytes_read += static_cast<u64>(read);
    }
    return ORBIS_OK;
}

bool AppendReadFileRecord(VAddr command_buffer, u32 file_id, VAddr destination, u64 size,
                          u64 file_offset, u64 bytes_read) {
    std::array<u8, ReadFileRecordSize> record{};
    *reinterpret_cast<u32*>(&record[0x00]) = 1;
    *reinterpret_cast<u32*>(&record[0x04]) = file_id;
    *reinterpret_cast<u64*>(&record[0x08]) = destination;
    *reinterpret_cast<u64*>(&record[0x10]) = size;
    *reinterpret_cast<u64*>(&record[0x18]) = file_offset;
    *reinterpret_cast<u64*>(&record[0x20]) = bytes_read;

    std::scoped_lock lock{g_command_buffer_mutex};
    auto it = g_command_buffers.find(command_buffer);
    if (it == g_command_buffers.end()) {
        CommandBufferState state;
        if (!ReadGuest(command_buffer + CommandBufferDataOffset, state.buffer) ||
            !ReadGuest(command_buffer + CommandBufferSizeOffset, state.size)) {
            return false;
        }
        it = g_command_buffers.emplace(command_buffer, state).first;
    }

    auto& state = it->second;
    if (state.buffer == 0 || state.write_offset + ReadFileRecordSize > state.size ||
        !IsValidGuestRange(state.buffer + state.write_offset, ReadFileRecordSize)) {
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(state.buffer + state.write_offset), record.data(),
                record.size());
    state.write_offset += ReadFileRecordSize;
    return true;
}

} // namespace

void* PS4_SYSV_ABI sceAmprCommandBufferConstructor(void* command_buffer, void* buffer, u64 size) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return nullptr;
    }
    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    if (!InitializeCommandBuffer(command_buffer_addr, reinterpret_cast<VAddr>(buffer), size, 0, 0,
                                 true)) {
        return nullptr;
    }
    return command_buffer;
}

void* PS4_SYSV_ABI sceAmprAprCommandBufferConstructor(void* command_buffer, u64 aux0, u64 aux1) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return nullptr;
    }

    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    VAddr buffer = 0;
    u64 size = 0;
    ReadGuest(command_buffer_addr + CommandBufferDataOffset, buffer);
    ReadGuest(command_buffer_addr + CommandBufferSizeOffset, size);
    if (!InitializeCommandBuffer(command_buffer_addr, buffer, size, aux0, aux1, false)) {
        return nullptr;
    }
    return command_buffer;
}

s32 PS4_SYSV_ABI sceAmprAprCommandBufferDestructor(void* command_buffer) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_OK;
    }
    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    if (!WriteGuest<u64>(command_buffer_addr + CommandBufferAux0Offset, 0) ||
        !WriteGuest<u64>(command_buffer_addr + CommandBufferAux1Offset, 0)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAmprCommandBufferDestructor(void* command_buffer) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_OK;
    }
    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    if (!WriteVisibleCommandBufferPointers(command_buffer_addr, 0, 0)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    std::scoped_lock lock{g_command_buffer_mutex};
    g_command_buffers.erase(command_buffer_addr);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAmprCommandBufferSetBuffer(void* command_buffer, void* buffer, u64 size) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (!WriteCommandBufferPointers(reinterpret_cast<VAddr>(command_buffer),
                                    reinterpret_cast<VAddr>(buffer), size)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAmprCommandBufferReset(void* command_buffer) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    CommandBufferState state;
    if (!TryGetCommandBufferState(command_buffer_addr, state) ||
        !WriteCommandBufferPointers(command_buffer_addr, state.buffer, state.size)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceAmprCommandBufferClearBuffer(void* command_buffer) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return nullptr;
    }
    const auto command_buffer_addr = reinterpret_cast<VAddr>(command_buffer);
    CommandBufferState state;
    if (!TryGetCommandBufferState(command_buffer_addr, state) ||
        !WriteVisibleCommandBufferPointers(command_buffer_addr, 0, 0)) {
        return nullptr;
    }
    std::scoped_lock lock{g_command_buffer_mutex};
    g_command_buffers.erase(command_buffer_addr);
    return reinterpret_cast<void*>(state.buffer);
}

s32 PS4_SYSV_ABI sceAmprAprCommandBufferReadFile(void* command_buffer, u64, u64, u32 file_id,
                                                 void* destination, u64 size, u64 file_offset) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr ||
        (destination == nullptr && size != 0)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::filesystem::path host_path;
    if (!Libraries::Kernel::TryGetAprHostPath(file_id, host_path) ||
        !std::filesystem::is_regular_file(host_path)) {
        LOG_WARNING(Lib_Kernel, "AMPR read_file missing file id {:#x}", file_id);
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    u64 bytes_read = 0;
    const auto result =
        ReadHostFileToGuest(host_path, file_offset, reinterpret_cast<VAddr>(destination), size,
                            bytes_read);
    if (result != ORBIS_OK) {
        return result;
    }
    if (!AppendReadFileRecord(reinterpret_cast<VAddr>(command_buffer), file_id,
                              reinterpret_cast<VAddr>(destination), size, file_offset, bytes_read)) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAmprAmmSubmitCommandBuffer(void* command_buffer, u64 arg1, u64 arg2, u64 arg3,
                                               u64 arg4, u64 arg5) {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return ORBIS_KERNEL_ERROR_ENOSYS;
    }
    std::fprintf(stderr,
                 "sceAmprAmmSubmitCommandBuffer cb=%p a1=0x%llx a2=0x%llx "
                 "a3=0x%llx a4=0x%llx a5=0x%llx -> 0\n",
                 command_buffer, static_cast<unsigned long long>(arg1),
                 static_cast<unsigned long long>(arg2), static_cast<unsigned long long>(arg3),
                 static_cast<unsigned long long>(arg4), static_cast<unsigned long long>(arg5));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAmprAmmWaitCommandBufferCompletion(u64 submission_id, u64 arg1, void* result,
                                                       u64 arg3, u64 arg4, u64 arg5) {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return ORBIS_KERNEL_ERROR_ENOSYS;
    }
    if (result != nullptr && IsValidGuestRange(reinterpret_cast<VAddr>(result), sizeof(u64))) {
        *reinterpret_cast<u64*>(result) = 0;
    }
    std::fprintf(stderr,
                 "sceAmprAmmWaitCommandBufferCompletion id=0x%llx a1=0x%llx "
                 "result=%p a3=0x%llx a4=0x%llx a5=0x%llx -> 0\n",
                 static_cast<unsigned long long>(submission_id),
                 static_cast<unsigned long long>(arg1), result,
                 static_cast<unsigned long long>(arg3), static_cast<unsigned long long>(arg4),
                 static_cast<unsigned long long>(arg5));
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return;
    }

    LIB_FUNCTION("8aI7R7WaOlc", "libSceAmpr", 1, "libSceAmpr", sceAmprCommandBufferConstructor);
    LIB_FUNCTION("a8uLzYY--tM", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAprCommandBufferConstructor);
    LIB_FUNCTION("Qs1xtplKo0U", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAprCommandBufferDestructor);
    LIB_FUNCTION("GuchCTefuZw", "libSceAmpr", 1, "libSceAmpr", sceAmprCommandBufferDestructor);
    LIB_FUNCTION("N-FSPA4S3nI", "libSceAmpr", 1, "libSceAmpr", sceAmprCommandBufferSetBuffer);
    LIB_FUNCTION("baQO9ez2gL4", "libSceAmpr", 1, "libSceAmpr", sceAmprCommandBufferReset);
    LIB_FUNCTION("ULvXMDz56po", "libSceAmpr", 1, "libSceAmpr", sceAmprCommandBufferClearBuffer);
    LIB_FUNCTION("mQ16-QdKv7k", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAprCommandBufferReadFile);
    LIB_FUNCTION("lwS-7y3jcBI", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAmmSubmitCommandBuffer);
    LIB_FUNCTION("OJf3vCckPAM", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAmmSubmitCommandBuffer);
    LIB_FUNCTION("NnKhlMJtIsI", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAmmSubmitCommandBuffer);
    LIB_FUNCTION("HXymib4T8gc", "libSceAmpr", 1, "libSceAmpr",
                 sceAmprAmmWaitCommandBufferCompletion);
}

} // namespace Libraries::Ampr
