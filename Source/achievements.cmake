# Achievement System Files
set(ACHIEVEMENT_SRCS
    Source/AchievementSystem.h
    Source/AchievementSystemImpl.h
    Source/AchievementMemoryMonitor.h
    Source/AchievementStateManager.h
    Source/AchievementRcheevosAdapter.h
)

if(BUILD_ACHIEVEMENTS)
    # Check rcheevos dependency
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/rcheevos/include/rcheevos.h")
        message(FATAL_ERROR "rcheevos dependency not found. Please run: git submodule update --init --recursive")
    endif()

    # Build rcheevos library
    set(RCHEEVOS_SRCS
        deps/rcheevos/src/rcheevos/alloc.c
        deps/rcheevos/src/rcheevos/condition.c
        deps/rcheevos/src/rcheevos/condset.c
        deps/rcheevos/src/rcheevos/consoleinfo.c
        deps/rcheevos/src/rcheevos/format.c
        deps/rcheevos/src/rcheevos/lboard.c
        deps/rcheevos/src/rcheevos/memref.c
        deps/rcheevos/src/rcheevos/operand.c
        deps/rcheevos/src/rcheevos/richpresence.c
        deps/rcheevos/src/rcheevos/runtime.c
        deps/rcheevos/src/rcheevos/runtime_progress.c
        deps/rcheevos/src/rcheevos/trigger.c
        deps/rcheevos/src/rcheevos/value.c
        deps/rcheevos/src/rapi/rc_api_common.c
        deps/rcheevos/src/rapi/rc_api_editor.c
        deps/rcheevos/src/rapi/rc_api_info.c
        deps/rcheevos/src/rapi/rc_api_runtime.c
        deps/rcheevos/src/rapi/rc_api_user.c
        deps/rcheevos/src/rhash/md5.c
    )

    add_library(rcheevos STATIC ${RCHEEVOS_SRCS})
    target_include_directories(rcheevos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/rcheevos/include
    )
    target_compile_definitions(rcheevos PRIVATE
        RC_DISABLE_LUA=1
    )

    # Add achievement system files to the project
    target_sources(PlayCore PRIVATE ${ACHIEVEMENT_SRCS})
    
    # Add achievement system include directories
    target_include_directories(PlayCore PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/rcheevos/include
    )

    # Add achievement system compile definitions
    target_compile_definitions(PlayCore PRIVATE
        ENABLE_ACHIEVEMENTS=1
    )

    # Link against rcheevos
    target_link_libraries(PlayCore rcheevos)
endif()