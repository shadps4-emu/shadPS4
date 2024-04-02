// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QActionGroup>
#include <QDragEnterEvent>
#include <QMainWindow>
#include <QMimeData>
#include "game_grid_frame.h"
#include "game_info.h"
#include "game_list_frame.h"
#include "game_list_utils.h"
#include "main_window_themes.h"
#include "main_window_ui.h"
#include "pkg_viewer.h"

class GuiSettings;
class GameListFrame;

class MainWindow : public QMainWindow {
    Q_OBJECT

    std::unique_ptr<Ui_MainWindow> ui;

    bool m_is_list_mode = true;
    bool m_save_slider_pos = false;
    int m_other_slider_pos = 0;
signals:
    void WindowResized(QResizeEvent* event);

public:
    explicit MainWindow(std::shared_ptr<GuiSettings> gui_settings, QWidget* parent = nullptr);
    ~MainWindow();
    bool Init();
    void InstallPkg();
    void InstallDragDropPkg(std::string file, int pkgNum, int nPkg);
    void InstallDirectory();

private Q_SLOTS:
    void ConfigureGuiFromSettings();
    void SaveWindowState() const;
    void SearchGameTable(const QString& text);
    void RefreshGameTable();
    void HandleResize(QResizeEvent* event);

private:
    void AddUiWidgets();
    void CreateActions();
    void CreateDockWindows();
    void LoadGameLists();
    void CreateConnects();
    void SetLastUsedTheme();
    void SetLastIconSizeBullet();
    void SetUiIcons(bool isWhite);
    QIcon RecolorIcon(const QIcon& icon, bool isWhite);

    bool isIconBlack = false;
    bool isTableList = true;

    QActionGroup* m_icon_size_act_group = nullptr;
    QActionGroup* m_list_mode_act_group = nullptr;
    QActionGroup* m_theme_act_group = nullptr;

    // Dockable widget frames
    QMainWindow* m_main_window = nullptr;
    WindowThemes m_window_themes;
    GameListUtils m_game_list_utils;
    QDockWidget* m_dock_widget = nullptr;
    // Game Lists
    GameListFrame* m_game_list_frame = nullptr;
    GameGridFrame* m_game_grid_frame = nullptr;
    // Packge Viewer
    PKGViewer* m_pkg_viewer = nullptr;
    // Status Bar.
    QStatusBar* statusBar = nullptr;

    std::shared_ptr<GameInfoClass> m_game_info = std::make_shared<GameInfoClass>();
    std::shared_ptr<GuiSettings> m_gui_settings;

protected:
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
                InstallDragDropPkg(url.toLocalFile().toStdString(), pkgNum, nPkg);
            }
        }
    }

    void resizeEvent(QResizeEvent* event) override;
};
