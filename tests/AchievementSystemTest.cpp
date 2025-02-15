#include "AchievementSystemImpl.h"
#include "MemoryMap.h"
#include "gtest/gtest.h"
#include <memory>

class CAchievementSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_memoryMap = std::make_shared<CMemoryMap>();
        m_achievementSystem = std::make_unique<CAchievementSystemImpl>(*m_memoryMap);
    }

    void TearDown() override
    {
        m_achievementSystem.reset();
        m_memoryMap.reset();
    }

    std::shared_ptr<CMemoryMap> m_memoryMap;
    std::unique_ptr<CAchievementSystemImpl> m_achievementSystem;
};

TEST_F(CAchievementSystemTest, InitializationTest)
{
    EXPECT_FALSE(m_achievementSystem->IsInitialized());
    m_achievementSystem->Initialize();
    EXPECT_TRUE(m_achievementSystem->IsInitialized());
}

TEST_F(CAchievementSystemTest, HardcoreModeTest)
{
    m_achievementSystem->Initialize();
    
    EXPECT_FALSE(m_achievementSystem->IsHardcoreModeActive());
    EXPECT_TRUE(m_achievementSystem->IsSaveStateAllowed());

    m_achievementSystem->SetHardcoreMode(true);
    EXPECT_TRUE(m_achievementSystem->IsHardcoreModeActive());
    EXPECT_FALSE(m_achievementSystem->IsSaveStateAllowed());

    m_achievementSystem->SetHardcoreMode(false);
    EXPECT_FALSE(m_achievementSystem->IsHardcoreModeActive());
    EXPECT_TRUE(m_achievementSystem->IsSaveStateAllowed());
}

TEST_F(CAchievementSystemTest, MemoryWatchTest)
{
    m_achievementSystem->Initialize();
    
    bool watchTriggered = false;
    uint32 watchedAddress = 0x1000;
    uint32 watchedValue = 0;

    m_achievementSystem->AddMemoryWatch(watchedAddress, 4,
        [&watchTriggered, &watchedValue](uint32 address, uint32 value)
        {
            watchTriggered = true;
            watchedValue = value;
        });

    // Write to watched memory
    m_memoryMap->SetWord(watchedAddress, 0x12345678);
    m_achievementSystem->Update();

    EXPECT_TRUE(watchTriggered);
    EXPECT_EQ(watchedValue, 0x12345678);
}

TEST_F(CAchievementSystemTest, SaveStateTest)
{
    m_achievementSystem->Initialize();
    m_achievementSystem->SetHardcoreMode(true);

    // Save states should disable achievements in hardcore mode
    m_achievementSystem->OnSaveStateCreated();
    m_achievementSystem->Update();

    // Memory watches should be cleared
    bool watchTriggered = false;
    m_achievementSystem->AddMemoryWatch(0x1000, 4,
        [&watchTriggered](uint32, uint32)
        {
            watchTriggered = true;
        });

    m_memoryMap->SetWord(0x1000, 0x12345678);
    m_achievementSystem->Update();

    EXPECT_FALSE(watchTriggered);
}

TEST_F(CAchievementSystemTest, EventHandlerTest)
{
    m_achievementSystem->Initialize();

    bool unlockHandlerCalled = false;
    bool progressHandlerCalled = false;
    
    m_achievementSystem->RegisterUnlockHandler(
        [&unlockHandlerCalled](const std::string&, const std::string&)
        {
            unlockHandlerCalled = true;
        });

    m_achievementSystem->RegisterProgressHandler(
        [&progressHandlerCalled](const std::string&, float)
        {
            progressHandlerCalled = true;
        });

    // TODO: Add achievement unlock/progress events when rcheevos integration is complete
    // For now, just verify handlers are registered
    EXPECT_FALSE(unlockHandlerCalled);
    EXPECT_FALSE(progressHandlerCalled);
}