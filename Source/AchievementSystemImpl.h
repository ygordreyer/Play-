#pragma once

#include "Types.h"
#include "AchievementSystem.h"
#include "AchievementMemoryMonitor.h"
#include "AchievementStateManager.h"
#include "AchievementRcheevosAdapter.h"
#include "AchievementsConfig.h"
#include <memory>
#include <mutex>

class CAchievementSystemImpl : public CAchievementSystem
{
public:
    CAchievementSystemImpl(CMemoryMap& memoryMap)
        : m_memoryMap(memoryMap)
        , m_stateManager(std::make_unique<CAchievementStateManager>())
        , m_rcheevosAdapter(std::make_unique<CAchievementRcheevosAdapter>(memoryMap))
        , m_initialized(false)
    {
    }

    ~CAchievementSystemImpl()
    {
        if(m_memoryMonitor)
        {
            m_memoryMonitor->Shutdown();
        }
    }

    // System management
    void Initialize() override
    {
        auto lock = GetLock();

        if (m_initialized)
            return;

        try
        {
            // Set up state change handler
            m_stateManager->RegisterStateChangeHandler(
                [this](bool enabled)
                {
                    auto handler_lock = GetLock();
                    if (!enabled)
                    {
                        if (m_memoryMonitor)
                        {
                            m_memoryMonitor->Shutdown();
                            m_memoryMonitor.reset();
                        }
                        m_rcheevosAdapter->UnloadGame();
                    }
                });

            // Initialize rcheevos adapter with credentials if available
            auto config = CAchievementsConfig::LoadConfig();
            if (!config)
            {
                printf("Failed to load achievements config\n");
                return;
            }

            auto auth = config->GetAuth();
            if (auth.username.empty() || auth.token.empty())
            {
                printf("No achievement credentials found\n");
                return;
            }

            if (!m_rcheevosAdapter->Initialize(auth.username, auth.token))
            {
                printf("Failed to initialize rcheevos adapter\n");
                return;
            }

            m_initialized = true;
        }
        catch (const std::exception& e)
        {
            printf("Achievement initialization error: %s\n", e.what());
            
            // Clean up on failure
            if (m_memoryMonitor)
            {
                m_memoryMonitor->Shutdown();
                m_memoryMonitor.reset();
            }
            m_rcheevosAdapter->UnloadGame();
            m_initialized = false;
        }
    }

    void GameChanged(const std::string& path, uint32 crc)
    {
        auto lock = GetLock();

        try
        {
            // Basic state validation
            if (!m_initialized || !m_stateManager->AreAchievementsEnabled())
            {
                printf("Achievements: System not initialized or disabled\n");
                return;
            }

            printf("Achievements: Processing game change for '%s'\n", path.c_str());

            // Clean up previous game state
            if (m_memoryMonitor)
            {
                printf("Achievements: Shutting down previous memory monitor\n");
                m_memoryMonitor->Shutdown();
                m_memoryMonitor.reset();
            }

            // Unload current game before loading new one
            m_rcheevosAdapter->UnloadGame();

            // Skip loading if no valid game
            if (path.empty() || crc == 0)
            {
                printf("Achievements: Invalid game path or CRC\n");
                return;
            }

            // Let rcheevos adapter handle game state first
            printf("Achievements: Loading game in rcheevos adapter\n");
            if (!m_rcheevosAdapter->LoadGame(path, crc))
            {
                printf("Achievements: Failed to load game in rcheevos adapter\n");
                return;
            }

            // Wait for game to be fully loaded and validated
            if (!m_rcheevosAdapter->HasActiveGame())
            {
                printf("Achievements: Game not active in rcheevos adapter\n");
                return;
            }

            // Check if game has achievements before proceeding
            if (!m_rcheevosAdapter->HasAchievements())
            {
                printf("Achievements: Game has no achievements, skipping memory monitor\n");
                return;
            }

            // Create memory monitor only after all validations pass
            printf("Achievements: Creating memory monitor\n");
            CreateMemoryMonitor();
            
            if (!m_memoryMonitor || !m_memoryMonitor->IsInitialized())
            {
                printf("Achievements: Failed to initialize memory monitor\n");
                m_rcheevosAdapter->UnloadGame();
                return;
            }

            printf("Achievements: Game change processed successfully\n");
        }
        catch (const std::exception& e)
        {
            printf("Achievements: Error in GameChanged: %s\n", e.what());
            
            // Clean up on error
            if (m_memoryMonitor)
            {
                m_memoryMonitor->Shutdown();
                m_memoryMonitor.reset();
            }
            m_rcheevosAdapter->UnloadGame();
        }
    }

