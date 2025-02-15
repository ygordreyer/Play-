#pragma once

#include "Types.h"
#include <functional>
#include <vector>
#include <algorithm>

class CMemoryWatch
{
public:
    typedef std::function<void(uint32, uint32, uint32)> WatchCallback;

    CMemoryWatch() = default;
    virtual ~CMemoryWatch() = default;

    void AddWatchPoint(uint32 address, uint32 size, uint32 flags, const WatchCallback& callback = nullptr);
    void RemoveWatchPoint(uint32 address);
    void ClearWatchPoints();
    bool HasWatchPoint(uint32 address, uint32 size) const;

    void CheckRead(uint32 address, uint32 size, uint32 value);
    void CheckWrite(uint32 address, uint32 size, uint32 oldValue, uint32 newValue);

private:
    struct WATCHPOINT
    {
        uint32 address;
        uint32 size;
        uint32 flags;
        WatchCallback callback;
    };

    std::vector<WATCHPOINT> m_watchPoints;
};