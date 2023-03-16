#pragma once

#include "game_list_table.h"
#include "custom_dock_widget.h"
#include "shadps4gui.h"
#include "game_list_grid.h"
#include "game_list_item.h"
#include <QHeaderView>
#include <QScrollbar>
#include <QStackedWidget>
#include <QWidget>
#include <deque>
#include <QFutureWatcher>
#include <QtConcurrent>

class game_list_frame : public custom_dock_widget
{
	Q_OBJECT
public :
	explicit game_list_frame(std::shared_ptr<gui_settings> gui_settings,QWidget* parent = nullptr);
	~game_list_frame();
	/** Fix columns with width smaller than the minimal section size */
	void FixNarrowColumns() const;

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

	/** Resizes the columns to their contents and adds a small spacing */
	void ResizeColumnsToContents(int spacing = 20) const;

	/** Refresh the gamelist with/without loading game data from files. Public so that main frame can refresh after vfs or install */
	void Refresh(const bool from_drive = false, const bool scroll_after = true);

	/** Repaint Gamelist Icons with new background color */
	void RepaintIcons(const bool& from_settings = false);

	/** Resize Gamelist Icons to size given by slider position */
	void ResizeIcons(const int& slider_pos);

public Q_SLOTS:
	void SetSearchText(const QString& text);
	void SetListMode(const bool& is_list);
private Q_SLOTS:
	void OnHeaderColumnClicked(int col);
	void OnRepaintFinished();
	void OnRefreshFinished();
Q_SIGNALS:
	void GameListFrameClosed();
	void RequestIconSizeChange(const int& val);
protected:
	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	QPixmap PaintedPixmap(const QPixmap& icon) const;
	void SortGameList() const;
	std::string CurrentSelectionPath();
	void PopulateGameList();

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_game_dock = nullptr;
	QStackedWidget* m_central_widget = nullptr;

	// Game Grid
	game_list_grid* m_game_grid = nullptr;

	// Game List
	game_list_table* m_game_list = nullptr;
	QList<QAction*> m_columnActs;
	Qt::SortOrder m_col_sort_order;
	int m_sort_column;

	// List Mode
	bool m_is_list_layout = true;
	bool m_old_layout_is_list = true;

	// data
	std::shared_ptr<gui_settings> m_gui_settings;
	QList<game_info> m_game_data;
	std::vector<std::string> m_path_list;
	std::deque<game_info> m_games;
	QFutureWatcher<game_list_item*> m_repaint_watcher;
	QFutureWatcher<void> m_refresh_watcher;

	// Search
	QString m_search_text;

	// Icon Size
	int m_icon_size_index = 0;

	// Icons
	QSize m_icon_size;
	QColor m_icon_color;
	qreal m_margin_factor;
	qreal m_text_factor;

};

