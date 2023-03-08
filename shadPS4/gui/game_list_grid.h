#pragma once

#include "game_list_table.h"

class game_list_grid_delegate;

class game_list_grid : public game_list_table
{
	Q_OBJECT

		QSize m_icon_size;
	QColor m_icon_color;
	qreal m_margin_factor;
	qreal m_text_factor;
	bool m_text_enabled = true;

public:
	explicit game_list_grid(const QSize& icon_size, QColor icon_color, const qreal& margin_factor, const qreal& text_factor, const bool& showText);

	void enableText(const bool& enabled);
	void setIconSize(const QSize& size) const;
	QTableWidgetItem* addItem(const game_info& app, const QString& name, const QString& movie_path, const int& row, const int& col);

	[[nodiscard]] qreal getMarginFactor() const;

private:
	game_list_grid_delegate* grid_item_delegate;
};

