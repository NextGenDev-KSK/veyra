#include "AudioBridge.h"

#include <chrono>
#include <cstring>
#include <filesystem>
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
#include <avrt.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include "BridgeAudioUtil.h"
#include "ConfigDsp.h"
#include "chain/DspChain.h"
#include "veyra/Logging.h"

namespace veyra::service {

namespace {

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
                name = bridgeutil::wideToUtf8(pv.pwszVal);
            PropVariantClear(&pv);
            props->Release();
        }
        log->info("AudioBridge: render endpoint \"" + name + "\" = " + bridgeutil::wideToUtf8(id));
        if (id) CoTaskMemFree(id);
        d->Release();
    }
    coll->Release();
}

// Measured MIT KEMAR set ships next to the service exe at hrtf/diffuse; if it's
// present the chain uses measured HRTFs, otherwise it falls back to synthetic.
std::filesystem::path kemarDir()
{
    wchar_t buf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return {};
    return std::filesystem::path(buf).parent_path() / "hrtf" / "diffuse";
}

} // namespace

AudioBridge::AudioBridge(Logger* log) : log_(log)
{
    kickEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr); // auto-reset
}

AudioBridge::~AudioBridge()
{
    stop();
    if (kickEvent_) { CloseHandle(kickEvent_); kickEvent_ = nullptr; }
}

bool AudioBridge::start()
{
    running_.store(true);
    thread_ = std::thread(&AudioBridge::run, this);
    return true;
}

void AudioBridge::stop()
{
    if (!running_.exchange(false)) return;
    kick(); // break out of the backoff wait promptly
    if (thread_.joinable()) thread_.join();
}

void AudioBridge::setConfig(const Config& config)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cfg_ = config;
        haveCfg_ = true;
    }
    kick(); // apply routing changes without waiting out the backoff
}

void AudioBridge::kick()
{
    if (kickEvent_)
        SetEvent(kickEvent_);
}

