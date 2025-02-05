// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QMessageBox>
#include "common/path_util.h"
#include "control_settings.h"
#include "kbm_config_dialog.h"
#include "ui_control_settings.h"

ControlSettings::ControlSettings(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent)
    : QDialog(parent), m_game_info(game_info_get), ui(new Ui::ControlSettings) {

    ui->setupUi(this);
    ui->PerGameCheckBox->setChecked(!Config::GetUseUnifiedInputConfig());

    AddBoxItems();
    SetUIValuestoMappings();
    ui->KBMButton->setFocus();

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
    connect(ui->KBMButton, &QPushButton::clicked, this, [this] {
        auto KBMWindow = new EditorDialog(this);
        KBMWindow->exec();
        SetUIValuestoMappings();
    });
    connect(ui->ProfileComboBox, &QComboBox::currentTextChanged, this, [this] {
        GetGameTitle();
        SetUIValuestoMappings();
    });

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
    QList<QComboBox*> list;
    list << ui->RStickUpBox << ui->RStickRightBox << ui->LStickUpBox << ui->LStickRightBox;
    int count_axis_left_x = 0, count_axis_left_y = 0, count_axis_right_x = 0,
        count_axis_right_y = 0;
    for (const auto& i : list) {
        if (i->currentText() == "axis_left_x") {
            count_axis_left_x = count_axis_left_x + 1;
        } else if (i->currentText() == "axis_left_y") {
            count_axis_left_y = count_axis_left_y + 1;
        } else if (i->currentText() == "axis_right_x") {
            count_axis_right_x = count_axis_right_x + 1;
        } else if (i->currentText() == "axis_right_y") {
            count_axis_right_y = count_axis_right_y + 1;
        }
    }

    if (count_axis_left_x > 1 | count_axis_left_y > 1 | count_axis_right_x > 1 |
        count_axis_right_y > 1) {
        QMessageBox::StandardButton nosave;
        nosave = QMessageBox::information(this, "Unable to Save",
                                          "Cannot bind axis values more than once");
        return;
    }

    std::string config_id;
    config_id = (ui->ProfileComboBox->currentText() == "Common Config")
                    ? "default"
                    : ui->ProfileComboBox->currentText().toStdString();
    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);

    int lineCount = 0;
    std::string line;
    std::vector<std::string> lines;
    std::string output_string = "", input_string = "";
    std::fstream file(config_file);

    while (std::getline(file, line)) {
        lineCount++;

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            if (!line.contains("Range of deadzones"))
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

        if (std::find(ControllerInputs.begin(), ControllerInputs.end(), input_string) !=
                ControllerInputs.end() ||
            output_string == "analog_deadzone") {
            line.erase();
            continue;
        }
        lines.push_back(line);
    }

    file.close();

    input_string = "cross";
    output_string = ui->ABox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "circle";
    output_string = ui->BBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "square";
    output_string = ui->XBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "triangle";
    output_string = ui->YBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");

    input_string = "l1";
    output_string = ui->LBBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "r1";
    output_string = ui->RBBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "l2";
    output_string = ui->LTBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "r2";
    output_string = ui->RTBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "l3";
    output_string = ui->LClickBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "r3";
    output_string = ui->RClickBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");

    input_string = "back";
    output_string = ui->BackBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "options";
    output_string = ui->StartBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");

    input_string = "pad_up";
    output_string = ui->DpadUpBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "pad_down";
    output_string = ui->DpadDownBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "pad_left";
    output_string = ui->DpadLeftBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "pad_right";
    output_string = ui->DpadRightBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");

    input_string = "axis_left_x";
    output_string = ui->LStickRightBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "axis_left_y";
    output_string = ui->LStickUpBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "axis_right_x";
    output_string = ui->RStickRightBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    input_string = "axis_right_y";
    output_string = ui->RStickUpBox->currentText().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");
    lines.push_back("# Range of deadzones: 1 (almost none) to 127 (max)");

    std::string deadzonevalue = std::to_string(ui->LeftDeadzoneSlider->value());
    lines.push_back("analog_deadzone = leftjoystick, " + deadzonevalue + ", 127");

    deadzonevalue = std::to_string(ui->RightDeadzoneSlider->value());
    lines.push_back("analog_deadzone = rightjoystick, " + deadzonevalue + ", 127");

    std::vector<std::string> save;
    bool CurrentLineEmpty = false, LastLineEmpty = false;
    for (auto const& line : lines) {
        LastLineEmpty = CurrentLineEmpty ? true : false;
        CurrentLineEmpty = line.empty() ? true : false;
        if (!CurrentLineEmpty || !LastLineEmpty)
            save.push_back(line);
    }

    std::ofstream output_file(config_file);
    for (auto const& line : save) {
        output_file << line << '\n';
    }
    output_file.close();

    Config::SetUseUnifiedInputConfig(!ui->PerGameCheckBox->isChecked());
    Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");

    if (CloseOnSave)
        QWidget::close();
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
    ui->DpadUpBox->addItems(ButtonOutputs);
    ui->DpadDownBox->addItems(ButtonOutputs);
    ui->DpadLeftBox->addItems(ButtonOutputs);
    ui->DpadRightBox->addItems(ButtonOutputs);
    ui->LBBox->addItems(ButtonOutputs);
    ui->RBBox->addItems(ButtonOutputs);
    ui->LTBox->addItems(ButtonOutputs);
    ui->RTBox->addItems(ButtonOutputs);
    ui->RClickBox->addItems(ButtonOutputs);
    ui->LClickBox->addItems(ButtonOutputs);
    ui->StartBox->addItems(ButtonOutputs);
    ui->ABox->addItems(ButtonOutputs);
    ui->BBox->addItems(ButtonOutputs);
    ui->XBox->addItems(ButtonOutputs);
    ui->YBox->addItems(ButtonOutputs);
    ui->BackBox->addItems(ButtonOutputs);

    ui->LStickUpBox->addItems(StickOutputs);
    ui->LStickDownBox->addItems(StickOutputs);
    ui->LStickLeftBox->addItems(StickOutputs);
    ui->LStickRightBox->addItems(StickOutputs);
    ui->RStickUpBox->addItems(StickOutputs);
    ui->RStickDownBox->addItems(StickOutputs);
    ui->RStickLeftBox->addItems(StickOutputs);
    ui->RStickRightBox->addItems(StickOutputs);

    ui->ProfileComboBox->addItem("Common Config");
    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        ui->ProfileComboBox->addItem(QString::fromStdString(m_game_info->m_games[i].serial));
    }
    ui->ProfileComboBox->setCurrentText("Common Config");
    ui->TitleLabel->setText("Common Config");
}

