// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <QCompleter>
#include <QDirIterator>
#include <QFileDialog>
#include <QHoverEvent>
#include <QMessageBox>
#include <SDL3/SDL.h>
#include <fmt/format.h>

#include "common/config.h"
#include "common/scm_rev.h"
#include "core/libraries/audio/audioout.h"
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
#include "log_presets_dialog.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_presenter.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

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
QMap<QString, QString> presentModeMap;
QMap<QString, QString> chooseHomeTabMap;
QMap<QString, QString> micMap;

int backgroundImageOpacitySlider_backup;
int bgm_volume_backup;

static std::vector<QString> m_physical_devices;

SettingsDialog::SettingsDialog(std::shared_ptr<gui_settings> gui_settings,
                               std::shared_ptr<CompatibilityInfoClass> m_compat_info,
                               QWidget* parent, bool is_game_specific, std::string gsc_serial)
    : QDialog(parent), ui(new Ui::SettingsDialog), m_gui_settings(std::move(gui_settings)),
      game_specific(is_game_specific), gs_serial(gsc_serial) {

    ui->setupUi(this);
    ui->tabWidgetSettings->setUsesScrollButtons(false);
    initialHeight = this->height();

    // Add a small clear "x" button inside the Log Filter input
    ui->logFilterLineEdit->setClearButtonEnabled(true);

    if (game_specific) {
        // Paths tab
        ui->tabWidgetSettings->setTabVisible(5, false);
        ui->chooseHomeTabComboBox->removeItem(5);

        // Frontend tab
        ui->tabWidgetSettings->setTabVisible(1, false);
        ui->chooseHomeTabComboBox->removeItem(1);

    } else {
        // Experimental tab
        ui->tabWidgetSettings->setTabVisible(8, false);
        ui->chooseHomeTabComboBox->removeItem(8);
    }

    std::filesystem::path config_file =
        game_specific
            ? Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (gs_serial + ".toml")
            : Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml";

    ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();

    channelMap = {{tr("Release"), "Release"}, {tr("Nightly"), "Nightly"}};
    logTypeMap = {{tr("async"), "async"}, {tr("sync"), "sync"}};
    screenModeMap = {{tr("Fullscreen (Borderless)"), "Fullscreen (Borderless)"},
                     {tr("Windowed"), "Windowed"},
                     {tr("Fullscreen"), "Fullscreen"}};
    presentModeMap = {{tr("Mailbox (Vsync)"), "Mailbox"},
                      {tr("Fifo (Vsync)"), "Fifo"},
                      {tr("Immediate (No Vsync)"), "Immediate"}};
    chooseHomeTabMap = {{tr("General"), "General"},
                        {tr("Frontend"), "Frontend"},
                        {tr("Graphics"), "Graphics"},
                        {tr("User"), "User"},
                        {tr("Input"), "Input"},
                        {tr("Paths"), "Paths"},
                        {tr("Log"), "Log"},
                        {tr("Debug"), "Debug"},
                        {tr("Experimental"), "Experimental"}};
    micMap = {{tr("None"), "None"}, {tr("Default Device"), "Default Device"}};

    if (m_physical_devices.empty()) {
        // Populate cache of physical devices.
        Vulkan::Instance instance(false, false);
        auto physical_devices = instance.GetPhysicalDevices();
        for (const vk::PhysicalDevice physical_device : physical_devices) {
            auto prop = physical_device.getProperties();
            QString name = QString::fromUtf8(prop.deviceName, -1);
            if (prop.apiVersion < Vulkan::TargetVulkanApiVersion) {
                name += tr(" * Unsupported Vulkan Version");
            }
            m_physical_devices.push_back(name);
        }
    }

    // Add list of available GPUs
    ui->graphicsAdapterBox->addItem(tr("Auto Select")); // -1, auto selection
    for (const auto& device : m_physical_devices) {
        ui->graphicsAdapterBox->addItem(device);
    }

    ui->consoleLanguageComboBox->addItems(languageNames);

    QCompleter* completer = new QCompleter(languageNames, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->consoleLanguageComboBox->setCompleter(completer);

    ui->hideCursorComboBox->addItem(tr("Never"));
    ui->hideCursorComboBox->addItem(tr("Idle"));
    ui->hideCursorComboBox->addItem(tr("Always"));

    ui->micComboBox->addItem(micMap.key("None"), "None");
    ui->micComboBox->addItem(micMap.key("Default Device"), "Default Device");
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    int count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&count);
    if (devices) {
        for (int i = 0; i < count; ++i) {
            SDL_AudioDeviceID devId = devices[i];
            const char* name = SDL_GetAudioDeviceName(devId);
            if (name) {
                QString qname = QString::fromUtf8(name);
                ui->micComboBox->addItem(qname, QString::number(devId));
            }
        }
        SDL_free(devices);
    } else {
        qDebug() << "Erro SDL_GetAudioRecordingDevices:" << SDL_GetError();
    }

    InitializeEmulatorLanguages();
    LoadValuesFromConfig();

    defaultTextEdit = tr("Point your mouse at an option to display its description.");
    ui->descriptionText->setText(defaultTextEdit);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this,
            [this, config_file](QAbstractButton* button) {
                if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
                    is_saving = true;
                    UpdateSettings(game_specific);
                    Config::save(config_file, game_specific);
                    QWidget::close();
                } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
                    UpdateSettings(game_specific);
                    Config::save(config_file, game_specific);
                } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
                    setDefaultValues();
                    Config::setDefaultValues(game_specific);
                    Config::save(config_file, game_specific);
                    LoadValuesFromConfig();
                } else if (button == ui->buttonBox->button(QDialogButtonBox::Close)) {
                    ui->backgroundImageOpacitySlider->setValue(backgroundImageOpacitySlider_backup);
                    emit BackgroundOpacityChanged(backgroundImageOpacitySlider_backup);
                    ui->BGMVolumeSlider->setValue(bgm_volume_backup);
                    BackgroundMusicPlayer::getInstance().setVolume(bgm_volume_backup);
                    SyncRealTimeWidgetstoConfig();
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
        connect(ui->horizontalVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
            VolumeSliderChange(value);
            Config::setVolumeSlider(value, game_specific);
            Libraries::AudioOut::AdjustVol();
        });

