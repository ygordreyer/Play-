#include "AchievementRcheevosAdapter.h"
#include "AchievementMemoryMonitor.h"
#include "../deps/rcheevos/include/rc_runtime.h"
#include "../deps/rcheevos/include/rc_client.h"
#include "../deps/rcheevos/src/rhash/md5.h"
#include <cassert>
#include <sstream>
#include <fstream>
#include <vector>

static uint32_t ReadMemory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
{
    auto* adapter = static_cast<CAchievementRcheevosAdapter*>(rc_client_get_userdata(client));
    if(!adapter || !buffer || num_bytes == 0)
        return 0;

    // Read memory one byte at a time
    for(uint32_t i = 0; i < num_bytes; i++)
    {
        buffer[i] = adapter->PeekByte(address + i, 0, adapter);
    }
    return num_bytes;
}

static void HandleEvent(const rc_client_event_t* event, rc_client_t* client)
{
    auto* adapter = static_cast<CAchievementRcheevosAdapter*>(rc_client_get_userdata(client));
    if(!adapter)
        return;

    switch(event->type)
    {
        case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
            if(event->achievement)
            {
                adapter->OnAchievementUnlocked(
                    std::to_string(event->achievement->id),
                    event->achievement->title ? event->achievement->title : "");
            }
            break;

        case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
            if(event->achievement)
            {
                adapter->OnProgressUpdated(std::to_string(event->achievement->id), 
                    event->achievement->measured_percent);
            }
            break;

        case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
            if(event->leaderboard && event->leaderboard_scoreboard)
            {
                std::stringstream ss(event->leaderboard_scoreboard->submitted_score);
                uint32_t score;
                ss >> score;
                adapter->OnLeaderboardSubmitted(std::to_string(event->leaderboard->id), score);
            }
            break;
    }
}

CAchievementRcheevosAdapter::CAchievementRcheevosAdapter(CMemoryMap& memoryMap)
    : m_client(nullptr, rc_client_destroy)
    , m_memoryMap(memoryMap)
    , m_gameLoaded(false)
    , m_loadGameRequest(nullptr)
    , m_loginRequest(nullptr)
    , m_requestRetries(0)
{
    auto* client = rc_client_create(ReadMemory, nullptr);
    if(client)
    {
        rc_client_set_event_handler(client, HandleEvent);
        rc_client_set_userdata(client, this);
        m_client.reset(client);
    }
}

CAchievementRcheevosAdapter::~CAchievementRcheevosAdapter()
{
    UnloadGame();
}

void CAchievementRcheevosAdapter::RegisterUnlockHandler(const CAchievementSystem::AchievementUnlockedHandler& handler)
{
    m_unlockHandler = handler;
}

void CAchievementRcheevosAdapter::RegisterProgressHandler(const CAchievementSystem::ProgressUpdatedHandler& handler)
{
    m_progressHandler = handler;
}

void CAchievementRcheevosAdapter::RegisterLeaderboardHandler(const CAchievementSystem::LeaderboardSubmittedHandler& handler)
{
    m_leaderboardHandler = handler;
}

bool CAchievementRcheevosAdapter::Initialize(const std::string& username, const std::string& token)
{
    if(!m_client || username.empty() || token.empty())
        return false;

    // Reset state before initializing
    Reset();

    rc_client_begin_login_with_token(m_client.get(), username.c_str(), token.c_str(),
        [](int result, const char* error_message, rc_client_t* client, void* userdata) {
            auto* adapter = static_cast<CAchievementRcheevosAdapter*>(rc_client_get_userdata(client));
            if(!adapter)
                return;

            if(result != RC_OK)
            {
                // Login failed, reset state
                adapter->Reset();
            }
        }, nullptr);

    return true;
}

void CAchievementRcheevosAdapter::Reset()
{
    if(m_client)
    {
        rc_client_reset(m_client.get());
    }

    // Clean up memory monitor
    if(m_memoryMonitor)
    {
        m_memoryMonitor->Shutdown();
        m_memoryMonitor.reset();
    }

    m_gameLoaded = false;
}

