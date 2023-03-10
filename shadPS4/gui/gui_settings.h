#pragma once

#include <QSettings>

namespace gui
{
	enum game_list_columns
	{
		column_icon,
		column_name,
		column_serial,
		column_firmware,
		column_version,
		column_category,
		column_path,

		column_count
	};

}

class gui_settings
{

public:
	explicit gui_settings(QObject* parent = nullptr);

private:
	std::unique_ptr<QSettings> m_settings;
};