#ifdef ENABLE_UPDATER
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->updateCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            m_gui_settings->SetValue(gui::gen_checkForUpdates, state == Qt::Checked);
        });

        connect(ui->changelogCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            m_gui_settings->SetValue(gui::gen_showChangeLog, state == Qt::Checked);
        });
#else
        connect(ui->updateCheckBox, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState state) {
                    m_gui_settings->SetValue(gui::gen_checkForUpdates, state == Qt::Checked);
                });

        connect(ui->changelogCheckBox, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState state) {
                    m_gui_settings->SetValue(gui::gen_showChangeLog, state == Qt::Checked);
                });
#endif

        connect(ui->updateComboBox, &QComboBox::currentTextChanged, this,
                [this](const QString& channel) {
                    if (channelMap.contains(channel)) {
                        m_gui_settings->SetValue(gui::gen_updateChannel, channelMap.value(channel));
                    }
                });

        connect(ui->checkUpdateButton, &QPushButton::clicked, this, [this]() {
            auto checkUpdate = new CheckUpdate(m_gui_settings, true);
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

    // GUI TAB
    {
        connect(ui->backgroundImageOpacitySlider, &QSlider::valueChanged, this,
                [this](int value) { emit BackgroundOpacityChanged(value); });

        connect(ui->BGMVolumeSlider, &QSlider::valueChanged, this,
                [](int value) { BackgroundMusicPlayer::getInstance().setVolume(value); });

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->showBackgroundImageCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
#else
        connect(ui->showBackgroundImageCheckBox, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState state) {
#endif
            m_gui_settings->SetValue(gui::gl_showBackgroundImage, state == Qt::Checked);
        });
    }

    // USER TAB
    {
        connect(ui->OpenCustomTrophyLocationButton, &QPushButton::clicked, this, []() {
            QString userPath;
            Common::FS::PathToQString(userPath,
                                      Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy));
            QDesktopServices::openUrl(QUrl::fromLocalFile(userPath));
        });
    }

    // INPUT TAB
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

        connect(ui->folderButton, &QPushButton::clicked, this, [this]() {
            const auto dlc_folder_path = Config::getAddonInstallDir();
            QString initial_path;
            Common::FS::PathToQString(initial_path, dlc_folder_path);

            QString dlc_folder_path_string =
                QFileDialog::getExistingDirectory(this, tr("Select the DLC folder"), initial_path);

            auto file_path = Common::FS::PathFromQString(dlc_folder_path_string);
            if (!file_path.empty()) {
                Config::setAddonInstallDir(file_path);
                ui->currentDLCFolder->setText(dlc_folder_path_string);
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

        // Log presets popup button
        connect(ui->logPresetsButton, &QPushButton::clicked, this, [this]() {
            auto dlg = new LogPresetsDialog(m_gui_settings, this);
            connect(dlg, &LogPresetsDialog::PresetChosen, this,
                    [this](const QString& filter) { ui->logFilterLineEdit->setText(filter); });
            dlg->exec();
        });
    }

    // GRAPHICS TAB
    connect(ui->RCASSlider, &QSlider::valueChanged, this, [this](int value) {
        QString RCASValue = QString::number(value / 1000.0, 'f', 3);
        ui->RCASValue->setText(RCASValue);
    });

    if (presenter) {
        connect(ui->RCASSlider, &QSlider::valueChanged, this, [this](int value) {
            presenter->GetFsrSettingsRef().rcas_attenuation = static_cast<float>(value / 1000.0f);
        });

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
        connect(ui->FSRCheckBox, &QCheckBox::stateChanged, this,
                [this](int state) { presenter->GetFsrSettingsRef().enable = state; });

        connect(ui->RCASCheckBox, &QCheckBox::stateChanged, this,
                [this](int state) { presenter->GetFsrSettingsRef().use_rcas = state; });
#else
        connect(ui->FSRCheckBox, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState state) { presenter->GetFsrSettingsRef().enable = state; });

        connect(ui->RCASCheckBox, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState state) { presenter->GetFsrSettingsRef().use_rcas = state; });
#endif
    }

    // Descriptions
    {
        // General
        ui->consoleLanguageGroupBox->installEventFilter(this);
        ui->emulatorLanguageGroupBox->installEventFilter(this);
        ui->showSplashCheckBox->installEventFilter(this);
        ui->discordRPCCheckbox->installEventFilter(this);
        ui->volumeSliderElement->installEventFilter(this);
#ifdef ENABLE_UPDATER
        ui->updaterGroupBox->installEventFilter(this);
#endif

        // GUI
        ui->GUIBackgroundImageGroupBox->installEventFilter(this);
        ui->GUIMusicGroupBox->installEventFilter(this);
        ui->enableCompatibilityCheckBox->installEventFilter(this);
        ui->checkCompatibilityOnStartupCheckBox->installEventFilter(this);
        ui->updateCompatibilityButton->installEventFilter(this);

        // User
        ui->userName->installEventFilter(this);
        ui->disableTrophycheckBox->installEventFilter(this);
        ui->OpenCustomTrophyLocationButton->installEventFilter(this);
        ui->label_Trophy->installEventFilter(this);
        ui->trophyKeyLineEdit->installEventFilter(this);

        // Input
        ui->hideCursorGroupBox->installEventFilter(this);
        ui->idleTimeoutGroupBox->installEventFilter(this);
        ui->backgroundControllerCheckBox->installEventFilter(this);
        ui->motionControlsCheckBox->installEventFilter(this);
        ui->micComboBox->installEventFilter(this);

        // Graphics
        ui->graphicsAdapterGroupBox->installEventFilter(this);
        ui->windowSizeGroupBox->installEventFilter(this);
        ui->presentModeGroupBox->installEventFilter(this);
        ui->heightDivider->installEventFilter(this);
        ui->nullGpuCheckBox->installEventFilter(this);
        ui->enableHDRCheckBox->installEventFilter(this);
        ui->chooseHomeTabGroupBox->installEventFilter(this);
        ui->gameSizeCheckBox->installEventFilter(this);

        // Paths
        ui->gameFoldersGroupBox->installEventFilter(this);
        ui->gameFoldersListWidget->installEventFilter(this);
        ui->addFolderButton->installEventFilter(this);
        ui->removeFolderButton->installEventFilter(this);
        ui->saveDataGroupBox->installEventFilter(this);
        ui->currentSaveDataPath->installEventFilter(this);
        ui->currentDLCFolder->installEventFilter(this);
        ui->browseButton->installEventFilter(this);
        ui->folderButton->installEventFilter(this);
        ui->PortableUserFolderGroupBox->installEventFilter(this);

        // Log
        ui->logTypeGroupBox->installEventFilter(this);
        ui->logFilter->installEventFilter(this);
        ui->enableLoggingCheckBox->installEventFilter(this);
        ui->separateLogFilesCheckbox->installEventFilter(this);
        ui->OpenLogLocationButton->installEventFilter(this);

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
        ui->readbacksCheckBox->installEventFilter(this);
        ui->readbackLinearImagesCheckBox->installEventFilter(this);
        ui->dumpShadersCheckBox->installEventFilter(this);
        ui->dmaCheckBox->installEventFilter(this);
        ui->devkitCheckBox->installEventFilter(this);
        ui->neoCheckBox->installEventFilter(this);
        ui->networkConnectedCheckBox->installEventFilter(this);
        ui->psnSignInCheckBox->installEventFilter(this);
    }
}

