#pragma once

#include "Types.h"
#include "MIPS.h"
#include "ee/PS2OS.h"
#include "ee/DMAC.h"
#include <functional>
#include <vector>
#include <algorithm>

class CAchievementMemory
{
public:
    typedef std::function<void(uint32, uint32, uint32)> MemoryCallback;  // address, old value, new value

    CAchievementMemory(CMIPS&, CPS2OS&, CDMAC&);
    virtual ~CAchievementMemory() = default;

    // Achievement memory monitoring
    void AddMemoryWatch(uint32 address, uint32 size, const MemoryCallback& callback = nullptr);
    void RemoveMemoryWatch(uint32 address);
    void ClearMemoryWatches();

    // Memory access control (for hardcore mode)
    void EnableMemoryProtection(bool enable);
    void SetProtectedRegion(uint32 start, uint32 size);
    void ClearProtectedRegions();

private:
    struct MEMORYWATCH
    {
        uint32 address;
        uint32 size;
        uint32 value;
        bool active;
        MemoryCallback callback;
    };

    void InstallHandlers();
    void UpdateMemoryWatch(uint32 address, uint32 oldValue, uint32 newValue);
    bool IsAddressProtected(uint32 address) const;

    CMIPS& m_ee;
    CPS2OS& m_os;
    CDMAC& m_dmac;

    std::vector<MEMORYWATCH> m_memoryWatches;
    std::vector<std::pair<uint32, uint32>> m_protectedRegions;  // start, size pairs
    bool m_protectionEnabled;
};