// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QWheelEvent>

#include "common/path_util.h"
#include "kbm_config_dialog.h"
#include "kbm_gui.h"
#include "kbm_help_dialog.h"
#include "ui_kbm_gui.h"

HelpDialog* HelpWindow;

KBMSettings::KBMSettings(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent)
    : QDialog(parent), m_game_info(game_info_get), ui(new Ui::KBMSettings) {

    ui->setupUi(this);
    ui->PerGameCheckBox->setChecked(!Config::GetUseUnifiedInputConfig());
    ui->TextEditorButton->setFocus();
    this->setFocusPolicy(Qt::StrongFocus);

    ui->MouseJoystickBox->addItem("none");
    ui->MouseJoystickBox->addItem("right");
    ui->MouseJoystickBox->addItem("left");

    ui->ProfileComboBox->addItem("Common Config");
    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        ui->ProfileComboBox->addItem(QString::fromStdString(m_game_info->m_games[i].serial));
    }

    ButtonsList = {
        ui->CrossButton,       ui->CircleButton,   ui->TriangleButton,   ui->SquareButton,
        ui->L1Button,          ui->R1Button,       ui->L2Button,         ui->R2Button,
        ui->L3Button,          ui->R3Button,       ui->TouchpadButton,   ui->OptionsButton,
        ui->TouchpadButton,    ui->DpadUpButton,   ui->DpadDownButton,   ui->DpadLeftButton,
        ui->DpadRightButton,   ui->LStickUpButton, ui->LStickDownButton, ui->LStickLeftButton,
        ui->LStickRightButton, ui->RStickUpButton, ui->RStickDownButton, ui->RStickLeftButton,
        ui->RStickRightButton, ui->LHalfButton,    ui->RHalfButton};

    ButtonConnects();
    SetUIValuestoMappings("default");
    installEventFilter(this);

    ui->ProfileComboBox->setCurrentText("Common Config");
    ui->TitleLabel->setText("Common Config");

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
            if (HelpWindowOpen) {
                HelpWindow->close();
                HelpWindowOpen = false;
            }
            SaveKBMConfig(true);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
            SetDefault();
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
            SaveKBMConfig(false);
        }
    });

    connect(ui->HelpButton, &QPushButton::clicked, this, &KBMSettings::onHelpClicked);
    connect(ui->TextEditorButton, &QPushButton::clicked, this, [this]() {
        auto kbmWindow = new EditorDialog(this);
        kbmWindow->exec();
    });

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [this] {
        QWidget::close();
        if (HelpWindowOpen) {
            HelpWindow->close();
            HelpWindowOpen = false;
        }
    });

    connect(ui->ProfileComboBox, &QComboBox::currentTextChanged, this, [this] {
        GetGameTitle();
        std::string config_id = (ui->ProfileComboBox->currentText() == "Common Config")
                                    ? "default"
                                    : ui->ProfileComboBox->currentText().toStdString();
        SetUIValuestoMappings(config_id);
    });

    connect(ui->CopyCommonButton, &QPushButton::clicked, this, [this] {
        if (ui->ProfileComboBox->currentText() == "Common Config") {
            QMessageBox::information(this, tr("Common Config Selected"),
                                     // clang-format off
tr("This button copies mappings from the Common Config to the currently selected profile, and cannot be used when the currently selected profile is the Common Config."));
            // clang-format on
        } else {
            QMessageBox::StandardButton reply =
                QMessageBox::question(this, tr("Copy values from Common Config"),
                                      // clang-format off
tr("Do you want to overwrite existing mappings with the mappings from the Common Config?"),
                                      // clang-format on
                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                SetUIValuestoMappings("default");
            }
        }
    });

    connect(ui->DeadzoneOffsetSlider, &QSlider::valueChanged, this, [this](int value) {
        QString DOSValue = QString::number(value / 100.0, 'f', 2);
        QString DOSString = tr("Deadzone Offset (def 0.50):") + " " + DOSValue;
        ui->DeadzoneOffsetLabel->setText(DOSString);
    });

    connect(ui->SpeedMultiplierSlider, &QSlider::valueChanged, this, [this](int value) {
        QString SMSValue = QString::number(value / 10.0, 'f', 1);
        QString SMSString = tr("Speed Multiplier (def 1.0):") + " " + SMSValue;
        ui->SpeedMultiplierLabel->setText(SMSString);
    });

    connect(ui->SpeedOffsetSlider, &QSlider::valueChanged, this, [this](int value) {
        QString SOSValue = QString::number(value / 1000.0, 'f', 3);
        QString SOSString = tr("Speed Offset (def 0.125):") + " " + SOSValue;
        ui->SpeedOffsetLabel->setText(SOSString);
    });
}

