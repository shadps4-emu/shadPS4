// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

#include "common/io_file.h"
#include "core/file_format/pkg.h"
#include "core/loader.h"
#include "game_install_dialog.h"
#include "game_list_frame.h"
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
    // add toolbar widgets
    QApplication::setStyle("Fusion");
    ui->toolBar->setObjectName("mw_toolbar");
    ui->sizeSlider->setRange(0, gui::game_list_max_slider_pos);
    ui->toolBar->addWidget(ui->sizeSliderContainer);
    ui->toolBar->addWidget(ui->mw_searchbar);

    CreateActions();
    CreateDockWindows();
    CreateConnects();
    SetLastUsedTheme();

    setMinimumSize(350, minimumSizeHint().height());
    setWindowTitle(QString::fromStdString("ShadPS4 v0.0.2"));

    ConfigureGuiFromSettings();

    show();

    // Fix possible hidden game list columns. The game list has to be visible already. Use this
    // after show()
    m_game_list_frame->FixNarrowColumns();

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

void MainWindow::CreateDockWindows() {
    m_main_window = new QMainWindow();
    m_main_window->setContextMenuPolicy(Qt::PreventContextMenu);

    m_game_list_frame = new GameListFrame(m_gui_settings, m_main_window);
    m_game_list_frame->setObjectName("gamelist");

    m_main_window->addDockWidget(Qt::LeftDockWidgetArea, m_game_list_frame);

    m_main_window->setDockNestingEnabled(true);

    setCentralWidget(m_main_window);

    connect(m_game_list_frame, &GameListFrame::GameListFrameClosed, this, [this]() {
        if (ui->showGameListAct->isChecked()) {
            ui->showGameListAct->setChecked(false);
            m_gui_settings->SetValue(gui::main_window_gamelist_visible, false);
        }
    });
}
void MainWindow::CreateConnects() {
    connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);

    connect(ui->showGameListAct, &QAction::triggered, this, [this](bool checked) {
        checked ? m_game_list_frame->show() : m_game_list_frame->hide();
        m_gui_settings->SetValue(gui::main_window_gamelist_visible, checked);
    });
    connect(ui->refreshGameListAct, &QAction::triggered, this,
            [this] { m_game_list_frame->Refresh(true); });

    connect(m_icon_size_act_group, &QActionGroup::triggered, this, [this](QAction* act) {
        static const int index_small = gui::get_Index(gui::game_list_icon_size_small);
        static const int index_medium = gui::get_Index(gui::game_list_icon_size_medium);

        int index;

        if (act == ui->setIconSizeTinyAct)
            index = 0;
        else if (act == ui->setIconSizeSmallAct)
            index = index_small;
        else if (act == ui->setIconSizeMediumAct)
            index = index_medium;
        else
            index = gui::game_list_max_slider_pos;

        m_save_slider_pos = true;
        ResizeIcons(index);
    });
    connect(m_game_list_frame, &GameListFrame::RequestIconSizeChange, this, [this](const int& val) {
        const int idx = ui->sizeSlider->value() + val;
        m_save_slider_pos = true;
        ResizeIcons(idx);
    });

    connect(m_list_mode_act_group, &QActionGroup::triggered, this, [this](QAction* act) {
        const bool is_list_act = act == ui->setlistModeListAct;
        if (is_list_act == m_is_list_mode)
            return;

        const int slider_pos = ui->sizeSlider->sliderPosition();
        ui->sizeSlider->setSliderPosition(m_other_slider_pos);
        SetIconSizeActions(m_other_slider_pos);
        m_other_slider_pos = slider_pos;

        m_is_list_mode = is_list_act;
        m_game_list_frame->SetListMode(m_is_list_mode);
    });
    connect(ui->sizeSlider, &QSlider::valueChanged, this, &MainWindow::ResizeIcons);
    connect(ui->sizeSlider, &QSlider::sliderReleased, this, [this] {
        const int index = ui->sizeSlider->value();
        m_gui_settings->SetValue(
            m_is_list_mode ? gui::game_list_iconSize : gui::game_list_iconSizeGrid, index);
        SetIconSizeActions(index);
    });
    connect(ui->sizeSlider, &QSlider::actionTriggered, this, [this](int action) {
        if (action != QAbstractSlider::SliderNoAction &&
            action !=
                QAbstractSlider::SliderMove) { // we only want to save on mouseclicks or slider
                                               // release (the other connect handles this)
            m_save_slider_pos = true; // actionTriggered happens before the value was changed
        }
    });

    connect(ui->mw_searchbar, &QLineEdit::textChanged, m_game_list_frame,
            &GameListFrame::SetSearchText);
    connect(ui->bootInstallPkgAct, &QAction::triggered, this, [this] { InstallPkg(); });
    connect(ui->gameInstallPathAct, &QAction::triggered, this, [this] { InstallDirectory(); });

    // Themes
    connect(ui->setThemeLight, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Light, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Light));
    });
    connect(ui->setThemeDark, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Dark, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Dark));
    });
    connect(ui->setThemeGreen, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Green, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Green));
    });
    connect(ui->setThemeBlue, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Blue, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Blue));
    });
    connect(ui->setThemeViolet, &QAction::triggered, &m_window_themes, [this]() {
        m_window_themes.SetWindowTheme(Theme::Violet, ui->mw_searchbar);
        m_gui_settings->SetValue(gui::mw_themes, static_cast<int>(Theme::Violet));
    });
}

