#include "gui/shadps4gui.h"
#include <QtWidgets/QApplication>
#include "gui/gui_settings.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	std::shared_ptr<gui_settings> m_gui_settings;
	m_gui_settings.reset(new gui_settings());
	shadps4gui w(m_gui_settings,nullptr);
	w.show();
	return a.exec();
}