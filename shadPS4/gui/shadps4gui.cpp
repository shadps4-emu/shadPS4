#include "shadps4gui.h"
#include <QMessageBox>
shadps4gui::shadps4gui(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}

shadps4gui::~shadps4gui()
{}

void shadps4gui::installPKG()
{
	QMessageBox::critical(this, "PKG ERROR", "Not yet", QMessageBox::Ok, 0);
}
