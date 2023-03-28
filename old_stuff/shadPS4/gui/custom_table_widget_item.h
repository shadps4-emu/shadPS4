#pragma once
#include "game_list_item.h"
#include <QTableWidgetItem>

class custom_table_widget_item : public game_list_item
{
private:
	int m_sort_role = Qt::DisplayRole;

public:
	using QTableWidgetItem::setData;

	custom_table_widget_item() = default;
	custom_table_widget_item(const std::string& text, int sort_role = Qt::DisplayRole, const QVariant& sort_value = 0);
	custom_table_widget_item(const QString& text, int sort_role = Qt::DisplayRole, const QVariant& sort_value = 0);

	bool operator<(const QTableWidgetItem& other) const override;

	void setData(int role, const QVariant& value, bool assign_sort_role);
};
