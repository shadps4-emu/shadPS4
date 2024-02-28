// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFutureWatcher>

namespace gui {
namespace utils {
template <typename T>
void stop_future_watcher(QFutureWatcher<T>& watcher, bool cancel) {
    if (watcher.isStarted() || watcher.isRunning()) {
        if (cancel) {
            watcher.cancel();
        }
        watcher.waitForFinished();
    }
}
} // namespace utils
} // namespace gui
