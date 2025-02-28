// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "main_window_themes.h"

void WindowThemes::SetWindowTheme(Theme theme, QLineEdit* mw_searchbar) {
    QPalette themePalette;

    qApp->setStyleSheet("");
    switch (theme) {
    case Theme::Dark:
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #1e1e1e; color: #ffffff; border: 1px solid #ffffff; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #2A82DA; }");
        themePalette.setColor(QPalette::Window, QColor(50, 50, 50));
        themePalette.setColor(QPalette::WindowText, Qt::white);
        themePalette.setColor(QPalette::Base, QColor(20, 20, 20));
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
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #ffffff; color: #000000; border: 1px solid #000000; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #2A82DA; }");
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
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #192819; color: #ffffff; border: 1px solid #ffffff; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #2A82DA; }");
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
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #14283c; color: #ffffff; border: 1px solid #ffffff; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #2A82DA; }");
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
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #501e5a; color: #ffffff; border: 1px solid #ffffff; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #2A82DA; }");
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
    case Theme::Gruvbox:
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #1d2021; color: #f9f5d7; border: 1px solid #f9f5d7; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #83A598; }");
        themePalette.setColor(QPalette::Window, QColor(29, 32, 33));
        themePalette.setColor(QPalette::WindowText, QColor(249, 245, 215));
        themePalette.setColor(QPalette::Base, QColor(29, 32, 33));
        themePalette.setColor(QPalette::AlternateBase, QColor(50, 48, 47));
        themePalette.setColor(QPalette::ToolTipBase, QColor(249, 245, 215));
        themePalette.setColor(QPalette::ToolTipText, QColor(249, 245, 215));
        themePalette.setColor(QPalette::Text, QColor(249, 245, 215));
        themePalette.setColor(QPalette::Button, QColor(40, 40, 40));
        themePalette.setColor(QPalette::ButtonText, QColor(249, 245, 215));
        themePalette.setColor(QPalette::BrightText, QColor(251, 73, 52));
        themePalette.setColor(QPalette::Link, QColor(131, 165, 152));
        themePalette.setColor(QPalette::Highlight, QColor(131, 165, 152));
        themePalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(themePalette);
        break;
    case Theme::TokyoNight:
        mw_searchbar->setStyleSheet(
            "QLineEdit {"
            "background-color: #1a1b26; color: #9d7cd8; border: 1px solid #9d7cd8; "
            "border-radius: 4px; padding: 5px; }"
            "QLineEdit:focus {"
            "border: 1px solid #7aa2f7; }");
        themePalette.setColor(QPalette::Window, QColor(31, 35, 53));
        themePalette.setColor(QPalette::WindowText, QColor(192, 202, 245));
        themePalette.setColor(QPalette::Base, QColor(25, 28, 39));
        themePalette.setColor(QPalette::AlternateBase, QColor(36, 40, 59));
        themePalette.setColor(QPalette::ToolTipBase, QColor(192, 202, 245));
        themePalette.setColor(QPalette::ToolTipText, QColor(192, 202, 245));
        themePalette.setColor(QPalette::Text, QColor(192, 202, 245));
        themePalette.setColor(QPalette::Button, QColor(30, 30, 41));
        themePalette.setColor(QPalette::ButtonText, QColor(192, 202, 245));
        themePalette.setColor(QPalette::BrightText, QColor(197, 59, 83));
        themePalette.setColor(QPalette::Link, QColor(79, 214, 190));
        themePalette.setColor(QPalette::Highlight, QColor(79, 214, 190));
        themePalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(themePalette);
        break;
    case Theme::Oled:
        mw_searchbar->setStyleSheet("QLineEdit:focus {"
                                    "border: 1px solid #2A82DA; }");
        themePalette.setColor(QPalette::Window, Qt::black);
        themePalette.setColor(QPalette::WindowText, Qt::white);
        themePalette.setColor(QPalette::Base, Qt::black);
        themePalette.setColor(QPalette::AlternateBase, Qt::black);
        themePalette.setColor(QPalette::ToolTipBase, Qt::black);
        themePalette.setColor(QPalette::ToolTipText, Qt::white);
        themePalette.setColor(QPalette::Text, Qt::white);
        themePalette.setColor(QPalette::Button, QColor(5, 5, 5));
        themePalette.setColor(QPalette::ButtonText, Qt::white);
        themePalette.setColor(QPalette::BrightText, Qt::red);
        themePalette.setColor(QPalette::Link, QColor(42, 130, 218));
        themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        themePalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(themePalette);
        qApp->setStyleSheet("QLineEdit {"
                            "background-color: #000000; color: #ffffff; border: 1px solid #a0a0a0; "
                            "border-radius: 4px; padding: 5px; }"

                            "QCheckBox::indicator:unchecked {"
                            "border: 1px solid #808080; border-radius: 4px; }");
        break;
    }
}