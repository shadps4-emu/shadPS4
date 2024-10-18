// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/path_util.h"

struct GameInfo {
    std::filesystem::path path;      // root path of game directory
                                     // (normally directory that contains eboot.bin)
    std::filesystem::path icon_path; // path of icon0.png
    std::filesystem::path pic_path;  // path of pic1.png
    std::filesystem::path snd0_path; // path of snd0.at9
    QImage icon;
    std::string size;
    // variables extracted from param.sfo
    std::string name = "Unknown";
    std::string serial = "Unknown";
    std::string version = "Unknown";
    std::string region = "Unknown";
    std::string fw = "Unknown";

    std::string play_time = "Unknown";
};

class GameListUtils {
public:
    static QString FormatSize(qint64 size) {
        static const QStringList suffixes = {"B", "KB", "MB", "GB", "TB"};
        int suffixIndex = 0;

        double gameSize = static_cast<double>(size);
        while (gameSize >= 1024 && suffixIndex < suffixes.size() - 1) {
            gameSize /= 1024;
            ++suffixIndex;
        }

        // Format the size with a specified precision
        QString sizeString;
        if (gameSize < 10.0) {
            sizeString = QString::number(gameSize, 'f', 2);
        } else if (gameSize < 100.0) {
            sizeString = QString::number(gameSize, 'f', 1);
        } else {
            sizeString = QString::number(gameSize, 'f', 0);
        }

        return sizeString + " " + suffixes[suffixIndex];
    }

    static void GetFolderSize(GameInfo& game) {
        QString dirPath;
        Common::FS::PathToQString(dirPath, game.path);
        QDir dir(dirPath);
        QDirIterator it(dir.absolutePath(), QDirIterator::Subdirectories);
        qint64 total = 0;
        while (it.hasNext()) {
            it.next();
            total += it.fileInfo().size();
        }
        game.size = FormatSize(total).toStdString();
    }

    static QString GetRegion(char region) {
        switch (region) {
        case 'U':
            return "USA";
        case 'E':
            return "Europe";
        case 'J':
            return "Japan";
        case 'H':
            return "Asia";
        case 'I':
            return "World";
        default:
            return "Unknown";
        }
    }

    static QString GetAppType(int type) {
        switch (type) {
        case 0:
            return "Not Specified";
        case 1:
            return "FULL APP";
        case 2:
            return "UPGRADABLE";
        case 3:
            return "DEMO";
        case 4:
            return "FREEMIUM";
        default:
            return "Unknown";
        }
    }

    QImage BlurImage(const QImage& image, const QRect& rect, int radius) {
        int tab[] = {14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2};
        int alpha = (radius < 1) ? 16 : (radius > 17) ? 1 : tab[radius - 1];

        QImage result = image.convertToFormat(QImage::Format_ARGB32);
        int r1 = rect.top();
        int r2 = rect.bottom();
        int c1 = rect.left();
        int c2 = rect.right();

        int bpl = result.bytesPerLine();
        int rgba[4];
        unsigned char* p;

        int i1 = 0;
        int i2 = 3;

        for (int col = c1; col <= c2; col++) {
            p = result.scanLine(r1) + col * 4;
            for (int i = i1; i <= i2; i++)
                rgba[i] = p[i] << 4;

            p += bpl;
            for (int j = r1; j < r2; j++, p += bpl)
                for (int i = i1; i <= i2; i++)
                    p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
        }

        for (int row = r1; row <= r2; row++) {
            p = result.scanLine(row) + c1 * 4;
            for (int i = i1; i <= i2; i++)
                rgba[i] = p[i] << 4;

            p += 4;
            for (int j = c1; j < c2; j++, p += 4)
                for (int i = i1; i <= i2; i++)
                    p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
        }

        for (int col = c1; col <= c2; col++) {
            p = result.scanLine(r2) + col * 4;
            for (int i = i1; i <= i2; i++)
                rgba[i] = p[i] << 4;

            p -= bpl;
            for (int j = r1; j < r2; j++, p -= bpl)
                for (int i = i1; i <= i2; i++)
                    p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
        }

        for (int row = r1; row <= r2; row++) {
            p = result.scanLine(row) + c2 * 4;
            for (int i = i1; i <= i2; i++)
                rgba[i] = p[i] << 4;

            p -= 4;
            for (int j = c1; j < c2; j++, p -= 4)
                for (int i = i1; i <= i2; i++)
                    p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
        }

        return result;
    }
};
