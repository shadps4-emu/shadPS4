// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_composition.h"
#include "core/libraries/ime/ime_ui_shared.h"

#include <algorithm>
#include <array>
#include <utility>
#include <imgui_internal.h>

namespace Libraries::Ime {
namespace {

std::u16string Utf16(std::string_view text) {
    std::u16string result;
    for (const char* p = text.data(); p < text.data() + text.size();) {
        unsigned int character = 0;
        const auto bytes = ImTextCharFromUtf8(&character, p, text.data() + text.size());
        if (bytes <= 0 || character == 0) {
            break;
        }
        p += bytes;
        if (character > 0xffff) {
            result.push_back(static_cast<char16_t>(0xd800 + ((character - 0x10000) >> 10)));
            result.push_back(static_cast<char16_t>(0xdc00 + ((character - 0x10000) & 1023)));
        } else {
            result.push_back(static_cast<char16_t>(character));
        }
    }
    return result;
}

std::string Utf8(std::u16string_view text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned int character = text[i];
        if (character >= 0xd800 && character <= 0xdbff && i + 1 < text.size() &&
            text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff) {
            character = 0x10000 + ((character - 0xd800) << 10) + text[++i] - 0xdc00;
        }
        char buffer[5]{};
        const auto count = ImTextCharToUtf8(buffer, character);
        result.append(buffer, count);
    }
    return result;
}

bool IsJapanese(ImeKbVariant variant) {
    return variant == ImeKbVariant::Japanese || variant == ImeKbVariant::Japanese_Gojuon;
}

std::u16string Romaji(std::u16string_view source, bool finish) {
    constexpr std::pair<std::u16string_view, std::u16string_view> syllables[]{
        {u"kya", u"きゃ"}, {u"kyu", u"きゅ"}, {u"kyo", u"きょ"}, {u"gya", u"ぎゃ"},
        {u"gyu", u"ぎゅ"}, {u"gyo", u"ぎょ"}, {u"sha", u"しゃ"}, {u"shu", u"しゅ"},
        {u"sho", u"しょ"}, {u"she", u"しぇ"}, {u"sya", u"しゃ"}, {u"syu", u"しゅ"},
        {u"syo", u"しょ"}, {u"cha", u"ちゃ"}, {u"chu", u"ちゅ"}, {u"cho", u"ちょ"},
        {u"che", u"ちぇ"}, {u"tya", u"ちゃ"}, {u"tyu", u"ちゅ"}, {u"tyo", u"ちょ"},
        {u"cya", u"ちゃ"}, {u"cyu", u"ちゅ"}, {u"cyo", u"ちょ"}, {u"nya", u"にゃ"},
        {u"nyu", u"にゅ"}, {u"nyo", u"にょ"}, {u"hya", u"ひゃ"}, {u"hyu", u"ひゅ"},
        {u"hyo", u"ひょ"}, {u"bya", u"びゃ"}, {u"byu", u"びゅ"}, {u"byo", u"びょ"},
        {u"pya", u"ぴゃ"}, {u"pyu", u"ぴゅ"}, {u"pyo", u"ぴょ"}, {u"mya", u"みゃ"},
        {u"myu", u"みゅ"}, {u"myo", u"みょ"}, {u"rya", u"りゃ"}, {u"ryu", u"りゅ"},
        {u"ryo", u"りょ"}, {u"jya", u"じゃ"}, {u"jyu", u"じゅ"}, {u"jyo", u"じょ"},
        {u"ja", u"じゃ"},  {u"ju", u"じゅ"},  {u"jo", u"じょ"},  {u"je", u"じぇ"},
        {u"zya", u"じゃ"}, {u"zyu", u"じゅ"}, {u"zyo", u"じょ"}, {u"dya", u"ぢゃ"},
        {u"dyu", u"ぢゅ"}, {u"dyo", u"ぢょ"}, {u"shi", u"し"},   {u"chi", u"ち"},
        {u"tsu", u"つ"},   {u"tsa", u"つぁ"}, {u"tsi", u"つぃ"}, {u"tse", u"つぇ"},
        {u"tso", u"つぉ"}, {u"xtsu", u"っ"},  {u"ltsu", u"っ"},  {u"xtu", u"っ"},
        {u"ltu", u"っ"},   {u"ka", u"か"},    {u"ki", u"き"},    {u"ku", u"く"},
        {u"ke", u"け"},    {u"ko", u"こ"},    {u"ga", u"が"},    {u"gi", u"ぎ"},
        {u"gu", u"ぐ"},    {u"ge", u"げ"},    {u"go", u"ご"},    {u"sa", u"さ"},
        {u"si", u"し"},    {u"su", u"す"},    {u"se", u"せ"},    {u"so", u"そ"},
        {u"za", u"ざ"},    {u"zi", u"じ"},    {u"zu", u"ず"},    {u"ze", u"ぜ"},
        {u"zo", u"ぞ"},    {u"ta", u"た"},    {u"ti", u"ち"},    {u"tu", u"つ"},
        {u"te", u"て"},    {u"to", u"と"},    {u"da", u"だ"},    {u"di", u"ぢ"},
        {u"du", u"づ"},    {u"de", u"で"},    {u"do", u"ど"},    {u"na", u"な"},
        {u"ni", u"に"},    {u"nu", u"ぬ"},    {u"ne", u"ね"},    {u"no", u"の"},
        {u"ha", u"は"},    {u"hi", u"ひ"},    {u"hu", u"ふ"},    {u"he", u"へ"},
        {u"ho", u"ほ"},    {u"ba", u"ば"},    {u"bi", u"び"},    {u"bu", u"ぶ"},
        {u"be", u"べ"},    {u"bo", u"ぼ"},    {u"pa", u"ぱ"},    {u"pi", u"ぴ"},
        {u"pu", u"ぷ"},    {u"pe", u"ぺ"},    {u"po", u"ぽ"},    {u"ma", u"ま"},
        {u"mi", u"み"},    {u"mu", u"む"},    {u"me", u"め"},    {u"mo", u"も"},
        {u"ra", u"ら"},    {u"ri", u"り"},    {u"ru", u"る"},    {u"re", u"れ"},
        {u"ro", u"ろ"},    {u"ya", u"や"},    {u"yu", u"ゆ"},    {u"yo", u"よ"},
        {u"wa", u"わ"},    {u"wi", u"うぃ"},  {u"we", u"うぇ"},  {u"wo", u"を"},
        {u"fa", u"ふぁ"},  {u"fi", u"ふぃ"},  {u"fu", u"ふ"},    {u"fe", u"ふぇ"},
        {u"fo", u"ふぉ"},  {u"va", u"ゔぁ"},  {u"vi", u"ゔぃ"},  {u"vu", u"ゔ"},
        {u"ve", u"ゔぇ"},  {u"vo", u"ゔぉ"},  {u"xa", u"ぁ"},    {u"xi", u"ぃ"},
        {u"xu", u"ぅ"},    {u"xe", u"ぇ"},    {u"xo", u"ぉ"},    {u"la", u"ぁ"},
        {u"li", u"ぃ"},    {u"lu", u"ぅ"},    {u"le", u"ぇ"},    {u"lo", u"ぉ"},
        {u"xya", u"ゃ"},   {u"xyu", u"ゅ"},   {u"xyo", u"ょ"},   {u"lya", u"ゃ"},
        {u"lyu", u"ゅ"},   {u"lyo", u"ょ"},   {u"xwa", u"ゎ"},   {u"lwa", u"ゎ"},
        {u"ji", u"じ"},    {u"a", u"あ"},     {u"i", u"い"},     {u"u", u"う"},
        {u"e", u"え"},     {u"o", u"お"},
    };
    std::u16string result;
    for (std::size_t i = 0; i < source.size();) {
        const auto character = source[i];
        if (character == u'-') {
            result += u'ー';
            ++i;
            continue;
        }
        if (character == u'n' && i + 1 < source.size() && source[i + 1] == u'n') {
            result += u'ん';
            i += i + 2 < source.size() && std::u16string_view(u"aiueoy").find(source[i + 2]) !=
                                              std::u16string_view::npos
                     ? 1
                     : 2;
            continue;
        }
        if (character == u'n' &&
            ((i + 1 == source.size() && finish) ||
             (i + 1 < source.size() &&
              std::u16string_view(u"aiueoy").find(source[i + 1]) == std::u16string_view::npos))) {
            result += u'ん';
            i += i + 1 < source.size() && source[i + 1] == u'\'' ? 2 : 1;
            continue;
        }
        if (i + 1 < source.size() && character == source[i + 1] && character >= u'a' &&
            character <= u'z' &&
            std::u16string_view(u"aiueon").find(character) == std::u16string_view::npos) {
            result += u'っ';
            ++i;
            continue;
        }
        std::u16string_view match;
        std::u16string_view converted;
        for (const auto& [key, text] : syllables) {
            if (key.size() > match.size() && source.substr(i).starts_with(key)) {
                match = key;
                converted = text;
            }
        }
        if (!match.empty()) {
            result += converted;
            i += match.size();
        } else {
            result += character;
            ++i;
        }
    }
    return result;
}

std::u16string Hangul(std::u16string_view source) {
    constexpr std::u16string_view initials = u"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";
    constexpr std::u16string_view vowels = u"ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ";
    constexpr std::u16string_view finals =
        u" ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ";
    constexpr std::pair<std::u16string_view, char16_t> pairs[]{
        {u"ㅗㅏ", u'ㅘ'}, {u"ㅗㅐ", u'ㅙ'}, {u"ㅗㅣ", u'ㅚ'}, {u"ㅜㅓ", u'ㅝ'}, {u"ㅜㅔ", u'ㅞ'},
        {u"ㅜㅣ", u'ㅟ'}, {u"ㅡㅣ", u'ㅢ'}, {u"ㄱㅅ", u'ㄳ'}, {u"ㄴㅈ", u'ㄵ'}, {u"ㄴㅎ", u'ㄶ'},
        {u"ㄹㄱ", u'ㄺ'}, {u"ㄹㅁ", u'ㄻ'}, {u"ㄹㅂ", u'ㄼ'}, {u"ㄹㅅ", u'ㄽ'}, {u"ㄹㅌ", u'ㄾ'},
        {u"ㄹㅍ", u'ㄿ'}, {u"ㄹㅎ", u'ㅀ'}, {u"ㅂㅅ", u'ㅄ'},
    };
    std::u16string result;
    for (std::size_t i = 0; i < source.size();) {
        const auto initial = initials.find(source[i]);
        if (initial == std::u16string_view::npos || i + 1 == source.size() ||
            vowels.find(source[i + 1]) == std::u16string_view::npos) {
            bool combined = false;
            for (const auto& [pair, character] : pairs) {
                if (i + 1 < source.size() && source.substr(i, 2) == pair &&
                    (vowels.find(character) != std::u16string_view::npos ||
                     i + 2 == source.size() ||
                     vowels.find(source[i + 2]) == std::u16string_view::npos)) {
                    result += character;
                    i += 2;
                    combined = true;
                    break;
                }
            }
            if (!combined) {
                result += source[i++];
            }
            continue;
        }
        auto vowel = source[++i];
        ++i;
        for (const auto& [pair, combined] : pairs) {
            if (i < source.size() && pair[0] == vowel && pair[1] == source[i] &&
                vowels.find(combined) != std::u16string_view::npos) {
                vowel = combined;
                ++i;
                break;
            }
        }
        std::size_t final = 0;
        if (i < source.size() && finals.find(source[i]) != std::u16string_view::npos &&
            !(i + 1 < source.size() && vowels.find(source[i + 1]) != std::u16string_view::npos)) {
            final = finals.find(source[i++]);
            for (const auto& [pair, combined] : pairs) {
                if (i < source.size() && pair[0] == finals[final] && pair[1] == source[i] &&
                    finals.find(combined) != std::u16string_view::npos &&
                    !(i + 1 < source.size() &&
                      vowels.find(source[i + 1]) != std::u16string_view::npos)) {
                    final = finals.find(combined);
                    ++i;
                    break;
                }
            }
        }
        result += static_cast<char16_t>(0xac00 + (initial * 21 + vowels.find(vowel)) * 28 + final);
    }
    return result;
}

} // namespace

