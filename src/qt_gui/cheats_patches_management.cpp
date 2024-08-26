#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStringListModel>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include "cheats_patches.h"
#include "cheats_patches_management.h"
#include "common/path_util.h"

using namespace Common::FS;

CheatsPatchesManagement::CheatsPatchesManagement(QObject* parent) : QObject(parent) {}

void CheatsPatchesManagement::setupCheatsManagementWidget(QWidget* parent) {
    const auto& CHEATS_DIR = Common::FS::GetUserPath(Common::FS::PathType::CheatsDir);
    QString CHEATS_DIR_QString = QString::fromStdString(CHEATS_DIR.string());

    QWidget* cheatWidget = new QWidget(parent, Qt::Window);
    cheatWidget->setAttribute(Qt::WA_DeleteOnClose);
    cheatWidget->setWindowTitle("Cheats/Patches Management");
    cheatWidget->resize(800, 800);

    QVBoxLayout* mainLayout = new QVBoxLayout(cheatWidget);

    QTabWidget* tabWidget = new QTabWidget();

    QWidget* cheatsTab = new QWidget();
    QVBoxLayout* cheatsLayout = new QVBoxLayout(cheatsTab);

    listView_selectFile = new QListView();
    listView_selectFile->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    listView_selectFile->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_selectFile->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout* fileListLayout = new QVBoxLayout();
    fileListLayout->addWidget(new QLabel("Select Cheat File:"));
    fileListLayout->addWidget(listView_selectFile);

    QPushButton* deleteCheatButton = new QPushButton("Delete File");
    fileListLayout->addWidget(deleteCheatButton);

    QString defaultTextEdit =
        "With the button below 'Download' you can download Cheats for all games that are "
        "installed on the emulator, they will be downloaded from all available repositories, "
        "GoldHEN, wolf2022, shadPS4.\n"
        "With the button 'Delete File' you can delete the chosen Cheat file from the list "
        "above.\n\nIn the main menu, by right-clicking, you can open each game individually.";

    QTextEdit* infoTextEdit = new QTextEdit();
    infoTextEdit->setText(defaultTextEdit);
    infoTextEdit->setReadOnly(true);
    infoTextEdit->setFixedHeight(90);

    fileListLayout->addWidget(infoTextEdit);
    fileListLayout->setAlignment(Qt::AlignTop);

    QWidget* leftWidget = new QWidget();
    leftWidget->setLayout(fileListLayout);
    leftWidget->setMinimumWidth(600);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    QGroupBox* cheatsGroupBox = new QGroupBox();
    rightLayout = new QVBoxLayout(cheatsGroupBox);
    rightLayout->setAlignment(Qt::AlignTop);
    cheatsGroupBox->setLayout(rightLayout);

    scrollArea->setWidget(cheatsGroupBox);

    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightWidgetLayout = new QVBoxLayout(rightWidget);
    rightWidgetLayout->addWidget(scrollArea);
    rightWidget->setMinimumWidth(400);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(leftWidget);
    topLayout->addWidget(rightWidget);

    cheatsLayout->addLayout(topLayout);

    QHBoxLayout* cheatsButtonLayout = new QHBoxLayout();
    QPushButton* checkUpdateButton = new QPushButton("Download Cheats For All Installed Games");

    connect(checkUpdateButton, &QPushButton::clicked, [this]() { onCheckUpdateButtonClicked(); });

    connect(deleteCheatButton, &QPushButton::clicked, [=]() {
        QStringListModel* model = qobject_cast<QStringListModel*>(listView_selectFile->model());
        if (!model) {
            return;
        }
        QItemSelectionModel* selectionModel = listView_selectFile->selectionModel();
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

        if (!selectedIndexes.isEmpty()) {
            int row = selectedIndexes.first().row();
            QString selectedFileName = model->stringList().at(row);
            QString filePath =
                CHEATS_DIR_QString + "/" + selectedFileName.split("|").first().trimmed();

            if (QFile::remove(filePath)) {
                QMessageBox::information(cheatWidget, "File Deleted",
                                         "File has been successfully deleted.");
                populateFileListCheats();
            } else {
                QMessageBox::critical(cheatWidget, "File Deletion Failed",
                                      "Failed to delete the selected file.");
            }
        }
    });

    cheatsButtonLayout->addWidget(checkUpdateButton);
    cheatsButtonLayout->setAlignment(Qt::AlignBottom);

    cheatsLayout->addLayout(cheatsButtonLayout);

    tabWidget->addTab(cheatsTab, "CHEATS");

    QWidget* patchesTab = new QWidget();
    QVBoxLayout* patchesLayout = new QVBoxLayout(patchesTab);

    QString default2TextEdit = "implementing  :)";
    QTextEdit* infoTextEdit2 = new QTextEdit();
    infoTextEdit2->setText(default2TextEdit);
    infoTextEdit2->setReadOnly(true);
    infoTextEdit2->setFixedHeight(200);

    patchesLayout->addWidget(infoTextEdit2);

    tabWidget->addTab(patchesTab, "PATCHES");

    mainLayout->addWidget(tabWidget);

    cheatWidget->setLayout(mainLayout);
    populateFileListCheats();
    cheatWidget->show();
}

