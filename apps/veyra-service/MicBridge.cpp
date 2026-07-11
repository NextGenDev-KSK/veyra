#include "MicBridge.h"

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>

#include "BridgeAudioUtil.h"
#include "veyra/Logging.h"
#include "veyra/RnnoiseDenoiser.h"
#include "voice/VoiceChain.h"

namespace veyra::service {

namespace {

dsp::VoiceParams voiceParamsFromConfig(const Config& c)
{
    dsp::VoiceParams vp;
    vp.enabled           = c.voice.enabled;
    vp.highPassHz        = c.voice.highPassHz;
    vp.noiseSuppression  = c.voice.noiseSuppression;
    vp.compressionAmount = c.voice.compressionAmount;
    vp.deEssAmount       = c.voice.deEssAmount;
    vp.presenceDb        = c.voice.presenceDb;
    vp.outputGainDb      = c.voice.outputGainDb;
    vp.sideToneLevel     = c.voice.sideToneLevel;
    vp.agc               = c.voice.agc;
    return vp;
}

} // namespace

MicBridge::MicBridge(Logger* log) : log_(log)
{
    kickEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr); // auto-reset
}

MicBridge::~MicBridge()
{
    stop();
    if (kickEvent_) { CloseHandle(kickEvent_); kickEvent_ = nullptr; }
}

bool MicBridge::start()
{
    running_.store(true);
    thread_ = std::thread(&MicBridge::run, this);
    return true;
}

void MicBridge::stop()
{
    if (!running_.exchange(false)) return;
    kick();
    if (thread_.joinable()) thread_.join();
}

void MicBridge::setConfig(const Config& config)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cfg_ = config;
        haveCfg_ = true;
    }
    kick();
}

void MicBridge::kick()
{
    if (kickEvent_)
        SetEvent(kickEvent_);
}

void MicBridge::run()
{
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    while (running_.load())
    {
        if (!session())
        {
            if (kickEvent_)
                WaitForSingleObject(kickEvent_, 750);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
        }
    }
    if (SUCCEEDED(comInit))
        CoUninitialize();
}

