#pragma once

#include "Types.h"
#include <functional>
#include <vector>

class CAchievementStateManager
{
public:
    using StateChangeHandler = std::function<void(bool)>;

    CAchievementStateManager()
        : m_hardcoreModeEnabled(false)
        , m_saveStateAllowed(true)
    {
    }

    virtual ~CAchievementStateManager() = default;

    void EnableHardcoreMode(bool enable)
    {
        if(m_hardcoreModeEnabled != enable)
        {
            m_hardcoreModeEnabled = enable;
            m_saveStateAllowed = !enable;
            
            for(const auto& handler : m_stateChangeHandlers)
            {
                handler(enable);
            }
        }
    }

    bool IsHardcoreModeEnabled() const
    {
        return m_hardcoreModeEnabled;
    }

    bool IsSaveStateAllowed() const
    {
        return m_saveStateAllowed;
    }

    void RegisterStateChangeHandler(const StateChangeHandler& handler)
    {
        m_stateChangeHandlers.push_back(handler);
    }

    void OnSaveStateLoaded()
    {
        if(m_hardcoreModeEnabled)
        {
            // In hardcore mode, loading a save state disables achievements
            DisableAchievements();
        }
    }

    void OnSaveStateCreated()
    {
        if(m_hardcoreModeEnabled)
        {
            // In hardcore mode, creating a save state disables achievements
            DisableAchievements();
        }
    }

    void Reset()
    {
        // Keep hardcore mode setting but reset other states
        m_saveStateAllowed = !m_hardcoreModeEnabled;
        m_achievementsEnabled = true;
    }

    bool AreAchievementsEnabled() const
    {
        return m_achievementsEnabled;
    }

private:
    void DisableAchievements()
    {
        m_achievementsEnabled = false;
        // Notify about achievements being disabled
        for(const auto& handler : m_stateChangeHandlers)
        {
            handler(false);
        }
    }

    bool m_hardcoreModeEnabled;
    bool m_saveStateAllowed;
    bool m_achievementsEnabled = true;
    std::vector<StateChangeHandler> m_stateChangeHandlers;
};