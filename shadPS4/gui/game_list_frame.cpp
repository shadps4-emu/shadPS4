#include "game_list_frame.h"
#include "gui_settings.h"

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> gui_settings,QWidget* parent)
	: QWidget(parent)
	, m_gui_settings(std::move(gui_settings))
{
	m_game_list = new game_list_table();
	m_game_list->setShowGrid(false);
	m_game_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_game_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_game_list->setSelectionMode(QAbstractItemView::SingleSelection);
	m_game_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_list->verticalScrollBar()->installEventFilter(this);
	m_game_list->verticalScrollBar()->setSingleStep(20);
	m_game_list->horizontalScrollBar()->setSingleStep(20);
	m_game_list->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_game_list->verticalHeader()->setVisible(false);
	m_game_list->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_list->horizontalHeader()->setHighlightSections(false);
	m_game_list->horizontalHeader()->setSortIndicatorShown(true);
	m_game_list->horizontalHeader()->setStretchLastSection(true);
	m_game_list->horizontalHeader()->setDefaultSectionSize(150);
	m_game_list->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_game_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_list->setAlternatingRowColors(true);
	m_game_list->installEventFilter(this);
	m_game_list->setColumnCount(gui::column_count);

	//temp code
	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	QSpacerItem* item = new QSpacerItem(100, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
	layout->addWidget(m_game_list);
	setLayout(layout);
	//endof temp code

	// Actions regarding showing/hiding columns
	auto add_column = [this](gui::game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_game_list->setHorizontalHeaderItem(col, new QTableWidgetItem(header_text));
		m_columnActs.append(new QAction(action_text, this));
	};

	add_column(gui::column_icon, tr("Icon"), tr("Show Icons"));
	add_column(gui::column_name, tr("Name"), tr("Show Names"));
	add_column(gui::column_serial, tr("Serial"), tr("Show Serials"));
	add_column(gui::column_firmware, tr("Firmware"), tr("Show Firmwares"));
	add_column(gui::column_version, tr("Version"), tr("Show Versions"));
	add_column(gui::column_category, tr("Category"), tr("Show Categories"));
	add_column(gui::column_path, tr("Path"), tr("Show Paths"));

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_columnActs[col]->setCheckable(true);

		connect(m_columnActs[col], &QAction::triggered, this, [this, col](bool checked)
			{
				if (!checked) // be sure to have at least one column left so you can call the context menu at all time
				{
					int c = 0;
					for (int i = 0; i < m_columnActs.count(); ++i)
					{
						if (m_gui_settings->GetGamelistColVisibility(i) && ++c > 1)
							break;
					}
					if (c < 2)
					{
						m_columnActs[col]->setChecked(true); // re-enable the checkbox if we don't change the actual state
						return;
					}
				}
				m_game_list->setColumnHidden(col, !checked); // Negate because it's a set col hidden and we have menu say show.
				m_gui_settings->SetGamelistColVisibility(col, checked);

				if (checked) // handle hidden columns that have zero width after showing them (stuck between others)
				{
					FixNarrowColumns();
				}
			});
	}

	//events
	connect(m_game_list->horizontalHeader(), &QHeaderView::customContextMenuRequested, this, [this](const QPoint& pos)
		{
			QMenu* configure = new QMenu(this);
			configure->addActions(m_columnActs);
			configure->exec(m_game_list->horizontalHeader()->viewport()->mapToGlobal(pos));
		});
}
game_list_frame::~game_list_frame(){
	SaveSettings();
}
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

void game_list_frame::SortGameList() const
{
	// Back-up old header sizes to handle unwanted column resize in case of zero search results
	QList<int> column_widths;
	const int old_row_count = m_game_list->rowCount();
	const int old_game_count = m_game_data.count();

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		column_widths.append(m_game_list->columnWidth(i));
	}

	// Sorting resizes hidden columns, so unhide them as a workaround
	QList<int> columns_to_hide;

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		if (m_game_list->isColumnHidden(i))
		{
			m_game_list->setColumnHidden(i, false);
			columns_to_hide << i;
		}
	}

	// Sort the list by column and sort order
	m_game_list->sortByColumn(m_sort_column, m_col_sort_order);

	// Hide columns again
	for (auto i : columns_to_hide)
	{
		m_game_list->setColumnHidden(i, true);
	}

	// Don't resize the columns if no game is shown to preserve the header settings
	if (!m_game_list->rowCount())
	{
		for (int i = 0; i < m_game_list->columnCount(); i++)
		{
			m_game_list->setColumnWidth(i, column_widths[i]);
		}

		m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);
		return;
	}

	// Fixate vertical header and row height
	m_game_list->verticalHeader()->setMinimumSectionSize(m_icon_size.height());
	m_game_list->verticalHeader()->setMaximumSectionSize(m_icon_size.height());
	m_game_list->resizeRowsToContents();

	// Resize columns if the game list was empty before
	if (!old_row_count && !old_game_count)
	{
		ResizeColumnsToContents();
	}
	else
	{
		m_game_list->resizeColumnToContents(gui::column_icon);
	}

	// Fixate icon column
	m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_list->resizeColumnToContents(gui::column_count - 1);
}
void game_list_frame::LoadSettings()
{
	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		const bool vis = m_gui_settings->GetGamelistColVisibility(col);
		m_columnActs[col]->setChecked(vis);
		m_game_list->setColumnHidden(col, !vis);
	}
	FixNarrowColumns();

	m_game_list->horizontalHeader()->restoreState(m_game_list->horizontalHeader()->saveState());

}
void game_list_frame::SaveSettings()
{
	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_gui_settings->SetGamelistColVisibility(col, m_columnActs[col]->isChecked());
	}
}