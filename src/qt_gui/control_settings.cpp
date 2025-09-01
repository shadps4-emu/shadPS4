// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include "common/logging/log.h"
#include "common/path_util.h"
#include "control_settings.h"
#include "input/input_handler.h"
#include "sdl_window.h"
#include "ui_control_settings.h"

ControlSettings::ControlSettings(std::shared_ptr<GameInfoClass> game_info_get, bool isGameRunning,
                                 std::string GameRunningSerial, QWidget* parent)
    : QDialog(parent), m_game_info(game_info_get), GameRunning(isGameRunning),
      RunningGameSerial(GameRunningSerial), ui(new Ui::ControlSettings) {

    ui->setupUi(this);

    if (!GameRunning) {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);
        SDL_InitSubSystem(SDL_INIT_EVENTS);
    } else {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    }

    AddBoxItems();
    SetUIValuestoMappings();
    UpdateLightbarColor();
    CheckGamePad();
    installEventFilter(this);

    ButtonsList = {ui->CrossButton,
                   ui->CircleButton,
                   ui->TriangleButton,
                   ui->SquareButton,
                   ui->L1Button,
                   ui->R1Button,
                   ui->L2Button,
                   ui->R2Button,
                   ui->L3Button,
                   ui->R3Button,
                   ui->OptionsButton,
                   ui->TouchpadLeftButton,
                   ui->TouchpadCenterButton,
                   ui->TouchpadRightButton,
                   ui->DpadUpButton,
                   ui->DpadDownButton,
                   ui->DpadLeftButton,
                   ui->DpadRightButton};

    AxisList = {ui->LStickUpButton,    ui->LStickDownButton, ui->LStickLeftButton,
                ui->LStickRightButton, ui->RStickUpButton,   ui->RStickDownButton,
                ui->RStickLeftButton,  ui->RStickRightButton};

    for (auto& button : ButtonsList) {
        connect(button, &QPushButton::clicked, this,
                [this, &button]() { StartTimer(button, true); });
    }

    for (auto& button : AxisList) {
        connect(button, &QPushButton::clicked, this,
                [this, &button]() { StartTimer(button, false); });
    }

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
            SaveControllerConfig(true);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
            SetDefault();
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
            SaveControllerConfig(false);
        }
    });

    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Restore Defaults"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    ui->PerGameCheckBox->setChecked(!Config::GetUseUnifiedInputConfig());

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    connect(ui->ProfileComboBox, &QComboBox::currentTextChanged, this, [this] {
        GetGameTitle();
        SetUIValuestoMappings();
    });

    connect(ui->LeftDeadzoneSlider, &QSlider::valueChanged, this,
            [this](int value) { ui->LeftDeadzoneValue->setText(QString::number(value)); });
    connect(ui->RightDeadzoneSlider, &QSlider::valueChanged, this,
            [this](int value) { ui->RightDeadzoneValue->setText(QString::number(value)); });

    connect(ui->RSlider, &QSlider::valueChanged, this, [this](int value) {
        QString RedValue = QString("%1").arg(value, 3, 10, QChar('0'));
        QString RValue = tr("R:") + " " + RedValue;
        ui->RLabel->setText(RValue);
        UpdateLightbarColor();
    });

    connect(ui->GSlider, &QSlider::valueChanged, this, [this](int value) {
        QString GreenValue = QString("%1").arg(value, 3, 10, QChar('0'));
        QString GValue = tr("G:") + " " + GreenValue;
        ui->GLabel->setText(GValue);
        UpdateLightbarColor();
    });

    connect(ui->BSlider, &QSlider::valueChanged, this, [this](int value) {
        QString BlueValue = QString("%1").arg(value, 3, 10, QChar('0'));
        QString BValue = tr("B:") + " " + BlueValue;
        ui->BLabel->setText(BValue);
        UpdateLightbarColor();
    });

    connect(this, &ControlSettings::PushGamepadEvent, this,
            [this]() { CheckMapping(MappingButton); });
    connect(this, &ControlSettings::AxisChanged, this,
            [this]() { ConnectAxisInputs(MappingButton); });
    connect(ui->ActiveGamepadBox, &QComboBox::currentIndexChanged, this,
            &ControlSettings::ActiveControllerChanged);

    connect(ui->DefaultGamepadButton, &QPushButton::clicked, this, [this]() {
        ui->DefaultGamepadName->setText(ui->ActiveGamepadBox->currentText());
        std::string GUID =
            GamepadSelect::GetGUIDString(gamepads, ui->ActiveGamepadBox->currentIndex());
        ui->DefaultGamepadLabel->setText(tr("ID: ") + QString::fromStdString(GUID).right(16));
        Config::setDefaultControllerID(GUID);
        Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
        QMessageBox::information(this, tr("Default Controller Selected"),
                                 tr("Active controller set as default"));
    });

    connect(ui->RemoveDefaultGamepadButton, &QPushButton::clicked, this, [this]() {
        ui->DefaultGamepadName->setText(tr("No default selected"));
        ui->DefaultGamepadLabel->setText(tr("n/a"));
        Config::setDefaultControllerID("");
        Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
        QMessageBox::information(this, tr("Default Controller Removed"),
                                 tr("Default controller setting removed"));
    });

    RemapWrapper = SdlEventWrapper::Wrapper::GetInstance();
    SdlEventWrapper::Wrapper::wrapperActive = true;
    QObject::connect(RemapWrapper, &SdlEventWrapper::Wrapper::SDLEvent, this,
                     &ControlSettings::processSDLEvents);

    if (!GameRunning) {
        Polling = QtConcurrent::run(&ControlSettings::pollSDLEvents, this);
    }
}

