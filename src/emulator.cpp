// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <set>
#include <fmt/core.h>

#include "common/config.h"
#include "common/debug.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#ifdef ENABLE_QT_GUI
#include <QtCore>
#include "common/memory_patcher.h"
#endif
#include "common/assert.h"
#ifdef ENABLE_DISCORD_RPC
#include "common/discord_rpc_handler.h"
#endif
#include "common/elf_info.h"
#include "common/ntapi.h"
#include "common/path_util.h"
#include "common/polyfill_thread.h"
#include "common/scm_rev.h"
#include "common/singleton.h"
#include "common/version.h"
#include "core/file_format/psf.h"
#include "core/file_format/splash.h"
#include "core/file_format/trp.h"
#include "core/file_sys/fs.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/libc_internal/libc_internal.h"
#include "core/libraries/libs.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/np_trophy/np_trophy.h"
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
#endif

    // Create stdin/stdout/stderr
    Common::Singleton<FileSys::HandleTable>::Instance()->CreateStdHandles();

    // Defer until after logging is initialized.
    memory = Core::Memory::Instance();
    controller = Common::Singleton<Input::GameController>::Instance();
    linker = Common::Singleton<Core::Linker>::Instance();

    // Load renderdoc module.
    VideoCore::LoadRenderDoc();

    // Start the timer (Play Time)
#ifdef ENABLE_QT_GUI
    start_time = std::chrono::steady_clock::now();
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    QString filePath = QString::fromStdString((user_dir / "play_time.txt").string());
    QFile file(filePath);
    ASSERT_MSG(file.open(QIODevice::ReadWrite | QIODevice::Text),
               "Error opening or creating play_time.txt");
#endif
}

Emulator::~Emulator() {
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::saveMainWindow(config_dir / "config.toml");
}

