// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kbm_help_dialog.h"

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

ExpandableSection::ExpandableSection(const QString& title, const QString& content,
                                     QWidget* parent = nullptr)
    : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Button to toggle visibility of content
    toggleButton = new QPushButton(title);
    layout->addWidget(toggleButton);

    // QTextBrowser for content (initially hidden)
    contentBrowser = new QTextBrowser();
    contentBrowser->setPlainText(content);
    contentBrowser->setVisible(false);

    // Remove scrollbars from QTextBrowser
    contentBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set size policy to allow vertical stretching only
    contentBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Calculate and set initial height based on content
    updateContentHeight();

    layout->addWidget(contentBrowser);

    // Connect button click to toggle visibility
    connect(toggleButton, &QPushButton::clicked, [this]() {
        contentBrowser->setVisible(!contentBrowser->isVisible());
        if (contentBrowser->isVisible()) {
            updateContentHeight(); // Update height when expanding
        }
        emit expandedChanged(); // Notify for layout adjustments
    });

    // Connect to update height if content changes
    connect(contentBrowser->document(), &QTextDocument::contentsChanged, this,
            &ExpandableSection::updateContentHeight);

    // Minimal layout settings for spacing
    layout->setSpacing(2);
    layout->setContentsMargins(0, 0, 0, 0);
}

void HelpDialog::closeEvent(QCloseEvent* event) {
    *help_open_ptr = false;
    close();
}
void HelpDialog::reject() {
    *help_open_ptr = false;
    close();
}