void ControlSettings::SaveControllerConfig(bool CloseOnSave) {
    QList<QPushButton*> list;
    list << ui->RStickUpButton << ui->RStickRightButton << ui->LStickUpButton
         << ui->LStickRightButton;
    int count_axis_left_x = 0, count_axis_left_y = 0, count_axis_right_x = 0,
        count_axis_right_y = 0;
    for (const auto& i : list) {
        if (i->text() == "axis_left_x") {
            count_axis_left_x = count_axis_left_x + 1;
        } else if (i->text() == "axis_left_y") {
            count_axis_left_y = count_axis_left_y + 1;
        } else if (i->text() == "axis_right_x") {
            count_axis_right_x = count_axis_right_x + 1;
        } else if (i->text() == "axis_right_y") {
            count_axis_right_y = count_axis_right_y + 1;
        }
    }

    if (count_axis_left_x > 1 | count_axis_left_y > 1 | count_axis_right_x > 1 |
        count_axis_right_y > 1) {
        QMessageBox::information(this, tr("Unable to Save"),
                                 tr("Cannot bind axis values more than once"));
        return;
    }

    std::string config_id;
    config_id = (ui->ProfileComboBox->currentText() == tr("Common Config"))
                    ? "default"
                    : ui->ProfileComboBox->currentText().toStdString();
    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);

    int lineCount = 0;
    std::string line;
    std::vector<std::string> lines, inputs;
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

        bool controllerInputdetected = false;
        for (std::string input : ControllerInputs) {
            // Needed to avoid detecting backspace while detecting back
            if (input_string.contains(input) && !input_string.contains("backspace")) {
                controllerInputdetected = true;
                break;
            }
        }

        if (controllerInputdetected || output_string == "analog_deadzone" ||
            output_string == "override_controller_color") {
            line.erase();
            continue;
        }
        lines.push_back(line);
    }

    file.close();

    // Lambda to reduce repetitive code for mapping buttons to config lines
    auto add_mapping = [&](const QString& buttonText, const std::string& output_name) {
        input_string = buttonText.toStdString();
        output_string = output_name;
        if (input_string != "unmapped") {
            lines.push_back(output_string + " = " + input_string);
            inputs.push_back(input_string);
        }
    };

    add_mapping(ui->CrossButton->text(), "cross");
    add_mapping(ui->CircleButton->text(), "circle");
    add_mapping(ui->SquareButton->text(), "square");
    add_mapping(ui->TriangleButton->text(), "triangle");

    lines.push_back("");

    add_mapping(ui->L1Button->text(), "l1");
    add_mapping(ui->R1Button->text(), "r1");
    add_mapping(ui->L2Button->text(), "l2");
    add_mapping(ui->R2Button->text(), "r2");
    add_mapping(ui->L3Button->text(), "l3");
    add_mapping(ui->R3Button->text(), "r3");

    lines.push_back("");

    add_mapping(ui->TouchpadLeftButton->text(), "touchpad_left");
    add_mapping(ui->TouchpadCenterButton->text(), "touchpad_center");
    add_mapping(ui->TouchpadRightButton->text(), "touchpad_right");
    add_mapping(ui->OptionsButton->text(), "options");

    lines.push_back("");

    add_mapping(ui->DpadUpButton->text(), "pad_up");
    add_mapping(ui->DpadDownButton->text(), "pad_down");
    add_mapping(ui->DpadLeftButton->text(), "pad_left");
    add_mapping(ui->DpadRightButton->text(), "pad_right");

    lines.push_back("");

    output_string = "axis_left_x";
    input_string = ui->LStickRightButton->text().toStdString();
    lines.push_back(output_string + " = " + input_string);

    output_string = "axis_left_y";
    input_string = ui->LStickUpButton->text().toStdString();
    lines.push_back(output_string + " = " + input_string);

    output_string = "axis_right_x";
    input_string = ui->RStickRightButton->text().toStdString();
    lines.push_back(output_string + " = " + input_string);

    output_string = "axis_right_y";
    input_string = ui->RStickUpButton->text().toStdString();
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");
    lines.push_back("# Range of deadzones: 1 (almost none) to 127 (max)");

    std::string deadzonevalue = std::to_string(ui->LeftDeadzoneSlider->value());
    lines.push_back("analog_deadzone = leftjoystick, " + deadzonevalue + ", 127");

    deadzonevalue = std::to_string(ui->RightDeadzoneSlider->value());
    lines.push_back("analog_deadzone = rightjoystick, " + deadzonevalue + ", 127");

    lines.push_back("");
    std::string OverrideLB = ui->LightbarCheckBox->isChecked() ? "true" : "false";
    std::string LightBarR = std::to_string(ui->RSlider->value());
    std::string LightBarG = std::to_string(ui->GSlider->value());
    std::string LightBarB = std::to_string(ui->BSlider->value());
    lines.push_back("override_controller_color = " + OverrideLB + ", " + LightBarR + ", " +
                    LightBarG + ", " + LightBarB);

    // Prevent duplicate inputs that break the input engine
    bool duplicateFound = false;
    QSet<QString> duplicateMappings;

    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        if (std::find(it + 1, inputs.end(), *it) != inputs.end()) {
            duplicateFound = true;
            duplicateMappings.insert(QString::fromStdString(*it));
        }
    }

    if (duplicateFound) {
        QStringList duplicatesList;
        for (const QString mapping : duplicateMappings) {
            for (const auto& button : ButtonsList) {
                if (button->text() == mapping)
                    duplicatesList.append(button->objectName() + " - " + mapping);
            }
        }
        QMessageBox::information(
            this, tr("Unable to Save"),
            // clang-format off
            QString(tr("Cannot bind any unique input more than once. Duplicate inputs mapped to the following buttons:\n\n%1").arg(duplicatesList.join("\n"))));
        // clang-format on
        return;
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
    Config::SetOverrideControllerColor(ui->LightbarCheckBox->isChecked());
    Config::SetControllerCustomColor(ui->RSlider->value(), ui->GSlider->value(),
                                     ui->BSlider->value());
    Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");

    if (GameRunning) {
        Config::GetUseUnifiedInputConfig() ? Input::ParseInputConfig("default")
                                           : Input::ParseInputConfig(RunningGameSerial);
    }

    if (CloseOnSave)
        QWidget::close();
}

