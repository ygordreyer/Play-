#pragma once

#include "../../Framework/include/Types.h"
#include "MemoryMap.h"
#include <functional>
#include <mutex>
#include <vector>

// Size of the EE physical memory exposed to RetroAchievements
static constexpr uint32 EXPOSED_EE_RAM_SIZE = 0x02000000;  // 32MB
static constexpr uint32 EXPOSED_SCRATCHPAD_SIZE = 0x4000;  // 16KB
static constexpr uint32 SCRATCHPAD_START = 0x70000000;

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
    virtual ~CAchievementMemoryMonitor();

    // System management
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Watch management
    void AddWatch(uint32 address, uint32 size, const WatchCallback& callback = nullptr);
    void RemoveWatch(uint32 address);
    void ClearWatches();

    // Memory access handlers
    uint32 ReadMemory(uint32 address);
    void WriteMemory(uint32 address, uint32 value);
    
    // Update memory watches
    void Update();

private:
    void InstallHandlers();
    void RemoveHandlers();
    void UpdateWatch(uint32 address, uint32 oldValue, uint32 newValue);
    bool ValidateMemoryAccess(uint32 address, uint32 size) const;

    CMemoryMap& m_memoryMap;
    std::recursive_mutex m_watchLock;
    std::vector<MEMORY_WATCH> m_watches;
    bool m_initialized;
    bool m_handlersInstalled;
};