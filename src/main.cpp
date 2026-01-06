// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_messagebox.h>
#include "functional"
#include "iostream"
#include "string"
#include "system_error"
#include "unordered_map"

#include <fmt/core.h>
#include "common/config.h"
#include "common/logging/backend.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    IPC::Instance().Init();

    // Load configurations
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    bool has_game_argument = false;
    std::string game_path;
    std::vector<std::string> game_args{};
    std::optional<std::filesystem::path> game_folder;

    bool waitForDebugger = false;
    std::optional<int> waitPid;

    // Map of argument strings to lambda functions
    std::unordered_map<std::string, std::function<void(int&)>> arg_map = {
        {"-h",
         [&](int&) {
             std::cout
                 << "Usage: shadps4 [options] <elf or eboot.bin path>\n"
                    "Options:\n"
                    "  -g, --game <path|ID>          Specify game path to launch\n"
                    " -- ...                         Parameters passed to the game ELF. "
                    "Needs to be at the end of the line, and everything after \"--\" is a "
                    "game argument.\n"
                    "  -p, --patch <patch_file>      Apply specified patch file\n"
                    "  -i, --ignore-game-patch       Disable automatic loading of game patch\n"
                    "  -f, --fullscreen <true|false> Specify window initial fullscreen "
                    "state. Does not overwrite the config file.\n"
                    "  --add-game-folder <folder>    Adds a new game folder to the config.\n"
                    "  --set-addon-folder <folder>   Sets the addon folder to the config.\n"
                    "  --log-append                  Append log output to file instead of "
                    "overwriting it.\n"
                    "  --override-root <folder>      Override the game root folder. Default is the "
                    "parent of game path\n"
                    "  --wait-for-debugger           Wait for debugger to attach\n"
                    "  --wait-for-pid <pid>          Wait for process with specified PID to stop\n"
                    "  --config-clean                Run the emulator with the default config "
                    "values, ignores the config file(s) entirely.\n"
                    "  --config-global               Run the emulator with the base config file "
                    "only, ignores game specific configs.\n"
                    "  --show-fps                    Enable FPS counter display at startup\n"
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
                 MemoryPatcher::patch_file = argv[++i];
             } else {
                 std::cerr << "Error: Missing argument for -p/--patch\n";
                 exit(1);
             }
         }},
        {"--patch", [&](int& i) { arg_map["-p"](i); }},

        {"-i", [&](int&) { Core::FileSys::MntPoints::ignore_game_patches = true; }},
        {"--ignore-game-patch", [&](int& i) { arg_map["-i"](i); }},
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
             Config::setIsFullscreen(is_fullscreen);
         }},
        {"--fullscreen", [&](int& i) { arg_map["-f"](i); }},
        {"--add-game-folder",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for --add-game-folder\n";
                 exit(1);
             }
             std::string config_dir(argv[i]);
             std::filesystem::path config_path = std::filesystem::path(config_dir);
             std::error_code discard;
             if (!std::filesystem::exists(config_path, discard)) {
                 std::cerr << "Error: File does not exist: " << config_path << "\n";
                 exit(1);
             }

             Config::addGameInstallDir(config_path);
             Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
             std::cout << "Game folder successfully saved.\n";
             exit(0);
         }},
        {"--set-addon-folder",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for --add-addon-folder\n";
                 exit(1);
             }
             std::string config_dir(argv[i]);
             std::filesystem::path config_path = std::filesystem::path(config_dir);
             std::error_code discard;
             if (!std::filesystem::exists(config_path, discard)) {
                 std::cerr << "Error: File does not exist: " << config_path << "\n";
                 exit(1);
             }

             Config::setAddonInstallDir(config_path);
             Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
             std::cout << "Addon folder successfully saved.\n";
             exit(0);
         }},
        {"--log-append", [&](int& i) { Common::Log::SetAppend(); }},
        {"--config-clean", [&](int& i) { Config::setConfigMode(Config::ConfigMode::Clean); }},
        {"--config-global", [&](int& i) { Config::setConfigMode(Config::ConfigMode::Global); }},
        {"--override-root",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for --override-root\n";
                 exit(1);
             }
             std::string folder_str{argv[i]};
             std::filesystem::path folder{folder_str};
             if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
                 std::cerr << "Error: Folder does not exist: " << folder_str << "\n";
                 exit(1);
             }
             game_folder = folder;
         }},
        {"--wait-for-debugger", [&](int& i) { waitForDebugger = true; }},
        {"--wait-for-pid",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for --wait-for-pid\n";
                 exit(1);
             }
             waitPid = std::stoi(argv[i]);
         }},
        {"--show-fps", [&](int& i) { Config::setShowFpsCounter(true); }}};

    if (argc == 1) {
        if (!SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_INFORMATION, "shadPS4",
                "This is a CLI application. Please use the QTLauncher for a GUI: "
                "https://github.com/shadps4-emu/shadps4-qtlauncher/releases",
                nullptr))
            std::cerr << "Could not display SDL message box! Error: " << SDL_GetError() << "\n";
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
        } else if (std::string(argv[i]) == "--") {
            if (i + 1 == argc) {
                std::cerr << "Warning: -- is set, but no game arguments are added!\n";
                break;
            }
            for (int j = i + 1; j < argc; j++) {
                game_args.push_back(argv[j]);
            }
            break;
        } else if (i + 1 < argc && std::string(argv[i + 1]) == "--") {
            if (!has_game_argument) {
                game_path = argv[i];
                has_game_argument = true;
            }
        } else {
            std::cerr << "Unknown argument: " << cur_arg << ", see --help for info.\n";
        }
    }

    // If no game directory is set and no command line argument, prompt for it
    if (Config::getGameInstallDirs().empty()) {
        std::cerr << "Warning: No game folder set, please set it by calling shadps4"
                     " with the --add-game-folder <folder_name> argument\n";
    }

    if (!has_game_argument) {
        std::cerr << "Error: Please provide a game path or ID.\n";
        exit(1);
    }

    // Check if the game path or ID exists
    std::filesystem::path eboot_path(game_path);

    // Check if the provided path is a valid file
    if (!std::filesystem::exists(eboot_path)) {
        // If not a file, treat it as a game ID and search in install directories recursively
        bool game_found = false;
        const int max_depth = 5;
        for (const auto& install_dir : Config::getGameInstallDirs()) {
            if (auto found_path = Common::FS::FindGameByID(install_dir, game_path, max_depth)) {
                eboot_path = *found_path;
                game_found = true;
                break;
            }
        }
        if (!game_found) {
            std::cerr << "Error: Game ID or file path not found: " << game_path << std::endl;
            return 1;
        }
    }

    if (waitPid.has_value()) {
        Core::Debugger::WaitForPid(waitPid.value());
    }

    // Run the emulator with the resolved eboot path
    Core::Emulator* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(eboot_path, game_args, game_folder);

    return 0;
}
