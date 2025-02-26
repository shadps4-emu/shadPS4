// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <codecvt>
#include <sstream>
#include <string>
#include <pugixml.hpp>
#ifdef ENABLE_QT_GUI
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListView>
#include <QMessageBox>
#include <QString>
#include <QXmlStreamReader>
#endif
#include "common/logging/log.h"
#include "common/path_util.h"
#include "memory_patcher.h"

namespace MemoryPatcher {

uintptr_t g_eboot_address;
uint64_t g_eboot_image_size;
std::string g_game_serial;
std::string patchFile;
std::vector<patchInfo> pending_patches;

std::string toHex(u64 value, size_t byteSize) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(byteSize * 2) << value;
    return ss.str();
}

std::string convertValueToHex(const std::string type, const std::string valueStr) {
    std::string result;

    if (type == "byte") {
        const u32 value = std::stoul(valueStr, nullptr, 16);
        result = toHex(value, 1);
    } else if (type == "bytes16") {
        const u32 value = std::stoul(valueStr, nullptr, 16);
        result = toHex(value, 2);
    } else if (type == "bytes32") {
        const u32 value = std::stoul(valueStr, nullptr, 16);
        result = toHex(value, 4);
    } else if (type == "bytes64") {
        const u64 value = std::stoull(valueStr, nullptr, 16);
        result = toHex(value, 8);
    } else if (type == "float32") {
        union {
            float f;
            uint32_t i;
        } floatUnion;
        floatUnion.f = std::stof(valueStr);
        result = toHex(floatUnion.i, sizeof(floatUnion.i));
    } else if (type == "float64") {
        union {
            double d;
            uint64_t i;
        } doubleUnion;
        doubleUnion.d = std::stod(valueStr);
        result = toHex(doubleUnion.i, sizeof(doubleUnion.i));
    } else if (type == "utf8") {
        std::vector<unsigned char> byteArray =
            std::vector<unsigned char>(valueStr.begin(), valueStr.end());
        byteArray.push_back('\0');
        std::stringstream ss;
        for (unsigned char c : byteArray) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        result = ss.str();
    } else if (type == "utf16") {
        std::wstring wide_str(valueStr.size(), L'\0');
        std::mbstowcs(&wide_str[0], valueStr.c_str(), valueStr.size());
        wide_str.resize(std::wcslen(wide_str.c_str()));

        std::u16string valueStringU16;

        for (wchar_t wc : wide_str) {
            if (wc <= 0xFFFF) {
                valueStringU16.push_back(static_cast<char16_t>(wc));
            } else {
                wc -= 0x10000;
                valueStringU16.push_back(static_cast<char16_t>(0xD800 | (wc >> 10)));
                valueStringU16.push_back(static_cast<char16_t>(0xDC00 | (wc & 0x3FF)));
            }
        }

        std::vector<unsigned char> byteArray;
        // convert to little endian
        for (char16_t ch : valueStringU16) {
            unsigned char low_byte = static_cast<unsigned char>(ch & 0x00FF);
            unsigned char high_byte = static_cast<unsigned char>((ch >> 8) & 0x00FF);

            byteArray.push_back(low_byte);
            byteArray.push_back(high_byte);
        }
        byteArray.push_back('\0');
        byteArray.push_back('\0');
        std::stringstream ss;

        for (unsigned char ch : byteArray) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
        result = ss.str();
    } else if (type == "bytes") {
        result = valueStr;
    } else if (type == "mask" || type == "mask_jump32") {
        result = valueStr;
    } else {
        LOG_INFO(Loader, "Error applying Patch, unknown type: {}", type);
    }
    return result;
}

