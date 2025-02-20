#include "AchievementMemoryMonitor.h"
#include "MIPS.h"
#include <cassert>

CAchievementMemoryMonitor::CAchievementMemoryMonitor(CMemoryMap& memoryMap)
    : m_memoryMap(memoryMap)
    , m_initialized(false)
    , m_handlersInstalled(false)
{
}

CAchievementMemoryMonitor::~CAchievementMemoryMonitor()
{
    Shutdown();
}

bool CAchievementMemoryMonitor::Initialize()
{
    if(m_initialized)
        return true;

    try
    {
        // Don't install handlers immediately - wait for first Update()
        m_initialized = true;
        printf("Achievement Memory Monitor: Initialized successfully\n");
        return true;
    }
    catch(const std::exception& e)
    {
        printf("Achievement Memory Monitor: Initialization failed - %s\n", e.what());
        return false;
    }
    catch(...)
    {
        printf("Achievement Memory Monitor: Initialization failed - Unknown error\n");
        return false;
    }
}

void CAchievementMemoryMonitor::Shutdown()
{
    if(!m_initialized)
        return;

    RemoveHandlers();
    ClearWatches();
    m_initialized = false;
}

bool CAchievementMemoryMonitor::ValidateMemoryAccess(uint32 address, uint32 size) const
{
    // Check if access is within EE RAM
    if(address < EXPOSED_EE_RAM_SIZE)
    {
        return (address + size) <= EXPOSED_EE_RAM_SIZE;
    }
    
    // Check if access is within scratchpad
    if(address >= SCRATCHPAD_START && address < (SCRATCHPAD_START + EXPOSED_SCRATCHPAD_SIZE))
    {
        return (address + size) <= (SCRATCHPAD_START + EXPOSED_SCRATCHPAD_SIZE);
    }

    return false;
}

void CAchievementMemoryMonitor::AddWatch(uint32 address, uint32 size, const WatchCallback& callback)
{
    if(!m_initialized)
        return;

    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    try
    {
        // Validate memory access before adding watch
        if(!ValidateMemoryAccess(address, size))
            return;

        // Remove any existing watch at this address
        RemoveWatch(address);

        // Test read to ensure memory is accessible
        uint32 testRead = ReadMemory(address);

        MEMORY_WATCH watch;
        watch.address = address;
        watch.size = size;
        watch.lastValue = testRead;
        watch.active = true;
        watch.callback = callback;

        m_watches.push_back(watch);
    }
    catch(...)
    {
        // If we can't read the memory location, don't add the watch
    }
}

void CAchievementMemoryMonitor::RemoveWatch(uint32 address)
{
    if(!m_initialized)
        return;

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
    if(!m_initialized)
        return;

    std::lock_guard<std::recursive_mutex> lock(m_watchLock);
    m_watches.clear();
}

uint32 CAchievementMemoryMonitor::ReadMemory(uint32 address)
{
    if(!m_initialized)
        return 0;

    std::lock_guard<std::recursive_mutex> lock(m_watchLock);
    
    try
    {
        // Find the watch for this address to determine size
        uint32 size = 4; // Default to word size if no watch found
        for(const auto& watch : m_watches)
        {
            if(watch.address == address)
            {
                size = watch.size;
                break;
            }
        }

        // Validate memory access
        if(!ValidateMemoryAccess(address, size))
        {
            printf("Achievement Memory Monitor: Invalid memory access at 0x%08X (size %u)\n",
                address, size);
            return 0;
        }

        // Fast paths for common sizes
        switch(size)
        {
        case 1: // Byte access
            return m_memoryMap.GetByte(address);

        case 2: // Half-word access
            if((address & 1) == 0) // Aligned
                return m_memoryMap.GetHalf(address);
            else // Unaligned - use fast byte reads
            {
                uint32 value = m_memoryMap.GetByte(address);
                value |= (m_memoryMap.GetByte(address + 1) << 8);
                return value;
            }

        case 4: // Word access
            if((address & 3) == 0) // Aligned
                return m_memoryMap.GetWord(address);
            else // Unaligned - use fast byte reads
            {
                uint32 value = m_memoryMap.GetByte(address);
                value |= (m_memoryMap.GetByte(address + 1) << 8);
                value |= (m_memoryMap.GetByte(address + 2) << 16);
                value |= (m_memoryMap.GetByte(address + 3) << 24);
                return value;
            }

        case 8: // Double-word access
            if((address & 7) == 0) // Aligned
            {
                uint64 value = static_cast<uint64>(m_memoryMap.GetWord(address));
                value |= (static_cast<uint64>(m_memoryMap.GetWord(address + 4)) << 32);
                return static_cast<uint32>(value); // Return lower 32 bits
            }
            else // Unaligned - use fast byte reads
            {
                uint32 value = m_memoryMap.GetByte(address);
                value |= (m_memoryMap.GetByte(address + 1) << 8);
                value |= (m_memoryMap.GetByte(address + 2) << 16);
                value |= (m_memoryMap.GetByte(address + 3) << 24);
                return value; // Return first 32 bits only
            }

        default:
            printf("Achievement Memory Monitor: Unsupported memory access size %u at 0x%08X\n",
                size, address);
            return m_memoryMap.GetWord(address); // Default to word access
        }
    }
    catch(const std::exception& e)
    {
        printf("Achievement Memory Monitor: Memory read failed at 0x%08X - %s\n",
            address, e.what());
        return 0;
    }
    catch(...)
    {
        printf("Achievement Memory Monitor: Unknown error reading memory at 0x%08X\n",
            address);
        return 0;
    }
}

