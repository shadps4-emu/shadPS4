#pragma once

#include <QMainWindow>
#include "ui_shadps4gui.h"
#include "GameListViewer.h"

class shadps4gui : public QMainWindow
{
	Q_OBJECT

public:
	shadps4gui(QWidget *parent = nullptr);
	~shadps4gui();

public slots:
	void installPKG();

private:
	Ui::shadps4guiClass ui;
	GameListViewer* game_list;
};
