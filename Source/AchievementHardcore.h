#pragma once

#include "../../Framework/include/Types.h"
#include <vector>

class CAchievementMemoryMonitor;

class CAchievementHardcore
{
public:
    CAchievementHardcore(CAchievementMemoryMonitor& memory);
    virtual ~CAchievementHardcore() = default;

    // State management
    void Enable(bool enable);
    bool IsEnabled() const;

    // Memory protection
    void AddProtectedRegion(uint32 start, uint32 size);
    bool IsAddressProtected(uint32 address) const;

    // State validation
    bool ValidateMemoryState();
    bool ValidateSystemState();
    bool ValidateSaveStateOperation(const char* operation);

private:
    struct PROTECTED_REGION
    {
        uint32 start;
        uint32 size;
    };

    CAchievementMemoryMonitor& m_memory;
    bool m_enabled;
    std::vector<PROTECTED_REGION> m_protectedRegions;
};