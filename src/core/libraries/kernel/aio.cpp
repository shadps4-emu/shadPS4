// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

#include "aio.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "common/thread.h"
#include "core/file_sys/fs.h"
#include "core/file_sys/storage_scheduler.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/memory.h"

namespace Libraries::Kernel {
namespace {

constexpr u32 MaxRequests = 512;
constexpr u32 SlotBits = 9;
constexpr u32 SlotMask = (1u << SlotBits) - 1;

void PublishResult(OrbisKernelAioResult* result, s64 return_value, AioState state) {
    result->returnValue = return_value;
    std::atomic_ref{result->state}.store(state, std::memory_order_release);
}

struct AioRecord {
    AioRecord(OrbisKernelAioSubmitId id, u32 command_count, bool abort_on_error)
        : id{id}, remaining{command_count}, abort_on_error{abort_on_error} {}

    [[nodiscard]] bool IsPending() const {
        return state.load(std::memory_order_acquire) <= ORBIS_KERNEL_AIO_STATE_PROCESSING;
    }

    [[nodiscard]] bool IsComplete() const {
        return state.load(std::memory_order_acquire) >= ORBIS_KERNEL_AIO_STATE_COMPLETED;
    }

    const OrbisKernelAioSubmitId id;
    std::atomic<s32> state{ORBIS_KERNEL_AIO_STATE_PROCESSING};
    std::atomic<u32> remaining;
    std::atomic_bool cancel_requested{};
    const bool abort_on_error;
    std::mutex mutex;
    std::vector<Core::FileSys::StorageRequestHandle> storage_requests;
};

class AioManager {
public:
    ~AioManager() {
        worker.request_stop();
        work_cv.notify_all();
    }

    std::shared_ptr<AioRecord> Allocate(u32 command_count, bool abort_on_error) {
        std::scoped_lock lock{records_mutex};
        for (u32 attempt = 0; attempt < MaxRequests - 1; ++attempt) {
            const u32 slot = next_slot;
            next_slot = next_slot == MaxRequests - 1 ? 1 : next_slot + 1;
            const auto& existing = records[slot];
            if (existing && existing->IsPending()) {
                continue;
            }

            u32 generation = ++generations[slot];
            if (generation == 0 || generation >= (1u << (31 - SlotBits))) {
                generation = generations[slot] = 1;
            }
            const auto id = static_cast<s32>((generation << SlotBits) | slot);
            auto record = std::make_shared<AioRecord>(id, command_count, abort_on_error);
            records[slot] = record;
            return record;
        }
        return {};
    }

    std::shared_ptr<AioRecord> Lookup(OrbisKernelAioSubmitId id) const {
        std::scoped_lock lock{records_mutex};
        const auto slot = FindSlot(id);
        return slot ? records[*slot] : nullptr;
    }

    std::shared_ptr<AioRecord> Remove(OrbisKernelAioSubmitId id) {
        std::scoped_lock lock{records_mutex};
        const auto slot = FindSlot(id);
        return slot ? std::exchange(records[*slot], nullptr) : nullptr;
    }

    // Mirrors the PS4 kernel: an in-flight request cannot be deleted (EBUSY). This also
    // guarantees no completion can publish into guest memory after a successful delete.
    s32 Delete(OrbisKernelAioSubmitId id) {
        std::scoped_lock lock{records_mutex};
        const auto slot = FindSlot(id);
        if (!slot) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        if (records[*slot]->IsPending()) {
            return ORBIS_KERNEL_ERROR_EBUSY;
        }
        records[*slot] = nullptr;
        return ORBIS_OK;
    }

    void Enqueue(std::function<void()> operation) {
        {
            std::scoped_lock lock{work_mutex};
            if (!worker.joinable()) {
                worker = std::jthread{[this](std::stop_token stop_token) { Run(stop_token); }};
            }
            work.push_back(std::move(operation));
        }
        work_cv.notify_one();
    }

