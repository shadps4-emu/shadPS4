// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QActionGroup>
#include <QDragEnterEvent>
#include <QMainWindow>
#include <QMimeData>
#include "main_window_themes.h"
#include "main_window_ui.h"

class GuiSettings;
class GameListFrame;

class MainWindow : public QMainWindow {
    Q_OBJECT

    std::unique_ptr<Ui_MainWindow> ui;

    bool m_is_list_mode = true;
    bool m_save_slider_pos = false;
    int m_other_slider_pos = 0;

public:
    explicit MainWindow(std::shared_ptr<GuiSettings> gui_settings, QWidget* parent = nullptr);
    ~MainWindow();
    bool Init();
    void InstallPkg();
    void InstallDragDropPkg(std::string file, int pkgNum, int nPkg);
    void InstallDirectory();

private Q_SLOTS:
    void ConfigureGuiFromSettings();
    void SetIconSizeActions(int idx) const;
    void ResizeIcons(int index);
    void SaveWindowState() const;

private:
    void CreateActions();
    void CreateDockWindows();
    void CreateConnects();
    void SetLastUsedTheme();

    QActionGroup* m_icon_size_act_group = nullptr;
    QActionGroup* m_list_mode_act_group = nullptr;
    QActionGroup* m_theme_act_group = nullptr;

    // Dockable widget frames
    QMainWindow* m_main_window = nullptr;
    GameListFrame* m_game_list_frame = nullptr;
    WindowThemes m_window_themes;

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
};