void OnGameLoaded() {

    if (!patchFile.empty()) {
        std::filesystem::path patchDir = Common::FS::GetUserPath(Common::FS::PathType::PatchesDir);

        auto filePath = (patchDir / patchFile).native();

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filePath.c_str());

        if (result) {
            auto patchXML = doc.child("Patch");
            for (pugi::xml_node_iterator it = patchXML.children().begin();
                 it != patchXML.children().end(); ++it) {

                if (std::string(it->name()) == "Metadata") {
                    if (std::string(it->attribute("isEnabled").value()) == "true") {
                        auto patchList = it->first_child();

                        std::string currentPatchName = it->attribute("Name").value();

                        for (pugi::xml_node_iterator patchLineIt = patchList.children().begin();
                             patchLineIt != patchList.children().end(); ++patchLineIt) {

                            std::string type = patchLineIt->attribute("Type").value();
                            std::string address = patchLineIt->attribute("Address").value();
                            std::string patchValue = patchLineIt->attribute("Value").value();
                            std::string maskOffsetStr = patchLineIt->attribute("Offset").value();
                            std::string targetStr = "";
                            std::string sizeStr = "";
                            if (type == "mask_jump32") {
                                targetStr = patchLineIt->attribute("Target").value();
                                sizeStr = patchLineIt->attribute("Size").value();
                            } else {
                                patchValue = convertValueToHex(type, patchValue);
                            }

                            bool littleEndian = false;

                            if (type == "bytes16" || type == "bytes32" || type == "bytes64") {
                                littleEndian = true;
                            }

                            MemoryPatcher::PatchMask patchMask = MemoryPatcher::PatchMask::None;
                            int maskOffsetValue = 0;

                            if (type == "mask")
                                patchMask = MemoryPatcher::PatchMask::Mask;

                            if (type == "mask_jump32")
                                patchMask = MemoryPatcher::PatchMask::Mask_Jump32;

                            if (type == "mask" || type == "mask_jump32" && !maskOffsetStr.empty()) {
                                maskOffsetValue = std::stoi(maskOffsetStr, 0, 10);
                            }

                            MemoryPatcher::PatchMemory(currentPatchName, address, patchValue,
                                                       targetStr, sizeStr, false, littleEndian,
                                                       patchMask, maskOffsetValue);
                        }
                    }
                }
            }
        } else
            LOG_ERROR(Loader, "couldnt patch parse xml : {}", result.description());

        ApplyPendingPatches();
        return;
    }

#ifdef ENABLE_QT_GUI
    // We use the QT headers for the xml and json parsing, this define is only true on QT builds
    QString patchDir;
    Common::FS::PathToQString(patchDir, Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QDir dir(patchDir);
    QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& folder : folders) {
        QString filesJsonPath = patchDir + "/" + folder + "/files.json";

        QFile jsonFile(filesJsonPath);
        if (!jsonFile.open(QIODevice::ReadOnly)) {
            LOG_ERROR(Loader, "Unable to open files.json for reading in repository {}",
                      folder.toStdString());
            continue;
        }

        QByteArray jsonData = jsonFile.readAll();
        jsonFile.close();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObject = jsonDoc.object();

        QString selectedFileName;
        QString serial = QString::fromStdString(g_game_serial);

        for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
            QString filePath = it.key();
            QJsonArray idsArray = it.value().toArray();

            if (idsArray.contains(QJsonValue(serial))) {
                selectedFileName = filePath;
                break;
            }
        }

        if (selectedFileName.isEmpty()) {
            LOG_ERROR(Loader, "No patch file found for the current serial in repository {}",
                      folder.toStdString());
            continue;
        }

        QString filePath = patchDir + "/" + folder + "/" + selectedFileName;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            LOG_ERROR(Loader, "Unable to open the file for reading.");
            continue;
        }

        QByteArray xmlData = file.readAll();
        file.close();

        QString newXmlData;

        QXmlStreamReader xmlReader(xmlData);
        bool insideMetadata = false;

        bool isEnabled = false;
        std::string currentPatchName;
        while (!xmlReader.atEnd()) {
            xmlReader.readNext();

            if (xmlReader.isStartElement()) {
                QJsonArray patchLines;
                if (xmlReader.name() == QStringLiteral("Metadata")) {
                    insideMetadata = true;

                    QString name = xmlReader.attributes().value("Name").toString();
                    currentPatchName = name.toStdString();

                    // Check and update the isEnabled attribute
                    for (const QXmlStreamAttribute& attr : xmlReader.attributes()) {
                        if (attr.name() == QStringLiteral("isEnabled")) {
                            if (attr.value().toString() == "true") {
                                isEnabled = true;
                            } else
                                isEnabled = false;
                        }
                    }
                } else if (xmlReader.name() == QStringLiteral("PatchList")) {
                    QJsonArray linesArray;
                    while (!xmlReader.atEnd() &&
                           !(xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                             xmlReader.name() == QStringLiteral("PatchList"))) {
                        xmlReader.readNext();
                        if (xmlReader.tokenType() == QXmlStreamReader::StartElement &&
                            xmlReader.name() == QStringLiteral("Line")) {
                            QXmlStreamAttributes attributes = xmlReader.attributes();
                            QJsonObject lineObject;
                            lineObject["Type"] = attributes.value("Type").toString();
                            lineObject["Address"] = attributes.value("Address").toString();
                            lineObject["Value"] = attributes.value("Value").toString();
                            lineObject["Offset"] = attributes.value("Offset").toString();
                            if (lineObject["Type"].toString() == "mask_jump32") {
                                lineObject["Target"] = attributes.value("Target").toString();
                                lineObject["Size"] = attributes.value("Size").toString();
                            }
                            linesArray.append(lineObject);
                        }
                    }

                    patchLines = linesArray;
                    if (isEnabled) {
                        foreach (const QJsonValue& value, patchLines) {
                            QJsonObject lineObject = value.toObject();
                            QString type = lineObject["Type"].toString();
                            QString address = lineObject["Address"].toString();
                            QString patchValue = lineObject["Value"].toString();
                            QString maskOffsetStr = lineObject["Offset"].toString();

                            QString targetStr;
                            QString sizeStr;
                            if (type == "mask_jump32") {
                                targetStr = lineObject["Target"].toString();
                                sizeStr = lineObject["Size"].toString();
                            } else {
                                patchValue = QString::fromStdString(convertValueToHex(
                                    type.toStdString(), patchValue.toStdString()));
                            }

                            bool littleEndian = false;

                            if (type == "bytes16" || type == "bytes32" || type == "bytes64")
                                littleEndian = true;

                            MemoryPatcher::PatchMask patchMask = MemoryPatcher::PatchMask::None;
                            int maskOffsetValue = 0;

                            if (type == "mask")
                                patchMask = MemoryPatcher::PatchMask::Mask;

                            if (type == "mask_jump32")
                                patchMask = MemoryPatcher::PatchMask::Mask_Jump32;

                            if (type == "mask" ||
                                type == "mask_jump32" && !maskOffsetStr.toStdString().empty()) {
                                maskOffsetValue = std::stoi(maskOffsetStr.toStdString(), 0, 10);
                            }

                            MemoryPatcher::PatchMemory(
                                currentPatchName, address.toStdString(), patchValue.toStdString(),
                                targetStr.toStdString(), sizeStr.toStdString(), false, littleEndian,
                                patchMask, maskOffsetValue);
                        }
                    }
                }
            }
        }

        if (xmlReader.hasError()) {
            LOG_ERROR(Loader, "Failed to parse XML for {}", g_game_serial);
        } else {
            LOG_INFO(Loader, "Patches loaded successfully");
        }
        ApplyPendingPatches();
    }