void KBMSettings::ButtonConnects() {
    connect(ui->CrossButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->CrossButton); });
    connect(ui->CircleButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->CircleButton); });
    connect(ui->TriangleButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->TriangleButton); });
    connect(ui->SquareButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->SquareButton); });

    connect(ui->L1Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->L1Button); });
    connect(ui->L2Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->L2Button); });
    connect(ui->L3Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->L3Button); });
    connect(ui->R1Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->R1Button); });
    connect(ui->R2Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->R2Button); });
    connect(ui->R3Button, &QPushButton::clicked, this, [this]() { StartTimer(ui->R3Button); });

    connect(ui->TouchpadButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->TouchpadButton); });
    connect(ui->OptionsButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->OptionsButton); });

    connect(ui->DpadUpButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->DpadUpButton); });
    connect(ui->DpadDownButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->DpadDownButton); });
    connect(ui->DpadLeftButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->DpadLeftButton); });
    connect(ui->DpadRightButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->DpadRightButton); });

    connect(ui->LStickUpButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->LStickUpButton); });
    connect(ui->LStickDownButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->LStickDownButton); });
    connect(ui->LStickLeftButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->LStickLeftButton); });
    connect(ui->LStickRightButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->LStickRightButton); });

    connect(ui->RStickUpButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->RStickUpButton); });
    connect(ui->RStickDownButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->RStickDownButton); });
    connect(ui->RStickLeftButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->RStickLeftButton); });
    connect(ui->RStickRightButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->RStickRightButton); });

    connect(ui->LHalfButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->LHalfButton); });
    connect(ui->RHalfButton, &QPushButton::clicked, this,
            [this]() { StartTimer(ui->RHalfButton); });
}

void KBMSettings::DisableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(false);
    }
}

void KBMSettings::EnableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(true);
    }
}

void KBMSettings::SaveKBMConfig(bool CloseOnSave) {
    std::string output_string = "", input_string = "";
    std::vector<std::string> lines, inputs;

    lines.push_back("#Feeling lost? Check out the Help section!");
    lines.push_back("");
    lines.push_back("#Keyboard bindings");
    lines.push_back("");

    input_string = ui->CrossButton->text().toStdString();
    output_string = "cross";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->CircleButton->text().toStdString();
    output_string = "circle";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->TriangleButton->text().toStdString();
    output_string = "triangle";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->SquareButton->text().toStdString();
    output_string = "square";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->DpadUpButton->text().toStdString();
    output_string = "pad_up";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->DpadDownButton->text().toStdString();
    output_string = "pad_down";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->DpadLeftButton->text().toStdString();
    output_string = "pad_left";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->DpadRightButton->text().toStdString();
    output_string = "pad_right";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->L1Button->text().toStdString();
    output_string = "l1";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->R1Button->text().toStdString();
    output_string = "r1";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->L2Button->text().toStdString();
    output_string = "l2";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->R2Button->text().toStdString();
    output_string = "r2";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->L3Button->text().toStdString();
    output_string = "l3";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->R3Button->text().toStdString();
    output_string = "r3";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->OptionsButton->text().toStdString();
    output_string = "options";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->TouchpadButton->text().toStdString();
    output_string = "touchpad";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->LStickUpButton->text().toStdString();
    output_string = "axis_left_y_minus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->LStickDownButton->text().toStdString();
    output_string = "axis_left_y_plus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->LStickLeftButton->text().toStdString();
    output_string = "axis_left_x_minus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->LStickRightButton->text().toStdString();
    output_string = "axis_left_x_plus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->RStickUpButton->text().toStdString();
    output_string = "axis_right_y_minus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->RStickDownButton->text().toStdString();
    output_string = "axis_right_y_plus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->RStickLeftButton->text().toStdString();
    output_string = "axis_right_x_minus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->RStickRightButton->text().toStdString();
    output_string = "axis_right_x_plus";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    lines.push_back("");

    input_string = ui->MouseJoystickBox->currentText().toStdString();
    output_string = "mouse_to_joystick";
    if (input_string != "unmapped")
        lines.push_back(output_string + " = " + input_string);

    input_string = ui->LHalfButton->text().toStdString();
    output_string = "leftjoystick_halfmode";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    input_string = ui->RHalfButton->text().toStdString();
    output_string = "rightjoystick_halfmode";
    lines.push_back(output_string + " = " + input_string);
    if (input_string != "unmapped")
        inputs.push_back(input_string);

    std::string DOString = std::format("{:.2f}", (ui->DeadzoneOffsetSlider->value() / 100.f));
    std::string SMString = std::format("{:.1f}", (ui->SpeedMultiplierSlider->value() / 10.f));
    std::string SOString = std::format("{:.3f}", (ui->SpeedOffsetSlider->value() / 1000.f));
    input_string = DOString + ", " + SMString + ", " + SOString;
    output_string = "mouse_movement_params";
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");

    std::string config_id = (ui->ProfileComboBox->currentText() == "Common Config")
                                ? "default"
                                : ui->ProfileComboBox->currentText().toStdString();
    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);
    std::fstream file(config_file);
    int lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineCount++;

        if (line.empty()) {
            lines.push_back(line);
            continue;
        }

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            if (!line.contains("Keyboard bindings") && !line.contains("Feeling lost") &&
                !line.contains("Alternatives for users"))
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
            output_string == "analog_deadzone" || output_string == "override_controller_color") {
            lines.push_back(line);
        }
    }
    file.close();

    // Prevent duplicate inputs for KBM as this breaks the engine
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        if (std::find(it + 1, inputs.end(), *it) != inputs.end()) {
            QMessageBox::information(this, tr("Unable to Save"),
                                     tr("Cannot bind any unique input more than once"));
            return;
        }
    }

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

