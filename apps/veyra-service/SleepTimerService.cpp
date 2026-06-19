#include "SleepTimerService.h"

#include <chrono>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include "loudness/SleepTimer.h"
#include "veyra/Logging.h"

namespace veyra::service {

namespace {
IAudioEndpointVolume* acquireEndpointVolume()
{
    IMMDeviceEnumerator*  en = nullptr;
    IMMDevice*            dev = nullptr;
    IAudioEndpointVolume* vol = nullptr;

    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                   __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en))) &&
        SUCCEEDED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)))
    {
        dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                      reinterpret_cast<void**>(&vol));
    }
    if (dev) dev->Release();
    if (en) en->Release();
    return vol; // caller releases
}
} // namespace

SleepTimerService::~SleepTimerService()
{
    stop();
}

bool SleepTimerService::start()
{
    running_.store(true);
    thread_ = std::thread(&SleepTimerService::run, this);
    return true;
}

void SleepTimerService::stop()
{
    if (!running_.exchange(false))
        return;
    if (thread_.joinable())
        thread_.join();
}

void SleepTimerService::setConfig(const LoudnessConfig& loud)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cfg_ = loud;
}

void SleepTimerService::run()
{
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    dsp::SleepTimer       timer;
    IAudioEndpointVolume* vol = nullptr;
    float                 baseline = 1.0f;
    bool                  armed = false;
    bool                  changed = false; // did we alter the system volume?

    auto restore = [&]
    {
        if (vol && changed)
            vol->SetMasterVolumeLevelScalar(baseline, nullptr);
        if (vol) { vol->Release(); vol = nullptr; }
        changed = false;
    };

    while (running_.load())
    {
        LoudnessConfig c;
        { std::lock_guard<std::mutex> lock(mutex_); c = cfg_; }

        if (c.sleepTimerEnabled && !armed)
        {
            vol = acquireEndpointVolume();
            if (vol)
            {
                float b = 1.0f;
                vol->GetMasterVolumeLevelScalar(&b);
                baseline = b;
            }
            timer.configure(c.sleepTimerMinutes, c.sleepFadeSeconds);
            timer.start();
            armed = true;
            changed = false;
            if (log_)
                log_->info("SleepTimer: armed");
        }
        else if (!c.sleepTimerEnabled && armed)
        {
            restore();
            armed = false;
            if (log_)
                log_->info("SleepTimer: cancelled, volume restored");
        }

        if (armed && vol)
        {
            timer.advance(0.1);
            vol->SetMasterVolumeLevelScalar(baseline * timer.gain(), nullptr);
            changed = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    restore(); // never leave the system muted on shutdown

    if (SUCCEEDED(comInit))
        CoUninitialize();
}

} // namespace veyra::service
