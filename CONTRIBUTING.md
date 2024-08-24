<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Style guidelines

## General Rules

* Line width is typically 100 characters. Please do not use 80-characters.
* Don't ever introduce new external dependencies into Core
* Don't use any platform specific code in Core
* Use namespaces often
* Avoid the use of C-style casts and instead prefer C++-style static_cast and reinterpret_cast. Try to avoid using dynamic_cast. Never use const_cast except for when dealing with external const-incorrect APIs.

## Naming Rules

* Functions: `PascalCase`
* Variables: `lower_case_underscored. Prefix with g_ if global.`
* Classes: `PascalCase`
* Files and Directories: `lower_case_underscored`
* Namespaces: `PascalCase`, `_` may also be used for clarity (e.g. `ARM_InitCore`)

# Indentation/Whitespace Style

Follow the indentation/whitespace style shown below. Do not use tabs, use 4-spaces instead.

# Comments

* For regular comments, use C++ style (//) comments, even for multi-line ones.
* For doc-comments (Doxygen comments), use /// if it's a single line, else use the /** */ style featured in the example. Start the text on the second line, not the first containing /**.
* For items that are both defined and declared in two separate files, put the doc-comment only next to the associated declaration. (In a header file, usually.) Otherwise, put it next to the implementation. Never duplicate doc-comments in both places.

```c++
// Includes should be sorted lexicographically
// STD includes first
#include <array>
#include <map>
#include <memory>

// then, library includes
#include <nihstro/shared_binary.h>

// finally, shadps4 includes
#include "common/math_util.h"
#include "common/vector_math.h"

// each major module is separated
#include "video_core/pica.h"
#include "video_core/video_core.h"

namespace Example {

// Namespace contents are not indented

// Declare globals at the top (better yet, don't use globals at all!)
int g_foo{}; // {} can be used to initialize types as 0, false, or nullptr
char* g_some_pointer{}; // Pointer * and reference & stick to the type name, and make sure to initialize as nullptr!

/// A colorful enum.
enum class SomeEnum {
    Red,   ///< The color of fire.
    Green, ///< The color of grass.
    Blue,  ///< Not actually the color of water.
};

/**
 * Very important struct that does a lot of stuff.
 * Note that the asterisks are indented by one space to align to the first line.
 */
struct Position {
    // Always initialize member variables!
    int x{};
    int y{};
};

// Use "typename" rather than "class" here
template <typename T>
void FooBar() {
    const std::string some_string{"prefer uniform initialization"};

    const std::array<int, 4> some_array{
        5,
        25,
        7,
        42,
    };

    if (note == the_space_after_the_if) {
        CallAFunction();
    } else {
        // Use a space after the // when commenting
    }

    // Place a single space after the for loop semicolons, prefer pre-increment
    for (int i = 0; i != 25; ++i) {
        // This is how we write loops
    }

    DoStuff(this, function, call, takes, up, multiple,
            lines, like, this);

    if (this || condition_takes_up_multiple &&
        lines && like && this || everything ||
        alright || then) {

        // Leave a blank space before the if block body if the condition was continued across
        // several lines.
    }

    // No indentation for case labels
    switch (var) {
    case 1: {
        const int case_var{var + 3};
        DoSomething(case_var);
        break;
    }
    case 3:
        DoSomething(var);
        return;
    default:
        // Yes, even break for the last case
        break;
    }
}

} // namespace Example
```