std::u16string ImeKeyboardKanaText(std::u16string_view input, bool katakana, bool half_width_mode) {
    std::u16string text(input);
    if (katakana) {
        for (auto& character : text) {
            if (character >= u'ぁ' && character <= u'ゖ') {
                character += 0x60;
            }
        }
    }
    if (!half_width_mode) {
        return text;
    }
    constexpr std::u16string_view half_width =
        u"。「」、・ヲァィゥェォャュョッーアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフ"
        u"ヘホマミムメモヤユヨラリルレロワン゛゜";
    std::u16string result;
    for (auto character : text) {
        char16_t mark = 0;
        if ((character >= u'ガ' && character <= u'ヂ' && ((character - u'ガ') % 2) == 0) ||
            (character >= u'ヅ' && character <= u'ド' && ((character - u'ヅ') % 2) == 0)) {
            --character;
            mark = 0xff9e;
        } else if (character >= u'ハ' && character <= u'ポ') {
            const auto accent = (character - u'ハ') % 3;
            character -= accent;
            mark = accent ? 0xff9d + accent : 0;
        } else if (character == u'ヴ') {
            character = u'ウ';
            mark = 0xff9e;
        }
        const auto index = half_width.find(character);
        result.push_back(index == std::u16string_view::npos ? character : 0xff61 + index);
        if (mark) {
            result.push_back(mark);
        }
    }
    return result;
}

