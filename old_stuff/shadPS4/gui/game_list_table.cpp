#include "game_list_table.h"

void game_list_table ::clear_list()
{
	clearSelection();
	clearContents();
}

void game_list_table::mousePressEvent(QMouseEvent* event)
{
	if (QTableWidgetItem* item = itemAt(event->pos()); !item || !item->data(Qt::UserRole).isValid())
	{
		clearSelection();
		setCurrentItem(nullptr); // Needed for currentItemChanged
	}
	QTableWidget::mousePressEvent(event);
}