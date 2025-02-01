// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QMessageBox>
#include "common/path_util.h"
#include "control_settings.h"
#include "input/input_handler.h"
#include "kbm_config_dialog.h"
#include "ui_control_settings.h"

// #include "common/logging/log.h"

static std::string game_id = "";

ControlSettings::ControlSettings(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent)
    : QDialog(parent), m_game_info(game_info_get), ui(new Ui::ControlSettings) {

    ui->setupUi(this);

    AddBoxItems();
    SetUIValuestoMappings();

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
            SaveControllerConfig(true);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
            SetDefault();
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
            SaveControllerConfig(false);
        }
    });

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);
    connect(ui->KBMButton, &QPushButton::clicked, this, &ControlSettings::KBMClicked);
    connect(ui->ProfileComboBox, &QComboBox::currentTextChanged, this,
            &ControlSettings::OnProfileChanged);

    connect(ui->LeftDeadzoneSlider, &QSlider::valueChanged, this,
            [this](int value) { ui->LeftDeadzoneValue->setText(QString::number(value)); });
    connect(ui->RightDeadzoneSlider, &QSlider::valueChanged, this,
            [this](int value) { ui->RightDeadzoneValue->setText(QString::number(value)); });

    connect(ui->LStickUpBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->LStickDownBox->setCurrentIndex(value); });
    connect(ui->LStickDownBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->LStickUpBox->setCurrentIndex(value); });
    connect(ui->LStickRightBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->LStickLeftBox->setCurrentIndex(value); });
    connect(ui->LStickLeftBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->LStickRightBox->setCurrentIndex(value); });

    connect(ui->RStickUpBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->RStickDownBox->setCurrentIndex(value); });
    connect(ui->RStickDownBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->RStickUpBox->setCurrentIndex(value); });
    connect(ui->RStickRightBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->RStickLeftBox->setCurrentIndex(value); });
    connect(ui->RStickLeftBox, &QComboBox::currentIndexChanged, this,
            [this](int value) { ui->RStickRightBox->setCurrentIndex(value); });
}

void ControlSettings::SaveControllerConfig(bool CloseOnSave) {
    game_id = (ui->ProfileComboBox->currentText().toStdString());
    const auto config_file = Config::GetFoolproofKbmConfigFile(game_id);

    int lineCount = 0;
    int count_axis_left_x = 0;
    int count_axis_left_y = 0;
    int count_axis_right_x = 0;
    int count_axis_right_y = 0;
    std::fstream file(config_file);
    std::string line;
    std::string add;
    std::vector<std::string> lines;
    std::string output_string = "";
    std::string input_string = "";

    while (std::getline(file, line)) {
        lineCount++;

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            lines.push_back(line);
            continue;
        }

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            lines.push_back(line);
            continue;
        }

        output_string = line.substr(0, equal_pos - 1);
        input_string = line.substr(equal_pos + 2);

        if (std::find(controlleroutputs.begin(), controlleroutputs.end(), input_string) !=
                controlleroutputs.end() ||
            output_string == "analog_deadzone") {

            // can be made a lot more elegent if you could map a string to a widget
            if (input_string == "cross") {
                output_string = ui->ABox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "circle") {
                output_string = ui->BBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "square") {
                output_string = ui->XBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "triangle") {
                output_string = ui->YBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "l1") {
                output_string = ui->LBBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "r1") {
                output_string = ui->RBBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "l2") {
                output_string = ui->LTBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "r2") {
                output_string = ui->RTBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "l3") {
                output_string = ui->LClickBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "r3") {
                output_string = ui->RClickBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "options") {
                output_string = ui->StartBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "pad_up") {
                output_string = ui->DpadUpBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "pad_down") {
                output_string = ui->DpadDownBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "pad_left") {
                output_string = ui->DpadLeftBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "pad_right") {
                output_string = ui->DpadRightBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "back") {
                output_string = ui->BackBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "axis_left_x") {
                output_string = ui->LStickRightBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "axis_left_y") {
                output_string = ui->LStickUpBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "axis_right_x") {
                output_string = ui->RStickRightBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string == "axis_right_y") {
                output_string = ui->RStickUpBox->currentText().toStdString();
                line = output_string + " = " + input_string;
            } else if (input_string.contains("leftjoystick")) {
                std::string deadzonevalue = std::to_string(ui->LeftDeadzoneSlider->value());
                line = output_string + " = leftjoystick, " + deadzonevalue;
            } else if (input_string.contains("rightjoystick")) {
                std::string deadzonevalue = std::to_string(ui->RightDeadzoneSlider->value());
                line = output_string + " = rightjoystick, " + deadzonevalue;
            }
        }

        lines.push_back(line);

        if (output_string == "axis_left_x") {
            count_axis_left_x = count_axis_left_x + 1;
        } else if (output_string == "axis_left_y") {
            count_axis_left_y = count_axis_left_y + 1;
        } else if (output_string == "axis_right_x") {
            count_axis_right_x = count_axis_right_x + 1;
        } else if (output_string == "axis_right_y") {
            count_axis_right_y = count_axis_right_y + 1;
        }

        if (count_axis_left_x > 1 | count_axis_left_y > 1 | count_axis_right_x > 1 |
            count_axis_right_y > 1) {
            QMessageBox::StandardButton nosave;
            nosave = QMessageBox::information(this, "Unable to Save",
                                              "Cannot bind axis values more than once");
            return;
        }
    }

    std::ofstream output_file(config_file);
    for (auto const& line : lines) {
        output_file << line << '\n';
    }

    Input::ParseInputConfig(game_id);
    if (CloseOnSave) {
        QWidget::close();
    }
}

