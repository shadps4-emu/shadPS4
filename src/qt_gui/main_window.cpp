// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStatusBar>
#include <QtConcurrent>

#include "about_dialog.h"
#include "common/io_file.h"
#include "common/version.h"
#include "core/file_format/pkg.h"
#include "core/loader.h"
#include "game_install_dialog.h"
#include "main_window.h"
#include "settings_dialog.h"
#include "video_core/renderer_vulkan/vk_instance.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
}

MainWindow::~MainWindow() {
    SaveWindowState();
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::save(config_dir / "config.toml");
}

bool MainWindow::Init() {
    auto start = std::chrono::steady_clock::now();
    // setup ui
    AddUiWidgets();
    CreateActions();
    CreateRecentGameActions();
    ConfigureGuiFromSettings();
    CreateDockWindows();
    CreateConnects();
    SetLastUsedTheme();
    SetLastIconSizeBullet();
    GetPhysicalDevices();
    // show ui
    setMinimumSize(350, minimumSizeHint().height());
    setWindowTitle(QString::fromStdString("shadPS4 v" + std::string(Common::VERSION)));
    this->show();
    // load game list
    LoadGameLists();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    statusBar.reset(new QStatusBar);
    this->setStatusBar(statusBar.data());
    // Update status bar
    int numGames = m_game_info->m_games.size();
    QString statusMessage =
        "Games: " + QString::number(numGames) + " (" + QString::number(duration.count()) + "ms)";
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
    m_theme_act_group->addAction(ui->setThemeDark);
    m_theme_act_group->addAction(ui->setThemeLight);
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
    // place holder widget is needed for good health they say :)
    QWidget* phCentralWidget = new QWidget(this);
    setCentralWidget(phCentralWidget);

    m_dock_widget.reset(new QDockWidget("Game List", this));
    m_game_list_frame.reset(new GameListFrame(m_game_info, this));
    m_game_list_frame->setObjectName("gamelist");
    m_game_grid_frame.reset(new GameGridFrame(m_game_info, this));
    m_game_grid_frame->setObjectName("gamegridlist");
    m_elf_viewer.reset(new ElfViewer(this));
    m_elf_viewer->setObjectName("elflist");

    int table_mode = Config::getTableMode();
    int slider_pos = 0;
    if (table_mode == 0) { // List
        m_game_grid_frame->hide();
        m_elf_viewer->hide();
        m_game_list_frame->show();
        m_dock_widget->setWidget(m_game_list_frame.data());
        slider_pos = Config::getSliderPositon();
        ui->sizeSlider->setSliderPosition(slider_pos); // set slider pos at start;
        isTableList = true;
    } else if (table_mode == 1) { // Grid
        m_game_list_frame->hide();
        m_elf_viewer->hide();
        m_game_grid_frame->show();
        m_dock_widget->setWidget(m_game_grid_frame.data());
        slider_pos = Config::getSliderPositonGrid();
        ui->sizeSlider->setSliderPosition(slider_pos); // set slider pos at start;
        isTableList = false;
    } else {
        m_game_list_frame->hide();
        m_game_grid_frame->hide();
        m_elf_viewer->show();
        m_dock_widget->setWidget(m_elf_viewer.data());
        isTableList = false;
    }

    m_dock_widget->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_dock_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_dock_widget->resize(this->width(), this->height());
    addDockWidget(Qt::LeftDockWidgetArea, m_dock_widget.data());
    this->setDockNestingEnabled(true);

    // handle resize like this for now, we deal with it when we add more docks
    connect(this, &MainWindow::WindowResized, this, [&]() {
        this->resizeDocks({m_dock_widget.data()}, {this->width()}, Qt::Orientation::Horizontal);
    });
}

void MainWindow::LoadGameLists() {
    // Get game info from game folders.
    m_game_info->GetGameInfo(this);
    if (isTableList) {
        m_game_list_frame->PopulateGameList();
    } else {
        m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    }
}

void MainWindow::GetPhysicalDevices() {
    Vulkan::Instance instance(false, false);
    auto physical_devices = instance.GetPhysicalDevices();
    for (const vk::PhysicalDevice physical_device : physical_devices) {
        auto prop = physical_device.getProperties();
        QString name = QString::fromUtf8(prop.deviceName, -1);
        if (prop.apiVersion < Vulkan::TargetVulkanApiVersion) {
            name += " * Unsupported Vulkan Version";
        }
        m_physical_devices.push_back(name);
    }
}

