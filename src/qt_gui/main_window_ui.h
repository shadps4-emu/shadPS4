// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMenuBar>
#include <QPushButton>
#include <QToolBar>

class Ui_MainWindow {
public:
    QAction* bootInstallPkgAct;
    QAction* bootGameAct;
    QAction* addElfFolderAct;
    QAction* exitAct;
    QAction* showGameListAct;
    QAction* refreshGameListAct;
    QAction* setIconSizeTinyAct;
    QAction* setIconSizeSmallAct;
    QAction* setIconSizeMediumAct;
    QAction* setIconSizeLargeAct;
    QAction* setlistModeListAct;
    QAction* setlistModeGridAct;
    QAction* setlistElfAct;
    QAction* gameInstallPathAct;
    QAction* downloadCheatsPatchesAct;
    QAction* dumpGameListAct;
    QAction* pkgViewerAct;
    QAction* aboutAct;
    QAction* configureAct;
    QAction* setThemeDark;
    QAction* setThemeLight;
    QAction* setThemeGreen;
    QAction* setThemeBlue;
    QAction* setThemeViolet;
    QWidget* centralWidget;
    QLineEdit* mw_searchbar;
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QPushButton* refreshButton;
    QPushButton* settingsButton;
    QPushButton* controllerButton;

    QWidget* sizeSliderContainer;
    QHBoxLayout* sizeSliderContainer_layout;
    QSlider* sizeSlider;
    QMenuBar* menuBar;
    QMenu* menuFile;
    QMenu* menuRecent;
    QMenu* menuView;
    QMenu* menuGame_List_Icons;
    QMenu* menuGame_List_Mode;
    QMenu* menuSettings;
    QMenu* menuUtils;
    QMenu* menuThemes;
    QMenu* menuAbout;
    QToolBar* toolBar;

