// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCompleter>
#include <QDirIterator>

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

        connect(ui->dumpPM4CheckBox, &QCheckBox::stateChanged, this,
                [](int val) { Config::setDumpPM4(val); });
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
    ui->dumpPM4CheckBox->setChecked(Config::dumpPM4());

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
