#pragma once

#include <QMainWindow>
#include <QActionGroup>

class gui_settings;
class game_list_frame;

namespace Ui
{
	class main_window;
}

class main_window : public QMainWindow
{
	Q_OBJECT

	std::unique_ptr<Ui::main_window> ui;

	bool m_is_list_mode = true;
	bool m_save_slider_pos = false;
	int m_other_slider_pos = 0;

public:
	explicit main_window(std::shared_ptr<gui_settings> gui_settings,QWidget* parent = nullptr);
	~main_window();
	bool Init();
	void InstallPkg();

private Q_SLOTS:
	void ConfigureGuiFromSettings();
	void SetIconSizeActions(int idx) const;
	void ResizeIcons(int index);
	void SaveWindowState() const;
private:
	void CreateActions();
	void CreateDockWindows();
	void CreateConnects();

	QActionGroup* m_icon_size_act_group = nullptr;
	QActionGroup* m_list_mode_act_group = nullptr;

	// Dockable widget frames
	QMainWindow* m_main_window = nullptr;
	game_list_frame* m_game_list_frame = nullptr;

	std::shared_ptr<gui_settings> m_gui_settings;

};