void ControlSettings::SetDefault() {
    ui->CrossButton->setText("cross");
    ui->CircleButton->setText("circle");
    ui->SquareButton->setText("square");
    ui->TriangleButton->setText("triangle");
    ui->DpadUpButton->setText("pad_up");
    ui->DpadDownButton->setText("pad_down");
    ui->DpadLeftButton->setText("pad_left");
    ui->DpadRightButton->setText("pad_right");
    ui->L3Button->setText("l3");
    ui->R3Button->setText("r3");
    ui->L1Button->setText("l1");
    ui->R1Button->setText("r1");
    ui->L2Button->setText("l2");
    ui->R2Button->setText("r2");
    ui->OptionsButton->setText("options");
    ui->TouchpadLeftButton->setText("back");
    ui->TouchpadCenterButton->setText("unmapped");
    ui->TouchpadRightButton->setText("unmapped");

    ui->LStickUpButton->setText("axis_left_y");
    ui->LStickDownButton->setText("axis_left_y");
    ui->LStickLeftButton->setText("axis_left_x");
    ui->LStickRightButton->setText("axis_left_x");
    ui->RStickUpButton->setText("axis_right_y");
    ui->RStickDownButton->setText("axis_right_y");
    ui->RStickLeftButton->setText("axis_right_x");
    ui->RStickRightButton->setText("axis_right_x");

    ui->LeftDeadzoneSlider->setValue(2);
    ui->RightDeadzoneSlider->setValue(2);

    ui->RSlider->setValue(0);
    ui->GSlider->setValue(0);
    ui->BSlider->setValue(255);
    ui->LightbarCheckBox->setChecked(false);
    ui->PerGameCheckBox->setChecked(false);
}

