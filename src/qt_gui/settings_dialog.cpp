// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCompleter>
#include <QDirIterator>
#include <QHoverEvent>

#include <common/version.h>
#ifdef ENABLE_DISCORD_RPC
#include "common/discord_rpc_handler.h"
#endif
#ifdef ENABLE_UPDATER
#include "check_update.h"
#endif
#include <toml.hpp>
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/formatter.h"
#include "main_window.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"
QStringList languageNames = {"Arabic",
                             "Czech",
                             "Danish",
                             "Dutch",
                             "English (United Kingdom)",
                             "English (United States)",
                             "Finnish",
                             "French (Canada)",
                             "French (France)",
                             "German",
                             "Greek",
                             "Hungarian",
                             "Indonesian",
                             "Italian",
                             "Japanese",
                             "Korean",
                             "Norwegian (Bokmaal)",
                             "Polish",
                             "Portuguese (Brazil)",
                             "Portuguese (Portugal)",
                             "Romanian",
                             "Russian",
                             "Simplified Chinese",
                             "Spanish (Latin America)",
                             "Spanish (Spain)",
                             "Swedish",
                             "Thai",
                             "Traditional Chinese",
                             "Turkish",
                             "Ukrainian",
                             "Vietnamese"};

const QVector<int> languageIndexes = {21, 23, 14, 6, 18, 1, 12, 22, 2, 4,  25, 24, 29, 5,  0, 9,
                                      15, 16, 17, 7, 26, 8, 11, 20, 3, 13, 27, 10, 19, 30, 28};

