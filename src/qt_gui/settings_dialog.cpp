// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCompleter>
#include <QDirIterator>
#include <QHoverEvent>

#include <common/version.h>
#include "check_update.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
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
                             "Norwegian",
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
                             "Vietnamese"};

const QVector<int> languageIndexes = {21, 23, 14, 6,  18, 1,  12, 22, 2,  4, 25, 24, 29, 5,  0,
                                      9,  15, 16, 17, 7,  26, 8,  11, 20, 3, 13, 27, 10, 19, 28};

SettingsDialog::SettingsDialog(std::span<const QString> physical_devices, QWidget* parent)
    : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    ui->tabWidgetSettings->setUsesScrollButtons(false);
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

    InitializeEmulatorLanguages();
    LoadValuesFromConfig();

    defaultTextEdit = tr("Point your mouse at an option to display its description.");
    ui->descriptionText->setText(defaultTextEdit);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this,
            [this, config_dir](QAbstractButton* button) {
                if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
                    Config::save(config_dir / "config.toml");
                    QWidget::close();
                } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
                    Config::save(config_dir / "config.toml");
                } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
                    Config::setDefaultValues();
                    LoadValuesFromConfig();
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
        connect(ui->userNameLineEdit, &QLineEdit::textChanged, this,
                [](const QString& text) { Config::setUserName(text.toStdString()); });

        connect(ui->consoleLanguageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [](int index) {
                    if (index >= 0 && index < languageIndexes.size()) {
                        int languageCode = languageIndexes[index];
                        Config::setLanguage(languageCode);
                    }
                });

        connect(ui->fullscreenCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setFullscreenMode(val); });

        connect(ui->showSplashCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setShowSplash(val); });

        connect(ui->ps4proCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setNeoMode(val); });

        connect(ui->logTypeComboBox, &QComboBox::currentTextChanged, this,
                [](const QString& text) { Config::setLogType(text.toStdString()); });

        connect(ui->logFilterLineEdit, &QLineEdit::textChanged, this,
                [](const QString& text) { Config::setLogFilter(text.toStdString()); });

        connect(ui->updateCheckBox, &QCheckBox::stateChanged, this,
                [](int state) { Config::setAutoUpdate(state == Qt::Checked); });

        connect(ui->updateComboBox, &QComboBox::currentTextChanged, this,
                [](const QString& channel) { Config::setUpdateChannel(channel.toStdString()); });

        connect(ui->checkUpdateButton, &QPushButton::clicked, this, []() {
            auto checkUpdate = new CheckUpdate(true);
            checkUpdate->exec();
        });

        connect(ui->playBGMCheckBox, &QCheckBox::stateChanged, this, [](int val) {
            Config::setPlayBGM(val);
            if (val == Qt::Unchecked) {
                BackgroundMusicPlayer::getInstance().stopMusic();
            }
        });

        connect(ui->BGMVolumeSlider, &QSlider::valueChanged, this, [](float val) {
            Config::setBGMvolume(val);
            BackgroundMusicPlayer::getInstance().setVolume(val);
        });
    }

    // GPU TAB
    {
        // First options is auto selection -1, so gpuId on the GUI will always have to subtract 1
        // when setting and add 1 when getting to select the correct gpu in Qt
        connect(ui->graphicsAdapterBox, &QComboBox::currentIndexChanged, this,
                [](int index) { Config::setGpuId(index - 1); });

        connect(ui->widthSpinBox, &QSpinBox::valueChanged, this,
                [](int val) { Config::setScreenWidth(val); });

        connect(ui->heightSpinBox, &QSpinBox::valueChanged, this,
                [](int val) { Config::setScreenHeight(val); });

        connect(ui->vblankSpinBox, &QSpinBox::valueChanged, this,
                [](int val) { Config::setVblankDiv(val); });

        connect(ui->dumpShadersCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setDumpShaders(val); });

        connect(ui->nullGpuCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setNullGpu(val); });
    }

    // DEBUG TAB
    {
        connect(ui->debugDump, &QCheckBox::stateChanged, this,
                [](int val) { Config::setDebugDump(val); });

        connect(ui->vkValidationCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setVkValidation(val); });

        connect(ui->vkSyncValidationCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setVkSyncValidation(val); });

        connect(ui->rdocCheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setRdocEnabled(val); });
    }

    // Descriptions
    {
        // General
        ui->consoleLanguageGroupBox->installEventFilter(this);
        ui->emulatorLanguageGroupBox->installEventFilter(this);
        ui->fullscreenCheckBox->installEventFilter(this);
        ui->showSplashCheckBox->installEventFilter(this);
        ui->ps4proCheckBox->installEventFilter(this);
        ui->userName->installEventFilter(this);
        ui->logTypeGroupBox->installEventFilter(this);
        ui->logFilter->installEventFilter(this);
        ui->updaterGroupBox->installEventFilter(this);
        ui->GUIgroupBox->installEventFilter(this);

        // Graphics
        ui->graphicsAdapterGroupBox->installEventFilter(this);
        ui->widthGroupBox->installEventFilter(this);
        ui->heightGroupBox->installEventFilter(this);
        ui->heightDivider->installEventFilter(this);
        ui->dumpShadersCheckBox->installEventFilter(this);
        ui->nullGpuCheckBox->installEventFilter(this);

        // Debug
        ui->debugDump->installEventFilter(this);
        ui->vkValidationCheckBox->installEventFilter(this);
        ui->vkSyncValidationCheckBox->installEventFilter(this);
        ui->rdocCheckBox->installEventFilter(this);
    }
}

