// INAA License @marecl 2025

#include "../quasi_errno.h"
#include "../quasi_types.h"

#include "core/file_sys/quasifs/quasifs.h"

#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_inode_virtualfile.h"
#include "core/file_sys/quasifs/quasifs_partition.h"

#include "../../quasi_log.h"

namespace QuasiFS {
std::string file_mode(u16 mode) {
    std::string s;

    if (QUASI_S_ISREG(mode))
        s += '-';
    else if (QUASI_S_ISDIR(mode))
        s += 'd';
    else if (QUASI_S_ISLNK(mode))
        s += 'l';
    else if (QUASI_S_ISCHR(mode))
        s += 'c';
    else if (QUASI_S_ISBLK(mode))
        s += 'b';
    else if (QUASI_S_ISFIFO(mode))
        s += 'p';
    else if (QUASI_S_ISSOCK(mode))
        s += 's';
    else
        s += '?';

    // owner
    s += (mode & QUASI_S_IRUSR) ? 'r' : '-';
    s += (mode & QUASI_S_IWUSR) ? 'w' : '-';
    s += (mode & QUASI_S_IXUSR) ? 'x' : '-';

    // group
    s += (mode & QUASI_S_IRGRP) ? 'r' : '-';
    s += (mode & QUASI_S_IWGRP) ? 'w' : '-';
    s += (mode & QUASI_S_IXGRP) ? 'x' : '-';

    // other
    s += (mode & QUASI_S_IROTH) ? 'r' : '-';
    s += (mode & QUASI_S_IWOTH) ? 'w' : '-';
    s += (mode & QUASI_S_IXOTH) ? 'x' : '-';

    return s;
}

void _printTree(const inode_ptr& node, const std::string& name, int depth) {

    std::string depEnt = "";
    for (uint8_t q = 0; q < depth; q++) {
        depEnt = depEnt + "|--";
    }
    if (depth > 0)
        depEnt[depEnt.length() - 1] = '>';

    if (!name.empty()) {
        auto st = node->st;
        char timebuf[64];
        std::tm* t = std::localtime(&st.st_mtime);
        std::strftime(timebuf, sizeof(timebuf), "%EY-%m-%d %H:%M", t);
        // TODO: UID/GID

        std::cout << "[ls -la] "
                  << std::format("{} {:08} {:03d} {}:{} {:>08} {}\t{}{}\n", file_mode(st.st_mode),
                                 st.st_mode, st.st_nlink, /*st.st_uid*/ 0, /* st.st_gid*/ 0,
                                 st.st_size, timebuf, depEnt, name);
    } else
        depth--;

    if (node->is_link())
        std::cout << "[ls -la] "
                  << std::format("\t\t\t\t\t\t\tsymlinked to ->{}\n",
                                 std::static_pointer_cast<Symlink>(node)->follow().string());

    if (node->is_dir()) {
        if ("." == name)
            return;
        if (".." == name)
            return;

        auto dir = std::dynamic_pointer_cast<Directory>(node);
        if (dir->mounted_root) {
            std::cout << "[ls -la] "
                      << std::format("\t\t\t\t\t\t\t|--{}{}\n", depEnt, "[MOUNTPOINT]");
            _printTree(dir->mounted_root, "", depth + 1);
        } else {
            for (auto& [childName, child] : dir->entries) {
                _printTree(child, childName, depth + 1);
            }
        }
    }
}

void printTree(const dir_ptr& node, const std::string& name, int depth) {
    _printTree(node, name, depth);
}

QFS::QFS(const fs::path& host_path) {
    this->rootfs = Partition::Create(host_path);
    this->root = rootfs->GetRoot();

    mount_t mount_options = {
        .mounted_at{"/"},
        .parentdir = this->root,
        .options = MountOptions::MOUNT_RW,
    };

    this->block_devices[this->rootfs] = mount_options;
}

int QFS::SyncHost(void) {
    for (auto& [part, info] : this->block_devices) {
        if (part->IsHostMounted())
            SyncHostImpl(part);
    }

    return 0;
}

int QFS::SyncHost(fs::path path) {
    Resolved res;
    int status = Resolve(path, res);

    if (0 != status)
        return -QUASI_ENOENT;

    if (nullptr == res.mountpoint)
        return -QUASI_ENOMEDIUM;

    SyncHostImpl(res.mountpoint);

    return 0;
}

// mount fs at path (target must exist and be directory)
int QFS::Mount(const fs::path& path, partition_ptr fs, unsigned int options) {
    Resolved res;
    int status = Resolve(path, res);

    if (0 != status)
        return status;

    if (!res.node->is_dir())
        return -QUASI_ENOTDIR;

    mount_t* existing_fs_options = GetPartitionInfo(fs);

    if (options & MountOptions::MOUNT_REMOUNT) {
        if (nullptr == existing_fs_options) {
            LogError("Can't remount {}: Not mounted", path.string());
            return -QUASI_EINVAL;
        }

        auto curopt = &existing_fs_options->options;
        *curopt = options & (~MountOptions::MOUNT_REMOUNT);
        return 0;
    }

    dir_ptr dir = std::static_pointer_cast<Directory>(res.node);
    if (nullptr != existing_fs_options || dir->mounted_root) {
        // fs_options exists or there's something (else?) mounted there already
        LogError("Can't mount {}: Already mounted", path.string());
        return -QUASI_EEXIST;
    }

    if (options & MountOptions::MOUNT_BIND)
        LogError("Mount --bind not implemented");

    dir_ptr fs_root = fs->GetRoot();
    mount_t fs_options = {
        .mounted_at = path,
        .parentdir = dir,
        .options = options,
    };

    dir->mounted_root = fs_root;
    this->block_devices[fs] = fs_options;

    return 0;
}

// mount fs at path (target must exist and be directory)
int QFS::Unmount(const fs::path& path) {
    Resolved res;
    int status = Resolve(path, res);

    if (0 != status)
        return status;

    partition_ptr part = res.mountpoint;
    mount_t* part_opts = GetPartitionInfo(part);

    if (nullptr == part_opts)
        return -QUASI_EINVAL;

    dir_ptr options_parentdir = part_opts->parentdir;
    dir_ptr res_parentdir = res.parent;
    dir_ptr res_rootdir = std::static_pointer_cast<Directory>(res.node);

    if (options_parentdir != res_parentdir)
        LogError("Resolved mountpoint has different parent in metadata and in resolution result");

    if (nullptr == res_rootdir)
        // mounted but rootdir disappeared O.o
        return -QUASI_EINVAL;

    options_parentdir->mounted_root = nullptr;
    this->block_devices.erase(part);

    return 0;
}

int QFS::ForceInsert(const fs::path& path, const std::string& name, inode_ptr node) {
    // it's just one of those days
    Resolved res;
    int resolve_status = this->Resolve(path, res);
    if (0 != resolve_status)
        return resolve_status;
    if (!res.node->is_dir())
        return -QUASI_ENOTDIR;
    return res.mountpoint->touch(std::static_pointer_cast<Directory>(res.node), name, node);
}

int QFS::Resolve(const fs::path& path, Resolved& res) {
    if (path.empty())
        return -QUASI_EINVAL;
    if (path.is_relative())
        return -QUASI_EINVAL;

    // on return:
    // node - last element of the path (if exists)
    // parent - parent element of the path (if parent dir is 1 level above last element)
    // mountpoint - target partition
    // leaf - name of the last element in the path (if exists)

    // guard against circular binds
    uint8_t safety_counter = 40;
    //
    int status{-1};

    fs::path iter_path = path;

    res.mountpoint = this->rootfs;
    res.local_path = iter_path;
    res.parent = this->root;
    res.node = this->root;

    do {
        if (iter_path.string().size() >= 256)
            return -QUASI_ENAMETOOLONG;

        status = res.mountpoint->Resolve(iter_path, res);

        if (0 != status)
            return status;

        if (res.node->is_link()) {
            // symlinks consume path from the front, since they point to an absolute location
            // let's say /link is linked to /dirA/dirB, and we need to resolve /link/dirC
            // path resolution will enter /link, and extract it as /dirA/dirB.
            // from that same path, /dirC will be preserved and appened to symlink's target,
            // which will yield /dirA/dirB/dirC
            fs::path leftover = iter_path;
            // main path is overwritten with absolute path from symlink
            iter_path = std::static_pointer_cast<Symlink>(res.node)->follow();
            // and if it's really in the way - restore leftover items

            //   Log("Found a symlink to [{}] // merging with // {}", iter_path.string(),
            //   leftover.string());

            if (!leftover.empty())
                iter_path /= leftover;
            // reset everything to point to rootfs, where absolute path can be resolved again
            res.mountpoint = this->rootfs;
            res.parent = this->root;
            res.node = this->root;
            res.leaf = "/";
            continue;
        }

        if (res.node->is_dir()) {
            dir_ptr mntparent = res.parent;
            dir_ptr mntroot = std::static_pointer_cast<Directory>(res.node);

            if (nullptr != mntparent->mounted_root) {
                if (mntroot != mntparent->mounted_root)
                    LogError("Resolved conflicting mount root and node");

                // just like symlinks, only trailing path is saved
                // directory, in which partition is mounted, belongs to upstream filesystem,
                // so everything before (including) that directory is consumed

                partition_ptr mounted_partition = GetPartitionByParent(mntparent);

                if (nullptr == mounted_partition) {
                    res.mountpoint = nullptr;
                    return -QUASI_ENOENT;
                }

                res.mountpoint = mounted_partition;
                res.parent = mntparent;
                res.node = mntroot;
                res.leaf = "/";

                if (iter_path != "/")
                    continue;
            }
        }

        break;

    } while (--safety_counter > 0);

    if (0 == safety_counter)
        return -QUASI_ELOOP;

    return 0;
}

int QFS::GetHostPath(fs::path& output, const fs::path& path) {
    Resolved res;
    int status = Resolve(path, res);
    if (status != 0)
        return status;

    return res.mountpoint->GetHostPath(output, res.local_path);
}

bool QFS::IsOpen(const int fd) noexcept {
    fd_handle_ptr fh = this->GetHandle(fd);
    if (nullptr == fh)
        return false;
    return fh->IsOpen();
}

int QFS::SetSize(const int fd, uint64_t size) noexcept {
    fd_handle_ptr fh = this->GetHandle(fd);
    if (nullptr == fh)
        return -QUASI_EBADF;
    return this->Operation.FTruncate(fd, size);
}

s64 QFS::GetSize(const int fd) noexcept {
    fd_handle_ptr fh = this->GetHandle(fd);
    if (nullptr == fh)
        return -QUASI_EBADF;
    if (nullptr == fh->node)
        return -QUASI_EBADF;

    return fh->node->st.st_size;
};

s64 QFS::GetDirectorySize(const fs::path& path) noexcept {
    UNIMPLEMENTED();
    return -QUASI_ENOSYS;
};

//
// Privates (don't touch)
//

void QFS::SyncHostImpl(partition_ptr part) {
    fs::path host_path{};
    if (0 != part->GetHostPath(host_path)) {
        LogError("Cannot safely resolve host directory for blkdev {}", part->GetBlkId());
        return; // false
    }

    // cut out host-root, remainder is Partition path
    auto host_path_components = std::distance(host_path.begin(), host_path.end()) - 1;
    auto slice_path = [host_path_components](const fs::path& p) {
        fs::path out;
        auto it = p.begin();
        std::advance(it, host_path_components);
        for (; it != p.end(); ++it)
            out /= *it;
        return out;
    };

    try {
        for (auto entry = fs::recursive_directory_iterator(host_path);
             entry != fs::recursive_directory_iterator(); entry++) {
            fs::path entry_path = entry->path();
            fs::path pp = "/" / slice_path(entry->path());
            fs::path parent_path = pp.parent_path();
            fs::path leaf = pp.filename();

            Resolved res;
            part->Resolve(parent_path, res);

            if (nullptr == res.node) {
                LogError("Cannot resolve quasi-target for sync: {}", parent_path.string());
                continue;
            }

            dir_ptr parent_dir =
                res.node->is_dir() ? std::static_pointer_cast<Directory>(res.node) : nullptr;
            inode_ptr new_inode{};

            if (entry->is_directory()) {
                new_inode = Directory::Create();
                part->mkdir(parent_dir, leaf, std::static_pointer_cast<Directory>(new_inode));
            } else if (entry->is_regular_file()) {
                new_inode = QuasiFile::Create<RegularFile>();
                part->touch(parent_dir, leaf, std::static_pointer_cast<RegularFile>(new_inode));
            } else {
                LogError("Unsupported host file type: {}", entry_path.string());
                continue;
            }

            if (0 != this->hio_driver.Stat(entry_path, &new_inode->st)) {
                LogError("Cannot stat file: {}", entry_path.string());
                continue;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Błąd: " << e.what() << "\n";
    }
    return; // true
}

int QFS::GetFreeHandleNo() {
    auto open_fd_size = open_fd.size();
    for (size_t idx = 0; idx < open_fd_size; idx++) {
        if (nullptr == this->open_fd[idx])
            return idx;
    }
    open_fd.push_back(nullptr);
    return open_fd_size;
}

fd_handle_ptr QFS::GetHandle(int fd) {
    if (fd < 0 || fd >= this->open_fd.size())
        return nullptr;
    return this->open_fd.at(fd);
}

mount_t* QFS::GetPartitionInfo(const partition_ptr part) {
    auto target_part_info = this->block_devices.find(part);
    // already mounted
    if (this->block_devices.end() == target_part_info)
        return nullptr;
    return &(target_part_info->second);
}

partition_ptr QFS::GetPartitionByPath(const fs::path& path) {
    for (auto& [part, info] : this->block_devices) {
        if (info.mounted_at == path)
            return part;
    }
    return nullptr;
}

partition_ptr QFS::GetPartitionByParent(const dir_ptr dir) {
    for (auto& [part, info] : this->block_devices) {
        if (info.parentdir == dir)
            return part;
    }
    return nullptr;
}

int QFS::IsPartitionRO(partition_ptr part) {
    mount_t* part_info = GetPartitionInfo(part);
    if (nullptr == part_info)
        return -QUASI_ENODEV;
    if (part_info->options & MountOptions::MOUNT_RW)
        return 0;
    return 1;
}
}; // namespace QuasiFS