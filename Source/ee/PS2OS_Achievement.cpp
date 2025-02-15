#include "PS2OS.h"
#include "AchievementMemory.h"
#include "Log.h"

#define LOG_NAME "PS2OS_Achievement"

// Achievement memory instance
static std::unique_ptr<CAchievementMemory> g_AchievementMemory;

void CPS2OS::InitializeAchievements()
{
    if (!g_AchievementMemory)
    {
        g_AchievementMemory = std::make_unique<CAchievementMemory>(GetEeContext(), *this, GetDmac());
        CLog::GetInstance().Print(LOG_NAME, "Achievement memory system initialized\r\n");
    }
}

void CPS2OS::ShutdownAchievements()
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory.reset();
        CLog::GetInstance().Print(LOG_NAME, "Achievement memory system shutdown\r\n");
    }
}

void CPS2OS::EnableAchievementMemoryProtection(bool enable)
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->EnableMemoryProtection(enable);
        CLog::GetInstance().Print(LOG_NAME, "Achievement memory protection %s\r\n",
            enable ? "enabled" : "disabled");
    }
}

void CPS2OS::AddAchievementMemoryWatch(uint32 address, uint32 size, 
    const std::function<void(uint32, uint32, uint32)>& callback)
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->AddMemoryWatch(address, size, callback);
        CLog::GetInstance().Print(LOG_NAME, "Added achievement memory watch at 0x%08X\r\n", 
            address);
    }
}

void CPS2OS::RemoveAchievementMemoryWatch(uint32 address)
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->RemoveMemoryWatch(address);
        CLog::GetInstance().Print(LOG_NAME, "Removed achievement memory watch at 0x%08X\r\n", 
            address);
    }
}

void CPS2OS::ClearAchievementMemoryWatches()
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->ClearMemoryWatches();
        CLog::GetInstance().Print(LOG_NAME, "Cleared all achievement memory watches\r\n");
    }
}

bool CPS2OS::ValidateAchievementMemoryState()
{
    if (g_AchievementMemory)
    {
        return g_AchievementMemory->ValidateMemoryState();
    }
    return true;
}

void CPS2OS::SetAchievementProtectedRegion(uint32 start, uint32 size)
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->SetProtectedRegion(start, size);
        CLog::GetInstance().Print(LOG_NAME, "Set achievement protected region: 0x%08X - 0x%08X\r\n",
            start, start + size);
    }
}

void CPS2OS::ClearAchievementProtectedRegions()
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->ClearProtectedRegions();
        CLog::GetInstance().Print(LOG_NAME, "Cleared all achievement protected regions\r\n");
    }
}

void CPS2OS::OnAchievementModeChange(bool hardcoreMode)
{
    if (g_AchievementMemory)
    {
        g_AchievementMemory->EnableMemoryProtection(hardcoreMode);
        CLog::GetInstance().Print(LOG_NAME, "Achievement mode changed: %s\r\n",
            hardcoreMode ? "Hardcore" : "Normal");
    }
}