// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <set>
#include <fmt/core.h>
#include <hwinfo/hwinfo.h>

#include "common/config.h"
#include "common/debug.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#ifdef ENABLE_QT_GUI
#include <QtCore>
#endif
#include "common/assert.h"
#ifdef ENABLE_DISCORD_RPC
#include "common/discord_rpc_handler.h"
#endif
#ifdef _WIN32
#include <WinSock2.h>
#endif
#include "common/elf_info.h"
#include "common/memory_patcher.h"
#include "common/ntapi.h"
#include "common/path_util.h"
#include "common/polyfill_thread.h"
#include "common/scm_rev.h"
#include "common/singleton.h"
#include "core/devtools/widget/module_list.h"
#include "core/file_format/psf.h"
#include "core/file_format/trp.h"
#include "core/file_sys/fs.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/libc_internal/libc_internal.h"
#include "core/libraries/libs.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/np/np_trophy.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/save_data/save_backup.h"
#include "core/linker.h"
#include "core/memory.h"
#include "emulator.h"
#include "video_core/renderdoc.h"

Frontend::WindowSDL* g_window = nullptr;

namespace Core {

Emulator::Emulator() {
    // Initialize NT API functions and set high priority
#ifdef _WIN32
    Common::NtApi::Initialize();
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    // need to init this in order for winsock2 to work
    WORD versionWanted = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(versionWanted, &wsaData);
#endif
}

Emulator::~Emulator() {}

void Emulator::Run(std::filesystem::path file, const std::vector<std::string> args) {
    if (std::filesystem::is_directory(file)) {
        file /= "eboot.bin";
    }

    const auto eboot_name = file.filename().string();

    auto game_folder = file.parent_path();
    if (const auto game_folder_name = game_folder.filename().string();
        game_folder_name.ends_with("-UPDATE") || game_folder_name.ends_with("-patch")) {
        // If an executable was launched from a separate update directory,
        // use the base game directory as the game folder.
        const std::string base_name = game_folder_name.substr(0, game_folder_name.rfind('-'));
        const auto base_path = game_folder.parent_path() / base_name;
        if (std::filesystem::is_directory(base_path)) {
            game_folder = base_path;
        }
    }

    // Applications expect to be run from /app0 so mount the file's parent path as app0.
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Mount(game_folder, "/app0", true);
    // Certain games may use /hostapp as well such as CUSA001100
    mnt->Mount(game_folder, "/hostapp", true);

    const auto param_sfo_path = mnt->GetHostPath("/app0/sce_sys/param.sfo");
    const auto param_sfo_exists = std::filesystem::exists(param_sfo_path);

    // Load param.sfo details if it exists
    std::string id;
    std::string title;
    std::string app_version;
    u32 fw_version;
    Common::PSFAttributes psf_attributes{};
    if (param_sfo_exists) {
        auto* param_sfo = Common::Singleton<PSF>::Instance();
        ASSERT_MSG(param_sfo->Open(param_sfo_path), "Failed to open param.sfo");

        const auto content_id = param_sfo->GetString("CONTENT_ID");
        const auto title_id = param_sfo->GetString("TITLE_ID");
        if (content_id.has_value() && !content_id->empty()) {
            id = std::string(*content_id, 7, 9);
        } else if (title_id.has_value()) {
            id = *title_id;
        }
        title = param_sfo->GetString("TITLE").value_or("Unknown title");
        fw_version = param_sfo->GetInteger("SYSTEM_VER").value_or(0x4700000);
        app_version = param_sfo->GetString("APP_VER").value_or("Unknown version");
        if (const auto raw_attributes = param_sfo->GetInteger("ATTRIBUTE")) {
            psf_attributes.raw = *raw_attributes;
        }
    }

    Config::load(Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (id + ".toml"),
                 true);

    // Initialize logging as soon as possible
    if (!id.empty() && Config::getSeparateLogFilesEnabled()) {
        Common::Log::Initialize(id + ".log");
    } else {
        Common::Log::Initialize();
    }
    Common::Log::Start();
    if (!std::filesystem::exists(file)) {
        LOG_CRITICAL(Loader, "eboot.bin does not exist: {}",
                     std::filesystem::absolute(file).string());
        std::quick_exit(0);
    }

    LOG_INFO(Loader, "Starting shadps4 emulator v{} ", Common::g_version);
    LOG_INFO(Loader, "Revision {}", Common::g_scm_rev);
    LOG_INFO(Loader, "Branch {}", Common::g_scm_branch);
    LOG_INFO(Loader, "Description {}", Common::g_scm_desc);
    LOG_INFO(Loader, "Remote {}", Common::g_scm_remote_url);

    const bool has_game_config = std::filesystem::exists(
        Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (id + ".toml"));
    LOG_INFO(Config, "Game-specific config exists: {}", has_game_config);

    LOG_INFO(Config, "General LogType: {}", Config::getLogType());
    LOG_INFO(Config, "General isNeo: {}", Config::isNeoModeConsole());
    LOG_INFO(Config, "General isDevKit: {}", Config::isDevKitConsole());
    LOG_INFO(Config, "General isConnectedToNetwork: {}", Config::getIsConnectedToNetwork());
    LOG_INFO(Config, "General isPsnSignedIn: {}", Config::getPSNSignedIn());
    LOG_INFO(Config, "GPU isNullGpu: {}", Config::nullGpu());
    LOG_INFO(Config, "GPU readbacks: {}", Config::readbacks());
    LOG_INFO(Config, "GPU readbackLinearImages: {}", Config::readbackLinearImages());
    LOG_INFO(Config, "GPU directMemoryAccess: {}", Config::directMemoryAccess());
    LOG_INFO(Config, "GPU shouldDumpShaders: {}", Config::dumpShaders());
    LOG_INFO(Config, "GPU vblankFrequency: {}", Config::vblankFreq());
    LOG_INFO(Config, "Vulkan gpuId: {}", Config::getGpuId());
    LOG_INFO(Config, "Vulkan vkValidation: {}", Config::vkValidationEnabled());
    LOG_INFO(Config, "Vulkan vkValidationSync: {}", Config::vkValidationSyncEnabled());
    LOG_INFO(Config, "Vulkan vkValidationGpu: {}", Config::vkValidationGpuEnabled());
    LOG_INFO(Config, "Vulkan crashDiagnostics: {}", Config::getVkCrashDiagnosticEnabled());
    LOG_INFO(Config, "Vulkan hostMarkers: {}", Config::getVkHostMarkersEnabled());
    LOG_INFO(Config, "Vulkan guestMarkers: {}", Config::getVkGuestMarkersEnabled());
    LOG_INFO(Config, "Vulkan rdocEnable: {}", Config::isRdocEnabled());

    hwinfo::Memory ram;
    hwinfo::OS os;
    const auto cpus = hwinfo::getAllCPUs();
    for (const auto& cpu : cpus) {
        LOG_INFO(Config, "CPU Model: {}", cpu.modelName());
        LOG_INFO(Config, "CPU Physical Cores: {}, Logical Cores: {}", cpu.numPhysicalCores(),
                 cpu.numLogicalCores());
    }
    LOG_INFO(Config, "Total RAM: {} GB", std::round(ram.total_Bytes() / pow(1024, 3)));
    LOG_INFO(Config, "Operating System: {}", os.name());

    if (param_sfo_exists) {
        LOG_INFO(Loader, "Game id: {} Title: {}", id, title);
        LOG_INFO(Loader, "Fw: {:#x} App Version: {}", fw_version, app_version);
        LOG_INFO(Loader, "PSVR Supported: {}", (bool)psf_attributes.support_ps_vr.Value());
        LOG_INFO(Loader, "PSVR Required: {}", (bool)psf_attributes.require_ps_vr.Value());
    }
    if (!args.empty()) {
        const auto argc = std::min<size_t>(args.size(), 32);
        for (auto i = 0; i < argc; i++) {
            LOG_INFO(Loader, "Game argument {}: {}", i, args[i]);
        }
        if (args.size() > 32) {
            LOG_ERROR(Loader, "Too many game arguments, only passing the first 32");
        }
    }

    // Create stdin/stdout/stderr
    Common::Singleton<FileSys::HandleTable>::Instance()->CreateStdHandles();

    // Initialize components
    memory = Core::Memory::Instance();
    controller = Common::Singleton<Input::GameController>::Instance();
    linker = Common::Singleton<Core::Linker>::Instance();

    // Load renderdoc module
    VideoCore::LoadRenderDoc();

    // Initialize patcher and trophies
    if (!id.empty()) {
        MemoryPatcher::g_game_serial = id;
        Libraries::Np::NpTrophy::game_serial = id;

        const auto trophyDir =
            Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / id / "TrophyFiles";
        if (!std::filesystem::exists(trophyDir)) {
            TRP trp;
            if (!trp.Extract(game_folder, id)) {
                LOG_ERROR(Loader, "Couldn't extract trophies");
            }
        }
    }

    auto& game_info = Common::ElfInfo::Instance();
    game_info.initialized = true;
    game_info.game_serial = id;
    game_info.title = title;
    game_info.app_ver = app_version;
    game_info.firmware_ver = fw_version & 0xFFF00000;
    game_info.raw_firmware_ver = fw_version;
    game_info.psf_attributes = psf_attributes;

    const auto pic1_path = mnt->GetHostPath("/app0/sce_sys/pic1.png");
    if (std::filesystem::exists(pic1_path)) {
        game_info.splash_path = pic1_path;
    }

    game_info.game_folder = game_folder;

    std::string game_title = fmt::format("{} - {} <{}>", id, title, app_version);
    std::string window_title = "";
    std::string remote_url(Common::g_scm_remote_url);
    std::string remote_host = Common::GetRemoteNameFromLink();
    if (Common::g_is_release) {
        if (remote_host == "shadps4-emu" || remote_url.length() == 0) {
            window_title = fmt::format("shadPS4 v{} | {}", Common::g_version, game_title);
        } else {
            window_title =
                fmt::format("shadPS4 {}/v{} | {}", remote_host, Common::g_version, game_title);
        }
    } else {
        if (remote_host == "shadps4-emu" || remote_url.length() == 0) {
            window_title = fmt::format("shadPS4 v{} {} {} | {}", Common::g_version,
                                       Common::g_scm_branch, Common::g_scm_desc, game_title);
        } else {
            window_title = fmt::format("shadPS4 v{} {}/{} {} | {}", Common::g_version, remote_host,
                                       Common::g_scm_branch, Common::g_scm_desc, game_title);
        }
    }
    window = std::make_unique<Frontend::WindowSDL>(
        Config::getWindowWidth(), Config::getWindowHeight(), controller, window_title);

    g_window = window.get();

    const auto& mount_data_dir = Common::FS::GetUserPath(Common::FS::PathType::GameDataDir) / id;
    if (!std::filesystem::exists(mount_data_dir)) {
        std::filesystem::create_directory(mount_data_dir);
    }
    mnt->Mount(mount_data_dir, "/data"); // should just exist, manually create with game serial

    // Mounting temp folders
    const auto& mount_temp_dir = Common::FS::GetUserPath(Common::FS::PathType::TempDataDir) / id;
    if (std::filesystem::exists(mount_temp_dir)) {
        // Temp folder should be cleared on each boot.
        std::filesystem::remove_all(mount_temp_dir);
    }
    std::filesystem::create_directory(mount_temp_dir);
    mnt->Mount(mount_temp_dir, "/temp0");
    mnt->Mount(mount_temp_dir, "/temp");

    const auto& mount_download_dir =
        Common::FS::GetUserPath(Common::FS::PathType::DownloadDir) / id;
    if (!std::filesystem::exists(mount_download_dir)) {
        std::filesystem::create_directory(mount_download_dir);
    }
    mnt->Mount(mount_download_dir, "/download0");

    const auto& mount_captures_dir = Common::FS::GetUserPath(Common::FS::PathType::CapturesDir);
    if (!std::filesystem::exists(mount_captures_dir)) {
        std::filesystem::create_directory(mount_captures_dir);
    }
    VideoCore::SetOutputDir(mount_captures_dir, id);

    // Initialize kernel and library facilities.
    Libraries::InitHLELibs(&linker->GetHLESymbols());

    // Load the module with the linker
    const auto eboot_path = mnt->GetHostPath("/app0/" + eboot_name);
    if (linker->LoadModule(eboot_path) == -1) {
        LOG_CRITICAL(Loader, "Failed to load game's eboot.bin: {}",
                     std::filesystem::absolute(eboot_path).string());
        std::quick_exit(0);
    }

    // check if we have system modules to load
    LoadSystemModules(game_info.game_serial);

    // Load all prx from game's sce_module folder
    mnt->IterateDirectory("/app0/sce_module", [this](const auto& path, const auto is_file) {
        if (is_file) {
            LOG_INFO(Loader, "Loading {}", fmt::UTF(path.u8string()));
            linker->LoadModule(path);
        }
    });

#ifdef ENABLE_DISCORD_RPC
    // Discord RPC
    if (Config::getEnableDiscordRPC()) {
        auto* rpc = Common::Singleton<DiscordRPCHandler::RPC>::Instance();
        if (rpc->getRPCEnabled() == false) {
            rpc->init();
        }
        rpc->setStatusPlaying(game_info.title, id);
    }
#endif

    // Start the timer (Play Time)
#ifdef ENABLE_QT_GUI
    if (!id.empty()) {
        start_time = std::chrono::steady_clock::now();

        std::thread([this, id]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(60));
                UpdatePlayTime(id);
                start_time = std::chrono::steady_clock::now();
            }
        }).detach();
    }
