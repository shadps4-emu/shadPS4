// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <zlib.h>
#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "core/file_format/pkg.h"
#include "core/file_format/pkg_type.h"

static void DecompressPFSC(std::span<char> compressed_data, std::span<char> decompressed_data) {
    z_stream decompressStream;
    decompressStream.zalloc = Z_NULL;
    decompressStream.zfree = Z_NULL;
    decompressStream.opaque = Z_NULL;

    if (inflateInit(&decompressStream) != Z_OK) {
        // std::cerr << "Error initializing zlib for deflation." << std::endl;
    }

    decompressStream.avail_in = compressed_data.size();
    decompressStream.next_in = reinterpret_cast<unsigned char*>(compressed_data.data());
    decompressStream.avail_out = decompressed_data.size();
    decompressStream.next_out = reinterpret_cast<unsigned char*>(decompressed_data.data());

    if (inflate(&decompressStream, Z_FINISH)) {
    }
    if (inflateEnd(&decompressStream) != Z_OK) {
        // std::cerr << "Error ending zlib inflate" << std::endl;
    }
}

u32 GetPFSCOffset(std::span<const u8> pfs_image) {
    static constexpr u32 PfscMagic = 0x43534650;
    u32 value;
    for (u32 i = 0x20000; i < pfs_image.size(); i += 0x10000) {
        std::memcpy(&value, &pfs_image[i], sizeof(u32));
        if (value == PfscMagic)
            return i;
    }
    return -1;
}

PKG::PKG() = default;

PKG::~PKG() = default;

bool PKG::Open(const std::filesystem::path& filepath, std::string& failreason) {
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }
    pkgSize = file.GetSize();

    file.Read(pkgheader);
    if (pkgheader.magic != 0x7F434E54)
        return false;

    for (const auto& flag : flagNames) {
        if (isFlagSet(pkgheader.pkg_content_flags, flag.first)) {
            if (!pkgFlags.empty())
                pkgFlags += (", ");
            pkgFlags += (flag.second);
        }
    }

    // Find title id it is part of pkg_content_id starting at offset 0x40
    file.Seek(0x47); // skip first 7 characters of content_id
    file.Read(pkgTitleID);

    u32 offset = pkgheader.pkg_table_entry_offset;
    u32 n_files = pkgheader.pkg_table_entry_count;

    if (!file.Seek(offset)) {
        failreason = "Failed to seek to PKG table entry offset";
        return false;
    }

    for (int i = 0; i < n_files; i++) {
        PKGEntry entry{};
        file.Read(entry.id);
        file.Read(entry.filename_offset);
        file.Read(entry.flags1);
        file.Read(entry.flags2);
        file.Read(entry.offset);
        file.Read(entry.size);
        file.Seek(8, Common::FS::SeekOrigin::CurrentPosition);

        // Try to figure out the name
        const auto name = GetEntryNameByType(entry.id);
        if (name == "param.sfo") {
            sfo.clear();
            if (!file.Seek(entry.offset)) {
                failreason = "Failed to seek to param.sfo offset";
                return false;
            }
            sfo.resize(entry.size);
            file.ReadRaw<u8>(sfo.data(), entry.size);
        }
    }
    file.Close();

    return true;
}

