#include "MemoryAccess.h"
#include "Log.h"
#include <algorithm>

#define LOG_NAME "MemoryAccess"

CMemoryAccess::CMemoryAccess()
    : m_historyLimit(DEFAULT_HISTORY_LIMIT)
    , m_trackingEnabled(true)
{
}

void CMemoryAccess::TrackAccess(uint32 address, uint32 size, uint32 value, ACCESS_TYPE type)
{
    if (!m_trackingEnabled)
        return;

    ACCESS_RECORD record;
    record.address = address;
    record.size = size;
    record.value = value;
    record.type = type;
    record.timestamp = static_cast<uint64>(time(nullptr));  // Current time

    m_accessHistory.push_front(record);
    TrimHistory();

    NotifyCallbacks(record);

    CLog::GetInstance().Print(LOG_NAME, "%s access at 0x%08X, size: %d, value: 0x%08X\r\n",
        (type == ACCESS_READ) ? "Read" : "Write", address, size, value);
}

void CMemoryAccess::ClearHistory()
{
    m_accessHistory.clear();
}

bool CMemoryAccess::HasRecentAccess(uint32 address, uint32 size, uint64 timeWindow) const
{
    if (m_accessHistory.empty())
        return false;

    uint64 currentTime = static_cast<uint64>(time(nullptr));
    
    for (const auto& record : m_accessHistory)
    {
        // Check if access is within time window
        if ((currentTime - record.timestamp) > timeWindow)
            break;  // History is ordered, so we can stop here

        // Check if addresses overlap
        if ((record.address < (address + size)) && (address < (record.address + record.size)))
            return true;
    }

    return false;
}

uint32 CMemoryAccess::GetAccessCount(uint32 address, uint32 size) const
{
    uint32 count = 0;
    
    for (const auto& record : m_accessHistory)
    {
        // Check if addresses overlap
        if ((record.address < (address + size)) && (address < (record.address + record.size)))
            count++;
    }

    return count;
}

void CMemoryAccess::AddAccessCallback(AccessCallback callback)
{
    m_callbacks.push_back(callback);
}

void CMemoryAccess::RemoveAllCallbacks()
{
    m_callbacks.clear();
}

void CMemoryAccess::SetHistoryLimit(size_t limit)
{
    m_historyLimit = limit;
    TrimHistory();
}

void CMemoryAccess::EnableTracking(bool enable)
{
    m_trackingEnabled = enable;
    if (!enable)
        ClearHistory();
}

void CMemoryAccess::NotifyCallbacks(const ACCESS_RECORD& record)
{
    for (const auto& callback : m_callbacks)
    {
        callback(record);
    }
}

void CMemoryAccess::TrimHistory()
{
    while (m_accessHistory.size() > m_historyLimit)
    {
        m_accessHistory.pop_back();
    }
}