void KBMSettings::SetDefault() {
    ui->CrossButton->setText("kp2");
    ui->CircleButton->setText("kp6");
    ui->TriangleButton->setText("kp8");
    ui->SquareButton->setText("kp4");

    ui->L1Button->setText("q");
    ui->L2Button->setText("e");
    ui->L3Button->setText("x");
    ui->R1Button->setText("u");
    ui->R2Button->setText("o");
    ui->R3Button->setText("m");

    ui->TouchpadButton->setText("space");
    ui->OptionsButton->setText("enter");

    ui->DpadUpButton->setText("up");
    ui->DpadDownButton->setText("down");
    ui->DpadLeftButton->setText("left");
    ui->DpadRightButton->setText("right");

    ui->LStickUpButton->setText("w");
    ui->LStickDownButton->setText("s");
    ui->LStickLeftButton->setText("a");
    ui->LStickRightButton->setText("d");

    ui->RStickUpButton->setText("i");
    ui->RStickDownButton->setText("k");
    ui->RStickLeftButton->setText("j");
    ui->RStickRightButton->setText("l");

    ui->LHalfButton->setText("unmapped");
    ui->RHalfButton->setText("unmapped");

    ui->MouseJoystickBox->setCurrentText("none");
    ui->DeadzoneOffsetSlider->setValue(50);
    ui->SpeedMultiplierSlider->setValue(10);
    ui->SpeedOffsetSlider->setValue(125);
}

