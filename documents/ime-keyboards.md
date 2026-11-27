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

Settings > Input > Text Input (IME) groups the text-entry preferences. Direct
Input is off by default (`Input.ime_direct_input`). When enabled, a PC keyboard
can edit the text field. Raw IME shows this additional emulator field only when
Direct Input is available; otherwise the application displays its own text.
IME Dialog retains its normal text display without PC-style editing. Games can
disable external-keyboard editing. The direct-typing fallback for an
unimplemented keyboard layout remains available.

Select languages opens a separate picker with a compact summary on the settings
page. The configuration keeps only selected language IDs in
`Input.ime_input_languages`; an empty array uses English and the console language.
Individual users can override this list in User Manager.

`ime_kb_data.cpp` stores shared character records and mode-specific page maps.
Layouts retain their paired text, blank cells and spanning keys. Combining-mark labels include a dotted circle; inserting the key
adds only its actual character. Both IME interfaces use the same page builder.

`OrbisImeType::Number` selects the shared `Numeric` family in both IME interfaces.
It uses the PS4 three-column keypad with digits, `.`, `,`, `-`, backspace and a
two-column Done key. The numeric grid and close-only top panel take precedence
over language profiles and alphabet-panel priority. Enter labels still
come from the selected language profile. Alphabet and symbol shortcuts are
disabled while the numeric keypad is active.

Enabled input keyboards are configured separately from the console display
language, including individual Japanese and Chinese variants. The launcher
provides a global checklist under Settings > Input, and per-user overrides in
the User Manager context menu. Empty settings use English (US) plus the console
language; this is an emulator default, not an imported console configuration.

An application's supported-language mask restricts the enabled keyboards. If
there is no overlap, the application mask takes precedence. `LANGUAGES_FORCED`
uses the application's languages regardless of user language preferences;
selected input methods within an enabled language are retained. A zero mask
permits the API-supported languages before applying these rules.

The initial keyboard restores that user's last-used layout when available,
then falls back within the permitted keyboards. With no saved selection, the
console language and `Input.ime_keyboard_layout` preference determine the
initial choice. The keyboard-change control appears when multiple keyboard
variants are available. It shrinks the keyboard into a horizontal carousel
with neighboring previews and a language title. D-pad or left-stick navigation
moves between choices in console-language order; Cross confirms and Circle
cancels. Preview casing follows capitalization and the caller's panel priority. The
picker stops at each end without wrapping. Previewing does not edit the text or
save a preference. Confirmed selections are saved separately for each user.

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
   function keys, and both IME interfaces against console output. Confirm that
   the font stack supplies the new script and translated controls.

Profiles define virtual-keyboard presentation and key text. Japanese Gojuon
controls edit small and voiced kana and cycle punctuation through the normal
bounded editing path. Conversion uses locally installed dictionaries as
described below. Speech output remains unavailable.
Arabic button labels use shaped visual forms; the editable text field still
shares the renderer's limitations for bidirectional text and complex shaping.

The console display language and text-input keyboards are separate settings.
In the Qt launcher, Settings > Input > Text Input Languages selects the enabled
keyboards. Automatic mode uses English (US) plus the console language as an
emulator default; a custom list is saved globally as `Input.ime_input_languages`
using the console-language IDs. It does not override the application's language
request. The initial keyboard prefers the console language when it is allowed.

For ordinary requests, available keyboards are the intersection of the configured
list and the application's `supported_languages`. If there is no overlap, the
application's requested languages take priority. An omitted application mask uses
the configured list, subject to the application's API version. With
`LANGUAGES_FORCED`, the application's list is used directly; a zero mask makes all
API-supported languages available. The switch control appears only when the
effective list contains multiple languages. Dictionary-dependent layouts still
require the user's dumped dictionaries. Exact last-used-keyboard/resume ordering
and separate per-user keyboard lists are not implemented.

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

