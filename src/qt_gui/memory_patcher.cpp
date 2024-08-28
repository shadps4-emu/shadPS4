// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <string>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListView>
#include <QMessageBox>
#include <QXmlStreamReader>
#include "common/logging/log.h"
#include "common/path_util.h"
#include "memory_patcher.h"

namespace MemoryPatcher {

uintptr_t g_eboot_address;
u64 g_eboot_image_size;
std::string g_game_serial;
std::vector<patchInfo> pending_patches;

QString toHex(unsigned long long value, size_t byteSize) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(byteSize * 2) << value;
    return QString::fromStdString(ss.str());
}

QString convertValueToHex(const QString& type, const QString& valueStr) {
    QString result;
    std::string typeStr = type.toStdString();
    std::string valueStrStd = valueStr.toStdString();

    if (typeStr == "byte") {
        unsigned int value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 1);
    } else if (typeStr == "bytes16") {
        unsigned int value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 2);
    } else if (typeStr == "bytes32") {
        unsigned long value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 4);
    } else if (typeStr == "bytes64") {
        unsigned long long value = std::stoull(valueStrStd, nullptr, 16);
        result = toHex(value, 8);
    } else if (typeStr == "float32") {
        union {
            float f;
            uint32_t i;
        } floatUnion;
        floatUnion.f = std::stof(valueStrStd);
        result = toHex(floatUnion.i, sizeof(floatUnion.i));
    } else if (typeStr == "float64") {
        union {
            double d;
            uint64_t i;
        } doubleUnion;
        doubleUnion.d = std::stod(valueStrStd);
        result = toHex(doubleUnion.i, sizeof(doubleUnion.i));
    } else if (typeStr == "utf8") {
        QByteArray byteArray = QString::fromStdString(valueStrStd).toUtf8();
        byteArray.append('\0');
        std::stringstream ss;
        for (unsigned char c : byteArray) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        result = QString::fromStdString(ss.str());
    } else if (typeStr == "utf16") {
        QByteArray byteArray(
            reinterpret_cast<const char*>(QString::fromStdString(valueStrStd).utf16()),
            QString::fromStdString(valueStrStd).size() * 2);
        byteArray.append('\0');
        byteArray.append('\0');
        std::stringstream ss;
        for (unsigned char c : byteArray) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        result = QString::fromStdString(ss.str());
    } else if (typeStr == "bytes") {
        result = valueStr;
    } else if (typeStr == "mask" || typeStr == "mask_jump32") {
        result = valueStr;
    } else {
        LOG_INFO(Loader, "Error applying Patch, unknown type: {}", typeStr);
    }
    return result;
}

void OnGameLoaded() {

    // We use the QT headers for the xml and json parsing, this define is only true on QT builds
    QString patchDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir).string());
    QString repositories[] = {"GoldHEN", "shadPS4"};

    for (const QString& repository : repositories) {
        QString filesJsonPath = patchDir + "/" + repository + "/files.json";

        QFile jsonFile(filesJsonPath);
        if (!jsonFile.open(QIODevice::ReadOnly)) {
            LOG_ERROR(Loader, "Unable to open files.json for reading.");
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
            LOG_ERROR(Loader, "No patch file found for the current serial.");
            continue;
        }

        QString filePath = patchDir + "/" + repository + "/" + selectedFileName;
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

                            patchValue = convertValueToHex(type, patchValue);

                            bool littleEndian = false;

                            if (type == "bytes16") {
                                littleEndian = true;
                            } else if (type == "bytes32") {
                                littleEndian = true;
                            } else if (type == "bytes64") {
                                littleEndian = true;
                            }

                            MemoryPatcher::PatchMask patchMask = MemoryPatcher::PatchMask::None;
                            int maskOffsetValue = 0;

                            if (type == "mask") {
                                patchMask = MemoryPatcher::PatchMask::Mask;

                                // im not sure if this works, there is no games to test the mask
                                // offset on yet
                                if (!maskOffsetStr.toStdString().empty())
                                    maskOffsetValue = std::stoi(maskOffsetStr.toStdString(), 0, 10);
                            }

                            if (type == "mask_jump32")
                                patchMask = MemoryPatcher::PatchMask::Mask_Jump32;

                            MemoryPatcher::PatchMemory(currentPatchName, address.toStdString(),
                                                       patchValue.toStdString(), false,
                                                       littleEndian, patchMask);
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
}

void AddPatchToQueue(patchInfo patchToAdd) {
    pending_patches.push_back(patchToAdd);
}

void ApplyPendingPatches() {

    for (size_t i = 0; i < pending_patches.size(); ++i) {
        patchInfo currentPatch = pending_patches[i];

        if (currentPatch.gameSerial != g_game_serial)
            continue;

        PatchMemory(currentPatch.modNameStr, currentPatch.offsetStr, currentPatch.valueStr,
                    currentPatch.isOffset, currentPatch.littleEndian, currentPatch.patchMask,
                    currentPatch.maskOffset);
    }

    pending_patches.clear();
}

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr, bool isOffset,
                 bool littleEndian, PatchMask patchMask, int maskOffset) {
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

    // TODO: implement mask_jump32

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