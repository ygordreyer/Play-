#pragma once

#include "../../Framework/include/Types.h"
#include "MemoryMap.h"
#include <functional>
#include <mutex>
#include <vector>

class CAchievementMemoryMonitor
{
public:
    typedef std::function<void(uint32, uint32)> WatchCallback;  // address, value

    struct MEMORY_WATCH
    {
        uint32 address;
        uint32 size;
        uint32 lastValue;
        bool active;
        WatchCallback callback;
    };

    CAchievementMemoryMonitor(CMemoryMap& memoryMap);
    virtual ~CAchievementMemoryMonitor() = default;

    // Watch management
    void AddWatch(uint32 address, uint32 size, const WatchCallback& callback = nullptr);
    void RemoveWatch(uint32 address);
    void ClearWatches();

    // Memory access handlers
    uint32 ReadMemory(uint32 address);
    void WriteMemory(uint32 address, uint32 value);

private:
    void InstallHandlers();
    void UpdateWatch(uint32 address, uint32 oldValue, uint32 newValue);

    CMemoryMap& m_memoryMap;
    std::recursive_mutex m_watchLock;
    std::vector<MEMORY_WATCH> m_watches;

};