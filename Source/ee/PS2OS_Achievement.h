#pragma once

#include "Types.h"
#include "AchievementMemory.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

// Forward declarations
class CMIPS;
class CDMAC;

// Achievement-related methods for PS2OS
class CPS2OS_Achievement
{
public:
    // Achievement memory system initialization
    void InitializeAchievements();
    void ShutdownAchievements();

    // Memory protection
    void EnableAchievementMemoryProtection(bool enable);
    void SetAchievementProtectedRegion(uint32 start, uint32 size);
    void ClearAchievementProtectedRegions();

    // Memory watching
    void AddAchievementMemoryWatch(uint32 address, uint32 size, 
        const CAchievementMemory::MemoryCallback& callback = nullptr);
    void RemoveAchievementMemoryWatch(uint32 address);
    void ClearAchievementMemoryWatches();

    // Memory validation
    bool ValidateAchievementMemoryState();

    // State management
    void SaveAchievementMemoryState(Framework::CZipArchiveWriter& archive);
    void LoadAchievementMemoryState(Framework::CZipArchiveReader& archive);

    // Mode changes
    void OnAchievementModeChange(bool hardcoreMode);

protected:
    // Achievement memory instance
    std::unique_ptr<CAchievementMemory> m_AchievementMemory;

    // References to required systems
    CMIPS& m_ee;
    CDMAC& m_dmac;
};