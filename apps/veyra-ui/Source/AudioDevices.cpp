#include "AudioDevices.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <initguid.h> // define PKEY_Device_FriendlyName in this TU
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

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

std::vector<OutputDevice> listRenderEndpoints()
{
    std::vector<OutputDevice> out;

    const bool comInitedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    IMMDeviceEnumerator* en = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                   __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    {
        std::string defaultId;
        if (IMMDevice* def = nullptr; SUCCEEDED(en->GetDefaultAudioEndpoint(eRender, eConsole, &def)) && def)
        {
            LPWSTR id = nullptr;
            if (SUCCEEDED(def->GetId(&id))) { defaultId = wideToUtf8(id); CoTaskMemFree(id); }
            def->Release();
        }

        IMMDeviceCollection* coll = nullptr;
        if (SUCCEEDED(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &coll)) && coll)
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

} // namespace veyra::ui