void AudioBridge::run()
{
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    while (running_.load())
    {
        if (!session())
        {
            // Idle / back off — but wake instantly on kick() (config change,
            // device arrival, power resume) instead of sleeping blind.
            if (kickEvent_)
                WaitForSingleObject(kickEvent_, 750);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
        }
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
    WAVEFORMATEX*        dstMixFmt = nullptr;
    HANDLE               renderEvent = nullptr;
    HANDLE               mmcss = nullptr;
    bool ok = false;

    auto cleanup = [&]
    {
        if (srcClient) srcClient->Stop();
        if (dstClient) dstClient->Stop();
        if (srcFmt) CoTaskMemFree(srcFmt);
        if (dstMixFmt) CoTaskMemFree(dstMixFmt);
        bridgeutil::safeRelease(capture); bridgeutil::safeRelease(render);
        bridgeutil::safeRelease(srcClient); bridgeutil::safeRelease(dstClient);
        bridgeutil::safeRelease(srcDev); bridgeutil::safeRelease(dstDev); bridgeutil::safeRelease(en);
        if (renderEvent) { CloseHandle(renderEvent); renderEvent = nullptr; }
        if (mmcss) { AvRevertMmThreadCharacteristics(mmcss); mmcss = nullptr; }
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    { cleanup(); return false; }

    logRenderEndpoints(en, log_); // help the user discover device ids

    srcDev = bridgeutil::openEndpoint(en, eRender, cfg.bridge.sourceDeviceId);
    dstDev = bridgeutil::openEndpoint(en, eRender, cfg.bridge.targetDeviceId);
    if (!srcDev || !dstDev)
    { cleanup(); return false; }

    // Same endpoint for capture and render would loop the bridge's own output
    // back into its capture (echo build-up), so refuse and idle instead.
    if (bridgeutil::sameEndpoint(srcDev, dstDev))
    {
        if (log_)
            log_->warn("AudioBridge: source and target resolve to the same endpoint; "
                       "idling to avoid doubled/feedback audio. Pick two different devices.");
        cleanup();
        return false;
    }

    if (FAILED(srcDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&srcClient))) ||
        FAILED(dstDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&dstClient))))
    { cleanup(); return false; }

    if (FAILED(srcClient->GetMixFormat(&srcFmt)) || FAILED(dstClient->GetMixFormat(&dstMixFmt)))
    { cleanup(); return false; }

    // Shared-mode mix formats are float32 in practice; the channel count varies
    // (mono/stereo/5.1/7.1). Non-float or exotic layouts idle with a log line.
    const int srcCh = srcFmt->nChannels;
    if (!bridgeutil::isFloat32(srcFmt) || srcCh < 1 || srcCh > 8)
    {
        if (log_)
            log_->warn("AudioBridge: unsupported source format (" + std::to_string(srcCh)
                       + " ch, " + std::to_string(srcFmt->wBitsPerSample)
                       + "-bit); expected 1-8 channel 32-bit float");
        cleanup();
        return false;
    }

    const REFERENCE_TIME srcDur = 1000000; // 100 ms capture cushion
    if (FAILED(srcClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                     srcDur, 0, srcFmt, nullptr)) ||
        FAILED(srcClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture))))
    { cleanup(); return false; }

    // Render an explicit stereo float32 stream at the source rate. When that
    // matches the target's mix format, open through IAudioClient3 at the audio
    // engine's minimum period (lowest latency Windows offers in shared mode);
    // otherwise fall back to a 20 ms event-driven stream with auto-conversion.
    WAVEFORMATEXTENSIBLE outFmt = bridgeutil::makeStereoFloat(srcFmt->nSamplesPerSec);

    renderEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!renderEvent) { cleanup(); return false; }

    bool lowLatency = false;
    {
        IAudioClient3* dst3 = nullptr;
        const bool mixMatches = bridgeutil::isFloat32(dstMixFmt)
                             && dstMixFmt->nChannels == 2
                             && dstMixFmt->nSamplesPerSec == outFmt.Format.nSamplesPerSec;
        if (mixMatches &&
            SUCCEEDED(dstClient->QueryInterface(__uuidof(IAudioClient3), reinterpret_cast<void**>(&dst3))))
        {
            UINT32 defP = 0, fund = 0, minP = 0, maxP = 0;
            if (SUCCEEDED(dst3->GetSharedModeEnginePeriod(&outFmt.Format, &defP, &fund, &minP, &maxP)) &&
                SUCCEEDED(dst3->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                            minP, &outFmt.Format, nullptr)))
            {
                lowLatency = true;
                if (log_)
                    log_->info("AudioBridge: low-latency stream at " + std::to_string(minP)
                               + " frames/period (" + std::to_string(outFmt.Format.nSamplesPerSec) + " Hz)");
            }
            dst3->Release();
        }
    }
    if (!lowLatency)
    {
        const REFERENCE_TIME dstDur = 200000; // 20 ms event-driven buffer
        if (FAILED(dstClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                         AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                                         AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                         AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                         dstDur, 0, &outFmt.Format, nullptr)))
        { cleanup(); return false; }
    }

    if (FAILED(dstClient->SetEventHandle(renderEvent)) ||
        FAILED(dstClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render))))
    { cleanup(); return false; }

    UINT32 dstBufFrames = 0;
    dstClient->GetBufferSize(&dstBufFrames);

    dsp::DspChain chain;
    chain.setHrtfDirectory(kemarDir()); // measured HRTF when packaged; synthetic otherwise
    chain.prepare(srcFmt->nSamplesPerSec);

    // Pre-fill one buffer of silence so the stream starts without an underrun.
    {
        BYTE* out = nullptr;
        if (SUCCEEDED(render->GetBuffer(dstBufFrames, &out)) && out)
            render->ReleaseBuffer(dstBufFrames, AUDCLNT_BUFFERFLAGS_SILENT);
    }

    if (FAILED(srcClient->Start()) || FAILED(dstClient->Start()))
    { cleanup(); return false; }

    // Glitch-resistant scheduling for the streaming loop.
    DWORD mmcssTask = 0;
    mmcss = AvSetMmThreadCharacteristicsW(L"Pro Audio", &mmcssTask);

    // FIFO between capture and render so packet sizes don't have to line up
    // with render periods. Pre-sized; the hot path never allocates.
    bridgeutil::StereoFifo fifo(dstBufFrames * 4 + 4096);
    std::vector<float> left(8192), right(8192);

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
            const auto pb = parametricBandsFromConfig(cfg_);
            chain.setParametricBands(pb.first, pb.second);
        }

        // Paced by the render engine; 40 ms cap keeps config polls responsive
        // even if the render device stalls.
        WaitForSingleObject(renderEvent, 40);

        // Drain every pending capture packet into the FIFO.
        UINT32 packet = 0;
        if (FAILED(capture->GetNextPacketSize(&packet)))
            break;
        while (packet > 0)
        {
            BYTE*  data = nullptr;
            UINT32 frames = 0;
            DWORD  flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
            { packet = 0; break; }

            if (frames > left.size()) { left.resize(frames); right.resize(frames); }
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
            bridgeutil::downmixToStereo(silent ? nullptr : reinterpret_cast<const float*>(data),
                                        srcCh, frames, left.data(), right.data());

            chain.processStereo(left.data(), right.data(), static_cast<int>(frames));
            fifo.push(left.data(), right.data(), frames);

            capture->ReleaseBuffer(frames);
            if (FAILED(capture->GetNextPacketSize(&packet)))
            { packet = 0; break; }
        }

        // Fill the render buffer from the FIFO (silence-pad when it runs dry).
        UINT32 pad = 0;
        if (FAILED(dstClient->GetCurrentPadding(&pad)))
            break;
        const UINT32 avail = (dstBufFrames > pad) ? dstBufFrames - pad : 0;
        if (avail > 0)
        {
            BYTE* out = nullptr;
            if (SUCCEEDED(render->GetBuffer(avail, &out)) && out)
            {
                auto* of = reinterpret_cast<float*>(out);
                const UINT32 got = fifo.pop(of, avail); // interleaved stereo
                for (UINT32 i = got; i < avail; ++i)    // pad the remainder
                { of[i * 2] = 0.0f; of[i * 2 + 1] = 0.0f; }
                render->ReleaseBuffer(avail, 0);
            }
        }
    }

    cleanup();
    return ok;
}

} // namespace veyra::service
