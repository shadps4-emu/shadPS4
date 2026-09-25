// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <thread>
#include <core/emulator_state.h>

#include "common/config.h"
#include "common/logging/backend.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc_client.h"
#include "core/libraries/audio/audioout.h"
#include "emulator.h"
#include "game_directory_dialog.h"
#include "imgui/big_picture.h"
#include "iostream"
#include "main_window.h"
#include "main_window_themes.h"
#include "qt_gui/compatibility_info.h"
#include "system_error"
#include "unordered_map"
#include "vector"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "welcome_dialog.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;
WindowThemes m_window_themes;

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#endif
#include <input/input_handler.h>
std::shared_ptr<IpcClient> m_ipc_client;

void customMessageHandler(QtMsgType, const QMessageLogContext&, const QString&) {}

void StopProgram() {
    exit(0);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    int argc_w;
    wchar_t** argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);

    QApplication* a = nullptr;

    if (argv_w != nullptr) {
        std::vector<std::string> argv_utf8;
        std::vector<char*> argv_converted;
        argv_utf8.reserve(argc_w);
        argv_converted.reserve(argc_w + 1);

        for (int i = 0; i < argc_w; ++i) {
            QString arg = QString::fromWCharArray(argv_w[i]);
            argv_utf8.push_back(arg.toStdString());
            argv_converted.push_back(const_cast<char*>(argv_utf8[i].c_str()));
        }
        argv_converted.push_back(nullptr);

        a = new QApplication(argc_w, argv_converted.data());

        LocalFree(argv_w);
    } else {
        a = new QApplication(argc, argv);
    }
#else
    QApplication a(argc, argv);
    QApplication* app_ptr = &a;