char16_t UpdateImeGreekDeadKey(char16_t previous, char16_t character) {
    if (character != u'΄' && character != u'¨') {
        return 0;
    }
    return previous && previous != character ? u'΅' : character;
}

std::u16string CombineImeKeyboardCharacter(char16_t previous, char16_t character,
                                           ImeKbVariant variant) {
    const auto original = character;
    if (variant == ImeKbVariant::Korean) {
        constexpr std::u16string_view initials = u"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";
        constexpr std::u16string_view vowels = u"ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ";
        std::u16string source;
        if (previous >= 0xac00 && previous <= 0xd7a3) {
            const auto syllable = previous - 0xac00;
            source += initials[syllable / (21 * 28)];
            source += vowels[syllable / 28 % 21];
            const auto final = syllable % 28;
            constexpr std::array<std::u16string_view, 28> final_parts{
                u"",     u"ㄱ",   u"ㄲ",   u"ㄱㅅ", u"ㄴ",   u"ㄴㅈ", u"ㄴㅎ",
                u"ㄷ",   u"ㄹ",   u"ㄹㄱ", u"ㄹㅁ", u"ㄹㅂ", u"ㄹㅅ", u"ㄹㅌ",
                u"ㄹㅍ", u"ㄹㅎ", u"ㅁ",   u"ㅂ",   u"ㅂㅅ", u"ㅅ",   u"ㅆ",
                u"ㅇ",   u"ㅈ",   u"ㅊ",   u"ㅋ",   u"ㅌ",   u"ㅍ",   u"ㅎ"};
            source += final_parts[final];
        } else if (initials.contains(previous) || vowels.contains(previous)) {
            source += previous;
        } else {
            constexpr std::u16string_view compounds = u"ㄳㄵㄶㄺㄻㄼㄽㄾㄿㅀㅄ";
            constexpr std::array<std::u16string_view, 11> parts{u"ㄱㅅ", u"ㄴㅈ", u"ㄴㅎ", u"ㄹㄱ",
                                                                u"ㄹㅁ", u"ㄹㅂ", u"ㄹㅅ", u"ㄹㅌ",
                                                                u"ㄹㅍ", u"ㄹㅎ", u"ㅂㅅ"};
            const auto index = compounds.find(previous);
            if (index == std::u16string_view::npos) {
                return std::u16string(1, character);
            }
            source += parts[index];
        }
        source += character;
        return Hangul(source);
    }
    if (variant == ImeKbVariant::Greek && std::u16string_view(u"΄¨΅").contains(previous)) {
        if (const auto dead_key = UpdateImeGreekDeadKey(previous, character)) {
            return std::u16string(1, dead_key);
        }
    }
    if (previous == u'΄' || previous == u'¨' || previous == u'΅') {
        const auto mark = previous == u'΄' ? u'\u0301' : u'\u0308';
        auto result = CombineImeKeyboardCharacter(character, mark, variant);
        if (result.front() != mark) {
            if (previous == u'΅') {
                auto accented = CombineImeKeyboardCharacter(result.back(), u'\u0301', variant);
                if (accented.front() != u'\u0301') {
                    result.pop_back();
                }
                result += accented;
            }
            return result;
        }
    }
    if (character == u'΄') {
        character = u'\u0301';
    } else if (character == u'¨') {
        character = u'\u0308';
    }
    if (character == u'゛') {
        character = u'\u3099';
    } else if (character == u'゜') {
        character = u'\u309a';
    }
    struct Combination {
        char16_t mark;
        std::u16string_view bases;
        std::u16string_view combined;
    };
    static constexpr Combination combinations[]{
        {u'\u0300', u"AEIOUaeiouÜüNnЕИеиĒēŌōWwÂâĂăÊêÔôƠơƯưYy",
         u"ÀÈÌÒÙàèìòùǛǜǸǹЀЍѐѝḔḕṐṑẀẁẦầẰằỀềỒồỜờỪừỲỳ"},
        {u'\u0301',
         u"AEIOUYaeiouyCcLlNnRrSsZzÜüGgÅåÆæØø¨ΑΕΗΙΟΥΩϊαεηιϋουωϒГКгкÇçĒēÏïKkMmÕõŌōPpŨũWwÂâĂăÊêÔôƠơƯ"
         u"ư",
         u"ÁÉÍÓÚÝáéíóúýĆćĹĺŃńŔŕŚśŹźǗǘǴǵǺǻǼǽǾǿ΅ΆΈΉΊΌΎΏΐάέήίΰόύώϓЃЌѓќḈḉḖḗḮḯḰḱḾḿṌṍṒṓṔṕṸṹẂẃẤấẮắẾếỐốỚớỨ"
         u"ứ"},
        {u'\u0302', u"AEIOUaeiouCcGgHhJjSsWwYyZzẠạẸẹỌọ", u"ÂÊÎÔÛâêîôûĈĉĜĝĤĥĴĵŜŝŴŵŶŷẐẑẬậỆệỘộ"},
        {u'\u0303', u"ANOanoIiUuVvÂâĂăEeÊêÔôƠơƯưYy", u"ÃÑÕãñõĨĩŨũṼṽẪẫẴẵẼẽỄễỖỗỠỡỮữỸỹ"},
        {u'\u0304', u"AaEeIiOoUuÜüÄäȦȧÆæǪǫÖöÕõȮȯYyИиУуGgḶḷṚṛ",
         u"ĀāĒēĪīŌōŪūǕǖǞǟǠǡǢǣǬǭȪȫȬȭȰȱȲȳӢӣӮӯḠḡḸḹṜṝ"},
        {u'\u0306', u"AaEeGgIiOoUuУИиуЖжАаЕеȨȩẠạ", u"ĂăĔĕĞğĬĭŎŏŬŭЎЙйўӁӂӐӑӖӗḜḝẶặ"},
        {u'\u0307', u"CcEeGgIZzAaOoBbDdFfHhMmNnPpRrSsŚśŠšṢṣTtWwXxYyſ",
         u"ĊċĖėĠġİŻżȦȧȮȯḂḃḊḋḞḟḢḣṀṁṄṅṖṗṘṙṠṡṤṥṦṧṨṩṪṫẆẇẊẋẎẏẛ"},
        {u'\u0308', u"AEIOUaeiouyYΙΥιυϒЕІеіАаӘәЖжЗзИиОоӨөЭэУуЧчЫыHhÕõŪūWwXxt",
         u"ÄËÏÖÜäëïöüÿŸΪΫϊϋϔЁЇёїӒӓӚӛӜӝӞӟӤӥӦӧӪӫӬӭӰӱӴӵӸӹḦḧṎṏṺṻẄẅẌẍẗ"},
        {u'\u0309', u"AaÂâĂăEeÊêIiOoÔôƠơUuƯưYy", u"ẢảẨẩẲẳẺẻỂểỈỉỎỏỔổỞởỦủỬửỶỷ"},
        {u'\u030a', u"AaUuwy", u"ÅåŮůẘẙ"},
        {u'\u030b', u"OoUuУу", u"ŐőŰűӲӳ"},
        {u'\u030c', u"CcDdEeLlNnRrSsTtZzAaIiOoUuÜüGgKkƷʒjHh",
         u"ČčĎďĚěĽľŇňŘřŠšŤťŽžǍǎǏǐǑǒǓǔǙǚǦǧǨǩǮǯǰȞȟ"},
        {u'\u030f', u"AaEeIiOoRrUuѴѵ", u"ȀȁȄȅȈȉȌȍȐȑȔȕѶѷ"},
        {u'\u0311', u"AaEeIiOoRrUu", u"ȂȃȆȇȊȋȎȏȒȓȖȗ"},
        {u'\u031b', u"OoUu", u"ƠơƯư"},
        {u'\u0323', u"BbDdHhKkLlMmNnRrSsTtVvWwZzAaEeIiOoƠơUuƯưYy",
         u"ḄḅḌḍḤḥḲḳḶḷṂṃṆṇṚṛṢṣṬṭṾṿẈẉẒẓẠạẸẹỊịỌọỢợỤụỰựỴỵ"},
        {u'\u0324', u"Uu", u"Ṳṳ"},
        {u'\u0325', u"Aa", u"Ḁḁ"},
        {u'\u0326', u"SsTt", u"ȘșȚț"},
        {u'\u0327', u"CcGgKkLlNnRrSsTtEeDdHh", u"ÇçĢģĶķĻļŅņŖŗŞşŢţȨȩḐḑḨḩ"},
        {u'\u0328', u"AaEeIiUuOo", u"ĄąĘęĮįŲųǪǫ"},
        {u'\u032d', u"DdEeLlNnTtUu", u"ḒḓḘḙḼḽṊṋṰṱṶṷ"},
        {u'\u032e', u"Hh", u"Ḫḫ"},
        {u'\u0330', u"EeIiUu", u"ḚḛḬḭṴṵ"},
        {u'\u0331', u"BbDdKkLlNnRrTtZzh", u"ḆḇḎḏḴḵḺḻṈṉṞṟṮṯẔẕẖ"},
        {u'\u3099',
         u"かきくけこさしすせそたちつてとはひふへほうゝカキクケコサシスセソタチツテトハヒフヘホウワ"
         u"ヰヱヲヽ",
         u"がぎぐげござじずぜぞだぢづでどばびぶべぼゔゞガギグゲゴザジズゼゾダヂヅデドバビブベボヴヷ"
         u"ヸヹヺヾ"},
        {u'\u309a', u"はひふへほハヒフヘホ", u"ぱぴぷぺぽパピプペポ"},
    };
    for (const auto& row : combinations) {
        if (row.mark == character) {
            const auto index = row.bases.find(previous);
            if (index != std::u16string_view::npos) {
                return std::u16string(1, row.combined[index]);
            }
        }
    }
    return std::u16string(1, original);
}