void SettingsDialog::closeEvent(QCloseEvent* event) {
    if (!is_saving) {
        ui->backgroundImageOpacitySlider->setValue(backgroundImageOpacitySlider_backup);
        emit BackgroundOpacityChanged(backgroundImageOpacitySlider_backup);
        ui->BGMVolumeSlider->setValue(bgm_volume_backup);
        BackgroundMusicPlayer::getInstance().setVolume(bgm_volume_backup);
        SyncRealTimeWidgetstoConfig();
    }
    QDialog::closeEvent(event);
}

void SettingsDialog::LoadValuesFromConfig() {
    std::filesystem::path config_file;
    config_file =
        game_specific
            ? Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (gs_serial + ".toml")
            : Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml";

    std::error_code error;
    bool is_newly_created = false;
    if (!std::filesystem::exists(config_file, error)) {
        Config::save(config_file, game_specific);
        is_newly_created = true;
    }

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        const toml::value data = toml::parse(config_file);
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    const toml::value data = toml::parse(config_file);
    const QVector<int> languageIndexes = {21, 23, 14, 6, 18, 1, 12, 22, 2, 4,  25, 24, 29, 5,  0, 9,
                                          15, 16, 17, 7, 26, 8, 11, 20, 3, 13, 27, 10, 19, 30, 28};

    // Entries with no game-specific settings
    if (!game_specific) {
        const auto save_data_path = Config::GetSaveDataPath();
        QString save_data_path_string;
        Common::FS::PathToQString(save_data_path_string, save_data_path);
        ui->currentSaveDataPath->setText(save_data_path_string);

        const auto dlc_folder_path = Config::getAddonInstallDir();
        QString dlc_folder_path_string;
        Common::FS::PathToQString(dlc_folder_path_string, dlc_folder_path);
        ui->currentDLCFolder->setText(dlc_folder_path_string);

        ui->emulatorLanguageComboBox->setCurrentIndex(
            languages[m_gui_settings->GetValue(gui::gen_guiLanguage).toString().toStdString()]);

        ui->playBGMCheckBox->setChecked(
            m_gui_settings->GetValue(gui::gl_playBackgroundMusic).toBool());

        ui->RCASSlider->setValue(toml::find_or<int>(data, "GPU", "rcasAttenuation", 250));
        ui->RCASValue->setText(QString::number(ui->RCASSlider->value() / 1000.0, 'f', 3));

        ui->BGMVolumeSlider->setValue(
            m_gui_settings->GetValue(gui::gl_backgroundMusicVolume).toInt());
        ui->discordRPCCheckbox->setChecked(
            toml::find_or<bool>(data, "General", "enableDiscordRPC", true));

        ui->gameSizeCheckBox->setChecked(
            toml::find_or<bool>(data, "GUI", "loadGameSizeEnabled", true));
        ui->trophyKeyLineEdit->setText(
            QString::fromStdString(toml::find_or<std::string>(data, "Keys", "TrophyKey", "")));
        ui->trophyKeyLineEdit->setEchoMode(QLineEdit::Password);
        ui->enableCompatibilityCheckBox->setChecked(
            toml::find_or<bool>(data, "General", "compatibilityEnabled", false));
        ui->checkCompatibilityOnStartupCheckBox->setChecked(
            toml::find_or<bool>(data, "General", "checkCompatibilityOnStartup", false));

        ui->removeFolderButton->setEnabled(!ui->gameFoldersListWidget->selectedItems().isEmpty());
        ui->backgroundImageOpacitySlider->setValue(
            m_gui_settings->GetValue(gui::gl_backgroundImageOpacity).toInt());
        ui->showBackgroundImageCheckBox->setChecked(
            m_gui_settings->GetValue(gui::gl_showBackgroundImage).toBool());

        backgroundImageOpacitySlider_backup =
            m_gui_settings->GetValue(gui::gl_backgroundImageOpacity).toInt();
        bgm_volume_backup = m_gui_settings->GetValue(gui::gl_backgroundMusicVolume).toInt();

#ifdef ENABLE_UPDATER
        ui->updateCheckBox->setChecked(m_gui_settings->GetValue(gui::gen_checkForUpdates).toBool());
        ui->changelogCheckBox->setChecked(
            m_gui_settings->GetValue(gui::gen_showChangeLog).toBool());

        QString updateChannel = m_gui_settings->GetValue(gui::gen_updateChannel).toString();
        ui->updateComboBox->setCurrentText(
            channelMap.key(updateChannel != "Release" && updateChannel != "Nightly"
                               ? (Common::g_is_release ? "Release" : "Nightly")
                               : updateChannel));
#endif

        SyncRealTimeWidgetstoConfig();
    }

    // Entries with game-specific settings, *load these from toml file, not from Config::get*
    ui->consoleLanguageComboBox->setCurrentIndex(
        std::distance(languageIndexes.begin(),
                      std::find(languageIndexes.begin(), languageIndexes.end(),
                                toml::find_or<int>(data, "Settings", "consoleLanguage", 6))) %
        languageIndexes.size());

    std::string micDevice =
        toml::find_or<std::string>(data, "Input", "micDevice", "Default Device");
    QString micValue = QString::fromStdString(micDevice);
    int micIndex = ui->micComboBox->findData(micValue);
    if (micIndex != -1) {
        ui->micComboBox->setCurrentIndex(micIndex);
    } else {
        ui->micComboBox->setCurrentIndex(0);
    }

    ui->readbacksCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "readbacks", false));
    ui->readbackLinearImagesCheckBox->setChecked(
        toml::find_or<bool>(data, "GPU", "readbackLinearImages", false));
    ui->dmaCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "directMemoryAccess", false));
    ui->neoCheckBox->setChecked(toml::find_or<bool>(data, "General", "isPS4Pro", false));
    ui->devkitCheckBox->setChecked(toml::find_or<bool>(data, "General", "isDevKit", false));
    ui->networkConnectedCheckBox->setChecked(
        toml::find_or<bool>(data, "General", "isConnectedToNetwork", false));
    ui->psnSignInCheckBox->setChecked(toml::find_or<bool>(data, "General", "isPSNSignedIn", false));

    // First options is auto selection -1, so gpuId on the GUI will always have to subtract 1
    // when setting and add 1 when getting to select the correct gpu in Qt
    ui->graphicsAdapterBox->setCurrentIndex(toml::find_or<int>(data, "Vulkan", "gpuId", -1) + 1);
    ui->widthSpinBox->setValue(toml::find_or<int>(data, "GPU", "screenWidth", 1280));
    ui->heightSpinBox->setValue(toml::find_or<int>(data, "GPU", "screenHeight", 720));
    ui->vblankSpinBox->setValue(toml::find_or<int>(data, "GPU", "vblankFrequency", 60));
    ui->dumpShadersCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "dumpShaders", false));
    ui->nullGpuCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "nullGpu", false));
    ui->enableHDRCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "allowHDR", false));
    ui->FSRCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "fsrEnabled", true));
    ui->RCASCheckBox->setChecked(toml::find_or<bool>(data, "GPU", "rcasEnabled", true));
    ui->RCASSlider->setValue(toml::find_or<int>(data, "GPU", "rcasAttenuation", 250));
    ui->RCASValue->setText(QString::number(ui->RCASSlider->value() / 1000.0, 'f', 3));
    ui->disableTrophycheckBox->setChecked(
        toml::find_or<bool>(data, "General", "isTrophyPopupDisabled", false));
    ui->popUpDurationSpinBox->setValue(
        toml::find_or<double>(data, "General", "trophyNotificationDuration", 6.0));
    ui->showSplashCheckBox->setChecked(toml::find_or<bool>(data, "General", "showSplash", false));
    ui->hideCursorComboBox->setCurrentIndex(toml::find_or<int>(data, "Input", "cursorState", 1));
    OnCursorStateChanged(toml::find_or<int>(data, "Input", "cursorState", 1));
    ui->idleTimeoutSpinBox->setValue(toml::find_or<int>(data, "Input", "cursorHideTimeout", 5));
    ui->motionControlsCheckBox->setChecked(
        toml::find_or<bool>(data, "Input", "isMotionControlsEnabled", true));
    ui->backgroundControllerCheckBox->setChecked(
        toml::find_or<bool>(data, "Input", "backgroundControllerInput", false));

    std::string sideTrophy = toml::find_or<std::string>(data, "General", "sideTrophy", "right");
    QString side = QString::fromStdString(sideTrophy);
    ui->radioButton_Left->setChecked(side == "left");
    ui->radioButton_Right->setChecked(side == "right");
    ui->radioButton_Top->setChecked(side == "top");
    ui->radioButton_Bottom->setChecked(side == "bottom");

    ui->horizontalVolumeSlider->setValue(toml::find_or<int>(data, "General", "volumeSlider", 100));
    ui->volumeText->setText(QString::number(ui->horizontalVolumeSlider->sliderPosition()) + "%");

    std::string fullScreenMode =
        toml::find_or<std::string>(data, "GPU", "FullscreenMode", "Windowed");
    QString translatedText_FullscreenMode =
        screenModeMap.key(QString::fromStdString(fullScreenMode));
    ui->displayModeComboBox->setCurrentText(translatedText_FullscreenMode);

    std::string presentMode = toml::find_or<std::string>(data, "GPU", "presentMode", "Mailbox");
    QString translatedText_PresentMode = presentModeMap.key(QString::fromStdString(presentMode));
    ui->presentModeComboBox->setCurrentText(translatedText_PresentMode);

    std::string logType = toml::find_or<std::string>(data, "General", "logType", "sync");
    QString translatedText_logType = logTypeMap.key(QString::fromStdString(logType));
    if (!translatedText_logType.isEmpty()) {
        ui->logTypeComboBox->setCurrentText(translatedText_logType);
    }
    ui->logFilterLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "logFilter", "")));
    ui->userNameLineEdit->setText(
        QString::fromStdString(toml::find_or<std::string>(data, "General", "userName", "shadPS4")));

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
    ui->enableLoggingCheckBox->setChecked(toml::find_or<bool>(data, "Debug", "logEnabled", true));

    std::string chooseHomeTab =
        toml::find_or<std::string>(data, "General", "chooseHomeTab", "General");
    QString translatedText = chooseHomeTabMap.key(QString::fromStdString(chooseHomeTab));
    if (translatedText.isEmpty()) {
        translatedText = tr("General");
    }
    ui->chooseHomeTabComboBox->setCurrentText(translatedText);

    QStringList tabNames = {tr("General"), tr("GUI"),   tr("Graphics"),
                            tr("User"),    tr("Input"), tr("Paths"),
                            tr("Log"),     tr("Debug"), tr("Experimental")};
    int indexTab = tabNames.indexOf(translatedText);
    if (indexTab == -1 || !ui->tabWidgetSettings->isTabVisible(indexTab) || is_newly_created)
        indexTab = 0;
    ui->tabWidgetSettings->setCurrentIndex(indexTab);
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

    emit LanguageChanged(ui->emulatorLanguageComboBox->itemData(index).toString());
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