    void setupUi(QMainWindow* MainWindow) {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        // MainWindow->resize(1280, 720);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":images/shadps4.ico"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setMinimumSize(QSize(4, 0));
        MainWindow->setAutoFillBackground(false);
        MainWindow->setAnimated(true);
        MainWindow->setDockNestingEnabled(true);
        MainWindow->setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks |
                                   QMainWindow::AnimatedDocks | QMainWindow::GroupedDragging);
        bootInstallPkgAct = new QAction(MainWindow);
        bootInstallPkgAct->setObjectName("bootInstallPkgAct");
        bootInstallPkgAct->setIcon(QIcon(":images/file_icon.png"));
        bootGameAct = new QAction(MainWindow);
        bootGameAct->setObjectName("bootGameAct");
        bootGameAct->setIcon(QIcon(":images/play_icon.png"));
        addElfFolderAct = new QAction(MainWindow);
        addElfFolderAct->setObjectName("addElfFolderAct");
        addElfFolderAct->setIcon(QIcon(":images/folder_icon.png"));
        exitAct = new QAction(MainWindow);
        exitAct->setObjectName("exitAct");
        exitAct->setIcon(QIcon(":images/exit_icon.png"));
        showGameListAct = new QAction(MainWindow);
        showGameListAct->setObjectName("showGameListAct");
        showGameListAct->setCheckable(true);
        refreshGameListAct = new QAction(MainWindow);
        refreshGameListAct->setObjectName("refreshGameListAct");
        refreshGameListAct->setIcon(QIcon(":images/refresh_icon.png"));
        setIconSizeTinyAct = new QAction(MainWindow);
        setIconSizeTinyAct->setObjectName("setIconSizeTinyAct");
        setIconSizeTinyAct->setCheckable(true);
        setIconSizeSmallAct = new QAction(MainWindow);
        setIconSizeSmallAct->setObjectName("setIconSizeSmallAct");
        setIconSizeSmallAct->setCheckable(true);
        setIconSizeMediumAct = new QAction(MainWindow);
        setIconSizeMediumAct->setObjectName("setIconSizeMediumAct");
        setIconSizeMediumAct->setCheckable(true);
        setIconSizeLargeAct = new QAction(MainWindow);
        setIconSizeLargeAct->setObjectName("setIconSizeLargeAct");
        setIconSizeLargeAct->setCheckable(true);
        setlistModeListAct = new QAction(MainWindow);
        setlistModeListAct->setObjectName("setlistModeListAct");
        setlistModeListAct->setCheckable(true);
        setlistModeListAct->setChecked(true);
        setlistModeListAct->setIcon(QIcon(":images/list_icon.png"));
        setlistModeGridAct = new QAction(MainWindow);
        setlistModeGridAct->setObjectName("setlistModeGridAct");
        setlistModeGridAct->setCheckable(true);
        setlistModeGridAct->setIcon(QIcon(":images/grid_icon.png"));
        setlistElfAct = new QAction(MainWindow);
        setlistElfAct->setObjectName("setlistModeGridAct");
        setlistElfAct->setCheckable(true);
        gameInstallPathAct = new QAction(MainWindow);
        gameInstallPathAct->setObjectName("gameInstallPathAct");
        gameInstallPathAct->setIcon(QIcon(":images/folder_icon.png"));
        downloadCheatsPatchesAct = new QAction(MainWindow);
        downloadCheatsPatchesAct->setObjectName("downloadCheatsPatchesAct");
        dumpGameListAct = new QAction(MainWindow);
        dumpGameListAct->setObjectName("dumpGameList");
        pkgViewerAct = new QAction(MainWindow);
        pkgViewerAct->setObjectName("pkgViewer");
        pkgViewerAct->setIcon(QIcon(":images/file_icon.png"));
        aboutAct = new QAction(MainWindow);
        aboutAct->setObjectName("aboutAct");
        aboutAct->setIcon(QIcon(":images/about_icon.png"));
        configureAct = new QAction(MainWindow);
        configureAct->setObjectName("configureAct");
        configureAct->setIcon(QIcon(":images/settings_icon.png"));
        setThemeDark = new QAction(MainWindow);
        setThemeDark->setObjectName("setThemeDark");
        setThemeDark->setCheckable(true);
        setThemeDark->setChecked(true);
        setThemeLight = new QAction(MainWindow);
        setThemeLight->setObjectName("setThemeLight");
        setThemeLight->setCheckable(true);
        setThemeGreen = new QAction(MainWindow);
        setThemeGreen->setObjectName("setThemeGreen");
        setThemeGreen->setCheckable(true);
        setThemeBlue = new QAction(MainWindow);
        setThemeBlue->setObjectName("setThemeBlue");
        setThemeBlue->setCheckable(true);
        setThemeViolet = new QAction(MainWindow);
        setThemeViolet->setObjectName("setThemeViolet");
        setThemeViolet->setCheckable(true);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName("centralWidget");
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        mw_searchbar = new QLineEdit(centralWidget);
        mw_searchbar->setObjectName("mw_searchbar");
        mw_searchbar->setGeometry(QRect(250, 10, 130, 31));
        mw_searchbar->setMaximumWidth(250);
        QFont font;
        font.setPointSize(10);
        font.setBold(false);
        mw_searchbar->setFont(font);
        mw_searchbar->setFocusPolicy(Qt::ClickFocus);
        mw_searchbar->setFrame(false);
        mw_searchbar->setClearButtonEnabled(false);

        playButton = new QPushButton(centralWidget);
        playButton->setFlat(true);
        playButton->setIcon(QIcon(":images/play_icon.png"));
        playButton->setIconSize(QSize(40, 40));
        pauseButton = new QPushButton(centralWidget);
        pauseButton->setFlat(true);
        pauseButton->setIcon(QIcon(":images/pause_icon.png"));
        pauseButton->setIconSize(QSize(40, 40));
        stopButton = new QPushButton(centralWidget);
        stopButton->setFlat(true);
        stopButton->setIcon(QIcon(":images/stop_icon.png"));
        stopButton->setIconSize(QSize(40, 40));
        refreshButton = new QPushButton(centralWidget);
        refreshButton->setFlat(true);
        refreshButton->setIcon(QIcon(":images/refresh_icon.png"));
        refreshButton->setIconSize(QSize(32, 32));
        settingsButton = new QPushButton(centralWidget);
        settingsButton->setFlat(true);
        settingsButton->setIcon(QIcon(":images/settings_icon.png"));
        settingsButton->setIconSize(QSize(40, 40));
        controllerButton = new QPushButton(centralWidget);
        controllerButton->setFlat(true);
        controllerButton->setIcon(QIcon(":images/controller_icon.png"));
        controllerButton->setIconSize(QSize(40, 40));

