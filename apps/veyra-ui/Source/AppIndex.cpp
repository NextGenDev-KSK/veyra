#include "AppIndex.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h> // process snapshot

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace veyra::ui {

namespace {

juce::Image hiconToImage(HICON icon)
{
    if (icon == nullptr)
        return {};
    ICONINFO ii{};
    if (!GetIconInfo(icon, &ii))
        return {};

    BITMAP bm{};
    GetObject(ii.hbmColor, sizeof(bm), &bm);
    const int w = bm.bmWidth, h = bm.bmHeight;
    juce::Image img;
    if (w > 0 && h > 0)
    {
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h; // top-down
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        std::vector<uint8_t> buf((size_t) w * (size_t) h * 4);
        HDC dc = GetDC(nullptr);
        if (GetDIBits(dc, ii.hbmColor, 0, (UINT) h, buf.data(), &bi, DIB_RGB_COLORS))
        {
            img = juce::Image(juce::Image::ARGB, w, h, true);
            juce::Image::BitmapData bd(img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                {
                    const uint8_t* p = &buf[((size_t) y * w + x) * 4]; // BGRA
                    bd.setPixelColour(x, y, juce::Colour::fromRGBA(p[2], p[1], p[0], p[3]));
                }
        }
        ReleaseDC(nullptr, dc);
    }

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask)  DeleteObject(ii.hbmMask);
    return img;
}

juce::Image extractExeIcon(const juce::File& exe)
{
    SHFILEINFOW sfi{};
    if (SHGetFileInfoW(exe.getFullPathName().toWideCharPointer(), 0, &sfi, sizeof(sfi),
                       SHGFI_ICON | SHGFI_LARGEICON))
    {
        juce::Image img = hiconToImage(sfi.hIcon);
        if (sfi.hIcon) DestroyIcon(sfi.hIcon);
        return img;
    }
    return {};
}

} // namespace

std::vector<InstalledApp> scanInstalledApps()
{
    std::vector<InstalledApp> out;
    std::set<std::string> seen;

    auto scanDir = [&](const juce::File& dir)
    {
        if (!dir.isDirectory())
            return;
        for (const auto& lnk : dir.findChildFiles(juce::File::findFiles, true, "*.lnk"))
        {
            if (out.size() >= 250)
                break;
            const juce::File target = lnk.getLinkedTarget();
            if (!target.existsAsFile() || !target.hasFileExtension("exe"))
                continue;
            const std::string match = target.getFileNameWithoutExtension().toLowerCase().toStdString();
            if (match.empty() || seen.count(match))
                continue;
            // Skip obvious uninstallers / helpers.
            if (match.find("unins") != std::string::npos || match == "setup")
                continue;
            seen.insert(match);

            InstalledApp a;
            a.name  = lnk.getFileNameWithoutExtension();
            a.match = juce::String(match);
            a.icon  = extractExeIcon(target);
            out.push_back(std::move(a));
        }
    };

    auto env = [](const char* v) { return juce::SystemStats::getEnvironmentVariable(v, {}); };
    scanDir(juce::File(env("ProgramData") + "\\Microsoft\\Windows\\Start Menu\\Programs"));
    scanDir(juce::File(env("APPDATA") + "\\Microsoft\\Windows\\Start Menu\\Programs"));

    std::sort(out.begin(), out.end(),
              [](const InstalledApp& a, const InstalledApp& b) { return a.name.compareIgnoreCase(b.name) < 0; });
    return out;
}

std::vector<InstalledApp> scanRunningProcesses()
{
    std::vector<InstalledApp> out;
    std::set<std::string> seen;

    const juce::String winDir = juce::SystemStats::getEnvironmentVariable("SystemRoot", "C:\\Windows")
                                    .toLowerCase();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return out;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (out.size() >= 120)
                break;
            const std::string base = juce::String(pe.szExeFile)
                                         .upToLastOccurrenceOf(".exe", false, true)
                                         .toLowerCase().toStdString();
            if (base.empty() || seen.count(base))
                continue;

            // Resolve the full image path so we can both filter out Windows/system
            // processes and extract the EXE's icon.
            juce::String path;
            if (HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID))
            {
                wchar_t buf[MAX_PATH]; DWORD n = MAX_PATH;
                if (QueryFullProcessImageNameW(h, 0, buf, &n))
                    path = juce::String(buf, n);
                CloseHandle(h);
            }
            if (path.isEmpty() || path.toLowerCase().startsWith(winDir))
                continue; // skip system/background processes

            seen.insert(base);
            InstalledApp a;
            a.name  = juce::String(pe.szExeFile).upToLastOccurrenceOf(".exe", false, true);
            a.match = juce::String(base);
            a.icon  = extractExeIcon(juce::File(path));
            out.push_back(std::move(a));
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    std::sort(out.begin(), out.end(),
              [](const InstalledApp& a, const InstalledApp& b) { return a.name.compareIgnoreCase(b.name) < 0; });
    return out;
}

juce::Image defaultAppIcon()
{
    static juce::Image cached = [] {
        auto img = juce::ImageCache::getFromMemory(BinaryData::Veyra_Icon_square_png,
                                                   BinaryData::Veyra_Icon_square_pngSize);
        return img.isValid() ? img.rescaled(32, 32, juce::Graphics::highResamplingQuality)
                             : juce::Image();
    }();
    return cached;
}

} // namespace veyra::ui
