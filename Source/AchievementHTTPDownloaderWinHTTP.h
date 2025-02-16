#pragma once

#include "AchievementHTTPDownloader.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <windows.h>
#include <winhttp.h>

class AchievementHTTPDownloaderWinHTTP final : public AchievementHTTPDownloader
{
public:
    AchievementHTTPDownloaderWinHTTP();
    ~AchievementHTTPDownloaderWinHTTP() override;

    bool Initialize(std::string user_agent);

protected:
    Request* InternalCreateRequest() override;
    void InternalPollRequests() override;
    bool StartRequest(AchievementHTTPDownloader::Request* request) override;
    void CloseRequest(AchievementHTTPDownloader::Request* request) override;

private:
    struct Request : AchievementHTTPDownloader::Request
    {
        HINTERNET connection = nullptr;
        HINTERNET request_handle = nullptr;
        std::wstring wide_url;
        std::wstring host;
        std::wstring path;
        bool https = false;
    };

    static void CALLBACK StatusCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD info_len);
    static std::pair<std::wstring, std::wstring> ParseURL(const std::wstring& url, bool* is_https);

    HINTERNET m_session = nullptr;
    std::wstring m_user_agent;
};