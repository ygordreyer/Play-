cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME Tests)
project(${PROJECT_NAME})

include(../Source/achievements.cmake)

set(TEST_SRC
    TestMain.cpp
)

if(BUILD_ACHIEVEMENTS)
    set(ACHIEVEMENT_TEST_SRC
        AchievementSystemTest.cpp
    )
    list(APPEND TEST_SRC ${ACHIEVEMENT_TEST_SRC})
endif()

add_executable(${PROJECT_NAME} ${TEST_SRC})
target_link_libraries(${PROJECT_NAME} PlayCore gtest gtest_main)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})