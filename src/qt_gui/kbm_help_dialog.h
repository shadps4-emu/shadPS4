// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

class ExpandableSection : public QWidget {
    Q_OBJECT
public:
    explicit ExpandableSection(const QString& title, const QString& content, QWidget* parent);

signals:
    void expandedChanged(); // Signal to indicate layout size change

private:
    QPushButton* toggleButton;
    QTextBrowser* contentBrowser; // Changed from QLabel to QTextBrowser
    QPropertyAnimation* animation;
    int contentHeight;
    void updateContentHeight() {
        int contentHeight = contentBrowser->document()->size().height();
        contentBrowser->setMinimumHeight(contentHeight + 5);
        contentBrowser->setMaximumHeight(contentHeight + 50);
    }
};

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(bool* open_flag = nullptr, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void reject() override;

private:
    bool* help_open_ptr;

    QString quickstart() {
        return
            R"(The keyboard and controller remapping backend, GUI and documentation have been written by kalaposfos

In this section, you will find information about the project, its features and help on setting up your ideal setup.
To view the config file's syntax, check out the Syntax tab, for keybind names, visit Normal Keybinds and Special Bindings, and if you are here to view emulator-wide keybinds, you can find it in the FAQ section.
This project started out because I didn't like the original unchangeable keybinds, but rather than waiting for someone else to do it, I implemented this myself. From the default keybinds, you can clearly tell this was a project built for Bloodborne, but ovbiously you can make adjustments however you like.
)";
    }
    QString faq() {
        return
            R"(Q: What are the emulator-wide keybinds?
A: -F12: Triggers Renderdoc capture
-F11: Toggles fullscreen
-F10: Toggles FPS counter
-Ctrl F10: Open the debug menu
-F9: Pauses emultor, if the debug menu is open
-F8: Reparses the config file while in-game
-F7: Toggles mouse capture and mouse input

Q: How do I change between mouse and controller joystick input, and why is it even required?
A: You can switch between them with F7, and it is required, because mouse input is done with polling, which means mouse movement is checked every frame, and if it didn't move, the code manually sets the emulator's virtual controller to 0 (back to the center), even if other input devices would update it.

Q: What happens if I accidentally make a typo in the config?
A: The code recognises the line as wrong, and skip it, so the rest of the file will get parsed, but that line in question will be treated like a comment line. You can find these lines in the log, if you search for 'input_handler'.

Q: I want to bind <input> to <output>, but your code doesn't support <input>!
A: Some keys are intentionally omitted, but if you read the bindings through, and you're sure it is not there and isn't one of the intentionally disabled ones, open an issue on https://github.com/shadps4-emu/shadPS4.
)";
    }
    QString syntax() {
        return
            R"(This is the full list of currently supported mouse, keyboard and controller inputs, and how to use them.
Emulator-reserved keys: F1 through F12

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
axis_left_y_minus = w;

You can make a comment line by putting # as the first character.
Whitespace doesn't matter, <output>=<input>; is just as valid as <output> = <input>;
';' at the ends of lines is also optional.
)";
    }
    QString bindings() {
        return
            R"(The following names should be interpreted without the '' around them, and for inputs that have left and right versions, only the left one is shown, but the right can be inferred from that.
Example: 'lshift', 'rshift'

Keyboard:
Alphabet: 'a', 'b', ..., 'z'
Numbers: '0', '1', ..., '9'
Keypad: 'kp0', kp1', ..., 'kp9', 'kpperiod', 'kpcomma', 
        'kpdivide', 'kpmultiply', 'kpdivide', 'kpplus', 'kpminus', 'kpenter'
Punctuation and misc:
        'space', 'comma', 'period', 'question', 'semicolon', 'minus', 'plus', 'lparenthesis', 'lbracket', 'lbrace', 'backslash', 'dash',
        'enter', 'tab', backspace', 'escape'
Arrow keys: 'up', 'down', 'left', 'right'
Modifier keys:
        'lctrl', 'lshift', 'lalt', 'lwin' = 'lmeta' (same input, different names, so if you are not on Windows and don't like calling this the Windows key, there is an alternative)

Mouse:
        'leftbutton', 'rightbutton', 'middlebutton', 'sidebuttonforward', 'sidebuttonback'
    The following wheel inputs cannot be bound to axis input, only button:
        'mousewheelup', 'mousewheeldown', 'mousewheelleft', 'mousewheelright'

Controller:
    The touchpad currently can't be rebound to anything else, but you can bind buttons to it.
    If you have a controller that has different names for buttons, it will still work, just look up what are the equivalent names for that controller
    The same left-right rule still applies here.
    Buttons:
        'triangle', 'circle', 'cross', 'square', 'l1', 'l3',
        'options', touchpad', 'up', 'down', 'left', 'right'
    Axes if you bind them to a button input:
        'axis_left_x_plus', 'axis_left_x_minus', 'axis_left_y_plus', 'axis_left_y_minus',
        'axis_right_x_plus', ..., 'axis_right_y_minus',
        'l2'
    Axes if you bind them to another axis input:
        'axis_left_x' 'axis_left_y' 'axis_right_x' 'axis_right_y',
        'l2'
)";
    }
    QString special() {
        return
            R"(There are some extra bindings you can put into the config file, that don't correspond to a controller input, but rather something else.
You can find these here, with detailed comments, examples and suggestions for most of them.

'leftjoystick_halfmode' and 'rightjoystick_halfmode' = <key>;
    These are a pair of input modifiers, that change the way keyboard button bound axes work. By default, those push the joystick to the max in their respective direction, but if their respective joystick_halfmode modifier value is true, they only push it... halfway. With this, you can change from run to walk in games like Bloodborne.

'mouse_to_joystick' = 'none', 'left' or 'right';
    This binds the mouse movement to either joystick. If it recieves a value that is not 'left' or 'right', it defaults to 'none'.

'mouse_movement_params' = float, float, float;
    (If you don't know what a float is, it is a data type that stores non-whole numbers.)
    Default values: 0.5, 1, 0.125
    Let's break each parameter down:
        1st: mouse_deadzone_offset: this value should have a value between 0 and 1 (It gets clamped to that range anyway), with 0 being no offset and 1 being pushing the joystick to the max in the direction the mouse moved.
            This controls the minimum distance the joystick gets moved, when moving the mouse. If set to 0, it will emulate raw mouse input, which doesn't work very well due to deadzones preventing input if the movement is not large enough.
        2nd: mouse_speed: It's just a standard multiplier to the mouse input speed.
            If you input a negative number, the axis directions get reversed (Keep in mind that the offset can still push it back to positive, if it's big enough)
        3rd: mouse_speed_offset: This also should be in the 0 to 1 range, with 0 being no offset and 1 being offsetting to the max possible value.
            This is best explained through an example: Let's set mouse_deadzone to 0.5, and this to 0: This means that if we move the mousevery slowly, it still inputs a half-strength joystick input, and if we increase the speed, it would stay that way until we move faster than half the max speed. If we instead set this to 0.25, we now only need to move the mouse faster than the 0.5-0.25=0.25=quarter of the max speed, to get an increase in joystick speed. If we set it to 0.5, then even moving the mouse at 1 pixel per frame will result in a faster-than-minimum speed.

'key_toggle' = <key>, <key_to_toggle>; 
    This assigns a key to another key, and if pressed, toggles that key's virtual value. If it's on, then it doesn't matter if the key is pressed or not, the input handler will treat it as if it's pressed.
    You can make an input toggleable with this, for example: Let's say we want to be able to toggle l1 with t. You can then bind l1 to a key you won't use, like kpenter, then bind t to toggle that, so you will end up with this:
        l1 = kpenter;
        key_toggle = t, kpenter;
)";
    }
};