#endif

    QApplication::setApplicationName("shadps4");
    QApplication::setDesktopFileName("net.shadps4.shadPS4");

    // Init emulator state
    std::shared_ptr<EmulatorState> m_emu_state = std::make_shared<EmulatorState>();
    EmulatorState::SetInstance(m_emu_state);

    // Initialize paths properly before loading config
    // This ensures we load from the correct global or portable path
    if (!Common::FS::IsUserPathsInitialized()) {
        auto portable_dir = Common::FS::GetPortablePath();
        auto global_dir = Common::FS::GetGlobalPath();

        bool portableExists = std::filesystem::exists(portable_dir);
        bool globalExists = std::filesystem::exists(global_dir);

        Common::FS::PathInitState detected_state;
        if (portableExists) {
            detected_state = Common::FS::PathInitState::Portable;
        } else if (globalExists) {
            detected_state = Common::FS::PathInitState::Global;
        } else {
            detected_state = Common::FS::PathInitState::Portable;
        }

        Common::FS::InitializeUserPaths(detected_state);
    }

    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");
    bool ignore_mods_path = false;

    bool has_command_line_argument = argc > 1;
    bool show_gui = false, has_game_argument = false;
    bool big_picture_mode = false;
    std::string game_path;
    std::vector<std::string> game_arg{};

    std::optional<std::filesystem::path> gameFolder;
    std::optional<std::filesystem::path> modsFolder;

    bool waitForDebugger = false;
    std::optional<int> waitPid;

    Core::Emulator* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;

    bool has_emulator_argument = false;
    QStringList emulator_args{};
    std::string emulator_path;

    std::unordered_map<std::string, std::function<void(int&)>> arg_map = {
        {"-h",
         [&](int&) {
             std::cout
                 << "Usage: shadps4 [options]\n"
                    "Options:\n"
                    "  No arguments: Opens the GUI.\n"
                    "  -g, --game <path|ID>          Specify <eboot.bin or elf path> or "
                    "<game ID (CUSAXXXXX)> to launch\n"
                    " -- ...                         Parameters passed to the game ELF. "
                    "Needs to be at the end of the line, and everything after \"--\" is a "
                    "game argument.\n"
                    "  -p, --patch <patch_file>      Apply specified patch file\n"
                    "  -i, --ignore-game-patch       Disable automatic loading of game patch\n"
                    "  -s, --show-gui                Show the GUI\n"
                    "  -b, --bigpicture              Launch in Big Picture mode\n"
                    "  -f, --fullscreen <true|false> Specify window initial fullscreen "
                    "state. Does not overwrite the config file.\n"
                    "  --add-game-folder <folder>    Adds a new game folder to the config.\n"
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
                    "  -h, --help                    Display this help message\n";
             exit(0);
         }},
        {"--help", [&](int& i) { arg_map["-h"](i); }},

        {"-s", [&](int&) { show_gui = true; }},
        {"--show-gui", [&](int& i) { arg_map["-s"](i); }},
        {"-b", [&](int&) { big_picture_mode = true; }},
        {"--bigpicture", [&](int& i) { arg_map["-b"](i); }},

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

        {"-e",
         [&](int& i) {
             if (i + 1 < argc) {
                 emulator_path = argv[++i];
                 has_emulator_argument = true;
             } else {
                 std::cerr << "Error: Missing argument for -e/--emulator\n";
                 exit(1);
             }
         }},
        {"--emulator", [&](int& i) { arg_map["-e"](i); }},

        {"-p",
         [&](int& i) {
             if (i + 1 < argc) {
                 MemoryPatcher::patch_file = argv[++i];
             } else {
                 std::cerr << "Error: Missing argument for -p\n";
                 exit(1);
             }
         }},
        {"--patch", [&](int& i) { arg_map["-p"](i); }},
        {"-i", [&](int&) { Core::FileSys::MntPoints::ignore_game_patches = true; }},
        {"--ignore-game-patch", [&](int& i) { arg_map["-i"](i); }},
        {"-im",
         [&](int&) {
             ignore_mods_path = true;
             Core::FileSys::MntPoints::enable_mods = false;
         }},
        {"--ignore-mods-path", [&](int& i) { arg_map["-im"](i); }},
        {"-mf",
         [&](int& i) {
             if (i + 1 < argc) {
                 std::filesystem::path dir(argv[++i]);
                 if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                     std::cerr << "Error: Invalid mods folder: " << dir << "\n";
                     exit(1);
                 }
                 Core::FileSys::MntPoints::enable_mods = true;
                 Core::FileSys::MntPoints::manual_mods_path = dir;
                 modsFolder = dir;
                 std::cout << "Mods folder set: " << dir << "\n";
             } else {
                 std::cerr << "Error: Missing argument for -mf/--mods-folder\n";
                 exit(1);
             }
         }},
        {"--mods-folder", [&](int& i) { arg_map["-mf"](i); }},
        {"-f",
         [&](int& i) {
             if (++i >= argc) {
                 std::cerr
                     << "Error: Invalid argument for -f/--fullscreen. Use 'true' or 'false'.\n";
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
             if (!std::filesystem::is_directory(config_path, discard)) {
                 std::cerr << "Error: Directory does not exist: " << config_path << "\n";
                 exit(1);
             }

             Config::addGameDirectories(config_path);
             Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
             std::cout << "Game folder successfully saved.\n";
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
             gameFolder = folder;
         }},
        {"--wait-for-debugger", [&](int& i) { waitForDebugger = true; }},
        {"--wait-for-pid", [&](int& i) {
             if (++i >= argc) {
                 std::cerr << "Error: Missing argument for --wait-for-pid\n";
                 exit(1);
             }
             waitPid = std::stoi(argv[i]);
         }}};

    for (int i = 1; i < argc; ++i) {
        std::string cur_arg = argv[i];
        auto it = arg_map.find(cur_arg);

        if (it != arg_map.end()) {
            it->second(i);
            continue;
        }

        if (!has_game_argument && !cur_arg.empty() && cur_arg[0] != '-') {
            game_path = cur_arg;
            has_game_argument = true;
            continue;

        } else if (std::string(argv[i]) == "--") {
            if (i + 1 == argc) {
                std::cerr << "Warning: -- is set, but no game arguments are added!\n";
                break;
            }
            for (int j = i + 1; j < argc; j++) {
                if (has_emulator_argument)
                    emulator_args.push_back(argv[j]);
                else
                    game_arg.push_back(argv[j]);
            }
            break;
        } else if (i + 1 < argc && std::string(argv[i + 1]) == "--") {
            if (!has_game_argument) {
                game_path = argv[i];
                has_game_argument = true;
            }
        } else {
            std::cerr << "Unknown argument: " << cur_arg << ", see --help for info.\n";
            return 1;
        }
    }
    if (has_emulator_argument && !has_game_argument) {
        game_path = emulator_path;
        has_game_argument = true;

        for (const auto& arg : emulator_args) {
            game_arg.push_back(arg.toStdString());
        }

        has_command_line_argument = true;
    }

    if (big_picture_mode) {
        if (has_game_argument) {
            std::cerr << "Error: Big picture mode cannot be used with a specific game. Use big "
                         "picture mode to select games.\n";
            exit(1);
        }
        BigPictureMode::Launch();
        return 0;
    }

    if (has_game_argument) {
        std::filesystem::path eboot_path(game_path);
        if (!std::filesystem::exists(eboot_path)) {
            bool found = false;
            const int max_depth = 5;
            for (const auto& dir : Config::getGameDirectories()) {
                if (auto found_path = Common::FS::FindGameByID(dir, game_path, max_depth)) {
                    eboot_path = *found_path;
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "Error: Game not found: " << game_path << "\n";
                exit(1);
            }
        }

        if (!modsFolder.has_value()) {
            auto base_folder = eboot_path.parent_path();
            auto parent = base_folder.parent_path();
            auto game_folder_name = base_folder.filename().string();

            std::vector<std::string> modSuffixes = {"-mods", "-MODS", "-Mods"};
            bool found = false;

            for (const auto& suffix : modSuffixes) {
                auto auto_mods_folder = parent / (game_folder_name + suffix);
                if (Common::FS::Zar::Exists(auto_mods_folder) &&
                    std::filesystem::is_directory(auto_mods_folder)) {
                    modsFolder = auto_mods_folder;
                    Core::FileSys::MntPoints::enable_mods = true;
                    std::cout << "Auto-detected mods folder: " << auto_mods_folder << "\n";
                    found = true;
                    break;
                }
            }

            // Patch/update overlays are resolved generically by fs.cpp (which prefers
            // "-UPDATE", falling back to "-patch") and don't need an explicit override here,
            // but log the detected overlay for parity/visibility with mods detection.
            std::filesystem::path patch_overlay_path = base_folder;
            patch_overlay_path += "-UPDATE";
            if (!std::filesystem::exists(patch_overlay_path)) {
                patch_overlay_path = base_folder;
                patch_overlay_path += "-patch";
            }
            if (std::filesystem::exists(patch_overlay_path)) {
                std::cout << "Detected patch/update overlay: " << patch_overlay_path << "\n";
            }
        } else {
            std::cout << "Using manually specified mods folder: " << modsFolder->string() << "\n";
        }
    }

    WelcomeDialog welcomeDlg(&m_window_themes);
    m_window_themes.ApplyThemeToDialog(&welcomeDlg);

    if (!has_command_line_argument && Config::getShowWelcomeDialog()) {
        welcomeDlg.exec();
    }

    // If no game directories are set and no command line argument, prompt for it
    if (Config::getGameDirectoriesEnabled().empty() && !has_command_line_argument) {
        GameDirectoryDialog dlg;
        dlg.exec();
    }

    qInstallMessageHandler(customMessageHandler);

    MainWindow* m_main_window = new MainWindow(nullptr);

    if ((has_command_line_argument && show_gui) || !has_command_line_argument) {
        m_main_window->Init();
    }

    if (has_command_line_argument && !has_game_argument) {
        std::cerr << "Error: Please provide a game path or ID.\n";
        exit(1);
    }

    if (waitPid.has_value()) {
        Core::Debugger::WaitForPid(waitPid.value());
    }

    const bool ipc_enabled = qEnvironmentVariableIsSet("SHADPS4_ENABLE_IPC");
    const bool mods_env_enabled = qEnvironmentVariableIsSet("SHADPS4_ENABLE_MODS") &&
                                  qEnvironmentVariable("SHADPS4_ENABLE_MODS") == "1";

    const bool ignorePatches = qEnvironmentVariableIsSet("SHADPS4_BASE_GAME") &&
                               qEnvironmentVariable("SHADPS4_BASE_GAME") == "1";

    if (!modsFolder.has_value()) {
        Core::FileSys::MntPoints::enable_mods = mods_env_enabled;
    }

    if (qEnvironmentVariableIsSet("SHADPS4_BASE_GAME")) {
        Core::FileSys::MntPoints::ignore_game_patches = ignorePatches;
    }

    if (ignore_mods_path) {
        Core::FileSys::MntPoints::enable_mods = false;
        Core::FileSys::MntPoints::manual_mods_path.clear();
    }

    if (ipc_enabled) {
        std::cout << ";#IPC_ENABLED\n";
        std::cout << ";ENABLE_MEMORY_PATCH\n";
        std::cout << ";ENABLE_EMU_CONTROL\n";
        std::cout << ";SET_ACTIVE_PAD\n";
        std::cout << ";SET_ACTIVE_CONTROLLER\n";
        std::cout << ";#IPC_END\n";
        std::cout.flush();

        auto next_str = [&]() -> const std::string& {
            static std::string line_buffer;
            do {
                std::getline(std::cin, line_buffer);
            } while (!line_buffer.empty() && line_buffer.back() == '\\');
            return line_buffer;
        };

        auto next_u64 = [&]() -> u64 {
            auto& str = next_str();
            return std::stoull(str, nullptr, 0);
        };

        std::thread([emulator, ipc_client = m_ipc_client, &next_str, &next_u64]() {
            std::string cmd;
            while (std::getline(std::cin, cmd)) {
                if (cmd == "PATCH_MEMORY") {
                    std::string modName, offset, value, target, size, isOffset, littleEndian, mask,
                        maskOffset;
                    std::getline(std::cin, modName);
                    std::getline(std::cin, offset);
                    std::getline(std::cin, value);
                    std::getline(std::cin, target);
                    std::getline(std::cin, size);
                    std::getline(std::cin, isOffset);
                    std::getline(std::cin, littleEndian);
                    std::getline(std::cin, mask);
                    std::getline(std::cin, maskOffset);

                    MemoryPatcher::ApplyRuntimePatch(modName, offset, value, target, size,
                                                     isOffset == "1", littleEndian == "1",
                                                     std::stoi(mask), std::stoi(maskOffset));
                } else if (cmd == "PAUSE" || cmd == "RESUME") {
                    SDL_Event e;
                    e.type = SDL_EVENT_TOGGLE_PAUSE;
                    SDL_PushEvent(&e);
                } else if (cmd == "ADJUST_VOLUME") {
                    int value = static_cast<int>(std::stoull(next_str(), nullptr, 0));
                    bool is_game_specific = next_u64() != 0;
                    Config::setVolumeSlider(value, is_game_specific);
                    Libraries::AudioOut::AdjustVol();
                } else if (cmd == "SET_FSR") {
                    bool use_fsr = std::stoull(next_str(), nullptr, 0) != 0;
                    if (presenter)
                        presenter->GetFsrSettingsRef().enable = use_fsr;
                } else if (cmd == "SET_RCAS") {
                    bool use_rcas = std::stoull(next_str(), nullptr, 0) != 0;
                    if (presenter)
                        presenter->GetFsrSettingsRef().use_rcas = use_rcas;
                } else if (cmd == "SET_RCAS_ATTENUATION") {
                    int value = static_cast<int>(std::stoull(next_str(), nullptr, 0));
                    if (presenter)
                        presenter->GetFsrSettingsRef().rcasAttenuation =
                            static_cast<float>(value / 1000.0f);
                } else if (cmd == "RELOAD_INPUTS") {
                    std::string config = next_str();
                    Input::ParseInputConfig(config);
                } else if (cmd == "SET_ACTIVE_PAD") {
                    int padIndex = static_cast<int>(std::stoull(next_str(), nullptr, 0));

                    try {
                        std::string active_controller = next_str();
                        GamepadSelect::SetSelectedGamepad(active_controller);
                    } catch (...) {
                        std::cerr
                            << "[IPC] SET_ACTIVE_PAD: cannot call GamepadSelect::SetActivePad\n";
                    }

                    SDL_Event checkGamepad;
                    SDL_memset(&checkGamepad, 0, sizeof(checkGamepad));
                    checkGamepad.type = SDL_EVENT_CHANGE_CONTROLLER;
                    SDL_PushEvent(&checkGamepad);

                } else if (cmd == "SET_ACTIVE_CONTROLLER") {
                    std::string guid = next_str();
                    GamepadSelect::SetSelectedGamepad(guid);

                    SDL_Event checkGamepad;
                    SDL_memset(&checkGamepad, 0, sizeof(checkGamepad));
                    checkGamepad.type = SDL_EVENT_CHANGE_CONTROLLER;
                    SDL_PushEvent(&checkGamepad);

                } else if (cmd == "STOP") {
                    if (!Config::getGameRunning())
                        continue;
                    if (ipc_client)
                        ipc_client->stopGame();
                    Config::setGameRunning(false);
                } else if (cmd == "RESTART") {
                    if (!Config::getGameRunning())
                        continue;
                    if (ipc_client)
                        ipc_client->restartGame();
                }
            }
        }).detach();
    }

    if (has_game_argument) {
        std::filesystem::path game_file_path(game_path);
        const auto archive_component_exists = [](const std::filesystem::path& p) -> bool {
            std::filesystem::path accum;
            for (const auto& comp : p) {
                accum /= comp;
                if (comp.extension() == ".zar") {
                    return std::filesystem::is_regular_file(accum);
                }
            }
            return false;
        };
        if (!std::filesystem::exists(game_file_path) && !archive_component_exists(game_file_path)) {
            bool game_found = false;
            const int max_depth = 5;
            for (const auto& install_dir : Config::getGameDirectories()) {
                if (auto found_path = Common::FS::FindGameByID(install_dir, game_path, max_depth)) {
                    game_file_path = *found_path;
                    game_found = true;
                    break;
                }
            }
            if (!game_found) {
                std::cerr << "Error: Game ID or file path not found: " << game_path << std::endl;
                return 1;
            }
        }

        emulator->Run(game_file_path.string(), game_arg, gameFolder);
        if (!show_gui) {
            return 0;
        }
    }

    m_main_window->show();
#ifdef _WIN32
    return a->exec();
#else
    return app_ptr->exec();
#endif
}