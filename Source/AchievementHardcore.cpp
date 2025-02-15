#include "AchievementHardcore.h"
#include "AchievementMemory.h"
#include "Log.h"

#define LOG_NAME "AchievementHardcore"

CAchievementHardcore::CAchievementHardcore(CAchievementMemory& memory)
    : m_memory(memory)
    , m_enabled(false)
{
}

void CAchievementHardcore::Enable(bool enable)
{
    if (m_enabled == enable)
        return;

    m_enabled = enable;
    m_memory.EnableMemoryProtection(enable);

    CLog::GetInstance().Print(LOG_NAME, "Hardcore mode %s\r\n", 
        enable ? "enabled" : "disabled");

    NotifyModeChange(enable);
}

void CAchievementHardcore::AddModeChangeCallback(const HardcoreModeCallback& callback)
{
    m_modeCallbacks.push_back(callback);
}

bool CAchievementHardcore::ValidateSaveStateOperation(const char* operation)
{
    if (!m_enabled)
        return true;

    CLog::GetInstance().Print(LOG_NAME, "Save state %s blocked in hardcore mode\r\n", 
        operation);
    return false;
}

bool CAchievementHardcore::CheckSaveStateAllowed(Framework::CZipArchiveReader& reader)
{
    if (!m_enabled)
        return true;

    try
    {
        auto stream = reader.BeginReadFile("header");
        SAVESTATE_HEADER header;
        stream->Read(&header, sizeof(header));

        return ValidateSaveStateHeader(header);
    }
    catch(...)
    {
        CLog::GetInstance().Print(LOG_NAME, "Failed to read save state header\r\n");
        return false;
    }
}

bool CAchievementHardcore::CheckLoadStateAllowed(Framework::CZipArchiveReader& reader)
{
    if (!m_enabled)
        return true;

    try
    {
        auto stream = reader.BeginReadFile("header");
        SAVESTATE_HEADER header;
        stream->Read(&header, sizeof(header));

        if (!ValidateSaveStateHeader(header))
            return false;

        return IsSaveStateCompatible(header);
    }
    catch(...)
    {
        CLog::GetInstance().Print(LOG_NAME, "Failed to read save state header\r\n");
        return false;
    }
}

void CAchievementHardcore::AddProtectedRegion(uint32 start, uint32 size)
{
    PROTECTED_REGION region;
    region.start = start;
    region.size = size;
    m_protectedRegions.push_back(region);

    if (m_enabled)
    {
        m_memory.SetProtectedRegion(start, size);
    }

    CLog::GetInstance().Print(LOG_NAME, "Added protected region: 0x%08X - 0x%08X\r\n",
        start, start + size);
}

void CAchievementHardcore::ClearProtectedRegions()
{
    m_protectedRegions.clear();
    
    if (m_enabled)
    {
        m_memory.ClearProtectedRegions();
    }

    CLog::GetInstance().Print(LOG_NAME, "Cleared all protected regions\r\n");
}

bool CAchievementHardcore::IsAddressProtected(uint32 address) const
{
    if (!m_enabled)
        return false;

    for (const auto& region : m_protectedRegions)
    {
        if (address >= region.start && address < (region.start + region.size))
            return true;
    }
    return false;
}

bool CAchievementHardcore::ValidateMemoryState()
{
    if (!m_enabled)
        return true;

    for (const auto& region : m_protectedRegions)
    {
        if (!ValidateRegion(region))
            return false;
    }
    return true;
}

bool CAchievementHardcore::ValidateSystemState()
{
    if (!m_enabled)
        return true;

    // Validate memory state
    if (!ValidateMemoryState())
        return false;

    // Additional system state validation can be added here
    return true;
}

bool CAchievementHardcore::ValidateRegion(const PROTECTED_REGION& region)
{
    return m_memory.ValidateRange(region.start, region.start + region.size);
}

void CAchievementHardcore::NotifyModeChange(bool enabled)
{
    for (const auto& callback : m_modeCallbacks)
    {
        callback(enabled);
    }
}

bool CAchievementHardcore::ValidateSaveStateHeader(const SAVESTATE_HEADER& header)
{
    if (header.magic != SAVESTATE_MAGIC)
    {
        CLog::GetInstance().Print(LOG_NAME, "Invalid save state magic\r\n");
        return false;
    }

    return true;
}

bool CAchievementHardcore::IsSaveStateCompatible(const SAVESTATE_HEADER& header)
{
    // Check version compatibility
    // Add more compatibility checks as needed
    return true;
}