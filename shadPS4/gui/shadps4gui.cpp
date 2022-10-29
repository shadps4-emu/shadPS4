#include "shadps4gui.h"
#include "../emulator/Loader.h"
#include <QFileDialog>
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
	std::string file(QFileDialog::getOpenFileName(this, tr("Install PKG File"), QDir::currentPath(), tr("PKG File (*.PKG)")).toStdString());
	if (detectFileType(file) == FILETYPE_PKG)
	{

	}
	else
	{
		QMessageBox::critical(this, "PKG ERROR", "File doesn't appear to be a valid PKG file", QMessageBox::Ok, 0);
	}
}