#endif
}

void AddPatchToQueue(patchInfo patchToAdd) {
    pending_patches.push_back(patchToAdd);
}

void ApplyPendingPatches() {

    for (size_t i = 0; i < pending_patches.size(); ++i) {
        patchInfo currentPatch = pending_patches[i];

        if (currentPatch.gameSerial != g_game_serial)
            continue;

        PatchMemory(currentPatch.modNameStr, currentPatch.offsetStr, currentPatch.valueStr, "", "",
                    currentPatch.isOffset, currentPatch.littleEndian, currentPatch.patchMask,
                    currentPatch.maskOffset);
    }

    pending_patches.clear();
}

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr,
                 std::string targetStr, std::string sizeStr, bool isOffset, bool littleEndian,
                 PatchMask patchMask, int maskOffset) {
    // Send a request to modify the process memory.
    void* cheatAddress = nullptr;

    if (patchMask == PatchMask::None) {
        if (isOffset) {
            cheatAddress = reinterpret_cast<void*>(g_eboot_address + std::stoi(offsetStr, 0, 16));
        } else {
            cheatAddress =
                reinterpret_cast<void*>(g_eboot_address + (std::stoi(offsetStr, 0, 16) - 0x400000));
        }
    }

    if (patchMask == PatchMask::Mask) {
        cheatAddress = reinterpret_cast<void*>(PatternScan(offsetStr) + maskOffset);
    }

    if (patchMask == PatchMask::Mask_Jump32) {
        int jumpSize = std::stoi(sizeStr);

        constexpr int MAX_PATTERN_LENGTH = 256;
        if (jumpSize < 5) {
            LOG_ERROR(Loader, "Jump size must be at least 5 bytes");
            return;
        }
        if (jumpSize > MAX_PATTERN_LENGTH) {
            LOG_ERROR(Loader, "Jump size must be no more than {} bytes.", MAX_PATTERN_LENGTH);
            return;
        }

        // Find the base address using "Address"
        uintptr_t baseAddress = PatternScan(offsetStr);
        if (baseAddress == 0) {
            LOG_ERROR(Loader, "PatternScan failed for mask_jump32 with pattern: {}", offsetStr);
            return;
        }
        uintptr_t patchAddress = baseAddress + maskOffset;

        // Fills the original region (jumpSize bytes) with NOPs
        std::vector<u8> nopBytes(jumpSize, 0x90);
        std::memcpy(reinterpret_cast<void*>(patchAddress), nopBytes.data(), nopBytes.size());

        // Use "Target" to locate the start of the code cave
        uintptr_t jump_target = PatternScan(targetStr);
        if (jump_target == 0) {
            LOG_ERROR(Loader, "PatternScan failed to Target with pattern: {}", targetStr);
            return;
        }

        // Converts the Value attribute to a byte array (payload)
        std::vector<u8> payload;
        for (size_t i = 0; i < valueStr.length(); i += 2) {

            std::string tempStr = valueStr.substr(i, 2);
            const char* byteStr = tempStr.c_str();
            char* endPtr;
            unsigned int byteVal = std::strtoul(byteStr, &endPtr, 16);

            if (endPtr != byteStr + 2) {
                LOG_ERROR(Loader, "Invalid byte in Value: {}", valueStr.substr(i, 2));
                return;
            }
            payload.push_back(static_cast<u8>(byteVal));
        }

        // Calculates the end of the code cave (where the return jump will be inserted)
        uintptr_t code_cave_end = jump_target + payload.size();

        // Write the payload to the code cave, from jump_target
        std::memcpy(reinterpret_cast<void*>(jump_target), payload.data(), payload.size());

        // Inserts the initial jump in the original region to divert to the code cave
        u8 jumpInstruction[5];
        jumpInstruction[0] = 0xE9;
        s32 relJump = static_cast<s32>(jump_target - patchAddress - 5);
        std::memcpy(&jumpInstruction[1], &relJump, sizeof(relJump));
        std::memcpy(reinterpret_cast<void*>(patchAddress), jumpInstruction,
                    sizeof(jumpInstruction));

        // Inserts jump back at the end of the code cave to resume execution after patching
        u8 jumpBack[5];
        jumpBack[0] = 0xE9;
        // Calculates the relative offset to return to the instruction immediately following the
        // overwritten region
        s32 target_return = static_cast<s32>((patchAddress + jumpSize) - (code_cave_end + 5));
        std::memcpy(&jumpBack[1], &target_return, sizeof(target_return));
        std::memcpy(reinterpret_cast<void*>(code_cave_end), jumpBack, sizeof(jumpBack));

        LOG_INFO(Loader,
                 "Applied Patch mask_jump32: {}, PatchAddress: {:#x}, JumpTarget: {:#x}, "
                 "CodeCaveEnd: {:#x}, JumpSize: {}",
                 modNameStr, patchAddress, jump_target, code_cave_end, jumpSize);
        return;
    }

    if (cheatAddress == nullptr) {
        LOG_ERROR(Loader, "Failed to get address for patch {}", modNameStr);
        return;
    }

    std::vector<unsigned char> bytePatch;

    for (size_t i = 0; i < valueStr.length(); i += 2) {
        unsigned char byte =
            static_cast<unsigned char>(std::strtol(valueStr.substr(i, 2).c_str(), nullptr, 16));

        bytePatch.push_back(byte);
    }

    if (littleEndian) {
        std::reverse(bytePatch.begin(), bytePatch.end());
    }

    std::memcpy(cheatAddress, bytePatch.data(), bytePatch.size());

    LOG_INFO(Loader, "Applied patch: {}, Offset: {}, Value: {}", modNameStr,
             (uintptr_t)cheatAddress, valueStr);
}

static std::vector<int32_t> PatternToByte(const std::string& pattern) {
    std::vector<int32_t> bytes;
    const char* start = pattern.data();
    const char* end = start + pattern.size();

    for (const char* current = start; current < end; ++current) {
        if (*current == '?') {
            ++current;
            if (*current == '?')
                ++current;
            bytes.push_back(-1);
        } else {
            bytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
        }
    }

    return bytes;
}

uintptr_t PatternScan(const std::string& signature) {
    std::vector<int32_t> patternBytes = PatternToByte(signature);
    const auto scanBytes = static_cast<uint8_t*>((void*)g_eboot_address);

    const int32_t* sigPtr = patternBytes.data();
    const size_t sigSize = patternBytes.size();

    uint32_t foundResults = 0;
    for (uint32_t i = 0; i < g_eboot_image_size - sigSize; ++i) {
        bool found = true;
        for (uint32_t j = 0; j < sigSize; ++j) {
            if (scanBytes[i + j] != sigPtr[j] && sigPtr[j] != -1) {
                found = false;
                break;
            }
        }

        if (found) {
            foundResults++;
            return reinterpret_cast<uintptr_t>(&scanBytes[i]);
        }
    }

    return 0;
}

} // namespace MemoryPatcher