void CheatsPatchesManagement::setGameInfo(const QList<QPair<QString, QString>>& gameInfoPairs) {
    m_gameInfoPairs = gameInfoPairs;
}

void CheatsPatchesManagement::onCheckUpdateButtonClicked() {
    QEventLoop eventLoop;
    int pendingDownloads = 0;

    auto onDownloadFinished = [&]() {
        if (--pendingDownloads <= 0) {
            eventLoop.quit();
            // Exit loop when all downloads are complete
        }
    };

    for (const QPair<QString, QString>& gameInfo : m_gameInfoPairs) {
        QString gameSerial = gameInfo.first;
        QString gameVersion = gameInfo.second;
        QString empty = "";
        CheatsPatches* cheatsPatches =
            new CheatsPatches(empty, empty, empty, empty, empty, nullptr);
        connect(cheatsPatches, &CheatsPatches::downloadFinished, onDownloadFinished);

        // Count how many downloads are in progress, 3 downloads for each game
        pendingDownloads += 3;

        cheatsPatches->downloadCheats("wolf2022", gameSerial, gameVersion, false);
        cheatsPatches->downloadCheats("GoldHEN", gameSerial, gameVersion, false);
        cheatsPatches->downloadCheats("shadPS4", gameSerial, gameVersion, false);
    }
    // Block until all downloads are complete
    eventLoop.exec();
    QMessageBox::information(nullptr, "Cheats Downloaded Successfully",
                             "You have downloaded cheats for all the games you have installed.\n\n"
                             "If you want, you can use the button to delete a file.");
    populateFileListCheats();
}

void CheatsPatchesManagement::populateFileListCheats() {
    const auto& CHEATS_DIR = Common::FS::GetUserPath(Common::FS::PathType::CheatsDir);
    QString CHEATS_DIR_QString = QString::fromStdString(CHEATS_DIR.string());

    QDir dir(CHEATS_DIR_QString);
    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);

    QFileInfoList fileList = dir.entryInfoList(QDir::Files);
    QStringList fileNames;
    QStringList filePaths;

    for (const QFileInfo& fileInfo : fileList) {
        fileNames << fileInfo.fileName();
        filePaths << fileInfo.absoluteFilePath();
    }

    QStringList formattedFileNames;
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile file(filePaths[i]);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = file.readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObject = jsonDoc.object();
            QString gameName = jsonObject["name"].toString();
            formattedFileNames << QString("%1 | %2").arg(fileNames[i]).arg(gameName);
            file.close();
        }
    }

    QStringListModel* newModel = new QStringListModel(formattedFileNames, nullptr);
    listView_selectFile->setModel(newModel);

    connect(listView_selectFile->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this, filePaths, newModel]() {
                QModelIndexList selectedIndexes =
                    listView_selectFile->selectionModel()->selectedIndexes();
                if (!selectedIndexes.isEmpty()) {
                    QString selectedFilePath = filePaths[selectedIndexes.first().row()];
                    loadCheats(selectedFilePath);
                } else {
                    addMods(QJsonArray());
                }
            });

    if (!formattedFileNames.isEmpty()) {
        QModelIndex firstIndex = newModel->index(0, 0);
        listView_selectFile->selectionModel()->select(firstIndex, QItemSelectionModel::Select |
                                                                      QItemSelectionModel::Rows);
        listView_selectFile->setCurrentIndex(firstIndex);
        QString selectedFilePath = filePaths.first();
        loadCheats(selectedFilePath);
    } else {
        listView_selectFile->selectionModel()->clearSelection();
        addMods(QJsonArray());
    }
}

void CheatsPatchesManagement::addMods(const QJsonArray& modsArray) {
    if (!rightLayout)
        return;

    QLayoutItem* item;
    while ((item = rightLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (const QJsonValue& modValue : modsArray) {
        QJsonObject modObject = modValue.toObject();
        QString modName = modObject["name"].toString();
        QString modType = modObject["type"].toString();
        if (modType == "checkbox") {
            QCheckBox* cheatCheckBox = new QCheckBox(modName);
            cheatCheckBox->setChecked(false);
            cheatCheckBox->setEnabled(false);
            rightLayout->addWidget(cheatCheckBox);
        } else if (modType == "button") {
            QPushButton* cheatButton = new QPushButton(modName);
            cheatButton->setEnabled(false);
            rightLayout->addWidget(cheatButton);
        }
    }
}

void CheatsPatchesManagement::loadCheats(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray modsArray = jsonObject["mods"].toArray();
        addMods(modsArray);
    }
}
