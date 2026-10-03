// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common/io_file.h"
#include "common/singleton.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"

namespace fs = std::filesystem;
namespace {
fs::path root;
Common::FS::IOFile* existing_host{};
bool existing_entry{};
bool read_only{};
int success_logs{};

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void Create(const char* name = "file") {
    std::ofstream(root / name) << "save contents";
}
} // namespace

namespace Core::FileSys {
struct File {
    Common::FS::IOFile* GetHostFile() const {
        return existing_host;
    }
};
struct HandleTable {
    File file;
    File* GetFile(const fs::path&) {
        return existing_entry ? &file : nullptr;
    }
};
struct MntPoints {
    fs::path GetHostPath(std::string_view path, bool* ro) {
        *ro = read_only;
        return path.starts_with("/savedata0/") ? root / path.substr(11) : fs::path{};
    }
};
} // namespace Core::FileSys

namespace Common::Log {
std::array<Level, NUM_LOG_CLASSES> g_class_levels{};
void VLog(Class, Level, const char*, int, const char*, fmt::string_view format, fmt::format_args) {
    if (std::string_view(format.data(), format.size()) == "Unlinked {}") {
        ++success_logs;
    }
    // A logger or cleanup can change host errno; syscall results must survive it.
    errno = ERANGE;
}
} // namespace Common::Log
namespace Common::FS {
std::string PathToUTF8String(const fs::path& path) {
    return path.string();
}
} // namespace Common::FS
void assert_fail_impl() {
    std::abort();
}
[[noreturn]] void unreachable_impl() {
    std::abort();
}

namespace Libraries::Kernel {
constexpr int ORBIS_MAX_PATH = 255;
#include "errno.inc"
#include "unlink.inc"
} // namespace Libraries::Kernel

int main(int argc, char** argv) {
    using namespace Libraries::Kernel;
    Check(argc == 2, "Expected a test case");
    char pattern[] = "/tmp/shadps4-unlink-test-XXXXXX";
    const auto* path = mkdtemp(pattern);
    Check(path != nullptr, "Cannot create isolated fixture");
    root = path;
    struct Cleanup {
        ~Cleanup() {
            std::error_code ec;
            fs::permissions(root, fs::perms::owner_all, ec);
            fs::remove_all(root, ec);
        }
    } cleanup;
    const std::string which = argv[1];
    auto failed = [](s32 result, int expected) {
        Check(result == -1, "Failed unlink returned success");
        Check(*__Error() == expected, "Wrong guest errno");
        Check(success_logs == 0, "Failed unlink logged success");
    };
    if (which == "missing") {
        failed(posix_unlink("/savedata0/TEMP0_RDR2OPTIONS.SAV"), POSIX_ENOENT);
    } else if (which == "sce_missing") {
        Check(sceKernelUnlink("/savedata0/missing") == ORBIS_KERNEL_ERROR_ENOENT,
              "SCE export did not translate ENOENT");
        Check(success_logs == 0, "SCE failure logged success");
    } else if (which == "null") {
        failed(posix_unlink(nullptr), POSIX_EFAULT);
        Check(sceKernelUnlink(nullptr) == ORBIS_KERNEL_ERROR_EFAULT,
              "SCE export did not translate EFAULT");
    } else if (which == "long_path") {
        failed(posix_unlink(std::string(256, 'x').c_str()), POSIX_ENAMETOOLONG);
    } else if (which == "directory") {
        fs::create_directory(root / "dir");
        failed(posix_unlink("/savedata0/dir"), POSIX_EPERM);
        Check(fs::is_directory(root / "dir"), "Directory removed by unlink");
    } else if (which == "read_only") {
        Create();
        read_only = true;
        failed(posix_unlink("/savedata0/file"), POSIX_EROFS);
        Check(fs::exists(root / "file"), "Read-only file was removed");
    } else if (which == "unknown_mount") {
        failed(posix_unlink("/unknown/file"), POSIX_ENOENT);
    } else if (which == "success") {
        Create();
        Check(sceKernelUnlink("/savedata0/file") == 0, "Existing file was not removed");
        Check(!fs::exists(root / "file"), "Unlink left the pathname present");
        Check(success_logs == 1, "Successful unlink was not logged once");
    } else if (which == "open_file") {
        Create();
        Common::FS::IOFile file(root / "file", Common::FS::FileAccessMode::ReadWrite);
        existing_host = &file;
        existing_entry = true;
        Check(posix_unlink("/savedata0/file") == 0, "Open file unlink failed");
        Check(!fs::exists(root / "file"), "Open file pathname remained");
        Check(file.IsOpen() && file.ReadString(13) == "save contents",
              "Unlink invalidated the open descriptor or file contents");
    } else if (which == "already_removed") {
        Create();
        Common::FS::IOFile file(root / "file", Common::FS::FileAccessMode::ReadWrite);
        existing_host = &file;
        existing_entry = true;
        fs::remove(root / "file");
        failed(posix_unlink("/savedata0/file"), POSIX_ENOENT);
    } else if (which == "no_host") {
        Create();
        existing_entry = true;
        failed(posix_unlink("/savedata0/file"), POSIX_EROFS);
        Check(fs::exists(root / "file"), "Backend without a host file was modified");
    } else if (which == "errno_mapping") {
        Check(NativeToPosixErrno(ENAMETOOLONG) == POSIX_ENAMETOOLONG,
              "Host ENAMETOOLONG leaked into guest ABI");
        Check(NativeToPosixErrno(ELOOP) == POSIX_ELOOP, "Host ELOOP leaked into guest ABI");
    } else if (which == "io_errors") {
        Common::FS::IOFile closed;
        Check(closed.Unlink() == EBADF, "Closed IOFile reported a successful unlink");
        Create();
        Common::FS::IOFile file(root / "file", Common::FS::FileAccessMode::ReadWrite);
        fs::remove(root / "file");
        Check(file.Unlink() == ENOENT, "IOFile discarded the native deletion error");
    } else if (which == "permission") {
        Create();
        Common::FS::IOFile file(root / "file", Common::FS::FileAccessMode::ReadWrite);
        existing_host = &file;
        existing_entry = true;
        fs::permissions(root, fs::perms::owner_read | fs::perms::owner_exec |
                                  fs::perms::group_read | fs::perms::group_exec |
                                  fs::perms::others_read | fs::perms::others_exec);
        const auto pid = fork();
        Check(pid >= 0, "Cannot fork permission fixture");
        if (pid == 0) {
            if (geteuid() == 0) {
                Check(setgid(65534) == 0 && setuid(65534) == 0, "Cannot drop fixture privileges");
            }
            failed(posix_unlink("/savedata0/file"), POSIX_EACCES);
            _exit(0);
        }
        int status{};
        Check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "Host deletion error was not propagated");
        Check(fs::exists(root / "file"), "Permission failure removed file");
    } else {
        Check(false, "Unknown case");
    }
    std::cout << "PASS: " << which << '\n';
}
