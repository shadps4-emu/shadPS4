// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "functional"
#include "iostream"
#include "string"
#include "unordered_map"

#include <fmt/core.h>
#include "common/config.h"
#include "common/memory_patcher.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    bool has_game_argument = false;
    std::string game_path;

    // Map of argument strings to lambda functions
    std::unordered_map<std::string, std::function<void(int&)>> arg_map = {
        {"-h",
         [&](int&) {
             std::cout << "Usage: shadps4 [options] <elf or eboot.bin path>\n"
                          "Options:\n"
                          "  -g, --game <path|ID>          Specify game path to launch\n"
                          "  -p, --patch <patch_file>      Apply specified patch file\n"
                          "  -f, --fullscreen <true|false> Specify window initial fullscreen "
                          "state. Does not overwrite the config file.\n"
                          "  -h, --help                    Display this help message\n";
             exit(0);
         }},
        {"--help", [&](int& i) { arg_map["-h"](i); }},

        {"-g",
         [&](int& i) {
             if (i + 1 < argc) {
                 game_path = argv[++i];
                 has_game_argument = true;
             } else {
                 std::cerr << "Error: Missing argument for -g/--game\n";
                 exit(1);
             }
         }},
        {"--game", [&](int& i) { arg_map["-g"](i); }},

        {"-p",
         [&](int& i) {
             if (i + 1 < argc) {
                 MemoryPatcher::patchFile = argv[++i];
             } else {
                 std::cerr << "Error: Missing argument for -p/--patch\n";
                 exit(1);
             }
         }},
        {"--patch", [&](int& i) { arg_map["-p"](i); }},
        {"-f",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for -f/--fullscreen\n";
                 exit(1);
             }
             std::string f_param(argv[i]);
             bool is_fullscreen;
             if (f_param == "true") {
                 is_fullscreen = true;
             } else if (f_param == "false") {
                 is_fullscreen = false;
             } else {
                 std::cerr
                     << "Error: Invalid argument for -f/--fullscreen. Use 'true' or 'false'.\n";
                 exit(1);
             }
             // Set fullscreen mode without saving it to config file
             Config::setFullscreenMode(is_fullscreen);
         }},
        {"--fullscreen", [&](int& i) { arg_map["-f"](i); }},
    };

    if (argc == 1) {
        int dummy = 0; // one does not simply pass 0 directly
        arg_map.at("-h")(dummy);
        return -1;
    }

    // Parse command-line arguments using the map
    for (int i = 1; i < argc; ++i) {
        std::string cur_arg = argv[i];
        auto it = arg_map.find(cur_arg);
        if (it != arg_map.end()) {
            it->second(i); // Call the associated lambda function
        } else if (i == argc - 1 && !has_game_argument) {
            // Assume the last argument is the game file if not specified via -g/--game
            game_path = argv[i];
            has_game_argument = true;
        } else {
            std::cerr << "Unknown argument: " << cur_arg << ", see --help for info.\n";
            return 1;
        }
    }

    if (!has_game_argument) {
        std::cerr << "Error: Please provide a game path or ID.\n";
        exit(1);
    }

    // Check if the game path or ID exists
    if (!std::filesystem::exists(game_path)) {
        std::cerr << "Error: Game file not found\n";
        return -1;
    }

    // Run the emulator with the specified game
    Core::Emulator emulator;
    emulator.Run(game_path);

    return 0;
}
