// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDockWidget>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QProgressDialog>

#include "about_dialog.h"
#include "cheats_patches.h"
#ifdef ENABLE_UPDATER
#include "check_update.h"
#endif
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/scm_rev.h"
#include "common/string_util.h"
#include "common/version.h"
#include "control_settings.h"
#include "core/file_format/pkg.h"
#include "core/loader.h"
#include "game_install_dialog.h"
#include "install_dir_select.h"
#include "main_window.h"
#include "settings_dialog.h"

#include "kbm_config_dialog.h"

#include "video_core/renderer_vulkan/vk_instance.h"
#ifdef ENABLE_DISCORD_RPC
#include "common/discord_rpc_handler.h"
#endif

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    installEventFilter(this);
    setAttribute(Qt::WA_DeleteOnClose);
}

MainWindow::~MainWindow() {
    SaveWindowState();
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::saveMainWindow(config_dir / "config.toml");
}

bool MainWindow::Init() {
    auto start = std::chrono::steady_clock::now();
    // setup ui
    AddUiWidgets();
    CreateActions();
    CreateRecentGameActions();
    ConfigureGuiFromSettings();
    LoadTranslation();
    CreateDockWindows();
    CreateConnects();
    SetLastUsedTheme();
    SetLastIconSizeBullet();
    GetPhysicalDevices();
    // show ui
    setMinimumSize(720, 405);
    std::string window_title = "";
    if (Common::isRelease) {
        window_title = fmt::format("shadPS4 v{}", Common::VERSION);
    } else {
        std::string remote_url(Common::g_scm_remote_url);
        std::string remote_host;
        try {
            remote_host = remote_url.substr(19, remote_url.rfind('/') - 19);
        } catch (...) {
            remote_host = "unknown";
        }
        if (remote_host == "shadps4-emu" || remote_url.length() == 0) {
            window_title = fmt::format("shadPS4 v{} {} {}", Common::VERSION, Common::g_scm_branch,
                                       Common::g_scm_desc);
        } else {
            window_title = fmt::format("shadPS4 v{} {}/{} {}", Common::VERSION, remote_host,
                                       Common::g_scm_branch, Common::g_scm_desc);
        }
    }
    setWindowTitle(QString::fromStdString(window_title));
    this->show();
    // load game list
    LoadGameLists();
#ifdef ENABLE_UPDATER
    // Check for update
    CheckUpdateMain(true);
#endif

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    statusBar.reset(new QStatusBar);
    this->setStatusBar(statusBar.data());
    // Update status bar
    int numGames = m_game_info->m_games.size();
    QString statusMessage = tr("Games: ") + QString::number(numGames) + " (" +
                            QString::number(duration.count()) + "ms)";
    statusBar->showMessage(statusMessage);

#ifdef ENABLE_DISCORD_RPC
    if (Config::getEnableDiscordRPC()) {
        auto* rpc = Common::Singleton<DiscordRPCHandler::RPC>::Instance();
        rpc->init();
        rpc->setStatusIdling();
    }
#endif

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
    m_list_mode_act_group->addAction(ui->setlistElfAct);

    // create action group for themes
    m_theme_act_group = new QActionGroup(this);
    m_theme_act_group->addAction(ui->setThemeDark);
    m_theme_act_group->addAction(ui->setThemeLight);
    m_theme_act_group->addAction(ui->setThemeGreen);
    m_theme_act_group->addAction(ui->setThemeBlue);
    m_theme_act_group->addAction(ui->setThemeViolet);
    m_theme_act_group->addAction(ui->setThemeGruvbox);
    m_theme_act_group->addAction(ui->setThemeTokyoNight);
    m_theme_act_group->addAction(ui->setThemeOled);
}

