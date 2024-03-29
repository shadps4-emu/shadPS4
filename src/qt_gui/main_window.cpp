// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStatusBar>
#include <QtConcurrent>
#include "common/io_file.h"
#include "core/file_format/pkg.h"
#include "core/loader.h"
#include "game_install_dialog.h"
#include "gui_settings.h"
#include "main_window.h"

MainWindow::MainWindow(std::shared_ptr<GuiSettings> gui_settings, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_gui_settings(std::move(gui_settings)) {
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
}

MainWindow::~MainWindow() {
    SaveWindowState();
}

bool MainWindow::Init() {
    auto start = std::chrono::steady_clock::now();
    AddUiWidgets();
    CreateActions();
    CreateDockWindows();
    CreateConnects();
    SetLastUsedTheme();
    SetLastIconSizeBullet();
    ConfigureGuiFromSettings();
    LoadGameLists();

    setMinimumSize(350, minimumSizeHint().height());
    setWindowTitle(QString::fromStdString("ShadPS4 v0.0.3"));
    show();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    statusBar = new QStatusBar(this);
    m_main_window->setStatusBar(statusBar);
    // Update status bar
    int numGames = m_game_info->m_games.size();
    QString statusMessage = "Games: " + QString::number(numGames) + " (" +
                            QString::number(duration.count()) + "ms). Ready.";
    statusBar->showMessage(statusMessage);
    return true;
}

void MainWindow::CreateActions() {
    // create action group for icon size
    m_icon_size_act_group = new QActionGroup(this);
    m_icon_size_act_group->addAction(ui->setIconSizeTinyAct);
    m_icon_size_act_group->addAction(ui->setIconSizeSmallAct);
    m_icon_size_act_group->addAction(ui->setIconSizeMediumAct);
    m_icon_size_act_group->addAction(ui->setIconSizeLargeAct);

    // create action group for list mode
    m_list_mode_act_group = new QActionGroup(this);
    m_list_mode_act_group->addAction(ui->setlistModeListAct);
    m_list_mode_act_group->addAction(ui->setlistModeGridAct);

    // create action group for themes
    m_theme_act_group = new QActionGroup(this);
    m_theme_act_group->addAction(ui->setThemeLight);
    m_theme_act_group->addAction(ui->setThemeDark);
    m_theme_act_group->addAction(ui->setThemeGreen);
    m_theme_act_group->addAction(ui->setThemeBlue);
    m_theme_act_group->addAction(ui->setThemeViolet);
}

void MainWindow::AddUiWidgets() {
    // add toolbar widgets
    QApplication::setStyle("Fusion");
    ui->toolBar->setObjectName("mw_toolbar");
    ui->toolBar->addWidget(ui->playButton);
    ui->toolBar->addWidget(ui->pauseButton);
    ui->toolBar->addWidget(ui->stopButton);
    ui->toolBar->addWidget(ui->settingsButton);
    ui->toolBar->addWidget(ui->controllerButton);
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::StyledPanel);
    line->setFrameShadow(QFrame::Sunken);
    ui->toolBar->addWidget(line);
    ui->toolBar->addWidget(ui->sizeSliderContainer);
    ui->toolBar->addWidget(ui->mw_searchbar);
}

void MainWindow::CreateDockWindows() {
    m_main_window = new QMainWindow();
    m_main_window->setContextMenuPolicy(Qt::PreventContextMenu);

    // resize window to last W and H
    QSize window_size = m_gui_settings->GetValue(gui::m_window_size).toSize();
    m_main_window->resize(window_size.width(), window_size.height());

    // Add the game table.
    m_dock_widget = new QDockWidget("Game List", m_main_window);
    m_game_list_frame = new GameListFrame(m_game_info, m_gui_settings, m_main_window);
    m_game_list_frame->setObjectName("gamelist");
    m_game_grid_frame = new GameGridFrame(m_game_info, m_gui_settings, m_main_window);
    m_game_grid_frame->setObjectName("gamegridlist");

    int table_mode = m_gui_settings->GetValue(gui::m_table_mode).toInt();
    int slider_pos = 0;
    if (table_mode == 0) { // List
        m_game_grid_frame->hide();
        m_dock_widget->setWidget(m_game_list_frame);
        slider_pos = m_gui_settings->GetValue(gui::m_slide_pos).toInt();
        ui->sizeSlider->setSliderPosition(slider_pos); // set slider pos at start;
        isTableList = true;
    } else { // Grid
        m_game_list_frame->hide();
        m_dock_widget->setWidget(m_game_grid_frame);
        slider_pos = m_gui_settings->GetValue(gui::m_slide_pos_grid).toInt();
        ui->sizeSlider->setSliderPosition(slider_pos); // set slider pos at start;
        isTableList = false;
    }

    m_main_window->addDockWidget(Qt::LeftDockWidgetArea, m_dock_widget);
    m_main_window->setDockNestingEnabled(true);

    setCentralWidget(m_main_window);
}

