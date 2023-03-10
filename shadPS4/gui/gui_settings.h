#pragma once

#include "settings.h"
#include <QColor>

namespace gui
{
	enum custom_roles
	{
		game_role = Qt::UserRole + 1337,
	};

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

	const QSize game_list_icon_size_min = QSize(40, 22);
	const QSize game_list_icon_size_small = QSize(80, 44);
	const QSize game_list_icon_size_medium = QSize(160, 88);
	const QSize game_list_icon_size_max = QSize(320, 176);

	const QString game_list = "GameList";

	const QColor game_list_icon_color = QColor(240, 240, 240, 255);

	const gui_save game_list_sortAsc = gui_save(game_list, "sortAsc", true);
	const gui_save game_list_sortCol = gui_save(game_list, "sortCol", 1);
	const gui_save game_list_state = gui_save(game_list, "state", QByteArray());
	const gui_save game_list_iconColor = gui_save(game_list, "iconColor", game_list_icon_color);
	const gui_save game_list_listMode = gui_save(game_list, "listMode", true);
	const gui_save game_list_textFactor = gui_save(game_list, "textFactor", qreal{ 2.0 });
	const gui_save game_list_marginFactor = gui_save(game_list, "marginFactor", qreal{ 0.09 });


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

