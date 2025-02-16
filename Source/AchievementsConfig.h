#pragma once

#include "deps/Framework/include/Types.h"
#include "deps/Framework/include/Config.h"
#include <string>

// Preference names
#define PREF_ACHIEVEMENTS_ENABLED             ("achievements.enabled")
#define PREF_ACHIEVEMENTS_HARDCOREMODE       ("achievements.hardcoremode")
#define PREF_ACHIEVEMENTS_ENCOREMODE         ("achievements.encoremode")
#define PREF_ACHIEVEMENTS_SPECTATORMODE      ("achievements.spectatormode")
#define PREF_ACHIEVEMENTS_UNOFFICIALMODE     ("achievements.unofficialmode")
#define PREF_ACHIEVEMENTS_NOTIFICATIONS      ("achievements.notifications")
#define PREF_ACHIEVEMENTS_LEADERBOARD_NOTIF  ("achievements.leaderboardnotifications")
#define PREF_ACHIEVEMENTS_SOUNDEFFECTS       ("achievements.soundeffects")
#define PREF_ACHIEVEMENTS_OVERLAYS           ("achievements.overlays")
#define PREF_ACHIEVEMENTS_NOTIF_DURATION     ("achievements.notifications.duration")
#define PREF_ACHIEVEMENTS_LEADERBOARD_DUR    ("achievements.leaderboards.duration")
#define PREF_ACHIEVEMENTS_TOKEN              ("achievements.token")
#define PREF_ACHIEVEMENTS_USERNAME           ("achievements.username")
#define PREF_ACHIEVEMENTS_POINTS             ("achievements.points")
#define PREF_ACHIEVEMENTS_UNREAD             ("achievements.unreadmessages")

class CAchievementsConfig : public Framework::CConfig
{
public:
    struct Settings {
        bool enabled = false;
        bool hardcoreMode = false;
        bool encoreMode = false;
        bool spectatorMode = false;
        bool unofficialTestMode = false;
        
        // UI settings
        bool notifications = true;
        bool leaderboardNotifications = true;
        bool soundEffects = true;
        bool overlays = true;
        
        // Durations
        uint32 notificationsDuration = 3000;  // 3 seconds
        uint32 leaderboardsDuration = 3000;   // 3 seconds
    };
    
    struct Auth {
        std::string token;
        std::string username;
        uint32 points = 0;
        uint32 unreadMessages = 0;
    };

    CAchievementsConfig(const Framework::CConfig::PathType& path);
    virtual ~CAchievementsConfig() = default;

    // Global settings
    static Framework::CConfig::PathType GetConfigPath();
    static std::unique_ptr<CAchievementsConfig> LoadConfig();

    // Per-game settings
    static bool IsValidGameId(const std::string& gameId);
    static Framework::CConfig::PathType GetGameConfigPath();
    static Framework::CConfig::PathType GetGameConfig(const std::string& gameId);
    static std::unique_ptr<CAchievementsConfig> LoadGameConfig(const std::string& gameId);

    // Settings access
    Settings GetSettings();
    void SetSettings(const Settings&);
    
    // Authentication
    Auth GetAuth();
    void SetAuth(const Auth&);

private:
    static const char* CONFIG_PATH;
    static const char* GAME_CONFIG_PATH;
};