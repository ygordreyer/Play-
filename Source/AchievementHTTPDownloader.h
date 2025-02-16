#pragma once

#include "../../Framework/include/Types.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class AchievementHTTPDownloader
{
public:
    enum : int32
    {
        HTTP_STATUS_CANCELLED = -3,
        HTTP_STATUS_TIMEOUT = -2,
        HTTP_STATUS_ERROR = -1,
        HTTP_STATUS_OK = 200
    };

    struct Request
    {
        using Data = std::vector<uint8>;
        using Callback = std::function<void(int32 status_code, const std::string& content_type, Data data)>;

        enum class Type
        {
            Get,
            Post,
        };

        enum class State
        {
            Pending,
            Cancelled,
            Started,
            Receiving,
            Complete,
        };

        AchievementHTTPDownloader* parent;
        Callback callback;
        std::string url;
        std::string post_data;
        std::string content_type;
        Data data;
        uint64 start_time;
        int32 status_code = 0;
        uint32 content_length = 0;
        uint32 last_progress_update = 0;
        Type type = Type::Get;
        std::atomic<State> state{State::Pending};
    };

    AchievementHTTPDownloader();
    virtual ~AchievementHTTPDownloader();

    static std::unique_ptr<AchievementHTTPDownloader> Create(std::string user_agent = DEFAULT_USER_AGENT);

    void SetTimeout(float timeout);
    void SetMaxActiveRequests(uint32 max_active_requests);

    void CreateRequest(std::string url, Request::Callback callback);
    void CreatePostRequest(std::string url, std::string post_data, Request::Callback callback);
    void PollRequests();
    void WaitForAllRequests();
    bool HasAnyRequests();

    static const char DEFAULT_USER_AGENT[];

protected:
    virtual Request* InternalCreateRequest() = 0;
    virtual void InternalPollRequests() = 0;

    virtual bool StartRequest(Request* request) = 0;
    virtual void CloseRequest(Request* request) = 0;

    void LockedAddRequest(Request* request);
    uint32 LockedGetActiveRequestCount();
    void LockedPollRequests(std::unique_lock<std::mutex>& lock);

    float m_timeout;
    uint32 m_max_active_requests;

    std::mutex m_pending_http_request_lock;
    std::vector<Request*> m_pending_http_requests;
};