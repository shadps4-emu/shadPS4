#include "game_list_frame.h"



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