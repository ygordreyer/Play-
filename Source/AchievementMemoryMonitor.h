#pragma once

#include "Types.h"
#include "MemoryMap.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

class CAchievementMemoryMonitor
{
public:
    struct MEMORY_WATCH
    {
        uint32 address;
        uint32 size;
        std::function<void(uint32, uint32)> callback;
    };

    CAchievementMemoryMonitor(CMemoryMap& memoryMap)
        : m_memoryMap(memoryMap)
    {
    }

    virtual ~CAchievementMemoryMonitor() = default;

    void AddWatch(uint32 address, uint32 size, const std::function<void(uint32, uint32)>& callback)
    {
        MEMORY_WATCH watch;
        watch.address = address;
        watch.size = size;
        watch.callback = callback;
        m_watches.push_back(std::move(watch));
    }

    void RemoveWatch(uint32 address)
    {
        m_watches.erase(
            std::remove_if(m_watches.begin(), m_watches.end(),
                [address](const MEMORY_WATCH& watch) { return watch.address == address; }),
            m_watches.end());
    }

    void ClearWatches()
    {
        m_watches.clear();
    }

    void Update()
    {
        for(const auto& watch : m_watches)
        {
            uint32 value = 0;
            switch(watch.size)
            {
            case 1:
                value = m_memoryMap.GetByte(watch.address);
                break;
            case 2:
                value = m_memoryMap.GetHalf(watch.address);
                break;
            case 4:
                value = m_memoryMap.GetWord(watch.address);
                break;
            default:
                continue;
            }
            
            auto prevValue = m_lastValues[watch.address];
            if(value != prevValue)
            {
                m_lastValues[watch.address] = value;
                if(watch.callback)
                {
                    watch.callback(watch.address, value);
                }
            }
        }
    }

private:
    CMemoryMap& m_memoryMap;
    std::vector<MEMORY_WATCH> m_watches;
    std::map<uint32, uint32> m_lastValues;
};