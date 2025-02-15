#include "AchievementMemory.h"
#include "MIPS.h"
#include "ee/PS2OS.h"
#include "ee/DMAC.h"
#include <algorithm>

CAchievementMemory::CAchievementMemory(CMIPS& ee, CPS2OS& os, CDMAC& dmac)
    : m_ee(ee)
    , m_os(os)
    , m_dmac(dmac)
    , m_protectionEnabled(false)
{
    InstallHandlers();
}

void CAchievementMemory::AddMemoryWatch(uint32 address, uint32 size, const MemoryCallback& callback)
{
    // Validate size to prevent overflow
    if (size == 0 || (address + size) <= address)
        return;

    MEMORYWATCH watch;
    watch.address = address;
    watch.size = size;
    watch.value = 0;
    watch.active = true;
    watch.callback = callback;
    m_memoryWatches.push_back(watch);
}

void CAchievementMemory::RemoveMemoryWatch(uint32 address)
{
    m_memoryWatches.erase(
        std::remove_if(m_memoryWatches.begin(), m_memoryWatches.end(),
            [address](const MEMORYWATCH& watch) { return watch.address == address; }),
        m_memoryWatches.end());
}

void CAchievementMemory::ClearMemoryWatches()
{
    m_memoryWatches.clear();
}

void CAchievementMemory::EnableMemoryProtection(bool enable)
{
    m_protectionEnabled = enable;
}

void CAchievementMemory::SetProtectedRegion(uint32 startAddress, uint32 size)
{
    // Validate size to prevent overflow
    if (size == 0 || (startAddress + size) <= startAddress)
        return;

    m_protectedRegions.push_back(std::make_pair(startAddress, size));
}

void CAchievementMemory::ClearProtectedRegions()
{
    m_protectedRegions.clear();
}

void CAchievementMemory::InstallHandlers()
{
    // Install memory access handlers for achievement system
    // These will be called by the emulator when memory is accessed
    // The actual implementation will depend on the emulator's memory access system
    // TODO: Implement proper memory access handlers when CMIPS interface is available
    // Possible implementation:
    // - Register read/write handlers with m_ee
    // - Set up memory access callbacks
    // - Configure protection regions
}

void CAchievementMemory::UpdateMemoryWatch(uint32 address, uint32 oldValue, uint32 newValue)
{
    for (auto& watch : m_memoryWatches)
    {
        if (!watch.active)
            continue;

        // Check if address falls within the watched range
        if (address >= watch.address && address < (watch.address + watch.size))
        {
            if (watch.callback)
            {
                watch.callback(address, oldValue, newValue);
            }
        }
    }
}

bool CAchievementMemory::IsAddressProtected(uint32 address) const
{
    if (!m_protectionEnabled)
        return false;

    for (const auto& region : m_protectedRegions)
    {
        // Check if address falls within the protected range
        if (address >= region.first && address < (region.first + region.second))
        {
            return true;
        }
    }
    return false;
}
