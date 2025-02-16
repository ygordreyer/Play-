#include "AchievementMemoryMonitor.h"
#include "MIPS.h"
#include <cassert>

CAchievementMemoryMonitor::CAchievementMemoryMonitor(CMemoryMap& memoryMap)
    : m_memoryMap(memoryMap)
{
    InstallHandlers();
}

void CAchievementMemoryMonitor::AddWatch(uint32 address, uint32 size, const WatchCallback& callback)
{
    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    // Remove any existing watch at this address
    RemoveWatch(address);

    MEMORY_WATCH watch;
    watch.address = address;
    watch.size = size;
    watch.lastValue = ReadMemory(address);
    watch.active = true;
    watch.callback = callback;

    m_watches.push_back(watch);
}

void CAchievementMemoryMonitor::RemoveWatch(uint32 address)
{
    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    for (auto it = m_watches.begin(); it != m_watches.end(); ++it)
    {
        if (it->address == address)
        {
            m_watches.erase(it);
            break;
        }
    }
}

void CAchievementMemoryMonitor::ClearWatches()
{
    std::lock_guard<std::recursive_mutex> lock(m_watchLock);
    m_watches.clear();
}

uint32 CAchievementMemoryMonitor::ReadMemory(uint32 address)
{
    // Read based on size
    uint32 value = 0;
    switch (m_watches[0].size)
    {
    case 1:
        value = m_memoryMap.GetByte(address);
        break;
    case 2:
        value = m_memoryMap.GetHalf(address);
        break;
    case 4:
        value = m_memoryMap.GetWord(address);
        break;
    default:
        assert(false);
        break;
    }
    return value;
}

void CAchievementMemoryMonitor::WriteMemory(uint32 address, uint32 value)
{
    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    for (auto& watch : m_watches)
    {
        if (!watch.active)
            continue;

        // Check if write overlaps with watch region
        if (address >= watch.address && address < (watch.address + watch.size))
        {
            uint32 oldValue = watch.lastValue;
            uint32 newValue = ReadMemory(watch.address);
            
            if (oldValue != newValue)
            {
                UpdateWatch(watch.address, oldValue, newValue);
                watch.lastValue = newValue;
            }
        }
    }
}

void CAchievementMemoryMonitor::InstallHandlers()
{
    // Create memory handlers that capture this instance
    auto readHandler = [this](uint32 address, uint32) -> uint32 {
        return this->ReadMemory(address);
    };

    auto writeHandler = [this](uint32 address, uint32 value) -> uint32 {
        this->WriteMemory(address, value);
        return 0;
    };

    // TODO: Install handlers for specific memory regions based on game requirements
    // m_memoryMap.InsertReadMap(start, end, readHandler, 0);
    // m_memoryMap.InsertWriteMap(start, end, writeHandler, 0);
}

void CAchievementMemoryMonitor::UpdateWatch(uint32 address, uint32 oldValue, uint32 newValue)
{
    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    for (auto& watch : m_watches)
    {
        if (!watch.active)
            continue;

        if (watch.address == address)
        {
            if (watch.callback)
            {
                watch.callback(address, newValue);
            }
            break;
        }
    }
}