void ControlSettings::SetUIValuestoMappings() {
    std::string config_id;
    config_id = (ui->ProfileComboBox->currentText() == "Common Config")
                    ? "default"
                    : ui->ProfileComboBox->currentText().toStdString();

    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);
    std::ifstream file(config_file);

    bool CrossExists = false, CircleExists = false, SquareExists = false, TriangleExists = false,
         L1Exists = false, L2Exists = false, L3Exists = false, R1Exists = false, R2Exists = false,
         R3Exists = false, DPadUpExists = false, DPadDownExists = false, DPadLeftExists = false,
         DPadRightExists = false, StartExists = false, BackExists = false, LStickXExists = false,
         LStickYExists = false, RStickXExists = false, RStickYExists = false;
    int lineCount = 0;
    std::string line = "";
    while (std::getline(file, line)) {
        lineCount++;

        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (line.empty())
            continue;

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos)
            continue;

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 1);

        if (std::find(ControllerInputs.begin(), ControllerInputs.end(), input_string) !=
                ControllerInputs.end() ||
            output_string == "analog_deadzone") {
            if (input_string == "cross") {
                ui->ABox->setCurrentText(QString::fromStdString(output_string));
                CrossExists = true;
            } else if (input_string == "circle") {
                ui->BBox->setCurrentText(QString::fromStdString(output_string));
                CircleExists = true;
            } else if (input_string == "square") {
                ui->XBox->setCurrentText(QString::fromStdString(output_string));
                SquareExists = true;
            } else if (input_string == "triangle") {
                ui->YBox->setCurrentText(QString::fromStdString(output_string));
                TriangleExists = true;
            } else if (input_string == "l1") {
                ui->LBBox->setCurrentText(QString::fromStdString(output_string));
                L1Exists = true;
            } else if (input_string == "l2") {
                ui->LTBox->setCurrentText(QString::fromStdString(output_string));
                L2Exists = true;
            } else if (input_string == "r1") {
                ui->RBBox->setCurrentText(QString::fromStdString(output_string));
                R1Exists = true;
            } else if (input_string == "r2") {
                ui->RTBox->setCurrentText(QString::fromStdString(output_string));
                R2Exists = true;
            } else if (input_string == "l3") {
                ui->LClickBox->setCurrentText(QString::fromStdString(output_string));
                L3Exists = true;
            } else if (input_string == "r3") {
                ui->RClickBox->setCurrentText(QString::fromStdString(output_string));
                R3Exists = true;
            } else if (input_string == "pad_up") {
                ui->DpadUpBox->setCurrentText(QString::fromStdString(output_string));
                DPadUpExists = true;
            } else if (input_string == "pad_down") {
                ui->DpadDownBox->setCurrentText(QString::fromStdString(output_string));
                DPadDownExists = true;
            } else if (input_string == "pad_left") {
                ui->DpadLeftBox->setCurrentText(QString::fromStdString(output_string));
                DPadLeftExists = true;
            } else if (input_string == "pad_right") {
                ui->DpadRightBox->setCurrentText(QString::fromStdString(output_string));
                DPadRightExists = true;
            } else if (input_string == "options") {
                ui->StartBox->setCurrentText(QString::fromStdString(output_string));
                StartExists = true;
            } else if (input_string == "back") {
                ui->BackBox->setCurrentText(QString::fromStdString(output_string));
                BackExists = true;
            } else if (input_string == "axis_left_x") {
                ui->LStickRightBox->setCurrentText(QString::fromStdString(output_string));
                ui->LStickLeftBox->setCurrentText(QString::fromStdString(output_string));
                LStickXExists = true;
            } else if (input_string == "axis_left_y") {
                ui->LStickUpBox->setCurrentText(QString::fromStdString(output_string));
                ui->LStickDownBox->setCurrentText(QString::fromStdString(output_string));
                LStickYExists = true;
            } else if (input_string == "axis_right_x") {
                ui->RStickRightBox->setCurrentText(QString::fromStdString(output_string));
                ui->RStickLeftBox->setCurrentText(QString::fromStdString(output_string));
                RStickXExists = true;
            } else if (input_string == "axis_right_y") {
                ui->RStickUpBox->setCurrentText(QString::fromStdString(output_string));
                ui->RStickDownBox->setCurrentText(QString::fromStdString(output_string));
                RStickYExists = true;
            } else if (input_string.contains("leftjoystick")) {
                std::size_t comma_pos = line.find(',');
                if (comma_pos != std::string::npos) {
                    int deadzonevalue = std::stoi(line.substr(comma_pos + 1));
                    ui->LeftDeadzoneSlider->setValue(deadzonevalue);
                    ui->LeftDeadzoneValue->setText(QString::number(deadzonevalue));
                } else {
                    ui->LeftDeadzoneSlider->setValue(2);
                    ui->LeftDeadzoneValue->setText("2");
                }
            } else if (input_string.contains("rightjoystick")) {
                std::size_t comma_pos = line.find(',');
                if (comma_pos != std::string::npos) {
                    int deadzonevalue = std::stoi(line.substr(comma_pos + 1));
                    ui->RightDeadzoneSlider->setValue(deadzonevalue);
                    ui->RightDeadzoneValue->setText(QString::number(deadzonevalue));
                } else {
                    ui->RightDeadzoneSlider->setValue(2);
                    ui->RightDeadzoneValue->setText("2");
                }
            }
        }
    }

    // If an entry does not exist in the config file, we assume the user wants it unmapped
    if (!CrossExists)
        ui->ABox->setCurrentText("unmapped");
    if (!CircleExists)
        ui->BBox->setCurrentText("unmapped");
    if (!SquareExists)
        ui->XBox->setCurrentText("unmapped");
    if (!TriangleExists)
        ui->YBox->setCurrentText("unmapped");
    if (!L1Exists)
        ui->LBBox->setCurrentText("unmapped");
    if (!L2Exists)
        ui->LTBox->setCurrentText("unmapped");
    if (!L3Exists)
        ui->LClickBox->setCurrentText("unmapped");
    if (!R1Exists)
        ui->RBBox->setCurrentText("unmapped");
    if (!R2Exists)
        ui->RTBox->setCurrentText("unmapped");
    if (!R3Exists)
        ui->RClickBox->setCurrentText("unmapped");
    if (!DPadUpExists)
        ui->DpadUpBox->setCurrentText("unmapped");
    if (!DPadDownExists)
        ui->DpadDownBox->setCurrentText("unmapped");
    if (!DPadLeftExists)
        ui->DpadLeftBox->setCurrentText("unmapped");
    if (!DPadRightExists)
        ui->DpadRightBox->setCurrentText("unmapped");
    if (!BackExists)
        ui->BackBox->setCurrentText("unmapped");
    if (!StartExists)
        ui->StartBox->setCurrentText("unmapped");

    if (!LStickXExists) {
        ui->LStickRightBox->setCurrentText("unmapped");
        ui->LStickLeftBox->setCurrentText("unmapped");
    }
    if (!LStickYExists) {
        ui->LStickUpBox->setCurrentText("unmapped");
        ui->LStickDownBox->setCurrentText("unmapped");
    }
    if (!RStickXExists) {
        ui->RStickRightBox->setCurrentText("unmapped");
        ui->RStickLeftBox->setCurrentText("unmapped");
    }
    if (!RStickYExists) {
        ui->RStickUpBox->setCurrentText("unmapped");
        ui->RStickDownBox->setCurrentText("unmapped");
    }

    file.close();
}

void ControlSettings::GetGameTitle() {
    if (ui->ProfileComboBox->currentText() == "Common Config") {
        ui->TitleLabel->setText("Common Config");
    } else {
        for (int i = 0; i < m_game_info->m_games.size(); i++) {
            if (m_game_info->m_games[i].serial ==
                ui->ProfileComboBox->currentText().toStdString()) {
                ui->TitleLabel->setText(QString::fromStdString(m_game_info->m_games[i].name));
            }
        }
    }
}

ControlSettings::~ControlSettings() {}
