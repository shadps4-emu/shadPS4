#pragma once

#include <QString>
#include <QVariant>

struct gui_save
{
	QString key;
	QString name;
	QVariant def;

	gui_save()
	{
		key = "";
		name = "";
		def = QVariant();
	}

	gui_save(const QString& k, const QString& n, const QVariant& d)
	{
		key = k;
		name = n;
		def = d;
	}

	bool operator==(const gui_save& rhs) const noexcept
	{
		return key == rhs.key && name == rhs.name && def == rhs.def;
	}
};