void Emulator::Run(const std::filesystem::path& file, const std::vector<std::string> args) {
    const auto eboot_name = file.filename().string();
    auto game_folder = file.parent_path();
    if (const auto game_folder_name = game_folder.filename().string();
        game_folder_name.ends_with("-UPDATE")) {
        // If an executable was launched from a separate update directory,
        // use the base game directory as the game folder.
        const auto base_name = game_folder_name.substr(0, game_folder_name.size() - 7);
        const auto base_path = game_folder.parent_path() / base_name;
        if (std::filesystem::is_directory(base_path)) {
            game_folder = base_path;
        }
    }

    // Applications expect to be run from /app0 so mount the file's parent path as app0.
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Mount(game_folder, "/app0");
    // Certain games may use /hostapp as well such as CUSA001100
    mnt->Mount(game_folder, "/hostapp");

    auto& game_info = Common::ElfInfo::Instance();

    // Loading param.sfo file if exists
    std::string id;
    std::string title;
    std::string app_version;
    u32 fw_version;
    Common::PSFAttributes psf_attributes{};

    const auto param_sfo_path = mnt->GetHostPath("/app0/sce_sys/param.sfo");
    if (!std::filesystem::exists(param_sfo_path) || !Config::getSeparateLogFilesEnabled()) {
        Common::Log::Initialize();
        Common::Log::Start();
    }

    if (std::filesystem::exists(param_sfo_path)) {
        auto* param_sfo = Common::Singleton<PSF>::Instance();
        const bool success = param_sfo->Open(param_sfo_path);
        ASSERT_MSG(success, "Failed to open param.sfo");
        const auto content_id = param_sfo->GetString("CONTENT_ID");
        ASSERT_MSG(content_id.has_value(), "Failed to get CONTENT_ID");
        id = std::string(*content_id, 7, 9);

        if (Config::getSeparateLogFilesEnabled()) {
            Common::Log::Initialize(id + ".log");
            Common::Log::Start();
        }
        LOG_INFO(Loader, "Starting shadps4 emulator v{} ", Common::VERSION);
        LOG_INFO(Loader, "Revision {}", Common::g_scm_rev);
        LOG_INFO(Loader, "Branch {}", Common::g_scm_branch);
        LOG_INFO(Loader, "Description {}", Common::g_scm_desc);
        LOG_INFO(Loader, "Remote {}", Common::g_scm_remote_url);

        LOG_INFO(Config, "General LogType: {}", Config::getLogType());
        LOG_INFO(Config, "General isNeo: {}", Config::isNeoModeConsole());
        LOG_INFO(Config, "GPU isNullGpu: {}", Config::nullGpu());
        LOG_INFO(Config, "GPU shouldDumpShaders: {}", Config::dumpShaders());
        LOG_INFO(Config, "GPU vblankDivider: {}", Config::vblankDiv());
        LOG_INFO(Config, "Vulkan gpuId: {}", Config::getGpuId());
        LOG_INFO(Config, "Vulkan vkValidation: {}", Config::vkValidationEnabled());
        LOG_INFO(Config, "Vulkan vkValidationSync: {}", Config::vkValidationSyncEnabled());
        LOG_INFO(Config, "Vulkan vkValidationGpu: {}", Config::vkValidationGpuEnabled());
        LOG_INFO(Config, "Vulkan crashDiagnostics: {}", Config::getVkCrashDiagnosticEnabled());
        LOG_INFO(Config, "Vulkan hostMarkers: {}", Config::getVkHostMarkersEnabled());
        LOG_INFO(Config, "Vulkan guestMarkers: {}", Config::getVkGuestMarkersEnabled());
        LOG_INFO(Config, "Vulkan rdocEnable: {}", Config::isRdocEnabled());

        Libraries::NpTrophy::game_serial = id;
        const auto trophyDir =
            Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / id / "TrophyFiles";
        if (!std::filesystem::exists(trophyDir)) {
            TRP trp;
            if (!trp.Extract(game_folder, id)) {
                LOG_ERROR(Loader, "Couldn't extract trophies");
            }
        }
#ifdef ENABLE_QT_GUI
        MemoryPatcher::g_game_serial = id;

        // Timer for 'Play Time'
        QTimer* timer = new QTimer();
        QObject::connect(timer, &QTimer::timeout, [this, id]() {
            UpdatePlayTime(id);
            start_time = std::chrono::steady_clock::now();
        });
        timer->start(60000); // 60000 ms = 1 minute
#endif
        title = param_sfo->GetString("TITLE").value_or("Unknown title");
        LOG_INFO(Loader, "Game id: {} Title: {}", id, title);
        fw_version = param_sfo->GetInteger("SYSTEM_VER").value_or(0x4700000);
        app_version = param_sfo->GetString("APP_VER").value_or("Unknown version");
        LOG_INFO(Loader, "Fw: {:#x} App Version: {}", fw_version, app_version);
        if (const auto raw_attributes = param_sfo->GetInteger("ATTRIBUTE")) {
            psf_attributes.raw = *raw_attributes;
        }
        if (!args.empty()) {
            int argc = std::min<int>(args.size(), 32);
            for (int i = 0; i < argc; i++) {
                LOG_INFO(Loader, "Game argument {}: {}", i, args[i]);
            }
            if (args.size() > 32) {
                LOG_ERROR(Loader, "Too many game arguments, only passing the first 32");
            }
        }
    }

    const auto pic1_path = mnt->GetHostPath("/app0/sce_sys/pic1.png");
    if (std::filesystem::exists(pic1_path)) {
        auto* splash = Common::Singleton<Splash>::Instance();
        if (!splash->IsLoaded()) {
            if (!splash->Open(pic1_path)) {
                LOG_ERROR(Loader, "Game splash: unable to open file");
            }
        }
    }

    game_info.initialized = true;
    game_info.game_serial = id;
    game_info.title = title;
    game_info.app_ver = app_version;
    game_info.firmware_ver = fw_version & 0xFFF00000;
    game_info.raw_firmware_ver = fw_version;
    game_info.psf_attributes = psf_attributes;

    std::string game_title = fmt::format("{} - {} <{}>", id, title, app_version);
    std::string window_title = "";
    if (Common::isRelease) {
        window_title = fmt::format("shadPS4 v{} | {}", Common::VERSION, game_title);
    } else {
        std::string remote_url(Common::g_scm_remote_url);
        std::string remote_host;
        try {
            remote_host = remote_url.substr(19, remote_url.rfind('/') - 19);
        } catch (...) {
            remote_host = "unknown";
        }
        if (remote_host == "shadps4-emu" || remote_url.length() == 0) {
            window_title = fmt::format("shadPS4 v{} {} {} | {}", Common::VERSION,
                                       Common::g_scm_branch, Common::g_scm_desc, game_title);
        } else {
            window_title = fmt::format("shadPS4 v{} {}/{} {} | {}", Common::VERSION, remote_host,
                                       Common::g_scm_branch, Common::g_scm_desc, game_title);
        }
    }
    window = std::make_unique<Frontend::WindowSDL>(
        Config::getScreenWidth(), Config::getScreenHeight(), controller, window_title);

    g_window = window.get();

    const auto& mount_data_dir = Common::FS::GetUserPath(Common::FS::PathType::GameDataDir) / id;
    if (!std::filesystem::exists(mount_data_dir)) {
        std::filesystem::create_directory(mount_data_dir);
    }
    mnt->Mount(mount_data_dir, "/data"); // should just exist, manually create with game serial
    const auto& mount_temp_dir = Common::FS::GetUserPath(Common::FS::PathType::TempDataDir) / id;
    if (!std::filesystem::exists(mount_temp_dir)) {
        std::filesystem::create_directory(mount_temp_dir);
    }
    mnt->Mount(mount_temp_dir, "/temp0"); // called in app_content ==> stat/mkdir
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
    linker->LoadModule(eboot_path);

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
    constexpr std::array<SysModules, 11> ModulesToLoad{
        {{"libSceNgs2.sprx", &Libraries::Ngs2::RegisterlibSceNgs2},
         {"libSceUlt.sprx", nullptr},
         {"libSceJson.sprx", nullptr},
         {"libSceJson2.sprx", nullptr},
         {"libSceLibcInternal.sprx", &Libraries::LibcInternal::RegisterlibSceLibcInternal},
         {"libSceDiscMap.sprx", &Libraries::DiscMap::RegisterlibSceDiscMap},
         {"libSceRtc.sprx", &Libraries::Rtc::RegisterlibSceRtc},
         {"libSceCesCs.sprx", nullptr},
         {"libSceFont.sprx", nullptr},
         {"libSceFontFt.sprx", nullptr},
         {"libSceFreeTypeOt.sprx", nullptr}}};

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
