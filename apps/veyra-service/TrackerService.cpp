#include "TrackerService.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include "analyzer/Analyzer.h"
#include "tracker/SoundTracker.h"
#include "veyra/Logging.h"

namespace veyra::service {

namespace {
template <class T>
void safeRelease(T*& p)
{
    if (p) { p->Release(); p = nullptr; }
}

double nowSeconds()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

ipc::TrackerEventType toWire(dsp::SoundClass c)
{
    switch (c)
    {
    case dsp::SoundClass::Footstep: return ipc::TrackerEventType::Footstep;
    case dsp::SoundClass::Gunshot:  return ipc::TrackerEventType::Gunshot;
    case dsp::SoundClass::Voice:    return ipc::TrackerEventType::Voice;
    case dsp::SoundClass::Other:    return ipc::TrackerEventType::Other;
    default:                        return ipc::TrackerEventType::None;
    }
}
} // namespace

TrackerService::~TrackerService()
{
    stop();
}

bool TrackerService::start()
{
    if (!region_.create(ipc::kSharedTrackerName, sizeof(ipc::VeyraTrackerData)))
    {
        if (log_)
            log_->error("TrackerService: failed to create tracker shared block");
        return false;
    }
    data_ = static_cast<ipc::VeyraTrackerData*>(region_.data());
    if (region_.createdNew())
        data_->writeCount.store(0, std::memory_order_release);

    // Live metering block (best-effort; the visualizer just stays idle without it).
    if (analyzerRegion_.create(ipc::kSharedAnalyzerName, sizeof(ipc::VeyraAnalyzerData)))
    {
        analyzerData_ = static_cast<ipc::VeyraAnalyzerData*>(analyzerRegion_.data());
        if (analyzerRegion_.createdNew())
            ipc::publishAnalyzer(analyzerData_, ipc::VeyraAnalyzerPayload{});
    }

    running_.store(true);
    thread_ = std::thread(&TrackerService::run, this);
    if (log_)
        log_->info("TrackerService: started");
    return true;
}

void TrackerService::stop()
{
    if (!running_.exchange(false))
        return;
    if (thread_.joinable())
        thread_.join();
}

void TrackerService::setConfig(const GamerModeConfig& gm)
{
    enabled_.store(gm.enabled, std::memory_order_relaxed);
    sensitivity_.store(gm.sensitivity, std::memory_order_relaxed);
}

void TrackerService::publishAnalyzerFrame(const dsp::AnalyzerFrame& fr)
{
    ipc::VeyraAnalyzerPayload p;
    p.vuL = fr.rmsL; p.vuR = fr.rmsR;
    p.peakL = fr.peakL; p.peakR = fr.peakR;
    p.clip = fr.clipped ? 1u : 0u;

    // Log-spaced downsample of the 256-bin spectrum to the visualizer's 48 bars,
    // taking the peak magnitude per band.
    const int nbins = dsp::kAnalyzerBins;
    float raw[ipc::kAnalyzerBars] = {};
    float frameMax = 1.0e-6f;
    for (int b = 0; b < ipc::kAnalyzerBars; ++b)
    {
        const float f0 = (float) b / ipc::kAnalyzerBars;
        const float f1 = (float) (b + 1) / ipc::kAnalyzerBars;
        int lo = (int) std::pow((float) (nbins - 1), f0);
        int hi = std::max(lo + 1, (int) std::pow((float) (nbins - 1), f1));
        hi = std::min(hi, nbins);
        float m = 0.0f;
        for (int k = lo; k < hi; ++k)
            m = std::max(m, fr.spectrum[k]);
        raw[b] = m;
        frameMax = std::max(frameMax, m);
    }

    // Auto-gain: normalise to a slowly-decaying running max so the spectrum
    // fills the meter regardless of absolute level.
    specMax_ = std::max(frameMax, specMax_ * 0.995f);
    for (int b = 0; b < ipc::kAnalyzerBars; ++b)
        p.bars[b] = std::min(1.0f, raw[b] / (specMax_ + 1.0e-6f));

    ipc::publishAnalyzer(analyzerData_, p);
}

void TrackerService::run()
{
    // The capture thread is its own COM apartment.
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Capture runs continuously so the visualizer always has live metering; the
    // Gamer Mode flag only gates whether tracker detections are emitted.
    while (running_.load())
    {
        if (!captureSession())
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // back off, then retry
    }

    if (SUCCEEDED(comInit))
        CoUninitialize();
}

bool TrackerService::captureSession()
{
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice*           device = nullptr;
    IAudioClient*        client = nullptr;
    IAudioCaptureClient* capture = nullptr;
    WAVEFORMATEX*        fmt = nullptr;
    bool ok = false;

    auto cleanup = [&]
    {
        if (client) client->Stop();
        if (fmt) CoTaskMemFree(fmt);
        safeRelease(capture);
        safeRelease(client);
        safeRelease(device);
        safeRelease(enumerator);
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
    { cleanup(); return false; }

    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)))
    { cleanup(); return false; }

    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&client))))
    { cleanup(); return false; }

    if (FAILED(client->GetMixFormat(&fmt)) || !fmt)
    { cleanup(); return false; }

    // We only analyse 32-bit float mixes (shared-mode default). Bail otherwise.
    const bool isFloat =
        fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         reinterpret_cast<WAVEFORMATEXTENSIBLE*>(fmt)->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT);
    if (!isFloat || fmt->wBitsPerSample != 32 || fmt->nChannels < 1)
    { cleanup(); return false; }

    const int channels = fmt->nChannels;
    const REFERENCE_TIME bufDuration = 2 * 10000 * 10; // ~200 ms (units: 100 ns)
    if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                  bufDuration, 0, fmt, nullptr)))
    { cleanup(); return false; }

    if (FAILED(client->GetService(__uuidof(IAudioCaptureClient),
                                  reinterpret_cast<void**>(&capture))))
    { cleanup(); return false; }

    dsp::SoundTracker tracker;
    tracker.prepare(fmt->nSamplesPerSec);
    tracker.setSensitivity(sensitivity_.load(std::memory_order_relaxed));

    dsp::Analyzer analyzer;
    analyzer.prepare(fmt->nSamplesPerSec);

    if (FAILED(client->Start()))
    { cleanup(); return false; }

    std::vector<float> left, right;
    float lastSensitivity = sensitivity_.load(std::memory_order_relaxed);

    while (running_.load())
    {
        const float sens = sensitivity_.load(std::memory_order_relaxed);
        if (sens != lastSensitivity)
        {
            tracker.setSensitivity(sens);
            lastSensitivity = sens;
        }

        UINT32 packet = 0;
        if (FAILED(capture->GetNextPacketSize(&packet)))
            break;

        while (packet > 0)
        {
            BYTE*  raw = nullptr;
            UINT32 frames = 0;
            DWORD  flags = 0;
            if (FAILED(capture->GetBuffer(&raw, &frames, &flags, nullptr, nullptr)))
                break;

            left.resize(frames);
            right.resize(frames);
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            const auto* samples = reinterpret_cast<const float*>(raw);
            for (UINT32 i = 0; i < frames; ++i)
            {
                if (silent || !samples)
                {
                    left[i] = right[i] = 0.0f;
                }
                else
                {
                    const float l = samples[i * channels];
                    const float r = channels > 1 ? samples[i * channels + 1] : l;
                    left[i] = l;
                    right[i] = r;
                }
            }

            // Live metering — always published for the UI visualizer.
            if (analyzerData_)
            {
                analyzer.processStereo(left.data(), right.data(), static_cast<int>(frames));
                dsp::AnalyzerFrame fr;
                while (analyzer.popFrame(fr))
                    publishAnalyzerFrame(fr);
            }

            // Tracker detections — only while Gamer Mode is on.
            if (enabled_.load(std::memory_order_relaxed))
            {
                tracker.processStereo(left.data(), right.data(), static_cast<int>(frames));
                dsp::TrackerEvent ev;
                while (tracker.popEvent(ev))
                {
                    ipc::TrackerEventRecord rec;
                    rec.type = static_cast<int32_t>(toWire(ev.type));
                    rec.azimuthDeg = ev.azimuthDeg;
                    rec.intensity = ev.intensity;
                    rec.confidence = ev.confidence;
                    rec.timestampSec = nowSeconds();
                    ipc::writeTrackerEvent(data_, rec);
                }
            }

            capture->ReleaseBuffer(frames);
            if (FAILED(capture->GetNextPacketSize(&packet)))
            { packet = 0; break; }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        ok = true;
    }

    cleanup();
    return ok;
}

} // namespace veyra::service
