#include "AchievementSystem.h"
#include "AchievementSystemImpl.h"
#include "MemoryMap.h"

namespace {
	static std::unique_ptr<CAchievementSystem> g_instance;
}

// Static methods for singleton management
CAchievementSystem* CAchievementSystem::GetInstance()
{
	return g_instance.get();
}

void CAchievementSystem::CreateInstance(CMemoryMap* memoryMap)
{
	if (!g_instance)
	{
		g_instance = std::make_unique<CAchievementSystemImpl>(*memoryMap);
		g_instance->Initialize();
	}
}

void CAchievementSystem::DestroyInstance()
{
	if (g_instance)
	{
		g_instance->Reset();
		g_instance.reset();
	}
}