void MainWindow::SetIconSizeActions(int idx) const {
    static const int threshold_tiny =
        gui::get_Index((gui::game_list_icon_size_small + gui::game_list_icon_size_min) / 2);
    static const int threshold_small =
        gui::get_Index((gui::game_list_icon_size_medium + gui::game_list_icon_size_small) / 2);
    static const int threshold_medium =
        gui::get_Index((gui::game_list_icon_size_max + gui::game_list_icon_size_medium) / 2);

    if (idx < threshold_tiny)
        ui->setIconSizeTinyAct->setChecked(true);
    else if (idx < threshold_small)
        ui->setIconSizeSmallAct->setChecked(true);
    else if (idx < threshold_medium)
        ui->setIconSizeMediumAct->setChecked(true);
    else
        ui->setIconSizeLargeAct->setChecked(true);
}
void MainWindow::ResizeIcons(int index) {
    if (ui->sizeSlider->value() != index) {
        ui->sizeSlider->setSliderPosition(index);
        return; // ResizeIcons will be triggered again by setSliderPosition, so return here
    }

    if (m_save_slider_pos) {
        m_save_slider_pos = false;
        m_gui_settings->SetValue(
            m_is_list_mode ? gui::game_list_iconSize : gui::game_list_iconSizeGrid, index);

        // this will also fire when we used the actions, but i didn't want to add another boolean
        // member
        SetIconSizeActions(index);
    }

    m_game_list_frame->ResizeIcons(index);
}
void MainWindow::ConfigureGuiFromSettings() {
    // Restore GUI state if needed. We need to if they exist.
    if (!restoreGeometry(m_gui_settings->GetValue(gui::main_window_geometry).toByteArray())) {
        resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
    }

    restoreState(m_gui_settings->GetValue(gui::main_window_windowState).toByteArray());
    m_main_window->restoreState(m_gui_settings->GetValue(gui::main_window_mwState).toByteArray());

    ui->showGameListAct->setChecked(
        m_gui_settings->GetValue(gui::main_window_gamelist_visible).toBool());

    m_game_list_frame->setVisible(ui->showGameListAct->isChecked());

    // handle icon size options
    m_is_list_mode = m_gui_settings->GetValue(gui::game_list_listMode).toBool();
    if (m_is_list_mode)
        ui->setlistModeListAct->setChecked(true);
    else
        ui->setlistModeGridAct->setChecked(true);

    const int icon_size_index =
        m_gui_settings
            ->GetValue(m_is_list_mode ? gui::game_list_iconSize : gui::game_list_iconSizeGrid)
            .toInt();
    m_other_slider_pos =
        m_gui_settings
            ->GetValue(!m_is_list_mode ? gui::game_list_iconSize : gui::game_list_iconSizeGrid)
            .toInt();
    ui->sizeSlider->setSliderPosition(icon_size_index);
    SetIconSizeActions(icon_size_index);

    // Gamelist
    m_game_list_frame->LoadSettings();
}

void MainWindow::SaveWindowState() const {
    // Save gui settings
    m_gui_settings->SetValue(gui::main_window_geometry, saveGeometry());
    m_gui_settings->SetValue(gui::main_window_windowState, saveState());
    m_gui_settings->SetValue(gui::main_window_mwState, m_main_window->saveState());

    // Save column settings
    m_game_list_frame->SaveSettings();
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
                                  QMessageBox::Ok, 0);
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
                                         "Game successfully installed at " + path, QMessageBox::Ok,
                                         0);
                m_game_list_frame->Refresh(true);
            }
        }
    } else {
        QMessageBox::critical(this, "PKG ERROR", "File doesn't appear to be a valid PKG file",
                              QMessageBox::Ok, 0);
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
        break;
    case Theme::Dark:
        ui->setThemeDark->setChecked(true);
        break;
    case Theme::Green:
        ui->setThemeGreen->setChecked(true);
        break;
    case Theme::Blue:
        ui->setThemeBlue->setChecked(true);
        break;
    case Theme::Violet:
        ui->setThemeViolet->setChecked(true);
        break;
    }
}