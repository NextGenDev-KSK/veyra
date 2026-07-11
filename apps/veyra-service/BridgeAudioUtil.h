#pragma once

// Shared WASAPI helpers for the audio bridges (playback AudioBridge and
// MicBridge): format checks, endpoint opening, channel downmix, and a small
// single-threaded stereo FIFO that decouples capture packet sizes from render
// periods. Header-only; included from bridge .cpp files that already pull in
// the WASAPI headers.

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace veyra::service::bridgeutil {

template <class T>
void safeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

inline std::wstring utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0);
    std::wstring w((size_t) n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), w.data(), n);
    return w;
}

inline std::string wideToUtf8(const wchar_t* w)
{
    if (!w) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s((size_t) (n > 0 ? n - 1 : 0), '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}

inline bool isFloat32(const WAVEFORMATEX* f)
{
    if (!f || f->wBitsPerSample != 32) return false;
    if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
    return f->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f)->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT;
}

// device == nullptr/empty id -> the default endpoint for the given flow.
inline IMMDevice* openEndpoint(IMMDeviceEnumerator* en, EDataFlow flow, const std::string& id)
{
    IMMDevice* dev = nullptr;
    if (id.empty())
        en->GetDefaultAudioEndpoint(flow, eConsole, &dev);
    else
        en->GetDevice(utf8ToWide(id).c_str(), &dev);
    return dev;
}

inline bool sameEndpoint(IMMDevice* a, IMMDevice* b)
{
    if (!a || !b) return false;
    LPWSTR ia = nullptr, ib = nullptr;
    const bool same = SUCCEEDED(a->GetId(&ia)) && SUCCEEDED(b->GetId(&ib))
                   && ia != nullptr && ib != nullptr && wcscmp(ia, ib) == 0;
    if (ia) CoTaskMemFree(ia);
    if (ib) CoTaskMemFree(ib);
    return same;
}

inline WAVEFORMATEXTENSIBLE makeStereoFloat(DWORD sampleRate)
{
    WAVEFORMATEXTENSIBLE f{};
    f.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    f.Format.nChannels       = 2;
    f.Format.nSamplesPerSec  = sampleRate;
    f.Format.wBitsPerSample  = 32;
    f.Format.nBlockAlign     = (WORD) (f.Format.nChannels * f.Format.wBitsPerSample / 8);
    f.Format.nAvgBytesPerSec = f.Format.nSamplesPerSec * f.Format.nBlockAlign;
    f.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    f.Samples.wValidBitsPerSample = 32;
    f.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    f.SubFormat     = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    return f;
}

// Downmix an interleaved float32 stream of `ch` channels (standard WAVE order)
// to stereo. in == nullptr writes silence. Layouts: 1 = mono, 2 = stereo,
// 4 = quad, 6 = 5.1 (FL FR C LFE BL BR), 8 = 7.1 (+ SL SR); anything else
// falls back to the first one/two channels. LFE is dropped; C/back/side mix in
// at -3 dB with a -3 dB master trim for headroom (the chain's limiter guards
// the peaks regardless).
inline void downmixToStereo(const float* in, int ch, UINT32 frames, float* left, float* right)
{
    if (in == nullptr)
    {
        std::memset(left, 0, frames * sizeof(float));
        std::memset(right, 0, frames * sizeof(float));
        return;
    }
    constexpr float k = 0.7071f;
    switch (ch)
    {
    case 1:
        for (UINT32 i = 0; i < frames; ++i) { left[i] = right[i] = in[i]; }
        break;
    case 2:
        for (UINT32 i = 0; i < frames; ++i) { left[i] = in[i * 2]; right[i] = in[i * 2 + 1]; }
        break;
    case 4:
        for (UINT32 i = 0; i < frames; ++i)
        {
            const float* s = in + i * 4;
            left[i]  = (s[0] + k * s[2]) * k;
            right[i] = (s[1] + k * s[3]) * k;
        }
        break;
    case 6:
        for (UINT32 i = 0; i < frames; ++i)
        {
            const float* s = in + i * 6;
            left[i]  = (s[0] + k * s[2] + k * s[4]) * k;
            right[i] = (s[1] + k * s[2] + k * s[5]) * k;
        }
        break;
    case 8:
        for (UINT32 i = 0; i < frames; ++i)
        {
            const float* s = in + i * 8;
            left[i]  = (s[0] + k * s[2] + k * s[4] + k * s[6]) * k;
            right[i] = (s[1] + k * s[2] + k * s[5] + k * s[7]) * k;
        }
        break;
    default:
        for (UINT32 i = 0; i < frames; ++i)
        {
            const float* s = in + i * ch;
            left[i]  = s[0];
            right[i] = s[ch > 1 ? 1 : 0];
        }
        break;
    }
}

// Fixed-capacity stereo ring buffer; push planar, pop interleaved. Used from a
// single streaming thread, so no synchronisation. Overflow drops the oldest
// audio (keeps latency bounded instead of growing a backlog).
class StereoFifo {
public:
    explicit StereoFifo(size_t capacityFrames)
        : cap_(capacityFrames), l_(capacityFrames), r_(capacityFrames) {}

    void push(const float* l, const float* r, UINT32 frames)
    {
        for (UINT32 i = 0; i < frames; ++i)
        {
            if (count_ == cap_) // full: drop the oldest frame
            {
                head_ = (head_ + 1) % cap_;
                --count_;
            }
            const size_t w = (head_ + count_) % cap_;
            l_[w] = l[i];
            r_[w] = r[i];
            ++count_;
        }
    }

    // Pops up to `frames` into an interleaved stereo buffer; returns frames written.
    UINT32 pop(float* interleaved, UINT32 frames)
    {
        const UINT32 n = (UINT32) std::min<size_t>(frames, count_);
        for (UINT32 i = 0; i < n; ++i)
        {
            interleaved[i * 2]     = l_[head_];
            interleaved[i * 2 + 1] = r_[head_];
            head_ = (head_ + 1) % cap_;
        }
        count_ -= n;
        return n;
    }

    size_t available() const { return count_; }

private:
    size_t cap_;
    size_t head_ = 0;
    size_t count_ = 0;
    std::vector<float> l_, r_;
};

} // namespace veyra::service::bridgeutil
