#pragma once

#include "AchievementHTTPDownloader.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <curl/curl.h>

class AchievementHTTPDownloaderCurl final : public AchievementHTTPDownloader
{
public:
    AchievementHTTPDownloaderCurl();
    ~AchievementHTTPDownloaderCurl() override;

    bool Initialize(std::string user_agent);

protected:
    Request* InternalCreateRequest() override;
    void InternalPollRequests() override;
    bool StartRequest(AchievementHTTPDownloader::Request* request) override;
    void CloseRequest(AchievementHTTPDownloader::Request* request) override;

private:
    struct Request : AchievementHTTPDownloader::Request
    {
        CURL* handle = nullptr;
    };

    static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

    CURLM* m_multi_handle = nullptr;
    std::string m_user_agent;
};