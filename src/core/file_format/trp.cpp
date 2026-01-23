// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/aes.h"
#include "common/key_manager.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/file_format/npbind.h"
#include "core/file_format/trp.h"

static void DecryptEFSM(std::span<const u8, 16> trophyKey, std::span<const u8, 16> NPcommID,
                        std::span<const u8, 16> efsmIv, std::span<const u8> ciphertext,
                        std::span<u8> decrypted) {
    // Step 1: Encrypt NPcommID
    std::array<u8, 16> trophyIv{};
    std::array<u8, 16> trpKey;
    // Convert spans to pointers for the aes functions
    aes::encrypt_cbc(NPcommID.data(), NPcommID.size(), trophyKey.data(), trophyKey.size(),
                     trophyIv.data(), trpKey.data(), trpKey.size(), false);

    // Step 2: Decrypt EFSM
    aes::decrypt_cbc(ciphertext.data(), ciphertext.size(), trpKey.data(), trpKey.size(),
                     const_cast<u8*>(efsmIv.data()), decrypted.data(), decrypted.size(), nullptr);
}

TRP::TRP() = default;
TRP::~TRP() = default;

static void removePadding(std::vector<u8>& vec) {
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        if (*it == '>') {
            size_t pos = std::distance(vec.begin(), it.base());
            vec.resize(pos);
            break;
        }
    }
}

static void hexToBytes(const char* hex, unsigned char* dst) {
    for (size_t i = 0; hex[i] != 0; i++) {
        const unsigned char value = (hex[i] < 0x3A) ? (hex[i] - 0x30) : (hex[i] - 0x37);
        dst[i / 2] |= ((i % 2) == 0) ? (value << 4) : (value);
    }
}

