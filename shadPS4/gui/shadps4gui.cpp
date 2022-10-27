#include "shadps4gui.h"
#include <QDir>
#include <QMessageBox>
shadps4gui::shadps4gui(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	game_list = new GameListViewer();
	game_list->SetGamePath(QDir::currentPath() + "/game/");
	ui.horizontalLayout->addWidget(game_list);
	show();
	game_list->PopulateAsync();
}

shadps4gui::~shadps4gui()
{}

void shadps4gui::installPKG()
{
	QMessageBox::critical(this, "PKG ERROR", "Not yet", QMessageBox::Ok, 0);
}