void MainWindow::LoadGameLists() {
    // Get game info from game folders.
    m_game_info->GetGameInfo();
    if (isTableList) {
        m_game_list_frame->PopulateGameList();
    } else {
        m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    }
}

void MainWindow::CreateConnects() {
    connect(this, &MainWindow::WindowResized, this, &MainWindow::HandleResize);

    connect(ui->mw_searchbar, &QLineEdit::textChanged, this, &MainWindow::SearchGameTable);

    connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);

    connect(ui->sizeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size =
                36 + value; // 36 is the minimum icon size to use due to text disappearing.
            m_game_list_frame->ResizeIcons(36 + value);
            m_gui_settings->SetValue(gui::m_icon_size, 36 + value);
            m_gui_settings->SetValue(gui::m_slide_pos, value);
        } else {
            m_game_grid_frame->icon_size = 69 + value;
            m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
            m_gui_settings->SetValue(gui::m_icon_size_grid, 69 + value);
            m_gui_settings->SetValue(gui::m_slide_pos_grid, value);
        }
    });

    connect(ui->setIconSizeTinyAct, &QAction::triggered, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size =
                36; // 36 is the minimum icon size to use due to text disappearing.
            m_gui_settings->SetValue(gui::m_icon_size, 36);
            ui->sizeSlider->setValue(0); // icone_size - 36
            m_gui_settings->SetValue(gui::m_slide_pos, 0);
        } else {
            m_gui_settings->SetValue(gui::m_icon_size_grid, 69); // nice :3
            ui->sizeSlider->setValue(0);                         // icone_size - 36
            m_gui_settings->SetValue(gui::m_slide_pos_grid, 0);
        }
    });

    connect(ui->setIconSizeSmallAct, &QAction::triggered, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size = 64;
            m_gui_settings->SetValue(gui::m_icon_size, 64);
            ui->sizeSlider->setValue(28);
            m_gui_settings->SetValue(gui::m_slide_pos, 28);
        } else {
            m_gui_settings->SetValue(gui::m_icon_size_grid, 97);
            ui->sizeSlider->setValue(28);
            m_gui_settings->SetValue(gui::m_slide_pos_grid, 28);
        }
    });

    connect(ui->setIconSizeMediumAct, &QAction::triggered, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size = 128;
            m_gui_settings->SetValue(gui::m_icon_size, 128);
            ui->sizeSlider->setValue(92);
            m_gui_settings->SetValue(gui::m_slide_pos, 92);
        } else {
            m_gui_settings->SetValue(gui::m_icon_size_grid, 160);
            ui->sizeSlider->setValue(92);
            m_gui_settings->SetValue(gui::m_slide_pos_grid, 92);
        }
    });

    connect(ui->setIconSizeLargeAct, &QAction::triggered, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size = 256;
            m_gui_settings->SetValue(gui::m_icon_size, 256);
            ui->sizeSlider->setValue(220);
            m_gui_settings->SetValue(gui::m_slide_pos, 220);
        } else {
            m_gui_settings->SetValue(gui::m_icon_size_grid, 256);
            ui->sizeSlider->setValue(220);
            m_gui_settings->SetValue(gui::m_slide_pos_grid, 220);
        }
    });

    connect(ui->setlistModeListAct, &QAction::triggered, m_dock_widget, [this]() {
        m_dock_widget->setWidget(m_game_list_frame);
        m_game_list_frame->show();
        m_game_grid_frame->hide();
        m_game_list_frame->clearContents();
        m_game_list_frame->PopulateGameList();
        isTableList = true;
        m_gui_settings->SetValue(gui::m_table_mode, 0); // save table mode
        int slider_pos = m_gui_settings->GetValue(gui::m_slide_pos).toInt();
        ui->sizeSlider->setSliderPosition(slider_pos);
    });

    connect(ui->setlistModeGridAct, &QAction::triggered, m_dock_widget, [this]() {
        m_dock_widget->setWidget(m_game_grid_frame);
        m_game_grid_frame->show();
        m_game_list_frame->hide();
        isTableList = false;
        m_gui_settings->SetValue(gui::m_table_mode, 1); // save table mode
        int slider_pos_grid = m_gui_settings->GetValue(gui::m_slide_pos_grid).toInt();
        ui->sizeSlider->setSliderPosition(slider_pos_grid);
    });

    // Dump game list.
    connect(ui->dumpGameListAct, &QAction::triggered, this, [this] {
        QString filePath = qApp->applicationDirPath().append("/GameList.txt");
        QFile file(filePath);
        QTextStream out(&file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to open file for writing:" << file.errorString();
            return;
        }
        out << QString("%1 %2 %3 %4 %5\n")
                   .arg("          NAME", -50)
                   .arg("    ID", -10)
                   .arg("FW", -4)
                   .arg(" APP VERSION", -11)
                   .arg("                Path");
        for (const GameInfo& game : m_game_info->m_games) {
            out << QString("%1 %2 %3     %4 %5\n")
                       .arg(QString::fromStdString(game.name), -50)
                       .arg(QString::fromStdString(game.serial), -10)
                       .arg(QString::fromStdString(game.fw), -4)
                       .arg(QString::fromStdString(game.version), -11)
                       .arg(QString::fromStdString(game.path));
        }
    });

    // Package install.
    connect(ui->bootInstallPkgAct, &QAction::triggered, this, [this] { InstallPkg(); });
    connect(ui->gameInstallPathAct, &QAction::triggered, this, [this] { InstallDirectory(); });
    // Package Viewer.
    connect(ui->pkgViewerAct, &QAction::triggered, this, [this]() {
        PKGViewer* pkgViewer = new PKGViewer(m_game_info, m_gui_settings,
                                             [this](std::string file, int pkgNum, int nPkg) {
                                                 this->InstallDragDropPkg(file, pkgNum, nPkg);
                                             });
        pkgViewer->show();
    });

    // Themes
    connect(ui->setThemeLight, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Light, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Light));
        if (!isIconBlack) {
            SetUiIcons(true);
            isIconBlack = true;
        }
    });
    connect(ui->setThemeDark, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Dark, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Dark));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeGreen, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Green, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Green));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeBlue, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Blue, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Blue));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeViolet, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Violet, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Violet));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
}

