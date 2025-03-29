// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCompleter>
#include <QDirIterator>
#include <QFileDialog>
#include <QHoverEvent>
#include <QMessageBox>
#include <fmt/format.h>

#include "common/config.h"
#include "common/version.h"
#include "qt_gui/compatibility_info.h"
#ifdef ENABLE_DISCORD_RPC
#include "common/discord_rpc_handler.h"
#include "common/singleton.h"
#endif
#ifdef ENABLE_UPDATER
#include "check_update.h"
#endif
#include <QDesktopServices>
#include <toml.hpp>
#include "background_music_player.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
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
QMap<QString, QString> channelMap;
QMap<QString, QString> logTypeMap;
QMap<QString, QString> screenModeMap;
QMap<QString, QString> chooseHomeTabMap;

int backgroundImageOpacitySlider_backup;
int bgm_volume_backup;

SettingsDialog::SettingsDialog(std::span<const QString> physical_devices,
                               std::shared_ptr<CompatibilityInfoClass> m_compat_info,
                               QWidget* parent)
    : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    ui->tabWidgetSettings->setUsesScrollButtons(false);

    initialHeight = this->height();
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);

    ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();

    channelMap = {{tr("Release"), "Release"}, {tr("Nightly"), "Nightly"}};
    logTypeMap = {{tr("async"), "async"}, {tr("sync"), "sync"}};
    screenModeMap = {{tr("Fullscreen (Borderless)"), "Fullscreen (Borderless)"},
                     {tr("Windowed"), "Windowed"},
                     {tr("Fullscreen"), "Fullscreen"}};
    chooseHomeTabMap = {{tr("General"), "General"},   {tr("GUI"), "GUI"},
                        {tr("Graphics"), "Graphics"}, {tr("User"), "User"},
                        {tr("Input"), "Input"},       {tr("Paths"), "Paths"},
                        {tr("Debug"), "Debug"}};

    // Add list of available GPUs
    ui->graphicsAdapterBox->addItem(tr("Auto Select")); // -1, auto selection
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
                    is_saving = true;
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
                    ui->backgroundImageOpacitySlider->setValue(backgroundImageOpacitySlider_backup);
                    emit BackgroundOpacityChanged(backgroundImageOpacitySlider_backup);
                    ui->BGMVolumeSlider->setValue(bgm_volume_backup);
                    BackgroundMusicPlayer::getInstance().setVolume(bgm_volume_backup);
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->updateCheckBox, &QCheckBox::stateChanged, this,
                [](int state) { Config::setAutoUpdate(state == Qt::Checked); });

        connect(ui->changelogCheckBox, &QCheckBox::stateChanged, this,
                [](int state) { Config::setAlwaysShowChangelog(state == Qt::Checked); });
#else
        connect(ui->updateCheckBox, &QCheckBox::checkStateChanged, this,
                [](Qt::CheckState state) { Config::setAutoUpdate(state == Qt::Checked); });

        connect(ui->changelogCheckBox, &QCheckBox::checkStateChanged, this,
                [](Qt::CheckState state) { Config::setAlwaysShowChangelog(state == Qt::Checked); });
#endif

        connect(ui->updateComboBox, &QComboBox::currentTextChanged, this,
                [this](const QString& channel) {
                    if (channelMap.contains(channel)) {
                        Config::setUpdateChannel(channelMap.value(channel).toStdString());
                    }
                });

        connect(ui->checkUpdateButton, &QPushButton::clicked, this, []() {
            auto checkUpdate = new CheckUpdate(true);
            checkUpdate->exec();
        });
#else
        ui->updaterGroupBox->setVisible(false);
