#include "veyra/ipc/SharedMemory.h"

#include <aclapi.h>

namespace veyra::ipc {

bool SharedMemoryRegion::create(const std::wstring& name, size_t size)
{
    close();

    // NULL DACL: allow any process (incl. audiodg.exe) to open the mapping.
    // This is a deliberate, scoped relaxation for a parameters-only region that
    // carries no secrets; tighten to an audio-service SID ACL for production.
    SECURITY_DESCRIPTOR sd{};
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
    SECURITY_ATTRIBUTES sa{sizeof(sa), &sd, FALSE};

    mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
                                  0, static_cast<DWORD>(size), name.c_str());
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

    mapping_ = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
    if (!mapping_)
        return false;

    view_ = MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, size);
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
