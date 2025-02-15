#include "MemoryWatch.h"
#include <algorithm>

void CMemoryWatch::AddWatchPoint(uint32 address, uint32 size, uint32 flags, const WatchCallback& callback)
{
    WATCHPOINT watch;
    watch.address = address;
    watch.size = size;
    watch.flags = flags;
    watch.callback = callback;
    m_watchPoints.push_back(watch);
}

void CMemoryWatch::RemoveWatchPoint(uint32 address)
{
    m_watchPoints.erase(
        std::remove_if(m_watchPoints.begin(), m_watchPoints.end(),
            [address](const WATCHPOINT& watch) { return watch.address == address; }),
        m_watchPoints.end());
}

void CMemoryWatch::ClearWatchPoints()
{
    m_watchPoints.clear();
}

bool CMemoryWatch::HasWatchPoint(uint32 address, uint32 size) const
{
    return std::any_of(m_watchPoints.begin(), m_watchPoints.end(),
        [address, size](const WATCHPOINT& watch) {
            return (address >= watch.address && address < watch.address + watch.size) ||
                   (address + size > watch.address && address + size <= watch.address + watch.size);
        });
}

void CMemoryWatch::CheckRead(uint32 address, uint32 size, uint32 value)
{
    for (const auto& watch : m_watchPoints)
    {
        if (address >= watch.address && address + size <= watch.address + watch.size)
        {
            if (watch.callback)
            {
                watch.callback(address, value, value);
            }
        }
    }
}

void CMemoryWatch::CheckWrite(uint32 address, uint32 size, uint32 oldValue, uint32 newValue)
{
    for (const auto& watch : m_watchPoints)
    {
        if (address >= watch.address && address + size <= watch.address + watch.size)
        {
            if (watch.callback)
            {
                watch.callback(address, oldValue, newValue);
            }
        }
    }
}