Japanese romaji/kana, simplified and traditional Chinese Pinyin, Hong Kong
Jyutping, Zhuyin and Cangjie use users' own dumped IME dictionaries. In the Qt
launcher, open Settings > Paths > PS4 IME Dictionaries Path and select the
console dump's `preinst/priv/ime/dic` folder. The folder is read directly,
including its language subdirectories, just like the configured fonts folder.
The path is saved as `General.ime_dictionary_dir` in `config.json`; the default
is `user/ime/dic`. The configured root is also available to guest code at
`/priv/ime/dic` as a read-only mount. Restart the emulator after replacing
dictionary files.
Dictionary data is not embedded in or distributed with the emulator. If the
selected layout's dictionaries are missing, unreadable or empty, the keyboard
shows a notification and opens in English (US). Switching languages skips
layouts whose dictionaries are unavailable and keeps the current layout if none
are available. Korean syllable assembly works without dictionaries.

Typing in a conversion layout starts pre-edit text. The application buffer keeps
the previously confirmed text while the work buffer and raw IME events describe
the preview and its pre-edit range. Space cycles candidates, Enter confirms the
current preview, and a subsequent Enter completes the field. Backspace removes
the last input component; cancel restores the text preceding composition.
Candidates can also be selected with the mouse or the controller's prediction
row; the mouse wheel or horizontal navigation reaches further candidates.
Raw candidate events are enabled by `DISABLE_LIST_FOR_EXT_KEYBOARD`; the native
candidate row is hidden during external-keyboard input in that mode. Candidate
APIs act on the delivered list and reject stale or unavailable selections.
Application text replacement cancels pre-edit and discards older queued events.

Dictionary candidates support whole-reading lookup and prefix suggestions.
Sentence segmentation, adaptive learning and user dictionaries remain
unimplemented; candidate ordering is not guaranteed to match the console.
When no explicit initial keyboard mode is requested, committed Chinese and
Japanese text from the host input system can also enter an eligible field. Candidate APIs report `NOT_OPENED` when closed
and `INVALID_PARAM` for an unavailable candidate. After confirmation or
cancellation, candidate APIs report `INVALID_TIMING` until another list is sent.
Pre-edit supports 30 UTF-16 units, or 120 for Japanese when the application
requests expanded pre-edit. Lists contain up to 100 candidates. Pre-edit can
exceed the committed-text limit without overwriting the confirmed buffer.
Applications can provide an additional dictionary through a mounted guest path.
The loader validates the file size, checksum, record bounds and UTF-16 text.
Its terms feed the existing conversion layouts and are combined with system
candidates without duplicates; the priority
option places application terms first. The additional dictionary is loaded for
the current session and does not replace the user's dumped system dictionaries.
Latin-language predictive typing remains unavailable.

Applications can request initial external-keyboard input modes independently of
the on-screen keyboard. Supported modes include native-language input,
Japanese unconfirmed alphabet/hiragana/katakana, full-width Japanese and Chinese
text, and Arabic numeral and format-character options. Numeric-keypad digits
remain ASCII. Mode-specific options for other languages are ignored. Explicit initial modes use physical key events after application filtering,
so multi-character keys and repeats do not depend on host text composition.
Ordinary direct input retains the host keyboard layout.

When a connected external keyboard has precedence and the application requests
a hidden key panel, the grid and its navigation targets are removed. The text
and candidate fields use the compact panel dimensions. Raw IME still accepts
keyboard input without displaying the optional PC editor. Accessibility panels
retain their fixed layout and ignore this hide option.

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

External-keyboard filters receive the current pre-edit, candidate and native
method state. Accepted native, script and width changes persist for subsequent
keys. Input-method gestures and keyboard-reception output modes have separate
state. Reception supports automatic/manual native switching, partial or composed
characters, kana variants, Arabic numeral and format-character options, and
replacement/continuous character events. The physical layout currently defaults
to the console-language profile; there is no separate physical-layout setting.

With both OSK reception options enabled, virtual character and editing keys enter
the same bounded, client-owned queue as physical keys. Events identify OSK origin;
shortcuts retain their origin flag. Editing at a different position finishes the
previous pre-edit before inserting or replacing text. Korean Space confirms the
current syllable and inserts the delimiter within the field's text limit.

Managed physical input keeps its layout and method state independently of the
virtual keyboard selection. Each input event retains that context through the
UI queue, including mode-only changes, so composition does not depend on how
characters are grouped into frames. Text belonging to a closed IME session is
not applied to a later session. Physical composition may use a different language
from the displayed virtual keyboard; both still obey field type, text limits
and the existing dumped-dictionary requirements.