#endif
        connect(ui->updateCompatibilityButton, &QPushButton::clicked, this,
                [this, parent, m_compat_info]() {
                    m_compat_info->UpdateCompatibilityDatabase(this, true);
                    emit CompatibilityChanged();
                });

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->enableCompatibilityCheckBox, &QCheckBox::stateChanged, this,
                [this, m_compat_info](int state) {
#else
        connect(ui->enableCompatibilityCheckBox, &QCheckBox::checkStateChanged, this,
                [this, m_compat_info](Qt::CheckState state) {
#endif
                    Config::setCompatibilityEnabled(state);
                    if (state) {
                        m_compat_info->LoadCompatibilityFile();
                    }
                    emit CompatibilityChanged();
                });
    }

    // Gui TAB
    {
        connect(ui->backgroundImageOpacitySlider, &QSlider::valueChanged, this,
                [this](int value) { emit BackgroundOpacityChanged(value); });

        connect(ui->BGMVolumeSlider, &QSlider::valueChanged, this,
                [](int value) { BackgroundMusicPlayer::getInstance().setVolume(value); });

        connect(ui->chooseHomeTabComboBox, &QComboBox::currentTextChanged, this,
                [](const QString& hometab) { Config::setChooseHomeTab(hometab.toStdString()); });

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->showBackgroundImageCheckBox, &QCheckBox::stateChanged, this, [](int state) {
#else
        connect(ui->showBackgroundImageCheckBox, &QCheckBox::checkStateChanged, this,
                [](Qt::CheckState state) {
#endif
            Config::setShowBackgroundImage(state == Qt::Checked);
        });
    }

    // User TAB
    {
        connect(ui->OpenCustomTrophyLocationButton, &QPushButton::clicked, this, []() {
            QString userPath;
            Common::FS::PathToQString(userPath,
                                      Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy));
            QDesktopServices::openUrl(QUrl::fromLocalFile(userPath));
        });
    }

    // Input TAB
    {
        connect(ui->hideCursorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](s16 index) { OnCursorStateChanged(index); });
    }

    // PATH TAB
    {
        connect(ui->addFolderButton, &QPushButton::clicked, this, [this]() {
            QString file_path_string =
                QFileDialog::getExistingDirectory(this, tr("Directory to install games"));
            auto file_path = Common::FS::PathFromQString(file_path_string);
            if (!file_path.empty() && Config::addGameInstallDir(file_path, true)) {
                QListWidgetItem* item = new QListWidgetItem(file_path_string);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Checked);
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

        connect(ui->browseButton, &QPushButton::clicked, this, [this]() {
            const auto save_data_path = Config::GetSaveDataPath();
            QString initial_path;
            Common::FS::PathToQString(initial_path, save_data_path);

            QString save_data_path_string =
                QFileDialog::getExistingDirectory(this, tr("Directory to save data"), initial_path);

            auto file_path = Common::FS::PathFromQString(save_data_path_string);
            if (!file_path.empty()) {
                Config::setSaveDataPath(file_path);
                ui->currentSaveDataPath->setText(save_data_path_string);
            }
        });

        connect(ui->PortableUserButton, &QPushButton::clicked, this, []() {
            QString userDir;
            Common::FS::PathToQString(userDir, std::filesystem::current_path() / "user");
            if (std::filesystem::exists(std::filesystem::current_path() / "user")) {
                QMessageBox::information(NULL, tr("Cannot create portable user folder"),
                                         tr("%1 already exists").arg(userDir));
            } else {
                std::filesystem::copy(Common::FS::GetUserPath(Common::FS::PathType::UserDir),
                                      std::filesystem::current_path() / "user",
                                      std::filesystem::copy_options::recursive);
                QMessageBox::information(NULL, tr("Portable user folder created"),
                                         tr("%1 successfully created.").arg(userDir));
            }
        });
    }

    // DEBUG TAB
    {
        connect(ui->OpenLogLocationButton, &QPushButton::clicked, this, []() {
            QString userPath;
            Common::FS::PathToQString(userPath,
                                      Common::FS::GetUserPath(Common::FS::PathType::LogDir));
            QDesktopServices::openUrl(QUrl::fromLocalFile(userPath));
        });
    }

    // Descriptions
    {
        // General
        ui->consoleLanguageGroupBox->installEventFilter(this);
        ui->emulatorLanguageGroupBox->installEventFilter(this);
        ui->separateUpdatesCheckBox->installEventFilter(this);
        ui->showSplashCheckBox->installEventFilter(this);
        ui->discordRPCCheckbox->installEventFilter(this);
        ui->userName->installEventFilter(this);
        ui->label_Trophy->installEventFilter(this);
        ui->trophyKeyLineEdit->installEventFilter(this);
        ui->logTypeGroupBox->installEventFilter(this);
        ui->logFilter->installEventFilter(this);
#ifdef ENABLE_UPDATER
        ui->updaterGroupBox->installEventFilter(this);
#endif
        ui->GUIBackgroundImageGroupBox->installEventFilter(this);
        ui->GUIMusicGroupBox->installEventFilter(this);
        ui->disableTrophycheckBox->installEventFilter(this);
        ui->enableCompatibilityCheckBox->installEventFilter(this);
        ui->checkCompatibilityOnStartupCheckBox->installEventFilter(this);
        ui->updateCompatibilityButton->installEventFilter(this);

        // User
        ui->OpenCustomTrophyLocationButton->installEventFilter(this);

        // Input
        ui->hideCursorGroupBox->installEventFilter(this);
        ui->idleTimeoutGroupBox->installEventFilter(this);
        ui->backButtonBehaviorGroupBox->installEventFilter(this);

        // Graphics
        ui->graphicsAdapterGroupBox->installEventFilter(this);
        ui->windowSizeGroupBox->installEventFilter(this);
        ui->heightDivider->installEventFilter(this);
        ui->dumpShadersCheckBox->installEventFilter(this);
        ui->nullGpuCheckBox->installEventFilter(this);
        ui->enableHDRCheckBox->installEventFilter(this);

        // Paths
        ui->gameFoldersGroupBox->installEventFilter(this);
        ui->gameFoldersListWidget->installEventFilter(this);
        ui->addFolderButton->installEventFilter(this);
        ui->removeFolderButton->installEventFilter(this);

        ui->saveDataGroupBox->installEventFilter(this);
        ui->currentSaveDataPath->installEventFilter(this);
        ui->browseButton->installEventFilter(this);
        ui->PortableUserFolderGroupBox->installEventFilter(this);

        // Debug
        ui->debugDump->installEventFilter(this);
        ui->vkValidationCheckBox->installEventFilter(this);
        ui->vkSyncValidationCheckBox->installEventFilter(this);
        ui->rdocCheckBox->installEventFilter(this);
        ui->crashDiagnosticsCheckBox->installEventFilter(this);
        ui->guestMarkersCheckBox->installEventFilter(this);
        ui->hostMarkersCheckBox->installEventFilter(this);
        ui->collectShaderCheckBox->installEventFilter(this);
        ui->copyGPUBuffersCheckBox->installEventFilter(this);
    }
}

void SettingsDialog::closeEvent(QCloseEvent* event) {
    if (!is_saving) {
        ui->backgroundImageOpacitySlider->setValue(backgroundImageOpacitySlider_backup);
        emit BackgroundOpacityChanged(backgroundImageOpacitySlider_backup);
        ui->BGMVolumeSlider->setValue(bgm_volume_backup);
        BackgroundMusicPlayer::getInstance().setVolume(bgm_volume_backup);
    }
    QDialog::closeEvent(event);
}

void SettingsDialog::LoadValuesFromConfig() {

    std::filesystem::path userdir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    std::error_code error;
    if (!std::filesystem::exists(userdir / "config.toml", error)) {
        Config::load(userdir / "config.toml");
        return;
    }

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        const toml::value data = toml::parse(userdir / "config.toml");
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    const toml::value data = toml::parse(userdir / "config.toml");
    const QVector<int> languageIndexes = {21, 23, 14, 6, 18, 1, 12, 22, 2, 4,  25, 24, 29, 5,  0, 9,
                                          15, 16, 17, 7, 26, 8, 11, 20, 3, 13, 27, 10, 19, 30, 28};

    const auto save_data_path = Config::GetSaveDataPath();
    QString save_data_path_string;
    Common::FS::PathToQString(save_data_path_string, save_data_path);
    ui->currentSaveDataPath->setText(save_data_path_string);

    ui->consoleLanguageComboBox->setCurrentIndex(
        std::distance(languageIndexes.begin(),
                      std::find(languageIndexes.begin(), languageIndexes.end(),
                                toml::find_or<int>(data, "Settings", "consoleLanguage", 6))) %
        languageIndexes.size());
    ui->emulatorLanguageComboBox->setCurrentIndex(
        languages[toml::find_or<std::string>(data, "GUI", "emulatorLanguage", "en_US")]);
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
    ui->enableHDRCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "allowHDR", false));
    ui->playBGMCheckBox->setChecked(toml::find_or<bool>(data, "General", "playBGM", false));
    ui->disableTrophycheckBox->setChecked(
        toml::find_or<bool>(data, "General", "isTrophyPopupDisabled", false));
    ui->popUpDurationSpinBox->setValue(Config::getTrophyNotificationDuration());

    QString side = QString::fromStdString(Config::sideTrophy());

    ui->radioButton_Left->setChecked(side == "left");
    ui->radioButton_Right->setChecked(side == "right");
    ui->radioButton_Top->setChecked(side == "top");
    ui->radioButton_Bottom->setChecked(side == "bottom");

    ui->BGMVolumeSlider->setValue(toml::find_or<int>(data, "General", "BGMvolume", 50));
    ui->discordRPCCheckbox->setChecked(
        toml::find_or<bool>(data, "General", "enableDiscordRPC", true));
    QString translatedText_FullscreenMode =
        screenModeMap.key(QString::fromStdString(Config::getFullscreenMode()));
    ui->displayModeComboBox->setCurrentText(translatedText_FullscreenMode);
    ui->separateUpdatesCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "separateUpdateEnabled", false));
    ui->gameSizeCheckBox->setChecked(toml::find_or<bool>(data, "GUI", "loadGameSizeEnabled", true));
    ui->showSplashCheckBox->setChecked(toml::find_or<bool>(data, "General", "showSplash", false));
    QString translatedText_logType = logTypeMap.key(QString::fromStdString(Config::getLogType()));
    if (!translatedText_logType.isEmpty()) {
        ui->logTypeComboBox->setCurrentText(translatedText_logType);
    }
    ui->logFilterLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "logFilter", "")));
    ui->userNameLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "userName", "shadPS4")));
    ui->trophyKeyLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "Keys", "TrophyKey", "")));
    ui->trophyKeyLineEdit->setEchoMode(QLineEdit::Password);
    ui->debugDump->setChecked(toml::find_or<bool>(data, "Debug", "DebugDump", false));
    ui->separateLogFilesCheckbox->setChecked(
        toml::find_or<bool>(data, "Debug", "isSeparateLogFilesEnabled", false));
    ui->vkValidationCheckBox->setChecked(toml::find_or<bool>(data, "Vulkan", "validation", false));
    ui->vkSyncValidationCheckBox->setChecked(
        toml::find_or<bool>(data, "Vulkan", "validation_sync", false));
    ui->rdocCheckBox->setChecked(toml::find_or<bool>(data, "Vulkan", "rdocEnable", false));
    ui->crashDiagnosticsCheckBox->setChecked(
        toml::find_or<bool>(data, "Vulkan", "crashDiagnostic", false));
    ui->guestMarkersCheckBox->setChecked(
        toml::find_or<bool>(data, "Vulkan", "guestMarkers", false));
    ui->hostMarkersCheckBox->setChecked(toml::find_or<bool>(data, "Vulkan", "hostMarkers", false));
    ui->copyGPUBuffersCheckBox->setChecked(
        toml::find_or<bool>(data, "GPU", "copyGPUBuffers", false));
    ui->collectShaderCheckBox->setChecked(
        toml::find_or<bool>(data, "Debug", "CollectShader", false));
    ui->enableCompatibilityCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "compatibilityEnabled", false));
    ui->checkCompatibilityOnStartupCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "checkCompatibilityOnStartup", false));

