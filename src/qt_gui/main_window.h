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
#include "gui_settings.h"
#include "main_window_themes.h"
#include "main_window_ui.h"

class GameListFrame;

class MainWindow : public QMainWindow {
    Q_OBJECT
signals:
    void WindowResized(QResizeEvent* event);

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    bool Init();
    void InstallDirectory();
    void StartGame();
    void StartGameWithPath(const QString&);
    void PauseGame();
    bool showLabels;
    void StopGame();
    void RestartGame();
    std::unique_ptr<Core::Emulator> emulator;
    bool pendingRestart = false;
    qint64 detachedGamePid = -1;
    bool isDetachedLaunch = false;
private Q_SLOTS:
    void ConfigureGuiFromSettings();
    void SaveWindowState();
    void SearchGameTable(const QString& text);
    void ShowGameList();
    void RefreshGameTable();
    void HandleResize(QResizeEvent* event);
    void OnLanguageChanged(const QString& locale);
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
    void LoadGameLists();

#ifdef ENABLE_UPDATER
    void CheckUpdateMain(bool checkSave);
#endif
    void CreateConnects();
#ifdef ENABLE_QT_GUI
    QString getLastEbootPath();
    QString lastGamePath;
#endif
    void SetLastUsedTheme();
    void SetLastIconSizeBullet();
    void SetUiIcons(bool isWhite);
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
    std::string runningGameSerial = "";

    QActionGroup* m_icon_size_act_group = nullptr;
    QActionGroup* m_list_mode_act_group = nullptr;
    QActionGroup* m_theme_act_group = nullptr;
    QActionGroup* m_recent_files_group = nullptr;
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

    PSF psf;

    std::shared_ptr<GameInfoClass> m_game_info = std::make_shared<GameInfoClass>();
    std::shared_ptr<CompatibilityInfoClass> m_compat_info =
        std::make_shared<CompatibilityInfoClass>();

    QTranslator* translator;
    std::shared_ptr<gui_settings> m_gui_settings;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event1) override {
        if (event1->mimeData()->hasUrls()) {
            event1->acceptProposedAction();
        }
    }

    void resizeEvent(QResizeEvent* event) override;

    std::filesystem::path last_install_dir = "";
    bool delete_file_on_install = false;
    bool use_for_all_queued = false;
};