bool MicBridge::session()
{
    Config cfg;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!haveCfg_) return false;
        cfg = cfg_;
    }
    if (!cfg.micBridge.enabled)
        return false;

    // The cleaned voice must land in a cable apps can pick as a microphone; an
    // empty target would render the mic to the default speakers, which is never
    // what anyone wants. Idle until the user picks a target.
    if (cfg.micBridge.targetDeviceId.empty())
        return false;

    // If the playback bridge captures the same endpoint we'd render into, apps'
    // audio and the mic would merge. Refuse; the UI explains the two-cable setup.
    if (cfg.bridge.enabled)
    {
        const std::string playbackSource = cfg.bridge.sourceDeviceId; // empty = default render
        if (!playbackSource.empty() && playbackSource == cfg.micBridge.targetDeviceId)
        {
            if (log_)
                log_->warn("MicBridge: target is the same cable the Audio Bridge captures; "
                           "idling. Use a second virtual cable (e.g. CABLE A) for the mic.");
            return false;
        }
    }

    IMMDeviceEnumerator* en = nullptr;
    IMMDevice*           micDev = nullptr;
    IMMDevice*           dstDev = nullptr;
    IAudioClient*        micClient = nullptr;
    IAudioClient*        dstClient = nullptr;
    IAudioCaptureClient* capture = nullptr;
    IAudioRenderClient*  render = nullptr;
    WAVEFORMATEX*        micFmt = nullptr;
    HANDLE               captureEvent = nullptr;
    HANDLE               mmcss = nullptr;
    bool ok = false;

    auto cleanup = [&]
    {
        if (micClient) micClient->Stop();
        if (dstClient) dstClient->Stop();
        if (micFmt) CoTaskMemFree(micFmt);
        bridgeutil::safeRelease(capture); bridgeutil::safeRelease(render);
        bridgeutil::safeRelease(micClient); bridgeutil::safeRelease(dstClient);
        bridgeutil::safeRelease(micDev); bridgeutil::safeRelease(dstDev); bridgeutil::safeRelease(en);
        if (captureEvent) { CloseHandle(captureEvent); captureEvent = nullptr; }
        if (mmcss) { AvRevertMmThreadCharacteristics(mmcss); mmcss = nullptr; }
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))))
    { cleanup(); return false; }

    micDev = bridgeutil::openEndpoint(en, eCapture, cfg.micBridge.micDeviceId);
    dstDev = bridgeutil::openEndpoint(en, eRender, cfg.micBridge.targetDeviceId);
    if (!micDev || !dstDev)
    { cleanup(); return false; }

    if (FAILED(micDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&micClient))) ||
        FAILED(dstDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&dstClient))))
    { cleanup(); return false; }

    if (FAILED(micClient->GetMixFormat(&micFmt)))
    { cleanup(); return false; }

    const int micCh = micFmt->nChannels;
    if (!bridgeutil::isFloat32(micFmt) || micCh < 1 || micCh > 8)
    {
        if (log_)
            log_->warn("MicBridge: unsupported mic format (" + std::to_string(micCh)
                       + " ch, " + std::to_string(micFmt->wBitsPerSample) + "-bit)");
        cleanup();
        return false;
    }

    captureEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!captureEvent) { cleanup(); return false; }

    // Event-driven capture keeps mic latency at the engine period (~10 ms).
    const REFERENCE_TIME micDur = 200000; // 20 ms cushion
    if (FAILED(micClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                     micDur, 0, micFmt, nullptr)) ||
        FAILED(micClient->SetEventHandle(captureEvent)) ||
        FAILED(micClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture))))
    { cleanup(); return false; }

    // Render the cleaned voice as stereo float at the mic rate; the cable
    // resamples via auto-conversion if its rate differs.
    WAVEFORMATEXTENSIBLE outFmt = bridgeutil::makeStereoFloat(micFmt->nSamplesPerSec);
    const REFERENCE_TIME dstDur = 200000; // 20 ms
    if (FAILED(dstClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                     AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                     AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                     dstDur, 0, &outFmt.Format, nullptr)) ||
        FAILED(dstClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render))))
    { cleanup(); return false; }

    UINT32 dstBufFrames = 0;
    dstClient->GetBufferSize(&dstBufFrames);

    // The same chain the capture APO runs. RNNoise engages at 48 kHz and
    // replaces the chain's own suppressor (they'd fight otherwise).
    dsp::VoiceChain voice;
    RnnoiseDenoiser rnnoise;
    voice.prepare(micFmt->nSamplesPerSec);
    rnnoise.prepare(micFmt->nSamplesPerSec);
    if (log_)
        log_->info(std::string("MicBridge: session at ") + std::to_string(micFmt->nSamplesPerSec)
                   + " Hz, RNNoise " + (rnnoise.active() ? "active" : "inactive (chain suppressor)"));

    // Pre-fill silence so the cable stream starts clean.
    {
        BYTE* out = nullptr;
        if (SUCCEEDED(render->GetBuffer(dstBufFrames, &out)) && out)
            render->ReleaseBuffer(dstBufFrames, AUDCLNT_BUFFERFLAGS_SILENT);
    }

    if (FAILED(micClient->Start()) || FAILED(dstClient->Start()))
    { cleanup(); return false; }

    DWORD mmcssTask = 0;
    mmcss = AvSetMmThreadCharacteristicsW(L"Pro Audio", &mmcssTask);

    std::vector<float> mono(8192), discard(8192);

    while (running_.load())
    {
        // Live config: exit cleanly when disabled or rerouted.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!cfg_.micBridge.enabled ||
                cfg_.micBridge.micDeviceId != cfg.micBridge.micDeviceId ||
                cfg_.micBridge.targetDeviceId != cfg.micBridge.targetDeviceId)
            { ok = true; break; }
            auto vp = voiceParamsFromConfig(cfg_);
            if (rnnoise.active())
                vp.noiseSuppression = 0.0f; // RNNoise owns the suppression stage
            voice.setParams(vp);
        }

        WaitForSingleObject(captureEvent, 40);

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

            if (frames > mono.size()) { mono.resize(frames); discard.resize(frames); }
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
            // Fold the mic to mono (downmix reuses the render-layout coefficients;
            // for a stereo mic that's a plain L/R average via the discard buffer).
            bridgeutil::downmixToStereo(silent ? nullptr : reinterpret_cast<const float*>(data),
                                        micCh, frames, mono.data(), discard.data());
            for (UINT32 i = 0; i < frames; ++i)
                mono[i] = 0.5f * (mono[i] + discard[i]);

            rnnoise.processMono(mono.data(), (int) frames);
            voice.processMono(mono.data(), (int) frames);

            // Duplicate to both cable channels; drop overflow rather than block.
            UINT32 pad = 0;
            dstClient->GetCurrentPadding(&pad);
            const UINT32 avail = (dstBufFrames > pad) ? dstBufFrames - pad : 0;
            const UINT32 toWrite = frames < avail ? frames : avail;
            BYTE* out = nullptr;
            if (toWrite > 0 && SUCCEEDED(render->GetBuffer(toWrite, &out)) && out)
            {
                auto* of = reinterpret_cast<float*>(out);
                for (UINT32 i = 0; i < toWrite; ++i)
                { of[i * 2] = mono[i]; of[i * 2 + 1] = mono[i]; }
                render->ReleaseBuffer(toWrite, 0);
            }

            capture->ReleaseBuffer(frames);
            if (FAILED(capture->GetNextPacketSize(&packet)))
            { packet = 0; break; }
        }
    }

    cleanup();
    return ok;
}

} // namespace veyra::service