bool PKG::Extract(const std::filesystem::path& filepath, const std::filesystem::path& extract,
                  std::string& failreason) {
    extract_path = extract;
    pkgpath = filepath;
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }
    pkgSize = file.GetSize();
    file.ReadRaw<u8>(&pkgheader, sizeof(PKGHeader));

    if (pkgheader.magic != 0x7F434E54)
        return false;

    if (pkgheader.pkg_size > pkgSize) {
        failreason = "PKG file size is different";
        return false;
    }
    if ((pkgheader.pkg_content_size + pkgheader.pkg_content_offset) > pkgheader.pkg_size) {
        failreason = "Content size is bigger than pkg size";
        return false;
    }

    u32 offset = pkgheader.pkg_table_entry_offset;
    u32 n_files = pkgheader.pkg_table_entry_count;

    std::array<u8, 64> concatenated_ivkey_dk3;
    std::array<u8, 32> seed_digest;
    std::array<std::array<u8, 32>, 7> digest1;
    std::array<std::array<u8, 256>, 7> key1;
    std::array<u8, 256> imgkeydata;

    if (!file.Seek(offset)) {
        failreason = "Failed to seek to PKG table entry offset";
        return false;
    }

    for (int i = 0; i < n_files; i++) {
        PKGEntry entry{};
        file.Read(entry.id);
        file.Read(entry.filename_offset);
        file.Read(entry.flags1);
        file.Read(entry.flags2);
        file.Read(entry.offset);
        file.Read(entry.size);
        file.Seek(8, Common::FS::SeekOrigin::CurrentPosition);

        auto currentPos = file.Tell();

        // Try to figure out the name
        const auto name = GetEntryNameByType(entry.id);
        const auto filepath = extract_path / "sce_sys" / name;
        std::filesystem::create_directories(filepath.parent_path());

        if (name.empty()) {
            // Just print with id
            Common::FS::IOFile out(extract_path / "sce_sys" / std::to_string(entry.id),
                                   Common::FS::FileAccessMode::Write);
            if (!file.Seek(entry.offset)) {
                failreason = "Failed to seek to PKG entry offset";
                return false;
            }

            std::vector<u8> data;
            data.resize(entry.size);
            file.ReadRaw<u8>(data.data(), entry.size);
            out.WriteRaw<u8>(data.data(), entry.size);
            out.Close();

            file.Seek(currentPos);
            continue;
        }

        if (entry.id == 0x1) {         // DIGESTS, seek;
                                       // file.Seek(entry.offset, fsSeekSet);
        } else if (entry.id == 0x10) { // ENTRY_KEYS, seek;
            file.Seek(entry.offset);
            file.Read(seed_digest);

            for (int i = 0; i < 7; i++) {
                file.Read(digest1[i]);
            }

            for (int i = 0; i < 7; i++) {
                file.Read(key1[i]);
            }

            PKG::crypto.RSA2048Decrypt(dk3_, key1[3], true); // decrypt DK3
        } else if (entry.id == 0x20) {                       // IMAGE_KEY, seek; IV_KEY
            file.Seek(entry.offset);
            file.Read(imgkeydata);

            // The Concatenated iv + dk3 imagekey for HASH256
            std::memcpy(concatenated_ivkey_dk3.data(), &entry, sizeof(entry));
            std::memcpy(concatenated_ivkey_dk3.data() + sizeof(entry), dk3_.data(), sizeof(dk3_));

            PKG::crypto.ivKeyHASH256(concatenated_ivkey_dk3, ivKey); // ivkey_
            // imgkey_ to use for last step to get ekpfs
            PKG::crypto.aesCbcCfb128Decrypt(ivKey, imgkeydata, imgKey);
            // ekpfs key to get data and tweak keys.
            PKG::crypto.RSA2048Decrypt(ekpfsKey, imgKey, false);
        } else if (entry.id == 0x80) {
            // GENERAL_DIGESTS, seek;
            // file.Seek(entry.offset, fsSeekSet);
        }

        Common::FS::IOFile out(extract_path / "sce_sys" / name, Common::FS::FileAccessMode::Write);
        if (!file.Seek(entry.offset)) {
            failreason = "Failed to seek to PKG entry offset";
            return false;
        }

        std::vector<u8> data;
        data.resize(entry.size);
        file.ReadRaw<u8>(data.data(), entry.size);
        out.WriteRaw<u8>(data.data(), entry.size);
        out.Close();

        // Decrypt Np stuff and overwrite.
        if (entry.id == 0x400 || entry.id == 0x401 || entry.id == 0x402 ||
            entry.id == 0x403) { // somehow 0x401 is not decrypting
            decNp.resize(entry.size);
            if (!file.Seek(entry.offset)) {
                failreason = "Failed to seek to PKG entry offset";
                return false;
            }

            std::vector<u8> data;
            data.resize(entry.size);
            file.ReadRaw<u8>(data.data(), entry.size);

            std::span<u8> cipherNp(data.data(), entry.size);
            std::array<u8, 64> concatenated_ivkey_dk3_;
            std::memcpy(concatenated_ivkey_dk3_.data(), &entry, sizeof(entry));
            std::memcpy(concatenated_ivkey_dk3_.data() + sizeof(entry), dk3_.data(), sizeof(dk3_));
            PKG::crypto.ivKeyHASH256(concatenated_ivkey_dk3_, ivKey);
            PKG::crypto.aesCbcCfb128DecryptEntry(ivKey, cipherNp, decNp);

            Common::FS::IOFile out(extract_path / "sce_sys" / name,
                                   Common::FS::FileAccessMode::Write);
            out.Write(decNp);
            out.Close();
        }

        file.Seek(currentPos);
    }

    // Read the seed
    std::array<u8, 16> seed;
    if (!file.Seek(pkgheader.pfs_image_offset + 0x370)) {
        failreason = "Failed to seek to PFS image offset";
        return false;
    }
    file.Read(seed);

    // Get data and tweak keys.
    PKG::crypto.PfsGenCryptoKey(ekpfsKey, seed, dataKey, tweakKey);
    const u32 length = pkgheader.pfs_cache_size * 0x2; // Seems to be ok.

    int num_blocks = 0;
    std::vector<u8> pfsc(length);
    if (length != 0) {
        // Read encrypted pfs_image
        std::vector<u8> pfs_encrypted(length);
        file.Seek(pkgheader.pfs_image_offset);
        file.Read(pfs_encrypted);
        file.Close();
        // Decrypt the pfs_image.
        std::vector<u8> pfs_decrypted(length);
        PKG::crypto.decryptPFS(dataKey, tweakKey, pfs_encrypted, pfs_decrypted, 0);

        // Retrieve PFSC from decrypted pfs_image.
        pfsc_offset = GetPFSCOffset(pfs_decrypted);
        std::memcpy(pfsc.data(), pfs_decrypted.data() + pfsc_offset, length - pfsc_offset);

        PFSCHdr pfsChdr;
        std::memcpy(&pfsChdr, pfsc.data(), sizeof(pfsChdr));

        num_blocks = (int)(pfsChdr.data_length / pfsChdr.block_sz2);
        sectorMap.resize(num_blocks + 1); // 8 bytes, need extra 1 to get the last offset.

        for (int i = 0; i < num_blocks + 1; i++) {
            std::memcpy(&sectorMap[i], pfsc.data() + pfsChdr.block_offsets + i * 8, 8);
        }
    }

    u32 ent_size = 0;
    u32 ndinode = 0;
    int ndinode_counter = 0;
    bool dinode_reached = false;
    bool uroot_reached = false;
    std::vector<char> compressedData;
    std::vector<char> decompressedData(0x10000);

    // Get iNdoes and Dirents.
    for (int i = 0; i < num_blocks; i++) {
        const u64 sectorOffset = sectorMap[i];
        const u64 sectorSize = sectorMap[i + 1] - sectorOffset;

        compressedData.resize(sectorSize);
        std::memcpy(compressedData.data(), pfsc.data() + sectorOffset, sectorSize);

        if (sectorSize == 0x10000) // Uncompressed data
            std::memcpy(decompressedData.data(), compressedData.data(), 0x10000);
        else if (sectorSize < 0x10000) // Compressed data
            DecompressPFSC(compressedData, decompressedData);

        if (i == 0) {
            std::memcpy(&ndinode, decompressedData.data() + 0x30, 4); // number of folders and files
        }

        int occupied_blocks =
            (ndinode * 0xA8) / 0x10000; // how many blocks(0x10000) are taken by iNodes.
        if (((ndinode * 0xA8) % 0x10000) != 0)
            occupied_blocks += 1;

        if (i >= 1 && i <= occupied_blocks) { // Get all iNodes, gives type, file size and location.
            for (int p = 0; p < 0x10000; p += 0xA8) {
                Inode node;
                std::memcpy(&node, &decompressedData[p], sizeof(node));
                if (node.Mode == 0) {
                    break;
                }
                iNodeBuf.push_back(node);
            }
        }

        // let's deal with the root/uroot entries here.
        // Sometimes it's more than 2 entries (Tomb Raider Remastered)
        const std::string_view flat_path_table(&decompressedData[0x10], 15);
        if (flat_path_table == "flat_path_table") {
            uroot_reached = true;
        }

        if (uroot_reached) {
            for (int i = 0; i < 0x10000; i += ent_size) {
                Dirent dirent;
                std::memcpy(&dirent, &decompressedData[i], sizeof(dirent));
                ent_size = dirent.entsize;
                if (dirent.ino != 0) {
                    ndinode_counter++;
                } else {
                    // Set the the folder according to the current inode.
                    // Can be 2 or more (rarely)
                    auto parent_path = extract_path.parent_path();
                    auto title_id = GetTitleID();

                    if (parent_path.filename() != title_id &&
                        !fmt::UTF(extract_path.u8string()).data.ends_with("-patch")) {
                        extractPaths[ndinode_counter] = parent_path / title_id;
                    } else {
                        // DLCs path has different structure
                        extractPaths[ndinode_counter] = extract_path;
                    }
                    uroot_reached = false;
                    break;
                }
            }
        }

        const char dot = decompressedData[0x10];
        const std::string_view dotdot(&decompressedData[0x28], 2);
        if (dot == '.' && dotdot == "..") {
            dinode_reached = true;
        }

        // Get folder and file names.
        bool end_reached = false;
        if (dinode_reached) {
            for (int j = 0; j < 0x10000; j += ent_size) { // Skip the first parent and child.
                Dirent dirent;
                std::memcpy(&dirent, &decompressedData[j], sizeof(dirent));

                // Stop here and continue the main loop
                if (dirent.ino == 0) {
                    break;
                }

                ent_size = dirent.entsize;
                auto& table = fsTable.emplace_back();
                table.name = std::string(dirent.name, dirent.namelen);
                table.inode = dirent.ino;
                table.type = dirent.type;

                if (table.type == PFS_CURRENT_DIR) {
                    current_dir = extractPaths[table.inode];
                }
                extractPaths[table.inode] = current_dir / std::filesystem::path(table.name);

                if (table.type == PFS_FILE || table.type == PFS_DIR) {
                    if (table.type == PFS_DIR) { // Create dirs.
                        std::filesystem::create_directory(extractPaths[table.inode]);
                    }
                    ndinode_counter++;
                    if ((ndinode_counter + 1) == ndinode) // 1 for the image itself (root).
                        end_reached = true;
                }
            }
            if (end_reached) {
                break;
            }
        }
    }
    return true;
}

