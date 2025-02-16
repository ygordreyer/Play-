#include "AchievementHardcore.h"
#include "AchievementMemoryMonitor.h"
#include <algorithm>

CAchievementHardcore::CAchievementHardcore(CAchievementMemoryMonitor& memory)
    : m_memory(memory)
    , m_enabled(false)
{
}

void CAchievementHardcore::Enable(bool enable)
{
    if (m_enabled == enable)
        return;

    m_enabled = enable;

    // When enabling hardcore mode, validate current state
    if (m_enabled)
    {
        ValidateSystemState();
    }
}

bool CAchievementHardcore::IsEnabled() const
{
    return m_enabled;
}

void CAchievementHardcore::AddProtectedRegion(uint32 start, uint32 size)
{
    // Remove any overlapping regions
    m_protectedRegions.erase(
        std::remove_if(m_protectedRegions.begin(), m_protectedRegions.end(),
            [start, size](const PROTECTED_REGION& region) {
                return (start < (region.start + region.size)) && ((start + size) > region.start);
            }),
        m_protectedRegions.end());

    // Add new region
    PROTECTED_REGION region;
    region.start = start;
    region.size = size;
    m_protectedRegions.push_back(region);
}

bool CAchievementHardcore::IsAddressProtected(uint32 address) const
{
    if (!m_enabled)
        return false;

    return std::any_of(m_protectedRegions.begin(), m_protectedRegions.end(),
        [address](const PROTECTED_REGION& region) {
            return (address >= region.start) && (address < (region.start + region.size));
        });
}

bool CAchievementHardcore::ValidateMemoryState()
{
    // In hardcore mode, protected memory regions must not be modified
    if (!m_enabled)
        return true;

    // TODO: Validate memory contents against known good values
    return true;
}

bool CAchievementHardcore::ValidateSystemState()
{
    if (!m_enabled)
        return true;

    // Validate memory state
    if (!ValidateMemoryState())
        return false;

    // TODO: Validate other system state (registers, etc.)
    return true;
}

bool CAchievementHardcore::ValidateSaveStateOperation(const char* operation)
{
    // In hardcore mode, save states are not allowed
    if (m_enabled)
    {
        // TODO: Log attempt to use save states in hardcore mode
        return false;
    }

    return true;
}