void CAchievementRcheevosAdapter::ProcessIdle()
{
    if(!m_client)
        return;

    rc_client_idle(m_client.get());
}

void CAchievementRcheevosAdapter::ProcessFrame()
{
    if(!m_gameLoaded || !m_client)
        return;

    // Only update memory if we have a valid monitor and achievements
    if(m_memoryMonitor && m_memoryMonitor->IsInitialized())
    {
        m_memoryMonitor->Update();
    }

    rc_client_do_frame(m_client.get());
}

void CAchievementRcheevosAdapter::Update()
{
    // Process idle updates first
    ProcessIdle();

    // Then process frame updates if we have achievements
    if(HasAchievements())
    {
        ProcessFrame();
    }
}

bool CAchievementRcheevosAdapter::LoadGame(const std::string& path, uint32 crc)
{
    if(!m_client)
        return false;

    // Validate input
    if(path.empty() || crc == 0)
        return false;

    // Clean up previous game state
    UnloadGame();

    printf("Achievements: Loading game from '%s'\n", path.c_str());

    // Get ELF name for hash
    std::string::size_type start = path.rfind('\\');
    if (start == std::string::npos)
        start = 0;
    else
        start++; // skip backslash

    std::string::size_type end = path.rfind(';');
    if (end == std::string::npos)
        end = path.size();

    if (end < start)
        end = start;

    const std::string name_for_hash = path.substr(start, end - start);
    if (name_for_hash.empty())
    {
        printf("Achievements: Failed to extract ELF name from path\n");
        return false;
    }

    // Calculate MD5 hash
    const uint32_t MAX_HASH_SIZE = 64 * 1024 * 1024;
    std::vector<uint8_t> elf_data;
    try
    {
        // Read ELF file
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            printf("Achievements: Failed to open ELF file '%s'\n", path.c_str());
            return false;
        }

        // Get file size
        const auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Read data
        const uint32_t hash_size = std::min<uint32_t>(static_cast<uint32_t>(size), MAX_HASH_SIZE);
        elf_data.resize(hash_size);
        if (!file.read(reinterpret_cast<char*>(elf_data.data()), hash_size))
        {
            printf("Achievements: Failed to read %u bytes from ELF file\n", hash_size);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        printf("Achievements: Exception while reading ELF file: %s\n", e.what());
        return false;
    }

    // Calculate MD5 using rcheevos' implementation
    md5_state_t md5;
    md5_init(&md5);
    md5_append(&md5, reinterpret_cast<const md5_byte_t*>(name_for_hash.data()),
        static_cast<int>(name_for_hash.size()));
    md5_append(&md5, reinterpret_cast<const md5_byte_t*>(elf_data.data()),
        static_cast<int>(elf_data.size()));

    uint8_t hash_bytes[16];
    md5_finish(&md5, reinterpret_cast<md5_byte_t*>(hash_bytes));

    // Convert to hex string
    char hash[33];
    snprintf(hash, sizeof(hash),
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        hash_bytes[0], hash_bytes[1], hash_bytes[2], hash_bytes[3],
        hash_bytes[4], hash_bytes[5], hash_bytes[6], hash_bytes[7],
        hash_bytes[8], hash_bytes[9], hash_bytes[10], hash_bytes[11],
        hash_bytes[12], hash_bytes[13], hash_bytes[14], hash_bytes[15]);

    printf("Achievements: Hash for '%s' (%zu bytes, %u bytes hashed): %s\n",
        name_for_hash.c_str(), elf_data.size(), static_cast<uint32_t>(elf_data.size()), hash);

    // Cancel any existing request
    if (m_loadGameRequest)
    {
        printf("Achievements: Canceling previous load game request\n");
        rc_client_abort_async(m_client.get(), m_loadGameRequest);
        m_loadGameRequest = nullptr;
    }

    // Reset retry counter
    m_requestRetries = 0;

    // Store hash for retries
    m_currentHash = hash;

    // Begin loading game with calculated hash
    m_loadGameRequest = rc_client_begin_load_game(m_client.get(), m_currentHash.c_str(), LoadGameCallback, nullptr);

    return true;
}