    void AttachStorageRequest(const std::shared_ptr<AioRecord>& record,
                              Core::FileSys::StorageRequestHandle request) {
        bool cancel{};
        {
            std::scoped_lock lock{record->mutex};
            cancel = record->cancel_requested.load(std::memory_order_acquire);
            if (!cancel && record->IsPending()) {
                record->storage_requests.push_back(std::move(request));
            }
        }
        if (cancel) {
            Core::FileSys::GetApp0StorageScheduler().Cancel(request);
        }
    }

    void Complete(const std::shared_ptr<AioRecord>& record, OrbisKernelAioResult* result,
                  s64 return_value, bool canceled) {
        const bool cancellation =
            canceled || record->cancel_requested.load(std::memory_order_acquire);
        const bool failed = return_value < 0;
        PublishResult(result,
                      cancellation ? static_cast<s64>(ORBIS_KERNEL_ERROR_ECANCELED) : return_value,
                      cancellation || failed ? ORBIS_KERNEL_AIO_STATE_ABORTED
                                             : ORBIS_KERNEL_AIO_STATE_COMPLETED);

        if (record->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            const bool abort_record = cancellation || (failed && record->abort_on_error);
            {
                std::scoped_lock lock{record->mutex};
                record->storage_requests.clear();
                record->state.store(abort_record ? ORBIS_KERNEL_AIO_STATE_ABORTED
                                                 : ORBIS_KERNEL_AIO_STATE_COMPLETED,
                                    std::memory_order_release);
            }
            wait_cv.notify_all();
        }
    }

    s32 Cancel(const std::shared_ptr<AioRecord>& record) {
        if (record->IsComplete()) {
            return record->state.load(std::memory_order_acquire);
        }
        record->cancel_requested.store(true, std::memory_order_release);
        std::vector<Core::FileSys::StorageRequestHandle> requests;
        {
            std::scoped_lock lock{record->mutex};
            requests = record->storage_requests;
        }
        for (const auto& request : requests) {
            Core::FileSys::GetApp0StorageScheduler().Cancel(request);
        }
        return record->state.load(std::memory_order_acquire);
    }

    s32 Wait(const std::vector<std::shared_ptr<AioRecord>>& records_to_wait, bool wait_any,
             u32* usec) {
        const auto predicate = [&] {
            if (wait_any) {
                return std::ranges::any_of(records_to_wait,
                                           [](const auto& record) { return record->IsComplete(); });
            }
            return std::ranges::all_of(records_to_wait,
                                       [](const auto& record) { return record->IsComplete(); });
        };

        std::unique_lock lock{wait_mutex};
        if (usec == nullptr || *usec == 0) {
            wait_cv.wait(lock, predicate);
            return ORBIS_OK;
        }
        const auto timeout = std::chrono::microseconds{*usec};
        const auto begin = std::chrono::steady_clock::now();
        if (!wait_cv.wait_for(lock, timeout, predicate)) {
            *usec = 0;
            return ORBIS_KERNEL_ERROR_ETIMEDOUT;
        }
        const auto elapsed = std::chrono::steady_clock::now() - begin;
        *usec = elapsed < timeout
                    ? static_cast<u32>(
                          std::chrono::duration_cast<std::chrono::microseconds>(timeout - elapsed)
                              .count())
                    : 0;
        return ORBIS_OK;
    }

private:
    // Validates an id and resolves it to its slot. Requires records_mutex to be held.
    std::optional<u32> FindSlot(OrbisKernelAioSubmitId id) const {
        if (id <= 0) {
            return std::nullopt;
        }
        const u32 slot = static_cast<u32>(id) & SlotMask;
        if (slot == 0 || slot >= MaxRequests || !records[slot] || records[slot]->id != id) {
            return std::nullopt;
        }
        return slot;
    }

    void Run(std::stop_token stop_token) {
        Common::SetCurrentThreadName("shadPS4:AioWork");
        while (!stop_token.stop_requested()) {
            std::function<void()> operation;
            {
                std::unique_lock lock{work_mutex};
                work_cv.wait(lock, [&] { return stop_token.stop_requested() || !work.empty(); });
                if (stop_token.stop_requested()) {
                    break;
                }
                operation = std::move(work.front());
                work.pop_front();
            }
            operation();
        }
    }

