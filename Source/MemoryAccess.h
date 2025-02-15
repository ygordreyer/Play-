#pragma once

#include "Types.h"
#include <vector>
#include <deque>
#include <functional>

class CMemoryAccess
{
public:
    enum ACCESS_TYPE
    {
        ACCESS_READ,
        ACCESS_WRITE
    };

    struct ACCESS_RECORD
    {
        uint32 address;
        uint32 size;
        uint32 value;
        ACCESS_TYPE type;
        uint64 timestamp;  // For tracking access timing
    };

    using AccessCallback = std::function<void(const ACCESS_RECORD&)>;

    CMemoryAccess();
    virtual ~CMemoryAccess() = default;

    // Access tracking
    void TrackAccess(uint32 address, uint32 size, uint32 value, ACCESS_TYPE type);
    void ClearHistory();
    
    // Access pattern analysis
    bool HasRecentAccess(uint32 address, uint32 size, uint64 timeWindow) const;
    uint32 GetAccessCount(uint32 address, uint32 size) const;
    
    // Callbacks for monitoring
    void AddAccessCallback(AccessCallback callback);
    void RemoveAllCallbacks();

    // Configuration
    void SetHistoryLimit(size_t limit);
    void EnableTracking(bool enable);

private:
    static constexpr size_t DEFAULT_HISTORY_LIMIT = 1000;
    
    std::deque<ACCESS_RECORD> m_accessHistory;
    std::vector<AccessCallback> m_callbacks;
    size_t m_historyLimit;
    bool m_trackingEnabled;

    void NotifyCallbacks(const ACCESS_RECORD& record);
    void TrimHistory();
};