SettingsDialog::SettingsDialog(std::span<const QString> physical_devices, QWidget* parent)
    : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    ui->tabWidgetSettings->setUsesScrollButtons(false);
    initialHeight = this->height();
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);

    ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();

    // Add list of available GPUs
    ui->graphicsAdapterBox->addItem("Auto Select"); // -1, auto selection
    for (const auto& device : physical_devices) {
        ui->graphicsAdapterBox->addItem(device);
    }

    ui->consoleLanguageComboBox->addItems(languageNames);

    QCompleter* completer = new QCompleter(languageNames, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->consoleLanguageComboBox->setCompleter(completer);

    ui->hideCursorComboBox->addItem(tr("Never"));
    ui->hideCursorComboBox->addItem(tr("Idle"));
    ui->hideCursorComboBox->addItem(tr("Always"));

    ui->backButtonBehaviorComboBox->addItem(tr("Touchpad Left"), "left");
    ui->backButtonBehaviorComboBox->addItem(tr("Touchpad Center"), "center");
    ui->backButtonBehaviorComboBox->addItem(tr("Touchpad Right"), "right");
    ui->backButtonBehaviorComboBox->addItem(tr("None"), "none");

    InitializeEmulatorLanguages();
    LoadValuesFromConfig();

    defaultTextEdit = tr("Point your mouse at an option to display its description.");
    ui->descriptionText->setText(defaultTextEdit);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this,
            [this, config_dir](QAbstractButton* button) {
                if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
                    UpdateSettings();
                    Config::save(config_dir / "config.toml");
                    QWidget::close();
                } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
                    UpdateSettings();
                    Config::save(config_dir / "config.toml");
                } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
                    Config::setDefaultValues();
                    Config::save(config_dir / "config.toml");
                    LoadValuesFromConfig();
                } else if (button == ui->buttonBox->button(QDialogButtonBox::Close)) {
                    ResetInstallFolders();
                }
                if (Common::Log::IsActive()) {
                    Common::Log::Filter filter;
                    filter.ParseFilterString(Config::getLogFilter());
                    Common::Log::SetGlobalFilter(filter);
                }
            });

    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Restore Defaults"));
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));

    connect(ui->tabWidgetSettings, &QTabWidget::currentChanged, this,
            [this]() { ui->buttonBox->button(QDialogButtonBox::Close)->setFocus(); });

    // GENERAL TAB
    {
#ifdef ENABLE_UPDATER
        connect(ui->updateCheckBox, &QCheckBox::stateChanged, this,
                [](int state) { Config::setAutoUpdate(state == Qt::Checked); });

        connect(ui->updateComboBox, &QComboBox::currentTextChanged, this,
                [](const QString& channel) { Config::setUpdateChannel(channel.toStdString()); });

        connect(ui->checkUpdateButton, &QPushButton::clicked, this, []() {
            auto checkUpdate = new CheckUpdate(true);
            checkUpdate->exec();
        });
#else
        ui->updaterGroupBox->setVisible(false);
        ui->GUIgroupBox->setMaximumSize(265, 16777215);
#endif
    }

    // Input TAB
    {
        connect(ui->hideCursorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](s16 index) { OnCursorStateChanged(index); });
    }

    // PATH TAB
    {
        connect(ui->addFolderButton, &QPushButton::clicked, this, [this]() {
            const auto config_dir = Config::getGameInstallDirs();
            QString file_path_string =
                QFileDialog::getExistingDirectory(this, tr("Directory to install games"));
            auto file_path = Common::FS::PathFromQString(file_path_string);
            if (!file_path.empty() && Config::addGameInstallDir(file_path)) {
                QListWidgetItem* item = new QListWidgetItem(file_path_string);
                ui->gameFoldersListWidget->addItem(item);
            }
        });

        connect(ui->gameFoldersListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
            ui->removeFolderButton->setEnabled(
                !ui->gameFoldersListWidget->selectedItems().isEmpty());
        });

        connect(ui->removeFolderButton, &QPushButton::clicked, this, [this]() {
            QListWidgetItem* selected_item = ui->gameFoldersListWidget->currentItem();
            QString item_path_string = selected_item ? selected_item->text() : QString();
            if (!item_path_string.isEmpty()) {
                auto file_path = Common::FS::PathFromQString(item_path_string);
                Config::removeGameInstallDir(file_path);
                delete selected_item;
            }
        });
    }

    // Descriptions
    {
        // General
        ui->consoleLanguageGroupBox->installEventFilter(this);
        ui->emulatorLanguageGroupBox->installEventFilter(this);
        ui->fullscreenCheckBox->installEventFilter(this);
        ui->separateUpdatesCheckBox->installEventFilter(this);
        ui->showSplashCheckBox->installEventFilter(this);
        ui->ps4proCheckBox->installEventFilter(this);
        ui->discordRPCCheckbox->installEventFilter(this);
        ui->userName->installEventFilter(this);
        ui->logTypeGroupBox->installEventFilter(this);
        ui->logFilter->installEventFilter(this);
#ifdef ENABLE_UPDATER
        ui->updaterGroupBox->installEventFilter(this);
#endif
        ui->GUIgroupBox->installEventFilter(this);

        // Input
        ui->hideCursorGroupBox->installEventFilter(this);
        ui->idleTimeoutGroupBox->installEventFilter(this);
        ui->backButtonBehaviorGroupBox->installEventFilter(this);

        // Graphics
        ui->graphicsAdapterGroupBox->installEventFilter(this);
        ui->widthGroupBox->installEventFilter(this);
        ui->heightGroupBox->installEventFilter(this);
        ui->heightDivider->installEventFilter(this);
        ui->dumpShadersCheckBox->installEventFilter(this);
        ui->nullGpuCheckBox->installEventFilter(this);

        // Paths
        ui->gameFoldersGroupBox->installEventFilter(this);
        ui->gameFoldersListWidget->installEventFilter(this);
        ui->addFolderButton->installEventFilter(this);
        ui->removeFolderButton->installEventFilter(this);

        // Debug
        ui->debugDump->installEventFilter(this);
        ui->vkValidationCheckBox->installEventFilter(this);
        ui->vkSyncValidationCheckBox->installEventFilter(this);
        ui->rdocCheckBox->installEventFilter(this);
    }
}