u32 ResolveImeExternalKeyboardMode(ImeKbVariant variant, u32 mode) {
    if (IsJapanese(variant)) {
        return mode & 0x1c000000U;
    }
    switch (variant) {
    case ImeKbVariant::Chinese_CN:
    case ImeKbVariant::Chinese_TW:
    case ImeKbVariant::Chinese_HK:
    case ImeKbVariant::Chinese_TW_Zhuyin:
    case ImeKbVariant::Chinese_TW_Cangjie:
        return mode & 0x14000000U;
    case ImeKbVariant::Arabic:
        return mode & 0x04000003U;
    case ImeKbVariant::Russian:
    case ImeKbVariant::Ukrainian:
    case ImeKbVariant::Korean:
    case ImeKbVariant::Greek:
    case ImeKbVariant::Thai:
        return mode & 0x04000000U;
    default:
        return 0;
    }
}

std::string TransformImeExternalText(std::string_view input, ImeKbVariant variant, u32 mode,
                                     bool map_native) {
    auto text = Utf16(input);
    const bool native = (mode & 0x04000000U) != 0;
    const bool full_width = (mode & 0x10000000U) != 0;
    constexpr std::u16string_view alphabet = u"1234567890qwertyuiopasdfghjklzxcvbnm[];',./\\`-=";
    constexpr std::u16string_view shifted_alphabet =
        u"!@#$%^&*()QWERTYUIOPASDFGHJKLZXCVBNM{}:\"<>?|~_+";
    std::u16string_view native_keys;
    std::u16string_view shifted_keys;
    switch (variant) {
    case ImeKbVariant::Chinese_TW_Cangjie:
        native_keys = u"1234567890手田水口廿卜山戈人心日尸木火土竹十大中重難金女月弓一[];',./\\`-=";
        shifted_keys = native_keys;
        break;
    case ImeKbVariant::Japanese_Gojuon:
        native_keys = u"ぬふあうえおやゆよわたていすかんなにらせちとしはきくまのりつさそひこみも゛"
                      u"゜れけねるめむろほへ";
        shifted_keys = u"ぬふぁぅぇぉゃゅょをたてぃすかんなにらせちとしはきくまのりっさそひこみも゛"
                       u"゜れけ、。・むろーへ";
        break;
    case ImeKbVariant::Russian:
        native_keys = u"1234567890йцукенгшщзфывапролдячсмитьхъжэбю.\\ё-=";
        shifted_keys = u"!\"№;%:?*()ЙЦУКЕНГШЩЗФЫВАПРОЛДЯЧСМИТЬХЪЖЭБЮ,/Ё_+";
        break;
    case ImeKbVariant::Ukrainian:
        native_keys = u"1234567890йцукенгшщзфівапролдячсмитьхїжєбю.\\ґ-=";
        shifted_keys = u"!\"№;%:?*()ЙЦУКЕНГШЩЗФІВАПРОЛДЯЧСМИТЬХЇЖЄБЮ,/Ґ_+";
        break;
    case ImeKbVariant::Greek:
        native_keys = u"1234567890;ςερτυθιοπασδφγηξκλζχψωβνμ[]΄',./\\`-=";
        shifted_keys = u"!@#$%^&*():ΣΕΡΤΥΘΙΟΠΑΣΔΦΓΗΞΚΛΖΧΨΩΒΝΜ{}¨\"<>?|~_+";
        break;
    case ImeKbVariant::Korean:
        native_keys = u"1234567890ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ[];',./\\`-=";
        shifted_keys =
            u"!@#$%^&*()ㅃㅉㄸㄲㅆㅛㅕㅑㅒㅖㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ{}:\"<>?|~_+";
        break;
    case ImeKbVariant::Chinese_TW_Zhuyin:
        native_keys = u"ㄅㄉˇˋㄓˊ˙ㄚㄞㄢㄆㄊㄍㄐㄔㄗㄧㄛㄟㄣㄇㄋㄎㄑㄕㄘㄨㄜㄠㄈㄌㄏㄒㄖㄙㄩ[]ㄤ'"
                      u"ㄝㄡㄥ\\`ㄦ=";
        shifted_keys = native_keys;
        break;
    case ImeKbVariant::Arabic:
        native_keys = u"1234567890ضصثقفغعهخحشسيبلاتنمئءؤرﻻىةجدكطوزظ\\ذ-=";
        shifted_keys = u"!@#$%^&*()ًٌَُﻹإ‘÷×؛ٍِ][ﻷأـ،/~ْ}{ﻵآ’}{:\"><؟|ّ_+";
        break;
    case ImeKbVariant::Thai:
        native_keys = u"ๅ/-ภถุึคตจๆไำพะัีรนยฟหกดเ้่าสผปแอิืทบลวงมใฝฃ_ขช";
        shifted_keys = u"+๑๒๓๔ู฿๕๖๗๐\"ฎฑธํ๊ณฯญฤฆฏโฌ็๋ษศ()ฉฮฺ์?ฐ,ซ.ฒฬฦฅ%๘๙";
        break;
    default:
        break;
    }
    if (native && map_native && native_keys.size() == alphabet.size() &&
        shifted_keys.size() == alphabet.size()) {
        for (auto& character : text) {
            if (const auto index = alphabet.find(character); index != std::u16string_view::npos) {
                character = native_keys[index];
            } else if (const auto shifted = shifted_alphabet.find(character);
                       shifted != std::u16string_view::npos) {
                character = shifted_keys[shifted];
            }
        }
    }
    for (auto& character : text) {
        if (variant == ImeKbVariant::Arabic && map_native) {
            if (!(mode & 1U) && character >= u'0' && character <= u'9') {
                character += 0x0660 - u'0';
            } else if ((mode & 1U) && character >= u'٠' && character <= u'٩') {
                character -= 0x0660 - u'0';
            }
            if (!(mode & 2U) && character >= 0x200c && character <= 0x200f) {
                character = 0;
            }
        } else if (full_width &&
                   (IsJapanese(variant) || variant == ImeKbVariant::Chinese_CN ||
                    variant == ImeKbVariant::Chinese_TW || variant == ImeKbVariant::Chinese_HK ||
                    variant == ImeKbVariant::Chinese_TW_Zhuyin ||
                    variant == ImeKbVariant::Chinese_TW_Cangjie)) {
            if (character >= 0x21 && character <= 0x7e) {
                character += 0xfee0;
            } else if (character == u' ') {
                character = 0x3000;
            }
        }
    }
    std::erase(text, char16_t{});
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] >= 0xfef5 && text[i] <= 0xfefc) {
            const char16_t alef = std::u16string_view(u"آأإا")[(text[i] - 0xfef5) / 2];
            text[i] = u'ل';
            text.insert(text.begin() + ++i, alef);
        }
    }
    return Utf8(text);
}