#endif

    linker->Execute(args);

    window->InitTimers();
    while (window->IsOpen()) {
        window->WaitEvent();
    }

#ifdef ENABLE_QT_GUI
    UpdatePlayTime(id);
#endif

    std::quick_exit(0);
}

void Emulator::LoadSystemModules(const std::string& game_serial) {
    constexpr auto ModulesToLoad = std::to_array<SysModules>(
        {{"libSceNgs2.sprx", &Libraries::Ngs2::RegisterLib},
         {"libSceUlt.sprx", nullptr},
         {"libSceJson.sprx", nullptr},
         {"libSceJson2.sprx", nullptr},
         {"libSceLibcInternal.sprx", &Libraries::LibcInternal::RegisterLib},
         {"libSceCesCs.sprx", nullptr},
         {"libSceFont.sprx", nullptr},
         {"libSceFontFt.sprx", nullptr},
         {"libSceFreeTypeOt.sprx", nullptr}});

    std::vector<std::filesystem::path> found_modules;
    const auto& sys_module_path = Common::FS::GetUserPath(Common::FS::PathType::SysModuleDir);
    for (const auto& entry : std::filesystem::directory_iterator(sys_module_path)) {
        found_modules.push_back(entry.path());
    }
    for (const auto& [module_name, init_func] : ModulesToLoad) {
        const auto it = std::ranges::find_if(
            found_modules, [&](const auto& path) { return path.filename() == module_name; });
        if (it != found_modules.end()) {
            LOG_INFO(Loader, "Loading {}", it->string());
            if (linker->LoadModule(*it) != -1) {
                continue;
            }
        }
        if (init_func) {
            LOG_INFO(Loader, "Can't Load {} switching to HLE", module_name);
            init_func(&linker->GetHLESymbols());
        } else {
            LOG_INFO(Loader, "No HLE available for {} module", module_name);
        }
    }
    if (!game_serial.empty() && std::filesystem::exists(sys_module_path / game_serial)) {
        for (const auto& entry :
             std::filesystem::directory_iterator(sys_module_path / game_serial)) {
            LOG_INFO(Loader, "Loading {} from game serial file {}", entry.path().string(),
                     game_serial);
            linker->LoadModule(entry.path());
        }
    }
}