void ControlSettings::SetDefault() {
    ui->ABox->setCurrentIndex(0);
    ui->BBox->setCurrentIndex(1);
    ui->XBox->setCurrentIndex(2);
    ui->YBox->setCurrentIndex(3);
    ui->DpadUpBox->setCurrentIndex(11);
    ui->DpadDownBox->setCurrentIndex(12);
    ui->DpadLeftBox->setCurrentIndex(13);
    ui->DpadRightBox->setCurrentIndex(14);
    ui->LClickBox->setCurrentIndex(8);
    ui->RClickBox->setCurrentIndex(9);
    ui->LBBox->setCurrentIndex(4);
    ui->RBBox->setCurrentIndex(5);
    ui->LTBox->setCurrentIndex(6);
    ui->RTBox->setCurrentIndex(7);
    ui->StartBox->setCurrentIndex(10);
    ui->BackBox->setCurrentIndex(15);

    ui->LStickUpBox->setCurrentIndex(1);
    ui->LStickDownBox->setCurrentIndex(1);
    ui->LStickLeftBox->setCurrentIndex(0);
    ui->LStickRightBox->setCurrentIndex(0);
    ui->RStickUpBox->setCurrentIndex(3);
    ui->RStickDownBox->setCurrentIndex(3);
    ui->RStickLeftBox->setCurrentIndex(2);
    ui->RStickRightBox->setCurrentIndex(2);

    ui->LeftDeadzoneSlider->setValue(2);
    ui->RightDeadzoneSlider->setValue(2);
}

void ControlSettings::AddBoxItems() {
    ui->DpadUpBox->addItems(ButtonInputs);
    ui->DpadDownBox->addItems(ButtonInputs);
    ui->DpadLeftBox->addItems(ButtonInputs);
    ui->DpadRightBox->addItems(ButtonInputs);
    ui->LBBox->addItems(ButtonInputs);
    ui->RBBox->addItems(ButtonInputs);
    ui->LTBox->addItems(ButtonInputs);
    ui->RTBox->addItems(ButtonInputs);
    ui->RClickBox->addItems(ButtonInputs);
    ui->LClickBox->addItems(ButtonInputs);
    ui->StartBox->addItems(ButtonInputs);
    ui->ABox->addItems(ButtonInputs);
    ui->BBox->addItems(ButtonInputs);
    ui->XBox->addItems(ButtonInputs);
    ui->YBox->addItems(ButtonInputs);
    ui->BackBox->addItems(ButtonInputs);

    ui->LStickUpBox->addItems(StickInputs);
    ui->LStickDownBox->addItems(StickInputs);
    ui->LStickLeftBox->addItems(StickInputs);
    ui->LStickRightBox->addItems(StickInputs);
    ui->RStickUpBox->addItems(StickInputs);
    ui->RStickDownBox->addItems(StickInputs);
    ui->RStickLeftBox->addItems(StickInputs);
    ui->RStickRightBox->addItems(StickInputs);

    ui->ProfileComboBox->addItem("Default");
    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        ui->ProfileComboBox->addItem(QString::fromStdString(m_game_info->m_games[i].serial));
    }
    ui->ProfileComboBox->setCurrentText("Default");
    ui->TitleLabel->setText("Default Config");
}