        sizeSliderContainer = new QWidget(centralWidget);
        sizeSliderContainer->setObjectName("sizeSliderContainer");
        sizeSliderContainer->setGeometry(QRect(280, 10, 181, 31));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizeSliderContainer->sizePolicy().hasHeightForWidth());
        sizeSliderContainer->setSizePolicy(sizePolicy1);
        sizeSliderContainer_layout = new QHBoxLayout(sizeSliderContainer);
        sizeSliderContainer_layout->setSpacing(0);
        sizeSliderContainer_layout->setContentsMargins(11, 11, 11, 11);
        sizeSliderContainer_layout->setObjectName("sizeSliderContainer_layout");
        sizeSliderContainer_layout->setContentsMargins(14, 0, 14, 0);
        sizeSlider = new QSlider(sizeSliderContainer);
        sizeSlider->setObjectName("sizeSlider");
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sizeSlider->sizePolicy().hasHeightForWidth());
        sizeSlider->setSizePolicy(sizePolicy2);
        sizeSlider->setFocusPolicy(Qt::ClickFocus);
        sizeSlider->setAutoFillBackground(false);
        sizeSlider->setOrientation(Qt::Horizontal);
        sizeSlider->setTickPosition(QSlider::NoTicks);
        sizeSlider->setMinimum(0);
        sizeSlider->setMaximum(220);

        sizeSliderContainer_layout->addWidget(sizeSlider);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 1058, 22));
        menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName("menuFile");
        menuRecent = new QMenu(menuFile);
        menuRecent->setObjectName("menuRecent");
        menuView = new QMenu(menuBar);
        menuView->setObjectName("menuView");
        menuGame_List_Icons = new QMenu(menuView);
        menuGame_List_Icons->setObjectName("menuGame_List_Icons");
        menuGame_List_Icons->setIcon(QIcon(":images/iconsize_icon.png"));
        menuGame_List_Mode = new QMenu(menuView);
        menuGame_List_Mode->setObjectName("menuGame_List_Mode");
        menuGame_List_Mode->setIcon(QIcon(":images/list_mode_icon.png"));
        menuSettings = new QMenu(menuBar);
        menuSettings->setObjectName("menuSettings");
        menuUtils = new QMenu(menuSettings);
        menuUtils->setObjectName("menuUtils");
        menuThemes = new QMenu(menuView);
        menuThemes->setObjectName("menuThemes");
        menuThemes->setIcon(QIcon(":images/themes_icon.png"));
        menuAbout = new QMenu(menuBar);
        menuAbout->setObjectName("menuAbout");
        MainWindow->setMenuBar(menuBar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName("toolBar");
        MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuSettings->menuAction());
        menuBar->addAction(menuAbout->menuAction());
        menuFile->addAction(bootInstallPkgAct);
        menuFile->addAction(bootGameAct);
        menuFile->addAction(addElfFolderAct);
        menuFile->addSeparator();
        menuFile->addAction(menuRecent->menuAction());
        menuFile->addSeparator();
        menuFile->addAction(exitAct);
        menuView->addAction(showGameListAct);
        menuView->addSeparator();
        menuView->addAction(refreshGameListAct);
        menuView->addAction(menuGame_List_Mode->menuAction());
        menuView->addAction(menuGame_List_Icons->menuAction());
        menuView->addAction(menuThemes->menuAction());
        menuThemes->addAction(setThemeDark);
        menuThemes->addAction(setThemeLight);
        menuThemes->addAction(setThemeGreen);
        menuThemes->addAction(setThemeBlue);
        menuThemes->addAction(setThemeViolet);
        menuGame_List_Icons->addAction(setIconSizeTinyAct);
        menuGame_List_Icons->addAction(setIconSizeSmallAct);
        menuGame_List_Icons->addAction(setIconSizeMediumAct);
        menuGame_List_Icons->addAction(setIconSizeLargeAct);
        menuGame_List_Mode->addAction(setlistModeListAct);
        menuGame_List_Mode->addAction(setlistModeGridAct);
        menuGame_List_Mode->addAction(setlistElfAct);
        menuSettings->addAction(configureAct);
        menuSettings->addAction(gameInstallPathAct);
        menuSettings->addAction(menuUtils->menuAction());
        menuUtils->addAction(downloadCheatsPatchesAct);
        menuUtils->addAction(dumpGameListAct);
        menuUtils->addAction(pkgViewerAct);
        menuAbout->addAction(aboutAct);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow* MainWindow) {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "shadPS4", nullptr));
        addElfFolderAct->setText(
            QCoreApplication::translate("MainWindow", "Open/Add Elf Folder", nullptr));
        bootInstallPkgAct->setText(
            QCoreApplication::translate("MainWindow", "Install Packages (PKG)", nullptr));
        bootGameAct->setText(QCoreApplication::translate("MainWindow", "Boot Game", nullptr));
        aboutAct->setText(QCoreApplication::translate("MainWindow", "About shadPS4", nullptr));
        configureAct->setText(QCoreApplication::translate("MainWindow", "Configure...", nullptr));