void SettingsDialog::LoadValuesFromConfig() {

    std::filesystem::path userdir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    std::error_code error;
    if (!std::filesystem::exists(userdir / "Config.toml", error)) {
        Config::load(userdir / "Config.toml");
        return;
    }

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        const toml::value data = toml::parse(userdir / "Config.toml");
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    const toml::value data = toml::parse(userdir / "Config.toml");
    const QVector<int> languageIndexes = {21, 23, 14, 6, 18, 1, 12, 22, 2, 4,  25, 24, 29, 5,  0, 9,
                                          15, 16, 17, 7, 26, 8, 11, 20, 3, 13, 27, 10, 19, 30, 28};

    ui->consoleLanguageComboBox->setCurrentIndex(
        std::distance(languageIndexes.begin(),
                      std::find(languageIndexes.begin(), languageIndexes.end(),
                                toml::find_or<int>(data, "Settings", "consoleLanguage", 6))) %
        languageIndexes.size());
    ui->emulatorLanguageComboBox->setCurrentIndex(
        languages[toml::find_or<std::string>(data, "GUI", "emulatorLanguage", "en")]);
    ui->hideCursorComboBox->setCurrentIndex(toml::find_or<int>(data, "Input", "cursorState", 1));
    OnCursorStateChanged(toml::find_or<int>(data, "Input", "cursorState", 1));
    ui->idleTimeoutSpinBox->setValue(toml::find_or<int>(data, "Input", "cursorHideTimeout", 5));
    // First options is auto selection -1, so gpuId on the GUI will always have to subtract 1
    // when setting and add 1 when getting to select the correct gpu in Qt
    ui->graphicsAdapterBox->setCurrentIndex(toml::find_or<int>(data, "Vulkan", "gpuId", -1) + 1);
    ui->widthSpinBox->setValue(toml::find_or<int>(data, "GPU", "screenWidth", 1280));
    ui->heightSpinBox->setValue(toml::find_or<int>(data, "GPU", "screenHeight", 720));
    ui->vblankSpinBox->setValue(toml::find_or<int>(data, "GPU", "vblankDivider", 1));
    ui->dumpShadersCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "dumpShaders", false));
    ui->nullGpuCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "nullGpu", false));
    ui->playBGMCheckBox->setChecked(toml::find_or<bool>(data, "General", "playBGM", false));
    ui->BGMVolumeSlider->setValue(toml::find_or<int>(data, "General", "BGMvolume", 50));
    ui->discordRPCCheckbox->setChecked(
        toml::find_or<bool>(data, "General", "enableDiscordRPC", true));
    ui->fullscreenCheckBox->setChecked(toml::find_or<bool>(data, "General", "Fullscreen", false));
    ui->separateUpdatesCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "separateUpdateEnabled", false));
    ui->showSplashCheckBox->setChecked(toml::find_or<bool>(data, "General", "showSplash", false));
    ui->ps4proCheckBox->setChecked(toml::find_or<bool>(data, "General", "isPS4Pro", false));
    ui->logTypeComboBox->setCurrentText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "logType", "async")));
    ui->logFilterLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "logFilter", "")));
    ui->userNameLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "userName", "shadPS4")));
    ui->debugDump->setChecked(toml::find_or<bool>(data, "Debug", "DebugDump", false));
    ui->vkValidationCheckBox->setChecked(toml::find_or<bool>(data, "Vulkan", "validation", false));
    ui->vkSyncValidationCheckBox->setChecked(
        toml::find_or<bool>(data, "Vulkan", "validation_sync", false));
    ui->rdocCheckBox->setChecked(toml::find_or<bool>(data, "Vulkan", "rdocEnable", false));

#ifdef ENABLE_UPDATER
    ui->updateCheckBox->setChecked(toml::find_or<bool>(data, "General", "autoUpdate", false));
    std::string updateChannel = toml::find_or<std::string>(data, "General", "updateChannel", "");
    if (updateChannel != "Release" && updateChannel != "Nightly") {
        if (Common::isRelease) {
            updateChannel = "Release";
        } else {
            updateChannel = "Nightly";
        }
    }
    ui->updateComboBox->setCurrentText(QString::fromStdString(updateChannel));
#endif

    QString backButtonBehavior = QString::fromStdString(
        toml::find_or<std::string>(data, "Input", "backButtonBehavior", "left"));
    int index = ui->backButtonBehaviorComboBox->findData(backButtonBehavior);
    ui->backButtonBehaviorComboBox->setCurrentIndex(index != -1 ? index : 0);

    ui->removeFolderButton->setEnabled(!ui->gameFoldersListWidget->selectedItems().isEmpty());
    ResetInstallFolders();
}

void SettingsDialog::InitializeEmulatorLanguages() {
    QDirIterator it(QStringLiteral(":/translations"), QDirIterator::NoIteratorFlags);

    QVector<QPair<QString, QString>> languagesList;

    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
        locale.remove(0, locale.lastIndexOf(QLatin1Char{'/'}) + 1);
        const QString lang = QLocale::languageToString(QLocale(locale).language());
        const QString country = QLocale::territoryToString(QLocale(locale).territory());

        QString displayName = QStringLiteral("%1 (%2)").arg(lang, country);
        languagesList.append(qMakePair(locale, displayName));
    }

    std::sort(languagesList.begin(), languagesList.end(),
              [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
                  return a.second < b.second;
              });

    int idx = 0;
    for (const auto& pair : languagesList) {
        const QString& locale = pair.first;
        const QString& displayName = pair.second;

        ui->emulatorLanguageComboBox->addItem(displayName, locale);
        languages[locale.toStdString()] = idx;
        idx++;
    }

    connect(ui->emulatorLanguageComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &SettingsDialog::OnLanguageChanged);
}