HelpDialog::HelpDialog(bool* open_flag, QWidget* parent) : QDialog(parent) {
    help_open_ptr = open_flag;
    // Main layout for the help dialog
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Container widget for the scroll area
    QWidget* containerWidget = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(containerWidget);

    // Add expandable sections to container layout
    auto* quickstartSection = new ExpandableSection(tr("Quickstart"), quickstart());
    auto* syntaxSection = new ExpandableSection(tr("Syntax"), syntax());
    auto* bindingsSection = new ExpandableSection(tr("Keybindings"), bindings());
    auto* specialSection = new ExpandableSection(tr("Special Bindings"), special());
    auto* faqSection = new ExpandableSection(tr("FAQ"), faq());

    containerLayout->addWidget(quickstartSection);
    containerLayout->addWidget(syntaxSection);
    containerLayout->addWidget(bindingsSection);
    containerLayout->addWidget(specialSection);
    containerLayout->addWidget(faqSection);
    containerLayout->addStretch(1);

    // Scroll area wrapping the container
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(containerWidget);

    // Add the scroll area to the main dialog layout
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

    // Minimum size for the dialog
    setMinimumSize(500, 400);

    // Re-adjust dialog layout when any section expands/collapses
    connect(quickstartSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(faqSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(syntaxSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(specialSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
    connect(bindingsSection, &ExpandableSection::expandedChanged, this, &HelpDialog::adjustSize);
}

// Helper functions that store the text contents for each tab inb the HelpDialog menu
QString HelpDialog::quickstart() {
    return R"(
The keyboard and controller remapping backend, GUI, and documentation have been written by kalaposfos.

In this section, you will find information about the project and its features, as well as help setting up your ideal setup.
To view the config file's syntax, check out the Syntax tab, for keybind names, visit Normal Keybinds and Special Bindings, and if you are here to view emulator-wide keybinds, you can find it in the FAQ section.
This project began because I disliked the original, unchangeable keybinds. Rather than waiting for someone else to do it, I implemented this myself. From the default keybinds, you can clearly tell this was a project built for Bloodborne, but obviously, you can make adjustments however you like.)";
}

QString HelpDialog::faq() {
    return R"(
Q: What are the emulator-wide keybinds?
A:
-F12: Triggers Renderdoc capture
-F11: Toggles fullscreen
-F10: Toggles FPS counter
-Ctrl+F10: Open the debug menu
-F9: Pauses the emulator if the debug menu is open
-F8: Reparses the config file while in-game
-F7: Toggles mouse capture and mouse input
-F6: Toggles mouse-to-gyro emulation

Q: How do I switch between mouse and controller joystick input? Why is it even required?
A: Pressing F7 toggles between mouse and controller joystick input. It is required because the program polls the mouse input, which means it checks mouse movement every frame. If it didn't move, the code would manually set the emulator's virtual controller to 0 (to the center), even if other input devices would update it.

Q: What happens if I accidentally make a typo in the config?
A: The code recognises the line as wrong and skips it, so the rest of the file will get parsed, but that line in question will be treated like a comment line. You can find these lines in the log if you search for 'input_handler'.

Q: I want to bind <input> to <output>, but your code doesn't support <input>!
A: Some keys are intentionally omitted, but if you read the bindings through, and you're sure it is not there and isn't one of the intentionally disabled ones, open an issue on https://github.com/shadps4-emu/shadPS4.

Q: What does default.ini do?
A: If you're using per-game configs, it's the base from which all new games generate their config file. If you use the unified config, then default.ini is used for every game directly instead.

Q: What does the use Per-game Config checkbox do?
A: It controls whether the config is loaded from CUSAXXXXX.ini for a game or from default.ini. This way, if you only want to manage one set of bindings, you can do so, but if you want to use a different setup for every game, that's possible as well.)";
}

QString HelpDialog::syntax() {
    return R"(
Below is the file format for mouse, keyboard, and controller inputs:

Rules:
- You can bind up to 3 inputs to one button.
- Adding '#' at the beginning of a line creates a comment.
- Extra whitespace doesn't affect input.
    <output>=<input>; is just as valid as <output> = <input>;
- ';' at the end of a line is also optional.

Syntax (aka how a line can look like):
    #Comment line
    <controller_button> = <input>, <input>, <input>;
    <controller_button> = <input>, <input>;
    <controller_button> = <input>;

Examples:
    #Interact
    cross = e;
    #Heavy attack (in BB)
    r2 = leftbutton, lshift;
    #Move forward
    axis_left_y_minus = w;)";
}

QString HelpDialog::bindings() {
    return R"(
The following names should be interpreted without the '' around them. For inputs that have left and right versions, only the left one is shown, but the right version also works.
    (Example: 'lshift', 'rshift')

Keyboard:
    Alphabet:
        'a', 'b', ..., 'z'
    Numbers:
        '0', '1', ..., '9'
    Keypad:
        'kp 0', 'kp 1', ..., 'kp 9',
        'kp .', 'kp ,', 'kp /', 'kp *', 'kp -', 'kp +', 'kp =', 'kp enter'
    Symbols:
        '`', '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '_', '=', '+', '{', '}', '[', ']', '\', '|',
        ';', ':', ''', '"', ',', '<', '.', '>', '/', '?'
    Special keys:
        'escape (text editor only)', 'printscreen', 'scrolllock', 'pausebreak',
        'backspace', 'insert', 'delete', 'home', 'end', 'pgup', 'pgdown', 'tab',
        'capslock', 'enter', 'space'
    Arrow keys:
        'up', 'down', 'left', 'right'
    Modifier keys:
        'lctrl', 'lshift', 'lalt', 'lwin' = 'lmeta' (same input, different names, so if you are not on Windows and don't like calling this the Windows key, there is an alternative)

Mouse:
        'leftbutton', 'rightbutton', 'middlebutton', 'sidebuttonforward',
        'sidebuttonback'
    The following wheel inputs cannot be bound to axis input, only button:
        'mousewheelup', 'mousewheeldown', 'mousewheelleft',
        'mousewheelright'

Controller:
    The touchpad currently can't be rebound to anything else, but you can bind buttons to it.
    If you have a controller that has different names for buttons, it will still work. Just look up the equivalent names for that controller.
    The same left-right rule still applies here.
    Buttons:
            'triangle', 'circle', 'cross', 'square', 'l1', 'l3',
            'options', touchpad', 'up', 'down', 'left', 'right'
        Input-only:
             'lpaddle_low', 'lpaddle_high'
        Output-only:
            'touchpad_left', 'touchpad_center', 'touchpad_right'
    Axes if you bind them to a button input:
        'axis_left_x_plus', 'axis_left_x_minus', 'axis_left_y_plus', 'axis_left_y_minus',
        'axis_right_x_plus', ..., 'axis_right_y_minus',
        'l2'
    Axes if you bind them to another axis input:
        'axis_left_x', 'axis_left_y', 'axis_right_x', 'axis_right_y',
        'l2'

Invalid Inputs:
    'F1-F12' are reserved for emulator-wide keybinds, and cannot be bound to controller inputs.)";
}

QString HelpDialog::special() {
    return R"(
There are some extra bindings you can put in the config file that don't correspond to a controller input but something else.
You can find these here, with detailed comments, examples, and suggestions for most of them.

'leftjoystick_halfmode' and 'rightjoystick_halfmode' = <key>;
    These are a pair of input modifiers that change the way keyboard button-bound axes work. By default, those push the joystick to the max in their respective direction, but if their respective 'joystick_halfmode' modifier value is true, they only push it... halfway. With this, you can change from run to walk in games like Bloodborne.

'mouse_to_joystick' = 'none', 'left' or 'right';
    This binds the mouse movement to either joystick. If it receives a value that is not 'left' or 'right', it defaults to 'none'.

'mouse_movement_params' = float, float, float;
    (If you don't know what a float is, it is a data type that stores decimal numbers.)
    Default values: 0.5, 1, 0.125
    Let's break each parameter down:
        1st: 'mouse_deadzone_offset' should have a value between 0 and 1 (it gets clamped to that range anyway), with 0 being no offset and 1 being pushing the joystick to the max in the direction the mouse moved.
            This controls the minimum distance the joystick gets moved when moving the mouse. If set to 0, it will emulate raw mouse input, which doesn't work very well due to deadzones preventing input if the movement is not large enough.
        2nd: 'mouse_speed' is just a standard multiplier to the mouse input speed.
            If you input a negative number, the axis directions get reversed. Keep in mind that the offset can still push it back to positive if it's big enough.
        3rd: 'mouse_speed_offset' should be in the 0 to 1 range, with 0 being no offset and 1 being offsetting to the max possible value.
            Let's set 'mouse_deadzone_offset' to 0.5, and 'mouse_speed_offset' to 0: This means that if we move the mouse very slowly, it still inputs a half-strength joystick input, and if we increase the speed, it would stay that way until we move faster than half the max speed. If we instead set this to 0.25, we now only need to move the mouse faster than the 0.5-0.25=0.25=quarter of the max speed, to get an increase in joystick speed. If we set it to 0.5, then even moving the mouse at 1 pixel per frame will result in a faster-than-minimum speed.

'key_toggle' = <key>, <key_to_toggle>; 
    This assigns a key to another key, and if pressed, toggles that key's virtual value. If it's on, then it doesn't matter if the key is pressed or not, the input handler will treat it as if it's pressed.
    Let's say we want to be able to toggle 'l1' with 't'. You can then bind 'l1' to a key you won't use, like 'kpenter'. Then bind 't' to toggle that. You will end up with this:
        l1 = kpenter;
        key_toggle = t, kpenter;

'analog_deadzone' = <device>, <value>, <value>;
    This sets the deadzone range for various inputs. The first value is the minimum deadzone while the second is the maximum. Values go from 1 to 127 (no deadzone to max deadzone).
    If you only want a minimum or maximum deadzone, set the other value to 1 or 127 respectively.
    Valid devices: 'leftjoystick', 'rightjoystick', 'l2', 'r2'

'mouse_gyro_roll_mode':
    Controls whether moving the mouse sideways causes a panning or a rolling motion while mouse-to-gyro emulation is active.)";
}
