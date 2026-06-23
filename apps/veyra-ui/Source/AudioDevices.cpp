#include "AudioDevices.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <initguid.h> // define PKEY_Device_FriendlyName in this TU
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include "DefaultEndpoint.h" // IPolicyConfig setDefaultOutput()

namespace veyra::ui {

namespace {
std::string wideToUtf8(const wchar_t* w)
{
    if (!w) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s((size_t) (n > 0 ? n - 1 : 0), '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}
} // namespace

namespace {
juce::String formFactorLabel(unsigned f)
{
    // EndpointFormFactor enum order (mmdeviceapi.h).
    static const char* k[] = {"Network", "Speakers", "Line", "Headphones", "Mic",
                              "Headset", "Handset", "Digital", "SPDIF", "HDMI", ""};
    return f < 11 ? juce::String(k[f]) : juce::String();
}

std::vector<OutputDevice> enumerate(EDataFlow flow)
{
    std::vector<OutputDevice> out;

    const bool comInitedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    IMMDeviceEnumerator* en = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                   __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    {
        std::string defaultId;
        if (IMMDevice* def = nullptr; SUCCEEDED(en->GetDefaultAudioEndpoint(flow, eConsole, &def)) && def)
        {
            LPWSTR id = nullptr;
            if (SUCCEEDED(def->GetId(&id))) { defaultId = wideToUtf8(id); CoTaskMemFree(id); }
            def->Release();
        }

        IMMDeviceCollection* coll = nullptr;
        if (SUCCEEDED(en->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &coll)) && coll)
        {
            UINT count = 0;
            coll->GetCount(&count);
            for (UINT i = 0; i < count; ++i)
            {
                IMMDevice* d = nullptr;
                if (FAILED(coll->Item(i, &d)) || !d) continue;

                OutputDevice dev;
                LPWSTR id = nullptr;
                if (SUCCEEDED(d->GetId(&id))) { dev.id = wideToUtf8(id); CoTaskMemFree(id); }

                IPropertyStore* props = nullptr;
                if (SUCCEEDED(d->OpenPropertyStore(STGM_READ, &props)) && props)
                {
                    PROPVARIANT pv; PropVariantInit(&pv);
                    if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.vt == VT_LPWSTR)
                        dev.name = juce::String(wideToUtf8(pv.pwszVal));
                    PropVariantClear(&pv);

                    PROPVARIANT ff; PropVariantInit(&ff);
                    if (SUCCEEDED(props->GetValue(PKEY_AudioEndpoint_FormFactor, &ff)) && ff.vt == VT_UI4)
                        dev.type = formFactorLabel(ff.uintVal);
                    PropVariantClear(&ff);
                    props->Release();
                }
                if (dev.name.isEmpty())
                    dev.name = "(unknown device)";
                dev.isDefault = !dev.id.empty() && dev.id == defaultId;
                out.push_back(std::move(dev));
                d->Release();
            }
            coll->Release();
        }
        en->Release();
    }

    if (comInitedHere)
        CoUninitialize();
    return out;
}
} // namespace

std::vector<OutputDevice> listRenderEndpoints()  { return enumerate(eRender); }
std::vector<OutputDevice> listCaptureEndpoints() { return enumerate(eCapture); }

bool setDefaultRenderEndpoint(const std::string& endpointId)
{
    if (endpointId.empty())
        return false;
    const int n = MultiByteToWideChar(CP_UTF8, 0, endpointId.c_str(), -1, nullptr, 0);
    if (n <= 0)
        return false;
    std::wstring w((size_t) (n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, endpointId.c_str(), -1, w.data(), n);
    return setDefaultOutput(w);
}

} // namespace veyra::ui
