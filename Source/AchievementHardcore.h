#pragma once

#include "Types.h"
#include <vector>
#include <functional>

class CAchievementMemory;

class CAchievementHardcore
{
public:
    typedef std::function<void(bool)> HardcoreModeCallback;

    CAchievementHardcore(CAchievementMemory& memory);
    virtual ~CAchievementHardcore() = default;

    // Hardcore mode control
    void Enable(bool enable);
    bool IsEnabled() const { return m_enabled; }
    void AddModeChangeCallback(const HardcoreModeCallback& callback);

    // Save state management
    bool ValidateSaveStateOperation(const char* operation);

    // Memory protection
    void AddProtectedRegion(uint32 start, uint32 size);
    void ClearProtectedRegions();
    bool IsAddressProtected(uint32 address) const;

    // State validation
    bool ValidateMemoryState();
    bool ValidateSystemState();

private:
    // Protected memory regions
    struct PROTECTED_REGION
    {
        uint32 start;
        uint32 size;
    };

    bool ValidateRegion(const PROTECTED_REGION& region);
    void NotifyModeChange(bool enabled);

    CAchievementMemory& m_memory;
    bool m_enabled;
    std::vector<PROTECTED_REGION> m_protectedRegions;
    std::vector<HardcoreModeCallback> m_modeCallbacks;
};