void KBMSettings::SetUIValuestoMappings(std::string config_id) {
    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);
    std::ifstream file(config_file);

    int lineCount = 0;
    std::string line = "";
    while (std::getline(file, line)) {
        lineCount++;

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos)
            continue;

        std::string output_string = line.substr(0, equal_pos - 1);
        std::string input_string = line.substr(equal_pos + 2);

        if (std::find(ControllerInputs.begin(), ControllerInputs.end(), input_string) ==
            ControllerInputs.end()) {

            if (output_string == "cross") {
                ui->CrossButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "circle") {
                ui->CircleButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "square") {
                ui->SquareButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "triangle") {
                ui->TriangleButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "l1") {
                ui->L1Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "l2") {
                ui->L2Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "r1") {
                ui->R1Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "r2") {
                ui->R2Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "l3") {
                ui->L3Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "r3") {
                ui->R3Button->setText(QString::fromStdString(input_string));
            } else if (output_string == "pad_up") {
                ui->DpadUpButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "pad_down") {
                ui->DpadDownButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "pad_left") {
                ui->DpadLeftButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "pad_right") {
                ui->DpadRightButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "options") {
                ui->OptionsButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "touchpad") {
                ui->TouchpadButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_left_x_minus") {
                ui->LStickLeftButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_left_x_plus") {
                ui->LStickRightButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_left_y_minus") {
                ui->LStickUpButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_left_y_plus") {
                ui->LStickDownButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_right_x_minus") {
                ui->RStickLeftButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_right_x_plus") {
                ui->RStickRightButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_right_y_minus") {
                ui->RStickUpButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "axis_right_y_plus") {
                ui->RStickDownButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "mouse_to_joystick") {
                ui->MouseJoystickBox->setCurrentText(QString::fromStdString(input_string));
            } else if (output_string == "leftjoystick_halfmode") {
                ui->LHalfButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "rightjoystick_halfmode") {
                ui->RHalfButton->setText(QString::fromStdString(input_string));
            } else if (output_string.contains("mouse_movement_params")) {
                std::size_t comma_pos = line.find(',');
                if (comma_pos != std::string::npos) {
                    std::string DOstring = line.substr(equal_pos + 1, comma_pos);
                    float DOffsetValue = std::stof(DOstring) * 100.0;
                    int DOffsetInt = int(DOffsetValue);
                    ui->DeadzoneOffsetSlider->setValue(DOffsetInt);
                    QString LabelValue = QString::number(DOffsetInt / 100.0, 'f', 2);
                    QString LabelString = tr("Deadzone Offset (def 0.50):") + " " + LabelValue;
                    ui->DeadzoneOffsetLabel->setText(LabelString);

                    std::string SMSOstring = line.substr(comma_pos + 1);
                    std::size_t comma_pos2 = SMSOstring.find(',');
                    if (comma_pos2 != std::string::npos) {
                        std::string SMstring = SMSOstring.substr(0, comma_pos2);
                        float SpeedMultValue = std::stof(SMstring) * 10.0;
                        int SpeedMultInt = int(SpeedMultValue);
                        if (SpeedMultInt < 1)
                            SpeedMultInt = 1;
                        if (SpeedMultInt > 50)
                            SpeedMultInt = 50;
                        ui->SpeedMultiplierSlider->setValue(SpeedMultInt);
                        LabelValue = QString::number(SpeedMultInt / 10.0, 'f', 1);
                        LabelString = tr("Speed Multiplier (def 1.0):") + " " + LabelValue;
                        ui->SpeedMultiplierLabel->setText(LabelString);

                        std::string SOstring = SMSOstring.substr(comma_pos2 + 1);
                        float SOffsetValue = std::stof(SOstring) * 1000.0;
                        int SOffsetInt = int(SOffsetValue);
                        ui->SpeedOffsetSlider->setValue(SOffsetInt);
                        LabelValue = QString::number(SOffsetInt / 1000.0, 'f', 3);
                        LabelString = tr("Speed Offset (def 0.125):") + " " + LabelValue;
                        ui->SpeedOffsetLabel->setText(LabelString);
                    }
                }
            }
        }
    }
    file.close();
}

