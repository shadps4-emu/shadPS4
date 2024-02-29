// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

/********************************************************************************
** Form generated from reading UI file 'main_window.ui'
**
** Created by: Qt User Interface Compiler version 6.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAIN_WINDOW_UI_H
#define MAIN_WINDOW_UI_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow {
public:
    QAction* bootInstallPkgAct;
    QAction* exitAct;
    QAction* showGameListAct;
    QAction* refreshGameListAct;
    QAction* setIconSizeTinyAct;
    QAction* setIconSizeSmallAct;
    QAction* setIconSizeMediumAct;
    QAction* setIconSizeLargeAct;
    QAction* setlistModeListAct;
    QAction* setlistModeGridAct;
    QAction* gameInstallPathAct;
    QAction* setThemeLight;
    QAction* setThemeDark;
    QAction* setThemeGreen;
    QAction* setThemeBlue;
    QAction* setThemeViolet;
    QWidget* centralWidget;
    QLineEdit* mw_searchbar;

    QWidget* sizeSliderContainer;
    QHBoxLayout* sizeSliderContainer_layout;
    QSlider* sizeSlider;
    QMenuBar* menuBar;
    QMenu* menuFile;
    QMenu* menuView;
    QMenu* menuGame_List_Icons;
    QMenu* menuGame_List_Mode;
    QMenu* menuSettings;
    QMenu* menuThemes;
    QToolBar* toolBar;

    void setupUi(QMainWindow* MainWindow) {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1058, 580);
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
        exitAct = new QAction(MainWindow);
        exitAct->setObjectName("exitAct");
        showGameListAct = new QAction(MainWindow);
        showGameListAct->setObjectName("showGameListAct");
        showGameListAct->setCheckable(true);
        refreshGameListAct = new QAction(MainWindow);
        refreshGameListAct->setObjectName("refreshGameListAct");
        setIconSizeTinyAct = new QAction(MainWindow);
        setIconSizeTinyAct->setObjectName("setIconSizeTinyAct");
        setIconSizeTinyAct->setCheckable(true);
        setIconSizeSmallAct = new QAction(MainWindow);
        setIconSizeSmallAct->setObjectName("setIconSizeSmallAct");
        setIconSizeSmallAct->setCheckable(true);
        setIconSizeSmallAct->setChecked(true);
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
        setlistModeGridAct = new QAction(MainWindow);
        setlistModeGridAct->setObjectName("setlistModeGridAct");
        setlistModeGridAct->setCheckable(true);
        gameInstallPathAct = new QAction(MainWindow);
        gameInstallPathAct->setObjectName("gameInstallPathAct");
        setThemeLight = new QAction(MainWindow);
        setThemeLight->setObjectName("setThemeLight");
        setThemeLight->setCheckable(true);
        setThemeLight->setChecked(true);
        setThemeDark = new QAction(MainWindow);
        setThemeDark->setObjectName("setThemeDark");
        setThemeDark->setCheckable(true);
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
        mw_searchbar->setGeometry(QRect(480, 10, 150, 31));
        sizePolicy.setHeightForWidth(mw_searchbar->sizePolicy().hasHeightForWidth());
        mw_searchbar->setSizePolicy(sizePolicy);
        mw_searchbar->setMaximumWidth(250);
        QFont font;
        font.setPointSize(10);
        font.setBold(false);
        mw_searchbar->setFont(font);
        mw_searchbar->setFocusPolicy(Qt::ClickFocus);
        mw_searchbar->setFrame(false);
        mw_searchbar->setClearButtonEnabled(false);

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

        sizeSliderContainer_layout->addWidget(sizeSlider);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 1058, 22));
        menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName("menuFile");
        menuView = new QMenu(menuBar);
        menuView->setObjectName("menuView");
        menuGame_List_Icons = new QMenu(menuView);
        menuGame_List_Icons->setObjectName("menuGame_List_Icons");
        menuGame_List_Mode = new QMenu(menuView);
        menuGame_List_Mode->setObjectName("menuGame_List_Mode");
        menuSettings = new QMenu(menuBar);
        menuSettings->setObjectName("menuSettings");
        menuThemes = new QMenu(menuView);
        menuThemes->setObjectName("menuThemes");
        MainWindow->setMenuBar(menuBar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName("toolBar");
        MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuSettings->menuAction());
        menuFile->addAction(bootInstallPkgAct);
        menuFile->addSeparator();
        menuFile->addAction(exitAct);
        menuView->addAction(showGameListAct);
        menuView->addSeparator();
        menuView->addAction(refreshGameListAct);
        menuView->addAction(menuGame_List_Mode->menuAction());
        menuView->addAction(menuGame_List_Icons->menuAction());
        menuView->addAction(menuThemes->menuAction());
        menuThemes->addAction(setThemeLight);
        menuThemes->addAction(setThemeDark);
        menuThemes->addAction(setThemeGreen);
        menuThemes->addAction(setThemeBlue);
        menuThemes->addAction(setThemeViolet);
        menuGame_List_Icons->addAction(setIconSizeTinyAct);
        menuGame_List_Icons->addAction(setIconSizeSmallAct);
        menuGame_List_Icons->addAction(setIconSizeMediumAct);
        menuGame_List_Icons->addAction(setIconSizeLargeAct);
        menuGame_List_Mode->addAction(setlistModeListAct);
        menuGame_List_Mode->addAction(setlistModeGridAct);
        menuSettings->addAction(gameInstallPathAct);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow* MainWindow) {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Shadps4", nullptr));
        bootInstallPkgAct->setText(
            QCoreApplication::translate("MainWindow", "Install Packages (PKG)", nullptr));
#if QT_CONFIG(tooltip)
        bootInstallPkgAct->setToolTip(QCoreApplication::translate(
            "MainWindow", "Install application from a .pkg file", nullptr));
#endif // QT_CONFIG(tooltip)
        exitAct->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
#if QT_CONFIG(tooltip)
        exitAct->setToolTip(QCoreApplication::translate("MainWindow", "Exit Shadps4", nullptr));
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
        gameInstallPathAct->setText(
            QCoreApplication::translate("MainWindow", "Game Install Directory", nullptr));
        mw_searchbar->setPlaceholderText(
            QCoreApplication::translate("MainWindow", "Search...", nullptr));
        // darkModeSwitch->setText(
        //    QCoreApplication::translate("MainWindow", "Game", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "View", nullptr));
        menuGame_List_Icons->setTitle(
            QCoreApplication::translate("MainWindow", "Game List Icons", nullptr));
        menuGame_List_Mode->setTitle(
            QCoreApplication::translate("MainWindow", "Game List Mode", nullptr));
        menuSettings->setTitle(QCoreApplication::translate("MainWindow", "Settings", nullptr));
        menuThemes->setTitle(QCoreApplication::translate("MainWindow", "Themes", nullptr));
        setThemeLight->setText(QCoreApplication::translate("MainWindow", "Light", nullptr));
        setThemeDark->setText(QCoreApplication::translate("MainWindow", "Dark", nullptr));
        setThemeGreen->setText(QCoreApplication::translate("MainWindow", "Green", nullptr));
        setThemeBlue->setText(QCoreApplication::translate("MainWindow", "Blue", nullptr));
        setThemeViolet->setText(QCoreApplication::translate("MainWindow", "Violet", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
    } // retranslateUi
};

namespace Ui {
class MainWindow : public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAIN_WINDOW_UI_H