void ControlSettings::AddBoxItems() {
    ui->ProfileComboBox->addItem(tr("Common Config"));
    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        ui->ProfileComboBox->addItem(QString::fromStdString(m_game_info->m_games[i].serial));
    }
    ui->ProfileComboBox->setCurrentText(tr("Common Config"));
    ui->TitleLabel->setText(tr("Common Config"));
}

void ControlSettings::SetUIValuestoMappings() {
    std::string config_id;
    config_id = (ui->ProfileComboBox->currentText() == tr("Common Config"))
                    ? "default"
                    : ui->ProfileComboBox->currentText().toStdString();

    const auto config_file = Config::GetFoolproofKbmConfigFile(config_id);
    std::ifstream file(config_file);

    bool CrossExists = false, CircleExists = false, SquareExists = false, TriangleExists = false,
         L1Exists = false, L2Exists = false, L3Exists = false, R1Exists = false, R2Exists = false,
         R3Exists = false, DPadUpExists = false, DPadDownExists = false, DPadLeftExists = false,
         DPadRightExists = false, OptionsExists = false, TouchpadLeftExists = false,
         TouchpadCenterExists = false, TouchpadRightExists = false, LStickXExists = false,
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

        bool controllerInputdetected = false;
        for (std::string input : ControllerInputs) {
            // Needed to avoid detecting backspace while detecting back
            if (input_string.contains(input) && !input_string.contains("backspace")) {
                controllerInputdetected = true;
                break;
            }
        }

        if (controllerInputdetected) {
            if (output_string == "cross") {
                ui->CrossButton->setText(QString::fromStdString(input_string));
                CrossExists = true;
            } else if (output_string == "circle") {
                ui->CircleButton->setText(QString::fromStdString(input_string));
                CircleExists = true;
            } else if (output_string == "square") {
                ui->SquareButton->setText(QString::fromStdString(input_string));
                SquareExists = true;
            } else if (output_string == "triangle") {
                ui->TriangleButton->setText(QString::fromStdString(input_string));
                TriangleExists = true;
            } else if (output_string == "l1") {
                ui->L1Button->setText(QString::fromStdString(input_string));
                L1Exists = true;
            } else if (output_string == "l2") {
                ui->L2Button->setText(QString::fromStdString(input_string));
                L2Exists = true;
            } else if (output_string == "r1") {
                ui->R1Button->setText(QString::fromStdString(input_string));
                R1Exists = true;
            } else if (output_string == "r2") {
                ui->R2Button->setText(QString::fromStdString(input_string));
                R2Exists = true;
            } else if (output_string == "l3") {
                ui->L3Button->setText(QString::fromStdString(input_string));
                L3Exists = true;
            } else if (output_string == "r3") {
                ui->R3Button->setText(QString::fromStdString(input_string));
                R3Exists = true;
            } else if (output_string == "pad_up") {
                ui->DpadUpButton->setText(QString::fromStdString(input_string));
                DPadUpExists = true;
            } else if (output_string == "pad_down") {
                ui->DpadDownButton->setText(QString::fromStdString(input_string));
                DPadDownExists = true;
            } else if (output_string == "pad_left") {
                ui->DpadLeftButton->setText(QString::fromStdString(input_string));
                DPadLeftExists = true;
            } else if (output_string == "pad_right") {
                ui->DpadRightButton->setText(QString::fromStdString(input_string));
                DPadRightExists = true;
            } else if (output_string == "options") {
                ui->OptionsButton->setText(QString::fromStdString(input_string));
                OptionsExists = true;
            } else if (output_string == "touchpad_left") {
                ui->TouchpadLeftButton->setText(QString::fromStdString(input_string));
                TouchpadLeftExists = true;
            } else if (output_string == "touchpad_center") {
                ui->TouchpadCenterButton->setText(QString::fromStdString(input_string));
                TouchpadCenterExists = true;
            } else if (output_string == "touchpad_right") {
                ui->TouchpadRightButton->setText(QString::fromStdString(input_string));
                TouchpadRightExists = true;
            } else if (output_string == "axis_left_x") {
                ui->LStickRightButton->setText(QString::fromStdString(input_string));
                ui->LStickLeftButton->setText(QString::fromStdString(input_string));
                LStickXExists = true;
            } else if (output_string == "axis_left_y") {
                ui->LStickUpButton->setText(QString::fromStdString(input_string));
                ui->LStickDownButton->setText(QString::fromStdString(input_string));
                LStickYExists = true;
            } else if (output_string == "axis_right_x") {
                ui->RStickRightButton->setText(QString::fromStdString(input_string));
                ui->RStickLeftButton->setText(QString::fromStdString(input_string));
                RStickXExists = true;
            } else if (output_string == "axis_right_y") {
                ui->RStickUpButton->setText(QString::fromStdString(input_string));
                ui->RStickDownButton->setText(QString::fromStdString(input_string));
                RStickYExists = true;
            }
        }

        if (input_string.contains("leftjoystick")) {
            std::size_t comma_pos = line.find(',');
            if (comma_pos != std::string::npos) {
                int deadzonevalue = std::stoi(line.substr(comma_pos + 1));
                ui->LeftDeadzoneSlider->setValue(deadzonevalue);
                ui->LeftDeadzoneValue->setText(QString::number(deadzonevalue));
            } else {
                ui->LeftDeadzoneSlider->setValue(2);
                ui->LeftDeadzoneValue->setText("2");
            }
        }

        if (input_string.contains("rightjoystick")) {
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

        if (output_string == "override_controller_color") {
            std::size_t comma_pos = line.find(',');
            if (comma_pos != std::string::npos) {
                std::string overridestring = line.substr(equal_pos + 1, comma_pos);
                bool override = overridestring.contains("true") ? true : false;
                ui->LightbarCheckBox->setChecked(override);

                std::string lightbarstring = line.substr(comma_pos + 1);
                std::size_t comma_pos2 = lightbarstring.find(',');
                if (comma_pos2 != std::string::npos) {
                    std::string Rstring = lightbarstring.substr(0, comma_pos2);
                    ui->RSlider->setValue(std::stoi(Rstring));
                    QString RedValue = QString("%1").arg(std::stoi(Rstring), 3, 10, QChar('0'));
                    QString RValue = tr("R:") + " " + RedValue;
                    ui->RLabel->setText(RValue);
                }

                std::string GBstring = lightbarstring.substr(comma_pos2 + 1);
                std::size_t comma_pos3 = GBstring.find(',');
                if (comma_pos3 != std::string::npos) {
                    std::string Gstring = GBstring.substr(0, comma_pos3);
                    ui->GSlider->setValue(std::stoi(Gstring));
                    QString GreenValue = QString("%1").arg(std::stoi(Gstring), 3, 10, QChar('0'));
                    QString GValue = tr("G:") + " " + GreenValue;
                    ui->GLabel->setText(GValue);

                    std::string Bstring = GBstring.substr(comma_pos3 + 1);
                    ui->BSlider->setValue(std::stoi(Bstring));
                    QString BlueValue = QString("%1").arg(std::stoi(Bstring), 3, 10, QChar('0'));
                    QString BValue = tr("B:") + " " + BlueValue;
                    ui->BLabel->setText(BValue);
                }
            }
        }
    }
    file.close();

    // If an entry does not exist in the config file, we assume the user wants it unmapped
    if (!CrossExists)
        ui->CrossButton->setText("unmapped");
    if (!CircleExists)
        ui->CircleButton->setText("unmapped");
    if (!SquareExists)
        ui->SquareButton->setText("unmapped");
    if (!TriangleExists)
        ui->TriangleButton->setText("unmapped");
    if (!L1Exists)
        ui->L1Button->setText("unmapped");
    if (!L2Exists)
        ui->L2Button->setText("unmapped");
    if (!L3Exists)
        ui->L3Button->setText("unmapped");
    if (!R1Exists)
        ui->R1Button->setText("unmapped");
    if (!R2Exists)
        ui->R2Button->setText("unmapped");
    if (!R3Exists)
        ui->R3Button->setText("unmapped");
    if (!DPadUpExists)
        ui->DpadUpButton->setText("unmapped");
    if (!DPadDownExists)
        ui->DpadDownButton->setText("unmapped");
    if (!DPadLeftExists)
        ui->DpadLeftButton->setText("unmapped");
    if (!DPadRightExists)
        ui->DpadRightButton->setText("unmapped");
    if (!TouchpadLeftExists)
        ui->TouchpadLeftButton->setText("unmapped");
    if (!TouchpadCenterExists)
        ui->TouchpadCenterButton->setText("unmapped");
    if (!TouchpadRightExists)
        ui->TouchpadRightButton->setText("unmapped");
    if (!OptionsExists)
        ui->OptionsButton->setText("unmapped");

    if (!LStickXExists) {
        ui->LStickRightButton->setText("unmapped");
        ui->LStickLeftButton->setText("unmapped");
    }
    if (!LStickYExists) {
        ui->LStickUpButton->setText("unmapped");
        ui->LStickDownButton->setText("unmapped");
    }
    if (!RStickXExists) {
        ui->RStickRightButton->setText("unmapped");
        ui->RStickLeftButton->setText("unmapped");
    }
    if (!RStickYExists) {
        ui->RStickUpButton->setText("unmapped");
        ui->RStickDownButton->setText("unmapped");
    }
}