void KBMSettings::GetGameTitle() {
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

void KBMSettings::onHelpClicked() {
    if (!HelpWindowOpen) {
        HelpWindow = new HelpDialog(&HelpWindowOpen, this);
        HelpWindow->setWindowTitle(tr("Help"));
        HelpWindow->setAttribute(Qt::WA_DeleteOnClose); // Clean up on close
        HelpWindow->show();
        HelpWindowOpen = true;
    } else {
        HelpWindow->close();
        HelpWindowOpen = false;
    }
}

void KBMSettings::StartTimer(QPushButton*& button) {
    MappingTimer = 3;
    EnableMapping = true;
    MappingCompleted = false;
    modifier = "";
    mapping = button->text();

    DisableMappingButtons();
    button->setText(tr("Press a key") + " [" + QString::number(MappingTimer) + "]");
    timer = new QTimer(this);
    MappingButton = button;
    connect(timer, &QTimer::timeout, this, [this]() { CheckMapping(MappingButton); });
    timer->start(1000);
}

void KBMSettings::CheckMapping(QPushButton*& button) {
    MappingTimer -= 1;
    button->setText(tr("Press a key") + " [" + QString::number(MappingTimer) + "]");

    if (MappingCompleted) {
        EnableMapping = false;
        EnableMappingButtons();
        timer->stop();

        if (mapping == "lshift" || mapping == "lalt" || mapping == "lctrl" || mapping == "lmeta" ||
            mapping == "lwin") {
            modifier = "";
        }

        if (modifier != "") {
            button->setText(modifier + ", " + mapping);
        } else {
            button->setText(mapping);
        }
    }

    if (MappingTimer <= 0) {
        button->setText(mapping);
        EnableMapping = false;
        EnableMappingButtons();
        timer->stop();
    }
}

void KBMSettings::SetMapping(QString input) {
    mapping = input;
    MappingCompleted = true;
}

bool KBMSettings::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Close) {
        if (HelpWindowOpen) {
            HelpWindow->close();
            HelpWindowOpen = false;
        }
    }

    if (EnableMapping) {
        if (Qt::ShiftModifier & QApplication::keyboardModifiers()) {
            modifier = "lshift";
        } else if (Qt::AltModifier & QApplication::keyboardModifiers()) {
            modifier = "lalt";
        } else if (Qt::ControlModifier & QApplication::keyboardModifiers()) {
            modifier = "lctrl";
        } else if (Qt::MetaModifier & QApplication::keyboardModifiers()) {
#ifdef _WIN32
            modifier = "lwin";
#else
            modifier = "lmeta";
#endif
        }

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            switch (keyEvent->key()) {
            case Qt::Key_Space:
                SetMapping("space");
                break;
            case Qt::Key_Comma:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kpcomma");
                } else {
                    SetMapping("comma");
                }
                break;
            case Qt::Key_Period:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kpperiod");
                } else {
                    SetMapping("period");
                }
                break;
            case Qt::Key_Slash:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers())
                    SetMapping("kpdivide");
                break;
            case Qt::Key_Asterisk:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers())
                    SetMapping("kpmultiply");
                break;
            case Qt::Key_Question:
                SetMapping("question");
                break;
            case Qt::Key_Semicolon:
                SetMapping("semicolon");
                break;
            case Qt::Key_Minus:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kpminus");
                } else {
                    SetMapping("minus");
                }
                break;
            case Qt::Key_Plus:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kpplus");
                } else {
                    SetMapping("plus");
                }
                break;
            case Qt::Key_ParenLeft:
                SetMapping("lparenthesis");
                break;
            case Qt::Key_ParenRight:
                SetMapping("rparenthesis");
                break;
            case Qt::Key_BracketLeft:
                SetMapping("lbracket");
                break;
            case Qt::Key_BracketRight:
                SetMapping("rbracket");
                break;
            case Qt::Key_BraceLeft:
                SetMapping("lbrace");
                break;
            case Qt::Key_BraceRight:
                SetMapping("rbrace");
                break;
            case Qt::Key_Backslash:
                SetMapping("backslash");
                break;
            case Qt::Key_Tab:
                SetMapping("tab");
                break;
            case Qt::Key_Backspace:
                SetMapping("backspace");
                break;
            case Qt::Key_Return:
                SetMapping("enter");
                break;
            case Qt::Key_Enter:
                SetMapping("kpenter");
                break;
            case Qt::Key_Escape:
                SetMapping("unmapped");
                break;
            case Qt::Key_Shift:
                SetMapping("lshift");
                break;
            case Qt::Key_Alt:
                SetMapping("lalt");
                break;
            case Qt::Key_Control:
                SetMapping("lctrl");
                break;
            case Qt::Key_Meta:
                activateWindow();
#ifdef _WIN32
                SetMapping("lwin");
#else
                SetMapping("lmeta");