void CAchievementRcheevosAdapter::UnloadGame()
{
    if (!m_client)
        return;

    printf("Achievements: Unloading game\n");

    // Clear achievement cache first to prevent any callbacks
    if (!m_achievementCache.empty())
    {
        printf("Achievements: Clearing achievement cache\n");
        m_achievementCache.clear();
    }

    // Cleanup memory monitor before unloading game
    if (m_memoryMonitor)
    {
        printf("Achievements: Shutting down memory monitor\n");
        m_memoryMonitor->Shutdown();
        m_memoryMonitor->ClearWatches();
        m_memoryMonitor.reset();
    }

    // Unload game from client
    if (m_gameLoaded)
    {
        printf("Achievements: Unloading game from client\n");
        rc_client_unload_game(m_client.get());
    }

    // Reset state
    m_gameLoaded = false;
    m_currentHash.clear();
    m_requestRetries = 0;

    // Cancel any pending requests
    if (m_loadGameRequest)
    {
        printf("Achievements: Canceling pending load game request\n");
        rc_client_abort_async(m_client.get(), m_loadGameRequest);
        m_loadGameRequest = nullptr;
    }

    printf("Achievements: Game unloaded successfully\n");
}

std::vector<CAchievementSystem::ACHIEVEMENT_INFO> CAchievementRcheevosAdapter::GetAchievements() const
{
    std::vector<CAchievementSystem::ACHIEVEMENT_INFO> achievements;
    
    if(!m_client)
        return achievements;

    rc_client_achievement_list_t* list = rc_client_create_achievement_list(m_client.get(), 
        RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
        RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);

    if(list)
    {
        for(uint32_t i = 0; i < list->num_buckets; i++)
        {
            for(uint32_t j = 0; j < list->buckets[i].num_achievements; j++)
            {
                const auto* achievement = list->buckets[i].achievements[j];
                CAchievementSystem::ACHIEVEMENT_INFO info;
                info.id = std::to_string(achievement->id);
                info.title = achievement->title;
                info.description = achievement->description;
                info.points = achievement->points;
                info.unlocked = (achievement->state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED);
                info.progress = achievement->measured_percent;
                achievements.push_back(info);
            }
        }

        rc_client_destroy_achievement_list(list);
    }
    
    return achievements;
}

bool CAchievementRcheevosAdapter::IsAchievementUnlocked(const std::string& id) const
{
    if(!m_client)
        return false;

    uint32_t achievementId;
    std::stringstream ss(id);
    ss >> achievementId;

    const auto* achievement = rc_client_get_achievement_info(m_client.get(), achievementId);
    return achievement && achievement->state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
}

float CAchievementRcheevosAdapter::GetAchievementProgress(const std::string& id) const
{
    if(!m_client)
        return 0.0f;

    uint32_t achievementId;
    std::stringstream ss(id);
    ss >> achievementId;

    const auto* achievement = rc_client_get_achievement_info(m_client.get(), achievementId);
    return achievement ? achievement->measured_percent : 0.0f;
}

uint8_t CAchievementRcheevosAdapter::PeekByte(uint32_t address, uint32_t flags, void* userdata)
{
    auto* adapter = static_cast<CAchievementRcheevosAdapter*>(userdata);
    if(!adapter || !adapter->m_memoryMonitor || !adapter->m_memoryMonitor->IsInitialized() || !adapter->m_gameLoaded)
        return 0;

    try
    {
        return static_cast<uint8_t>(adapter->m_memoryMonitor->ReadMemory(address));
    }
    catch(...)
    {
        return 0;
    }
}

void CAchievementRcheevosAdapter::PokeByte(uint32_t address, uint8_t value, uint32_t flags, void* userdata)
{
    auto* adapter = static_cast<CAchievementRcheevosAdapter*>(userdata);
    if(!adapter || !adapter->m_memoryMonitor || !adapter->m_memoryMonitor->IsInitialized() || !adapter->m_gameLoaded)
        return;

    try
    {
        adapter->m_memoryMonitor->WriteMemory(address, value);
    }
    catch(...)
    {
        // Ignore write errors
    }
}

