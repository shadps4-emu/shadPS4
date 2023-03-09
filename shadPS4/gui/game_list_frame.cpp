#include "game_list_frame.h"
#include "gui_settings.h"


void game_list_frame::FixNarrowColumns() const
{
	qApp->processEvents();

	// handle columns (other than the icon column) that have zero width after showing them (stuck between others)
	for (int col = 1; col < m_columnActs.count(); ++col)
	{
		if (m_game_list->isColumnHidden(col))
		{
			continue;
		}

		if (m_game_list->columnWidth(col) <= m_game_list->horizontalHeader()->minimumSectionSize())
		{
			m_game_list->setColumnWidth(col, m_game_list->horizontalHeader()->minimumSectionSize());
		}
	}
}

void game_list_frame::ResizeColumnsToContents(int spacing) const
{
	if (!m_game_list)
	{
		return;
	}

	m_game_list->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	m_game_list->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

	// Make non-icon columns slighty bigger for better visuals
	for (int i = 1; i < m_game_list->columnCount(); i++)
	{
		if (m_game_list->isColumnHidden(i))
		{
			continue;
		}

		const int size = m_game_list->horizontalHeader()->sectionSize(i) + spacing;
		m_game_list->horizontalHeader()->resizeSection(i, size);
	}
}

void game_list_frame::SortGameList() const
{
	// Back-up old header sizes to handle unwanted column resize in case of zero search results
	QList<int> column_widths;
	const int old_row_count = m_game_list->rowCount();
	const int old_game_count = m_game_data.count();

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		column_widths.append(m_game_list->columnWidth(i));
	}

	// Sorting resizes hidden columns, so unhide them as a workaround
	QList<int> columns_to_hide;

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		if (m_game_list->isColumnHidden(i))
		{
			m_game_list->setColumnHidden(i, false);
			columns_to_hide << i;
		}
	}

	// Sort the list by column and sort order
	m_game_list->sortByColumn(m_sort_column, m_col_sort_order);

	// Hide columns again
	for (auto i : columns_to_hide)
	{
		m_game_list->setColumnHidden(i, true);
	}

	// Don't resize the columns if no game is shown to preserve the header settings
	if (!m_game_list->rowCount())
	{
		for (int i = 0; i < m_game_list->columnCount(); i++)
		{
			m_game_list->setColumnWidth(i, column_widths[i]);
		}

		m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);
		return;
	}

	// Fixate vertical header and row height
	m_game_list->verticalHeader()->setMinimumSectionSize(m_icon_size.height());
	m_game_list->verticalHeader()->setMaximumSectionSize(m_icon_size.height());
	m_game_list->resizeRowsToContents();

	// Resize columns if the game list was empty before
	if (!old_row_count && !old_game_count)
	{
		ResizeColumnsToContents();
	}
	else
	{
		m_game_list->resizeColumnToContents(gui::column_icon);
	}

	// Fixate icon column
	m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_list->resizeColumnToContents(gui::column_count - 1);
}