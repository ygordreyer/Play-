#include "AchievementHTTPDownloaderWinHTTP.h"
#include <cassert>
#include <chrono>


AchievementHTTPDownloaderWinHTTP::AchievementHTTPDownloaderWinHTTP()
{
}

AchievementHTTPDownloaderWinHTTP::~AchievementHTTPDownloaderWinHTTP()
{
    if (m_session)
    {
        WinHttpCloseHandle(m_session);
        m_session = nullptr;
    }
}

bool AchievementHTTPDownloaderWinHTTP::Initialize(std::string user_agent)
{
    std::wstring wide_user_agent;
    wide_user_agent.resize(user_agent.size());
    MultiByteToWideChar(CP_UTF8, 0, user_agent.c_str(), static_cast<int>(user_agent.size()),
        wide_user_agent.data(), static_cast<int>(wide_user_agent.size()));
    m_user_agent = std::move(wide_user_agent);

    m_session = WinHttpOpen(m_user_agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    return (m_session != nullptr);
}

AchievementHTTPDownloader::Request* AchievementHTTPDownloaderWinHTTP::InternalCreateRequest()
{
    return new Request();
}

void AchievementHTTPDownloaderWinHTTP::InternalPollRequests()
{
    // WinHTTP is async, so we don't need to poll
}

bool AchievementHTTPDownloaderWinHTTP::StartRequest(AchievementHTTPDownloader::Request* request_)
{
    Request* request = static_cast<Request*>(request_);

    // Convert URL to wide string
    request->wide_url.resize(request->url.size());
    MultiByteToWideChar(CP_UTF8, 0, request->url.c_str(), static_cast<int>(request->url.size()),
        request->wide_url.data(), static_cast<int>(request->wide_url.size()));

    // Parse URL
    bool is_https;
    std::tie(request->host, request->path) = ParseURL(request->wide_url, &is_https);
    request->https = is_https;

    // Create connection
    request->connection = WinHttpConnect(m_session, request->host.c_str(),
        is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!request->connection)
        return false;

    // Create request handle
    const wchar_t* verb = (request->type == Request::Type::Post) ? L"POST" : L"GET";
    request->request_handle = WinHttpOpenRequest(request->connection, verb, request->path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        is_https ? WINHTTP_FLAG_SECURE : 0);
    if (!request->request_handle)
    {
        WinHttpCloseHandle(request->connection);
        request->connection = nullptr;
        return false;
    }

    // Set callback
    if (!WinHttpSetStatusCallback(request->request_handle, &StatusCallback,
        WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0))
    {
        CloseRequest(request);
        return false;
    }

    // Set timeout
    DWORD timeout = static_cast<DWORD>(m_timeout * 1000.0f);
    WinHttpSetTimeouts(request->request_handle, timeout, timeout, timeout, timeout);

    // Send request
    if (request->type == Request::Type::Post)
    {
        if (!WinHttpSendRequest(request->request_handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            const_cast<char*>(request->post_data.data()), static_cast<DWORD>(request->post_data.size()),
            static_cast<DWORD>(request->post_data.size()), reinterpret_cast<DWORD_PTR>(request)))
        {
            CloseRequest(request);
            return false;
        }
    }
    else
    {
        if (!WinHttpSendRequest(request->request_handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, reinterpret_cast<DWORD_PTR>(request)))
        {
            CloseRequest(request);
            return false;
        }
    }

    request->start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    request->state.store(Request::State::Started);
    return true;
}

void AchievementHTTPDownloaderWinHTTP::CloseRequest(AchievementHTTPDownloader::Request* request_)
{
    Request* request = static_cast<Request*>(request_);
    if (request->request_handle)
    {
        WinHttpCloseHandle(request->request_handle);
        request->request_handle = nullptr;
    }
    if (request->connection)
    {
        WinHttpCloseHandle(request->connection);
        request->connection = nullptr;
    }
}

void CALLBACK AchievementHTTPDownloaderWinHTTP::StatusCallback(HINTERNET handle, DWORD_PTR context,
    DWORD status, LPVOID info, DWORD info_len)
{
    Request* request = reinterpret_cast<Request*>(context);
    if (!request)
        return;

    switch (status)
    {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            WinHttpReceiveResponse(handle, nullptr);
            break;
        }

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            DWORD status_code = 0;
            DWORD size = sizeof(DWORD);
            WinHttpQueryHeaders(handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);
            request->status_code = static_cast<int32>(status_code);

            wchar_t content_type[256] = {};
            size = sizeof(content_type);
            if (WinHttpQueryHeaders(handle, WINHTTP_QUERY_CONTENT_TYPE,
                WINHTTP_HEADER_NAME_BY_INDEX, content_type, &size, WINHTTP_NO_HEADER_INDEX))
            {
                char utf8_content_type[256] = {};
                WideCharToMultiByte(CP_UTF8, 0, content_type, -1, utf8_content_type,
                    sizeof(utf8_content_type), nullptr, nullptr);
                request->content_type = utf8_content_type;
            }

            WinHttpQueryDataAvailable(handle, nullptr);
            break;
        }

        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            DWORD size = *reinterpret_cast<DWORD*>(info);
            if (size > 0)
            {
                size_t offset = request->data.size();
                request->data.resize(offset + size);
                WinHttpReadData(handle, request->data.data() + offset, size, nullptr);
                request->state.store(Request::State::Receiving);
            }
            else
            {
                request->state.store(Request::State::Complete);
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            if (info_len > 0)
            {
                WinHttpQueryDataAvailable(handle, nullptr);
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            request->status_code = HTTP_STATUS_ERROR;
            request->state.store(Request::State::Complete);
            break;
        }
    }
}

std::pair<std::wstring, std::wstring> AchievementHTTPDownloaderWinHTTP::ParseURL(
    const std::wstring& url, bool* is_https)
{
    // Skip protocol
    size_t host_start = url.find(L"://");
    if (host_start == std::wstring::npos)
        host_start = 0;
    else
    {
        *is_https = (url.substr(0, host_start) == L"https");
        host_start += 3;
    }

    // Find path
    size_t path_start = url.find(L'/', host_start);
    if (path_start == std::wstring::npos)
    {
        return std::make_pair(url.substr(host_start), L"/");
    }
    else
    {
        return std::make_pair(url.substr(host_start, path_start - host_start),
            url.substr(path_start));
    }
}