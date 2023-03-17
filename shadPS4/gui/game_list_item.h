#pragma once

#include <QTableWidgetItem>
#include <QObject>

#include <functional>

using icon_callback_t = std::function<void(int)>;

class game_list_item : public QTableWidgetItem
{
public:
	game_list_item() : QTableWidgetItem()
	{
	}
	game_list_item(const QString& text, int type = Type) : QTableWidgetItem(text, type)
	{
	}
	game_list_item(const QIcon& icon, const QString& text, int type = Type) : QTableWidgetItem(icon, text, type)
	{
	}

	~game_list_item()
	{

	}

	void call_icon_func() const
	{
		if (m_icon_callback)
		{
			m_icon_callback(0);
		}
	}

	void set_icon_func(const icon_callback_t& func)
	{
		m_icon_callback = func;
		call_icon_func();
	}

private:
	icon_callback_t m_icon_callback = nullptr;
};
