// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kbm_help_dialog.h"

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

ExpandableSection::ExpandableSection(const QString& title, const QString& content,
                                     QWidget* parent = nullptr)
    : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Button to toggle visibility of content
    toggleButton = new QPushButton(title);
    layout->addWidget(toggleButton);

    // QTextBrowser for content (initially hidden)
    contentBrowser = new QTextBrowser();
    contentBrowser->setPlainText(content);
    contentBrowser->setVisible(false);

    // Remove scrollbars from QTextBrowser
    contentBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set size policy to allow vertical stretching only
    contentBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Calculate and set initial height based on content
    updateContentHeight();

    layout->addWidget(contentBrowser);

    // Connect button click to toggle visibility
    connect(toggleButton, &QPushButton::clicked, [this]() {
        contentBrowser->setVisible(!contentBrowser->isVisible());
        if (contentBrowser->isVisible()) {
            updateContentHeight(); // Update height when expanding
        }
        emit expandedChanged(); // Notify for layout adjustments
    });

    // Connect to update height if content changes
    connect(contentBrowser->document(), &QTextDocument::contentsChanged, this,
            &ExpandableSection::updateContentHeight);

    // Minimal layout settings for spacing
    layout->setSpacing(2);
    layout->setContentsMargins(0, 0, 0, 0);
}

void HelpDialog::closeEvent(QCloseEvent* event) {
    *help_open_ptr = false;
    close();
}
void HelpDialog::reject() {
    *help_open_ptr = false;
    close();
}

HelpDialog::HelpDialog(bool* open_flag, QWidget* parent) : QDialog(parent) {
    help_open_ptr = open_flag;
    // Main layout for the help dialog
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Container widget for the scroll area
    QWidget* containerWidget = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(containerWidget);

    // Add expandable sections to container layout
    auto* quickstartSection = new ExpandableSection("Quickstart", quickstart());
    auto* faqSection = new ExpandableSection("FAQ", faq());
    auto* syntaxSection = new ExpandableSection("Syntax", syntax());
    auto* specialSection = new ExpandableSection("Special Bindings", special());
    auto* bindingsSection = new ExpandableSection("Keybindings", bindings());

    containerLayout->addWidget(quickstartSection);
    containerLayout->addWidget(faqSection);
    containerLayout->addWidget(syntaxSection);
    containerLayout->addWidget(specialSection);
    containerLayout->addWidget(bindingsSection);
    containerLayout->addStretch(1);

    // Scroll area wrapping the container
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(containerWidget);

    // Add the scroll area to the main dialog layout
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

    // Minimum size for the dialog
    setMinimumSize(500, 400);

    // Re-adjust dialog layout when any section expands/collapses
    connect(quickstartSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(faqSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(syntaxSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(specialSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(bindingsSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
}