#ifdef ENABLE_UPDATER
    ui->updateCheckBox->setChecked(toml::find_or<bool>(data, "General", "autoUpdate", false));
    ui->changelogCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "alwaysShowChangelog", false));

    QString updateChannel = QString::fromStdString(Config::getUpdateChannel());
    ui->updateComboBox->setCurrentText(
        channelMap.key(updateChannel != "Release" && updateChannel != "Nightly"
                           ? (Common::isRelease ? "Release" : "Nightly")
                           : updateChannel));
#endif

    std::string chooseHomeTab =
        toml::find_or<std::string>(data, "General", "chooseHomeTab", "General");
    QString translatedText = chooseHomeTabMap.key(QString::fromStdString(chooseHomeTab));
    if (translatedText.isEmpty()) {
        translatedText = tr("General");
    }
    ui->chooseHomeTabComboBox->setCurrentText(translatedText);

    QStringList tabNames = {tr("General"), tr("GUI"),   tr("Graphics"), tr("User"),
                            tr("Input"),   tr("Paths"), tr("Debug")};
    int indexTab = tabNames.indexOf(translatedText);
    if (indexTab == -1)
        indexTab = 0;
    ui->tabWidgetSettings->setCurrentIndex(indexTab);

    QString backButtonBehavior = QString::fromStdString(
        toml::find_or<std::string>(data, "Input", "backButtonBehavior", "left"));
    int index = ui->backButtonBehaviorComboBox->findData(backButtonBehavior);
    ui->backButtonBehaviorComboBox->setCurrentIndex(index != -1 ? index : 0);
    ui->motionControlsCheckBox->setChecked(
        toml::find_or<bool>(data, "Input", "isMotionControlsEnabled", true));

    ui->removeFolderButton->setEnabled(!ui->gameFoldersListWidget->selectedItems().isEmpty());
    ResetInstallFolders();
    ui->backgroundImageOpacitySlider->setValue(Config::getBackgroundImageOpacity());
    ui->showBackgroundImageCheckBox->setChecked(Config::getShowBackgroundImage());

    backgroundImageOpacitySlider_backup = Config::getBackgroundImageOpacity();
    bgm_volume_backup = Config::getBGMvolume();
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

    connect(ui->emulatorLanguageComboBox, &QComboBox::currentIndexChanged, this,
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
    QString text;

    // clang-format off
    // General
    if (elementName == "consoleLanguageGroupBox") {
        text = tr("Console Language:\\nSets the language that the PS4 game uses.\\nIt's recommended to set this to a language the game supports, which will vary by region.");
    } else if (elementName == "emulatorLanguageGroupBox") {
        text = tr("Emulator Language:\\nSets the language of the emulator's user interface.");
    } else if (elementName == "separateUpdatesCheckBox") {
        text = tr("Enable Separate Update Folder:\\nEnables installing game updates into a separate folder for easy management.\\nThis can be manually created by adding the extracted update to the game folder with the name \"CUSA00000-UPDATE\" where the CUSA ID matches the game's ID.");
    } else if (elementName == "showSplashCheckBox") {
        text = tr("Show Splash Screen:\\nShows the game's splash screen (a special image) while the game is starting.");
    } else if (elementName == "discordRPCCheckbox") {
        text = tr("Enable Discord Rich Presence:\\nDisplays the emulator icon and relevant information on your Discord profile.");
    } else if (elementName == "userName") {
        text = tr("Username:\\nSets the PS4's account username, which may be displayed by some games.");
    } else if (elementName == "label_Trophy" || elementName == "trophyKeyLineEdit") {
        text = tr("Trophy Key:\\nKey used to decrypt trophies. Must be obtained from your jailbroken console.\\nMust contain only hex characters.");
    } else if (elementName == "logTypeGroupBox") {
        text = tr("Log Type:\\nSets whether to synchronize the output of the log window for performance. May have adverse effects on emulation.");
    } else if (elementName == "logFilter") {
        text = tr("Log Filter:\\nFilters the log to only print specific information.\\nExamples: \"Core:Trace\" \"Lib.Pad:Debug Common.Filesystem:Error\" \"*:Critical\"\\nLevels: Trace, Debug, Info, Warning, Error, Critical - in this order, a specific level silences all levels preceding it in the list and logs every level after it.");
    #ifdef ENABLE_UPDATER
    } else if (elementName == "updaterGroupBox") {
        text = tr("Update:\\nRelease: Official versions released every month that may be very outdated, but are more reliable and tested.\\nNightly: Development versions that have all the latest features and fixes, but may contain bugs and are less stable.");
#endif
    } else if (elementName == "GUIBackgroundImageGroupBox") {
        text = tr("Background Image:\\nControl the opacity of the game background image.");
    } else if (elementName == "GUIMusicGroupBox") {
        text = tr("Play Title Music:\\nIf a game supports it, enable playing special music when selecting the game in the GUI.");
    } else if (elementName == "enableHDRCheckBox") {
        text = tr("Enable HDR:\\nEnables HDR in games that support it.\\nYour monitor must have support for the BT2020 PQ color space and the RGB10A2 swapchain format.");
    } else if (elementName == "disableTrophycheckBox") {
        text = tr("Disable Trophy Pop-ups:\\nDisable in-game trophy notifications. Trophy progress can still be tracked using the Trophy Viewer (right-click the game in the main window).");
    } else if (elementName == "enableCompatibilityCheckBox") {
        text = tr("Display Compatibility Data:\\nDisplays game compatibility information in table view. Enable \"Update Compatibility On Startup\" to get up-to-date information.");
    } else if (elementName == "checkCompatibilityOnStartupCheckBox") {
        text = tr("Update Compatibility On Startup:\\nAutomatically update the compatibility database when shadPS4 starts.");
    } else if (elementName == "updateCompatibilityButton") {
        text = tr("Update Compatibility Database:\\nImmediately update the compatibility database.");
    }

    //User
    if (elementName == "OpenCustomTrophyLocationButton") {
        text = tr("Open the custom trophy images/sounds folder:\\nYou can add custom images to the trophies and an audio.\\nAdd the files to custom_trophy with the following names:\\ntrophy.wav OR trophy.mp3, bronze.png, gold.png, platinum.png, silver.png\\nNote: The sound will only work in QT versions.");
    }

    // Input
    if (elementName == "hideCursorGroupBox") {
        text = tr("Hide Cursor:\\nChoose when the cursor will disappear:\\nNever: You will always see the mouse.\\nidle: Set a time for it to disappear after being idle.\\nAlways: you will never see the mouse.");
    } else if (elementName == "idleTimeoutGroupBox") {
        text = tr("Hide Idle Cursor Timeout:\\nThe duration (seconds) after which the cursor that has been idle hides itself.");
    } else if (elementName == "backButtonBehaviorGroupBox") {
        text = tr("Back Button Behavior:\\nSets the controller's back button to emulate tapping the specified position on the PS4 touchpad.");
    }

    // Graphics
    if (elementName == "graphicsAdapterGroupBox") {
        text = tr("Graphics Device:\\nOn multiple GPU systems, select the GPU the emulator will use from the drop down list,\\nor select \"Auto Select\" to automatically determine it.");
    } else if (elementName == "windowSizeGroupBox") {
        text = tr("Width/Height:\\nSets the size of the emulator window at launch, which can be resized during gameplay.\\nThis is different from the in-game resolution.");
    } else if (elementName == "heightDivider") {
        text = tr("Vblank Divider:\\nThe frame rate at which the emulator refreshes at is multiplied by this number. Changing this may have adverse effects, such as increasing the game speed, or breaking critical game functionality that does not expect this to change!");
    } else if (elementName == "dumpShadersCheckBox") {
        text = tr("Enable Shaders Dumping:\\nFor the sake of technical debugging, saves the games shaders to a folder as they render.");
    } else if (elementName == "nullGpuCheckBox") {
        text = tr("Enable Null GPU:\\nFor the sake of technical debugging, disables game rendering as if there were no graphics card.");
    }

    // Path
    if (elementName == "gameFoldersGroupBox" || elementName == "gameFoldersListWidget") {
        text = tr("Game Folders:\\nThe list of folders to check for installed games.");
    } else if (elementName == "addFolderButton") {
        text = tr("Add:\\nAdd a folder to the list.");
    } else if (elementName == "removeFolderButton") {
        text = tr("Remove:\\nRemove a folder from the list.");
    } else if (elementName == "PortableUserFolderGroupBox") {
        text = tr("Portable user folder:\\nStores shadPS4 settings and data that will be applied only to the shadPS4 build located in the current folder. Restart the app after creating the portable user folder to begin using it.");
    }

    // Save Data
    if (elementName == "saveDataGroupBox" || elementName == "currentSaveDataPath") {
        text = tr("Save Data Path:\\nThe folder where game save data will be saved.");
    } else if (elementName == "browseButton") {
        text = tr("Browse:\\nBrowse for a folder to set as the save data path.");
    }

    // Debug
    if (elementName == "debugDump") {
        text = tr("Enable Debug Dumping:\\nSaves the import and export symbols and file header information of the currently running PS4 program to a directory.");
    } else if (elementName == "vkValidationCheckBox") {
        text = tr("Enable Vulkan Validation Layers:\\nEnables a system that validates the state of the Vulkan renderer and logs information about its internal state.\\nThis will reduce performance and likely change the behavior of emulation.");
    } else if (elementName == "vkSyncValidationCheckBox") {
        text = tr("Enable Vulkan Synchronization Validation:\\nEnables a system that validates the timing of Vulkan rendering tasks.\\nThis will reduce performance and likely change the behavior of emulation.");
    } else if (elementName == "rdocCheckBox") {
        text = tr("Enable RenderDoc Debugging:\\nIf enabled, the emulator will provide compatibility with Renderdoc to allow capture and analysis of the currently rendered frame.");
    } else if (elementName == "crashDiagnosticsCheckBox") {
        text = tr("Crash Diagnostics:\\nCreates a .yaml file with info about the Vulkan state at the time of crashing.\\nUseful for debugging 'Device lost' errors. If you have this enabled, you should enable Host AND Guest Debug Markers.\\nDoes not work on Intel GPUs.\\nYou need Vulkan Validation Layers enabled and the Vulkan SDK for this to work.");
    } else if (elementName == "guestMarkersCheckBox") {
        text = tr("Guest Debug Markers:\\nInserts any debug markers the game itself has added to the command buffer.\\nIf you have this enabled, you should enable Crash Diagnostics.\\nUseful for programs like RenderDoc.");
    } else if (elementName == "hostMarkersCheckBox") {
        text = tr("Host Debug Markers:\\nInserts emulator-side information like markers for specific AMDGPU commands around Vulkan commands, as well as giving resources debug names.\\nIf you have this enabled, you should enable Crash Diagnostics.\\nUseful for programs like RenderDoc.");
    } else if (elementName == "copyGPUBuffersCheckBox") {
        text = tr("Copy GPU Buffers:\\nGets around race conditions involving GPU submits.\\nMay or may not help with PM4 type 0 crashes.");
    } else if (elementName == "collectShaderCheckBox") {
        text = tr("Collect Shaders:\\nYou need this enabled to edit shaders with the debug menu (Ctrl + F10).");
    } else if (elementName == "separateLogFilesCheckbox") {
        text = tr("Separate Log Files:\\nWrites a separate logfile for each game.");}
    // clang-format on
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
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void SettingsDialog::UpdateSettings() {

    const QVector<std::string> TouchPadIndex = {"left", "center", "right", "none"};
    Config::setBackButtonBehavior(TouchPadIndex[ui->backButtonBehaviorComboBox->currentIndex()]);
    Config::setIsFullscreen(screenModeMap.value(ui->displayModeComboBox->currentText()) !=
                            "Windowed");
    Config::setFullscreenMode(
        screenModeMap.value(ui->displayModeComboBox->currentText()).toStdString());
    Config::setIsMotionControlsEnabled(ui->motionControlsCheckBox->isChecked());
    Config::setisTrophyPopupDisabled(ui->disableTrophycheckBox->isChecked());
    Config::setTrophyNotificationDuration(ui->popUpDurationSpinBox->value());

    if (ui->radioButton_Top->isChecked()) {
        Config::setSideTrophy("top");
    } else if (ui->radioButton_Left->isChecked()) {
        Config::setSideTrophy("left");
    } else if (ui->radioButton_Right->isChecked()) {
        Config::setSideTrophy("right");
    } else if (ui->radioButton_Bottom->isChecked()) {
        Config::setSideTrophy("bottom");
    }

    Config::setPlayBGM(ui->playBGMCheckBox->isChecked());
    Config::setAllowHDR(ui->enableHDRCheckBox->isChecked());
    Config::setLogType(logTypeMap.value(ui->logTypeComboBox->currentText()).toStdString());
    Config::setLogFilter(ui->logFilterLineEdit->text().toStdString());
    Config::setUserName(ui->userNameLineEdit->text().toStdString());
    Config::setTrophyKey(ui->trophyKeyLineEdit->text().toStdString());
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
    Config::setLoadGameSizeEnabled(ui->gameSizeCheckBox->isChecked());
    Config::setShowSplash(ui->showSplashCheckBox->isChecked());
    Config::setDebugDump(ui->debugDump->isChecked());
    Config::setSeparateLogFilesEnabled(ui->separateLogFilesCheckbox->isChecked());
    Config::setVkValidation(ui->vkValidationCheckBox->isChecked());
    Config::setVkSyncValidation(ui->vkSyncValidationCheckBox->isChecked());
    Config::setRdocEnabled(ui->rdocCheckBox->isChecked());
    Config::setVkHostMarkersEnabled(ui->hostMarkersCheckBox->isChecked());
    Config::setVkGuestMarkersEnabled(ui->guestMarkersCheckBox->isChecked());
    Config::setVkCrashDiagnosticEnabled(ui->crashDiagnosticsCheckBox->isChecked());
    Config::setCollectShaderForDebug(ui->collectShaderCheckBox->isChecked());
    Config::setCopyGPUCmdBuffers(ui->copyGPUBuffersCheckBox->isChecked());
    Config::setAutoUpdate(ui->updateCheckBox->isChecked());
    Config::setAlwaysShowChangelog(ui->changelogCheckBox->isChecked());
    Config::setUpdateChannel(channelMap.value(ui->updateComboBox->currentText()).toStdString());
    Config::setChooseHomeTab(
        chooseHomeTabMap.value(ui->chooseHomeTabComboBox->currentText()).toStdString());
    Config::setCompatibilityEnabled(ui->enableCompatibilityCheckBox->isChecked());
    Config::setCheckCompatibilityOnStartup(ui->checkCompatibilityOnStartupCheckBox->isChecked());
    Config::setBackgroundImageOpacity(ui->backgroundImageOpacitySlider->value());
    emit BackgroundOpacityChanged(ui->backgroundImageOpacitySlider->value());
    Config::setShowBackgroundImage(ui->showBackgroundImageCheckBox->isChecked());

    std::vector<Config::GameInstallDir> dirs_with_states;
    for (int i = 0; i < ui->gameFoldersListWidget->count(); i++) {
        QListWidgetItem* item = ui->gameFoldersListWidget->item(i);
        QString path_string = item->text();
        auto path = Common::FS::PathFromQString(path_string);
        bool enabled = (item->checkState() == Qt::Checked);

        dirs_with_states.push_back({path, enabled});
    }
    Config::setAllGameInstallDirs(dirs_with_states);

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
}

void SettingsDialog::ResetInstallFolders() {
    ui->gameFoldersListWidget->clear();

    std::filesystem::path userdir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    const toml::value data = toml::parse(userdir / "config.toml");

    if (data.contains("GUI")) {
        const toml::value& gui = data.at("GUI");
        const auto install_dir_array =
            toml::find_or<std::vector<std::u8string>>(gui, "installDirs", {});

        std::vector<bool> install_dirs_enabled;
        try {
            install_dirs_enabled = Config::getGameInstallDirsEnabled();
        } catch (...) {
            // If it does not exist, assume that all are enabled.
            install_dirs_enabled.resize(install_dir_array.size(), true);
        }

        if (install_dirs_enabled.size() < install_dir_array.size()) {
            install_dirs_enabled.resize(install_dir_array.size(), true);
        }

        std::vector<Config::GameInstallDir> settings_install_dirs_config;

        for (size_t i = 0; i < install_dir_array.size(); i++) {
            std::filesystem::path dir = install_dir_array[i];
            bool enabled = install_dirs_enabled[i];

            settings_install_dirs_config.push_back({dir, enabled});

            QString path_string;
            Common::FS::PathToQString(path_string, dir);

            QListWidgetItem* item = new QListWidgetItem(path_string);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
            ui->gameFoldersListWidget->addItem(item);
        }

        Config::setAllGameInstallDirs(settings_install_dirs_config);
    }
}
