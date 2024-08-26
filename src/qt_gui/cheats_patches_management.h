#ifndef CHEATS_PATCHES_MANAGEMENT_H
#define CHEATS_PATCHES_MANAGEMENT_H

#include <QList>
#include <QListView>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class CheatsPatchesManagement : public QObject {
    Q_OBJECT

public:
    explicit CheatsPatchesManagement(QObject* parent = nullptr);
    void setupCheatsManagementWidget(QWidget* parent);
    void setGameInfo(const QList<QPair<QString, QString>>& gameInfoPairs);

private:
    void populateFileListCheats();
    void populateFileListPatches();
    void loadCheats(const QString& filePath);
    void addMods(const QJsonArray& modsArray);
    void onCheckUpdateButtonClicked();
    void onCheckPatchesUpdateButtonClicked();
    QList<QPair<QString, QString>> m_gameInfoPairs;

    QListView* listView_selectFile;
    QVBoxLayout* rightLayout;
    QListView* patchesListView;

signals:
    void downloadFinished();
};

#endif // CHEATS_PATCHES_MANAGEMENT_H
