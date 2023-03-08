#include "game_list_grid_delegate.h"

game_list_grid_delegate::game_list_grid_delegate(const QSize& size, const qreal& margin_factor, const qreal& text_factor, QObject* parent)
	: QStyledItemDelegate(parent), m_size(size), m_margin_factor(margin_factor), m_text_factor(text_factor)
{
}

void game_list_grid_delegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
	Q_UNUSED(index)

		// Remove the focus frame around selected items
		option->state &= ~QStyle::State_HasFocus;

	// Call initStyleOption without a model index, since we want to paint the relevant data ourselves
	QStyledItemDelegate::initStyleOption(option, QModelIndex());
}

void game_list_grid_delegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	const QRect r = option.rect;

	painter->setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	painter->eraseRect(r);

	// Get title and image
	const QPixmap image = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
	const QString title = index.data(Qt::DisplayRole).toString();

	// Paint from our stylesheet
	QStyledItemDelegate::paint(painter, option, index);

	// image
	if (image.isNull() == false)
	{
		painter->drawPixmap(option.rect, image);
	}

	const int h = r.height() / (1 + m_margin_factor + m_margin_factor * m_text_factor);
	const int height = r.height() - h - h * m_margin_factor;
	const int top = r.bottom() - height;

	// title
	if (option.state & QStyle::State_Selected)
	{
		painter->setPen(QPen(option.palette.color(QPalette::HighlightedText), 1, Qt::SolidLine));
	}
	else
	{
		painter->setPen(QPen(option.palette.color(QPalette::WindowText), 1, Qt::SolidLine));
	}

	painter->setFont(option.font);
	painter->drawText(QRect(r.left(), top, r.width(), height), +Qt::TextWordWrap | +Qt::AlignCenter, title);
}

QSize game_list_grid_delegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option)
		Q_UNUSED(index)
		return m_size;
}

void game_list_grid_delegate::setItemSize(const QSize& size)
{
	m_size = size;
}