void SettingsDialog::VolumeSliderChange(int value) {
    ui->volumeText->setText(QString::number(ui->horizontalVolumeSlider->sliderPosition()) + "%");
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
    } else if (elementName == "backgroundControllerCheckBox") {
        text = tr("Enable Controller Background Input:\\nAllow shadPS4 to detect controller inputs when the game window is not in focus.");
    }

    // Graphics
    if (elementName == "graphicsAdapterGroupBox") {
        text = tr("Graphics Device:\\nOn multiple GPU systems, select the GPU the emulator will use from the drop down list,\\nor select \"Auto Select\" to automatically determine it.");
    } else if (elementName == "presentModeGroupBox") {
        text = tr("Present Mode:\\nConfigures how video output will be presented to your screen.\\n\\n"
                  "Mailbox: Frames synchronize with your screen's refresh rate. New frames will replace any pending frames. Reduces latency but may skip frames if running behind.\\n"
                  "Fifo: Frames synchronize with your screen's refresh rate. New frames will be queued behind pending frames. Ensures all frames are presented but may increase latency.\\n"
                  "Immediate: Frames immediately present to your screen when ready. May result in tearing.");
    } else if (elementName == "windowSizeGroupBox") {
        text = tr("Width/Height:\\nSets the size of the emulator window at launch, which can be resized during gameplay.\\nThis is different from the in-game resolution.");
    } else if (elementName == "heightDivider") {
        text = tr("Vblank Frequency:\\nThe frame rate at which the emulator refreshes at (60hz is the baseline, whether the game runs at 30 or 60fps). Changing this may have adverse effects, such as increasing the game speed, or breaking critical game functionality that does not expect this to change!");
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

    // DLC Folder
    if (elementName == "dlcFolderGroupBox" || elementName == "currentDLCFolder") {
        text = tr("DLC Path:\\nThe folder where game DLC loaded from.");
    } else if (elementName == "folderButton") {
        text = tr("Browse:\\nBrowse for a folder to set as the DLC path.");
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
        text = tr("Enable Vulkan Validation Layers:\\nEnables a system that validates the state of the Vulkan renderer and logs information about its internal state.\\nThis will reduce performance and likely change the behavior of emulation.\\nYou need the Vulkan SDK for this to work.");
    } else if (elementName == "vkSyncValidationCheckBox") {
        text = tr("Enable Vulkan Synchronization Validation:\\nEnables a system that validates the timing of Vulkan rendering tasks.\\nThis will reduce performance and likely change the behavior of emulation.\\nYou need the Vulkan SDK for this to work.");
    } else if (elementName == "rdocCheckBox") {
        text = tr("Enable RenderDoc Debugging:\\nIf enabled, the emulator will provide compatibility with Renderdoc to allow capture and analysis of the currently rendered frame.");
    } else if (elementName == "crashDiagnosticsCheckBox") {
        text = tr("Crash Diagnostics:\\nCreates a .yaml file with info about the Vulkan state at the time of crashing.\\nUseful for debugging 'Device lost' errors. If you have this enabled, you should enable Host AND Guest Debug Markers.\\nYou need Vulkan Validation Layers enabled and the Vulkan SDK for this to work.");
    } else if (elementName == "guestMarkersCheckBox") {
        text = tr("Guest Debug Markers:\\nInserts any debug markers the game itself has added to the command buffer.\\nIf you have this enabled, you should enable Crash Diagnostics.\\nUseful for programs like RenderDoc.");
    } else if (elementName == "hostMarkersCheckBox") {
        text = tr("Host Debug Markers:\\nInserts emulator-side information like markers for specific AMDGPU commands around Vulkan commands, as well as giving resources debug names.\\nIf you have this enabled, you should enable Crash Diagnostics.\\nUseful for programs like RenderDoc.");
    } else if (elementName == "copyGPUBuffersCheckBox") {
        text = tr("Copy GPU Buffers:\\nGets around race conditions involving GPU submits.\\nMay or may not help with PM4 type 0 crashes.");
    } else if (elementName == "collectShaderCheckBox") {
        text = tr("Collect Shaders:\\nYou need this enabled to edit shaders with the debug menu (Ctrl + F10).");
    } else if (elementName == "readbacksCheckBox") {
        text = tr("Enable Readbacks:\\nEnable GPU memory readbacks and writebacks.\\nThis is required for proper behavior in some games.\\nMight cause stability and/or performance issues.");
    } else if (elementName == "readbackLinearImagesCheckBox") {
        text = tr("Enable Readback Linear Images:\\nEnables async downloading of GPU modified linear images.\\nMight fix issues in some games.");
    } else if (elementName == "separateLogFilesCheckbox") {
        text = tr("Separate Log Files:\\nWrites a separate logfile for each game.");
    } else if (elementName == "enableLoggingCheckBox") {
        text = tr("Enable Logging:\\nEnables logging.\\nDo not change this if you do not know what you're doing!\\nWhen asking for help, make sure this setting is ENABLED.");
    } else if (elementName == "OpenLogLocationButton") {
        text = tr("Open Log Location:\\nOpen the folder where the log file is saved.");
    } else if (elementName == "micComboBox") {
        text = tr("Microphone:\\nNone: Does not use the microphone.\\nDefault Device: Will use the default device defined in the system.\\nOr manually choose the microphone to be used from the list.");
    } else if (elementName == "volumeSliderElement") {
        text = tr("Volume:\\nAdjust volume for games on a global level, range goes from 0-500% with the default being 100%.");
    } else if (elementName == "chooseHomeTabGroupBox") {
        text = tr("Default tab when opening settings:\\nChoose which tab will open, the default is General.");
    } else if (elementName == "gameSizeCheckBox") {
        text = tr("Show Game Size In List:\\nThere is the size of the game in the list.");
    } else if (elementName == "motionControlsCheckBox") {
        text = tr("Enable Motion Controls:\\nWhen enabled it will use the controller's motion control if supported.");
    } else if (elementName == "dmaCheckBox") {
        text = tr("Enable Direct Memory Access:\\nEnables arbitrary memory access from the GPU to CPU memory.");
    } else if (elementName == "neoCheckBox") {
        text = tr("Enable PS4 Neo Mode:\\nAdds support for PS4 Pro emulation and memory size. Currently causes instability in a large number of tested games.");
    } else if (elementName == "devkitCheckBox") {
        text = tr("Enable Devkit Console Mode:\\nAdds support for Devkit console memory size.");
    } else if (elementName == "networkConnectedCheckBox") {
        text = tr("Set Network Connected to True:\\nForces games to detect an active network connection. Actual online capabilities are not yet supported.");
    } else if (elementName == "psnSignInCheckBox") {
        text = tr("Set PSN Signed-in to True:\\nForces games to detect an active PSN sign-in. Actual PSN capabilities are not supported."); 
    }
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

void SettingsDialog::UpdateSettings(bool game_specific) {
    // Entries with game-specific settings, needs the game-specific arg
    Config::setReadbacks(ui->readbacksCheckBox->isChecked(), game_specific);
    Config::setReadbackLinearImages(ui->readbackLinearImagesCheckBox->isChecked(), game_specific);
    Config::setDirectMemoryAccess(ui->dmaCheckBox->isChecked(), game_specific);
    Config::setDevKitConsole(ui->devkitCheckBox->isChecked(), game_specific);
    Config::setNeoMode(ui->neoCheckBox->isChecked(), game_specific);
    Config::setConnectedToNetwork(ui->networkConnectedCheckBox->isChecked(), game_specific);
    Config::setPSNSignedIn(ui->psnSignInCheckBox->isChecked(), game_specific);

    Config::setIsFullscreen(
        screenModeMap.value(ui->displayModeComboBox->currentText()) != "Windowed", game_specific);
    Config::setFullscreenMode(
        screenModeMap.value(ui->displayModeComboBox->currentText()).toStdString(), game_specific);
    Config::setPresentMode(
        presentModeMap.value(ui->presentModeComboBox->currentText()).toStdString(), game_specific);
    Config::setIsMotionControlsEnabled(ui->motionControlsCheckBox->isChecked(), game_specific);
    Config::setBackgroundControllerInput(ui->backgroundControllerCheckBox->isChecked(),
                                         game_specific);
    Config::setisTrophyPopupDisabled(ui->disableTrophycheckBox->isChecked(), game_specific);
    Config::setTrophyNotificationDuration(ui->popUpDurationSpinBox->value(), game_specific);

    if (ui->radioButton_Top->isChecked()) {
        Config::setSideTrophy("top", game_specific);
    } else if (ui->radioButton_Left->isChecked()) {
        Config::setSideTrophy("left", game_specific);
    } else if (ui->radioButton_Right->isChecked()) {
        Config::setSideTrophy("right", game_specific);
    } else if (ui->radioButton_Bottom->isChecked()) {
        Config::setSideTrophy("bottom", game_specific);
    }

    Config::setLoggingEnabled(ui->enableLoggingCheckBox->isChecked(), game_specific);
    Config::setAllowHDR(ui->enableHDRCheckBox->isChecked(), game_specific);
    Config::setLogType(logTypeMap.value(ui->logTypeComboBox->currentText()).toStdString(),
                       game_specific);
    Config::setMicDevice(ui->micComboBox->currentData().toString().toStdString(), game_specific);
    Config::setLogFilter(ui->logFilterLineEdit->text().toStdString(), game_specific);
    Config::setUserName(ui->userNameLineEdit->text().toStdString(), game_specific);
    Config::setCursorState(ui->hideCursorComboBox->currentIndex(), game_specific);
    Config::setCursorHideTimeout(ui->hideCursorComboBox->currentIndex(), game_specific);
    Config::setGpuId(ui->graphicsAdapterBox->currentIndex() - 1, game_specific);
    Config::setVolumeSlider(ui->horizontalVolumeSlider->value(), game_specific);
    Config::setLanguage(languageIndexes[ui->consoleLanguageComboBox->currentIndex()],
                        game_specific);
    Config::setWindowWidth(ui->widthSpinBox->value(), game_specific);
    Config::setWindowHeight(ui->heightSpinBox->value(), game_specific);
    Config::setVblankFreq(ui->vblankSpinBox->value(), game_specific);
    Config::setDumpShaders(ui->dumpShadersCheckBox->isChecked(), game_specific);
    Config::setNullGpu(ui->nullGpuCheckBox->isChecked(), game_specific);
    Config::setFsrEnabled(ui->FSRCheckBox->isChecked(), game_specific);
    Config::setRcasEnabled(ui->RCASCheckBox->isChecked(), game_specific);
    Config::setRcasAttenuation(ui->RCASSlider->value(), game_specific);
    Config::setShowSplash(ui->showSplashCheckBox->isChecked(), game_specific);
    Config::setDebugDump(ui->debugDump->isChecked(), game_specific);
    Config::setSeparateLogFilesEnabled(ui->separateLogFilesCheckbox->isChecked(), game_specific);
    Config::setVkValidation(ui->vkValidationCheckBox->isChecked(), game_specific);
    Config::setVkSyncValidation(ui->vkSyncValidationCheckBox->isChecked(), game_specific);
    Config::setRdocEnabled(ui->rdocCheckBox->isChecked(), game_specific);
    Config::setVkHostMarkersEnabled(ui->hostMarkersCheckBox->isChecked(), game_specific);
    Config::setVkGuestMarkersEnabled(ui->guestMarkersCheckBox->isChecked(), game_specific);
    Config::setVkCrashDiagnosticEnabled(ui->crashDiagnosticsCheckBox->isChecked(), game_specific);
    Config::setCollectShaderForDebug(ui->collectShaderCheckBox->isChecked(), game_specific);
    Config::setCopyGPUCmdBuffers(ui->copyGPUBuffersCheckBox->isChecked(), game_specific);
    Config::setChooseHomeTab(
        chooseHomeTabMap.value(ui->chooseHomeTabComboBox->currentText()).toStdString(),
        game_specific);

    // Entries with no game-specific settings
    if (!game_specific) {
        std::vector<Config::GameInstallDir> dirs_with_states;
        for (int i = 0; i < ui->gameFoldersListWidget->count(); i++) {
            QListWidgetItem* item = ui->gameFoldersListWidget->item(i);
            QString path_string = item->text();
            auto path = Common::FS::PathFromQString(path_string);
            bool enabled = (item->checkState() == Qt::Checked);

            dirs_with_states.push_back({path, enabled});
        }
        Config::setAllGameInstallDirs(dirs_with_states);

        BackgroundMusicPlayer::getInstance().setVolume(ui->BGMVolumeSlider->value());

#ifdef ENABLE_DISCORD_RPC
        auto* rpc = Common::Singleton<DiscordRPCHandler::RPC>::Instance();
        if (Config::getEnableDiscordRPC()) {
            rpc->init();
            rpc->setStatusIdling();
        } else {
            rpc->shutdown();
        }
#endif

        Config::setLoadGameSizeEnabled(ui->gameSizeCheckBox->isChecked());
        Config::setTrophyKey(ui->trophyKeyLineEdit->text().toStdString());
        Config::setEnableDiscordRPC(ui->discordRPCCheckbox->isChecked());
        Config::setCompatibilityEnabled(ui->enableCompatibilityCheckBox->isChecked());
        Config::setCheckCompatibilityOnStartup(
            ui->checkCompatibilityOnStartupCheckBox->isChecked());
        m_gui_settings->SetValue(gui::gl_playBackgroundMusic, ui->playBGMCheckBox->isChecked());
        m_gui_settings->SetValue(gui::gl_backgroundMusicVolume, ui->BGMVolumeSlider->value());
        m_gui_settings->SetValue(gui::gen_checkForUpdates, ui->updateCheckBox->isChecked());
        m_gui_settings->SetValue(gui::gen_showChangeLog, ui->changelogCheckBox->isChecked());
        m_gui_settings->SetValue(gui::gen_updateChannel,
                                 channelMap.value(ui->updateComboBox->currentText()));
        m_gui_settings->SetValue(gui::gl_showBackgroundImage,
                                 ui->showBackgroundImageCheckBox->isChecked());
        m_gui_settings->SetValue(gui::gl_backgroundImageOpacity,
                                 std::clamp(ui->backgroundImageOpacitySlider->value(), 0, 100));
        emit BackgroundOpacityChanged(ui->backgroundImageOpacitySlider->value());
    }
}

void SettingsDialog::SyncRealTimeWidgetstoConfig() {
    std::filesystem::path userdir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    const toml::value data = toml::parse(userdir / "config.toml");

    if (!game_specific) {
        ui->gameFoldersListWidget->clear();
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

    toml::value gs_data;
    game_specific
        ? gs_data = toml::parse(Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
                                (gs_serial + ".toml"))
        : gs_data = data;

    int sliderValue = toml::find_or<int>(gs_data, "General", "volumeSlider", 100);
    ui->horizontalVolumeSlider->setValue(sliderValue);

    // Since config::set can be called for volume slider (connected to the widget) outside the save
    // function, need to null it out if GS GUI is closed without saving
    game_specific ? Config::resetGameSpecificValue("volumeSlider")
                  : Config::setVolumeSlider(sliderValue);

    if (presenter) {
        presenter->GetFsrSettingsRef().enable =
            toml::find_or<bool>(gs_data, "GPU", "fsrEnabled", true);
        presenter->GetFsrSettingsRef().use_rcas =
            toml::find_or<bool>(gs_data, "GPU", "rcasEnabled", true);
        presenter->GetFsrSettingsRef().rcas_attenuation =
            static_cast<float>(toml::find_or<int>(gs_data, "GPU", "rcasAttenuation", 250) / 1000.f);
    }
}

void SettingsDialog::setDefaultValues() {
    if (!game_specific) {
        m_gui_settings->SetValue(gui::gl_showBackgroundImage, true);
        m_gui_settings->SetValue(gui::gl_backgroundImageOpacity, 50);
        m_gui_settings->SetValue(gui::gl_playBackgroundMusic, false);
        m_gui_settings->SetValue(gui::gl_backgroundMusicVolume, 50);
        m_gui_settings->SetValue(gui::gen_checkForUpdates, false);
        m_gui_settings->SetValue(gui::gen_showChangeLog, false);
        if (Common::g_is_release) {
            m_gui_settings->SetValue(gui::gen_updateChannel, "Release");
        } else {
            m_gui_settings->SetValue(gui::gen_updateChannel, "Nightly");
        }
        m_gui_settings->SetValue(gui::gen_guiLanguage, "en_US");
    }
}