void ControlSettings::GetGameTitle() {
    if (ui->ProfileComboBox->currentText() == tr("Common Config")) {
        ui->TitleLabel->setText(tr("Common Config"));
    } else {
        for (int i = 0; i < m_game_info->m_games.size(); i++) {
            if (m_game_info->m_games[i].serial ==
                ui->ProfileComboBox->currentText().toStdString()) {
                ui->TitleLabel->setText(QString::fromStdString(m_game_info->m_games[i].name));
            }
        }
    }
}

void ControlSettings::UpdateLightbarColor() {
    ui->LightbarColorFrame->setStyleSheet("");
    QString RValue = QString::number(ui->RSlider->value());
    QString GValue = QString::number(ui->GSlider->value());
    QString BValue = QString::number(ui->BSlider->value());
    QString colorstring = "background-color: rgb(" + RValue + "," + GValue + "," + BValue + ")";
    ui->LightbarColorFrame->setStyleSheet(colorstring);
}

void ControlSettings::ActiveControllerChanged(int value) {
    GamepadSelect::SetSelectedGamepad(GamepadSelect::GetGUIDString(gamepads, value));
    QString GUID = QString::fromStdString(GamepadSelect::GetSelectedGamepad()).right(16);
    ui->ActiveGamepadLabel->setText("ID: " + GUID);

    if (gamepad) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }

    gamepad = SDL_OpenGamepad(gamepads[value]);

    if (!gamepad) {
        LOG_ERROR(Input, "Failed to open gamepad: {}", SDL_GetError());
    }
}

