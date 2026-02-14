// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <fmt/core.h>
#include <fmt/xchar.h> // for wstring support
#include <toml.hpp>

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/formatter.h"
#include "common/path_util.h"
#include "common/scm_rev.h"

#include "input/input_handler.h"

using std::nullopt;
using std::optional;
using std::string;

namespace toml {
template <typename TC, typename K>
std::filesystem::path find_fs_path_or(const basic_value<TC>& v, const K& ky,
                                      std::filesystem::path opt) {
    try {
        auto str = find<string>(v, ky);
        if (str.empty()) {
            return opt;
        }
        std::u8string u8str{(char8_t*)&str.front(), (char8_t*)&str.back() + 1};
        return std::filesystem::path{u8str};
    } catch (...) {
        return opt;
    }
}

// why is it so hard to avoid exceptions with this library
template <typename T>
std::optional<T> get_optional(const toml::value& v, const std::string& key) {
    if (!v.is_table())
        return std::nullopt;
    const auto& tbl = v.as_table();
    auto it = tbl.find(key);
    if (it == tbl.end())
        return std::nullopt;

    if constexpr (std::is_same_v<T, int>) {
        if (it->second.is_integer()) {
            return static_cast<s32>(toml::get<int>(it->second));
        }
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        if (it->second.is_integer()) {
            return static_cast<u32>(toml::get<unsigned int>(it->second));
        }
    } else if constexpr (std::is_same_v<T, double>) {
        if (it->second.is_floating()) {
            return toml::get<T>(it->second);
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (it->second.is_string()) {
            return toml::get<T>(it->second);
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        if (it->second.is_boolean()) {
            return toml::get<T>(it->second);
        }
    } else if constexpr (std::is_same_v<T, std::array<string, 4>>) {
        if (it->second.is_array()) {
            return toml::get<T>(it->second);
        }
    } else {
        static_assert([] { return false; }(), "Unsupported type in get_optional<T>");
    }

    return std::nullopt;
}

} // namespace toml

namespace Config {

ConfigMode config_mode = ConfigMode::Default;

void setConfigMode(ConfigMode mode) {
    config_mode = mode;
}

template <typename T>
class ConfigEntry {
public:
    const T default_value;
    T base_value;
    optional<T> game_specific_value;
    ConfigEntry(const T& t = T()) : default_value(t), base_value(t), game_specific_value(nullopt) {}
    ConfigEntry operator=(const T& t) {
        base_value = t;
        return *this;
    }
    const T get() const {
        switch (config_mode) {
        case ConfigMode::Default:
            return game_specific_value.value_or(base_value);
        case ConfigMode::Global:
            return base_value;
        case ConfigMode::Clean:
            return default_value;
        default:
            UNREACHABLE();
        }
    }
    void setFromToml(const toml::value& v, const std::string& key, bool is_game_specific = false) {
        if (is_game_specific) {
            game_specific_value = toml::get_optional<T>(v, key);
        } else {
            base_value = toml::get_optional<T>(v, key).value_or(base_value);
        }
    }
    void set(const T value, bool is_game_specific = false) {
        is_game_specific ? game_specific_value = value : base_value = value;
    }
    void setDefault(bool is_game_specific = false) {
        is_game_specific ? game_specific_value = default_value : base_value = default_value;
    }
    void setTomlValue(toml::ordered_value& data, const std::string& header, const std::string& key,
                      bool is_game_specific = false) {
        if (is_game_specific) {
            data[header][key] = game_specific_value.value_or(base_value);
            game_specific_value = std::nullopt;
        } else {
            data[header][key] = base_value;
        }
    }
    // operator T() {
    //     return get();
    // }
};

// General
static ConfigEntry<std::array<std::string, 4>> userNames({
    "shadPS4",
    "shadps4-2",
    "shadPS4-3",
    "shadPS4-4",
});

static ConfigEntry<bool> useUnifiedInputConfig(true);

// Keys
static string trophyKey = "";

// Config version, used to determine if a user's config file is outdated.
static string config_version = Common::g_scm_rev;

// These entries aren't stored in the config
static bool overrideControllerColor = false;
static int controllerCustomColorRGB[3] = {0, 0, 255};

std::filesystem::path getFontsPath() {
    if (fonts_path.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::FontsDir);
    }
    return fonts_path;
}

void setFontsPath(const std::filesystem::path& path) {
    fonts_path = path;
}

bool GetUseUnifiedInputConfig() {
    return useUnifiedInputConfig.get();
}

void SetUseUnifiedInputConfig(bool use) {
    useUnifiedInputConfig.base_value = use;
}

bool GetOverrideControllerColor() {
    return overrideControllerColor;
}

void SetOverrideControllerColor(bool enable) {
    overrideControllerColor = enable;
}

int* GetControllerCustomColor() {
    return controllerCustomColorRGB;
}

void SetControllerCustomColor(int r, int b, int g) {
    controllerCustomColorRGB[0] = r;
    controllerCustomColorRGB[1] = b;
    controllerCustomColorRGB[2] = g;
}

string getTrophyKey() {
    return trophyKey;
}

void setTrophyKey(string key) {
    trophyKey = key;
}

void setUserName(int id, string name) {
    auto temp = userNames.get();
    temp[id] = name;
    userNames.set(temp);
}

std::array<string, 4> const getUserNames() {
    return userNames.get();
}

std::string getUserName(int id) {
    return userNames.get()[id];
}

void load(const std::filesystem::path& path, bool is_game_specific) {
    // If the configuration file does not exist, create it and return, unless it is game specific
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        if (!is_game_specific) {
            save(path);
        }
        return;
    }

    toml::value data;

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifs.open(path, std::ios_base::binary);
        data = toml::parse(ifs, string{fmt::UTF(path.filename().u8string()).data});
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    if (data.contains("General")) {
        const toml::value& general = data.at("General");
        userNames.setFromToml(general, "userNames", is_game_specific);
        fonts_path = toml::find_fs_path_or(general, "fontsPath", fonts_path);
    }

    if (data.contains("Input")) {
        const toml::value& input = data.at("Input");
        useUnifiedInputConfig.setFromToml(input, "useUnifiedInputConfig", is_game_specific);
    }

    string current_version = {};
    if (data.contains("Debug")) {
        const toml::value& debug = data.at("Debug");
        current_version = toml::find_or<std::string>(debug, "ConfigVersion", current_version);
    }

    if (data.contains("Keys")) {
        const toml::value& keys = data.at("Keys");
        trophyKey = toml::find_or<string>(keys, "TrophyKey", trophyKey);
    }

    // Run save after loading to generate any missing fields with default values.
    if (config_version != current_version && !is_game_specific) {
        save(path);
    }
}

void sortTomlSections(toml::ordered_value& data) {
    toml::ordered_value ordered_data;
    std::vector<string> section_order = {"General", "Input", "Audio", "GPU",     "Vulkan",
                                         "Debug",   "Keys",  "GUI",   "Settings"};

    for (const auto& section : section_order) {
        if (data.contains(section)) {
            std::vector<string> keys;
            for (const auto& item : data.at(section).as_table()) {
                keys.push_back(item.first);
            }

            std::sort(keys.begin(), keys.end(), [](const string& a, const string& b) {
                return std::lexicographical_compare(
                    a.begin(), a.end(), b.begin(), b.end(), [](char a_char, char b_char) {
                        return std::tolower(a_char) < std::tolower(b_char);
                    });
            });

            toml::ordered_value ordered_section;
            for (const auto& key : keys) {
                ordered_section[key] = data.at(section).at(key);
            }

            ordered_data[section] = ordered_section;
        }
    }

    data = ordered_data;
}

void save(const std::filesystem::path& path, bool is_game_specific) {
    toml::ordered_value data;

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        try {
            std::ifstream ifs;
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifs.open(path, std::ios_base::binary);
            data = toml::parse<toml::ordered_type_config>(
                ifs, string{fmt::UTF(path.filename().u8string()).data});
        } catch (const std::exception& ex) {
            fmt::print("Exception trying to parse config file. Exception: {}\n", ex.what());
            return;
        }
    } else {
        if (error) {
            fmt::print("Filesystem error: {}\n", error.message());
        }
        fmt::print("Saving new configuration file {}\n", fmt::UTF(path.u8string()));
    }
    // Entries saved by the game-specific settings GUI
    userNames.setTomlValue(data, "General", "userNames", is_game_specific);