void MainWindow::AddUiWidgets() {
    // add toolbar widgets
    QApplication::setStyle("Fusion");
    ui->toolBar->setObjectName("mw_toolbar");
    ui->toolBar->addWidget(ui->playButton);
    ui->toolBar->addWidget(ui->pauseButton);
    ui->toolBar->addWidget(ui->stopButton);
    ui->toolBar->addWidget(ui->refreshButton);
    ui->toolBar->addWidget(ui->settingsButton);
    ui->toolBar->addWidget(ui->controllerButton);
    ui->toolBar->addWidget(ui->keyboardButton);
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

    m_dock_widget.reset(new QDockWidget(tr("Game List"), this));
    m_game_list_frame.reset(new GameListFrame(m_game_info, m_compat_info, this));
    m_game_list_frame->setObjectName("gamelist");
    m_game_grid_frame.reset(new GameGridFrame(m_game_info, m_compat_info, this));
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
        slider_pos = Config::getSliderPosition();
        ui->sizeSlider->setSliderPosition(slider_pos); // set slider pos at start;
        isTableList = true;
    } else if (table_mode == 1) { // Grid
        m_game_list_frame->hide();
        m_elf_viewer->hide();
        m_game_grid_frame->show();
        m_dock_widget->setWidget(m_game_grid_frame.data());
        slider_pos = Config::getSliderPositionGrid();
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
    // Load compatibility database
    if (Config::getCompatibilityEnabled())
        m_compat_info->LoadCompatibilityFile();

    // Update compatibility database
    if (Config::getCheckCompatibilityOnStartup())
        m_compat_info->UpdateCompatibilityDatabase(this);

    // Get game info from game folders.
    m_game_info->GetGameInfo(this);
    if (isTableList) {
        m_game_list_frame->PopulateGameList();
    } else {
        m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    }
}

#ifdef ENABLE_UPDATER
void MainWindow::CheckUpdateMain(bool checkSave) {
    if (checkSave) {
        if (!Config::autoUpdate()) {
            return;
        }
    }
    auto checkUpdate = new CheckUpdate(false);
    checkUpdate->exec();
}
#endif

void MainWindow::GetPhysicalDevices() {
    Vulkan::Instance instance(false, false);
    auto physical_devices = instance.GetPhysicalDevices();
    for (const vk::PhysicalDevice physical_device : physical_devices) {
        auto prop = physical_device.getProperties();
        QString name = QString::fromUtf8(prop.deviceName, -1);
        if (prop.apiVersion < Vulkan::TargetVulkanApiVersion) {
            name += tr(" * Unsupported Vulkan Version");
        }
        m_physical_devices.push_back(name);
    }
}

void MainWindow::CreateConnects() {
    connect(this, &MainWindow::WindowResized, this, &MainWindow::HandleResize);
    connect(ui->mw_searchbar, &QLineEdit::textChanged, this, &MainWindow::SearchGameTable);
    connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);
    connect(ui->refreshGameListAct, &QAction::triggered, this, &MainWindow::RefreshGameTable);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::RefreshGameTable);
    connect(ui->showGameListAct, &QAction::triggered, this, &MainWindow::ShowGameList);
    connect(this, &MainWindow::ExtractionFinished, this, &MainWindow::RefreshGameTable);

    connect(ui->sizeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (isTableList) {
            m_game_list_frame->icon_size =
                36 + value; // 36 is the minimum icon size to use due to text disappearing.
            m_game_list_frame->ResizeIcons(36 + value);
            Config::setIconSize(36 + value);
            Config::setSliderPosition(value);
        } else {
            m_game_grid_frame->icon_size = 69 + value;
            m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
            Config::setIconSizeGrid(69 + value);
            Config::setSliderPositionGrid(value);
        }
    });

    connect(ui->shadFolderAct, &QAction::triggered, this, [this]() {
        QString userPath;
        Common::FS::PathToQString(userPath, Common::FS::GetUserPath(Common::FS::PathType::UserDir));
        QDesktopServices::openUrl(QUrl::fromLocalFile(userPath));
    });

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::StartGame);
    connect(m_game_grid_frame.get(), &QTableWidget::cellDoubleClicked, this,
            &MainWindow::StartGame);
    connect(m_game_list_frame.get(), &QTableWidget::cellDoubleClicked, this,
            &MainWindow::StartGame);

    connect(ui->configureAct, &QAction::triggered, this, [this]() {
        auto settingsDialog = new SettingsDialog(m_physical_devices, m_compat_info, this);

        connect(settingsDialog, &SettingsDialog::LanguageChanged, this,
                &MainWindow::OnLanguageChanged);

        connect(settingsDialog, &SettingsDialog::CompatibilityChanged, this,
                &MainWindow::RefreshGameTable);

        connect(settingsDialog, &SettingsDialog::accepted, this, &MainWindow::RefreshGameTable);
        connect(settingsDialog, &SettingsDialog::rejected, this, &MainWindow::RefreshGameTable);
        connect(settingsDialog, &SettingsDialog::close, this, &MainWindow::RefreshGameTable);

        connect(settingsDialog, &SettingsDialog::BackgroundOpacityChanged, this,
                [this](int opacity) {
                    Config::setBackgroundImageOpacity(opacity);
                    if (m_game_list_frame) {
                        QTableWidgetItem* current = m_game_list_frame->GetCurrentItem();
                        if (current) {
                            m_game_list_frame->SetListBackgroundImage(current);
                        }
                    }
                    if (m_game_grid_frame) {
                        if (m_game_grid_frame->IsValidCellSelected()) {
                            m_game_grid_frame->SetGridBackgroundImage(m_game_grid_frame->crtRow,
                                                                      m_game_grid_frame->crtColumn);
                        }
                    }
                });

        settingsDialog->exec();
    });

    connect(ui->settingsButton, &QPushButton::clicked, this, [this]() {
        auto settingsDialog = new SettingsDialog(m_physical_devices, m_compat_info, this);

        connect(settingsDialog, &SettingsDialog::LanguageChanged, this,
                &MainWindow::OnLanguageChanged);

        connect(settingsDialog, &SettingsDialog::CompatibilityChanged, this,
                &MainWindow::RefreshGameTable);

        connect(settingsDialog, &SettingsDialog::accepted, this, &MainWindow::RefreshGameTable);
        connect(settingsDialog, &SettingsDialog::rejected, this, &MainWindow::RefreshGameTable);
        connect(settingsDialog, &SettingsDialog::close, this, &MainWindow::RefreshGameTable);

        connect(settingsDialog, &SettingsDialog::BackgroundOpacityChanged, this,
                [this](int opacity) {
                    Config::setBackgroundImageOpacity(opacity);
                    if (m_game_list_frame) {
                        QTableWidgetItem* current = m_game_list_frame->GetCurrentItem();
                        if (current) {
                            m_game_list_frame->SetListBackgroundImage(current);
                        }
                    }
                    if (m_game_grid_frame) {
                        if (m_game_grid_frame->IsValidCellSelected()) {
                            m_game_grid_frame->SetGridBackgroundImage(m_game_grid_frame->crtRow,
                                                                      m_game_grid_frame->crtColumn);
                        }
                    }
                });

        settingsDialog->exec();
    });

    // this is the editor for kbm keybinds
    connect(ui->controllerButton, &QPushButton::clicked, this, [this]() {
        auto configWindow = new ControlSettings(m_game_info, this);
        configWindow->exec();
    });

    connect(ui->keyboardButton, &QPushButton::clicked, this, [this]() {
        auto kbmWindow = new EditorDialog(this);
        kbmWindow->exec();
    });

