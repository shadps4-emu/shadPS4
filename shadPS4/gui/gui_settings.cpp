#include "gui_settings.h"


gui_settings::gui_settings(QObject* parent)
{
	m_settings.reset(new QSettings("shadps4.ini", QSettings::Format::IniFormat, parent));
}

