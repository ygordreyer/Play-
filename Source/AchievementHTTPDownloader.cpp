#include "AchievementHTTPDownloader.h"
#include <cassert>

#ifdef _WIN32
#include "AchievementHTTPDownloaderWinHTTP.h"
#else
#include "AchievementHTTPDownloaderCurl.h"
#endif

const char AchievementHTTPDownloader::DEFAULT_USER_AGENT[] = "Play-Achievement-Client/1.0";

AchievementHTTPDownloader::~AchievementHTTPDownloader() = default;

std::unique_ptr<AchievementHTTPDownloader> AchievementHTTPDownloader::Create(std::string user_agent)
{
#ifdef _WIN32
    auto downloader = std::make_unique<AchievementHTTPDownloaderWinHTTP>();
#else
    auto downloader = std::make_unique<AchievementHTTPDownloaderCurl>();
#endif
    if (!downloader->Initialize(std::move(user_agent)))
        return nullptr;
    return downloader;
}

void AchievementHTTPDownloader::SetTimeout(float timeout)
{
    m_timeout = timeout;
}

void AchievementHTTPDownloader::SetMaxActiveRequests(uint32 max_active_requests)
{
    m_max_active_requests = max_active_requests;
}

void AchievementHTTPDownloader::CreateRequest(std::string url, Request::Callback callback)
{
    Request* request = InternalCreateRequest();
    request->parent = this;
    request->url = std::move(url);
    request->callback = std::move(callback);
    request->type = Request::Type::Get;
    LockedAddRequest(request);
}

void AchievementHTTPDownloader::CreatePostRequest(std::string url, std::string post_data, Request::Callback callback)
{
    Request* request = InternalCreateRequest();
    request->parent = this;
    request->url = std::move(url);
    request->post_data = std::move(post_data);
    request->callback = std::move(callback);
    request->type = Request::Type::Post;
    LockedAddRequest(request);
}

void AchievementHTTPDownloader::PollRequests()
{
    std::unique_lock<std::mutex> lock(m_pending_http_request_lock);
    LockedPollRequests(lock);
}

void AchievementHTTPDownloader::WaitForAllRequests()
{
    while (HasAnyRequests())
    {
        PollRequests();
    }
}

bool AchievementHTTPDownloader::HasAnyRequests()
{
    std::unique_lock<std::mutex> lock(m_pending_http_request_lock);
    return !m_pending_http_requests.empty();
}

void AchievementHTTPDownloader::LockedAddRequest(Request* request)
{
    std::unique_lock<std::mutex> lock(m_pending_http_request_lock);
    m_pending_http_requests.push_back(request);
}

uint32 AchievementHTTPDownloader::LockedGetActiveRequestCount()
{
    uint32 count = 0;
    for (const Request* request : m_pending_http_requests)
    {
        if (request->state.load() != Request::State::Pending)
            count++;
    }
    return count;
}

void AchievementHTTPDownloader::LockedPollRequests(std::unique_lock<std::mutex>& lock)
{
    InternalPollRequests();

    for (auto it = m_pending_http_requests.begin(); it != m_pending_http_requests.end();)
    {
        Request* request = *it;
        const Request::State state = request->state.load();

        if (state == Request::State::Pending)
        {
            if (LockedGetActiveRequestCount() < m_max_active_requests)
            {
                if (!StartRequest(request))
                {
                    request->status_code = HTTP_STATUS_ERROR;
                    request->state.store(Request::State::Complete);
                }
            }
            ++it;
            continue;
        }

        if (state == Request::State::Complete)
        {
            if (request->callback)
            {
                request->callback(request->status_code, request->content_type, std::move(request->data));
            }

            CloseRequest(request);
            delete request;
            it = m_pending_http_requests.erase(it);
            continue;
        }

        ++it;
    }
}