std::string ComposeImeExternalText(std::string_view input, ImeKbVariant variant, u32 mode,
                                   char16_t& dead_key) {
    if (variant != ImeKbVariant::Greek || !(mode & 0x04000000U)) {
        dead_key = 0;
        return std::string(input);
    }
    auto text = Utf16(input);
    if (text.size() != 1) {
        return std::string(input);
    }
    const auto character = text.front();
    if (const auto next_dead_key = UpdateImeGreekDeadKey(dead_key, character)) {
        dead_key = next_dead_key;
        return {};
    }
    if (!dead_key) {
        return std::string(input);
    }
    const auto accent = std::exchange(dead_key, char16_t{});
    if (character == u' ') {
        return Utf8(std::u16string(1, accent));
    }
    const std::u16string_view letters = accent == u'΄' ? u"αεηιουωΑΕΗΙΟΥΩ" : u"ιυΙΥ";
    const auto index = letters.find(character);
    if (index != std::u16string_view::npos) {
        const auto combined = (accent == u'΄'   ? std::u16string_view(u"άέήίόύώΆΈΉΊΌΎΏ")
                               : accent == u'¨' ? std::u16string_view(u"ϊϋΪΫ")
                                                : std::u16string_view(u"ΐΰΪΫ"))[index];
        text.assign(1, combined);
        if (accent == u'΅' && index >= 2) {
            text.push_back(0x0301);
        }
    } else {
        text.insert(text.begin(), accent);
    }
    return Utf8(text);
}

