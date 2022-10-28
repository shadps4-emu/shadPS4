#include "GameListViewer.h"
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QThreadPool>
#include <QVBoxLayout>
#include <QDirIterator>
#include <QLineEdit>
#include <QRegularExpression>
#include "../emulator/fileFormat/PSF.h"

GameListViewer::GameListViewer(QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout;
	QHBoxLayout* search_layout = new QHBoxLayout;
	proxyModel = new QSortFilterProxyModel;
	search_games = new QLineEdit;

	tree_view = new QTreeView;
	item_model = new QStandardItemModel(tree_view);
	proxyModel->setSourceModel(item_model);
	tree_view->setModel(proxyModel);

	tree_view->setAlternatingRowColors(true);
	tree_view->setSelectionMode(QHeaderView::SingleSelection);
	tree_view->setSelectionBehavior(QHeaderView::SelectRows);
	tree_view->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
	tree_view->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
	tree_view->setSortingEnabled(true);
	tree_view->setEditTriggers(QHeaderView::NoEditTriggers);
	tree_view->setUniformRowHeights(true);
	tree_view->setContextMenuPolicy(Qt::CustomContextMenu);

	item_model->insertColumns(0, 7);
	item_model->setHeaderData(0, Qt::Horizontal, "Icon");
	item_model->setHeaderData(1, Qt::Horizontal, "Name");
	item_model->setHeaderData(2, Qt::Horizontal, "Serial");
	item_model->setHeaderData(3, Qt::Horizontal, "FW");
	item_model->setHeaderData(4, Qt::Horizontal, "App Version");
	item_model->setHeaderData(5, Qt::Horizontal, "Category");
	item_model->setHeaderData(6, Qt::Horizontal, "Path");

	connect(tree_view, &QTreeView::activated, this, &GameListViewer::ValidateEntry);
	connect(search_games, &QLineEdit::textChanged, this, &GameListViewer::searchGame);
	connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &GameListViewer::RefreshGameDirectory);

	// We must register all custom types with the Qt Automoc system so that we are able to use it
	// with signals/slots. In this case, QList falls under the umbrells of custom types.
	qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");

	layout->setContentsMargins(0, 0, 0, 0);
	QSpacerItem* item = new QSpacerItem(100, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
	search_layout->setContentsMargins(0, 5, 0, 0);
	search_layout->addSpacerItem(item);
	search_layout->addWidget(search_games);
	layout->addLayout(search_layout);
	layout->addWidget(tree_view);
	setLayout(layout);
}

GameListViewer::~GameListViewer()
{
	emit ShouldCancelWorker();
}
void GameListViewer::searchGame(QString searchText)
{
	proxyModel->setFilterKeyColumn(1); //filter Name column only
	QString strPattern = searchText;
	QRegularExpression  regExp(strPattern, QRegularExpression::CaseInsensitiveOption);
	proxyModel->setFilterRegularExpression(regExp);
}
void GameListViewer::AddEntry(const QList<QStandardItem*>& entry_items) {
	item_model->invisibleRootItem()->appendRow(entry_items);
}

void GameListViewer::ValidateEntry(const QModelIndex& item) {
	// We don't care about the individual QStandardItem that was selected, but its row.
}

void GameListViewer::DonePopulating() {
	tree_view->setEnabled(true);
	tree_view->resizeColumnToContents(1);//resize tittle to fit the column
}

void GameListViewer::PopulateAsync() {
	QDir game_folder(game_path);
	if (!game_folder.exists())
	{
		//game directory doesn't exist
		return;
	}

	tree_view->setEnabled(false);
	// Delete any rows that might already exist if we're repopulating
	item_model->removeRows(0, item_model->rowCount());

	emit ShouldCancelWorker();

	auto watch_dirs = watcher.directories();
	if (!watch_dirs.isEmpty()) {
		watcher.removePaths(watch_dirs);
	}
	UpdateWatcherList(game_path.toStdString());
	GameListWorker* worker = new GameListWorker(game_path);

	connect(worker, &GameListWorker::EntryReady, this, &GameListViewer::AddEntry, Qt::QueuedConnection);
	connect(worker, &GameListWorker::Finished, this, &GameListViewer::DonePopulating, Qt::QueuedConnection);
	// Use DirectConnection here because worker->Cancel() is thread-safe and we want it to cancel without delay.
	connect(this, &GameListViewer::ShouldCancelWorker, worker, &GameListWorker::Cancel, Qt::DirectConnection);

	QThreadPool::globalInstance()->start(worker);
	current_worker = std::move(worker);
}

void GameListViewer::RefreshGameDirectory() {
	QDir game_folder(game_path);
	bool empty = game_folder.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::DirsFirst).count() == 0;
	if (!empty && current_worker != nullptr) {
		//Change detected in the games directory. Reloading game list
		PopulateAsync();
	}
}

/**
* Adds the game list folder to the QFileSystemWatcher to check for updates.
*
* The file watcher will fire off an update to the game list when a change is detected in the game
* list folder.
*
* Notice: This method is run on the UI thread because QFileSystemWatcher is not thread safe and
* this function is fast enough to not stall the UI thread. If performance is an issue, it should
* be moved to another thread and properly locked to prevent concurrency issues.
*
* @param dir folder to check for changes in
*/
void GameListViewer::UpdateWatcherList(const std::string& dir) {
	/*watcher.addPath(QString::fromStdString(dir));
	QDir parent_folder(QString::fromStdString(dir));
	QFileInfoList fList = parent_folder.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
	foreach(QFileInfo item, fList)
	{
		UpdateWatcherList(item.absoluteFilePath().toStdString());
	}*/
}

void GameListWorker::AddEntriesToGameList(const std::string& dir_path) {
	QDir parent_folder(QString::fromStdString(dir_path));
	QFileInfoList fList = parent_folder.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
	foreach(QFileInfo item, fList)
	{
		PSF psf;
		if (!psf.open(item.absoluteFilePath().toStdString() + "/PARAM.SFO"))
			continue;//if we can't open param.sfo go to the next entry

		//TODO std::string test = psf.get_string("TITLE_ID");
		QString iconpath(item.absoluteFilePath() + "/ICON0.PNG");

		emit EntryReady({
			new GameIconItem(iconpath),
			new GameListItem(QString::fromStdString(psf.get_string("TITLE"))),
			new GameListItem(QString::fromStdString(psf.get_string("TITLE_ID"))),
			new GameListItem(QString("%1").arg(psf.get_integer("SYSTEM_VER"), 8, 16, QLatin1Char('0'))),
			new GameListItem(QString::fromStdString(psf.get_string("APP_VER"))),
			new GameListItem(QString::fromStdString(psf.get_string("CATEGORY"))),
			new GameListItem(item.fileName())
			});

	}
}

void GameListWorker::run() {
	stop_processing = false;
	AddEntriesToGameList(dir_path.toStdString());
	emit Finished();
}

void GameListWorker::Cancel() {
	this->disconnect();
	stop_processing = true;
}