#endif
            case Qt::Key_1:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp1");
                } else {
                    SetMapping("1");
                }
                break;
            case Qt::Key_2:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp2");
                } else {
                    SetMapping("2");
                }
                break;
            case Qt::Key_3:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp3");
                } else {
                    SetMapping("3");
                }
                break;
            case Qt::Key_4:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp4");
                } else {
                    SetMapping("4");
                }
                break;
            case Qt::Key_5:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp5");
                } else {
                    SetMapping("5");
                }
                break;
            case Qt::Key_6:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp6");
                } else {
                    SetMapping("6");
                }
                break;
            case Qt::Key_7:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp7");
                } else {
                    SetMapping("7");
                }
                break;
            case Qt::Key_8:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp8");
                } else {
                    SetMapping("8");
                }
                break;
            case Qt::Key_9:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp9");
                } else {
                    SetMapping("9");
                }
                break;
            case Qt::Key_0:
                if (Qt::KeypadModifier & QApplication::keyboardModifiers()) {
                    SetMapping("kp0");
                } else {
                    SetMapping("0");
                }
                break;
            case Qt::Key_Up:
                activateWindow();
                SetMapping("up");
                break;
            case Qt::Key_Down:
                SetMapping("down");
                break;
            case Qt::Key_Left:
                SetMapping("left");
                break;
            case Qt::Key_Right:
                SetMapping("right");
                break;
            case Qt::Key_A:
                SetMapping("a");
                break;
            case Qt::Key_B:
                SetMapping("b");
                break;
            case Qt::Key_C:
                SetMapping("c");
                break;
            case Qt::Key_D:
                SetMapping("d");
                break;
            case Qt::Key_E:
                SetMapping("e");
                break;
            case Qt::Key_F:
                SetMapping("f");
                break;
            case Qt::Key_G:
                SetMapping("g");
                break;
            case Qt::Key_H:
                SetMapping("h");
                break;
            case Qt::Key_I:
                SetMapping("i");
                break;
            case Qt::Key_J:
                SetMapping("j");
                break;
            case Qt::Key_K:
                SetMapping("k");
                break;
            case Qt::Key_L:
                SetMapping("l");
                break;
            case Qt::Key_M:
                SetMapping("m");
                break;
            case Qt::Key_N:
                SetMapping("n");
                break;
            case Qt::Key_O:
                SetMapping("o");
                break;
            case Qt::Key_P:
                SetMapping("p");
                break;
            case Qt::Key_Q:
                SetMapping("q");
                break;
            case Qt::Key_R:
                SetMapping("r");
                break;
            case Qt::Key_S:
                SetMapping("s");
                break;
            case Qt::Key_T:
                SetMapping("t");
                break;
            case Qt::Key_U:
                SetMapping("u");
                break;
            case Qt::Key_V:
                SetMapping("v");
                break;
            case Qt::Key_W:
                SetMapping("w");
                break;
            case Qt::Key_X:
                SetMapping("x");
                break;
            case Qt::Key_Y:
                SetMapping("Y");
                break;
            case Qt::Key_Z:
                SetMapping("z");
                break;
            default:
                break;
            }
            return true;
        }

        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            switch (mouseEvent->button()) {
            case Qt::LeftButton:
                SetMapping("leftbutton");
                break;
            case Qt::RightButton:
                SetMapping("rightbutton");
                break;
            case Qt::MiddleButton:
                SetMapping("middlebutton");
                break;
            default:
                break;
            }
            return true;
        }

        const QList<QPushButton*> AxisList = {
            ui->LStickUpButton, ui->LStickDownButton, ui->LStickLeftButton, ui->LStickRightButton,
            ui->RStickUpButton, ui->LStickDownButton, ui->LStickLeftButton, ui->RStickRightButton};

        if (event->type() == QEvent::Wheel) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            if (wheelEvent->angleDelta().y() > 5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    SetMapping("mousewheelup");
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            } else if (wheelEvent->angleDelta().y() < -5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    SetMapping("mousewheeldown");
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            }
            if (wheelEvent->angleDelta().x() > 5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    // QT changes scrolling to horizontal for all widgets with the alt modifier
                    if (Qt::AltModifier & QApplication::keyboardModifiers()) {
                        SetMapping("mousewheelup");
                    } else {
                        SetMapping("mousewheelright");
                    }
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            } else if (wheelEvent->angleDelta().x() < -5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    if (Qt::AltModifier & QApplication::keyboardModifiers()) {
                        SetMapping("mousewheeldown");
                    } else {
                        SetMapping("mousewheelleft");
                    }
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            }
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

KBMSettings::~KBMSettings() {}