void CAchievementRcheevosAdapter::OnAchievementUnlocked(const std::string& id, const std::string& title)
{
    if(m_unlockHandler)
    {
        m_unlockHandler(id, title);
    }
}

void CAchievementRcheevosAdapter::OnProgressUpdated(const std::string& id, float progress)
{
    if(m_progressHandler)
    {
        m_progressHandler(id, progress);
    }
}

void CAchievementRcheevosAdapter::OnLeaderboardSubmitted(const std::string& id, uint32 score)
{
    if(m_leaderboardHandler)
    {
        m_leaderboardHandler(id, score);
    }
}

void CAchievementRcheevosAdapter::LoadGameCallback(int result, const char* error_message, rc_client_t* client, void* userdata)
{
    auto* adapter = static_cast<CAchievementRcheevosAdapter*>(rc_client_get_userdata(client));
    if(!adapter)
        return;

    adapter->m_loadGameRequest = nullptr;

    // Handle load errors
    if(result != RC_OK)
    {
        if(result == RC_NO_GAME_LOADED)
        {
            printf("Achievements: Unknown game hash, disabling achievements\n");
            adapter->UnloadGame();
        }
        else if(result == RC_LOGIN_REQUIRED)
        {
            printf("Achievements: Login required, keeping state for re-auth\n");
        }
        else if(result == RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR &&
                adapter->m_requestRetries < adapter->MAX_RETRIES)
        {
            // Retry on temporary errors
            adapter->m_requestRetries++;
            printf("Achievements: Temporary error, retrying (%u/%u)\n",
                adapter->m_requestRetries, adapter->MAX_RETRIES);
            
            // Exponential backoff
            const uint32_t delay = 1000 * (1 << adapter->m_requestRetries);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            // Retry request
            adapter->m_loadGameRequest = rc_client_begin_load_game(adapter->m_client.get(),
                adapter->m_currentHash.c_str(), LoadGameCallback, nullptr);
            return;
        }
        else
        {
            printf("Achievements: Loading game failed: %s (error %d)\n",
                error_message ? error_message : "Unknown error", result);
            adapter->UnloadGame();
        }
        return;
    }

    // Get game info
    const rc_client_game_t* info = rc_client_get_game_info(client);
    if(!info)
    {
        printf("Achievements: Failed to get game info\n");
        adapter->UnloadGame();
        return;
    }

    // Check if game has achievements or leaderboards
    const bool has_achievements = rc_client_has_achievements(client);
    const bool has_leaderboards = rc_client_has_leaderboards(client);
    const bool has_rich_presence = rc_client_has_rich_presence(client);

    printf("Achievements: Loaded game '%s' (ID: %u)\n", info->title, info->id);
    printf("Achievements: Has achievements: %s, leaderboards: %s, rich presence: %s\n",
        has_achievements ? "yes" : "no",
        has_leaderboards ? "yes" : "no",
        has_rich_presence ? "yes" : "no");

    if(!has_achievements && !has_leaderboards)
    {
        printf("Achievements: No achievements or leaderboards, disabling monitoring\n");
        adapter->UnloadGame();
        return;
    }

    // Create and initialize memory monitor only if we have achievements
    if(has_achievements)
    {
        adapter->m_memoryMonitor = std::make_unique<CAchievementMemoryMonitor>(adapter->m_memoryMap);
        if(!adapter->m_memoryMonitor->Initialize())
        {
            printf("Achievements: Failed to initialize memory monitor\n");
            adapter->UnloadGame();
            return;
        }
        printf("Achievements: Memory monitor initialized successfully\n");
    }

    // Mark game as loaded
    adapter->m_gameLoaded = true;

    // Cache achievement data
    adapter->GetAchievements();

    printf("Achievements: Game loaded and initialized successfully\n");
}

void CAchievementRcheevosAdapter::ProcessCallbacks()
{
    // Callbacks are now handled through the event system
}

void CAchievementRcheevosAdapter::UpdateAchievements()
{
    // Updates are now handled in rc_client_do_frame
}

void CAchievementRcheevosAdapter::UpdateLeaderboards()
{
    // Updates are now handled in rc_client_do_frame
}