#include "gui/shadps4gui.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	shadps4gui w;
	w.show();
	return a.exec();
}