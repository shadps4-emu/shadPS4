#include "gui_settings.h"


gui_settings::gui_settings(QObject* parent)
{
	m_settings.reset(new QSettings("shadps4.ini", QSettings::Format::IniFormat, parent)); //TODO make the path configurable
}

void gui_settings::SetGamelistColVisibility(int col, bool val) const
{
	SetValue(GetGuiSaveForColumn(col), val);
}

bool gui_settings::GetGamelistColVisibility(int col) const
{
	return GetValue(GetGuiSaveForColumn(col)).toBool();
}

gui_save gui_settings::GetGuiSaveForColumn(int col)
{
	return gui_save{ gui::game_list, "visibility_" + gui::get_game_list_column_name(static_cast<gui::game_list_columns>(col)), true };
}