bool ImeComposition::AcceptsExternal(std::string_view input, u32 mode) const {
    const bool native = (mode & 0x04000000U) != 0;
    const bool native2 = (mode & 0x08000000U) != 0;
    return (native || (IsJapanese(variant) && native2)) && Accepts(input);
}

std::u16string ImeComposition::Render(bool final) const {
    if (!IsJapanese(variant)) {
        return Korean() ? Hangul(source) : source;
    }
    if (external_mode && !(*external_mode & 0x04000000U)) {
        return Utf16(TransformImeExternalText(Utf8(source), variant, *external_mode));
    }
    auto text = Romaji(source, final);
    if (!external_mode) {
        return text;
    }
    if (*external_mode & 0x08000000U) {
        for (auto& character : text) {
            if (character >= u'ぁ' && character <= u'ゖ') {
                character += 0x60;
            }
        }
    }
    if (*external_mode & 0x10000000U) {
        return Utf16(TransformImeExternalText(Utf8(text), variant, *external_mode, false));
    }
    if (!(*external_mode & 0x08000000U)) {
        return text;
    }
    return ImeKeyboardKanaText(text, true, true);
}

void ImeComposition::Configure(ImeKbVariant input_variant, const std::filesystem::path& directory,
                               bool expanded_preedit,
                               std::shared_ptr<const ImeDictionary> application_dictionary,
                               bool application_priority) {
    auto input_dictionary = ImeDictionary::Load(directory, input_variant);
    const int preedit_length = expanded_preedit && IsJapanese(input_variant)
                                   ? ORBIS_IME_MAX_EXPANDED_PREEDIT_LENGTH
                                   : ORBIS_IME_MAX_PREEDIT_LENGTH;
    if (variant == input_variant && dictionary == input_dictionary &&
        max_preedit_length == preedit_length &&
        this->application_dictionary == application_dictionary &&
        this->application_priority == application_priority) {
        return;
    }
    Clear();
    variant = input_variant;
    dictionary = std::move(input_dictionary);
    max_preedit_length = preedit_length;
    this->application_dictionary = std::move(application_dictionary);
    this->application_priority = application_priority;
}