bool TRP::Extract(const std::filesystem::path& trophyPath, const std::string titleId) {
    std::filesystem::path gameSysDir = trophyPath / "sce_sys/trophy/";
    if (!std::filesystem::exists(gameSysDir)) {
        LOG_WARNING(Common_Filesystem, "Game trophy directory doesn't exist");
        return false;
    }

    const auto& user_key_vec =
        KeyManager::GetInstance()->GetAllKeys().TrophyKeySet.ReleaseTrophyKey;

    if (user_key_vec.size() != 16) {
        LOG_INFO(Common_Filesystem, "Trophy decryption key is not specified");
        return false;
    }

    std::array<u8, 16> user_key{};
    std::copy(user_key_vec.begin(), user_key_vec.end(), user_key.begin());

    // Load npbind.dat using the new class
    std::filesystem::path npbindPath = trophyPath / "sce_sys/npbind.dat";
    NPBindFile npbind;
    if (!npbind.Load(npbindPath.string())) {
        LOG_WARNING(Common_Filesystem, "Failed to load npbind.dat file");
    }

    auto npCommIds = npbind.GetNpCommIds();
    if (npCommIds.empty()) {
        LOG_WARNING(Common_Filesystem, "No NPComm IDs found in npbind.dat");
    }

    bool success = true;
    int trpFileIndex = 0;

    try {
        // Process each TRP file in the trophy directory
        for (const auto& it : std::filesystem::directory_iterator(gameSysDir)) {
            if (!it.is_regular_file() || it.path().extension() != ".trp") {
                continue; // Skip non-TRP files
            }

            // Get NPCommID for this TRP file (if available)
            std::string npCommId;
            if (trpFileIndex < static_cast<int>(npCommIds.size())) {
                npCommId = npCommIds[trpFileIndex];
                LOG_DEBUG(Common_Filesystem, "Using NPCommID: {} for {}", npCommId,
                          it.path().filename().string());
            } else {
                LOG_WARNING(Common_Filesystem, "No NPCommID found for TRP file index {}",
                            trpFileIndex);
            }

            Common::FS::IOFile file(it.path(), Common::FS::FileAccessMode::Read);
            if (!file.IsOpen()) {
                LOG_ERROR(Common_Filesystem, "Unable to open trophy file: {}", it.path().string());
                success = false;
                continue;
            }

            TrpHeader header;
            if (!file.Read(header)) {
                LOG_ERROR(Common_Filesystem, "Failed to read TRP header from {}",
                          it.path().string());
                success = false;
                continue;
            }

            if (header.magic != TRP_MAGIC) {
                LOG_ERROR(Common_Filesystem, "Wrong trophy magic number in {}", it.path().string());
                success = false;
                continue;
            }

            s64 seekPos = sizeof(TrpHeader);
            std::filesystem::path trpFilesPath(
                Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / titleId /
                "TrophyFiles" / it.path().stem());

            // Create output directories
            if (!std::filesystem::create_directories(trpFilesPath / "Icons") ||
                !std::filesystem::create_directories(trpFilesPath / "Xml")) {
                LOG_ERROR(Common_Filesystem, "Failed to create output directories for {}", titleId);
                success = false;
                continue;
            }

            // Process each entry in the TRP file
            for (int i = 0; i < header.entry_num; i++) {
                if (!file.Seek(seekPos)) {
                    LOG_ERROR(Common_Filesystem, "Failed to seek to TRP entry offset");
                    success = false;
                    break;
                }
                seekPos += static_cast<s64>(header.entry_size);

                TrpEntry entry;
                if (!file.Read(entry)) {
                    LOG_ERROR(Common_Filesystem, "Failed to read TRP entry");
                    success = false;
                    break;
                }

                std::string_view name(entry.entry_name);

                if (entry.flag == ENTRY_FLAG_PNG) {
                    if (!ProcessPngEntry(file, entry, trpFilesPath, name)) {
                        success = false;
                        // Continue with next entry
                    }
                } else if (entry.flag == ENTRY_FLAG_ENCRYPTED_XML) {
                    // Check if we have a valid NPCommID for decryption
                    if (npCommId.size() >= 12 && npCommId[0] == 'N' && npCommId[1] == 'P') {
                        if (!ProcessEncryptedXmlEntry(file, entry, trpFilesPath, name, user_key,
                                                      npCommId)) {
                            success = false;
                            // Continue with next entry
                        }
                    } else {
                        LOG_WARNING(Common_Filesystem,
                                    "Skipping encrypted XML entry - invalid NPCommID");
                        // Skip this entry but continue
                    }
                } else {
                    LOG_DEBUG(Common_Filesystem, "Unknown entry flag: {} for {}",
                              static_cast<unsigned int>(entry.flag), name);
                }
            }

            trpFileIndex++;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_CRITICAL(Common_Filesystem, "Filesystem error during trophy extraction: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_CRITICAL(Common_Filesystem, "Error during trophy extraction: {}", e.what());
        return false;
    }

    if (success) {
        LOG_INFO(Common_Filesystem, "Successfully extracted {} trophy files for {}", trpFileIndex,
                 titleId);
    }

    return success;
}

bool TRP::ProcessPngEntry(Common::FS::IOFile& file, const TrpEntry& entry,
                          const std::filesystem::path& outputPath, std::string_view name) {
    if (!file.Seek(entry.entry_pos)) {
        LOG_ERROR(Common_Filesystem, "Failed to seek to PNG entry offset");
        return false;
    }

    std::vector<u8> icon(entry.entry_len);
    if (!file.Read(icon)) {
        LOG_ERROR(Common_Filesystem, "Failed to read PNG data");
        return false;
    }

    auto outputFile = outputPath / "Icons" / name;
    size_t written = Common::FS::IOFile::WriteBytes(outputFile, icon);
    if (written != icon.size()) {
        LOG_ERROR(Common_Filesystem, "PNG write failed: wanted {} bytes, wrote {}", icon.size(),
                  written);
        return false;
    }

    return true;
}

bool TRP::ProcessEncryptedXmlEntry(Common::FS::IOFile& file, const TrpEntry& entry,
                                   const std::filesystem::path& outputPath, std::string_view name,
                                   const std::array<u8, 16>& user_key,
                                   const std::string& npCommId) {
    constexpr size_t IV_LEN = 16;

    if (!file.Seek(entry.entry_pos)) {
        LOG_ERROR(Common_Filesystem, "Failed to seek to encrypted XML entry offset");
        return false;
    }

    std::array<u8, IV_LEN> esfmIv;
    if (!file.Read(esfmIv)) {
        LOG_ERROR(Common_Filesystem, "Failed to read IV for encrypted XML");
        return false;
    }

    if (entry.entry_len <= IV_LEN) {
        LOG_ERROR(Common_Filesystem, "Encrypted XML entry too small");
        return false;
    }

    // Skip to the encrypted data (after IV)
    if (!file.Seek(entry.entry_pos + IV_LEN)) {
        LOG_ERROR(Common_Filesystem, "Failed to seek to encrypted data");
        return false;
    }

    std::vector<u8> ESFM(entry.entry_len - IV_LEN);
    std::vector<u8> XML(entry.entry_len - IV_LEN);

    if (!file.Read(ESFM)) {
        LOG_ERROR(Common_Filesystem, "Failed to read encrypted XML data");
        return false;
    }

    // Decrypt the data - FIX: Don't check return value since DecryptEFSM returns void
    std::span<const u8, 16> key_span(user_key);

    // Convert npCommId string to span (pad or truncate to 16 bytes)
    std::array<u8, 16> npcommid_array{};
    size_t copy_len = std::min(npCommId.size(), npcommid_array.size());
    std::memcpy(npcommid_array.data(), npCommId.data(), copy_len);
    std::span<const u8, 16> npcommid_span(npcommid_array);

    DecryptEFSM(key_span, npcommid_span, esfmIv, ESFM, XML);

    // Remove padding
    removePadding(XML);

    // Create output filename (replace ESFM with XML)
    std::string xml_name(entry.entry_name);
    size_t pos = xml_name.find("ESFM");
    if (pos != std::string::npos) {
        xml_name.replace(pos, 4, "XML");
    }

    auto outputFile = outputPath / "Xml" / xml_name;
    size_t written = Common::FS::IOFile::WriteBytes(outputFile, XML);
    if (written != XML.size()) {
        LOG_ERROR(Common_Filesystem, "XML write failed: wanted {} bytes, wrote {}", XML.size(),
                  written);
        return false;
    }

    return true;
}