void MainWindow::SearchGameTable(const QString& text) {
    if (isTableList) {
        for (int row = 0; row < m_game_list_frame->rowCount(); row++) {
            QString game_name = QString::fromStdString(m_game_info->m_games[row].name);
            bool match = (game_name.contains(text, Qt::CaseInsensitive)); // Check only in column 1
            m_game_list_frame->setRowHidden(row, !match);
        }
    } else {
        QVector<GameInfo> filteredGames;
        for (const auto& gameInfo : m_game_info->m_games) {
            QString game_name = QString::fromStdString(gameInfo.name);
            if (game_name.contains(text, Qt::CaseInsensitive)) {
                filteredGames.push_back(gameInfo);
            }
        }
        std::sort(filteredGames.begin(), filteredGames.end(), m_game_info->CompareStrings);
        m_game_grid_frame->PopulateGameGrid(filteredGames, true);
    }
}

void MainWindow::RefreshGameTable() {
    m_game_info->m_games.clear();
    m_game_info->GetGameInfo();
    m_game_list_frame->clearContents();
    m_game_list_frame->PopulateGameList();
    m_game_grid_frame->clearContents();
    m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    statusBar->clearMessage();
    int numGames = m_game_info->m_games.size();
    QString statusMessage = "Games: " + QString::number(numGames) + ". Ready.";
    statusBar->showMessage(statusMessage);
}