void SettingsDialog::LoadValuesFromConfig() {
    ui->consoleLanguageComboBox->setCurrentIndex(
        std::distance(
            languageIndexes.begin(),
            std::find(languageIndexes.begin(), languageIndexes.end(), Config::GetLanguage())) %
        languageIndexes.size());
    ui->emulatorLanguageComboBox->setCurrentIndex(languages[Config::getEmulatorLanguage()]);
    ui->graphicsAdapterBox->setCurrentIndex(Config::getGpuId() + 1);
    ui->widthSpinBox->setValue(Config::getScreenWidth());
    ui->heightSpinBox->setValue(Config::getScreenHeight());
    ui->vblankSpinBox->setValue(Config::vblankDiv());
    ui->dumpShadersCheckBox->setChecked(Config::dumpShaders());
    ui->nullGpuCheckBox->setChecked(Config::nullGpu());
    ui->playBGMCheckBox->setChecked(Config::getPlayBGM());
    ui->BGMVolumeSlider->setValue((Config::getBGMvolume()));
    ui->fullscreenCheckBox->setChecked(Config::isFullscreenMode());
    ui->showSplashCheckBox->setChecked(Config::showSplash());
    ui->ps4proCheckBox->setChecked(Config::isNeoMode());
    ui->logTypeComboBox->setCurrentText(QString::fromStdString(Config::getLogType()));
    ui->logFilterLineEdit->setText(QString::fromStdString(Config::getLogFilter()));
    ui->userNameLineEdit->setText(QString::fromStdString(Config::getUserName()));

    ui->debugDump->setChecked(Config::debugDump());
    ui->vkValidationCheckBox->setChecked(Config::vkValidationEnabled());
    ui->vkSyncValidationCheckBox->setChecked(Config::vkValidationSyncEnabled());
    ui->rdocCheckBox->setChecked(Config::isRdocEnabled());

    ui->updateCheckBox->setChecked(Config::autoUpdate());
    std::string updateChannel = Config::getUpdateChannel();
    if (updateChannel != "Release" && updateChannel != "Nightly") {
        if (Common::isRelease) {
            updateChannel = "Release";
        } else {
            updateChannel = "Nightly";
        }
    }
    ui->updateComboBox->setCurrentText(QString::fromStdString(updateChannel));
}

void SettingsDialog::InitializeEmulatorLanguages() {
    QDirIterator it(QStringLiteral(":/translations"), QDirIterator::NoIteratorFlags);

    int idx = 0;
    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
        locale.remove(0, locale.lastIndexOf(QLatin1Char{'/'}) + 1);
        const QString lang = QLocale::languageToString(QLocale(locale).language());
        const QString country = QLocale::territoryToString(QLocale(locale).territory());
        ui->emulatorLanguageComboBox->addItem(QStringLiteral("%1 (%2)").arg(lang, country), locale);

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
    } else if (elementName == "showSplashCheckBox") {
        text = tr("showSplashCheckBox");
    } else if (elementName == "ps4proCheckBox") {
        text = tr("ps4proCheckBox");
    } else if (elementName == "userName") {
        text = tr("userName");
    } else if (elementName == "logTypeGroupBox") {
        text = tr("logTypeGroupBox");
    } else if (elementName == "logFilter") {
        text = tr("logFilter");
    } else if (elementName == "updaterGroupBox") {
        text = tr("updaterGroupBox");
    } else if (elementName == "GUIgroupBox") {
        text = tr("GUIgroupBox");
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
    } else if (elementName == "dumpPM4CheckBox") {
        text = tr("dumpPM4CheckBox");
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

bool SettingsDialog::override(QObject* obj, QEvent* event) {
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
            int documentHeight = ui->descriptionText->document()->size().height();
            int visibleHeight = ui->descriptionText->viewport()->height();
            if (documentHeight > visibleHeight) {
                ui->descriptionText->setMinimumHeight(90);
            } else {
                ui->descriptionText->setMinimumHeight(70);
            }
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}
