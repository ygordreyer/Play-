if(BUILD_ACHIEVEMENTS)
    # Achievement System Files
    set(ACHIEVEMENT_SRCS
        Source/AchievementSystem.h
        Source/AchievementSystemImpl.h
        Source/AchievementMemoryMonitor.h
        Source/AchievementMemoryMonitor.cpp
        Source/AchievementStateManager.h
        Source/AchievementRcheevosAdapter.h
        Source/AchievementHTTPDownloader.h
        Source/AchievementHTTPDownloader.cpp
        #Source/AchievementHTTPDownloaderCurl.h
        #Source/AchievementHTTPDownloaderCurl.cpp
        Source/AchievementHardcore.h
        Source/AchievementHardcore.cpp
    )

    # Platform-specific HTTP implementation
    if(TARGET_PLATFORM_WIN32)
        list(APPEND ACHIEVEMENT_SRCS
            Source/AchievementHTTPDownloaderWinHTTP.h
            Source/AchievementHTTPDownloaderWinHTTP.cpp
        )
        # WinHTTP is always available on Windows
        target_compile_definitions(PlayCore PRIVATE HAVE_WINHTTP=1)
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
        set(CURL_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/curl")
        if(NOT EXISTS "${CURL_INSTALL_DIR}/include/curl/curl.h")
            message(STATUS "Downloading and building CURL for Android...")
            execute_process(
                COMMAND git clone --depth 1 --branch curl-8_5_0 https://github.com/curl/curl.git "${CURL_INSTALL_DIR}"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps
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