bool ImeComposition::Accepts(std::string_view input) const {
    if (!Enabled() || input.empty()) {
        return false;
    }
    for (const auto character : Utf16(input)) {
        if (variant == ImeKbVariant::Chinese_TW_Cangjie) {
            if (std::u16string_view(u"日月金木水火土竹戈十大中一弓人心手口尸廿山女田難卜")
                        .find(character) != std::u16string_view::npos ||
                (character >= u'a' && character <= u'z')) {
                continue;
            }
        } else if (variant == ImeKbVariant::Chinese_TW_Zhuyin) {
            if ((character >= u'ㄅ' && character <= u'ㄩ') ||
                std::u16string_view(u"ˊˇˋ˙").find(character) != std::u16string_view::npos) {
                continue;
            }
        } else if (Korean()) {
            if (character >= u'ㄱ' && character <= u'ㅣ') {
                continue;
            }
        } else {
            if ((character >= u'a' && character <= u'z') ||
                (character >= u'A' && character <= u'Z') || character == u'\'' ||
                (IsJapanese(variant) && ((character >= u'ぁ' && character <= u'ヿ') ||
                                         character == u'-' || character == u'ー'))) {
                continue;
            }
        }
        return false;
    }
    return true;
}

bool ImeComposition::Insert(std::string_view document, int first, int last, std::string_view input,
                            int limit, std::optional<u32> external_mode) {
    if (!Accepts(input)) {
        return false;
    }
    if (Active() && (first != Caret() || last != first || this->external_mode != external_mode)) {
        auto replacement = *this;
        replacement.Clear();
        if (!replacement.Insert(document, first, last, input, limit, external_mode)) {
            return false;
        }
        *this = std::move(replacement);
        return true;
    }
    const bool starting = !Active();
    if (starting) {
        this->external_mode = external_mode;
        const auto text = Utf16(document);
        first = std::clamp(first, 0, static_cast<int>(text.size()));
        last = std::clamp(last, first, static_cast<int>(text.size()));
        available = limit - static_cast<int>(text.size()) + last - first;
        if (available <= 0) {
            return false;
        }
        committed = document;
        prefix = text.substr(0, first);
        suffix = text.substr(last);
        start = first;
        composing = true;
    }
    auto insertion = Utf16(input);
    for (auto& character : insertion) {
        if (character >= u'A' && character <= u'Z' &&
            (!external_mode || (*external_mode & 0x04000000U))) {
            character += u'a' - u'A';
        }
        if (variant == ImeKbVariant::Chinese_TW_Cangjie && character >= u'a' && character <= u'z') {
            character = u"日月金木水火土竹戈十大中一弓人心手口尸廿山女田難卜重"[character - u'a'];
        }
    }
    const auto previous_size = source.size();
    source += insertion;
    if (!Rebuild()) {
        source.resize(previous_size);
        if (starting) {
            Clear();
        }
        return false;
    }
    return true;
}