    mutable std::mutex records_mutex;
    std::array<std::shared_ptr<AioRecord>, MaxRequests> records;
    std::array<u32, MaxRequests> generations{};
    u32 next_slot{1};

    std::mutex work_mutex;
    std::condition_variable work_cv;
    std::deque<std::function<void()>> work;

    std::mutex wait_mutex;
    std::condition_variable wait_cv;
    // Joined before the synchronization primitives and queues it accesses are destroyed.
    std::jthread worker;
};

AioManager& GetAioManager() {
    static AioManager manager;
    return manager;
}

s64 ReadNative(const std::shared_ptr<Core::FileSys::File>& file, void* buffer, u64 size,
               u64 offset) {
    if (!file || file->type != Core::FileSys::FileType::Regular || file->f.IsWriteOnly()) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    const u64 file_size = file->f.GetSize();
    const u64 readable = offset < file_size ? std::min(size, file_size - offset) : 0;
    Core::Memory::Instance()->InvalidateMemory(reinterpret_cast<VAddr>(buffer), readable);
    const s64 result = file->PRead(buffer, readable, offset);
    return result < 0 ? ORBIS_KERNEL_ERROR_EIO : result;
}

s64 WriteNative(const std::shared_ptr<Core::FileSys::File>& file, const void* buffer, u64 size,
                u64 offset) {
    if (!file || file->type != Core::FileSys::FileType::Regular) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    const s64 result = file->PWrite(buffer, size, offset);
    return result < 0 ? ORBIS_KERNEL_ERROR_EIO : result;
}

struct SubmittedCommand {
    OrbisKernelAioRWRequest request;
    std::shared_ptr<Core::FileSys::File> file;
};

s32 SubmitCommands(OrbisKernelAioRWRequest requests[], s32 size, s32 priority,
                   OrbisKernelAioSubmitId ids[], bool multiple, bool write) {
    if (requests == nullptr || ids == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (size <= 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* handles = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    std::vector<SubmittedCommand> commands;
    commands.reserve(size);
    for (s32 index = 0; index < size; ++index) {
        if (requests[index].result == nullptr ||
            (requests[index].buf == nullptr && requests[index].nbyte != 0)) {
            return ORBIS_KERNEL_ERROR_EFAULT;
        }
        if (requests[index].nbyte < 0 || requests[index].offset < 0) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        auto file = handles->GetFileShared(requests[index].fd);
        if (!file) {
            return ORBIS_KERNEL_ERROR_EBADF;
        }
        commands.push_back({requests[index], std::move(file)});
    }

    for (const auto& command : commands) {
        PublishResult(command.request.result, 0, ORBIS_KERNEL_AIO_STATE_SUBMITTED);
    }

    auto& manager = GetAioManager();
    std::vector<std::shared_ptr<AioRecord>> records;
    if (multiple) {
        records.reserve(size);
        for (s32 index = 0; index < size; ++index) {
            auto record = manager.Allocate(1, true);
            if (!record) {
                for (const auto& allocated : records) {
                    manager.Remove(allocated->id);
                }
                for (const auto& command : commands) {
                    PublishResult(command.request.result, ORBIS_KERNEL_ERROR_EAGAIN,
                                  ORBIS_KERNEL_AIO_STATE_ABORTED);
                }
                return ORBIS_KERNEL_ERROR_EAGAIN;
            }
            ids[index] = record->id;
            records.push_back(std::move(record));
        }
    } else {
        auto record = manager.Allocate(static_cast<u32>(size), false);
        if (!record) {
            for (const auto& command : commands) {
                PublishResult(command.request.result, ORBIS_KERNEL_ERROR_EAGAIN,
                              ORBIS_KERNEL_AIO_STATE_ABORTED);
            }
            return ORBIS_KERNEL_ERROR_EAGAIN;
        }
        ids[0] = record->id;
        records.assign(size, record);
    }

    for (s32 index = 0; index < size; ++index) {
        auto record = records[index];
        auto command = std::move(commands[index]);
        std::atomic_ref{command.request.result->state}.store(ORBIS_KERNEL_AIO_STATE_PROCESSING,
                                                             std::memory_order_release);

        if (!write && Core::FileSys::ShouldScheduleAppRead(*command.file)) {
            if (command.file->type != Core::FileSys::FileType::Regular ||
                command.file->f.IsWriteOnly()) {
                manager.Complete(record, command.request.result, ORBIS_KERNEL_ERROR_EBADF, false);
                continue;
            }
            const u64 offset = static_cast<u64>(command.request.offset);
            const u64 readable =
                offset < command.file->m_size
                    ? std::min<u64>(command.request.nbyte, command.file->m_size - offset)
                    : 0;
            Core::FileSys::StorageReadSpans spans{{command.request.buf, readable}};
            auto request = Core::FileSys::GetApp0StorageScheduler().SubmitRead(
                command.file, std::move(spans), offset, priority,
                [record, result = command.request.result](s64 value, bool canceled) {
                    GetAioManager().Complete(record, result,
                                             value < 0 ? ORBIS_KERNEL_ERROR_EIO : value, canceled);
                });
            manager.AttachStorageRequest(record, std::move(request));
            continue;
        }

        manager.Enqueue([record, command = std::move(command), write] {
            if (record->cancel_requested.load(std::memory_order_acquire)) {
                GetAioManager().Complete(record, command.request.result, -1, true);
                return;
            }
            const s64 result = write ? WriteNative(command.file, command.request.buf,
                                                   command.request.nbyte, command.request.offset)
                                     : ReadNative(command.file, command.request.buf,
                                                  command.request.nbyte, command.request.offset);
            GetAioManager().Complete(record, command.request.result, result, false);
        });
    }
    return ORBIS_OK;
}

std::vector<std::shared_ptr<AioRecord>> LookupRecords(const OrbisKernelAioSubmitId ids[], s32 num) {
    std::vector<std::shared_ptr<AioRecord>> records;
    records.reserve(num);
    for (s32 index = 0; index < num; ++index) {
        auto record = GetAioManager().Lookup(ids[index]);
        if (!record) {
            return {};
        }
        records.push_back(std::move(record));
    }
    return records;
}

} // namespace

s32 PS4_SYSV_ABI sceKernelAioInitializeImpl(void*, s32) {
    GetAioManager();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioDeleteRequest(OrbisKernelAioSubmitId id, s32* ret) {
    if (ret == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    const s32 result = GetAioManager().Delete(id);
    if (result != ORBIS_OK) {
        return result;
    }
    *ret = ORBIS_OK;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioDeleteRequests(OrbisKernelAioSubmitId ids[], s32 num, s32 ret[]) {
    if (ids == nullptr || ret == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 index = 0; index < num; ++index) {
        ret[index] = ORBIS_OK;
        const s32 error = sceKernelAioDeleteRequest(ids[index], &ret[index]);
        if (error != ORBIS_OK) {
            ret[index] = error;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioPollRequest(OrbisKernelAioSubmitId id, s32* state) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    auto record = GetAioManager().Lookup(id);
    if (!record) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    *state = record->state.load(std::memory_order_acquire);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioPollRequests(OrbisKernelAioSubmitId ids[], s32 num, s32 state[]) {
    if (ids == nullptr || state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 index = 0; index < num; ++index) {
        const s32 error = sceKernelAioPollRequest(ids[index], &state[index]);
        if (error != ORBIS_OK) {
            return error;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioCancelRequest(OrbisKernelAioSubmitId id, s32* state) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    auto record = GetAioManager().Lookup(id);
    if (!record) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    *state = GetAioManager().Cancel(record);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioCancelRequests(OrbisKernelAioSubmitId ids[], s32 num, s32 state[]) {
    if (ids == nullptr || state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 index = 0; index < num; ++index) {
        const s32 error = sceKernelAioCancelRequest(ids[index], &state[index]);
        if (error != ORBIS_OK) {
            return error;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioWaitRequest(OrbisKernelAioSubmitId id, s32* state, u32* usec) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    auto record = GetAioManager().Lookup(id);
    if (!record) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const s32 result = GetAioManager().Wait({record}, false, usec);
    *state = record->state.load(std::memory_order_acquire);
    return result;
}

s32 PS4_SYSV_ABI sceKernelAioWaitRequests(OrbisKernelAioSubmitId ids[], s32 num, s32 state[],
                                          u32 mode, u32* usec) {
    if (ids == nullptr || state == nullptr || num <= 0) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    auto records = LookupRecords(ids, num);
    if (records.size() != static_cast<size_t>(num)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const s32 result = GetAioManager().Wait(records, mode == 0x02, usec);
    for (s32 index = 0; index < num; ++index) {
        state[index] = records[index]->state.load(std::memory_order_acquire);
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelAioSubmitReadCommands(OrbisKernelAioRWRequest requests[], s32 size,
                                                s32 priority, OrbisKernelAioSubmitId* id) {
    return SubmitCommands(requests, size, priority, id, false, false);
}

s32 PS4_SYSV_ABI sceKernelAioSubmitReadCommandsMultiple(OrbisKernelAioRWRequest requests[],
                                                        s32 size, s32 priority,
                                                        OrbisKernelAioSubmitId ids[]) {
    return SubmitCommands(requests, size, priority, ids, true, false);
}

s32 PS4_SYSV_ABI sceKernelAioSubmitWriteCommands(OrbisKernelAioRWRequest requests[], s32 size,
                                                 s32 priority, OrbisKernelAioSubmitId* id) {
    return SubmitCommands(requests, size, priority, id, false, true);
}

s32 PS4_SYSV_ABI sceKernelAioSubmitWriteCommandsMultiple(OrbisKernelAioRWRequest requests[],
                                                         s32 size, s32 priority,
                                                         OrbisKernelAioSubmitId ids[]) {
    return SubmitCommands(requests, size, priority, ids, true, true);
}

s32 PS4_SYSV_ABI sceKernelAioSetParam() {
    LOG_ERROR(Kernel, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioInitializeParam() {
    LOG_ERROR(Kernel, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterAio(Core::Loader::SymbolsResolver* sym) {
    GetAioManager();
    LIB_FUNCTION("fR521KIGgb8", "libkernel", 1, "libkernel", sceKernelAioCancelRequest);
    LIB_FUNCTION("3Lca1XBrQdY", "libkernel", 1, "libkernel", sceKernelAioCancelRequests);
    LIB_FUNCTION("5TgME6AYty4", "libkernel", 1, "libkernel", sceKernelAioDeleteRequest);
    LIB_FUNCTION("Ft3EtsZzAoY", "libkernel", 1, "libkernel", sceKernelAioDeleteRequests);
    LIB_FUNCTION("vYU8P9Td2Zo", "libkernel", 1, "libkernel", sceKernelAioInitializeImpl);
    LIB_FUNCTION("nu4a0-arQis", "libkernel", 1, "libkernel", sceKernelAioInitializeParam);
    LIB_FUNCTION("2pOuoWoCxdk", "libkernel", 1, "libkernel", sceKernelAioPollRequest);
    LIB_FUNCTION("o7O4z3jwKzo", "libkernel", 1, "libkernel", sceKernelAioPollRequests);
    LIB_FUNCTION("9WK-vhNXimw", "libkernel", 1, "libkernel", sceKernelAioSetParam);
    LIB_FUNCTION("HgX7+AORI58", "libkernel", 1, "libkernel", sceKernelAioSubmitReadCommands);
    LIB_FUNCTION("lXT0m3P-vs4", "libkernel", 1, "libkernel",
                 sceKernelAioSubmitReadCommandsMultiple);
    LIB_FUNCTION("XQ8C8y+de+E", "libkernel", 1, "libkernel", sceKernelAioSubmitWriteCommands);
    LIB_FUNCTION("xT3Cpz0yh6Y", "libkernel", 1, "libkernel",
                 sceKernelAioSubmitWriteCommandsMultiple);
    LIB_FUNCTION("KOF-oJbQVvc", "libkernel", 1, "libkernel", sceKernelAioWaitRequest);
    LIB_FUNCTION("lgK+oIWkJyA", "libkernel", 1, "libkernel", sceKernelAioWaitRequests);
}

} // namespace Libraries::Kernel
