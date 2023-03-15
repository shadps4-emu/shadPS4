#include "game_list_frame.h"
#include "main_window.h"
#include "gui_settings.h"
#include "ui_main_window.h"

main_window::main_window(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::main_window)
{

	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
}

main_window::~main_window()
{

}

bool main_window::Init()
{
	// add toolbar widgets
	ui->toolBar->setObjectName("mw_toolbar");
	ui->sizeSlider->setRange(0, gui::game_list_max_slider_pos);
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addWidget(ui->mw_searchbar);

	CreateActions();
	CreateDockWindows();

	return true;
}

void main_window::CreateActions()
{
	//create action group for icon size
	m_icon_size_act_group = new QActionGroup(this);
	m_icon_size_act_group->addAction(ui->setIconSizeTinyAct);
	m_icon_size_act_group->addAction(ui->setIconSizeSmallAct);
	m_icon_size_act_group->addAction(ui->setIconSizeMediumAct);
	m_icon_size_act_group->addAction(ui->setIconSizeLargeAct);

	//create action group for list mode
	m_list_mode_act_group = new QActionGroup(this);
	m_list_mode_act_group->addAction(ui->setlistModeListAct);
	m_list_mode_act_group->addAction(ui->setlistModeGridAct);

}

void main_window::CreateDockWindows()
{
	m_main_window = new QMainWindow();
	m_main_window->setContextMenuPolicy(Qt::PreventContextMenu);

	m_game_list_frame = new game_list_frame(m_gui_settings,m_main_window);
	m_game_list_frame->setObjectName("gamelist");

	m_main_window->addDockWidget(Qt::LeftDockWidgetArea, m_game_list_frame);

	m_main_window->setDockNestingEnabled(true);

	setCentralWidget(m_main_window);

	connect(m_game_list_frame, &game_list_frame::GameListFrameClosed, this, [this]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
				m_gui_settings->SetValue(gui::main_window_gamelist_visible, false);
		}
	});
}