#ifdef ENABLE_UPDATER
    connect(ui->updaterAct, &QAction::triggered, this, [this]() {
        auto checkUpdate = new CheckUpdate(true);
        checkUpdate->exec();
    });
#endif

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
            Config::setSliderPosition(0);
        } else {
            ui->sizeSlider->setValue(0); // icone_size - 36
            Config::setIconSizeGrid(69);
            Config::setSliderPositionGrid(0);
        }
    });

    connect(ui->setIconSizeSmallAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 64;
            ui->sizeSlider->setValue(28);
            Config::setIconSize(64);
            Config::setSliderPosition(28);
        } else {
            ui->sizeSlider->setValue(28);
            Config::setIconSizeGrid(97);
            Config::setSliderPositionGrid(28);
        }
    });

    connect(ui->setIconSizeMediumAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 128;
            ui->sizeSlider->setValue(92);
            Config::setIconSize(128);
            Config::setSliderPosition(92);
        } else {
            ui->sizeSlider->setValue(92);
            Config::setIconSizeGrid(160);
            Config::setSliderPositionGrid(91);
        }
    });

    connect(ui->setIconSizeLargeAct, &QAction::triggered, this, [this]() {
        if (isTableList) {
            m_game_list_frame->icon_size = 256;
            ui->sizeSlider->setValue(220);
            Config::setIconSize(256);
            Config::setSliderPosition(220);
        } else {
            ui->sizeSlider->setValue(220);
            Config::setIconSizeGrid(256);
            Config::setSliderPositionGrid(220);
        }
    });
    // List
    connect(ui->setlistModeListAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        BackgroundMusicPlayer::getInstance().stopMusic();
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
        int slider_pos = Config::getSliderPosition();
        ui->sizeSlider->setEnabled(true);
        ui->sizeSlider->setSliderPosition(slider_pos);
        ui->mw_searchbar->setText("");
    });
    // Grid
    connect(ui->setlistModeGridAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        BackgroundMusicPlayer::getInstance().stopMusic();
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
        int slider_pos_grid = Config::getSliderPositionGrid();
        ui->sizeSlider->setEnabled(true);
        ui->sizeSlider->setSliderPosition(slider_pos_grid);
        ui->mw_searchbar->setText("");
    });
    // Elf Viewer
    connect(ui->setlistElfAct, &QAction::triggered, m_dock_widget.data(), [this]() {
        BackgroundMusicPlayer::getInstance().stopMusic();
        m_dock_widget->setWidget(m_elf_viewer.data());
        m_game_grid_frame->hide();
        m_game_list_frame->hide();
        m_elf_viewer->show();
        isTableList = false;
        ui->sizeSlider->setDisabled(true);
        Config::setTableMode(2);
    });

    // Cheats/Patches Download.
    connect(ui->downloadCheatsPatchesAct, &QAction::triggered, this, [this]() {
        QDialog* panelDialog = new QDialog(this);
        QVBoxLayout* layout = new QVBoxLayout(panelDialog);
        QPushButton* downloadAllCheatsButton =
            new QPushButton(tr("Download Cheats For All Installed Games"), panelDialog);
        QPushButton* downloadAllPatchesButton =
            new QPushButton(tr("Download Patches For All Games"), panelDialog);

        layout->addWidget(downloadAllCheatsButton);
        layout->addWidget(downloadAllPatchesButton);

        panelDialog->setLayout(layout);

        connect(downloadAllCheatsButton, &QPushButton::clicked, this, [this, panelDialog]() {
            QEventLoop eventLoop;
            int pendingDownloads = 0;

            auto onDownloadFinished = [&]() {
                if (--pendingDownloads <= 0) {
                    eventLoop.quit();
                }
            };

            for (const GameInfo& game : m_game_info->m_games) {
                QString empty = "";
                QString gameSerial = QString::fromStdString(game.serial);
                QString gameVersion = QString::fromStdString(game.version);

                CheatsPatches* cheatsPatches =
                    new CheatsPatches(empty, empty, empty, empty, empty, nullptr);
                connect(cheatsPatches, &CheatsPatches::downloadFinished, onDownloadFinished);

                pendingDownloads += 3;

                cheatsPatches->downloadCheats("wolf2022", gameSerial, gameVersion, false);
                cheatsPatches->downloadCheats("GoldHEN", gameSerial, gameVersion, false);
                cheatsPatches->downloadCheats("shadPS4", gameSerial, gameVersion, false);
            }
            eventLoop.exec();

            QMessageBox::information(
                nullptr, tr("Download Complete"),
                tr("You have downloaded cheats for all the games you have installed."));

            panelDialog->accept();
        });
        connect(downloadAllPatchesButton, &QPushButton::clicked, [panelDialog]() {
            QEventLoop eventLoop;
            int pendingDownloads = 0;

            auto onDownloadFinished = [&]() {
                if (--pendingDownloads <= 0) {
                    eventLoop.quit();
                }
            };

            QString empty = "";
            CheatsPatches* cheatsPatches =
                new CheatsPatches(empty, empty, empty, empty, empty, nullptr);
            connect(cheatsPatches, &CheatsPatches::downloadFinished, onDownloadFinished);

            pendingDownloads += 2;

            cheatsPatches->downloadPatches("GoldHEN", false);
            cheatsPatches->downloadPatches("shadPS4", false);

            eventLoop.exec();
            QMessageBox::information(
                nullptr, tr("Download Complete"),
                QString(tr("Patches Downloaded Successfully!") + "\n" +
                        tr("All Patches available for all games have been downloaded.")));
            cheatsPatches->createFilesJson("GoldHEN");
            cheatsPatches->createFilesJson("shadPS4");
            panelDialog->accept();
        });
        panelDialog->exec();
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
            QString game_path;
            Common::FS::PathToQString(game_path, game.path);
            out << QString("%1 %2 %3     %4 %5\n")
                       .arg(QString::fromStdString(game.name), -50)
                       .arg(QString::fromStdString(game.serial), -10)
                       .arg(QString::fromStdString(game.fw), -4)
                       .arg(QString::fromStdString(game.version), -11)
                       .arg(game_path);
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
    connect(ui->setThemeGruvbox, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Gruvbox, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Gruvbox));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeTokyoNight, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::TokyoNight, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::TokyoNight));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
    connect(ui->setThemeOled, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Oled, ui->mw_searchbar);
        Config::setMainWindowTheme(static_cast<int>(Theme::Oled));
        if (isIconBlack) {
            SetUiIcons(false);
            isIconBlack = false;
        }
    });
}

