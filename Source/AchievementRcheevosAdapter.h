#pragma once

#include "../../Framework/include/Types.h"
#include "AchievementSystem.h"
#include "../deps/rcheevos/include/rc_client.h"
#include "../deps/rcheevos/include/rc_runtime.h"
#include "../deps/rcheevos/src/rhash/md5.h"

// Forward declarations for rcheevos types
struct rc_client_t;
struct rc_client_async_handle_t;
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

// Forward declarations
class CAchievementMemoryMonitor;
class CMemoryMap;

class CAchievementRcheevosAdapter
{
public:
    CAchievementRcheevosAdapter(CMemoryMap& memoryMap);
    virtual ~CAchievementRcheevosAdapter();

    bool Initialize(const std::string& username, const std::string& token);
    void Reset();
    void Update();

    // Game management
    bool LoadGame(const std::string& path, uint32 crc);
    void UnloadGame();
    bool HasActiveGame() const { return m_gameLoaded && m_client != nullptr; }
    bool HasAchievements() const { return !m_achievementCache.empty(); }

    // Achievement info
    std::vector<CAchievementSystem::ACHIEVEMENT_INFO> GetAchievements() const;
    bool IsAchievementUnlocked(const std::string& id) const;
    float GetAchievementProgress(const std::string& id) const;

    // Frame processing
    void ProcessIdle();  // Process non-achievement updates (e.g. rich presence)
    void ProcessFrame(); // Process achievement and leaderboard updates

    // Memory access callbacks for rcheevos
    static uint8_t PeekByte(uint32_t address, uint32_t flags, void* userdata);
    static void PokeByte(uint32_t address, uint8_t value, uint32_t flags, void* userdata);

    // Event handlers
    void OnAchievementUnlocked(const std::string& id, const std::string& title);
    void OnProgressUpdated(const std::string& id, float progress);
    void OnLeaderboardSubmitted(const std::string& id, uint32 score);

    // Event registration
    void RegisterUnlockHandler(const CAchievementSystem::AchievementUnlockedHandler& handler);
    void RegisterProgressHandler(const CAchievementSystem::ProgressUpdatedHandler& handler);
    void RegisterLeaderboardHandler(const CAchievementSystem::LeaderboardSubmittedHandler& handler);

private:
    static void LoadGameCallback(int result, const char* error_message, rc_client_t* client, void* userdata);
    void ProcessCallbacks();
    void UpdateAchievements();
    void UpdateLeaderboards();

    std::unique_ptr<rc_client_t, void(*)(rc_client_t*)> m_client;
    std::unique_ptr<CAchievementMemoryMonitor> m_memoryMonitor;
    CMemoryMap& m_memoryMap;
    bool m_gameLoaded;
    
    // Async request tracking
    rc_client_async_handle_t* m_loadGameRequest;
    rc_client_async_handle_t* m_loginRequest;
    static constexpr float REQUEST_TIMEOUT = 60.0f;  // 60 seconds timeout
    static constexpr uint32 MAX_RETRIES = 3;
    uint32 m_requestRetries;
    std::string m_currentHash;  // Current game hash for retries
    
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