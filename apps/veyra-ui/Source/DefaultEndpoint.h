#pragma once

// ===========================================================================
// WIRED — RUNTIME VERIFICATION REQUIRED on real hardware.
// ===========================================================================
//
// Sets the Windows default *playback* endpoint via the undocumented (but stable
// since Vista) IPolicyConfig interface, so the system-wide Veyra APO processes
// the user's preferred device — the foundation of the "Preferred Output Device"
// auto-switch (no VB-Cable, no virtual sink, no user source selection).
//
// Used by AudioDevices.cpp (setDefaultRenderEndpoint) + the RootComponent device
// watcher: when BridgeConfig.preferredOutputId is present and isn't already the
// default, switch it; otherwise leave the Windows default alone (auto fallback).
// Setting the default endpoint requires no admin (per-user).
//
// The IPolicyConfig vtable order below is the documented Vista layout; it must be
// runtime-verified on hardware (a wrong offset would call the wrong method).

#include <mmdeviceapi.h> // ERole, eConsole/eMultimedia/eCommunications
#include <mmreg.h>       // WAVEFORMATEX
#include <propidl.h>     // PROPVARIANT, PROPERTYKEY

#include <string>

namespace veyra::ui {

struct DeviceShareMode; // opaque; only referenced by pointer below

struct __declspec(uuid("f8679f50-850a-41cf-9c72-430f290290c8")) IPolicyConfig : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX*, WAVEFORMATEX*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, INT64*, INT64*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, INT64*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR wszDeviceId, ERole eRole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

// CLSID_CPolicyConfigClient {870af99c-171d-4f9e-af0d-e63df40c2bc9}
static const CLSID kPolicyConfigClient =
    {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};

// Make `deviceId` the default playback device for all roles. Returns true on
// success. Caller must have CoInitialize'd the thread.
inline bool setDefaultOutput(const std::wstring& deviceId)
{
    IPolicyConfig* pc = nullptr;
    if (FAILED(CoCreateInstance(kPolicyConfigClient, nullptr, CLSCTX_ALL,
                                __uuidof(IPolicyConfig), reinterpret_cast<void**>(&pc)))
        || pc == nullptr)
        return false;

    bool ok = true;
    for (ERole role : {eConsole, eMultimedia, eCommunications})
        ok = SUCCEEDED(pc->SetDefaultEndpoint(deviceId.c_str(), role)) && ok;
    pc->Release();
    return ok;
}

} // namespace veyra::ui