void CAchievementMemoryMonitor::WriteMemory(uint32 address, uint32 value)
{
    if(!m_initialized)
        return;

    std::lock_guard<std::recursive_mutex> lock(m_watchLock);

    try
    {
        // Validate memory access
        if(!ValidateMemoryAccess(address, sizeof(uint32)))
            return;

        for(auto& watch : m_watches)
        {
            if(!watch.active)
                continue;

            // Check if write overlaps with watch region
            if(address >= watch.address && address < (watch.address + watch.size))
            {
                uint32 oldValue = watch.lastValue;
                uint32 newValue = 0;

                try
                {
                    newValue = ReadMemory(watch.address);
                    if(oldValue != newValue)
                    {
                        UpdateWatch(watch.address, oldValue, newValue);
                        watch.lastValue = newValue;
                    }
                }
                catch(...)
                {
                    // If reading fails, disable the watch
                    watch.active = false;
                }
            }
        }
    }
    catch(...)
    {
        // Ignore any other errors
    }
}

void CAchievementMemoryMonitor::InstallHandlers()
{
    if(m_handlersInstalled)
        return;

    // Create memory handlers that capture this instance
    auto readHandler = [this](uint32 address, uint32) -> uint32 {
        return this->ReadMemory(address);
    };

    auto writeHandler = [this](uint32 address, uint32 value) -> uint32 {
        this->WriteMemory(address, value);
        return 0;
    };

    // Install handlers for main memory region (0x00000000 - 0x02000000)
    m_memoryMap.InsertReadMap(0x00000000, EXPOSED_EE_RAM_SIZE, readHandler, 0);
    m_memoryMap.InsertWriteMap(0x00000000, EXPOSED_EE_RAM_SIZE, writeHandler, 0);

    // Install handlers for scratchpad (0x70000000 - 0x70004000)
    m_memoryMap.InsertReadMap(SCRATCHPAD_START, EXPOSED_SCRATCHPAD_SIZE, readHandler, 0);
    m_memoryMap.InsertWriteMap(SCRATCHPAD_START, EXPOSED_SCRATCHPAD_SIZE, writeHandler, 0);

    m_handlersInstalled = true;
}

void CAchievementMemoryMonitor::RemoveHandlers()
{
    if(!m_handlersInstalled)
        return;

    // Remove handlers by installing null handlers
    auto nullHandler = [](uint32, uint32) -> uint32 { return 0; };

    m_memoryMap.InsertReadMap(0x00000000, EXPOSED_EE_RAM_SIZE, nullHandler, 0);
    m_memoryMap.InsertWriteMap(0x00000000, EXPOSED_EE_RAM_SIZE, nullHandler, 0);
    m_memoryMap.InsertReadMap(SCRATCHPAD_START, EXPOSED_SCRATCHPAD_SIZE, nullHandler, 0);
    m_memoryMap.InsertWriteMap(SCRATCHPAD_START, EXPOSED_SCRATCHPAD_SIZE, nullHandler, 0);

    m_handlersInstalled = false;
}

void CAchievementMemoryMonitor::Update()
{
    if(!m_initialized)
        return;

    std::lock_guard<std::recursive_mutex> lock(m_watchLock);
    
    // Install handlers on first update if not already installed
    if (!m_handlersInstalled)
    {
        try
        {
            printf("Achievement Memory Monitor: Installing memory handlers\n");
            InstallHandlers();
            printf("Achievement Memory Monitor: Handlers installed successfully\n");
        }
        catch (const std::exception& e)
        {
            printf("Achievement Memory Monitor: Failed to install handlers - %s\n", e.what());
            return;
        }
    }

    // Remove any inactive watches first
    size_t initialWatchCount = m_watches.size();
    m_watches.erase(
        std::remove_if(m_watches.begin(), m_watches.end(),
            [](const MEMORY_WATCH& watch) { return !watch.active; }),
        m_watches.end());
    
    if (initialWatchCount != m_watches.size())
    {
        printf("Achievement Memory Monitor: Removed %zu inactive watches\n",
            initialWatchCount - m_watches.size());
    }

    if(m_watches.empty())
        return;

    // Update active watches
    for(auto& watch : m_watches)
    {
        if (!ValidateMemoryAccess(watch.address, watch.size))
        {
            printf("Achievement Memory Monitor: Invalid memory access at 0x%08X (size %u)\n",
                watch.address, watch.size);
            watch.active = false;
            continue;
        }

        try
        {
            uint32 currentValue = ReadMemory(watch.address);
            if(currentValue != watch.lastValue)
            {
                try
                {
                    UpdateWatch(watch.address, watch.lastValue, currentValue);
                    watch.lastValue = currentValue;
                }
                catch(const std::exception& e)
                {
                    printf("Achievement Memory Monitor: Watch update failed at 0x%08X - %s\n",
                        watch.address, e.what());
                    watch.active = false;
                }
            }
        }
        catch(const std::exception& e)
        {
            printf("Achievement Memory Monitor: Memory read failed at 0x%08X - %s\n",
                watch.address, e.what());
            watch.active = false;
        }
    }
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