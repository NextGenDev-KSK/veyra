#pragma once

// RAII wrapper over a named Win32 file mapping used to share the parameter
// block between the service (creator/writer) and the APO (opener/reader).
// Created with an explicit DACL: full access for System and Administrators,
// read-only for Everyone. Global\ prefix for cross-session objects; Local\
// fallback in console mode where SeCreateGlobalPrivilege is absent.

#include <cstddef>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace veyra::ipc {

class SharedMemoryRegion {
public:
    SharedMemoryRegion() = default;
    ~SharedMemoryRegion() { close(); }

    SharedMemoryRegion(const SharedMemoryRegion&) = delete;
    SharedMemoryRegion& operator=(const SharedMemoryRegion&) = delete;

    // Service side: create (or open if it already exists) a writable region.
    bool create(const std::wstring& name, size_t size);

    // Consumer side (APO / UI / overlay): open an existing region read-only.
    // Every cross-process consumer is a pure reader; requesting read access is
    // what the DACL grants them (Everyone: read). Requesting write here would
    // be denied for non-admin/non-System openers — including the APO inside
    // audiodg.exe, which runs as LocalService.
    bool open(const std::wstring& name, size_t size);

    void close();

    void* data() const noexcept { return view_; }
    bool  valid() const noexcept { return view_ != nullptr; }
    bool  createdNew() const noexcept { return createdNew_; }

private:
    HANDLE mapping_ = nullptr;
    void*  view_ = nullptr;
    bool   createdNew_ = false;
};

} // namespace veyra::ipc
