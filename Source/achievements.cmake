if(BUILD_ACHIEVEMENTS)
    # Achievement System Files
    set(ACHIEVEMENT_SRCS
        Source/AchievementSystem.h
        Source/AchievementSystem.cpp
        Source/AchievementSystemImpl.h
        Source/AchievementMemoryMonitor.h
        Source/AchievementMemoryMonitor.cpp
        Source/AchievementStateManager.h
        Source/AchievementRcheevosAdapter.h
        Source/AchievementRcheevosAdapter.cpp
        Source/AchievementHTTPDownloader.h
        Source/AchievementHTTPDownloader.cpp
        #Source/AchievementHTTPDownloaderCurl.h
        #Source/AchievementHTTPDownloaderCurl.cpp
        Source/AchievementHardcore.h
        Source/AchievementHardcore.cpp
        Source/AchievementsConfig.h
        Source/AchievementsConfig.cpp
    )

    # Platform-specific HTTP implementation
    if(TARGET_PLATFORM_WIN32)
        list(APPEND ACHIEVEMENT_SRCS
            Source/AchievementHTTPDownloaderWinHTTP.h
            Source/AchievementHTTPDownloaderWinHTTP.cpp
        )
        # WinHTTP is always available on Windows
        target_compile_definitions(PlayCore PRIVATE HAVE_WINHTTP=1)
        # Link against WinHTTP library
        target_link_libraries(PlayCore winhttp)
    endif()

    # Try to find system CURL
    find_package(CURL QUIET)

    if(CURL_FOUND)
        # Use system CURL if available
        target_include_directories(PlayCore PRIVATE ${CURL_INCLUDE_DIRS})
        target_link_libraries(PlayCore ${CURL_LIBRARIES})
        target_compile_definitions(PlayCore PRIVATE HAVE_LIBCURL=1)
    elseif(TARGET_PLATFORM_ANDROID)
        # For Android, we'll bundle CURL
        set(CURL_INSTALL_DIR "${CMAKE_SOURCE_DIR}/deps/curl")
        if(NOT EXISTS "${CURL_INSTALL_DIR}/include/curl/curl.h")
            message(STATUS "Downloading and building CURL for Android...")
            execute_process(
                COMMAND git clone --depth 1 --branch curl-8_5_0 https://github.com/curl/curl.git "${CURL_INSTALL_DIR}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps
            )
            # Build CURL for Android here
            # TODO: Add Android-specific CURL build configuration
        endif()
        target_include_directories(PlayCore PRIVATE "${CURL_INSTALL_DIR}/include")
        target_link_libraries(PlayCore "${CURL_INSTALL_DIR}/lib/libcurl.a")
        target_compile_definitions(PlayCore PRIVATE HAVE_LIBCURL=1)
    elseif(NOT TARGET_PLATFORM_WIN32)
        # For other platforms, require CURL
        message(FATAL_ERROR "libcurl is required for non-Windows platforms. Please install libcurl development files.")
    endif()

    # Check rcheevos dependency
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/deps/rcheevos/include/rcheevos.h")
        message(FATAL_ERROR "rcheevos dependency not found. Please run: git submodule update --init --recursive")
    endif()

    # Build rcheevos library
    set(RCHEEVOS_SRCS
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/alloc.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/condition.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/condset.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/consoleinfo.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/format.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/lboard.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/memref.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/operand.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/richpresence.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/runtime.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/runtime_progress.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/trigger.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rcheevos/value.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rapi/rc_api_common.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rapi/rc_api_editor.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rapi/rc_api_info.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rapi/rc_api_runtime.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rapi/rc_api_user.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rhash/md5.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rc_client.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rc_util.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rc_compat.c
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src/rc_version.c
    )

    add_library(rcheevos STATIC ${RCHEEVOS_SRCS})
    target_include_directories(rcheevos PUBLIC
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/include
    )
    target_compile_definitions(rcheevos PRIVATE
        RC_DISABLE_LUA=1
    )

    # Add achievement system files to the project
    target_sources(PlayCore PRIVATE ${ACHIEVEMENT_SRCS})
    
    # Add achievement system include directories
    target_include_directories(PlayCore PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/include
        ${CMAKE_SOURCE_DIR}/deps/rcheevos/src
        ${CMAKE_SOURCE_DIR}/deps/Framework/include
        ${CMAKE_SOURCE_DIR}/deps/Framework/src
        ${CMAKE_SOURCE_DIR}/deps/Framework
    )

    # Add achievement system compile definitions
    target_compile_definitions(PlayCore PRIVATE
        ENABLE_ACHIEVEMENTS=1
        BUILD_ACHIEVEMENTS=1
    )

    # Set position independent code for rcheevos
    set_property(TARGET rcheevos PROPERTY POSITION_INDEPENDENT_CODE ON)

    # Link against rcheevos using consistent plain syntax
    target_link_libraries(PlayCore rcheevos)
endif()