bool ImeComposition::Rebuild() {
    auto text = Render(false);
    if (text.size() > static_cast<std::size_t>(max_preedit_length)) {
        return false;
    }
    selected = -1;
    auto key = IsJapanese(variant) && (!external_mode || (*external_mode & 0x04000000U))
                   ? Romaji(source, true)
                   : text;
    if (IsJapanese(variant)) {
        for (auto& character : key) {
            if (character >= u'ァ' && character <= u'ヶ') {
                character -= 0x60;
            }
        }
    }
    if (variant == ImeKbVariant::Chinese_TW_Zhuyin) {
        std::erase_if(key, [](char16_t c) { return std::u16string_view(u"ˊˇˋ˙").contains(c); });
    } else if (!IsJapanese(variant)) {
        std::erase(key, u'\'');
    }
    reading = Utf8(text);
    display = reading;
    candidates.clear();
    const auto append_candidates = [&](const auto& source_dictionary) {
        if (!source_dictionary || candidates.size() >= ORBIS_IME_MAX_CANDIDATE_LIST_SIZE) {
            return;
        }
        for (const auto& word : source_dictionary->Find(key)) {
            if (word.size() > static_cast<std::size_t>(available) ||
                word.size() > ORBIS_IME_MAX_CANDIDATE_WORD_LENGTH ||
                std::ranges::any_of(word, [](char16_t character) {
                    return character < 0x20 || (character >= 0x7f && character < 0xa0) ||
                           (character >= 0xd800 && character <= 0xdfff);
                })) {
                continue;
            }
            auto candidate = Utf8(word);
            if (std::ranges::find(candidates, candidate) == candidates.end()) {
                candidates.push_back(std::move(candidate));
                if (candidates.size() == ORBIS_IME_MAX_CANDIDATE_LIST_SIZE) {
                    break;
                }
            }
        }
    };
    append_candidates(application_priority ? application_dictionary : dictionary);
    append_candidates(application_priority ? dictionary : application_dictionary);
    return true;
}

bool ImeComposition::Backspace() {
    if (!Active() || source.empty()) {
        return false;
    }
    const auto removed = source.back();
    source.pop_back();
    if (!Rebuild()) {
        source.push_back(removed);
        return false;
    }
    return true;
}

bool ImeComposition::Select(int index) {
    if (index < 0 || index >= static_cast<int>(candidates.size())) {
        return false;
    }
    selected = index;
    display = candidates[index];
    return true;
}

bool ImeComposition::Cycle(int direction) {
    if (candidates.empty()) {
        return false;
    }
    return Select(selected < 0 ? 0
                               : (selected + direction + static_cast<int>(candidates.size())) %
                                     static_cast<int>(candidates.size()));
}

bool ImeComposition::Rewrite(std::string_view text) {
    if (!Active() || !Accepts(text)) {
        return false;
    }
    auto previous = std::exchange(source, Utf16(text));
    if (!Rebuild()) {
        source = std::move(previous);
        return false;
    }
    return true;
}

std::string ImeComposition::Confirm() {
    auto result = selected >= 0 ? Utf16(display) : Render(true);
    if (result.size() > static_cast<std::size_t>(available)) {
        result.resize(std::max(0, available));
        if (!result.empty() && result.back() >= 0xd800 && result.back() <= 0xdbff) {
            result.pop_back();
        }
    }
    auto text = Utf8(prefix + result + suffix);
    const int caret = start + static_cast<int>(result.size());
    Clear();
    confirmed_caret = caret;
    return text;
}

std::string ImeComposition::Cancel() {
    auto text = committed;
    const int caret = start;
    Clear();
    confirmed_caret = caret;
    return text;
}

void ImeComposition::Clear() {
    source.clear();
    prefix.clear();
    suffix.clear();
    committed.clear();
    reading.clear();
    display.clear();
    candidates.clear();
    selected = -1;
    start = 0;
    available = 0;
    composing = false;
    external_mode.reset();
}

bool ImeComposition::SetExternalMode(u32 mode) {
    if (!Active() || external_mode == mode) {
        return false;
    }
    const auto previous = std::exchange(external_mode, mode);
    if (!Rebuild()) {
        external_mode = previous;
        return false;
    }
    return true;
}

ImeKbVariant ImeComposition::Variant() const {
    return variant;
}

std::optional<u32> ImeComposition::ExternalMode() const {
    return external_mode;
}

bool ImeComposition::Active() const {
    return composing;
}

bool ImeComposition::Enabled() const {
    return Korean() || (dictionary && !dictionary->Empty());
}

bool ImeComposition::Korean() const {
    return variant == ImeKbVariant::Korean;
}

int ImeComposition::Start() const {
    return start;
}

int ImeComposition::Length() const {
    return Utf16CountFromUtf8Range(display.data(), display.data() + display.size());
}

int ImeComposition::Caret() const {
    return composing ? start + Length() : confirmed_caret;
}

int ImeComposition::Selected() const {
    return selected;
}

const std::string& ImeComposition::Reading() const {
    return reading;
}

const std::vector<std::string>& ImeComposition::Candidates() const {
    return candidates;
}

std::string ImeComposition::Preview() const {
    return Utf8(prefix) + display + Utf8(suffix);
}

const std::string& ImeComposition::CommittedText() const {
    return committed;
}

} // namespace Libraries::Ime