    if (!is_game_specific) {
        // Non game-specific entries
        data["General"]["fontsPath"] = string{fmt::UTF(fonts_path.u8string()).data};

        // Do not save these entries in the game-specific dialog since they are not in the GUI
        data["Input"]["useUnifiedInputConfig"] = useUnifiedInputConfig.base_value;
    }

    // Sorting of TOML sections
    sortTomlSections(data);

    std::ofstream file(path, std::ios::binary);
    file << data;
    file.close();
}

void setDefaultValues(bool is_game_specific) {

    // Entries with game-specific settings that are in the game-specific setings GUI but not in
    // the global settings GUI

    // Entries with game-specific settings that are in both the game-specific and global GUI
    // GS - General
    userNames.setDefault(is_game_specific);

    // All other entries
    if (!is_game_specific) {
        // Input
        useUnifiedInputConfig.base_value = true;
        controllerCustomColorRGB[0] = 0;
        controllerCustomColorRGB[1] = 0;
        controllerCustomColorRGB[2] = 255;
    }
}

constexpr std::string_view GetDefaultGlobalConfig() {
    return R"(# Anything put here will be loaded for all games,
# alongside the game's config or default.ini depending on your preference.
)";
}

constexpr std::string_view GetDefaultInputConfig() {
    return R"(#Feeling lost? Check out the Help section!

# Keyboard bindings

triangle = kp8
circle = kp6
cross = kp2
square = kp4
# Alternatives for users without a keypad
triangle = c
circle = b
cross = n
square = v

l1 = q
r1 = u
l2 = e
r2 = o
l3 = x
r3 = m

options = enter
touchpad_center = space

pad_up = up
pad_down = down
pad_left = left
pad_right = right

axis_left_x_minus = a
axis_left_x_plus = d
axis_left_y_minus = w
axis_left_y_plus = s

axis_right_x_minus = j
axis_right_x_plus = l
axis_right_y_minus = i
axis_right_y_plus = k

# Controller bindings

triangle = triangle
cross = cross
square = square
circle = circle

l1 = l1
l2 = l2
l3 = l3
r1 = r1
r2 = r2
r3 = r3

options = options
touchpad_center = back

pad_up = pad_up
pad_down = pad_down
pad_left = pad_left
pad_right = pad_right

axis_left_x = axis_left_x
axis_left_y = axis_left_y
axis_right_x = axis_right_x
axis_right_y = axis_right_y

# Range of deadzones: 1 (almost none) to 127 (max)
analog_deadzone = leftjoystick, 2, 127
analog_deadzone = rightjoystick, 2, 127

override_controller_color = false, 0, 0, 255
)";
}
std::filesystem::path GetInputConfigFile(const string& game_id) {
    // Read configuration file of the game, and if it doesn't exist, generate it from default
    // If that doesn't exist either, generate that from getDefaultConfig() and try again
    // If even the folder is missing, we start with that.

    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "input_config";
    const auto config_file = config_dir / (game_id + ".ini");
    const auto default_config_file = config_dir / "default.ini";

    // Ensure the config directory exists
    if (!std::filesystem::exists(config_dir)) {
        std::filesystem::create_directories(config_dir);
    }

    // Check if the default config exists
    if (!std::filesystem::exists(default_config_file)) {
        // If the default config is also missing, create it from getDefaultConfig()
        const auto default_config = GetDefaultInputConfig();
        std::ofstream default_config_stream(default_config_file);
        if (default_config_stream) {
            default_config_stream << default_config;
        }
    }

    // if empty, we only need to execute the function up until this point
    if (game_id.empty()) {
        return default_config_file;
    }

    // Create global config if it doesn't exist yet
    if (game_id == "global" && !std::filesystem::exists(config_file)) {
        if (!std::filesystem::exists(config_file)) {
            const auto global_config = GetDefaultGlobalConfig();
            std::ofstream global_config_stream(config_file);
            if (global_config_stream) {
                global_config_stream << global_config;
            }
        }
    }
    if (game_id == "global") {
        std::map<string, string> default_bindings_to_add = {
            {"hotkey_renderdoc_capture", "f12"},
            {"hotkey_fullscreen", "f11"},
            {"hotkey_show_fps", "f10"},
            {"hotkey_pause", "f9"},
            {"hotkey_reload_inputs", "f8"},
            {"hotkey_toggle_mouse_to_joystick", "f7"},
            {"hotkey_toggle_mouse_to_gyro", "f6"},
            {"hotkey_add_virtual_user", "f5"},
            {"hotkey_remove_virtual_user", "f4"},
            {"hotkey_toggle_mouse_to_touchpad", "delete"},
            {"hotkey_quit", "lctrl, lshift, end"},
            {"hotkey_volume_up", "kpplus"},
            {"hotkey_volume_down", "kpminus"},
        };
        std::ifstream global_in(config_file);
        string line;
        while (std::getline(global_in, line)) {
            line.erase(std::remove_if(line.begin(), line.end(),
                                      [](unsigned char c) { return std::isspace(c); }),
                       line.end());
            std::size_t equal_pos = line.find('=');
            if (equal_pos == std::string::npos) {
                continue;
            }
            std::string output_string = line.substr(0, equal_pos);
            default_bindings_to_add.erase(output_string);
        }
        global_in.close();
        std::ofstream global_out(config_file, std::ios::app);
        for (auto const& b : default_bindings_to_add) {
            global_out << b.first << " = " << b.second << "\n";
        }
    }

    // If game-specific config doesn't exist, create it from the default config
    if (!std::filesystem::exists(config_file)) {
        std::filesystem::copy(default_config_file, config_file);
    }
    return config_file;
}

} // namespace Config
