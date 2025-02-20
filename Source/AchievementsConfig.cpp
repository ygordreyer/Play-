#include "AchievementsConfig.h"
#include <filesystem>

const char* CAchievementsConfig::CONFIG_PATH = "achievements.config";
const char* CAchievementsConfig::GAME_CONFIG_PATH = "achievements/games/";

CAchievementsConfig::CAchievementsConfig(const Framework::CConfig::PathType& path)
    : CConfig(path)
{
    // Register all preferences with default values
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_ENABLED, false);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_HARDCOREMODE, false);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_ENCOREMODE, false);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_SPECTATORMODE, false);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_UNOFFICIALMODE, false);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_NOTIFICATIONS, true);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_LEADERBOARD_NOTIF, true);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_SOUNDEFFECTS, true);
    RegisterPreferenceBoolean(PREF_ACHIEVEMENTS_OVERLAYS, true);
    RegisterPreferenceInteger(PREF_ACHIEVEMENTS_NOTIF_DURATION, 3000);
    RegisterPreferenceInteger(PREF_ACHIEVEMENTS_LEADERBOARD_DUR, 3000);
    RegisterPreferenceString(PREF_ACHIEVEMENTS_TOKEN, "");
    RegisterPreferenceString(PREF_ACHIEVEMENTS_USERNAME, "");
    RegisterPreferenceInteger(PREF_ACHIEVEMENTS_POINTS, 0);
    RegisterPreferenceInteger(PREF_ACHIEVEMENTS_UNREAD, 0);
}

Framework::CConfig::PathType CAchievementsConfig::GetConfigPath()
{
    return Framework::CConfig::PathType(CONFIG_PATH);
}

std::unique_ptr<CAchievementsConfig> CAchievementsConfig::LoadConfig()
{
    auto configPath = GetConfigPath();
    return std::make_unique<CAchievementsConfig>(configPath);
}

bool CAchievementsConfig::IsValidGameId(const std::string& gameId)
{
    if(gameId.empty()) return false;
    return std::all_of(gameId.begin(), gameId.end(), [](char c) {
        return std::isalnum(c) || c == '-' || c == '_';
    });
}

Framework::CConfig::PathType CAchievementsConfig::GetGameConfigPath()
{
    return Framework::CConfig::PathType(GAME_CONFIG_PATH);
}

Framework::CConfig::PathType CAchievementsConfig::GetGameConfig(const std::string& gameId)
{
    if(!IsValidGameId(gameId)) return Framework::CConfig::PathType();
    auto path = GetGameConfigPath();
    path /= (gameId + ".config");
    return path;
}

std::unique_ptr<CAchievementsConfig> CAchievementsConfig::LoadGameConfig(const std::string& gameId)
{
    auto configPath = GetGameConfig(gameId);
    if(configPath.empty()) return nullptr;
    return std::make_unique<CAchievementsConfig>(configPath);
}

CAchievementsConfig::Settings CAchievementsConfig::GetSettings()
{
    Settings settings;
    settings.enabled = GetPreferenceBoolean(PREF_ACHIEVEMENTS_ENABLED);
    settings.hardcoreMode = GetPreferenceBoolean(PREF_ACHIEVEMENTS_HARDCOREMODE);
    settings.encoreMode = GetPreferenceBoolean(PREF_ACHIEVEMENTS_ENCOREMODE);
    settings.spectatorMode = GetPreferenceBoolean(PREF_ACHIEVEMENTS_SPECTATORMODE);
    settings.unofficialTestMode = GetPreferenceBoolean(PREF_ACHIEVEMENTS_UNOFFICIALMODE);
    settings.notifications = GetPreferenceBoolean(PREF_ACHIEVEMENTS_NOTIFICATIONS);
    settings.leaderboardNotifications = GetPreferenceBoolean(PREF_ACHIEVEMENTS_LEADERBOARD_NOTIF);
    settings.soundEffects = GetPreferenceBoolean(PREF_ACHIEVEMENTS_SOUNDEFFECTS);
    settings.overlays = GetPreferenceBoolean(PREF_ACHIEVEMENTS_OVERLAYS);
    settings.notificationsDuration = GetPreferenceInteger(PREF_ACHIEVEMENTS_NOTIF_DURATION);
    settings.leaderboardsDuration = GetPreferenceInteger(PREF_ACHIEVEMENTS_LEADERBOARD_DUR);
    return settings;
}

void CAchievementsConfig::SetSettings(const Settings& settings)
{
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_ENABLED, settings.enabled);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_HARDCOREMODE, settings.hardcoreMode);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_ENCOREMODE, settings.encoreMode);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_SPECTATORMODE, settings.spectatorMode);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_UNOFFICIALMODE, settings.unofficialTestMode);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_NOTIFICATIONS, settings.notifications);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_LEADERBOARD_NOTIF, settings.leaderboardNotifications);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_SOUNDEFFECTS, settings.soundEffects);
    SetPreferenceBoolean(PREF_ACHIEVEMENTS_OVERLAYS, settings.overlays);
    SetPreferenceInteger(PREF_ACHIEVEMENTS_NOTIF_DURATION, settings.notificationsDuration);
    SetPreferenceInteger(PREF_ACHIEVEMENTS_LEADERBOARD_DUR, settings.leaderboardsDuration);
    Save();
}

CAchievementsConfig::Auth CAchievementsConfig::GetAuth()
{
    Auth auth;
    auth.token = GetPreferenceString(PREF_ACHIEVEMENTS_TOKEN);
    auth.username = GetPreferenceString(PREF_ACHIEVEMENTS_USERNAME);
    auth.points = GetPreferenceInteger(PREF_ACHIEVEMENTS_POINTS);
    auth.unreadMessages = GetPreferenceInteger(PREF_ACHIEVEMENTS_UNREAD);
    return auth;
}

void CAchievementsConfig::SetAuth(const Auth& auth)
{
    SetPreferenceString(PREF_ACHIEVEMENTS_TOKEN, auth.token.c_str());
    SetPreferenceString(PREF_ACHIEVEMENTS_USERNAME, auth.username.c_str());
    SetPreferenceInteger(PREF_ACHIEVEMENTS_POINTS, auth.points);
    SetPreferenceInteger(PREF_ACHIEVEMENTS_UNREAD, auth.unreadMessages);
    Save();
}