#ifdef ENABLE_QT_GUI
void Emulator::UpdatePlayTime(const std::string& serial) {
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    QString filePath = QString::fromStdString((user_dir / "play_time.txt").string());

    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        LOG_INFO(Loader, "Error opening play_time.txt");
        return;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    int totalSeconds = duration.count();

    QTextStream in(&file);
    QStringList lines;
    QString content;
    while (!in.atEnd()) {
        content += in.readLine() + "\n";
    }
    file.close();

    QStringList existingLines = content.split('\n', Qt::SkipEmptyParts);
    int accumulatedSeconds = 0;
    bool found = false;

    for (const QString& line : existingLines) {
        QStringList parts = line.split(' ');
        if (parts.size() == 2 && parts[0] == QString::fromStdString(serial)) {
            QStringList timeParts = parts[1].split(':');
            if (timeParts.size() == 3) {
                int hours = timeParts[0].toInt();
                int minutes = timeParts[1].toInt();
                int seconds = timeParts[2].toInt();
                accumulatedSeconds = hours * 3600 + minutes * 60 + seconds;
                found = true;
                break;
            }
        }
    }
    accumulatedSeconds += totalSeconds;
    int hours = accumulatedSeconds / 3600;
    int minutes = (accumulatedSeconds % 3600) / 60;
    int seconds = accumulatedSeconds % 60;
    QString playTimeSaved = QString::number(hours) + ":" +
                            QString::number(minutes).rightJustified(2, '0') + ":" +
                            QString::number(seconds).rightJustified(2, '0');

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        bool lineUpdated = false;

        for (const QString& line : existingLines) {
            if (line.startsWith(QString::fromStdString(serial))) {
                out << QString::fromStdString(serial) + " " + playTimeSaved + "\n";
                lineUpdated = true;
            } else {
                out << line << "\n";
            }
        }

        if (!lineUpdated) {
            out << QString::fromStdString(serial) + " " + playTimeSaved + "\n";
        }
    }
    LOG_INFO(Loader, "Playing time for {}: {}", serial, playTimeSaved.toStdString());
}
#endif

} // namespace Core
