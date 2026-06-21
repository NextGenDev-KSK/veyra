#include "UpdaterClient.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>

#include <chrono>

#include "veyra/Logging.h"

namespace veyra::service {

namespace {

std::wstring widen(const std::string& s)
{
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int) s.size(), nullptr, 0);
    std::wstring w((size_t) n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int) s.size(), w.data(), n);
    return w;
}

// Best-effort HTTPS GET. Returns true + body on success.
bool httpsGet(const std::string& url, std::string& out)
{
    const std::wstring wurl = widen(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {0}, path[2048] = {0};
    uc.lpszHostName = host; uc.dwHostNameLength = 255;
    uc.lpszUrlPath = path;  uc.dwUrlPathLength = 2047;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
        return false;

    HINTERNET session = WinHttpOpen(L"Veyra/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
        return false;

    HINTERNET conn = WinHttpConnect(session, host, uc.nPort, 0);
    HINTERNET req = conn ? WinHttpOpenRequest(conn, L"GET", path, nullptr, WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES,
                                              uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                         : nullptr;
    bool ok = false;
    if (req
        && WinHttpSendRequest(req, L"Accept: application/vnd.github+json\r\n", (DWORD) -1L,
                              WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        && WinHttpReceiveResponse(req, nullptr))
    {
        DWORD avail = 0;
        do {
            avail = 0;
            if (!WinHttpQueryDataAvailable(req, &avail) || avail == 0)
                break;
            std::string chunk((size_t) avail, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(req, chunk.data(), avail, &read))
                break;
            chunk.resize(read);
            out += chunk;
        } while (avail > 0);
        ok = !out.empty();
    }

    if (req) WinHttpCloseHandle(req);
    if (conn) WinHttpCloseHandle(conn);
    if (session) WinHttpCloseHandle(session);
    return ok;
}

} // namespace

UpdaterClient::~UpdaterClient() { stop(); }

void UpdaterClient::start(std::string currentVersion, std::string feedUrl)
{
    if (running_.exchange(true))
        return;
    current_ = std::move(currentVersion);
    url_ = std::move(feedUrl);
    thread_ = std::thread(&UpdaterClient::run, this);
}

void UpdaterClient::stop()
{
    if (!running_.exchange(false))
        return;
    if (thread_.joinable())
        thread_.join();
}

UpdateInfo UpdaterClient::latest() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

void UpdaterClient::run()
{
    while (running_.load())
    {
        std::string feed;
        if (httpsGet(url_, feed))
        {
            const UpdateInfo info = UpdateChecker::evaluate(current_, feed);
            { std::lock_guard<std::mutex> lock(mutex_); info_ = info; }
            if (log_)
            {
                if (info.available)
                    log_->info("Update available: v" + info.version + " (" + info.url + ")");
                else
                    log_->info("Veyra is up to date (v" + current_ + ").");
            }
        }
        else if (log_)
        {
            log_->warn("Update check failed to reach the release feed.");
        }

        // Re-check daily; wake promptly on shutdown.
        for (int i = 0; i < 24 * 60 && running_.load(); ++i)
            std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}

} // namespace veyra::service