void MainWindow::CreateConnects() {
    connect(this, &MainWindow::WindowResized, this, &MainWindow::HandleResize);
    connect(ui->mw_searchbar, &QLineEdit::textChanged, this, &MainWindow::SearchGameTable);
    connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);
    connect(ui->refreshGameListAct, &QAction::triggered, this, &MainWindow::RefreshGameTable);
    connect(this, &MainWindow::ExtractionFinished, this, &MainWindow::RefreshGameTable);

    connect(ui->sizeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size =
                36 + value; // 36 is the minimum icon size to use due to text disappearing.
            m_game_list_frame->ResizeIcons(36 + value);
            Config::setIconSize(36 + value);
            Config::setSliderPositon(value);
        } else {
            m_game_grid_frame->icon_size = 69 + value;
            m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
            Config::setIconSizeGrid(69 + value);
            Config::setSliderPositonGrid(value);
        }
    });

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::StartGame);
    connect(m_game_grid_frame.get(), &QTableWidget::cellDoubleClicked, this,
            &MainWindow::StartGame);
    connect(m_game_list_frame.get(), &QTableWidget::cellDoubleClicked, this,
            &MainWindow::StartGame);

    connect(ui->configureAct, &QAction::triggered, this, [this]() {
        auto settingsDialog = new SettingsDialog(m_physical_devices, this);
        settingsDialog->exec();
    });

    connect(ui->settingsButton, &QPushButton::clicked, this, [this]() {
        auto settingsDialog = new SettingsDialog(m_physical_devices, this);
        settingsDialog->exec();
    });

    connect(ui->aboutAct, &QAction::triggered, this, [this]() {
        auto aboutDialog = new AboutDialog(this);
        aboutDialog->exec();
    });

    connect(ui->setIconSizeTinyAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size =
                36; // 36 is the minimum icon size to use due to text disappearing.
            ui->sizeSlider->setValue(0); // icone_size - 36
            Config::setIconSize(36);
            Config::setSliderPositon(0);
        } else {
            ui->sizeSlider->setValue(0); // icone_size - 36
            Config::setIconSizeGrid(69);
            Config::setSliderPositonGrid(0);
        }
    });

    connect(ui->setIconSizeSmallAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 64;
            ui->sizeSlider->setValue(28);
            Config::setIconSize(64);
            Config::setSliderPositon(28);
        } else {
            ui->sizeSlider->setValue(28);
            Config::setIconSizeGrid(97);
            Config::setSliderPositonGrid(28);
        }
    });

    connect(ui->setIconSizeMediumAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 128;
            ui->sizeSlider->setValue(92);
            Config::setIconSize(128);
            Config::setSliderPositon(92);
        } else {
            ui->sizeSlider->setValue(92);
            Config::setIconSizeGrid(160);
            Config::setSliderPositonGrid(91);
        }
    });

    connect(ui->setIconSizeLargeAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 256;
            ui->sizeSlider->setValue(220);
            Config::setIconSize(256);
            Config::setSliderPositon(220);
        } else {
            ui->sizeSlider->setValue(220);
            Config::setIconSizeGrid(256);
            Config::setSliderPositonGrid(220);
        }
    });
    // List
    connect(ui->setlistModeListAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        m_dock_widget->setWidget(m_game_list_frame.data());
        m_game_grid_frame->hide();
        m_elf_viewer->hide();
        m_game_list_frame->show();
        if (m_game_list_frame->item(0, 0) == nullptr) {
            m_game_list_frame->clearContents();
            m_game_list_frame->PopulateGameList();
        }
        isTableList = true;
        Config::setTableMode(0);
        int slider_pos = Config::getSliderPositon();
        ui->sizeSlider->setEnabled(true);
        ui->sizeSlider->setSliderPosition(slider_pos);
    });
    // Grid
    connect(ui->setlistModeGridAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        m_dock_widget->setWidget(m_game_grid_frame.data());
        m_game_grid_frame->show();
        m_game_list_frame->hide();
        m_elf_viewer->hide();
        if (m_game_grid_frame->item(0, 0) == nullptr) {
            m_game_grid_frame->clearContents();
            m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
        }
        isTableList = false;
        Config::setTableMode(1);
        int slider_pos_grid = Config::getSliderPositonGrid();
        ui->sizeSlider->setEnabled(true);
        ui->sizeSlider->setSliderPosition(slider_pos_grid);
    });
    // Elf
    connect(ui->setlistElfAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        m_dock_widget->setWidget(m_elf_viewer.data());
        m_game_grid_frame->hide();
        m_game_list_frame->hide();
        m_elf_viewer->show();
        isTableList = false;
        ui->sizeSlider->setDisabled(true);
        Config::setTableMode(2);
    });

    // Dump game list.
    connect(ui->dumpGameListAct, &QAction::triggered, this, [&] {
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
    connect(ui->bootInstallPkgAct, &QAction::triggered, this, &MainWindow::InstallPkg);
    connect(ui->bootGameAct, &QAction::triggered, this, &MainWindow::BootGame);
    connect(ui->gameInstallPathAct, &QAction::triggered, this, &MainWindow::InstallDirectory);

    // elf viewer
    connect(ui->addElfFolderAct, &QAction::triggered, m_elf_viewer.data(),
            &ElfViewer::OpenElfFolder);

    // Package Viewer.
    connect(ui->pkgViewerAct, &QAction::triggered, this, [this]() {
        PKGViewer* pkgViewer = new PKGViewer(
            m_game_info, this, [this](std::filesystem::path file, int pkgNum, int nPkg) {
                this->InstallDragDropPkg(file, pkgNum, nPkg);
            });
        pkgViewer->show();
    });

    // Themes
    connect(ui->setThemeDark, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Dark, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Dark));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeLight, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Light, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Light));
        if (!isIconBlack) {
            SetUiIcons(true);
            isIconBlack = true;
        }
    });
    connect(ui->setThemeGreen, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Green, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Green));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeBlue, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Blue, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Blue));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeViolet, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Violet, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Violet));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
}

