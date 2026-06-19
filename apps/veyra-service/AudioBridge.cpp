#include "AudioBridge.h"

#include <chrono>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Define the property-key/GUID symbols (e.g. PKEY_Device_FriendlyName) in this
// TU. This is the only TU that uses the extern PROPERTYKEY symbols, so there is
// no ODR clash with the __uuidof-based usage elsewhere.
#include <initguid.h>
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include "ConfigDsp.h"
#include "chain/DspChain.h"
#include "veyra/Logging.h"

namespace veyra::service {

namespace {
template <class T>
void safeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

std::wstring utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0);
    std::wstring w((size_t) n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), w.data(), n);
    return w;
}

std::string wideToUtf8(const wchar_t* w)
{
    if (!w) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s((size_t) (n > 0 ? n - 1 : 0), '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}

bool isFloat32(const WAVEFORMATEX* f)
{
    if (!f || f->wBitsPerSample != 32) return false;
    if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
    return f->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f)->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT;
}

// device == nullptr/empty id -> default render endpoint.
IMMDevice* openDevice(IMMDeviceEnumerator* en, const std::string& id)
{
    IMMDevice* dev = nullptr;
    if (id.empty())
        en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
    else
        en->GetDevice(utf8ToWide(id).c_str(), &dev);
    return dev;
}

void logRenderEndpoints(IMMDeviceEnumerator* en, Logger* log)
{
    if (!log) return;
    IMMDeviceCollection* coll = nullptr;
    if (FAILED(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &coll)) || !coll)
        return;
    UINT count = 0;
    coll->GetCount(&count);
    for (UINT i = 0; i < count; ++i)
    {
        IMMDevice* d = nullptr;
        if (FAILED(coll->Item(i, &d)) || !d) continue;
        LPWSTR id = nullptr;
        d->GetId(&id);
        IPropertyStore* props = nullptr;
        std::string name;
        if (SUCCEEDED(d->OpenPropertyStore(STGM_READ, &props)) && props)
        {
            PROPVARIANT pv; PropVariantInit(&pv);
            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.vt == VT_LPWSTR)
                name = wideToUtf8(pv.pwszVal);
            PropVariantClear(&pv);
            props->Release();
        }
        log->info("AudioBridge: render endpoint \"" + name + "\" = " + wideToUtf8(id));
        if (id) CoTaskMemFree(id);
        d->Release();
    }
    coll->Release();
}
} // namespace

AudioBridge::~AudioBridge() { stop(); }

bool AudioBridge::start()
{
    running_.store(true);
    thread_ = std::thread(&AudioBridge::run, this);
    return true;
}

