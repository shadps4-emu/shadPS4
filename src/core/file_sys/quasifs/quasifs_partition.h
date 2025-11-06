// INAA License @marecl 2025

#pragma once

#include <unordered_map>

#include "quasi_types.h"

namespace QuasiFS {

class Partition : public std::enable_shared_from_this<Partition> {
private:
    fileno_t NextFileno(void) {
        return this->next_fileno++;
    };

    // file list
    std::unordered_map<fileno_t, inode_ptr> inode_table{};

    // root directory
    dir_ptr root;
    // next available fileno
    fileno_t next_fileno = 2;
    // technically it's a device+partition id, but block id is enough lmao
    const blkid_t block_id;

    static inline blkid_t next_block_id = 1;

    // path to host's directory this will be bound to
    const fs::path host_root{};

    // IO block size, allocation unit (multiples of 512)
    u32 ioblock_size{4096};
    // amount of raw on-disk blocks per io block
    // this is pretty much so we don't recalculate it over and over and over
    // and
    u32 block_size{512};

    u8 filesystem_format = FileSystem::NORMAL;

public:
    // host-bound directory, permissions for root directory
    Partition(const fs::path& host_root = "");
    ~Partition() = default;

    void Format(const int root_permissions = 0755, u8 format = FileSystem::NORMAL,
                const u32 block_size = 512, const u32 ioblock_size = 4096);

    template <typename... Args>
    static partition_ptr Create(Args&&... args) {
        return std::make_shared<Partition>(std::forward<Args>(args)...);
    }

    // empty - invalid, not empty - safe
    fs::path SanitizePath(const fs::path& path);
    // return - valid, out_path - sanitized path
    int GetHostPath(fs::path& output_path, const fs::path& local_path = "/");

    dir_ptr GetRoot(void) {
        return this->root;
    }
    bool IsHostMounted(void) {
        return !this->host_root.empty();
    }
    blkid_t GetBlkId(void) {
        return this->block_id;
    }
    void AdjustStat(Libraries::Kernel::OrbisKernelStat* statbuf) {
        auto& s = statbuf->st_size;
        statbuf->st_blksize = this->block_size;
        statbuf->st_blocks = (1 + (s / this->ioblock_size)) * this->ioblock_size / this->block_size;
    }
    inode_ptr GetInodeByFileno(fileno_t fileno);

    int Resolve(fs::path& path, Resolved& res);

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    template <typename T>
    int touch(dir_ptr parent, const std::string& name) {
        if constexpr (!std::is_base_of_v<Inode, T>)
            return touch(parent, name, T::Create());
        static_assert(std::is_base_of_v<Inode, T>,
                      " QuasiFS:Partition:Touch Created element must derive from Inode");
    }
    int touch(dir_ptr parent, const std::string& name, inode_ptr child);

    int mkdir(dir_ptr parent, const std::string& name);
    int mkdir(dir_ptr parent, const std::string& name, dir_ptr child);

    int rmdir(fs::path path);
    int rmdir(dir_ptr parent, const std::string& name);

    int link(inode_ptr source, dir_ptr destination_parent, const std::string& name);
    int unlink(dir_ptr parent, const std::string& name);

    static int chmod(inode_ptr target, u16 mode);

private:
    int rmInode(fileno_t fileno);
    int rmInode(inode_ptr node);
    bool IndexInode(inode_ptr node);
    static void mkrelative(dir_ptr parent, dir_ptr child);
};

}; // namespace QuasiFS