void SettingsDialog::OnLanguageChanged(int index) {
    if (index == -1)
        return;

    ui->retranslateUi(this);

    emit LanguageChanged(ui->emulatorLanguageComboBox->itemData(index).toString().toStdString());
}

void SettingsDialog::OnCursorStateChanged(s16 index) {
    if (index == -1)
        return;
    if (index == Config::HideCursorState::Idle) {
        ui->idleTimeoutGroupBox->show();
    } else {
        if (!ui->idleTimeoutGroupBox->isHidden()) {
            ui->idleTimeoutGroupBox->hide();
        }
    }
}

int SettingsDialog::exec() {
    return QDialog::exec();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::updateNoteTextEdit(const QString& elementName) {
    QString text; // texts are only in .ts translation files for better formatting

    // General
    if (elementName == "consoleLanguageGroupBox") {
        text = tr("consoleLanguageGroupBox");
    } else if (elementName == "emulatorLanguageGroupBox") {
        text = tr("emulatorLanguageGroupBox");
    } else if (elementName == "fullscreenCheckBox") {
        text = tr("fullscreenCheckBox");
    } else if (elementName == "separateUpdatesCheckBox") {
        text = tr("separateUpdatesCheckBox");
    } else if (elementName == "showSplashCheckBox") {
        text = tr("showSplashCheckBox");
    } else if (elementName == "ps4proCheckBox") {
        text = tr("ps4proCheckBox");
    } else if (elementName == "discordRPCCheckbox") {
        text = tr("discordRPCCheckbox");
    } else if (elementName == "userName") {
        text = tr("userName");
    } else if (elementName == "logTypeGroupBox") {
        text = tr("logTypeGroupBox");
    } else if (elementName == "logFilter") {
        text = tr("logFilter");
#ifdef ENABLE_UPDATER
    } else if (elementName == "updaterGroupBox") {
        text = tr("updaterGroupBox");
#endif
    } else if (elementName == "GUIgroupBox") {
        text = tr("GUIgroupBox");
    }

    // Input
    if (elementName == "hideCursorGroupBox") {
        text = tr("hideCursorGroupBox");
    } else if (elementName == "idleTimeoutGroupBox") {
        text = tr("idleTimeoutGroupBox");
    } else if (elementName == "backButtonBehaviorGroupBox") {
        text = tr("backButtonBehaviorGroupBox");
    }

    // Graphics
    if (elementName == "graphicsAdapterGroupBox") {
        text = tr("graphicsAdapterGroupBox");
    } else if (elementName == "widthGroupBox") {
        text = tr("resolutionLayout");
    } else if (elementName == "heightGroupBox") {
        text = tr("resolutionLayout");
    } else if (elementName == "heightDivider") {
        text = tr("heightDivider");
    } else if (elementName == "dumpShadersCheckBox") {
        text = tr("dumpShadersCheckBox");
    } else if (elementName == "nullGpuCheckBox") {
        text = tr("nullGpuCheckBox");
    }

    // Path
    if (elementName == "gameFoldersGroupBox" || elementName == "gameFoldersListWidget") {
        text = tr("gameFoldersBox");
    } else if (elementName == "addFolderButton") {
        text = tr("addFolderButton");
    } else if (elementName == "removeFolderButton") {
        text = tr("removeFolderButton");
    }

    // Debug
    if (elementName == "debugDump") {
        text = tr("debugDump");
    } else if (elementName == "vkValidationCheckBox") {
        text = tr("vkValidationCheckBox");
    } else if (elementName == "vkSyncValidationCheckBox") {
        text = tr("vkSyncValidationCheckBox");
    } else if (elementName == "rdocCheckBox") {
        text = tr("rdocCheckBox");
    }

    ui->descriptionText->setText(text.replace("\\n", "\n"));
}

bool SettingsDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        if (qobject_cast<QWidget*>(obj)) {
            bool hovered = (event->type() == QEvent::Enter);
            QString elementName = obj->objectName();

            if (hovered) {
                updateNoteTextEdit(elementName);
            } else {
                ui->descriptionText->setText(defaultTextEdit);
            }

            // if the text exceeds the size of the box, it will increase the size
            QRect currentGeometry = this->geometry();
            int newWidth = currentGeometry.width();

            int documentHeight = ui->descriptionText->document()->size().height();
            int visibleHeight = ui->descriptionText->viewport()->height();
            if (documentHeight > visibleHeight) {
                ui->descriptionText->setMaximumSize(16777215, 110);
                this->setGeometry(currentGeometry.x(), currentGeometry.y(), newWidth,
                                  currentGeometry.height() + 40);
            } else {
                ui->descriptionText->setMaximumSize(16777215, 70);
                this->setGeometry(currentGeometry.x(), currentGeometry.y(), newWidth,
                                  initialHeight);
            }
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void SettingsDialog::UpdateSettings() {

    const QVector<std::string> TouchPadIndex = {"left", "center", "right", "none"};
    Config::setBackButtonBehavior(TouchPadIndex[ui->backButtonBehaviorComboBox->currentIndex()]);
    Config::setNeoMode(ui->ps4proCheckBox->isChecked());
    Config::setFullscreenMode(ui->fullscreenCheckBox->isChecked());
    Config::setPlayBGM(ui->playBGMCheckBox->isChecked());
    Config::setNeoMode(ui->ps4proCheckBox->isChecked());
    Config::setLogType(ui->logTypeComboBox->currentText().toStdString());
    Config::setLogFilter(ui->logFilterLineEdit->text().toStdString());
    Config::setUserName(ui->userNameLineEdit->text().toStdString());
    Config::setCursorState(ui->hideCursorComboBox->currentIndex());
    Config::setCursorHideTimeout(ui->idleTimeoutSpinBox->value());
    Config::setGpuId(ui->graphicsAdapterBox->currentIndex() - 1);
    Config::setBGMvolume(ui->BGMVolumeSlider->value());
    Config::setLanguage(languageIndexes[ui->consoleLanguageComboBox->currentIndex()]);
    Config::setEnableDiscordRPC(ui->discordRPCCheckbox->isChecked());
    Config::setScreenWidth(ui->widthSpinBox->value());
    Config::setScreenHeight(ui->heightSpinBox->value());
    Config::setVblankDiv(ui->vblankSpinBox->value());
    Config::setDumpShaders(ui->dumpShadersCheckBox->isChecked());
    Config::setNullGpu(ui->nullGpuCheckBox->isChecked());
    Config::setSeparateUpdateEnabled(ui->separateUpdatesCheckBox->isChecked());
    Config::setShowSplash(ui->showSplashCheckBox->isChecked());
    Config::setDebugDump(ui->debugDump->isChecked());
    Config::setVkValidation(ui->vkValidationCheckBox->isChecked());
    Config::setVkSyncValidation(ui->vkSyncValidationCheckBox->isChecked());
    Config::setRdocEnabled(ui->rdocCheckBox->isChecked());
    Config::setAutoUpdate(ui->updateCheckBox->isChecked());
    Config::setUpdateChannel(ui->updateComboBox->currentText().toStdString());

#ifdef ENABLE_DISCORD_RPC
    auto* rpc = Common::Singleton<DiscordRPCHandler::RPC>::Instance();
    if (Config::getEnableDiscordRPC()) {
        rpc->init();
        rpc->setStatusIdling();
    } else {
        rpc->shutdown();
    }
#endif

    BackgroundMusicPlayer::getInstance().setVolume(ui->BGMVolumeSlider->value());
    ResetInstallFolders();
}

void SettingsDialog::ResetInstallFolders() {

    std::filesystem::path userdir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    const toml::value data = toml::parse(userdir / "Config.toml");

    if (data.contains("GUI")) {
        const toml::value& gui = data.at("GUI");
        const auto install_dir_array =
            toml::find_or<std::vector<std::string>>(gui, "installDirs", {});
        std::vector<std::filesystem::path> settings_install_dirs_config = {};

        for (const auto& dir : install_dir_array) {
            if (std::find(settings_install_dirs_config.begin(), settings_install_dirs_config.end(),
                          dir) == settings_install_dirs_config.end()) {
                settings_install_dirs_config.push_back(dir);
            }
        }

        for (const auto& dir : settings_install_dirs_config) {
            QString path_string;
            Common::FS::PathToQString(path_string, dir);
            QListWidgetItem* item = new QListWidgetItem(path_string);
            ui->gameFoldersListWidget->addItem(item);
        }
        Config::setGameInstallDirs(settings_install_dirs_config);
    }
}