#ifndef CHEATS_PATCHES_H
#define CHEATS_PATCHES_H

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListView>
#include <QMap>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class CheatsPatches : public QWidget {
    Q_OBJECT

public:
    CheatsPatches(const QString& gameName, const QString& gameSerial, const QString& gameVersion,
                  const QString& gameSize, const QPixmap& gameImage, QWidget* parent = nullptr);
    ~CheatsPatches();
    void downloadCheats(const QString& source, const QString& m_gameSerial,
                        const QString& m_gameVersion, const bool showMessageBox);
    void downloadPatches();

signals:
    void downloadFinished();

private:
    // UI Setup and Event Handlers
    void setupUI();
    void onSaveButtonClicked();
    QCheckBox* findCheckBoxByName(const QString& name);
    void onTabChanged(int index);
    void updateNoteTextEdit(const QString& patchName);

    // Cheat and Patch Management
    void populateFileListCheats();
    void onFileSelected(const QModelIndex& index);
    void createFilesJson();
    void uncheckAllCheatCheckBoxes();

    void loadCheats(const QString& filePath);
    void loadPatches(const QString& serial);

    void addCheatsToLayout(const QJsonArray& modsArray, const QJsonArray& creditsArray);
    void addPatchToLayout(const QString& name, const QString& author, const QString& note,
                          const QJsonArray& linesArray, const QString& serial, bool isEnabled);

    void applyCheat(const QString& modName, bool enabled);
    void applyPatch(const QString& patchName, bool enabled);
    QString convertValueToHex(const QString& type, const QString& valueStr);

    // Event Filtering
    bool eventFilter(QObject* obj, QEvent* event);
    void onPatchCheckBoxHovered(QCheckBox* checkBox, bool hovered);

    // Network Manager
    QNetworkAccessManager* manager;

    // Patch Info Structures
    struct MemoryMod {
        QString offset;
        QString on;
        QString off;
    };

    struct Cheat {
        QString name;
        QString type;
        bool hasHint;
        QVector<MemoryMod> memoryMods;
    };

    struct PatchInfo {
        QString name;
        QString author;
        QString note;
        QJsonArray linesArray;
        QString serial;
    };

    // Members
    QString m_gameName;
    QString m_gameSerial;
    QString m_gameVersion;
    QString m_gameSize;
    QPixmap m_gameImage;
    QString m_cheatFilePath;
    QMap<QString, Cheat> m_cheats;
    QMap<QString, PatchInfo> m_patchInfos;
    QVector<QCheckBox*> m_cheatCheckBoxes;

    // UI Elements
    QVBoxLayout* rightLayout;
    QVBoxLayout* patchesGroupBoxLayout;
    QGroupBox* patchesGroupBox;
    QVBoxLayout* patchesLayout;
    QTextEdit* instructionsTextEdit;
    QListView* listView_selectFile;
    QItemSelectionModel* selectionModel;
};

#endif // CHEATS_PATCHES_H
