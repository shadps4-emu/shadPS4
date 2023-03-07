#include "custom_table_widget_item.h"

#include <QDateTime>

custom_table_widget_item::custom_table_widget_item(const std::string& text, int sort_role, const QVariant& sort_value)
	: QTableWidgetItem(QString::fromStdString(text).simplified()) // simplified() forces single line text
{
	if (sort_role != Qt::DisplayRole)
	{
		setData(sort_role, sort_value, true);
	}
}

custom_table_widget_item::custom_table_widget_item(const QString& text, int sort_role, const QVariant& sort_value)
	: QTableWidgetItem(text.simplified()) // simplified() forces single line text
{
	if (sort_role != Qt::DisplayRole)
	{
		setData(sort_role, sort_value, true);
	}
}

bool custom_table_widget_item::operator<(const QTableWidgetItem& other) const
{
	if (m_sort_role == Qt::DisplayRole)
	{
		return QTableWidgetItem::operator<(other);
	}

	const QVariant data_l = data(m_sort_role);
	const QVariant data_r = other.data(m_sort_role);
	const QVariant::Type type_l = data_l.type();
	const QVariant::Type type_r = data_r.type();

	switch (type_l)
	{
	case QVariant::Type::Bool:
	case QVariant::Type::Int:
		return data_l.toInt() < data_r.toInt();
	case QVariant::Type::UInt:
		return data_l.toUInt() < data_r.toUInt();
	case QVariant::Type::LongLong:
		return data_l.toLongLong() < data_r.toLongLong();
	case QVariant::Type::ULongLong:
		return data_l.toULongLong() < data_r.toULongLong();
	case QVariant::Type::Double:
		return data_l.toDouble() < data_r.toDouble();
	case QVariant::Type::Date:
		return data_l.toDate() < data_r.toDate();
	case QVariant::Type::Time:
		return data_l.toTime() < data_r.toTime();
	case QVariant::Type::DateTime:
		return data_l.toDateTime() < data_r.toDateTime();
	case QVariant::Type::Char:
	case QVariant::Type::String:
		return data_l.toString() < data_r.toString();
	default:
		throw std::runtime_error("unsupported type");
	}
}

void custom_table_widget_item::setData(int role, const QVariant& value, bool assign_sort_role)
{
	if (assign_sort_role)
	{
		m_sort_role = role;
	}
	QTableWidgetItem::setData(role, value);
}

