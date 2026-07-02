#include "veyra/ipc/SharedMemory.h"

#include <aclapi.h>
#include <sddl.h>

namespace veyra::ipc {

namespace {

// Replace the name's "Global\" prefix with "Local\" for console-mode fallback.
std::wstring toLocalPrefix(const std::wstring& name)
{
    const auto pos = name.find(L"Global\\");
    if (pos != std::wstring::npos)
        return L"Local\\" + name.substr(pos + 7);
    return name;
}

// Build a SECURITY_ATTRIBUTES granting full access to System + Administrators and
// read-only to everyone else. Callers must LocalFree(sd) after use.
// Using an explicit DACL instead of the previous null DACL (which granted full
// access to all processes on the machine) tightens security while still allowing
// the UI (interactive user) and audiodg.exe (LocalSystem) to read the blocks.
bool buildMappingSa(SECURITY_ATTRIBUTES& sa, PSECURITY_DESCRIPTOR& sd)
{
    // D:  DACL
    //   (A;;GA;;;SY)  LocalSystem — full access (service + audiodg)
    //   (A;;GA;;;BA)  Administrators — full access
    //   (A;;GR;;;WD)  Everyone — generic read (UI / overlay read the viz/tracker)
    static constexpr wchar_t kSddl[] =
        L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GR;;;WD)";
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            kSddl, SDDL_REVISION_1, &sd, nullptr))
        return false;
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle       = FALSE;
    return true;
}

} // namespace

bool SharedMemoryRegion::create(const std::wstring& name, size_t size)
{
    close();

    PSECURITY_DESCRIPTOR sd  = nullptr;
    SECURITY_ATTRIBUTES  sa{};
    SECURITY_ATTRIBUTES* pSa = buildMappingSa(sa, sd) ? &sa : nullptr;

    mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, pSa, PAGE_READWRITE,
                                  0, static_cast<DWORD>(size), name.c_str());

    if (!mapping_ && GetLastError() == ERROR_ACCESS_DENIED)
    {
        // Console mode: the interactive user lacks SeCreateGlobalPrivilege.
        // Fall back to the Local\ namespace (same session — still works).
        const std::wstring local = toLocalPrefix(name);
        if (local != name)
            mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, pSa, PAGE_READWRITE,
                                          0, static_cast<DWORD>(size), local.c_str());
    }

    if (sd)
        LocalFree(sd);

    if (!mapping_)
        return false;

    createdNew_ = (GetLastError() != ERROR_ALREADY_EXISTS);

    view_ = MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!view_)
    {
        close();
        return false;
    }
    return true;
}

bool SharedMemoryRegion::open(const std::wstring& name, size_t size)
{
    close();

    // Read-only: consumers (APO / UI / overlay) never write, and the DACL only
    // grants them read. Requesting FILE_MAP_ALL_ACCESS here is denied for any
    // opener that isn't System/Administrators — notably the APO inside
    // audiodg.exe (LocalService) and the interactive-user UI.
    // Try the canonical name first (Global\ for cross-session objects).
    mapping_ = OpenFileMappingW(FILE_MAP_READ, FALSE, name.c_str());

    if (!mapping_)
    {
        // Installed service created it under Global\; if that failed it fell
        // back to Local\ (console mode). Try Local\ so both paths work.
        const std::wstring local = toLocalPrefix(name);
        if (local != name)
            mapping_ = OpenFileMappingW(FILE_MAP_READ, FALSE, local.c_str());
    }

    if (!mapping_)
        return false;

    view_ = MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, size);
    if (!view_)
    {
        close();
        return false;
    }
    createdNew_ = false;
    return true;
}

void SharedMemoryRegion::close()
{
    if (view_)
    {
        UnmapViewOfFile(view_);
        view_ = nullptr;
    }
    if (mapping_)
    {
        CloseHandle(mapping_);
        mapping_ = nullptr;
    }
    createdNew_ = false;
}

} // namespace veyra::ipc
