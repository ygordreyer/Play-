#pragma once

#include "Types.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class CMemoryMap;

class CAchievementSystem
{
public:
    // Singleton management
    static CAchievementSystem* GetInstance();
    static void CreateInstance(CMemoryMap* memoryMap);
    static void DestroyInstance();

    // Achievement system events
    using AchievementUnlockedHandler = std::function<void(const std::string& achievementId, const std::string& title)>;
    using ProgressUpdatedHandler = std::function<void(const std::string& achievementId, float progress)>;
    using LeaderboardSubmittedHandler = std::function<void(const std::string& leaderboardId, uint32 score)>;

    struct ACHIEVEMENT_INFO
    {
        std::string id;
        std::string title;
        std::string description;
        uint32 points;
        bool unlocked;
        float progress;
    };

    virtual ~CAchievementSystem() = default;

    // System management
    virtual void Initialize() = 0;
    virtual void Reset() = 0;
    virtual void Update() = 0;

    // Achievement state
    virtual bool IsInitialized() const = 0;
    virtual bool IsHardcoreModeActive() const = 0;
    virtual void SetHardcoreMode(bool enabled) = 0;

    // Achievement info
    virtual std::vector<ACHIEVEMENT_INFO> GetAchievements() const = 0;
    virtual bool IsAchievementUnlocked(const std::string& id) const = 0;
    virtual float GetAchievementProgress(const std::string& id) const = 0;

    // Event registration
    virtual void RegisterUnlockHandler(const AchievementUnlockedHandler& handler) = 0;
    virtual void RegisterProgressHandler(const ProgressUpdatedHandler& handler) = 0;
    virtual void RegisterLeaderboardHandler(const LeaderboardSubmittedHandler& handler) = 0;

    // Memory management
    virtual void AddMemoryWatch(uint32 address, uint32 size, const std::function<void(uint32, uint32)>& callback) = 0;
    virtual void RemoveMemoryWatch(uint32 address) = 0;
    virtual void ClearMemoryWatches() = 0;

    // Save state management
    virtual bool IsSaveStateAllowed() const = 0;
    virtual void OnSaveStateLoaded() = 0;
    virtual void OnSaveStateCreated() = 0;

protected:
    CAchievementSystem() = default;
};