void ControlSettings::CheckGamePad() {
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }

    gamepads = SDL_GetGamepads(&gamepad_count);

    if (!gamepads) {
        LOG_ERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
    }

    QString defaultGUID = "";
    int defaultIndex =
        GamepadSelect::GetIndexfromGUID(gamepads, gamepad_count, Config::getDefaultControllerID());
    int activeIndex = GamepadSelect::GetIndexfromGUID(gamepads, gamepad_count,
                                                      GamepadSelect::GetSelectedGamepad());

    if (!GameRunning) {
        if (activeIndex != -1) {
            gamepad = SDL_OpenGamepad(gamepads[activeIndex]);
        } else if (defaultIndex != -1) {
            gamepad = SDL_OpenGamepad(gamepads[defaultIndex]);
        } else {
            LOG_INFO(Input, "Got {} gamepads. Opening the first one.", gamepad_count);
            gamepad = SDL_OpenGamepad(gamepads[0]);
        }

        if (!gamepad) {
            LOG_ERROR(Input, "Failed to open gamepad: {}", SDL_GetError());
        }
    }

    if (!gamepads || gamepad_count == 0) {
        ui->ActiveGamepadBox->addItem("No gamepads detected");
        ui->ActiveGamepadBox->setCurrentIndex(0);
        return;
    } else {
        for (int i = 0; i < gamepad_count; i++) {
            QString name = SDL_GetGamepadNameForID(gamepads[i]);
            ui->ActiveGamepadBox->addItem(QString("%1: %2").arg(QString::number(i + 1), name));
        }
    }

    if (defaultIndex != -1) {
        defaultGUID =
            QString::fromStdString(GamepadSelect::GetGUIDString(gamepads, defaultIndex)).right(16);
        ui->DefaultGamepadName->setText(SDL_GetGamepadNameForID(gamepads[defaultIndex]));
        ui->DefaultGamepadLabel->setText(tr("ID: ") + defaultGUID);
    } else {
        ui->DefaultGamepadName->setText("Default controller not connected");
        ui->DefaultGamepadLabel->setText(tr("n/a"));
    }

    if (activeIndex != -1) {
        QString GUID =
            QString::fromStdString(GamepadSelect::GetGUIDString(gamepads, activeIndex)).right(16);
        ui->ActiveGamepadLabel->setText(tr("ID: ") + GUID);
        ui->ActiveGamepadBox->setCurrentIndex(activeIndex);
    } else if (defaultIndex != -1) {
        ui->ActiveGamepadLabel->setText(defaultGUID);
        ui->ActiveGamepadBox->setCurrentIndex(defaultIndex);
    } else {
        QString GUID = QString::fromStdString(GamepadSelect::GetGUIDString(gamepads, 0)).right(16);
        ui->ActiveGamepadLabel->setText("ID: " + GUID);
        ui->ActiveGamepadBox->setCurrentIndex(0);
    }
}

