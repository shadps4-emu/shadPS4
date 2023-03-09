#pragma once

#include "game_list_table.h"
#include "shadps4gui.h"

#include <QHeaderView>

class game_list_frame
{
public :
	/** Fix columns with width smaller than the minimal section size */
	void FixNarrowColumns() const;

	/** Resizes the columns to their contents and adds a small spacing */
	void ResizeColumnsToContents(int spacing = 20) const;

private:
	void SortGameList() const;

	// Game List
	game_list_table* m_game_list = nullptr;
	QList<QAction*> m_columnActs;
	Qt::SortOrder m_col_sort_order;
	int m_sort_column;

	// data
	QList<game_info> m_game_data;

	// Icons
	QSize m_icon_size;

};

