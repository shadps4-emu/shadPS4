// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_data.h"

#include <map>
#include <mutex>
#include <numeric>
#include <tuple>
#include <vector>

#include "common/assert.h"

namespace Libraries::Ime {
namespace {

struct KeyboardLayout {
    std::vector<ImeKbKeySpec> keys;
    ImeKbLayoutModel model;
};

ImeKbPage ResolvePage(const ImeKbLayoutSelection& selection, ImeKbVariant variant) {
    const bool shifted = selection.case_state != ImeKbCaseState::Lower;
    ImeKbPage page = shifted ? ImeKbPage::Shift : ImeKbPage::Default;
    if (selection.family == ImeKbLayoutFamily::Symbols) {
        page = selection.page == 0 ? ImeKbPage::Symbols1 : ImeKbPage::Symbols2;
        if (selection.full_width &&
            GetImeKeyboardPageData(variant, selection.input_type, ImeKbPage::FullWidthSymbols)) {
            page = ImeKbPage::FullWidthSymbols;
        }
    } else if (selection.family == ImeKbLayoutFamily::Alternate) {
        page = shifted ? ImeKbPage::AccentShift : ImeKbPage::Accent;
        if (!GetImeKeyboardPageData(variant, selection.input_type, page)) {
            page = shifted ? ImeKbPage::AlphabetShift : ImeKbPage::Alphabet;
        }
    } else if (selection.full_width) {
        page = shifted ? ImeKbPage::FullWidthShift : ImeKbPage::FullWidth;
    }
    if (!GetImeKeyboardPageData(variant, selection.input_type, page)) {
        if (page == ImeKbPage::AlphabetShift) {
            page = ImeKbPage::Alphabet;
        } else if (page == ImeKbPage::Symbols2) {
            page = ImeKbPage::Symbols1;
        } else {
            page = ImeKbPage::Default;
        }
    }
    return page;
}

KeyboardLayout MakeLayout(const ImeKbLayoutSelection& selection, ImeKbVariant variant,
                          const ImeKbPageData& page) {
    KeyboardLayout result;
    const auto& labels = GetImeKeyboardLanguageProfile(selection.language, variant).GetLabels();
    const bool numeric = selection.input_type == OrbisImeType::Number;
    const int columns = numeric ? 3 : std::lcm(10, static_cast<int>(page.cols));
    const int scale = columns / page.cols;
    result.keys.reserve(page.keys.size() + 18);
    for (std::size_t index = 0; index < page.keys.size(); ++index) {
        const auto& character = GetImeKeyboardCharacter(page.keys[index]);
        const bool more = character.action == ImeKbKeyAction::PageNext;
        const auto cell = index + (page.keys.size() == 39 && index >= 29 ? 1 : 0);
        ImeKbKeySpec key{
            .row = static_cast<u8>(more ? 2 : cell / page.cols),
            .col = static_cast<u8>((more ? 9 : cell % page.cols) * scale),
            .col_span = static_cast<u8>(scale),
            .row_span = static_cast<u8>(more ? 2 : 1),
            .label = character.label[0] ? character.label : nullptr,
            .action = character.action,
            .text = character.text,
        };
        if (key.action == ImeKbKeyAction::PageNext) {
            key.glyph = ImeKbKeyGlyph::ArrowRight;
        } else if (key.action == ImeKbKeyAction::JapaneseVoicing) {
            key.label = labels.script_action_labels[0];
        } else if (key.action == ImeKbKeyAction::JapaneseSmall) {
            key.label = labels.script_action_labels[1];
        } else if (key.action == ImeKbKeyAction::JapanesePunctuation) {
            key.label = labels.script_action_labels[2];
        }
        result.keys.push_back(key);
    }

    const auto add = [&](int row, int column, int span, ImeKbKeyAction action,
                         ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None, const char* label = nullptr,
                         const char* hotkey = nullptr) {
        result.keys.push_back({
            .row = static_cast<u8>(row),
            .col = static_cast<u8>(column),
            .col_span = static_cast<u8>(span),
            .label = action == ImeKbKeyAction::None ? nullptr : label,
            .hotkey_label = action == ImeKbKeyAction::None ? nullptr : hotkey,
            .action = action,
            .glyph = action == ImeKbKeyAction::None ? ImeKbKeyGlyph::None : glyph,
        });
    };
    const bool tab = (selection.options & InternalOption::EnableTab) != 0;
    if (numeric) {
        add(4, 2, 1, tab ? ImeKbKeyAction::Tab : ImeKbKeyAction::None, ImeKbKeyGlyph::Tab);
        add(5, 0, 1, ImeKbKeyAction::Backspace, ImeKbKeyGlyph::Backspace, nullptr, "▢");
        add(5, 1, 2, ImeKbKeyAction::Done, ImeKbKeyGlyph::None, nullptr, "R2");
    } else {
        const auto control = [&](int row, int col, int span, ImeKbKeyAction action,
                                 ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None,
                                 const char* label = nullptr, const char* hotkey = nullptr) {
            add(page.rows + row, col * columns / 10, span * columns / 10, action, glyph, label,
                hotkey);
        };
        const bool fixed = True(selection.ext_options & OrbisImeExtOption::PRIORITY_FIXED_PANEL);
        control(0, 0, 1, fixed ? ImeKbKeyAction::None : ImeKbKeyAction::Shift,
                ImeKbKeyGlyph::ShiftOutline, nullptr, "L2");
        control(0, 1, 1,
                (selection.options & InternalOption::DisableSymbols) ? ImeKbKeyAction::None
                                                                     : ImeKbKeyAction::SymbolsMode,
                ImeKbKeyGlyph::None, nullptr, "L2+△");
        const bool accents =
            GetImeKeyboardPageData(variant, selection.input_type, ImeKbPage::Accent);
        const bool alphabet =
            GetImeKeyboardPageData(variant, selection.input_type, ImeKbPage::Alphabet);
        const bool allow_alphabet = selection.input_type != OrbisImeType::FacebookPassword &&
                                    (selection.options & InternalOption::DisableAlphabet) == 0;
        control(0, 2, 1,
                accents || (alphabet && allow_alphabet) ? ImeKbKeyAction::AlternateMode
                                                        : ImeKbKeyAction::None,
                ImeKbKeyGlyph::None, nullptr, "L3");
        control(0, 3, 4,
                (selection.options & InternalOption::DisableSpace) ? ImeKbKeyAction::None
                                                                   : ImeKbKeyAction::Space,
                ImeKbKeyGlyph::None, nullptr, "△");
        const bool width =
            selection.input_type == OrbisImeType::Default &&
            (GetImeKeyboardPageData(variant, selection.input_type, ImeKbPage::FullWidth) ||
             GetImeKeyboardPageData(variant, selection.input_type, ImeKbPage::FullWidthSymbols));
        control(0, 7, 1, width ? ImeKbKeyAction::FullWidth : ImeKbKeyAction::None,
                ImeKbKeyGlyph::None, labels.width_labels[selection.full_width ? 1 : 0]);
        if (tab) {
            control(0, 8, 1, ImeKbKeyAction::Tab, ImeKbKeyGlyph::Tab);
        }
        control(0, tab ? 9 : 8, tab ? 1 : 2, ImeKbKeyAction::Backspace, ImeKbKeyGlyph::Backspace,
                nullptr, "▢");
        control(1, 0, 1, ImeKbKeyAction::ArrowDown, ImeKbKeyGlyph::ArrowDown);
        control(1, 1, 1, ImeKbKeyAction::ArrowUp, ImeKbKeyGlyph::ArrowUp);
        control(1, 2, 1, ImeKbKeyAction::ArrowLeft, ImeKbKeyGlyph::ArrowLeft, nullptr, "L1");
        control(1, 3, 1, ImeKbKeyAction::ArrowRight, ImeKbKeyGlyph::ArrowRight, nullptr, "R1");
        if (selection.accessibility) {
            control(1, 4, 3, ImeKbKeyAction::None);
        } else {
            control(1, 4, 1,
                    selection.multiple_languages ? ImeKbKeyAction::Keyboard : ImeKbKeyAction::None,
                    ImeKbKeyGlyph::Keyboard);
            control(1, 5, 1, ImeKbKeyAction::Menu, ImeKbKeyGlyph::Options);
            control(1, 6, 1,
                    True(selection.ext_options & OrbisImeExtOption::DISABLE_POINTER)
                        ? ImeKbKeyAction::None
                        : ImeKbKeyAction::Settings,
                    ImeKbKeyGlyph::Gyro, nullptr, "R3");
        }
        control(1, 7, 1, selection.multiline ? ImeKbKeyAction::NewLine : ImeKbKeyAction::None,
                ImeKbKeyGlyph::Enter);
        control(1, 8, 2, ImeKbKeyAction::Done, ImeKbKeyGlyph::None, nullptr, "R2");
    }
    result.model = {nullptr, result.keys.size(), static_cast<u8>(columns),
                    static_cast<u8>(numeric ? 6 : page.rows + 2), static_cast<u8>(numeric ? 1 : 2)};
    return result;
}

} // namespace

const ImeKbLayoutModel& GetImeKeyboardLayout(const ImeKbLayoutSelection& selection) {
    if (selection.family == ImeKbLayoutFamily::Numeric &&
        selection.input_type != OrbisImeType::Number) {
        auto numeric = selection;
        numeric.input_type = OrbisImeType::Number;
        return GetImeKeyboardLayout(numeric);
    }
    const auto& profile = GetImeKeyboardLanguageProfile(selection.language, selection.variant);
    const auto page_id = ResolvePage(selection, profile.variant);
    const auto* page = GetImeKeyboardPageData(profile.variant, selection.input_type, page_id);
    ASSERT(page);
    using CacheKey =
        std::tuple<ImeKbVariant, OrbisImeType, ImeKbPage, bool, bool, bool, bool, u32, u32>;
    const CacheKey key{
        profile.variant,
        selection.input_type,
        page_id,
        selection.multiline,
        selection.multiple_languages,
        selection.full_width,
        selection.accessibility,
        selection.options & (InternalOption::EnableTab | InternalOption::DisableAlphabet |
                             InternalOption::DisableSymbols | InternalOption::DisableSpace),
        static_cast<u32>(selection.ext_options & (OrbisImeExtOption::PRIORITY_FIXED_PANEL |
                                                  OrbisImeExtOption::DISABLE_POINTER))};
    static std::mutex mutex;
    static std::map<CacheKey, KeyboardLayout> layouts;
    const std::scoped_lock lock(mutex);
    const auto [entry, inserted] = layouts.try_emplace(key);
    if (inserted) {
        entry->second = MakeLayout(selection, profile.variant, *page);
        entry->second.model.keys = entry->second.keys.data();
    }
    return entry->second.model;
}

} // namespace Libraries::Ime
