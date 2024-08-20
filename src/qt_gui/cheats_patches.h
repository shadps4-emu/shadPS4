// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CHEATS_PATCHES_H
#define CHEATS_PATCHES_H

#include <QCheckBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class CheatsPatches : public QWidget {
    Q_OBJECT

public:
    CheatsPatches(const QString& gameName, const QString& gameSerial, const QString& gameVersion,
                  const QString& gameSize, const QPixmap& gameImage, QWidget* parent = nullptr);
    ~CheatsPatches();

private:
    void setupUI();
    void loadCheats(const QString& filePath);
    void addMods(const QJsonArray& modsArray);
    void applyCheat(const QString& modName, bool enabled);

    struct MemoryMod {
        QString offset;
        QString on;
        QString off;
    };

    struct Cheat {
        QString name;
        QString type;
        QVector<MemoryMod> memoryMods;
    };

    QString m_gameName;
    QString m_gameSerial;
    QString m_gameVersion;
    QString m_gameSize;
    QPixmap m_gameImage;
    QString m_cheatFilePath;

    QVBoxLayout* rightLayout;
    QString checkBoxStyle;
    QString buttonStyle;

    QMap<QString, Cheat> m_cheats;
};

#endif // CHEATS_PATCHES_H
