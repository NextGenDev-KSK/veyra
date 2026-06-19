#include "AppRuleWatcher.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::ui {

namespace {
std::string foregroundAppId()
{
    HWND hw = GetForegroundWindow();
    if (!hw)
        return {};
    DWORD pid = 0;
    GetWindowThreadProcessId(hw, &pid);
    if (!pid)
        return {};

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h)
        return {};

    std::string out;
    wchar_t buf[MAX_PATH];
    DWORD    n = MAX_PATH;
    if (QueryFullProcessImageNameW(h, 0, buf, &n))
    {
        std::wstring path(buf, n);
        const auto slash = path.find_last_of(L"\\/");
        const std::wstring base = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
        const int len = WideCharToMultiByte(CP_UTF8, 0, base.data(), (int) base.size(),
                                             nullptr, 0, nullptr, nullptr);
        out.resize((size_t) len);
        WideCharToMultiByte(CP_UTF8, 0, base.data(), (int) base.size(), out.data(), len, nullptr, nullptr);
        for (auto& c : out)
            c = (char) std::tolower((unsigned char) c);
    }
    CloseHandle(h);
    return out;
}
} // namespace

void AppRuleWatcher::setRulesFile(std::filesystem::path file)
{
    file_ = std::move(file);
    haveMtime_ = false;
    reloadIfChanged();
}

void AppRuleWatcher::reloadIfChanged()
{
    std::error_code ec;
    const auto mt = std::filesystem::last_write_time(file_, ec);
    if (ec) // no rules file yet
    {
        engine_.setRules({});
        haveMtime_ = false;
        return;
    }
    if (haveMtime_ && mt == mtime_)
        return;
    mtime_ = mt;
    haveMtime_ = true;

    std::ifstream in(file_, std::ios::binary);
    if (!in)
    {
        engine_.setRules({});
        return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    engine_.setRules(veyra::AppRuleEngine::rulesFromJson(ss.str()));
}

std::optional<std::string> AppRuleWatcher::poll()
{
    reloadIfChanged();

    const std::string app = foregroundAppId();
    if (app.empty() || app == lastApp_)
        return std::nullopt;
    lastApp_ = app;

    if (auto uuid = engine_.resolve(app); uuid && *uuid != lastApplied_)
    {
        lastApplied_ = *uuid;
        return uuid;
    }
    return std::nullopt;
}

} // namespace veyra::ui