#if QT_CONFIG(tooltip)
        bootInstallPkgAct->setToolTip(QCoreApplication::translate(
            "MainWindow", "Install application from a .pkg file", nullptr));
#endif // QT_CONFIG(tooltip)
        menuRecent->setTitle(QCoreApplication::translate("MainWindow", "Recent Games", nullptr));
        exitAct->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
#if QT_CONFIG(tooltip)
        exitAct->setToolTip(QCoreApplication::translate("MainWindow", "Exit shadPS4", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        exitAct->setStatusTip(
            QCoreApplication::translate("MainWindow", "Exit the application.", nullptr));
#endif // QT_CONFIG(statustip)
        showGameListAct->setText(
            QCoreApplication::translate("MainWindow", "Show Game List", nullptr));
        refreshGameListAct->setText(
            QCoreApplication::translate("MainWindow", "Game List Refresh", nullptr));
        setIconSizeTinyAct->setText(QCoreApplication::translate("MainWindow", "Tiny", nullptr));
        setIconSizeSmallAct->setText(QCoreApplication::translate("MainWindow", "Small", nullptr));
        setIconSizeMediumAct->setText(QCoreApplication::translate("MainWindow", "Medium", nullptr));
        setIconSizeLargeAct->setText(QCoreApplication::translate("MainWindow", "Large", nullptr));
        setlistModeListAct->setText(
            QCoreApplication::translate("MainWindow", "List View", nullptr));
        setlistModeGridAct->setText(
            QCoreApplication::translate("MainWindow", "Grid View", nullptr));
        setlistElfAct->setText(QCoreApplication::translate("MainWindow", "Elf Viewer", nullptr));
        gameInstallPathAct->setText(
            QCoreApplication::translate("MainWindow", "Game Install Directory", nullptr));
        downloadCheatsPatchesAct->setText(
            QCoreApplication::translate("MainWindow", "Download Cheats/Patches", nullptr));
        dumpGameListAct->setText(
            QCoreApplication::translate("MainWindow", "Dump Game List", nullptr));
        pkgViewerAct->setText(QCoreApplication::translate("MainWindow", "PKG Viewer", nullptr));
        mw_searchbar->setPlaceholderText(
            QCoreApplication::translate("MainWindow", "Search...", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "View", nullptr));
        menuGame_List_Icons->setTitle(
            QCoreApplication::translate("MainWindow", "Game List Icons", nullptr));
        menuGame_List_Mode->setTitle(
            QCoreApplication::translate("MainWindow", "Game List Mode", nullptr));
        menuSettings->setTitle(QCoreApplication::translate("MainWindow", "Settings", nullptr));
        menuUtils->setTitle(QCoreApplication::translate("MainWindow", "Utils", nullptr));
        menuThemes->setTitle(QCoreApplication::translate("MainWindow", "Themes", nullptr));
        menuAbout->setTitle(QCoreApplication::translate("MainWindow", "About", nullptr));
        setThemeDark->setText(QCoreApplication::translate("MainWindow", "Dark", nullptr));
        setThemeLight->setText(QCoreApplication::translate("MainWindow", "Light", nullptr));
        setThemeGreen->setText(QCoreApplication::translate("MainWindow", "Green", nullptr));
        setThemeBlue->setText(QCoreApplication::translate("MainWindow", "Blue", nullptr));
        setThemeViolet->setText(QCoreApplication::translate("MainWindow", "Violet", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
    } // retranslateUi
};

namespace Ui {
class MainWindow : public Ui_MainWindow {};
} // namespace Ui