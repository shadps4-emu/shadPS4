// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "main_window_themes.h"

void WindowThemes::SetWindowTheme(Theme theme, QLineEdit* mw_searchbar) {
    QPalette themePalette;

    switch (theme) {
    case Theme::Dark:
        mw_searchbar->setStyleSheet("background-color: #1e1e1e;" // Dark background
                                    "color: #ffffff;"            // White text
                                    "border: 2px solid #ffffff;" // White border
                                    "padding: 5px;");
        themePalette.setColor(QPalette::Window, QColor(50, 50, 50));
        themePalette.setColor(QPalette::WindowText, Qt::white);
        themePalette.setColor(QPalette::Base, QColor(20, 20, 20));
        themePalette.setColor(QPalette::AlternateBase, QColor(25, 25, 25));
        themePalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        themePalette.setColor(QPalette::ToolTipBase, Qt::white);
        themePalette.setColor(QPalette::ToolTipText, Qt::white);
        themePalette.setColor(QPalette::Text, Qt::white);
        themePalette.setColor(QPalette::Button, QColor(53, 53, 53));
        themePalette.setColor(QPalette::ButtonText, Qt::white);
        themePalette.setColor(QPalette::BrightText, Qt::red);
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218));
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        themePalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(themePalette);
        break;

    case Theme::Light:
        mw_searchbar->setStyleSheet("background-color: #ffffff;" // Light gray background
                                    "color: #000000;"            // Black text
                                    "border: 2px solid #000000;" // Black border
                                    "padding: 5px;");
        themePalette.setColor(QPalette::Window, QColor(240, 240, 240));   // Light gray
        themePalette.setColor(QPalette::WindowText, Qt::black);           // Black
        themePalette.setColor(QPalette::Base, QColor(230, 230, 230, 80)); // Grayish
        themePalette.setColor(QPalette::ToolTipBase, Qt::black);          // Black
        themePalette.setColor(QPalette::ToolTipText, Qt::black);          // Black
        themePalette.setColor(QPalette::Text, Qt::black);                 // Black
        themePalette.setColor(QPalette::Button, QColor(240, 240, 240));   // Light gray
        themePalette.setColor(QPalette::ButtonText, Qt::black);           // Black
        themePalette.setColor(QPalette::BrightText, Qt::red);             // Red
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218));      // Blue
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Blue
        themePalette.setColor(QPalette::HighlightedText, Qt::white);      // White
        qApp->setPalette(themePalette);
        break;

    case Theme::Green:
        mw_searchbar->setStyleSheet("background-color: #1e1e1e;" // Dark background
                                    "color: #ffffff;"            // White text
                                    "border: 2px solid #ffffff;" // White border
                                    "padding: 5px;");
        themePalette.setColor(QPalette::Window, QColor(53, 69, 53)); // Dark green background
        themePalette.setColor(QPalette::WindowText, Qt::white);      // White text
        themePalette.setColor(QPalette::Base, QColor(25, 40, 25));   // Darker green base
        themePalette.setColor(QPalette::AlternateBase,
                              QColor(53, 69, 53));                   // Dark green alternate base
        themePalette.setColor(QPalette::ToolTipBase, Qt::white);     // White tooltip background
        themePalette.setColor(QPalette::ToolTipText, Qt::white);     // White tooltip text
        themePalette.setColor(QPalette::Text, Qt::white);            // White text
        themePalette.setColor(QPalette::Button, QColor(53, 69, 53)); // Dark green button
        themePalette.setColor(QPalette::ButtonText, Qt::white);      // White button text
        themePalette.setColor(QPalette::BrightText, Qt::red);        // Bright red text for alerts
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218)); // Light blue links
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Light blue highlight
        themePalette.setColor(QPalette::HighlightedText, Qt::black);      // Black highlighted text

        qApp->setPalette(themePalette);
        break;

    case Theme::Blue:
        mw_searchbar->setStyleSheet("background-color: #1e1e1e;" // Dark background
                                    "color: #ffffff;"            // White text
                                    "border: 2px solid #ffffff;" // White border
                                    "padding: 5px;");
        themePalette.setColor(QPalette::Window, QColor(40, 60, 90)); // Dark blue background
        themePalette.setColor(QPalette::WindowText, Qt::white);      // White text
        themePalette.setColor(QPalette::Base, QColor(20, 40, 60));   // Darker blue base
        themePalette.setColor(QPalette::AlternateBase,
                              QColor(40, 60, 90));                   // Dark blue alternate base
        themePalette.setColor(QPalette::ToolTipBase, Qt::white);     // White tooltip background
        themePalette.setColor(QPalette::ToolTipText, Qt::white);     // White tooltip text
        themePalette.setColor(QPalette::Text, Qt::white);            // White text
        themePalette.setColor(QPalette::Button, QColor(40, 60, 90)); // Dark blue button
        themePalette.setColor(QPalette::ButtonText, Qt::white);      // White button text
        themePalette.setColor(QPalette::BrightText, Qt::red);        // Bright red text for alerts
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218)); // Light blue links
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Light blue highlight
        themePalette.setColor(QPalette::HighlightedText, Qt::black);      // Black highlighted text

        qApp->setPalette(themePalette);
        break;

    case Theme::Violet:
        mw_searchbar->setStyleSheet("background-color: #1e1e1e;" // Dark background
                                    "color: #ffffff;"            // White text
                                    "border: 2px solid #ffffff;" // White border
                                    "padding: 5px;");
        themePalette.setColor(QPalette::Window, QColor(100, 50, 120)); // Violet background
        themePalette.setColor(QPalette::WindowText, Qt::white);        // White text
        themePalette.setColor(QPalette::Base, QColor(80, 30, 90));     // Darker violet base
        themePalette.setColor(QPalette::AlternateBase,
                              QColor(100, 50, 120));                   // Violet alternate base
        themePalette.setColor(QPalette::ToolTipBase, Qt::white);       // White tooltip background
        themePalette.setColor(QPalette::ToolTipText, Qt::white);       // White tooltip text
        themePalette.setColor(QPalette::Text, Qt::white);              // White text
        themePalette.setColor(QPalette::Button, QColor(100, 50, 120)); // Violet button
        themePalette.setColor(QPalette::ButtonText, Qt::white);        // White button text
        themePalette.setColor(QPalette::BrightText, Qt::red);          // Bright red text for alerts
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218));   // Light blue links
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Light blue highlight
        themePalette.setColor(QPalette::HighlightedText, Qt::black);      // Black highlighted text

        qApp->setPalette(themePalette);
        break;
    }
}