void MainWindow::ConfigureGuiFromSettings() {
    // Restore GUI state if needed. We need to if they exist.
    if (!restoreGeometry(m_gui_settings->GetValue(gui::main_window_geometry).toByteArray())) {
        resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
    }

    m_main_window->restoreState(m_gui_settings->GetValue(gui::main_window_mwState).toByteArray());

    ui->showGameListAct->setChecked(
        m_gui_settings->GetValue(gui::main_window_gamelist_visible).toBool());

    if (isTableList) {
        ui->setlistModeListAct->setChecked(true);
    } else {
        ui->setlistModeGridAct->setChecked(true);
    }
}

void MainWindow::SaveWindowState() const {
    // Save gui settings
    m_gui_settings->SetValue(gui::main_window_geometry, saveGeometry());
    m_gui_settings->SetValue(gui::main_window_windowState, saveState());
    m_gui_settings->SetValue(gui::m_window_size,
                             QSize(m_main_window->width(), m_main_window->height()));
    m_gui_settings->SetValue(gui::main_window_mwState, m_main_window->saveState());
}

void MainWindow::InstallPkg() {
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, tr("Install PKG Files"), QDir::currentPath(), tr("PKG File (*.PKG)"));
    int nPkg = fileNames.size();
    int pkgNum = 0;
    for (const QString& file : fileNames) {
        pkgNum++;
        MainWindow::InstallDragDropPkg(file.toStdString(), pkgNum, nPkg);
    }
}

void MainWindow::InstallDragDropPkg(std::string file, int pkgNum, int nPkg) {
    if (Loader::DetectFileType(file) == Loader::FileTypes::Pkg) {
        PKG pkg;
        pkg.Open(file);
        std::string failreason;
        const auto extract_path =
            std::filesystem::path(
                m_gui_settings->GetValue(gui::settings_install_dir).toString().toStdString()) /
            pkg.GetTitleID();
        if (!pkg.Extract(file, extract_path, failreason)) {
            QMessageBox::critical(this, "PKG ERROR", QString::fromStdString(failreason),
                                  QMessageBox::Ok);
        } else {
            int nfiles = pkg.GetNumberOfFiles();

            QList<int> indices;
            for (int i = 0; i < nfiles; i++) {
                indices.append(i);
            }

            QProgressDialog dialog;
            dialog.setWindowTitle("PKG Extraction");
            QString extractmsg = QString("Extracting PKG %1/%2").arg(pkgNum).arg(nPkg);
            dialog.setLabelText(extractmsg);

            // Create a QFutureWatcher and connect signals and slots.
            QFutureWatcher<void> futureWatcher;
            QObject::connect(&futureWatcher, SIGNAL(finished()), &dialog, SLOT(reset()));
            QObject::connect(&dialog, SIGNAL(canceled()), &futureWatcher, SLOT(cancel()));
            QObject::connect(&futureWatcher, SIGNAL(progressRangeChanged(int, int)), &dialog,
                             SLOT(setRange(int, int)));
            QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &dialog,
                             SLOT(setValue(int)));

            futureWatcher.setFuture(QtConcurrent::map(
                indices, std::bind(&PKG::ExtractFiles, pkg, std::placeholders::_1)));

            // Display the dialog and start the event loop.
            dialog.exec();
            futureWatcher.waitForFinished();

            auto path = m_gui_settings->GetValue(gui::settings_install_dir).toString();
            if (pkgNum == nPkg) {
                QMessageBox::information(this, "Extraction Finished",
                                         "Game successfully installed at " + path, QMessageBox::Ok);
                // Refresh game table after extraction.
                RefreshGameTable();
            }
        }
    } else {
        QMessageBox::critical(this, "PKG ERROR", "File doesn't appear to be a valid PKG file",
                              QMessageBox::Ok);
    }
}

void MainWindow::InstallDirectory() {
    GameInstallDialog dlg(m_gui_settings);
    dlg.exec();
}

