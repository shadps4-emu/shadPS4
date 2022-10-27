#pragma once
#include <QFileSystemWatcher>
#include <QModelIndex>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>
#include <QWidget>
#include <atomic>
#include <QImage>
#include <QRunnable>
#include <QStandardItem>
#include <QString>
#include <QSortFilterProxyModel>

class GameListWorker;

class GameListViewer : public QWidget
{
	Q_OBJECT

public:
	explicit GameListViewer(QWidget* parent = nullptr);
	~GameListViewer();
	void PopulateAsync();
	void SetGamePath(QString game_path)
	{
		this->game_path = game_path;
	}
signals:
	void ShouldCancelWorker();

private:
	void AddEntry(const QList<QStandardItem*>& entry_items);
	void ValidateEntry(const QModelIndex& item);
	void DonePopulating();
	void UpdateWatcherList(const std::string& path);

	QTreeView* tree_view = nullptr;
	QStandardItemModel* item_model = nullptr;
	GameListWorker* current_worker = nullptr;
	QLineEdit* search_games = nullptr;
	QSortFilterProxyModel* proxyModel = nullptr;
	QFileSystemWatcher watcher;
	QString game_path;
public:
	void RefreshGameDirectory();

public slots:
	void searchGame(QString searchText);
};
class GameListItem : public QStandardItem {

public:
	GameListItem() : QStandardItem() {}
	GameListItem(const QString& string) : QStandardItem(string) {}
	virtual ~GameListItem() override {}
};

/**
* A specialization of GameListItem for icons
* If no icon found then create an empty one
*/
class GameIconItem : public GameListItem
{
public:
	GameIconItem() : GameListItem() {}
	GameIconItem(const QString& pix_path)
		: GameListItem() {

		QPixmap icon(pix_path);
		if (icon.isNull())
		{
			QPixmap emptyicon(80, 44);
			emptyicon.fill(Qt::transparent);
			setData(emptyicon.scaled(80, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
		}
		else
		{
			setData(icon.scaled(80, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::DecorationRole);
		}
	}
};

/**
* Asynchronous worker object for populating the game list.
* Communicates with other threads through Qt's signal/slot system.
*/
class GameListWorker : public QObject, public QRunnable {
	Q_OBJECT

public:
	GameListWorker(QString dir_path)
		: QObject(), QRunnable(), dir_path(dir_path) {}

public slots:
	/// Starts the processing of directory tree information.
	void run() override;
	/// Tells the worker that it should no longer continue processing. Thread-safe.
	void Cancel();

signals:
	/**
	* The `EntryReady` signal is emitted once an entry has been prepared and is ready
	* to be added to the game list.
	* @param entry_items a list with `QStandardItem`s that make up the columns of the new entry.
	*/
	void EntryReady(QList<QStandardItem*> entry_items);
	void Finished();

private:
	QString dir_path;
	std::atomic_bool stop_processing;

	void AddEntriesToGameList(const std::string& dir_path);
};
