#ifndef DISCORD_RPC_H
#define DISCORD_RPC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct DiscordRichPresence
 * @brief Struct representing Discord Rich Presence data.
 * @autor Kevin Mora (morkev)
 *
 * This struct holds various fields that define the Discord Rich Presence status,
 * including the state, timestamps, images, party information, and secrets for joining/spectating games.
 *
 * @param state The main status text for the Rich Presence (e.g., "In ShadPS4").
 * @param details Additional details for the Rich Presence.
 * @param start_timestamp The start time for the current activity (Unix timestamp).
 * @param end_timestamp The end time for the current activity (Unix timestamp).
 * @param large_image_key The key for the large image shown in Discord.
 * @param large_image_text The tooltip text for the large image.
 * @param small_image_key The key for the small image shown in Discord.
 * @param small_image_text The tooltip text for the small image.
 * @param party_id Unique identifier for the player's party.
 * @param party_size The number of people in the party.
 * @param party_max The maximum size of the party.
 * @param match_secret Secret for joining a multiplayer match.
 * @param join_secret Secret for joining a friend's game.
 * @param spectate_secret Secret for spectating a friend's game.
 * @param instance Whether or not the game is an instance.
*/
typedef struct DiscordRichPresence {
    const char* state;
    const char* details;
    int64_t start_timestamp;
    int64_t end_timestamp;
    const char* large_image_key;
    const char* large_image_text;
    const char* small_image_key;
    const char* small_image_text;
    const char* party_id;
    int party_size;
    int party_max;
    const char* match_secret;
    const char* join_secret;
    const char* spectate_secret;
    int8_t instance;
} DiscordRichPresence;

/**
 * @struct DiscordEventHandlers
 * @brief Struct representing event handlers for Discord Rich Presence.
 *
 * This struct defines function pointers for various Discord RPC events,
 * such as when the presence is ready, when the connection is disconnected,
 * and when there are errors or game join requests.
 *
 * @param ready Callback function when Discord RPC is ready.
 * @param disconnected Callback function when Discord RPC is disconnected.
 * @param errored Callback function for handling errors.
 * @param join_game Callback function for when a join game request is received.
 * @param spectate_game Callback function for when a spectate game request is received.
 * @param join_request Callback function for handling a join request.
 */
typedef struct DiscordEventHandlers {
    void (*ready)(void);
    void (*disconnected)(int errcode, const char* message);
    void (*errored)(int errcode, const char* message);
    void (*join_game)(const char* join_secret);
    void (*spectate_game)(const char* spectate_secret);
    void (*join_request)(const char* join_request);
} DiscordEventHandlers;

/**
 * @brief Initializes the Discord Rich Presence.
 *
 * This function sets up the Discord Rich Presence with the provided application ID,
 * event handlers, and optional Steam ID.
 *
 * @param application_id The Discord application ID.
 * @param handlers Pointer to the Discord event handlers.
 * @param auto_register Whether or not to auto-register the presence.
 * @param optional_steam_id Optional Steam ID for Steam integration.
 */
void Discord_Initialize(
        const char* application_id, DiscordEventHandlers* handlers,
        int auto_register, const char* optional_steam_id);

/**
 * @brief Shuts down the Discord Rich Presence.
 *
 * This function disconnects the Rich Presence from Discord and shuts it down.
 */
void Discord_Shutdown(void);

/**
 * @brief Updates the Discord Rich Presence with new information.
 *
 * This function updates the current Rich Presence data (e.g., state, timestamps, images).
 *
 * @param presence Pointer to a `DiscordRichPresence` struct containing the new data.
 */
void Discord_UpdatePresence(const DiscordRichPresence* presence);

/**
 * @brief Clears the current Discord Rich Presence.
 *
 * This function removes any active Rich Presence information from Discord.
 */
void Discord_ClearPresence(void);

#ifdef __cplusplus
}
#endif

#endif