void MainWindow::StartGame() {
    BackgroundMusicPlayer::getInstance().stopMusic();
    QString gamePath = "";
    int table_mode = Config::getTableMode();
    if (table_mode == 0) {
        if (m_game_list_frame->currentItem()) {
            int itemID = m_game_list_frame->currentItem()->row();
            Common::FS::PathToQString(gamePath, m_game_info->m_games[itemID].path / "eboot.bin");
        }
    } else if (table_mode == 1) {
        if (m_game_grid_frame->cellClicked) {
            int itemID = (m_game_grid_frame->crtRow * m_game_grid_frame->columnCnt) +
                         m_game_grid_frame->crtColumn;
            Common::FS::PathToQString(gamePath, m_game_info->m_games[itemID].path / "eboot.bin");
        }
    } else {
        if (m_elf_viewer->currentItem()) {
            int itemID = m_elf_viewer->currentItem()->row();
            gamePath = m_elf_viewer->m_elf_list[itemID];
        }
    }
    if (gamePath != "") {
        AddRecentFiles(gamePath);
        const auto path = Common::FS::PathFromQString(gamePath);
        if (!std::filesystem::exists(path)) {
            QMessageBox::critical(nullptr, tr("Run Game"), QString(tr("Eboot.bin file not found")));
            return;
        }
        StartEmulator(path);
    }
}

