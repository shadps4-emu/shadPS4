#pragma once

#include <QFutureWatcher>

namespace gui
{
	namespace utils
	{
		template <typename T>
		void stop_future_watcher(QFutureWatcher<T>& watcher, bool cancel)
		{
			if (watcher.isStarted() || watcher.isRunning())
			{
				if (cancel)
				{
					watcher.cancel();
				}
				watcher.waitForFinished();
			}
		}
	} // utils
} // gui

