// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QActionGroup>
#include <QDragEnterEvent>
#include <QProcess>
#include <QTranslator>

#include "background_music_player.h"
#include "common/config.h"
#include "common/path_util.h"
#include "compatibility_info.h"
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "elf_viewer.h"
#include "emulator.h"
#include "game_grid_frame.h"
#include "game_info.h"
#include "game_list_frame.h"
#include "game_list_utils.h"
#include "main_window_themes.h"
#include "main_window_ui.h"
#include "pkg_viewer.h"

class GameListFrame;

class MainWindow : public QMainWindow {
    Q_OBJECT
signals:
    void WindowResized(QResizeEvent* event);
    void ExtractionFinished();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    bool Init();
    void InstallDragDropPkg(std::filesystem::path file, int pkgNum, int nPkg);
    void InstallDirectory();
    void StartGame();
    void PauseGame();
    bool showLabels;

private Q_SLOTS:
    void ConfigureGuiFromSettings();
    void SaveWindowState() const;
    void SearchGameTable(const QString& text);
    void ShowGameList();
    void RefreshGameTable();
    void HandleResize(QResizeEvent* event);
    void OnLanguageChanged(const std::string& locale);
    void toggleLabelsUnderIcons();

private:
    Ui_MainWindow* ui;
    void AddUiWidgets();
    void UpdateToolbarLabels();
    void UpdateToolbarButtons();
    QWidget* createButtonWithLabel(QPushButton* button, const QString& labelText, bool showLabel);
    void CreateActions();
    void toggleFullscreen();
    void CreateRecentGameActions();
    void CreateDockWindows();
    void GetPhysicalDevices();
    void LoadGameLists();

#ifdef ENABLE_UPDATER
    void CheckUpdateMain(bool checkSave);
#endif
    void CreateConnects();
    void SetLastUsedTheme();
    void SetLastIconSizeBullet();
    void SetUiIcons(bool isWhite);
    void InstallPkg();
    void BootGame();
    void AddRecentFiles(QString filePath);
    void LoadTranslation();
    void PlayBackgroundMusic();
    QIcon RecolorIcon(const QIcon& icon, bool isWhite);
    void StartEmulator(std::filesystem::path);
    bool isIconBlack = false;
    bool isTableList = true;
    bool isGameRunning = false;
    bool isWhite = false;
    bool is_paused = false;

    QActionGroup* m_icon_size_act_group = nullptr;
    QActionGroup* m_list_mode_act_group = nullptr;
    QActionGroup* m_theme_act_group = nullptr;
    QActionGroup* m_recent_files_group = nullptr;
    PKG pkg;
    // Dockable widget frames
    WindowThemes m_window_themes;
    GameListUtils m_game_list_utils;
    QScopedPointer<QDockWidget> m_dock_widget;
    // Game Lists
    QScopedPointer<GameListFrame> m_game_list_frame;
    QScopedPointer<GameGridFrame> m_game_grid_frame;
    QScopedPointer<ElfViewer> m_elf_viewer;
    // Status Bar.
    QScopedPointer<QStatusBar> statusBar;
    // Available GPU devices
    std::vector<QString> m_physical_devices;

    PSF psf;

    std::shared_ptr<GameInfoClass> m_game_info = std::make_shared<GameInfoClass>();
    std::shared_ptr<CompatibilityInfoClass> m_compat_info =
        std::make_shared<CompatibilityInfoClass>();

    QTranslator* translator;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event1) override {
        if (event1->mimeData()->hasUrls()) {
            event1->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event1) override {
        const QMimeData* mimeData = event1->mimeData();
        if (mimeData->hasUrls()) {
            QList<QUrl> urlList = mimeData->urls();
            int pkgNum = 0;
            int nPkg = urlList.size();
            for (const QUrl& url : urlList) {
                pkgNum++;
                std::filesystem::path path = Common::FS::PathFromQString(url.toLocalFile());
                InstallDragDropPkg(path, pkgNum, nPkg);
            }
        }
    }

    void resizeEvent(QResizeEvent* event) override;

    std::filesystem::path last_install_dir = "";
    bool delete_file_on_install = false;
    bool use_for_all_queued = false;
};