void ControlSettings::DisableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(false);
    }

    for (const auto& i : AxisList) {
        i->setEnabled(false);
    }

    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);
}

void ControlSettings::EnableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(true);
    }

    for (const auto& i : AxisList) {
        i->setEnabled(true);
    }

    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(true);
}

void ControlSettings::ConnectAxisInputs(QPushButton*& button) {
    QString input = button->text();
    if (button == ui->LStickUpButton) {
        ui->LStickDownButton->setText(input);
    } else if (button == ui->LStickDownButton) {
        ui->LStickUpButton->setText(input);
    } else if (button == ui->LStickLeftButton) {
        ui->LStickRightButton->setText(input);
    } else if (button == ui->LStickRightButton) {
        ui->LStickLeftButton->setText(input);
    } else if (button == ui->RStickUpButton) {
        ui->RStickDownButton->setText(input);
    } else if (button == ui->RStickDownButton) {
        ui->RStickUpButton->setText(input);
    } else if (button == ui->RStickLeftButton) {
        ui->RStickRightButton->setText(input);
    } else if (button == ui->RStickRightButton) {
        ui->RStickLeftButton->setText(input);
    }
}

void ControlSettings::StartTimer(QPushButton*& button, bool isButton) {
    MappingTimer = 3;
    isButton ? EnableButtonMapping = true : EnableAxisMapping = true;
    MappingCompleted = false;
    mapping = button->text();
    DisableMappingButtons();

    EnableButtonMapping
        ? button->setText(tr("Press a button") + " [" + QString::number(MappingTimer) + "]")
        : button->setText(tr("Move analog stick") + " [" + QString::number(MappingTimer) + "]");

    timer = new QTimer(this);
    MappingButton = button;
    timer->start(1000);
    connect(timer, &QTimer::timeout, this, [this]() { CheckMapping(MappingButton); });
}

void ControlSettings::CheckMapping(QPushButton*& button) {
    MappingTimer -= 1;
    EnableButtonMapping
        ? button->setText(tr("Press a button") + " [" + QString::number(MappingTimer) + "]")
        : button->setText(tr("Move analog stick") + " [" + QString::number(MappingTimer) + "]");

    if (pressedButtons.size() > 0) {
        QStringList keyStrings;

        for (const QString& buttonAction : pressedButtons) {
            keyStrings << buttonAction;
        }

        QString combo = keyStrings.join(",");
        SetMapping(combo);
        MappingButton->setText(combo);
        pressedButtons.clear();
    }

    if (MappingCompleted || MappingTimer <= 0) {
        button->setText(mapping);
        EnableButtonMapping = false;
        EnableAxisMapping = false;
        L2Pressed = false;
        R2Pressed = false;
        EnableMappingButtons();
        timer->stop();
    }
}

void ControlSettings::SetMapping(QString input) {
    mapping = input;
    MappingCompleted = true;
    if (EnableAxisMapping) {
        emit PushGamepadEvent();
        emit AxisChanged();
    }
}

