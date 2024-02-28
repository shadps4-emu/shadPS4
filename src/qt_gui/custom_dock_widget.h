// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDockWidget>
#include <QPainter>
#include <QStyleOption>

class CustomDockWidget : public QDockWidget {
private:
    std::shared_ptr<QWidget> m_title_bar_widget;
    bool m_is_title_bar_visible = true;

public:
    explicit CustomDockWidget(const QString& title, QWidget* parent = Q_NULLPTR,
                              Qt::WindowFlags flags = Qt::WindowFlags())
        : QDockWidget(title, parent, flags) {
        m_title_bar_widget.reset(titleBarWidget());

        connect(this, &QDockWidget::topLevelChanged, [this](bool /* topLevel*/) {
            SetTitleBarVisible(m_is_title_bar_visible);
            style()->unpolish(this);
            style()->polish(this);
        });
    }

    void SetTitleBarVisible(bool visible) {
        if (visible || isFloating()) {
            if (m_title_bar_widget.get() != titleBarWidget()) {
                setTitleBarWidget(m_title_bar_widget.get());
                QMargins margins = widget()->contentsMargins();
                margins.setTop(0);
                widget()->setContentsMargins(margins);
            }
        } else {
            setTitleBarWidget(new QWidget());
            QMargins margins = widget()->contentsMargins();
            margins.setTop(1);
            widget()->setContentsMargins(margins);
        }

        m_is_title_bar_visible = visible;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        // We need to repaint the dock widgets as plain widgets in floating mode.
        // Source:
        // https://stackoverflow.com/questions/10272091/cannot-add-a-background-image-to-a-qdockwidget
        if (isFloating()) {
            QStyleOption opt;
            opt.initFrom(this);
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
            return;
        }

        // Use inherited method for docked mode because otherwise the dock would lose the title etc.
        QDockWidget::paintEvent(event);
    }
};
