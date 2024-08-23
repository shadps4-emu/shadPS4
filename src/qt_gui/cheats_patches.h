// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CHEATS_PATCHES_H
#define CHEATS_PATCHES_H

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
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
    // UI Setup and Event Handlers
    void setupUI();
    void onTabChanged(int index);
    void updateNoteTextEdit(const QString& patchName);

    // Cheat and Patch Management
    void loadCheats(const QString& filePath);
    void loadPatches(const QString& serial);

    void downloadCheats(const QString& url);
    void downloadPatches(const QString& url);

    void addCheatsToLayout(const QJsonArray& modsArray);
    void addPatchToLayout(const QString& name, const QString& author, const QString& note,
                          const QJsonArray& linesArray);

    void createFilesJson();
    void uncheckAllCheatCheckBoxes();
    void applyCheat(const QString& modName, bool enabled);
    void applyPatch(const QString& patchName, bool enabled);

    // Event Filtering
    bool eventFilter(QObject* obj, QEvent* event);
    void onPatchCheckBoxHovered(QCheckBox* checkBox, bool hovered);

    // Patch Info Structures
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

    struct PatchInfo {
        QString name;
        QString author;
        QString note;
        QJsonArray linesArray;
    };

    // Members
    QString m_gameName;
    QString m_gameSerial;
    QString m_gameVersion;
    QString m_gameSize;
    QPixmap m_gameImage;
    QString m_cheatFilePath;
    QMap<QString, Cheat> m_cheats;
    QMap<QString, PatchInfo> m_patchInfos;
    QVector<QCheckBox*> m_cheatCheckBoxes;

    // UI Elements
    QVBoxLayout* rightLayout;
    QVBoxLayout* patchesGroupBoxLayout;
    QGroupBox* patchesGroupBox;
    QVBoxLayout* patchesLayout;
    QTextEdit* instructionsTextEdit;
};

#endif // CHEATS_PATCHES_H