// use QT events instead of SDL to override default event closing the window with escape
bool ControlSettings::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress && EnableButtonMapping) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            SetMapping("unmapped");
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void ControlSettings::processSDLEvents(int Type, int Input, int Value) {
    if (EnableButtonMapping) {

        if (pressedButtons.size() >= 3) {
            return;
        }

        if (Type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (Input) {
            case SDL_GAMEPAD_BUTTON_SOUTH:
                pressedButtons.insert(5, "cross");
                break;
            case SDL_GAMEPAD_BUTTON_EAST:
                pressedButtons.insert(6, "circle");
                break;
            case SDL_GAMEPAD_BUTTON_NORTH:
                pressedButtons.insert(7, "triangle");
                break;
            case SDL_GAMEPAD_BUTTON_WEST:
                pressedButtons.insert(8, "square");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                pressedButtons.insert(3, "l1");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                pressedButtons.insert(4, "r1");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK:
                pressedButtons.insert(9, "l3");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                pressedButtons.insert(10, "r3");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
                pressedButtons.insert(13, "pad_up");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                pressedButtons.insert(14, "pad_down");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                pressedButtons.insert(15, "pad_left");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                pressedButtons.insert(16, "pad_right");
                break;
            case SDL_GAMEPAD_BUTTON_BACK:
                pressedButtons.insert(11, "back");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
                pressedButtons.insert(17, "lpaddle_high");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
                pressedButtons.insert(18, "rpaddle_high");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
                pressedButtons.insert(19, "lpaddle_low");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
                pressedButtons.insert(20, "rpaddle_low");
                break;
            case SDL_GAMEPAD_BUTTON_START:
                pressedButtons.insert(12, "options");
                break;
            default:
                break;
            }
        }

        if (Type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            // SDL trigger axis values range from 0 to 32000, set mapping on half movement
            // Set zone for trigger release signal arbitrarily at 5000
            switch (Input) {
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                if (Value > 16000) {
                    pressedButtons.insert(1, "l2");
                    L2Pressed = true;
                } else if (Value < 5000) {
                    if (L2Pressed && !R2Pressed)
                        emit PushGamepadEvent();
                }
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                if (Value > 16000) {
                    pressedButtons.insert(2, "r2");
                    R2Pressed = true;
                } else if (Value < 5000) {
                    if (R2Pressed && !L2Pressed)
                        emit PushGamepadEvent();
                }
                break;
            default:
                break;
            }
        }

        if (Type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            emit PushGamepadEvent();

    } else if (EnableAxisMapping) {
        if (Type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            // SDL stick axis values range from -32000 to 32000, set mapping on half movement
            if (Value > 16000 || Value < -16000) {
                switch (Input) {
                case SDL_GAMEPAD_AXIS_LEFTX:
                    SetMapping("axis_left_x");
                    break;
                case SDL_GAMEPAD_AXIS_LEFTY:
                    SetMapping("axis_left_y");
                    break;
                case SDL_GAMEPAD_AXIS_RIGHTX:
                    SetMapping("axis_right_x");
                    break;
                case SDL_GAMEPAD_AXIS_RIGHTY:
                    SetMapping("axis_right_y");
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (Type == SDL_EVENT_GAMEPAD_ADDED || SDL_EVENT_GAMEPAD_REMOVED) {
        ui->ActiveGamepadBox->clear();
        CheckGamePad();
    }
}

void ControlSettings::pollSDLEvents() {
    SDL_Event event;
    while (SdlEventWrapper::Wrapper::wrapperActive) {

        if (!SDL_WaitEvent(&event)) {
            return;
        }

        if (event.type == SDL_EVENT_QUIT) {
            return;
        }

        SdlEventWrapper::Wrapper::GetInstance()->Wrapper::ProcessEvent(&event);
    }
}

void ControlSettings::Cleanup() {
    SdlEventWrapper::Wrapper::wrapperActive = false;
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }

    SDL_free(gamepads);

    if (!GameRunning) {
        SDL_Event quitLoop{};
        quitLoop.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitLoop);
        Polling.waitForFinished();

        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        SDL_Quit();
    } else {
        if (!Config::getBackgroundControllerInput()) {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");
        }
        SDL_Event checkGamepad{};
        checkGamepad.type = SDL_EVENT_CHANGE_CONTROLLER;
        SDL_PushEvent(&checkGamepad);
    }
}

ControlSettings::~ControlSettings() {}