void MainWindow::StartGame() {
    QString gamePath = "";
    int table_mode = Config::getTableMode();
    if (table_mode == 0) {
        if (m_game_list_frame->currentItem()) {
            int itemID = m_game_list_frame->currentItem()->row();
            gamePath = QString::fromStdString(m_game_info->m_games[itemID].path + "/eboot.bin");
        }
    } else if (table_mode == 1) {
        if (m_game_grid_frame->cellClicked) {
            int itemID = (m_game_grid_frame->crtRow * m_game_grid_frame->columnCnt) +
                         m_game_grid_frame->crtColumn;
            gamePath = QString::fromStdString(m_game_info->m_games[itemID].path + "/eboot.bin");
        }
    } else {
        if (m_elf_viewer->currentItem()) {
            int itemID = m_elf_viewer->currentItem()->row();
            gamePath = QString::fromStdString(m_elf_viewer->m_elf_list[itemID].toStdString());
        }
    }
    if (gamePath != "") {
        AddRecentFiles(gamePath);
        Core::Emulator emulator;
        emulator.Run(gamePath.toUtf8().constData());
    }
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
    // m_game_info->m_games.clear();
    m_game_info->GetGameInfo(this);
    m_game_list_frame->clearContents();
    m_game_list_frame->PopulateGameList();
    m_game_grid_frame->clearContents();
    m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    statusBar->clearMessage();
    int numGames = m_game_info->m_games.size();
    QString statusMessage = "Games: " + QString::number(numGames);
    statusBar->showMessage(statusMessage);
}

void MainWindow::ConfigureGuiFromSettings() {
    setGeometry(Config::getMainWindowGeometryX(), Config::getMainWindowGeometryY(),
                Config::getMainWindowGeometryW(), Config::getMainWindowGeometryH());

    ui->showGameListAct->setChecked(true);
    if (isTableList) {
        ui->setlistModeListAct->setChecked(true);
    } else {
        ui->setlistModeGridAct->setChecked(true);
    }
}

void MainWindow::SaveWindowState() const {
    Config::setMainWindowWidth(this->width());
    Config::setMainWindowHeight(this->height());
    Config::setMainWindowGeometry(this->geometry().x(), this->geometry().y(),
                                  this->geometry().width(), this->geometry().height());
}

void MainWindow::InstallPkg() {
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("PKG File (*.PKG)"));
    if (dialog.exec()) {
        QStringList fileNames = dialog.selectedFiles();
        int nPkg = fileNames.size();
        int pkgNum = 0;
        for (const QString& file : fileNames) {
            ++pkgNum;
            std::filesystem::path path(file.toStdString());
#ifdef _WIN64
            path = std::filesystem::path(file.toStdWString());
#endif
            MainWindow::InstallDragDropPkg(path, pkgNum, nPkg);
        }
    }
}

