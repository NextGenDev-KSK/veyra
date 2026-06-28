#pragma once

#include <string>

namespace veyra::service {

// Service identity, shared by the installer and the SCM dispatcher.
inline constexpr wchar_t kServiceName[]  = L"VeyraAudioService";
inline constexpr wchar_t kDisplayName[]  = L"Veyra Audio Service";
inline constexpr wchar_t kDescription[]  =
    L"Orchestrates Veyra Sounds: config/preset state, detection, and IPC.";

// Registers the service to auto-start as NT AUTHORITY\LocalService, pointing at this exe.
// Returns true on success; on failure, 'error' receives a human-readable reason.
bool installService(std::wstring& error);

// Stops (if running) and removes the service. Returns true on success.
bool uninstallService(std::wstring& error);

} // namespace veyra::service
