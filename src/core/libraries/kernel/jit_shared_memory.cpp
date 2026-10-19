// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/devices/base_device.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/jit_shared_memory.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "core/memory.h"

#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Libraries::Kernel {

#ifdef __linux__

namespace {

// Objects get their address on creation, and every process maps an object at that address. The
// window is a choice of the emulator, the range used by the PS4 kernel is unknown.
constexpr VAddr JitWindowBase = 0x100000000;
constexpr VAddr JitWindowEnd = 0x200000000;
constexpr u64 JitPageSize = 16_KB;

constexpr Core::MemoryProt CpuProtMask = Core::MemoryProt::CpuReadWrite | Core::MemoryProt::CpuExec;

Core::MemoryProt WithImpliedRead(Core::MemoryProt prot) {
    if (True(prot & Core::MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= Core::MemoryProt::CpuRead;
    }
    return prot;
}

class HostFd {
public:
    HostFd() = default;
    explicit HostFd(int fd_) : fd{fd_} {}
    ~HostFd() {
        Reset();
    }
    HostFd(HostFd&& other) noexcept : fd{std::exchange(other.fd, -1)} {}
    HostFd& operator=(HostFd&& other) noexcept {
        if (this != &other) {
            Reset();
            fd = std::exchange(other.fd, -1);
        }
        return *this;
    }

    int Get() const {
        return fd;
    }
    int Release() {
        return std::exchange(fd, -1);
    }
    void Reset() {
        if (fd >= 0) {
            close(fd);
        }
        fd = -1;
    }

private:
    int fd = -1;
};

struct FileId {
    u64 dev;
    u64 ino;
    auto operator<=>(const FileId&) const = default;
};

std::optional<FileId> IdOf(int fd) {
    struct stat st{};
    if (fstat(fd, &st) != 0) {
        return std::nullopt;
    }
    return FileId{static_cast<u64>(st.st_dev), static_cast<u64>(st.st_ino)};
}

bool HasSeals(int fd, int seals) {
    const int present = fcntl(fd, F_GET_SEALS);
    return present >= 0 && (present & seals) == seals;
}

int CreateSealableMemory(const char* name) {
#ifdef MFD_NOEXEC_SEAL
    // The memfd is never run as a program, so it can be sealed against that (vm.memfd_noexec).
    // Executable mappings of it are not affected.
    const int fd = memfd_create(name, MFD_CLOEXEC | MFD_NOEXEC_SEAL);
    if (fd >= 0 || errno != EINVAL) {
        return fd;
    }
    // Kernels before 6.3 reject the flag.
#endif
    return memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
}

class JitSharedObject {
public:
    JitSharedObject(HostFd host_fd_, FileId id_, u64 size_, VAddr address_,
                    Core::MemoryProt max_prot_, std::string name_)
        : host_fd{std::move(host_fd_)}, id{id_}, size{size_}, address{address_},
          max_prot{max_prot_}, name{std::move(name_)} {}

    const HostFd host_fd;
    const FileId id;
    const u64 size;
    const VAddr address;
    const Core::MemoryProt max_prot;
    const std::string name;
};

// A guest descriptor of a JIT shared memory object, with its own protection limit.
class JitSharedMemoryAlias final : public Core::Devices::BaseDevice {
public:
    JitSharedMemoryAlias(std::shared_ptr<const JitSharedObject> object_, Core::MemoryProt max_prot_)
        : object{std::move(object_)}, max_prot{max_prot_} {}
    ~JitSharedMemoryAlias() override = default;

    const std::shared_ptr<const JitSharedObject> object;
    const Core::MemoryProt max_prot;
};

// Shared by every process of the family. Fixed layout, the same build reads and writes it.
struct FamilyPage {
    char magic[8];
    u32 version;
    u32 header_size;
    u64 window_base;
    u64 window_end;
    u64 next; // only accessed through std::atomic_ref
};
static_assert(sizeof(FamilyPage) == 40 && offsetof(FamilyPage, next) == 32);
static_assert(std::atomic_ref<u64>::is_always_lock_free &&
              std::atomic_ref<u64>::required_alignment <= alignof(u64));
constexpr char FamilyMagic[8] = {'S', 'H', 'J', 'I', 'T', 'F', 'A', '1'};
constexpr u32 FamilyVersion = 1;
constexpr size_t FamilyPageSize = 4096;
constexpr int FamilySeals = F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL;

// Metadata of a guest descriptor in transit. Fixed little-endian layout without padding.
struct Metadata {
    char magic[8];
    u32 version;
    u32 header_size;
    u64 family_dev;
    u64 family_ino;
    u64 object_dev;
    u64 object_ino;
    u64 address;
    u64 size;
    u32 object_max_prot;
    u32 alias_max_prot;
    char name[32];
    u8 reserved[24];
};
static_assert(sizeof(Metadata) == 128 && offsetof(Metadata, family_dev) == 16 &&
              offsetof(Metadata, object_max_prot) == 64 && offsetof(Metadata, name) == 72 &&
              offsetof(Metadata, reserved) == 104);
constexpr char MetadataMagic[8] = {'S', 'H', 'J', 'I', 'T', 'M', 'D', '1'};
constexpr u32 MetadataVersion = 1;
constexpr int MetadataSeals = F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL;
constexpr int ObjectSeals = F_SEAL_GROW | F_SEAL_SHRINK;

// Lock order: g_family_mutex, then the memory manager.
std::mutex g_family_mutex;
HostFd g_family_fd;
FamilyPage* g_family = nullptr;
FileId g_family_id{};

bool CreateFamilyLocked() {
    HostFd fd{CreateSealableMemory("shadps4-jit-family")};
    if (fd.Get() < 0 || ftruncate(fd.Get(), FamilyPageSize) != 0) {
        LOG_ERROR(Kernel_Vmm, "Unable to create the JIT family page: {}", strerror(errno));
        return false;
    }
    void* page = mmap(nullptr, FamilyPageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd.Get(), 0);
    if (page == MAP_FAILED) {
        LOG_ERROR(Kernel_Vmm, "Unable to map the JIT family page: {}", strerror(errno));
        return false;
    }
    auto* family = static_cast<FamilyPage*>(page);
    std::memcpy(family->magic, FamilyMagic, sizeof(FamilyMagic));
    family->version = FamilyVersion;
    family->header_size = sizeof(FamilyPage);
    family->window_base = JitWindowBase;
    family->window_end = JitWindowEnd;
    std::atomic_ref<u64>{family->next}.store(JitWindowBase);
    const auto id = IdOf(fd.Get());
    if (!id || fcntl(fd.Get(), F_ADD_SEALS, FamilySeals) != 0) {
        LOG_ERROR(Kernel_Vmm, "Unable to seal the JIT family page: {}", strerror(errno));
        munmap(page, FamilyPageSize);
        return false;
    }
    g_family_fd = std::move(fd);
    g_family = family;
    g_family_id = *id;
    return true;
}

FamilyPage* GetFamily() {
    std::scoped_lock lk{g_family_mutex};
    if (g_family == nullptr && !CreateFamilyLocked()) {
        return nullptr;
    }
    return g_family;
}

std::optional<FileId> GetFamilyId() {
    std::scoped_lock lk{g_family_mutex};
    return g_family != nullptr ? std::optional{g_family_id} : std::nullopt;
}

// Every family member draws from the same cursor, so no two objects of the family overlap.
VAddr AssignAddress(u64 size) {
    auto* family = GetFamily();
    if (family == nullptr) {
        return 0;
    }
    std::atomic_ref<u64> next{family->next};
    for (;;) {
        u64 start = next.load();
        do {
            if (start < JitWindowBase || start > JitWindowEnd || start % JitPageSize != 0) {
                LOG_ERROR(Kernel_Vmm, "JIT family cursor {:#x} is corrupt", start);
                return 0;
            }
            if (size > JitWindowEnd - start) {
                return 0;
            }
        } while (!next.compare_exchange_weak(start, start + size));
        // Taken here by something else: the range is lost for the whole family.
        if (Core::Memory::Instance()->FindFreeArea(start, start + size, size, JitPageSize) ==
            start) {
            return start;
        }
    }
}

std::mutex g_objects_mutex;
std::map<FileId, std::weak_ptr<const JitSharedObject>> g_objects;

void RegisterObjectLocked(const std::shared_ptr<const JitSharedObject>& object) {
    std::erase_if(g_objects, [](const auto& entry) { return entry.second.expired(); });
    g_objects[object->id] = object;
}

s32 CreateAliasHandle(std::shared_ptr<const JitSharedObject> object, Core::MemoryProt max_prot) {
    auto name = fmt::format("jit:{}", object->name);
    auto alias = std::make_shared<JitSharedMemoryAlias>(std::move(object), max_prot);
    return Common::Singleton<Core::FileSys::HandleTable>::Instance()->CreateDevice(std::move(alias),
                                                                                   std::move(name));
}

// Holds the alias, so a concurrent close of the descriptor doesn't free it.
std::shared_ptr<JitSharedMemoryAlias> GetAlias(s32 fd, bool* exists = nullptr) {
    auto ref = Common::Singleton<Core::FileSys::HandleTable>::Instance()->GetDevice(fd);
    if (exists != nullptr) {
        *exists = ref.exists;
    }
    return std::dynamic_pointer_cast<JitSharedMemoryAlias>(ref.device);
}

HostFd WriteMetadata(const JitSharedMemoryAlias& alias, const FileId& family) {
    Metadata meta;
    std::memset(&meta, 0, sizeof(meta));
    std::memcpy(meta.magic, MetadataMagic, sizeof(MetadataMagic));
    meta.version = MetadataVersion;
    meta.header_size = sizeof(Metadata);
    meta.family_dev = family.dev;
    meta.family_ino = family.ino;
    meta.object_dev = alias.object->id.dev;
    meta.object_ino = alias.object->id.ino;
    meta.address = alias.object->address;
    meta.size = alias.object->size;
    meta.object_max_prot = static_cast<u32>(alias.object->max_prot);
    meta.alias_max_prot = static_cast<u32>(alias.max_prot);
    alias.object->name.copy(meta.name, sizeof(meta.name) - 1);

    HostFd fd{CreateSealableMemory("shadps4-jit-meta")};
    if (fd.Get() < 0 || pwrite(fd.Get(), &meta, sizeof(meta), 0) != sizeof(meta) ||
        fcntl(fd.Get(), F_ADD_SEALS, MetadataSeals) != 0) {
        LOG_ERROR(Kernel_Vmm, "Unable to write JIT descriptor metadata: {}", strerror(errno));
        return HostFd{};
    }
    return fd;
}

struct ImportedAlias {
    std::shared_ptr<const JitSharedObject> object;
    Core::MemoryProt max_prot;
};

std::optional<ImportedAlias> ImportPair(HostFd object_fd, int metadata_fd) {
    const auto reject = [](std::string_view why) {
        LOG_ERROR(Kernel_Vmm, "Rejected a received JIT descriptor: {}", why);
        return std::nullopt;
    };
    struct stat meta_st{};
    if (fstat(metadata_fd, &meta_st) != 0 || !S_ISREG(meta_st.st_mode) ||
        meta_st.st_size != sizeof(Metadata) || !HasSeals(metadata_fd, MetadataSeals)) {
        return reject("metadata is not a sealed 128-byte file");
    }
    Metadata meta;
    if (pread(metadata_fd, &meta, sizeof(meta), 0) != sizeof(meta) ||
        std::memcmp(meta.magic, MetadataMagic, sizeof(MetadataMagic)) != 0 ||
        meta.version != MetadataVersion || meta.header_size != sizeof(Metadata)) {
        return reject("unknown metadata format");
    }
    const auto family = GetFamilyId();
    if (!family || meta.family_dev != family->dev || meta.family_ino != family->ino) {
        return reject("object of another process family");
    }
    struct stat object_st{};
    if (fstat(object_fd.Get(), &object_st) != 0 || !S_ISREG(object_st.st_mode) ||
        static_cast<u64>(object_st.st_dev) != meta.object_dev ||
        static_cast<u64>(object_st.st_ino) != meta.object_ino ||
        static_cast<u64>(object_st.st_size) != meta.size ||
        !HasSeals(object_fd.Get(), ObjectSeals)) {
        return reject("object does not match its metadata");
    }
    if (meta.size == 0 || meta.size % JitPageSize != 0 || meta.address % JitPageSize != 0 ||
        meta.address < JitWindowBase || meta.address >= JitWindowEnd ||
        meta.size > JitWindowEnd - meta.address) {
        return reject("address range outside the JIT window");
    }
    const auto object_max = static_cast<Core::MemoryProt>(meta.object_max_prot);
    const auto alias_max = static_cast<Core::MemoryProt>(meta.alias_max_prot);
    if (True(object_max & ~CpuProtMask) || True(alias_max & ~object_max) ||
        WithImpliedRead(object_max) != object_max || WithImpliedRead(alias_max) != alias_max) {
        return reject("invalid protection limits");
    }
    if (std::memchr(meta.name, 0, sizeof(meta.name)) == nullptr) {
        return reject("unterminated name");
    }

    const FileId id{meta.object_dev, meta.object_ino};
    std::scoped_lock lk{g_objects_mutex};
    if (const auto it = g_objects.find(id); it != g_objects.end()) {
        if (auto existing = it->second.lock()) {
            if (existing->address != meta.address || existing->size != meta.size ||
                existing->max_prot != object_max) {
                return reject("object known with other properties");
            }
            return ImportedAlias{std::move(existing), alias_max};
        }
    }
    auto object = std::make_shared<const JitSharedObject>(std::move(object_fd), id, meta.size,
                                                          meta.address, object_max, meta.name);
    RegisterObjectLocked(object);
    return ImportedAlias{std::move(object), alias_max};
}

} // namespace

int GetJitFamilyDescriptor() {
    std::scoped_lock lk{g_family_mutex};
    if (g_family == nullptr && !CreateFamilyLocked()) {
        return -1;
    }
    return g_family_fd.Get();
}

bool JoinJitFamily(int host_fd) {
    HostFd fd{host_fd};
    std::scoped_lock lk{g_family_mutex};
    if (g_family != nullptr) {
        LOG_ERROR(Kernel_Vmm, "This process already has a JIT family");
        return false;
    }
    struct stat st{};
    if (fstat(fd.Get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != FamilyPageSize ||
        !HasSeals(fd.Get(), FamilySeals)) {
        LOG_ERROR(Kernel_Vmm, "Inherited JIT family page is not a sealed page");
        return false;
    }
    void* page = mmap(nullptr, FamilyPageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd.Get(), 0);
    if (page == MAP_FAILED) {
        LOG_ERROR(Kernel_Vmm, "Unable to map the JIT family page: {}", strerror(errno));
        return false;
    }
    auto* family = static_cast<FamilyPage*>(page);
    const u64 next = std::atomic_ref<u64>{family->next}.load();
    if (std::memcmp(family->magic, FamilyMagic, sizeof(FamilyMagic)) != 0 ||
        family->version != FamilyVersion || family->header_size != sizeof(FamilyPage) ||
        family->window_base != JitWindowBase || family->window_end != JitWindowEnd ||
        next < JitWindowBase || next > JitWindowEnd || next % JitPageSize != 0 ||
        fcntl(fd.Get(), F_SETFD, FD_CLOEXEC) != 0) {
        LOG_ERROR(Kernel_Vmm, "Inherited JIT family page is not valid");
        munmap(page, FamilyPageSize);
        return false;
    }
    g_family_id = {static_cast<u64>(st.st_dev), static_cast<u64>(st.st_ino)};
    g_family_fd = std::move(fd);
    g_family = family;
    return true;
}

JitDescriptorBatch::~JitDescriptorBatch() {
    for (const int fd : metadata_fds) {
        close(fd);
    }
}

void JitDescriptorBatch::Add(int object_fd, int metadata_fd,
                             std::shared_ptr<const void> keep_alive) {
    metadata_fds.push_back(metadata_fd);
    objects.push_back(std::move(keep_alive));
    fds.push_back(object_fd);
    fds.push_back(metadata_fd);
}

int ExportJitDescriptors(std::span<const s32> guest_fds, JitDescriptorBatch* batch) {
    if (GetFamily() == nullptr) {
        return POSIX_ENOMEM;
    }
    const FileId family = *GetFamilyId();
    for (const s32 guest_fd : guest_fds) {
        bool exists = false;
        auto alias = GetAlias(guest_fd, &exists);
        if (!exists) {
            return POSIX_EBADF;
        }
        if (alias == nullptr) {
            LOG_ERROR(Kernel_Vmm, "Only JIT shared memory descriptors can be sent, not fd {}",
                      guest_fd);
            return POSIX_EOPNOTSUPP;
        }
        HostFd metadata = WriteMetadata(*alias, family);
        if (metadata.Get() < 0) {
            return POSIX_ENOMEM;
        }
        const int object_fd = alias->object->host_fd.Get();
        batch->Add(object_fd, metadata.Release(), std::move(alias));
    }
    return 0;
}

JitImportResult ImportJitDescriptors(std::span<const int> host_fds) {
    std::vector<HostFd> owned;
    owned.reserve(host_fds.size());
    for (const int fd : host_fds) {
        owned.emplace_back(fd);
    }
    JitImportResult result;
    if (owned.size() % 2 != 0) {
        // Half of a pair: the sender's message was cut short.
        result.dropped = true;
        owned.pop_back();
    }
    std::vector<ImportedAlias> imported;
    for (size_t i = 0; i < owned.size(); i += 2) {
        auto alias = ImportPair(std::move(owned[i]), owned[i + 1].Get());
        if (!alias) {
            result.dropped = true;
            return result;
        }
        imported.push_back(std::move(*alias));
    }
    for (auto& alias : imported) {
        const s32 fd = CreateAliasHandle(std::move(alias.object), alias.max_prot);
        LOG_INFO(Kernel_Vmm, "Received JIT descriptor as fd {}", fd);
        result.guest_fds.push_back(fd);
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelJitCreateSharedMemory(const char* name, u64 len, s32 max_prot,
                                                s32* out_fd) {
    std::string object_name =
        name != nullptr ? std::string(name, strnlen(name, ORBIS_KERNEL_MAXIMUM_NAME_LENGTH)) : "";
    LOG_INFO(Kernel_Vmm, "name = '{}', len = {:#x}, max_prot = {:#x}", object_name, len, max_prot);
    const auto prot = static_cast<Core::MemoryProt>(max_prot);
    if (len == 0 || True(prot & ~CpuProtMask)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len > JitWindowEnd - JitWindowBase) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }
    const u64 size = Common::AlignUp(len, JitPageSize);
    const auto limit = WithImpliedRead(prot);

    HostFd host_fd{CreateSealableMemory("shadps4-jit")};
    if (host_fd.Get() < 0 || ftruncate(host_fd.Get(), size) != 0 ||
        fcntl(host_fd.Get(), F_ADD_SEALS, ObjectSeals) != 0) {
        const int error = errno;
        LOG_ERROR(Kernel_Vmm, "Unable to create shared memory: {}", strerror(error));
        return error == EACCES || error == EPERM ? ORBIS_KERNEL_ERROR_EACCES
                                                 : ORBIS_KERNEL_ERROR_ENOMEM;
    }
    const auto id = IdOf(host_fd.Get());
    const VAddr address = id ? AssignAddress(size) : 0;
    if (address == 0) {
        LOG_ERROR(Kernel_Vmm, "No room for {:#x} bytes of JIT shared memory", size);
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }
    auto object = std::make_shared<const JitSharedObject>(std::move(host_fd), *id, size, address,
                                                          limit, std::move(object_name));
    {
        std::scoped_lock lk{g_objects_mutex};
        RegisterObjectLocked(object);
    }
    const s32 fd = CreateAliasHandle(std::move(object), limit);
    LOG_INFO(Kernel_Vmm, "fd = {}, address = {:#x}, size = {:#x}", fd, address, size);
    if (out_fd != nullptr) {
        *out_fd = fd;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelJitCreateAliasOfSharedMemory(s32 fd, s32 max_prot, s32* out_fd) {
    LOG_INFO(Kernel_Vmm, "fd = {}, max_prot = {:#x}", fd, max_prot);
    const auto source = GetAlias(fd);
    if (source == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    const auto prot = static_cast<Core::MemoryProt>(max_prot);
    if (True(prot & ~CpuProtMask)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const auto limit = WithImpliedRead(prot);
    if (True(limit & ~source->max_prot)) {
        LOG_ERROR(Kernel_Vmm, "Limit {:#x} exceeds the limit {:#x} of fd {}",
                  static_cast<u32>(limit), static_cast<u32>(source->max_prot), fd);
        return ORBIS_KERNEL_ERROR_EACCES;
    }
    const s32 alias_fd = CreateAliasHandle(source->object, limit);
    LOG_INFO(Kernel_Vmm, "alias fd = {}", alias_fd);
    if (out_fd != nullptr) {
        *out_fd = alias_fd;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelJitMapSharedMemory(s32 fd, s32 prot, void** addr) {
    LOG_INFO(Kernel_Vmm, "fd = {}, prot = {:#x}", fd, prot);
    const auto alias = GetAlias(fd);
    if (alias == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    if (True(mem_prot & ~CpuProtMask)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (True(WithImpliedRead(mem_prot) & ~alias->max_prot)) {
        LOG_ERROR(Kernel_Vmm, "Protection {:#x} exceeds the limit {:#x} of fd {}", prot,
                  static_cast<u32>(alias->max_prot), fd);
        return ORBIS_KERNEL_ERROR_EACCES;
    }
    // The address belongs to the object. Like the system library, ignore the value in *addr.
    const auto& object = *alias->object;
    const s32 result = Core::Memory::Instance()->MapJitSharedMemory(
        object.address, object.size, mem_prot, alias->max_prot, object.host_fd.Get(), object.name);
    if (result != ORBIS_OK) {
        return result;
    }
    LOG_INFO(Kernel_Vmm, "'{}' mapped at {:#x}", object.name, object.address);
    if (addr != nullptr) {
        *addr = reinterpret_cast<void*>(object.address);
    }
    return ORBIS_OK;
}

#else

s32 PS4_SYSV_ABI sceKernelJitCreateSharedMemory(const char* name, u64 len, s32 max_prot,
                                                s32* out_fd) {
    LOG_ERROR(Kernel_Vmm, "JIT shared memory is only implemented on Linux");
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

s32 PS4_SYSV_ABI sceKernelJitCreateAliasOfSharedMemory(s32 fd, s32 max_prot, s32* out_fd) {
    LOG_ERROR(Kernel_Vmm, "JIT shared memory is only implemented on Linux");
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

s32 PS4_SYSV_ABI sceKernelJitMapSharedMemory(s32 fd, s32 prot, void** addr) {
    LOG_ERROR(Kernel_Vmm, "JIT shared memory is only implemented on Linux");
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

#endif

void RegisterJitSharedMemory(Core::Loader::SymbolsResolver* sym) {
    for (const char* lib : {"libkernel", "libkernel_ps2emu"}) {
        LIB_FUNCTION("avvJ3J0H0EY", lib, 1, "libkernel", sceKernelJitCreateSharedMemory);
        LIB_FUNCTION("MR221Mwo0Pc", lib, 1, "libkernel", sceKernelJitCreateAliasOfSharedMemory);
        LIB_FUNCTION("YKT49TOLQWs", lib, 1, "libkernel", sceKernelJitMapSharedMemory);
    }
}

} // namespace Libraries::Kernel
