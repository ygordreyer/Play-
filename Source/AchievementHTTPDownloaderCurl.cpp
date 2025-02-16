#include "AchievementHTTPDownloaderCurl.h"
#include <cassert>
#include <chrono>


AchievementHTTPDownloaderCurl::AchievementHTTPDownloaderCurl()
{
    curl_global_init(CURL_GLOBAL_ALL);
    m_multi_handle = curl_multi_init();
}

AchievementHTTPDownloaderCurl::~AchievementHTTPDownloaderCurl()
{
    if (m_multi_handle)
    {
        curl_multi_cleanup(m_multi_handle);
        m_multi_handle = nullptr;
    }
    curl_global_cleanup();
}

bool AchievementHTTPDownloaderCurl::Initialize(std::string user_agent)
{
    m_user_agent = std::move(user_agent);
    return (m_multi_handle != nullptr);
}

AchievementHTTPDownloader::Request* AchievementHTTPDownloaderCurl::InternalCreateRequest()
{
    return new Request();
}

void AchievementHTTPDownloaderCurl::InternalPollRequests()
{
    int still_running = 0;
    curl_multi_perform(m_multi_handle, &still_running);

    int msgs_in_queue;
    while (CURLMsg* msg = curl_multi_info_read(m_multi_handle, &msgs_in_queue))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            CURL* easy_handle = msg->easy_handle;
            Request* request = nullptr;
            curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &request);
            assert(request != nullptr);

            if (msg->data.result == CURLE_OK)
            {
                long response_code;
                curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
                request->status_code = static_cast<int32>(response_code);

                char* content_type;
                curl_easy_getinfo(easy_handle, CURLINFO_CONTENT_TYPE, &content_type);
                if (content_type)
                    request->content_type = content_type;
            }
            else
            {
                request->status_code = HTTP_STATUS_ERROR;
            }

            request->state.store(Request::State::Complete);
            curl_multi_remove_handle(m_multi_handle, easy_handle);
        }
    }
}

bool AchievementHTTPDownloaderCurl::StartRequest(AchievementHTTPDownloader::Request* request_)
{
    Request* request = static_cast<Request*>(request_);
    request->handle = curl_easy_init();
    if (!request->handle)
        return false;

    curl_easy_setopt(request->handle, CURLOPT_URL, request->url.c_str());
    curl_easy_setopt(request->handle, CURLOPT_USERAGENT, m_user_agent.c_str());
    curl_easy_setopt(request->handle, CURLOPT_PRIVATE, request);
    curl_easy_setopt(request->handle, CURLOPT_WRITEFUNCTION, &WriteCallback);
    curl_easy_setopt(request->handle, CURLOPT_WRITEDATA, request);
    curl_easy_setopt(request->handle, CURLOPT_TIMEOUT_MS, static_cast<long>(m_timeout * 1000.0f));
    curl_easy_setopt(request->handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(request->handle, CURLOPT_SSL_VERIFYHOST, 2L);

    if (request->type == Request::Type::Post)
    {
        curl_easy_setopt(request->handle, CURLOPT_POST, 1L);
        curl_easy_setopt(request->handle, CURLOPT_POSTFIELDS, request->post_data.c_str());
    }

    CURLMcode mcode = curl_multi_add_handle(m_multi_handle, request->handle);
    if (mcode != CURLM_OK)
    {
        curl_easy_cleanup(request->handle);
        request->handle = nullptr;
        return false;
    }

    request->start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    request->state.store(Request::State::Started);
    return true;
}

void AchievementHTTPDownloaderCurl::CloseRequest(AchievementHTTPDownloader::Request* request_)
{
    Request* request = static_cast<Request*>(request_);
    if (request->handle)
    {
        curl_easy_cleanup(request->handle);
        request->handle = nullptr;
    }
}

size_t AchievementHTTPDownloaderCurl::WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    Request* request = static_cast<Request*>(userdata);
    const size_t bytes = size * nmemb;
    const size_t pos = request->data.size();
    request->data.resize(pos + bytes);
    std::memcpy(request->data.data() + pos, ptr, bytes);
    request->state.store(Request::State::Receiving);
    return bytes;
}