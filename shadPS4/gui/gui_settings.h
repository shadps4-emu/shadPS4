#pragma once

#include <QSettings>

class gui_settings
{

public:
	explicit gui_settings(QObject* parent = nullptr);

private:
	std::unique_ptr<QSettings> m_settings;
};