void MainWindow::BootGame() {
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("ELF files (*.bin *.elf *.oelf)"));
    if (dialog.exec()) {
        QStringList fileNames = dialog.selectedFiles();
        int nFiles = fileNames.size();

        if (nFiles > 1) {
            QMessageBox::critical(nullptr, "Game Boot", QString("Only one file can be selected!"));
        } else {
            std::filesystem::path path(fileNames[0].toStdString());
#ifdef _WIN64
            path = std::filesystem::path(fileNames[0].toStdWString());
#endif
            Core::Emulator emulator;
            emulator.Run(path);
        }
    }
}

void MainWindow::InstallDragDropPkg(std::filesystem::path file, int pkgNum, int nPkg) {
    if (Loader::DetectFileType(file) == Loader::FileTypes::Pkg) {
        pkg = PKG();
        pkg.Open(file);
        std::string failreason;
        const auto extract_path =
            std::filesystem::path(Config::getGameInstallDir()) / pkg.GetTitleID();
        QString pkgType = QString::fromStdString(pkg.GetPkgFlags());
        QDir game_dir(QString::fromStdString(extract_path.string()));
        if (game_dir.exists()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("PKG Extraction");
            if (pkgType.contains("PATCH")) {
                psf.open("", pkg.sfo);
                QString pkg_app_version = QString::fromStdString(psf.GetString("APP_VER"));
                psf.open(extract_path.string() + "/sce_sys/param.sfo", {});
                QString game_app_version = QString::fromStdString(psf.GetString("APP_VER"));
                double appD = game_app_version.toDouble();
                double pkgD = pkg_app_version.toDouble();
                if (pkgD == appD) {
                    msgBox.setText(
                        QString("Patch detected!\nPKG and Game versions match!: %1\nWould you like "
                                "to overwrite?")
                            .arg(pkg_app_version));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else if (pkgD < appD) {
                    msgBox.setText(QString("Patch detected!\nPKG Version %1 is older "
                                           "than installed version!: %2\nWould you like "
                                           "to overwrite?")
                                       .arg(pkg_app_version, game_app_version));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else {
                    msgBox.setText(QString("Patch detected!\nGame is installed: %1\nWould you like "
                                           "to install Patch: %2?")
                                       .arg(game_app_version, pkg_app_version));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                }
                int result = msgBox.exec();
                if (result == QMessageBox::Yes) {
                    // Do nothing.
                } else {
                    return;
                }
            } else {
                msgBox.setText(QString("Game already installed\n%1\nWould you like to overwrite?")
                                   .arg(QString::fromStdString(extract_path.string())));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                int result = msgBox.exec();
                if (result == QMessageBox::Yes) {
                    // Do nothing.
                } else {
                    return;
                }
            }
        } else {
            // Do nothing;
            if (pkgType.contains("PATCH")) {
                QMessageBox::information(this, "PKG Extraction",
                                         "PKG is a patch, please install the game first!");
                return;
            }
            // what else?
        }

        if (!pkg.Extract(file, extract_path, failreason)) {
            QMessageBox::critical(this, "PKG ERROR", QString::fromStdString(failreason));
        } else {
            int nfiles = pkg.GetNumberOfFiles();

            QVector<int> indices;
            for (int i = 0; i < nfiles; i++) {
                indices.append(i);
            }

            QProgressDialog dialog;
            dialog.setWindowTitle("PKG Extraction");
            dialog.setWindowModality(Qt::WindowModal);
            QString extractmsg = QString("Extracting PKG %1/%2").arg(pkgNum).arg(nPkg);
            dialog.setLabelText(extractmsg);
            dialog.setAutoClose(true);
            dialog.setRange(0, nfiles);

            QFutureWatcher<void> futureWatcher;
            connect(&futureWatcher, &QFutureWatcher<void>::finished, this, [=, this]() {
                if (pkgNum == nPkg) {
                    QString path = QString::fromStdString(Config::getGameInstallDir());
                    QMessageBox extractMsgBox(this);
                    extractMsgBox.setWindowTitle("Extraction Finished");
                    extractMsgBox.setText(QString("Game successfully installed at %1").arg(path));
                    extractMsgBox.addButton(QMessageBox::Ok);
                    extractMsgBox.setDefaultButton(QMessageBox::Ok);
                    connect(&extractMsgBox, &QMessageBox::buttonClicked, this,
                            [&](QAbstractButton* button) {
                                if (extractMsgBox.button(QMessageBox::Ok) == button) {
                                    extractMsgBox.close();
                                    emit ExtractionFinished();
                                }
                            });
                    extractMsgBox.exec();
                }
            });
            connect(&dialog, &QProgressDialog::canceled, [&]() { futureWatcher.cancel(); });
            connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &dialog,
                    &QProgressDialog::setValue);
            futureWatcher.setFuture(
                QtConcurrent::map(indices, [&](int index) { pkg.ExtractFiles(index); }));
            dialog.exec();
        }
    } else {
        QMessageBox::critical(this, "PKG ERROR", "File doesn't appear to be a valid PKG file");
    }
}

void MainWindow::InstallDirectory() {
    GameInstallDialog dlg;
    dlg.exec();
    RefreshGameTable();
}

void MainWindow::SetLastUsedTheme() {
    Theme lastTheme = static_cast<Theme>(Config::getMainWindowTheme());
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
    int lastSize = Config::getIconSize();
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
    QPixmap pixmap(icon.pixmap(icon.actualSize(QSize(120, 120))));
    QColor clr(isWhite ? Qt::white : Qt::black);
    QBitmap mask = pixmap.createMaskFromColor(clr, Qt::MaskOutColor);
    pixmap.fill(QColor(isWhite ? Qt::black : Qt::white));
    pixmap.setMask(mask);
    return QIcon(pixmap);
}

void MainWindow::SetUiIcons(bool isWhite) {
    ui->bootInstallPkgAct->setIcon(RecolorIcon(ui->bootInstallPkgAct->icon(), isWhite));
    ui->exitAct->setIcon(RecolorIcon(ui->exitAct->icon(), isWhite));
    ui->setlistModeListAct->setIcon(RecolorIcon(ui->setlistModeListAct->icon(), isWhite));
    ui->setlistModeGridAct->setIcon(RecolorIcon(ui->setlistModeGridAct->icon(), isWhite));
    ui->gameInstallPathAct->setIcon(RecolorIcon(ui->gameInstallPathAct->icon(), isWhite));
    ui->menuThemes->setIcon(RecolorIcon(ui->menuThemes->icon(), isWhite));
    ui->menuGame_List_Icons->setIcon(RecolorIcon(ui->menuGame_List_Icons->icon(), isWhite));
    ui->playButton->setIcon(RecolorIcon(ui->playButton->icon(), isWhite));
    ui->pauseButton->setIcon(RecolorIcon(ui->pauseButton->icon(), isWhite));
    ui->stopButton->setIcon(RecolorIcon(ui->stopButton->icon(), isWhite));
    ui->settingsButton->setIcon(RecolorIcon(ui->settingsButton->icon(), isWhite));
    ui->controllerButton->setIcon(RecolorIcon(ui->controllerButton->icon(), isWhite));
    ui->refreshGameListAct->setIcon(RecolorIcon(ui->refreshGameListAct->icon(), isWhite));
    ui->menuGame_List_Mode->setIcon(RecolorIcon(ui->menuGame_List_Mode->icon(), isWhite));
    ui->pkgViewerAct->setIcon(RecolorIcon(ui->pkgViewerAct->icon(), isWhite));
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

void MainWindow::AddRecentFiles(QString filePath) {
    std::vector<std::string> vec = Config::getRecentFiles();
    if (!vec.empty()) {
        if (filePath.toStdString() == vec.at(0)) {
            return;
        }
        auto it = std::find(vec.begin(), vec.end(), filePath.toStdString());
        if (it != vec.end()) {
            vec.erase(it);
        }
    }
    vec.insert(vec.begin(), filePath.toStdString());
    if (vec.size() > 6) {
        vec.pop_back();
    }
    Config::setRecentFiles(vec);
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::save(config_dir / "config.toml");
    CreateRecentGameActions(); // Refresh the QActions.
}

void MainWindow::CreateRecentGameActions() {
    m_recent_files_group = new QActionGroup(this);
    ui->menuRecent->clear();
    std::vector<std::string> vec = Config::getRecentFiles();
    for (int i = 0; i < vec.size(); i++) {
        QAction* recentFileAct = new QAction(this);
        recentFileAct->setText(QString::fromStdString(vec.at(i)));
        ui->menuRecent->addAction(recentFileAct);
        m_recent_files_group->addAction(recentFileAct);
    }

    connect(m_recent_files_group, &QActionGroup::triggered, this, [this](QAction* action) {
        QString gamePath = action->text();
        AddRecentFiles(gamePath); // Update the list.
        Core::Emulator emulator;
        emulator.Run(gamePath.toUtf8().constData());
    });
}
