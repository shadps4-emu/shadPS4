#pragma once

#include "settings.h"

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

	inline QString get_game_list_column_name(game_list_columns col)
	{
		switch (col)
		{
		case column_icon:
			return "column_icon";
		case column_name:
			return "column_name";
		case column_serial:
			return "column_serial";
		case column_firmware:
			return "column_firmware";
		case column_version:
			return "column_version";
		case column_category:
			return "column_category";
		case column_path:
			return "column_path";
		case column_count:
			return "";
		}

		throw std::exception("get_game_list_column_name: Invalid column");
	}

	const QString game_list = "GameList";

	const gui_save game_list_sortAsc = gui_save(game_list, "sortAsc", true);
	const gui_save game_list_sortCol = gui_save(game_list, "sortCol", 1);
	const gui_save game_list_state = gui_save(game_list, "state", QByteArray());

}

class gui_settings : public settings
{
	Q_OBJECT

public:
	explicit gui_settings(QObject* parent = nullptr);

	bool GetGamelistColVisibility(int col) const;


public Q_SLOTS:
	void SetGamelistColVisibility(int col, bool val) const;
	static gui_save GetGuiSaveForColumn(int col);
};