void AudioBridge::stop()
{
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

void AudioBridge::setConfig(const Config& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cfg_ = config;
    haveCfg_ = true;
}

void AudioBridge::run()
{
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    while (running_.load())
    {
        if (!session())
            std::this_thread::sleep_for(std::chrono::milliseconds(750)); // idle / back off
    }
    if (SUCCEEDED(comInit))
        CoUninitialize();
}

bool AudioBridge::session()
{
    Config cfg;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!haveCfg_) return false;
        cfg = cfg_;
    }
    if (!cfg.bridge.enabled)
        return false; // idle; run() backs off

    IMMDeviceEnumerator* en = nullptr;
    IMMDevice*           srcDev = nullptr;
    IMMDevice*           dstDev = nullptr;
    IAudioClient*        srcClient = nullptr;
    IAudioClient*        dstClient = nullptr;
    IAudioCaptureClient* capture = nullptr;
    IAudioRenderClient*  render = nullptr;
    WAVEFORMATEX*        srcFmt = nullptr;
    WAVEFORMATEX*        dstFmt = nullptr;
    bool ok = false;

    auto cleanup = [&]
    {
        if (srcClient) srcClient->Stop();
        if (dstClient) dstClient->Stop();
        if (srcFmt) CoTaskMemFree(srcFmt);
        if (dstFmt) CoTaskMemFree(dstFmt);
        safeRelease(capture); safeRelease(render);
        safeRelease(srcClient); safeRelease(dstClient);
        safeRelease(srcDev); safeRelease(dstDev); safeRelease(en);
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    { cleanup(); return false; }

    logRenderEndpoints(en, log_); // help the user discover device ids

    srcDev = openDevice(en, cfg.bridge.sourceDeviceId);
    dstDev = openDevice(en, cfg.bridge.targetDeviceId);
    if (!srcDev || !dstDev)
    { cleanup(); return false; }

    if (FAILED(srcDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&srcClient))) ||
        FAILED(dstDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&dstClient))))
    { cleanup(); return false; }

    if (FAILED(srcClient->GetMixFormat(&srcFmt)) || FAILED(dstClient->GetMixFormat(&dstFmt)))
    { cleanup(); return false; }

    if (!isFloat32(srcFmt) || !isFloat32(dstFmt) ||
        srcFmt->nChannels != 2 || dstFmt->nChannels != 2 ||
        srcFmt->nSamplesPerSec != dstFmt->nSamplesPerSec)
    {
        if (log_)
            log_->warn("AudioBridge: source/target must both be stereo 32-bit float at the same rate (no resampling yet)");
        cleanup();
        return false;
    }

    const REFERENCE_TIME dur = 1000000; // 100 ms
    if (FAILED(srcClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                     dur, 0, srcFmt, nullptr)) ||
        FAILED(srcClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture))))
    { cleanup(); return false; }

    if (FAILED(dstClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, dur, 0, dstFmt, nullptr)) ||
        FAILED(dstClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render))))
    { cleanup(); return false; }

    UINT32 dstBufFrames = 0;
    dstClient->GetBufferSize(&dstBufFrames);

    dsp::DspChain chain;
    chain.prepare(srcFmt->nSamplesPerSec);

    if (FAILED(srcClient->Start()) || FAILED(dstClient->Start()))
    { cleanup(); return false; }

    std::vector<float> left, right;

    while (running_.load())
    {
        // Live config: bail out (clean) if disabled or the routing changed.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!cfg_.bridge.enabled ||
                cfg_.bridge.sourceDeviceId != cfg.bridge.sourceDeviceId ||
                cfg_.bridge.targetDeviceId != cfg.bridge.targetDeviceId)
            { ok = true; break; }
            chain.setParameters(dspParamsFromConfig(cfg_));
        }

        UINT32 packet = 0;
        if (FAILED(capture->GetNextPacketSize(&packet)))
            break;

        while (packet > 0)
        {
            BYTE*  data = nullptr;
            UINT32 frames = 0;
            DWORD  flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
                break;

            left.resize(frames);
            right.resize(frames);
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            const auto* in = reinterpret_cast<const float*>(data);
            for (UINT32 i = 0; i < frames; ++i)
            {
                left[i]  = (silent || !in) ? 0.0f : in[i * 2];
                right[i] = (silent || !in) ? 0.0f : in[i * 2 + 1];
            }

            chain.processStereo(left.data(), right.data(), static_cast<int>(frames));

            // Render to the target (drop any overflow rather than block).
            UINT32 pad = 0;
            dstClient->GetCurrentPadding(&pad);
            const UINT32 avail = (dstBufFrames > pad) ? dstBufFrames - pad : 0;
            const UINT32 toWrite = (frames < avail) ? frames : avail;
            BYTE* out = nullptr;
            if (toWrite > 0 && SUCCEEDED(render->GetBuffer(toWrite, &out)) && out)
            {
                auto* of = reinterpret_cast<float*>(out);
                for (UINT32 i = 0; i < toWrite; ++i)
                {
                    of[i * 2]     = left[i];
                    of[i * 2 + 1] = right[i];
                }
                render->ReleaseBuffer(toWrite, 0);
            }

            capture->ReleaseBuffer(frames);
            if (FAILED(capture->GetNextPacketSize(&packet)))
            { packet = 0; break; }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    cleanup();
    return ok;
}

} // namespace veyra::service
