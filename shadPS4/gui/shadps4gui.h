#pragma once

#include <QMainWindow>
#include "ui_shadps4gui.h"

class shadps4gui : public QMainWindow
{
	Q_OBJECT

public:
	shadps4gui(QWidget *parent = nullptr);
	~shadps4gui();

private:
	Ui::shadps4guiClass ui;
};
