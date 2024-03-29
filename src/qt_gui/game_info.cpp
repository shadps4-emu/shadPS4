#include <future>
#include <thread>
#include "game_info.h"

void GameInfoClass::GetGameInfo() {
    QString installDir = m_gui_settings->GetValue(gui::settings_install_dir).toString();
    std::filesystem::path parent_folder(installDir.toStdString());
    std::vector<std::string> filePaths;
    for (const auto& dir : std::filesystem::directory_iterator(parent_folder)) {
        if (dir.is_directory()) {
            filePaths.push_back(dir.path().string());
        }
    }
    std::vector<std::future<GameInfo>> futures;

    for (const auto& filePath : filePaths) {
        futures.emplace_back(std::async(std::launch::async, readGameInfo, filePath));
    }

    for (auto& future : futures) {
        m_games.push_back(future.get());
    }
    std::sort(m_games.begin(), m_games.end(), CompareStrings);
}