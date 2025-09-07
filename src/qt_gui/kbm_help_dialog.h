// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

class ExpandableSection : public QWidget {
    Q_OBJECT
public:
    explicit ExpandableSection(const QString& title, const QString& content, QWidget* parent);

signals:
    void expandedChanged(); // Signal to indicate layout size change

private:
    QPushButton* toggleButton;
    QTextBrowser* contentBrowser; // Changed from QLabel to QTextBrowser
    QPropertyAnimation* animation;
    int contentHeight;
    void updateContentHeight() {
        int contentHeight = contentBrowser->document()->size().height();
        contentBrowser->setMinimumHeight(contentHeight + 5);
        contentBrowser->setMaximumHeight(contentHeight + 50);
    }
};

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(bool* open_flag = nullptr, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void reject() override;

private:
    bool* help_open_ptr;

    QString quickstart();
    QString faq();
    QString syntax();
    QString bindings();
    QString special();
};