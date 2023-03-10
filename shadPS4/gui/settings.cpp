#include "settings.h"

settings::settings(QObject* parent) : QObject(parent),
m_settings_dir(ComputeSettingsDir())
{
}

settings::~settings()
{
	if (m_settings)
	{
		m_settings->sync();
	}
}

QString settings::GetSettingsDir() const
{
	return m_settings_dir.absolutePath();
}

QString settings::ComputeSettingsDir()
{
	return ""; //TODO currently we configure same dir , make it configurable
}

void settings::RemoveValue(const QString& key, const QString& name) const
{
	if (m_settings)
	{
		m_settings->beginGroup(key);
		m_settings->remove(name);
		m_settings->endGroup();
	}
}

void settings::RemoveValue(const gui_save& entry) const
{
	RemoveValue(entry.key, entry.name);
}

QVariant settings::GetValue(const QString& key, const QString& name, const QVariant& def) const
{
	return m_settings ? m_settings->value(key + "/" + name, def) : def;
}

QVariant settings::GetValue(const gui_save& entry) const
{
	return GetValue(entry.key, entry.name, entry.def);
}

QVariant settings::List2Var(const q_pair_list& list)
{
	QByteArray ba;
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << list;
	return QVariant(ba);
}

q_pair_list settings::Var2List(const QVariant& var)
{
	q_pair_list list;
	QByteArray ba = var.toByteArray();
	QDataStream stream(&ba, QIODevice::ReadOnly);
	stream >> list;
	return list;
}

void settings::SetValue(const gui_save& entry, const QVariant& value) const
{
	if (m_settings)
	{
		m_settings->beginGroup(entry.key);
		m_settings->setValue(entry.name, value);
		m_settings->endGroup();
	}
}

void settings::SetValue(const QString& key, const QVariant& value) const
{
	if (m_settings)
	{
		m_settings->setValue(key, value);
	}
}

void settings::SetValue(const QString& key, const QString& name, const QVariant& value) const
{
	if (m_settings)
	{
		m_settings->beginGroup(key);
		m_settings->setValue(name, value);
		m_settings->endGroup();
	}
}
