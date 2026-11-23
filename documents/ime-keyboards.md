<!--
SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# IME keyboard languages

Both the embedded IME and the IME dialog use the profiles registered in
`src/core/libraries/ime/ime_kb_language.cpp`. English (US), English (UK), and Polish
currently use the shared Latin key tables.

`OrbisImeType::Number` selects the shared `Numeric` family in both IME interfaces.
It uses the PS4 three-column keypad with digits, `.`, `,`, `-`, backspace and a
two-column Done key. The numeric grid and close-only top panel take precedence
over language layout providers and alphabet-panel priority. Enter labels still
come from the selected language profile. Alphabet and symbol shortcuts are
disabled while the numeric keypad is active.

The initial selection prefers the configured console language when its profile
is allowed by the application's supported-language mask. Otherwise it selects an
allowed registered profile. A zero mask permits the configured profile. Requests
with no implemented matching profile retain the shared English fallback.
Opening a text keyboard with an unavailable language layout shows a notification
identifying the request and the English fallback. The shared numeric layout does
not require a language-specific alphabet layout and does not show this warning.

To integrate another keyboard language:

1. Add an `ImeKbLanguageProfile` entry with its existing `OrbisImeLanguage` bit and
   console-language ID. These are separate identifiers, not interchangeable values.
2. Define a complete `ImeKbLabels` table in `ime_kb_language.cpp` and point the
   profile's `labels` to it. Keep English and translated function-key, Enter
   (Default, Send, Search, Go), and edit-menu labels together in that source file.
   US and UK profiles share the English table; a null `labels` pointer also uses
   English. Set currency, preferred characters, and symbols/accents page counts
   on the profile. Layout tables define actions and character keys; their
   function-key text comes from the selected label table.
3. For a different key arrangement, implement `layout_provider` in the language's
   own source file and add that file to CMake. Return a pointer to a persistent
   `ImeKbLayoutModel` for each custom mode/case/page, or `nullptr` to reuse a shared
   table. The legacy `Latin` family denotes the alphabet panel; its model can
   contain another script. Providers must return their tables directly rather
   than calling `GetImeKeyboardLayout(selection)` recursively.
   Multiline models must provide a `NewLine` action, or return `nullptr` to use
   the shared multiline layout.
4. Set `top_panel_layout` if the language needs a different prediction/close grid.
   Navigation and keyboard sizing use the chosen model's rows, columns and spans.
5. Verify lowercase, Shift, Caps Lock, symbols, accents, controller navigation,
   function keys, and both IME interfaces against console output. Font collection
   visits every registered mode, case and page, including function keys and menus.
   Confirm that the installed font stack actually supplies the new script.

Profiles define virtual-keyboard presentation and key text. They do not implement
prediction dictionaries, composition/conversion engines, external keyboard
translation, a keyboard-language settings screen, or language-switching UI.

The raw IME and dialog share character limits, character restrictions, password
masking, and clipboard policy. Raw callbacks own text snapshots and report
selection ranges; programmatic text replacement invalidates older pending edits.
Panel queries report the displayed position after controller movement.

Keyboard reception tracks its owning user, reports host keyboard resources and
key events, and applies external-keyboard filters on the guest update thread.
Host layout translation stays on the SDL thread. Disabling controller input
returns it to the application; disabling external-keyboard input prevents it
from editing the IME. Composition engines and language-specific keyboard mode
conversion remain separate from this host input path.

Queued keyboard input belongs to the reception client that captured it and is
not delivered to a later client after close/reopen. Pending keyboard events are
bounded; the owning update call reports `EVENT_OVERFLOW` if events were lost.
The application must resynchronize its input state after that error.