bool isTable;
void MainWindow::SearchGameTable(const QString& text) {
    if (isTableList) {
        if (isTable != true) {
            m_game_info->m_games = m_game_info->m_games_backup;
            m_game_list_frame->PopulateGameList();
            isTable = true;
        }
        for (int row = 0; row < m_game_list_frame->rowCount(); row++) {
            QString game_name = QString::fromStdString(m_game_info->m_games[row].name);
            bool match = (game_name.contains(text, Qt::CaseInsensitive)); // Check only in column 1
            m_game_list_frame->setRowHidden(row, !match);
        }
    } else {
        isTable = false;
        m_game_info->m_games = m_game_info->m_games_backup;
        m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);

        QVector<GameInfo> filteredGames;
        for (const auto& gameInfo : m_game_info->m_games) {
            QString game_name = QString::fromStdString(gameInfo.name);
            if (game_name.contains(text, Qt::CaseInsensitive)) {
                filteredGames.push_back(gameInfo);
            }
        }
        std::sort(filteredGames.begin(), filteredGames.end(), m_game_info->CompareStrings);
        m_game_info->m_games = filteredGames;
        m_game_grid_frame->PopulateGameGrid(filteredGames, true);
    }
}

void MainWindow::ShowGameList() {
    if (ui->showGameListAct->isChecked()) {
        RefreshGameTable();
    } else {
        m_game_grid_frame->clearContents();
        m_game_list_frame->clearContents();
    }
};

void MainWindow::RefreshGameTable() {
    // m_game_info->m_games.clear();
    m_game_info->GetGameInfo(this);
    m_game_list_frame->clearContents();
    m_game_list_frame->PopulateGameList();
    m_game_grid_frame->clearContents();
    m_game_grid_frame->PopulateGameGrid(m_game_info->m_games, false);
    statusBar->clearMessage();
    int numGames = m_game_info->m_games.size();
    QString statusMessage = tr("Games: ") + QString::number(numGames);
    statusBar->showMessage(statusMessage);
}

void MainWindow::ConfigureGuiFromSettings() {
    setGeometry(Config::getMainWindowGeometryX(), Config::getMainWindowGeometryY(),
                Config::getMainWindowGeometryW(), Config::getMainWindowGeometryH());

    ui->showGameListAct->setChecked(true);
    if (Config::getTableMode() == 0) {
        ui->setlistModeListAct->setChecked(true);
    } else if (Config::getTableMode() == 1) {
        ui->setlistModeGridAct->setChecked(true);
    } else if (Config::getTableMode() == 2) {
        ui->setlistElfAct->setChecked(true);
    }
    BackgroundMusicPlayer::getInstance().setVolume(Config::getBGMvolume());
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
    dialog.setNameFilter(tr("PKG File (*.PKG *.pkg)"));
    if (dialog.exec()) {
        QStringList fileNames = dialog.selectedFiles();
        int nPkg = fileNames.size();
        int pkgNum = 0;
        for (const QString& file : fileNames) {
            ++pkgNum;
            std::filesystem::path path = Common::FS::PathFromQString(file);
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
            QMessageBox::critical(nullptr, tr("Game Boot"),
                                  QString(tr("Only one file can be selected!")));
        } else {
            std::filesystem::path path = Common::FS::PathFromQString(fileNames[0]);
            if (!std::filesystem::exists(path)) {
                QMessageBox::critical(nullptr, tr("Run Game"),
                                      QString(tr("Eboot.bin file not found")));
                return;
            }
            StartEmulator(path);
        }
    }
}

