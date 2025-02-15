#pragma once

#include "Types.h"
#include "AchievementSystem.h"
#include "AchievementMemoryMonitor.h"
#include "AchievementStateManager.h"
#include "AchievementRcheevosAdapter.h"
#include <memory>

class CAchievementSystemImpl : public CAchievementSystem
{
public:
    CAchievementSystemImpl(CMemoryMap& memoryMap)
        : m_memoryMonitor(std::make_unique<CAchievementMemoryMonitor>(memoryMap))
        , m_stateManager(std::make_unique<CAchievementStateManager>())
        , m_rcheevosAdapter(std::make_unique<CAchievementRcheevosAdapter>())
        , m_initialized(false)
    {
    }

    ~CAchievementSystemImpl() = default;

    // System management
    void Initialize() override
    {
        if(m_initialized) return;

        // Set up state change handler
        m_stateManager->RegisterStateChangeHandler(
            [this](bool enabled)
            {
                if(!enabled)
                {
                    // Clear memory watches when achievements are disabled
                    m_memoryMonitor->ClearWatches();
                }
            });

        m_initialized = true;
    }

    void Reset() override
    {
        m_memoryMonitor->ClearWatches();
        m_stateManager->Reset();
        m_rcheevosAdapter->Reset();
    }

    void Update() override
    {
        if(!m_initialized || !m_stateManager->AreAchievementsEnabled()) return;

        m_memoryMonitor->Update();
        m_rcheevosAdapter->Update();
    }

    // Achievement state
    bool IsInitialized() const override
    {
        return m_initialized;
    }

    bool IsHardcoreModeActive() const override
    {
        return m_stateManager->IsHardcoreModeEnabled();
    }

    void SetHardcoreMode(bool enabled) override
    {
        m_stateManager->EnableHardcoreMode(enabled);
    }

    // Achievement info
    std::vector<ACHIEVEMENT_INFO> GetAchievements() const override
    {
        return m_rcheevosAdapter->GetAchievements();
    }

    bool IsAchievementUnlocked(const std::string& id) const override
    {
        return m_rcheevosAdapter->IsAchievementUnlocked(id);
    }

    float GetAchievementProgress(const std::string& id) const override
    {
        return m_rcheevosAdapter->GetAchievementProgress(id);
    }

    // Event registration
    void RegisterUnlockHandler(const AchievementUnlockedHandler& handler) override
    {
        m_unlockHandlers.push_back(handler);
    }

    void RegisterProgressHandler(const ProgressUpdatedHandler& handler) override
    {
        m_progressHandlers.push_back(handler);
    }

    void RegisterLeaderboardHandler(const LeaderboardSubmittedHandler& handler) override
    {
        m_leaderboardHandlers.push_back(handler);
    }

    // Memory management
    void AddMemoryWatch(uint32 address, uint32 size, const std::function<void(uint32, uint32)>& callback) override
    {
        if(!m_initialized || !m_stateManager->AreAchievementsEnabled()) return;
        m_memoryMonitor->AddWatch(address, size, callback);
    }

    void RemoveMemoryWatch(uint32 address) override
    {
        m_memoryMonitor->RemoveWatch(address);
    }

    void ClearMemoryWatches() override
    {
        m_memoryMonitor->ClearWatches();
    }

    // Save state management
    bool IsSaveStateAllowed() const override
    {
        return m_stateManager->IsSaveStateAllowed();
    }

    void OnSaveStateLoaded() override
    {
        m_stateManager->OnSaveStateLoaded();
    }

    void OnSaveStateCreated() override
    {
        m_stateManager->OnSaveStateCreated();
    }

private:
    std::unique_ptr<CAchievementMemoryMonitor> m_memoryMonitor;
    std::unique_ptr<CAchievementStateManager> m_stateManager;
    std::unique_ptr<CAchievementRcheevosAdapter> m_rcheevosAdapter;
    bool m_initialized;

    std::vector<AchievementUnlockedHandler> m_unlockHandlers;
    std::vector<ProgressUpdatedHandler> m_progressHandlers;
    std::vector<LeaderboardSubmittedHandler> m_leaderboardHandlers;
};