void ControlSettings::SetUIValuestoMappings() {
    game_id = (ui->ProfileComboBox->currentText().toStdString());
    const auto config_file = Config::GetFoolproofKbmConfigFile(game_id);

    int lineCount = 0;
    std::ifstream file(config_file);
    std::string line = "";

    while (std::getline(file, line)) {
        lineCount++;

        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (line.empty()) {
            continue;
        }

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            continue;
        }

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 1);

        // can be made a lot more elegent if you could map a string to a widget
        if (std::find(controlleroutputs.begin(), controlleroutputs.end(), input_string) !=
                controlleroutputs.end() ||
            output_string == "analog_deadzone") {
            if (input_string == "cross") {
                ui->ABox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "circle") {
                ui->BBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "square") {
                ui->XBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "triangle") {
                ui->YBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "l1") {
                ui->LBBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "l2") {
                ui->LTBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "r1") {
                ui->RBBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "r2") {
                ui->RTBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "l3") {
                ui->LClickBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "r3") {
                ui->RClickBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "pad_up") {
                ui->DpadUpBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "pad_down") {
                ui->DpadDownBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "pad_left") {
                ui->DpadLeftBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "pad_right") {
                ui->DpadRightBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "options") {
                ui->StartBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "back") {
                ui->BackBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "axis_left_x") {
                ui->LStickRightBox->setCurrentText(QString::fromStdString(output_string));
                ui->LStickLeftBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "axis_left_y") {
                ui->LStickUpBox->setCurrentText(QString::fromStdString(output_string));
                ui->LStickDownBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "axis_right_x") {
                ui->RStickRightBox->setCurrentText(QString::fromStdString(output_string));
                ui->RStickLeftBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string == "axis_right_y") {
                ui->RStickUpBox->setCurrentText(QString::fromStdString(output_string));
                ui->RStickDownBox->setCurrentText(QString::fromStdString(output_string));
            } else if (input_string.contains("leftjoystick")) {
                std::size_t comma_pos = line.find(',');
                int deadzonevalue = std::stoi(line.substr(comma_pos + 1));
                ui->LeftDeadzoneSlider->setValue(deadzonevalue);
                ui->LeftDeadzoneValue->setText(QString::number(deadzonevalue));
            } else if (input_string.contains("rightjoystick")) {
                std::size_t comma_pos = line.find(',');
                int deadzonevalue = std::stoi(line.substr(comma_pos + 1));
                ui->RightDeadzoneSlider->setValue(deadzonevalue);
                ui->RightDeadzoneValue->setText(QString::number(deadzonevalue));
            }
        }
    }

    file.close();
}

void ControlSettings::GetGameTitle() {
    if (ui->ProfileComboBox->currentText() == "Default") {
        ui->TitleLabel->setText("Default Config");
    } else {
        for (int i = 0; i < m_game_info->m_games.size(); i++) {
            if (m_game_info->m_games[i].serial ==
                ui->ProfileComboBox->currentText().toStdString()) {
                ui->TitleLabel->setText(QString::fromStdString(m_game_info->m_games[i].name));
            }
        }
    }
}

void ControlSettings::OnProfileChanged() {
    GetGameTitle();
    SetUIValuestoMappings();
}

void ControlSettings::KBMClicked() {
    auto KBMWindow = new EditorDialog(this);
    KBMWindow->exec();
}

ControlSettings::~ControlSettings() {}
