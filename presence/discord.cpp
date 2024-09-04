#include "discord.h"
#include "discord_rpc.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

/**
 * @brief Constructor for the Discord class.
 * @author Kevin Mora (morkev)
 *
 * Initializes the Discord class with the provided launch code (Discord application ID)
 * and sets the application_running_ flag to false.
 *
 * @param launch_code The Discord application client ID.
 */
Discord::Discord(const std::string &launch_code)
        : launch_code_(launch_code), application_running_(false), time_of_start_(0) {}

/**
 * @brief Initializes the Discord RPC connection.
 *
 * Sets up the event handlers for Discord Rich Presence and connects to Discord
 * using the provided launch code.
 */
void Discord::Initialize() {
    DiscordEventHandlers handlers{};
    Discord_Initialize(launch_code_.c_str(), &handlers, 1, nullptr);
}

/**
 * @brief Shuts down the Discord RPC connection.
 *
 * Terminates the connection to Discord and cleans up any active presence.
 */
void Discord::Shutdown() {
    Discord_Shutdown();
}

/**
 * @brief Checks if a process with the given name is running on the system.
 *
 * This function iterates over all running processes to check if a process
 * named `process_name` (e.g., `shadps4.exe`) is running.
 *
 * @param process_name The name of the process to check for.
 * @return true if the process is running, false otherwise.
 */
bool Discord::IsProcessRunning(const std::string &process_name) {
    for (const auto &entry : std::filesystem::directory_iterator("/proc")) {
        try {
            std::string pid = entry.path().filename();
            std::string cmd_path = "/proc/" + pid + "/comm";
            std::ifstream cmd_file(cmd_path);
            std::string cmd;
            std::getline(cmd_file, cmd);
            if (cmd == process_name) {
                return true;
            }
        } catch (...) {
            continue;
        }
    }
    return false;
}

/**
 * @brief Retrieves the current presence data for Discord.
 *
 * Determines the state, large image, and large image text based on whether
 * `shadps4.exe` is running. If the process is found, it updates the status
 * to indicate the application is running.
 *
 * @param state Reference to the string where the current state will be stored (e.g., "In ShadPS4").
 * @param large_image Reference to the string where the large image key will be stored.
 * @param large_text Reference to the string where the large image's tooltip text will be stored.
 */
void Discord::GetData(std::string &state, std::string &large_image, std::string &large_text) {
    if (IsProcessRunning("shadps4.exe")) {
        state = "In ShadPS4";
        large_image = "shadps4";
        large_text = "ShadPS4";
    } else {
        state = "Unknown";
        large_image = "default";
        large_text = "Unknown";
    }
}

/**
 * @brief Updates the Discord Rich Presence with the latest status.
 *
 * If the application `shadps4.exe` is running, this method updates the start time
 * and sends the current state, large image, and text to Discord.
 * If the application is closed, it clears the presence from Discord.
 */
void Discord::UpdatePresence() {
    std::string state, large_image, large_text;
    GetData(state, large_image, large_text);

    DiscordRichPresence presence{};
    presence.state = state.c_str();
    presence.large_image_key = large_image.c_str();
    presence.large_image_text = large_text.c_str();
    presence.start_timestamp = time_of_start_;

    if (state != "Unknown") {
        Discord_UpdatePresence(&presence);
        std::cout << "Discord Rich Presence updated.\n";
    } else {
        Discord_ClearPresence();
        std::cout << "Application closed. Presence cleared.\n";
    }
}

/**
 * @brief Runs the main loop for updating Discord Rich Presence.
 *
 * Continuously checks if the `shadps4.exe` process is running and updates the Discord
 * status accordingly. This function handles the timing of updates and stops the timer
 * when the application is closed.
 */
void Discord::Run() {
    Initialize();
    while (true) {
        try {
            std::string state, large_image, large_text;
            GetData(state, large_image, large_text);

            if (state != "Unknown") {
                if (!application_running_) {
                    application_running_ = true;
                    time_of_start_ = std::time(nullptr);  // Set start time when the application starts
                }
                UpdatePresence();
                std::this_thread::sleep_for(std::chrono::seconds(10));  // Check every 10 seconds
            } else {
                if (application_running_) {
                    application_running_ = false;
                    std::cout << "Application stopped.\n";
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Faster check if app is not running
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << ". Retrying...\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    Shutdown();
}