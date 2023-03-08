#pragma once

#include <QTableWidget>
#include <QMouseEvent>
#include "../emulator/gameInfo.h"

struct gui_game_info
{
	GameInfo info{};
};

typedef std::shared_ptr<gui_game_info> game_info;
Q_DECLARE_METATYPE(game_info)

class game_list_table : public QTableWidget
{
public:
	void clear_list();

protected:
	void mousePressEvent(QMouseEvent* event) override;
};