void MainWindow::InstallDragDropPkg(std::filesystem::path file, int pkgNum, int nPkg) {
    if (Loader::DetectFileType(file) == Loader::FileTypes::Pkg) {
        std::string failreason;
        pkg = PKG();
        if (!pkg.Open(file, failreason)) {
            QMessageBox::critical(this, tr("PKG ERROR"), QString::fromStdString(failreason));
            return;
        }
        if (!psf.Open(pkg.sfo)) {
            QMessageBox::critical(this, tr("PKG ERROR"),
                                  "Could not read SFO. Check log for details");
            return;
        }
        auto category = psf.GetString("CATEGORY");

        if (!use_for_all_queued || pkgNum == 1) {
            InstallDirSelect ids;
            const auto selected = ids.exec();
            if (selected == QDialog::Rejected) {
                return;
            }

            last_install_dir = ids.getSelectedDirectory();
            delete_file_on_install = ids.deleteFileOnInstall();
            use_for_all_queued = ids.useForAllQueued();
        }
        std::filesystem::path game_install_dir = last_install_dir;

        QString pkgType = QString::fromStdString(pkg.GetPkgFlags());
        bool use_game_update = pkgType.contains("PATCH") && Config::getSeparateUpdateEnabled();

        // Default paths
        auto game_folder_path = game_install_dir / pkg.GetTitleID();
        auto game_update_path = use_game_update ? game_folder_path.parent_path() /
                                                      (std::string{pkg.GetTitleID()} + "-UPDATE")
                                                : game_folder_path;
        const int max_depth = 5;

        if (pkgType.contains("PATCH")) {
            // For patches, try to find the game recursively
            auto found_game = Common::FS::FindGameByID(game_install_dir,
                                                       std::string{pkg.GetTitleID()}, max_depth);
            if (found_game.has_value()) {
                game_folder_path = found_game.value().parent_path();
                game_update_path = use_game_update ? game_folder_path.parent_path() /
                                                         (std::string{pkg.GetTitleID()} + "-UPDATE")
                                                   : game_folder_path;
            }
        } else {
            // For base games, we check if the game is already installed
            auto found_game = Common::FS::FindGameByID(game_install_dir,
                                                       std::string{pkg.GetTitleID()}, max_depth);
            if (found_game.has_value()) {
                game_folder_path = found_game.value().parent_path();
            }
            // If the game is not found, we install it in the game install directory
            else {
                game_folder_path = game_install_dir / pkg.GetTitleID();
            }
            game_update_path = use_game_update ? game_folder_path.parent_path() /
                                                     (std::string{pkg.GetTitleID()} + "-UPDATE")
                                               : game_folder_path;
        }

        QString gameDirPath;
        Common::FS::PathToQString(gameDirPath, game_folder_path);
        QDir game_dir(gameDirPath);
        if (game_dir.exists()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("PKG Extraction"));

            std::string content_id;
            if (auto value = psf.GetString("CONTENT_ID"); value.has_value()) {
                content_id = std::string{*value};
            } else {
                QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no CONTENT_ID");
                return;
            }
            std::string entitlement_label = Common::SplitString(content_id, '-')[2];

            auto addon_extract_path =
                Config::getAddonInstallDir() / pkg.GetTitleID() / entitlement_label;
            QString addonDirPath;
            Common::FS::PathToQString(addonDirPath, addon_extract_path);
            QDir addon_dir(addonDirPath);

            if (pkgType.contains("PATCH")) {
                QString pkg_app_version;
                if (auto app_ver = psf.GetString("APP_VER"); app_ver.has_value()) {
                    pkg_app_version = QString::fromStdString(std::string{*app_ver});
                } else {
                    QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no APP_VER");
                    return;
                }
                std::filesystem::path sce_folder_path =
                    std::filesystem::exists(game_update_path / "sce_sys" / "param.sfo")
                        ? game_update_path / "sce_sys" / "param.sfo"
                        : game_folder_path / "sce_sys" / "param.sfo";
                psf.Open(sce_folder_path);
                QString game_app_version;
                if (auto app_ver = psf.GetString("APP_VER"); app_ver.has_value()) {
                    game_app_version = QString::fromStdString(std::string{*app_ver});
                } else {
                    QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no APP_VER");
                    return;
                }
                double appD = game_app_version.toDouble();
                double pkgD = pkg_app_version.toDouble();
                if (pkgD == appD) {
                    msgBox.setText(QString(tr("Patch detected!") + "\n" +
                                           tr("PKG and Game versions match: ") + pkg_app_version +
                                           "\n" + tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else if (pkgD < appD) {
                    msgBox.setText(QString(tr("Patch detected!") + "\n" +
                                           tr("PKG Version %1 is older than installed version: ")
                                               .arg(pkg_app_version) +
                                           game_app_version + "\n" +
                                           tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else {
                    msgBox.setText(QString(tr("Patch detected!") + "\n" +
                                           tr("Game is installed: ") + game_app_version + "\n" +
                                           tr("Would you like to install Patch: ") +
                                           pkg_app_version + " ?"));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                }
                int result = msgBox.exec();
                if (result == QMessageBox::Yes) {
                    // Do nothing.
                } else {
                    return;
                }
            } else if (category == "ac") {
                if (!addon_dir.exists()) {
                    QMessageBox addonMsgBox;
                    addonMsgBox.setWindowTitle(tr("DLC Installation"));
                    addonMsgBox.setText(QString(tr("Would you like to install DLC: %1?"))
                                            .arg(QString::fromStdString(entitlement_label)));

                    addonMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    addonMsgBox.setDefaultButton(QMessageBox::No);
                    int result = addonMsgBox.exec();
                    if (result == QMessageBox::Yes) {
                        game_update_path = addon_extract_path;
                    } else {
                        return;
                    }
                } else {
                    msgBox.setText(QString(tr("DLC already installed:") + "\n" + addonDirPath +
                                           "\n\n" + tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                    int result = msgBox.exec();
                    if (result == QMessageBox::Yes) {
                        game_update_path = addon_extract_path;
                    } else {
                        return;
                    }
                }
            } else {
                msgBox.setText(QString(tr("Game already installed") + "\n" + gameDirPath + "\n" +
                                       tr("Would you like to overwrite?")));
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
            if (pkgType.contains("PATCH") || category == "ac") {
                QMessageBox::information(
                    this, tr("PKG Extraction"),
                    tr("PKG is a patch or DLC, please install the game first!"));
                return;
            }
            // what else?
        }
        if (!pkg.Extract(file, game_update_path, failreason)) {
            QMessageBox::critical(this, tr("PKG ERROR"), QString::fromStdString(failreason));
        } else {
            int nfiles = pkg.GetNumberOfFiles();

            if (nfiles > 0) {
                QVector<int> indices;
                for (int i = 0; i < nfiles; i++) {
                    indices.append(i);
                }

                QProgressDialog dialog;
                dialog.setWindowTitle(tr("PKG Extraction"));
                dialog.setWindowModality(Qt::WindowModal);
                QString extractmsg = QString(tr("Extracting PKG %1/%2")).arg(pkgNum).arg(nPkg);
                dialog.setLabelText(extractmsg);
                dialog.setAutoClose(true);
                dialog.setRange(0, nfiles);

                dialog.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                                       dialog.size(), this->geometry()));

                QFutureWatcher<void> futureWatcher;
                connect(&futureWatcher, &QFutureWatcher<void>::finished, this, [=, this]() {
                    if (pkgNum == nPkg) {
                        QString path;

                        // We want to show the parent path instead of the full path
                        Common::FS::PathToQString(path, game_folder_path.parent_path());
                        QIcon windowIcon(
                            Common::FS::PathToUTF8String(game_folder_path / "sce_sys/icon0.png")
                                .c_str());

                        QMessageBox extractMsgBox(this);
                        extractMsgBox.setWindowTitle(tr("Extraction Finished"));
                        if (!windowIcon.isNull()) {
                            extractMsgBox.setWindowIcon(windowIcon);
                        }
                        extractMsgBox.setText(
                            QString(tr("Game successfully installed at %1")).arg(path));
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
                    if (delete_file_on_install) {
                        std::filesystem::remove(file);
                    }
                });
                connect(&dialog, &QProgressDialog::canceled, [&]() { futureWatcher.cancel(); });
                connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &dialog,
                        &QProgressDialog::setValue);
                futureWatcher.setFuture(
                    QtConcurrent::map(indices, [&](int index) { pkg.ExtractFiles(index); }));
                dialog.exec();
            }
        }
    } else {
        QMessageBox::critical(this, tr("PKG ERROR"),
                              tr("File doesn't appear to be a valid PKG file"));
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
    case Theme::Gruvbox:
        ui->setThemeGruvbox->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    case Theme::TokyoNight:
        ui->setThemeTokyoNight->setChecked(true);
        isIconBlack = false;
        SetUiIcons(false);
        break;
    case Theme::Oled:
        ui->setThemeOled->setChecked(true);
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
    ui->bootGameAct->setIcon(RecolorIcon(ui->bootGameAct->icon(), isWhite));
    ui->shadFolderAct->setIcon(RecolorIcon(ui->shadFolderAct->icon(), isWhite));
    ui->exitAct->setIcon(RecolorIcon(ui->exitAct->icon(), isWhite));
#ifdef ENABLE_UPDATER
    ui->updaterAct->setIcon(RecolorIcon(ui->updaterAct->icon(), isWhite));
#endif
    ui->downloadCheatsPatchesAct->setIcon(
        RecolorIcon(ui->downloadCheatsPatchesAct->icon(), isWhite));
    ui->dumpGameListAct->setIcon(RecolorIcon(ui->dumpGameListAct->icon(), isWhite));
    ui->aboutAct->setIcon(RecolorIcon(ui->aboutAct->icon(), isWhite));
    ui->setlistModeListAct->setIcon(RecolorIcon(ui->setlistModeListAct->icon(), isWhite));
    ui->setlistModeGridAct->setIcon(RecolorIcon(ui->setlistModeGridAct->icon(), isWhite));
    ui->gameInstallPathAct->setIcon(RecolorIcon(ui->gameInstallPathAct->icon(), isWhite));
    ui->menuThemes->setIcon(RecolorIcon(ui->menuThemes->icon(), isWhite));
    ui->menuGame_List_Icons->setIcon(RecolorIcon(ui->menuGame_List_Icons->icon(), isWhite));
    ui->menuUtils->setIcon(RecolorIcon(ui->menuUtils->icon(), isWhite));
    ui->playButton->setIcon(RecolorIcon(ui->playButton->icon(), isWhite));
    ui->pauseButton->setIcon(RecolorIcon(ui->pauseButton->icon(), isWhite));
    ui->stopButton->setIcon(RecolorIcon(ui->stopButton->icon(), isWhite));
    ui->refreshButton->setIcon(RecolorIcon(ui->refreshButton->icon(), isWhite));
    ui->settingsButton->setIcon(RecolorIcon(ui->settingsButton->icon(), isWhite));
    ui->controllerButton->setIcon(RecolorIcon(ui->controllerButton->icon(), isWhite));
    ui->keyboardButton->setIcon(RecolorIcon(ui->keyboardButton->icon(), isWhite));
    ui->refreshGameListAct->setIcon(RecolorIcon(ui->refreshGameListAct->icon(), isWhite));
    ui->menuGame_List_Mode->setIcon(RecolorIcon(ui->menuGame_List_Mode->icon(), isWhite));
    ui->pkgViewerAct->setIcon(RecolorIcon(ui->pkgViewerAct->icon(), isWhite));
    ui->configureAct->setIcon(RecolorIcon(ui->configureAct->icon(), isWhite));
    ui->addElfFolderAct->setIcon(RecolorIcon(ui->addElfFolderAct->icon(), isWhite));
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
    Config::saveMainWindow(config_dir / "config.toml");
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
        auto gamePath = Common::FS::PathFromQString(action->text());
        AddRecentFiles(action->text()); // Update the list.
        if (!std::filesystem::exists(gamePath)) {
            QMessageBox::critical(nullptr, tr("Run Game"), QString(tr("Eboot.bin file not found")));
            return;
        }
        StartEmulator(gamePath);
    });
}

void MainWindow::LoadTranslation() {
    auto language = QString::fromStdString(Config::getEmulatorLanguage());

    const QString base_dir = QStringLiteral(":/translations");
    QString base_path = QStringLiteral("%1/%2.qm").arg(base_dir).arg(language);

    if (QFile::exists(base_path)) {
        if (translator != nullptr) {
            qApp->removeTranslator(translator);
        }

        translator = new QTranslator(qApp);
        if (!translator->load(base_path)) {
            QMessageBox::warning(
                nullptr, QStringLiteral("Translation Error"),
                QStringLiteral("Failed to find load translation file for '%1':\n%2")
                    .arg(language)
                    .arg(base_path));
            delete translator;
        } else {
            qApp->installTranslator(translator);
            ui->retranslateUi(this);
        }
    }
}

void MainWindow::OnLanguageChanged(const std::string& locale) {
    Config::setEmulatorLanguage(locale);

    LoadTranslation();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
            auto tblMode = Config::getTableMode();
            if (tblMode != 2 && (tblMode != 1 || m_game_grid_frame->IsValidCellSelected())) {
                StartGame();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::StartEmulator(std::filesystem::path path) {
    if (isGameRunning) {
        QMessageBox::critical(nullptr, tr("Run Game"), QString(tr("Game is already running!")));
        return;
    }
    isGameRunning = true;
#ifdef __APPLE__
    // SDL on macOS requires main thread.
    Core::Emulator emulator;
    emulator.Run(path);
#else
    std::thread emulator_thread([=] {
        Core::Emulator emulator;
        emulator.Run(path);
    });
    emulator_thread.detach();
#endif
}
