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
                   ui->DpadRightButton,
                   ui->LStickUpButton,
                   ui->LStickDownButton,
                   ui->LStickLeftButton,
                   ui->LStickRightButton,
                   ui->RStickUpButton,
                   ui->RStickDownButton,
                   ui->RStickLeftButton,
                   ui->RStickRightButton,
                   ui->LHalfButton,
                   ui->RHalfButton};

    ButtonConnects();
    SetUIValuestoMappings("default");
    installEventFilter(this);

    ui->ProfileComboBox->setCurrentText("Common Config");
    ui->TitleLabel->setText("Common Config");
    config_id = "default";

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

    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Restore Defaults"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    connect(ui->HelpButton, &QPushButton::clicked, this, &KBMSettings::onHelpClicked);
    connect(ui->TextEditorButton, &QPushButton::clicked, this, [this]() {
        auto kbmWindow = new EditorDialog(this);
        kbmWindow->exec();
        SetUIValuestoMappings(config_id);
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
    for (auto& button : ButtonsList) {
        connect(button, &QPushButton::clicked, this, [this, &button]() { StartTimer(button); });
    }
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

void KBMSettings::SaveKBMConfig(bool close_on_save) {
    std::string output_string = "", input_string = "";
    std::vector<std::string> lines, inputs;

    // Comment lines for config file
    lines.push_back("#Feeling lost? Check out the Help section!");
    lines.push_back("");
    lines.push_back("#Keyboard bindings");
    lines.push_back("");

    // Lambda to reduce repetitive code for mapping buttons to config lines
    auto add_mapping = [&](const QString& buttonText, const std::string& output_name) {
        input_string = buttonText.toStdString();
        output_string = output_name;
        lines.push_back(output_string + " = " + input_string);
        if (input_string != "unmapped") {
            inputs.push_back(input_string);
        }
    };

    add_mapping(ui->CrossButton->text(), "cross");
    add_mapping(ui->CircleButton->text(), "circle");
    add_mapping(ui->TriangleButton->text(), "triangle");
    add_mapping(ui->SquareButton->text(), "square");

    lines.push_back("");

    add_mapping(ui->DpadUpButton->text(), "pad_up");
    add_mapping(ui->DpadDownButton->text(), "pad_down");
    add_mapping(ui->DpadLeftButton->text(), "pad_left");
    add_mapping(ui->DpadRightButton->text(), "pad_right");

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

    add_mapping(ui->LStickUpButton->text(), "axis_left_y_minus");
    add_mapping(ui->LStickDownButton->text(), "axis_left_y_plus");
    add_mapping(ui->LStickLeftButton->text(), "axis_left_x_minus");
    add_mapping(ui->LStickRightButton->text(), "axis_left_x_plus");

    lines.push_back("");

    add_mapping(ui->RStickUpButton->text(), "axis_right_y_minus");
    add_mapping(ui->RStickDownButton->text(), "axis_right_y_plus");
    add_mapping(ui->RStickLeftButton->text(), "axis_right_x_minus");
    add_mapping(ui->RStickRightButton->text(), "axis_right_x_plus");

    lines.push_back("");

    add_mapping(ui->MouseJoystickBox->currentText(), "mouse_to_joystick");
    add_mapping(ui->LHalfButton->text(), "leftjoystick_halfmode");
    add_mapping(ui->RHalfButton->text(), "rightjoystick_halfmode");

    std::string DOString = std::format("{:.2f}", (ui->DeadzoneOffsetSlider->value() / 100.f));
    std::string SMString = std::format("{:.1f}", (ui->SpeedMultiplierSlider->value() / 10.f));
    std::string SOString = std::format("{:.3f}", (ui->SpeedOffsetSlider->value() / 1000.f));
    input_string = DOString + ", " + SMString + ", " + SOString;
    output_string = "mouse_movement_params";
    lines.push_back(output_string + " = " + input_string);

    lines.push_back("");
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
    Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");

    if (close_on_save)
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

    ui->TouchpadLeftButton->setText("space");
    ui->TouchpadCenterButton->setText("unmapped");
    ui->TouchpadRightButton->setText("unmapped");
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
            } else if (output_string == "touchpad_left") {
                ui->TouchpadLeftButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "touchpad_center") {
                ui->TouchpadCenterButton->setText(QString::fromStdString(input_string));
            } else if (output_string == "touchpad_right") {
                ui->TouchpadRightButton->setText(QString::fromStdString(input_string));
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
                    int DOffsetInt = static_cast<int>(DOffsetValue);
                    ui->DeadzoneOffsetSlider->setValue(DOffsetInt);
                    QString LabelValue = QString::number(DOffsetInt / 100.0, 'f', 2);
                    QString LabelString = tr("Deadzone Offset (def 0.50):") + " " + LabelValue;
                    ui->DeadzoneOffsetLabel->setText(LabelString);

                    std::string SMSOstring = line.substr(comma_pos + 1);
                    std::size_t comma_pos2 = SMSOstring.find(',');
                    if (comma_pos2 != std::string::npos) {
                        std::string SMstring = SMSOstring.substr(0, comma_pos2);
                        float SpeedMultValue = std::clamp(std::stof(SMstring) * 10.0f, 1.0f, 50.0f);
                        int SpeedMultInt = static_cast<int>(SpeedMultValue);
                        ui->SpeedMultiplierSlider->setValue(SpeedMultInt);
                        LabelValue = QString::number(SpeedMultInt / 10.0, 'f', 1);
                        LabelString = tr("Speed Multiplier (def 1.0):") + " " + LabelValue;
                        ui->SpeedMultiplierLabel->setText(LabelString);

                        std::string SOstring = SMSOstring.substr(comma_pos2 + 1);
                        float SOffsetValue = std::stof(SOstring) * 1000.0;
                        int SOffsetInt = static_cast<int>(SOffsetValue);
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
    config_id = (ui->ProfileComboBox->currentText() == "Common Config")
                    ? "default"
                    : ui->ProfileComboBox->currentText().toStdString();
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

    if (pressedKeys.size() > 0) {
        QStringList keyStrings;

        for (const QString& buttonAction : pressedKeys) {
            keyStrings << buttonAction;
        }

        QString combo = keyStrings.join(",");
        SetMapping(combo);
        MappingCompleted = true;
        EnableMapping = false;

        MappingButton->setText(combo);
        pressedKeys.clear();
        timer->stop();
    }
    if (MappingCompleted) {
        EnableMapping = false;
        EnableMappingButtons();
        timer->stop();

        button->setText(mapping);
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

// Helper lambda to get the modified button text based on the current keyboard modifiers
auto GetModifiedButton = [](Qt::KeyboardModifiers modifier, const std::string& m_button,
                            const std::string& n_button) -> QString {
    if (QApplication::keyboardModifiers() & modifier) {
        return QString::fromStdString(m_button);
    } else {
        return QString::fromStdString(n_button);
    }
};

bool KBMSettings::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Close) {
        if (HelpWindowOpen) {
            HelpWindow->close();
            HelpWindowOpen = false;
        }
    }

    if (EnableMapping) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->isAutoRepeat())
                return true;

            if (pressedKeys.size() >= 3) {
                return true;
            }

            switch (keyEvent->key()) {
            // alphanumeric
            case Qt::Key_A:
                pressedKeys.insert("a");
                break;
            case Qt::Key_B:
                pressedKeys.insert("b");
                break;
            case Qt::Key_C:
                pressedKeys.insert("c");
                break;
            case Qt::Key_D:
                pressedKeys.insert("d");
                break;
            case Qt::Key_E:
                pressedKeys.insert("e");
                break;
            case Qt::Key_F:
                pressedKeys.insert("f");
                break;
            case Qt::Key_G:
                pressedKeys.insert("g");
                break;
            case Qt::Key_H:
                pressedKeys.insert("h");
                break;
            case Qt::Key_I:
                pressedKeys.insert("i");
                break;
            case Qt::Key_J:
                pressedKeys.insert("j");
                break;
            case Qt::Key_K:
                pressedKeys.insert("k");
                break;
            case Qt::Key_L:
                pressedKeys.insert("l");
                break;
            case Qt::Key_M:
                pressedKeys.insert("m");
                break;
            case Qt::Key_N:
                pressedKeys.insert("n");
                break;
            case Qt::Key_O:
                pressedKeys.insert("o");
                break;
            case Qt::Key_P:
                pressedKeys.insert("p");
                break;
            case Qt::Key_Q:
                pressedKeys.insert("q");
                break;
            case Qt::Key_R:
                pressedKeys.insert("r");
                break;
            case Qt::Key_S:
                pressedKeys.insert("s");
                break;
            case Qt::Key_T:
                pressedKeys.insert("t");
                break;
            case Qt::Key_U:
                pressedKeys.insert("u");
                break;
            case Qt::Key_V:
                pressedKeys.insert("v");
                break;
            case Qt::Key_W:
                pressedKeys.insert("w");
                break;
            case Qt::Key_X:
                pressedKeys.insert("x");
                break;
            case Qt::Key_Y:
                pressedKeys.insert("y");
                break;
            case Qt::Key_Z:
                pressedKeys.insert("z");
                break;
            case Qt::Key_0:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp0", "0"));
                break;
            case Qt::Key_1:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp1", "1"));
                break;
            case Qt::Key_2:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp2", "2"));
                break;
            case Qt::Key_3:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp3", "3"));
                break;
            case Qt::Key_4:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp4", "4"));
                break;
            case Qt::Key_5:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp5", "5"));
                break;
            case Qt::Key_6:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp6", "6"));
                break;
            case Qt::Key_7:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp7", "7"));
                break;
            case Qt::Key_8:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp8", "8"));
                break;
            case Qt::Key_9:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp9", "9"));
                break;

            // symbols
            case Qt::Key_Exclam:
                pressedKeys.insert("!");
                break;
            case Qt::Key_At:
                pressedKeys.insert("@");
                break;
            case Qt::Key_NumberSign:
                pressedKeys.insert("#");
                break;
            case Qt::Key_Dollar:
                pressedKeys.insert("$");
                break;
            case Qt::Key_Percent:
                pressedKeys.insert("%");
                break;
            case Qt::Key_AsciiCircum:
                pressedKeys.insert("^");
                break;
            case Qt::Key_Ampersand:
                pressedKeys.insert("&");
                break;
            case Qt::Key_Asterisk:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp*", "*"));
                break;
            case Qt::Key_ParenLeft:
                pressedKeys.insert("(");
                break;
            case Qt::Key_ParenRight:
                pressedKeys.insert(")");
                break;
            case Qt::Key_Minus:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp-", "-"));
                break;
            case Qt::Key_Underscore:
                pressedKeys.insert("_");
                break;
            case Qt::Key_Equal:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp=", "="));
                break;
            case Qt::Key_Plus:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp+", "+"));
                break;
            case Qt::Key_BracketLeft:
                pressedKeys.insert("[");
                break;
            case Qt::Key_BracketRight:
                pressedKeys.insert("]");
                break;
            case Qt::Key_BraceLeft:
                pressedKeys.insert("{");
                break;
            case Qt::Key_BraceRight:
                pressedKeys.insert("}");
                break;
            case Qt::Key_Backslash:
                pressedKeys.insert("\\");
                break;
            case Qt::Key_Bar:
                pressedKeys.insert("|");
                break;
            case Qt::Key_Semicolon:
                pressedKeys.insert(";");
                break;
            case Qt::Key_Colon:
                pressedKeys.insert(":");
                break;
            case Qt::Key_Apostrophe:
                pressedKeys.insert("'");
                break;
            case Qt::Key_QuoteDbl:
                pressedKeys.insert("\"");
                break;
            case Qt::Key_Comma:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp,", ","));
                break;
            case Qt::Key_Less:
                pressedKeys.insert("<");
                break;
            case Qt::Key_Period:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp.", "."));
                break;
            case Qt::Key_Greater:
                pressedKeys.insert(">");
                break;
            case Qt::Key_Slash:
                pressedKeys.insert(GetModifiedButton(Qt::KeypadModifier, "kp/", "/"));
                break;
            case Qt::Key_Question:
                pressedKeys.insert("question");
                break;

            // special keys
            case Qt::Key_Print:
                pressedKeys.insert("printscreen");
                break;
            case Qt::Key_ScrollLock:
                pressedKeys.insert("scrolllock");
                break;
            case Qt::Key_Pause:
                pressedKeys.insert("pausebreak");
                break;
            case Qt::Key_Backspace:
                pressedKeys.insert("backspace");
                break;
            case Qt::Key_Insert:
                pressedKeys.insert("insert");
                break;
            case Qt::Key_Delete:
                pressedKeys.insert("delete");
                break;
            case Qt::Key_Home:
                pressedKeys.insert("home");
                break;
            case Qt::Key_End:
                pressedKeys.insert("end");
                break;
            case Qt::Key_PageUp:
                pressedKeys.insert("pgup");
                break;
            case Qt::Key_PageDown:
                pressedKeys.insert("pgdown");
                break;
            case Qt::Key_Tab:
                pressedKeys.insert("tab");
                break;
            case Qt::Key_CapsLock:
                pressedKeys.insert("capslock");
                break;
            case Qt::Key_Return:
                pressedKeys.insert("enter");
                break;
            case Qt::Key_Enter:
                pressedKeys.insert(GetModifiedButton(Qt::ShiftModifier, "kpenter", "enter"));
                break;
            case Qt::Key_Shift:
                if (keyEvent->nativeScanCode() == LSHIFT_KEY) {
                    pressedKeys.insert("lshift");
                } else {
                    pressedKeys.insert("rshift");
                }
                break;
            case Qt::Key_Alt:
                if (keyEvent->nativeScanCode() == LALT_KEY) {
                    pressedKeys.insert("lalt");
                } else {
                    pressedKeys.insert("ralt");
                }
                break;
            case Qt::Key_Control:
                if (keyEvent->nativeScanCode() == LCTRL_KEY) {
                    pressedKeys.insert("lctrl");
                } else {
                    pressedKeys.insert("rctrl");
                }
                break;
            case Qt::Key_Meta:
                activateWindow();
#ifdef _WIN32
                pressedKeys.insert("lwin");
#else
                pressedKeys.insert("lmeta");
#endif
                break;
            case Qt::Key_Space:
                pressedKeys.insert("space");
                break;
            case Qt::Key_Up:
                activateWindow();
                pressedKeys.insert("up");
                break;
            case Qt::Key_Down:
                pressedKeys.insert("down");
                break;
            case Qt::Key_Left:
                pressedKeys.insert("left");
                break;
            case Qt::Key_Right:
                pressedKeys.insert("right");
                break;

            // cancel mapping
            case Qt::Key_Escape:
                pressedKeys.insert("unmapped");
                break;

            // default case
            default:
                break;
                // bottom text
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (pressedKeys.size() < 3) {
            switch (mouseEvent->button()) {
            case Qt::LeftButton:
                pressedKeys.insert("leftbutton");
                break;
            case Qt::RightButton:
                pressedKeys.insert("rightbutton");
                break;
            case Qt::MiddleButton:
                pressedKeys.insert("middlebutton");
                break;
            case Qt::XButton1:
                pressedKeys.insert("sidebuttonback");
                break;
            case Qt::XButton2:
                pressedKeys.insert("sidebuttonforward");
                break;

                // default case
            default:
                break;
                // bottom text
            }
            return true;
        }
    }

    const QList<QPushButton*> AxisList = {
        ui->LStickUpButton, ui->LStickDownButton, ui->LStickLeftButton, ui->LStickRightButton,
        ui->RStickUpButton, ui->LStickDownButton, ui->LStickLeftButton, ui->RStickRightButton};

    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if (pressedKeys.size() < 3) {
            if (wheelEvent->angleDelta().y() > 5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    pressedKeys.insert("mousewheelup");
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            } else if (wheelEvent->angleDelta().y() < -5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    pressedKeys.insert("mousewheeldown");
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            }
            if (wheelEvent->angleDelta().x() > 5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    // QT changes scrolling to horizontal for all widgets with the alt modifier
                    pressedKeys.insert(
                        GetModifiedButton(Qt::AltModifier, "mousewheelup", "mousewheelright"));
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            } else if (wheelEvent->angleDelta().x() < -5) {
                if (std::find(AxisList.begin(), AxisList.end(), MappingButton) == AxisList.end()) {
                    pressedKeys.insert(
                        GetModifiedButton(Qt::AltModifier, "mousewheeldown", "mousewheelleft"));
                } else {
                    QMessageBox::information(this, tr("Cannot set mapping"),
                                             tr("Mousewheel cannot be mapped to stick outputs"));
                }
            }
        }
    }

    return QDialog::eventFilter(obj, event);
}

KBMSettings::~KBMSettings() {}