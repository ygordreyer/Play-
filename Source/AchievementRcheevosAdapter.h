#pragma once

#include "Types.h"
#include "AchievementSystem.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations to avoid including rcheevos headers
struct rc_runtime_t;
struct rc_client_t;

class CAchievementRcheevosAdapter
{
public:
    CAchievementRcheevosAdapter();
    virtual ~CAchievementRcheevosAdapter();

    bool Initialize(const std::string& username, const std::string& token);
    void Reset();
    void Update();

    // Game management
    bool LoadGame(const std::string& path, uint32 crc);
    void UnloadGame();

    // Achievement info
    std::vector<CAchievementSystem::ACHIEVEMENT_INFO> GetAchievements() const;
    bool IsAchievementUnlocked(const std::string& id) const;
    float GetAchievementProgress(const std::string& id) const;

    // Memory access callbacks for rcheevos
    static uint8_t PeekByte(uint32_t address, uint32_t flags, void* userdata);
    static void PokeByte(uint32_t address, uint8_t value, uint32_t flags, void* userdata);

    // Event handlers
    void OnAchievementUnlocked(const std::string& id);
    void OnProgressUpdated(const std::string& id, float progress);
    void OnLeaderboardSubmitted(const std::string& id, uint32 score);

private:
    void ProcessCallbacks();
    void UpdateAchievements();
    void UpdateLeaderboards();

    std::unique_ptr<rc_runtime_t, void(*)(rc_runtime_t*)> m_runtime;
    std::unique_ptr<rc_client_t, void(*)(rc_client_t*)> m_client;
    bool m_gameLoaded;
    
    // Cached achievement data
    struct ACHIEVEMENT_CACHE
    {
        std::string id;
        std::string title;
        std::string description;
        uint32 points;
        bool unlocked;
        float progress;
    };
    std::vector<ACHIEVEMENT_CACHE> m_achievementCache;

    // Event handlers
    CAchievementSystem::AchievementUnlockedHandler m_unlockHandler;
    CAchievementSystem::ProgressUpdatedHandler m_progressHandler;
    CAchievementSystem::LeaderboardSubmittedHandler m_leaderboardHandler;
};