void MainWindow::SetLastUsedTheme() {
    Theme lastTheme = static_cast<Theme>(m_gui_settings->GetValue(gui::mw_themes).toInt());
    m_window_themes.SetWindowTheme(lastTheme, ui->mw_searchbar);

    switch (lastTheme) {
    case Theme::Light:
        ui->setThemeLight->setChecked(true);
        isIconBlack = true;
        break;
    case Theme::Dark:
        ui->setThemeDark->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    case Theme::Green:
        ui->setThemeGreen->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    case Theme::Blue:
        ui->setThemeBlue->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    case Theme::Violet:
        ui->setThemeViolet->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    }
}

void MainWindow::SetLastIconSizeBullet() {
    // set QAction bullet point if applicable
    int lastSize = m_gui_settings->GetValue(gui::m_icon_size).toInt();
    switch (lastSize) {
    case 36:
        ui->setIconSizeTinyAct->setChecked(true);
        break;
    case 64:
        ui->setIconSizeSmallAct->setChecked(true);
        break;
    case 128:
        ui->setIconSizeMediumAct->setChecked(true);
        break;
    case 256:
        ui->setIconSizeLargeAct->setChecked(true);
        break;
    }
}

QIcon MainWindow::RecolorIcon(const QIcon& icon, bool isWhite) {
    QPixmap pixmap(icon.pixmap(icon.actualSize(QSize(120, 120)), QIcon::Normal));
    QColor clr(isWhite ? Qt::white : Qt::black);
    QBitmap mask = pixmap.createMaskFromColor(clr, Qt::MaskOutColor);
    pixmap.fill(QColor(isWhite ? Qt::black : Qt::white));
    pixmap.setMask(mask);
    QIcon newIcon(pixmap);
    return newIcon;
}

void MainWindow::SetUiIcons(bool isWhite) {
    QIcon icon;
    icon = RecolorIcon(ui->bootInstallPkgAct->icon(), isWhite);
    ui->bootInstallPkgAct->setIcon(icon);
    icon = RecolorIcon(ui->exitAct->icon(), isWhite);
    ui->exitAct->setIcon(icon);
    icon = RecolorIcon(ui->setlistModeListAct->icon(), isWhite);
    ui->setlistModeListAct->setIcon(icon);
    icon = RecolorIcon(ui->setlistModeGridAct->icon(), isWhite);
    ui->setlistModeGridAct->setIcon(icon);
    icon = RecolorIcon(ui->gameInstallPathAct->icon(), isWhite);
    ui->gameInstallPathAct->setIcon(icon);
    icon = RecolorIcon(ui->menuThemes->icon(), isWhite);
    ui->menuThemes->setIcon(icon);
    icon = RecolorIcon(ui->menuGame_List_Icons->icon(), isWhite);
    ui->menuGame_List_Icons->setIcon(icon);
    icon = RecolorIcon(ui->playButton->icon(), isWhite);
    ui->playButton->setIcon(icon);
    icon = RecolorIcon(ui->pauseButton->icon(), isWhite);
    ui->pauseButton->setIcon(icon);
    icon = RecolorIcon(ui->stopButton->icon(), isWhite);
    ui->stopButton->setIcon(icon);
    icon = RecolorIcon(ui->settingsButton->icon(), isWhite);
    ui->settingsButton->setIcon(icon);
    icon = RecolorIcon(ui->controllerButton->icon(), isWhite);
    ui->controllerButton->setIcon(icon);
    icon = RecolorIcon(ui->refreshGameListAct->icon(), isWhite);
    ui->refreshGameListAct->setIcon(icon);
    icon = RecolorIcon(ui->menuGame_List_Mode->icon(), isWhite);
    ui->menuGame_List_Mode->setIcon(icon);
    icon = RecolorIcon(ui->pkgViewerAct->icon(), isWhite);
    ui->pkgViewerAct->setIcon(icon);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    emit WindowResized(event);
    QMainWindow::resizeEvent(event);
}

void MainWindow::HandleResize(QResizeEvent* event) {
    if (isTableList) {
        m_game_list_frame->RefreshListBackgroundImage();
    } else {
        m_game_grid_frame->windowWidth = this->width();
        m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
        m_game_grid_frame->RefreshGridBackgroundImage();
    }
}