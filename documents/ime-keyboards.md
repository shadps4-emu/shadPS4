<!--
SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# IME keyboard languages

Both the embedded IME and the IME dialog use the profiles registered in
`src/core/libraries/ime/ime_kb_language.cpp`. The registry contains 35 layout
variants across 31 input languages:

- Arabic, Czech, Danish, Dutch, English (US/UK), Finnish, French (France/Canada),
  German, Greek, Hungarian, Indonesian, Italian, Norwegian, Polish,
  Portuguese (Portugal/Brazil), Romanian, Russian, Spanish (Spain/Latin America),
  Swedish, Thai, Turkish, Ukrainian and Vietnamese.
- Japanese with QWERTY and Gojuon variants, Korean, simplified Chinese, and
  traditional Chinese with Hong Kong, Taiwan, Zhuyin and Cangjie variants.

`ime_kb_data.cpp` stores shared character records and mode-specific page maps.
Layouts retain their paired text, blank cells and spanning keys. Combining-mark labels include a dotted circle; inserting the key
adds only its actual character. Both IME interfaces use the same page builder.

`OrbisImeType::Number` selects the shared `Numeric` family in both IME interfaces.
It uses the PS4 three-column keypad with digits, `.`, `,`, `-`, backspace and a
two-column Done key. The numeric grid and close-only top panel take precedence
over language profiles and alphabet-panel priority. Enter labels still
come from the selected language profile. Alphabet and symbol shortcuts are
disabled while the numeric keypad is active.

The initial selection prefers the configured console language when its profile
is allowed by the application's supported-language mask. Otherwise it selects an
allowed registered profile. A zero mask permits the configured profile. Requests
with no implemented matching profile retain the shared English fallback.
The keyboard-change control appears when the application enables multiple
languages. It cycles through allowed language profiles and their variants.
The `Input.ime_keyboard_layout` setting can select a named variant of the active
language, such as `Japanese (Gojuon)`, `Chinese (Zhuyin)` or `Chinese (Cangjie)`.
An empty or unrecognized name uses the language's default variant. The preference
does not enable a language excluded by the application.

Default, Basic Latin, URL, email and number input select separate page maps.
Internal dialog modes also have separate social-name and password page maps;
they remain subject to the internal entry points' validation rules. Basic Latin,
email and URL input restrict insertion to printable ASCII.

The caller's options control Tab, line break, symbol and alphabet switching,
Space, fixed Shift state and pointer controls. Tab shares the backspace area
when enabled. Full-width pages are available for the corresponding layouts.
The Enter label follows the selected language and the caller's Done, Send,
Search or Go choice.
Held selector navigation pauses for half a second before wrapping across a grid
edge. Releasing stops movement; a fresh direction press clears the pending pause.
The dialog's large-text accessibility scene uses a centered keyboard with larger
keys. Speech output is unavailable, so its reserved control area is inactive.

To integrate another keyboard language:

1. Add an `ImeKbLanguageProfile` entry with its existing `OrbisImeLanguage` bit and
   console-language ID. These are separate identifiers, not interchangeable values.
2. Define a complete `ImeKbLabels` table in `ime_kb_language.cpp` and point the
   profile's `labels` to it. Keep English and translated function-key, Enter
   (Default, Send, Search, Go), and edit-menu labels together in that source file.
   US and UK profiles share the English table; a null `labels` pointer also uses
   English. Set preferred characters on the profile when they need emphasis.
3. Add the layout's character records and mode/page mapping to `ime_kb_data.cpp`.
   Equivalent pages and character records are shared. `Alphabet` selects the
   main script; `Alternate` selects accents or the Latin counterpart. The common
   builder adds function keys from the selected labels and caller options.
4. Verify lowercase, Shift, Caps Lock, symbols, accents, controller navigation,
   function keys, and both IME interfaces against console output. Font collection
   includes all character records and translated controls. Confirm that the font
   stack supplies the new script.

Profiles define virtual-keyboard presentation and key text. Japanese Gojuon
controls edit small and voiced kana and cycle punctuation through the normal
bounded editing path. Prediction dictionaries, native composition/conversion
engines, speech output and a keyboard-language settings screen remain separate
features. Layout coverage alone does not establish native conversion support.
Arabic button labels use shaped visual forms; the editable text field still
shares the renderer's limitations for bidirectional text and complex shaping.

The raw IME and dialog share character limits, character restrictions, password
masking, and clipboard policy. Basic Latin, URL and Mail fields accept printable
ASCII; Default fields restrict input to the enabled languages. An omitted
language mask uses the languages available to the application's version. These
restrictions apply to typed, pasted and on-screen input; application text
replacement preserves the separate API contract.

Japanese character membership uses the public
[CP932 mapping](https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT)
and its user-defined character range. The mapping's license is included in
`LICENSES/Unicode-3.0.txt`.

Committed Chinese and Japanese text from the host input system can be inserted
into an eligible field. Pinyin, Zhuyin, Cangjie and kana-to-kanji conversion on
the virtual keyboard require a conversion backend and candidate UI, which are
not implemented. Candidate APIs report `NOT_OPENED` when closed and
`INVALID_PARAM` while open without candidates.

Raw callbacks own text snapshots and report
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