void PKG::ExtractFiles(const int index) {
    int inode_number = fsTable[index].inode;
    int inode_type = fsTable[index].type;
    std::string inode_name = fsTable[index].name;

    if (inode_type == PFS_FILE) {
        int sector_loc = iNodeBuf[inode_number].loc;
        int nblocks = iNodeBuf[inode_number].Blocks;
        int bsize = iNodeBuf[inode_number].Size;

        Common::FS::IOFile inflated;
        inflated.Open(extractPaths[inode_number], Common::FS::FileAccessMode::Write);

        Common::FS::IOFile pkgFile; // Open the file for each iteration to avoid conflict.
        pkgFile.Open(pkgpath, Common::FS::FileAccessMode::Read);

        int size_decompressed = 0;
        std::vector<char> compressedData;
        std::vector<char> decompressedData(0x10000);

        u64 pfsc_buf_size = 0x11000; // extra 0x1000
        std::vector<u8> pfsc(pfsc_buf_size);
        std::vector<u8> pfs_decrypted(pfsc_buf_size);

        for (int j = 0; j < nblocks; j++) {
            u64 sectorOffset =
                sectorMap[sector_loc + j]; // offset into PFSC_image and not pfs_image.
            u64 sectorSize = sectorMap[sector_loc + j + 1] -
                             sectorOffset; // indicates if data is compressed or not.
            u64 fileOffset = (pkgheader.pfs_image_offset + pfsc_offset + sectorOffset);
            u64 currentSector1 =
                (pfsc_offset + sectorOffset) / 0x1000; // block size is 0x1000 for xts decryption.

            int sectorOffsetMask = (sectorOffset + pfsc_offset) & 0xFFFFF000;
            int previousData = (sectorOffset + pfsc_offset) - sectorOffsetMask;

            pkgFile.Seek(fileOffset - previousData);
            pkgFile.Read(pfsc);

            PKG::crypto.decryptPFS(dataKey, tweakKey, pfsc, pfs_decrypted, currentSector1);

            compressedData.resize(sectorSize);
            std::memcpy(compressedData.data(), pfs_decrypted.data() + previousData, sectorSize);

            if (sectorSize == 0x10000) // Uncompressed data
                std::memcpy(decompressedData.data(), compressedData.data(), 0x10000);
            else if (sectorSize < 0x10000) // Compressed data
                DecompressPFSC(compressedData, decompressedData);

            size_decompressed += 0x10000;

            if (j < nblocks - 1) {
                inflated.WriteRaw<u8>(decompressedData.data(), decompressedData.size());
            } else {
                // This is to remove the zeros at the end of the file.
                const u32 write_size = decompressedData.size() - (size_decompressed - bsize);
                inflated.WriteRaw<u8>(decompressedData.data(), write_size);
            }
        }
        pkgFile.Close();
        inflated.Close();
    }
}
