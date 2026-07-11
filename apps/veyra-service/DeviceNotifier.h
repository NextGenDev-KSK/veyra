#pragma once

// Endpoint change watcher: registers an IMMNotificationClient on its own
// MTA thread and fires a callback whenever audio devices are added, removed,
// change state, or the default endpoint moves. The bridges use this to retry
// immediately (kick) instead of waiting out their idle backoff, so unplugging
// headphones or a cable arriving is picked up within a session cycle.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <functional>
#include <thread>

namespace veyra::service {

class DeviceNotifier {
public:
    using Callback = std::function<void()>;

    explicit DeviceNotifier(Callback onChange) : onChange_(std::move(onChange)) {}
    ~DeviceNotifier() { stop(); }

    void start()
    {
        if (running_.exchange(true))
            return;
        stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        thread_ = std::thread(&DeviceNotifier::run, this);
    }

    void stop()
    {
        if (!running_.exchange(false))
            return;
        if (stopEvent_) SetEvent(stopEvent_);
        if (thread_.joinable()) thread_.join();
        if (stopEvent_) { CloseHandle(stopEvent_); stopEvent_ = nullptr; }
    }

private:
    // COM notification sink. Lifetime is bounded by the owning DeviceNotifier
    // (registered and unregistered on the same thread), so ref-counting is inert.
    class Sink : public IMMNotificationClient {
    public:
        explicit Sink(DeviceNotifier* owner) : owner_(owner) {}

        // IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
        {
            if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient))
            { *ppv = static_cast<IMMNotificationClient*>(this); return S_OK; }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        STDMETHODIMP_(ULONG) AddRef() override { return 2; }
        STDMETHODIMP_(ULONG) Release() override { return 1; }

        // IMMNotificationClient
        STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) override { owner_->fire(); return S_OK; }
        STDMETHODIMP OnDeviceAdded(LPCWSTR) override { owner_->fire(); return S_OK; }
        STDMETHODIMP OnDeviceRemoved(LPCWSTR) override { owner_->fire(); return S_OK; }
        STDMETHODIMP OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override { owner_->fire(); return S_OK; }
        STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

    private:
        DeviceNotifier* owner_;
    };

    void fire()
    {
        if (running_.load() && onChange_)
            onChange_();
    }

    void run()
    {
        const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IMMDeviceEnumerator* en = nullptr;
        Sink sink(this);
        const bool registered =
            SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                       __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&en)))
            && en != nullptr
            && SUCCEEDED(en->RegisterEndpointNotificationCallback(&sink));

        if (stopEvent_)
            WaitForSingleObject(stopEvent_, INFINITE);

        if (registered)
            en->UnregisterEndpointNotificationCallback(&sink);
        if (en)
            en->Release();
        if (SUCCEEDED(comInit))
            CoUninitialize();
    }

    Callback          onChange_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
    HANDLE            stopEvent_ = nullptr;
};

} // namespace veyra::service