    void Reset() override
    {
        auto lock = GetLock();

        // Validate state before reset
        if (!m_initialized)
            return;

        try
        {
            // Clean up memory monitor first
            if (m_memoryMonitor)
            {
                m_memoryMonitor->Shutdown();
                m_memoryMonitor->ClearWatches();
                m_memoryMonitor.reset();
            }

            // Reset state managers
            m_stateManager->Reset();
            m_rcheevosAdapter->Reset();

            // Only recreate memory monitor if:
            // 1. Game is active
            // 2. Game has achievements
            // 3. Achievements are enabled
            if (m_rcheevosAdapter->HasActiveGame() &&
                m_rcheevosAdapter->HasAchievements() &&
                m_stateManager->AreAchievementsEnabled())
            {
                CreateMemoryMonitor();
                if (!m_memoryMonitor)
                {
                    // Failed to create memory monitor, reset game state
                    m_rcheevosAdapter->UnloadGame();
                }
            }
        }
        catch (const std::exception& e)
        {
            // Log error and ensure cleanup
            printf("Achievement reset error: %s\n", e.what());
            if (m_memoryMonitor)
            {
                m_memoryMonitor->Shutdown();
                m_memoryMonitor.reset();
            }
            m_rcheevosAdapter->UnloadGame();
        }
    }

    void Update() override
    {
        auto lock = GetLock();

        // Basic state validation
        if (!m_initialized)
            return;

        try
        {
            // Process idle updates regardless of state
            m_rcheevosAdapter->ProcessIdle();

            // Don't update achievements until game is properly loaded
            if (!m_rcheevosAdapter->HasActiveGame())
                return;

            // Only process frame updates if:
            // 1. Achievements are enabled
            // 2. Game has achievements
            // 3. Memory monitor is active
            if (m_stateManager->AreAchievementsEnabled() &&
                m_rcheevosAdapter->HasAchievements())
            {
                // Update memory monitor first
                if (m_memoryMonitor)
                {
                    m_memoryMonitor->Update();
                }

                // Then process achievement frame
                m_rcheevosAdapter->ProcessFrame();
            }
        }
        catch (const std::exception& e)
        {
            // Log error and reset state
            printf("Achievement update error: %s\n", e.what());
            Reset();
        }
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
        m_rcheevosAdapter->RegisterUnlockHandler(handler);
    }

    void RegisterProgressHandler(const ProgressUpdatedHandler& handler) override
    {
        m_progressHandlers.push_back(handler);
        m_rcheevosAdapter->RegisterProgressHandler(handler);
    }

    void RegisterLeaderboardHandler(const LeaderboardSubmittedHandler& handler) override
    {
        m_leaderboardHandlers.push_back(handler);
        m_rcheevosAdapter->RegisterLeaderboardHandler(handler);
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
        auto lock = GetLock();
        try
        {
            // Validate state before processing
            if (!m_initialized || !m_stateManager->AreAchievementsEnabled())
                return;

            // Notify state manager first
            m_stateManager->OnSaveStateLoaded();

            // Reset memory monitor to ensure clean state
            if (m_memoryMonitor)
            {
                m_memoryMonitor->Shutdown();
                m_memoryMonitor->Initialize();
            }
        }
        catch (const std::exception& e)
        {
            printf("Error in OnSaveStateLoaded: %s\n", e.what());
            Reset();
        }
    }

    void OnSaveStateCreated() override
    {
        auto lock = GetLock();
        try
        {
            // Validate state before processing
            if (!m_initialized || !m_stateManager->AreAchievementsEnabled())
                return;

            // Notify state manager
            m_stateManager->OnSaveStateCreated();

            // Ensure memory monitor is in sync
            if (m_memoryMonitor && m_memoryMonitor->IsInitialized())
            {
                m_memoryMonitor->Update();
            }
        }
        catch (const std::exception& e)
        {
            printf("Error in OnSaveStateCreated: %s\n", e.what());
            Reset();
        }
    }

private:
    std::recursive_mutex m_mutex;
    std::unique_lock<std::recursive_mutex> GetLock()
    {
        return std::unique_lock(m_mutex);
    }

    CMemoryMap& m_memoryMap;
    std::unique_ptr<CAchievementMemoryMonitor> m_memoryMonitor;
    std::unique_ptr<CAchievementStateManager> m_stateManager;
    std::unique_ptr<CAchievementRcheevosAdapter> m_rcheevosAdapter;
    bool m_initialized;

    std::vector<AchievementUnlockedHandler> m_unlockHandlers;
    std::vector<ProgressUpdatedHandler> m_progressHandlers;
    std::vector<LeaderboardSubmittedHandler> m_leaderboardHandlers;

    void CreateMemoryMonitor()
    {
        if(!m_memoryMonitor)
        {
            m_memoryMonitor = std::make_unique<CAchievementMemoryMonitor>(m_memoryMap);
            if(!m_memoryMonitor->Initialize())
            {
                m_memoryMonitor.reset();
            }
        }
    }
};