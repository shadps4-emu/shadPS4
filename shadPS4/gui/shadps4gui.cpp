#include "shadps4gui.h"
#include "../emulator/Loader.h"
#include "../emulator/fileFormat/PKG.h"
#include "../core/FsFile.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>

shadps4gui::shadps4gui(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QMainWindow(parent)
	, m_gui_settings(std::move(gui_settings))
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
		PKG pkg;
		pkg.open(file);
		//if pkg is ok we procced with extraction
		std::string failreason;
		QString gamedir = QDir::currentPath() + "/game/" + QString::fromStdString(pkg.getTitleID());
		QDir dir(gamedir);
		if (!dir.exists()) {
			dir.mkpath(".");
		}
		std::string extractpath = QDir::currentPath().toStdString() + "/game/" + pkg.getTitleID() + "/";
		if (!pkg.extract(file, extractpath, failreason))
		{
			QMessageBox::critical(this, "PKG ERROR", QString::fromStdString(failreason), QMessageBox::Ok, 0);
		}
		else
		{
			QMessageBox::information(this, "Extraction Finished", "Game successfully installed at " + gamedir, QMessageBox::Ok, 0);
			game_list->RefreshGameDirectory();//force refreshing since filelistwatcher doesn't work properly
		}
		
	}
	else
	{
		QMessageBox::critical(this, "PKG ERROR", "File doesn't appear to be a valid PKG file", QMessageBox::Ok, 0);
	}
}
