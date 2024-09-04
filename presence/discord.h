#ifndef DISCORD_H
#define DISCORD_H

#include <string>
#include "discord_rpc.h"

/**
 * @class Discord
 * @brief Manages Discord Rich Presence for the shadPS4 Emulator.
 * @author Kevin Mora (morkev)
 *
 * The Discord class is responsible for managing Discord Rich Presence updates,
 * detecting if the `shadps4.exe` process is running, and updating Discord status accordingly.
 * It also handles connecting and disconnecting from the Discord RPC service.
 *
 * @param launch_code_ Discord application client ID.
 * @param application_running_ Tracks if the application (`shadps4.exe`) is running.
 * @param time_of_start_ Tracks the time when the application starts for presence timestamping.
 */
class Discord {
private:
    std::string launch_code_;
    bool application_running_;
    time_t time_of_start_;

    /**
     * @brief Checks if a specific process is running on the system.
     *
     * This method searches for a running process by its name.
     *
     * @param process_name The name of the process to check (e.g., `shadps4.exe`).
     * @return true if the process is running, false otherwise.
     */
    bool IsProcessRunning(const std::string &process_name);

    /**
     * @brief Retrieves the current status and presence data for Discord.
     *
     * This method sets the status, large image, and text details based on whether
     * `shadps4.exe` is currently running.
     *
     * @param state Output parameter for the current state (e.g., "In ShadPS4").
     * @param large_image Output parameter for the large image to display (e.g., "shadps4").
     * @param large_text Output parameter for the large image's text (e.g., "ShadPS4").
     */
    void GetData(std::string &state, std::string &large_image, std::string &large_text);

public:
    /**
     * @brief Constructor for the Discord class.
     *
     * Initializes the Discord class with the specified launch code for Discord RPC.
     *
     * @param launch_code Discord application client ID.
     */
    explicit Discord(const std::string &launch_code);

    void UpdatePresence();
    void Run();
    void Initialize();
    void Shutdown();
};

#endif