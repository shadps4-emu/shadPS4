#pragma once

#include <QMainWindow>
#include "ui_shadps4gui.h"
#include "GameListViewer.h"
#include "gui_settings.h"

class shadps4gui : public QMainWindow
{
	Q_OBJECT

public:
	shadps4gui(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
	~shadps4gui();

public slots:
	void installPKG();

private:
	Ui::shadps4guiClass ui;
	GameListViewer* game_list;
	std::shared_ptr<gui_settings> m_gui_settings;
};
