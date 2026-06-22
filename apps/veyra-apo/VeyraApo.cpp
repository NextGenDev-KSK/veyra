#include "VeyraApo.h"

#include <cstring>
#include <new>

// Defined by the SDK's audioenginebaseapo.h; provide a safe fallback in case a
// given SDK header revision doesn't expose the macro.
#ifndef APOERR_FORMAT_NOT_SUPPORTED
#define APOERR_FORMAT_NOT_SUPPORTED HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#endif

// Defined in dllmain.cpp; keeps the DLL alive while objects exist.
extern std::atomic<LONG> g_apoObjectCount;

using veyra::ipc::VeyraParamsPayload;
using veyra::ipc::VeyraSharedParameters;

VeyraApoEfx::VeyraApoEfx()
{
    g_apoObjectCount.fetch_add(1, std::memory_order_relaxed);
}

VeyraApoEfx::~VeyraApoEfx()
{
    g_apoObjectCount.fetch_sub(1, std::memory_order_relaxed);
}

// ---- IUnknown --------------------------------------------------------------

STDMETHODIMP VeyraApoEfx::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    if (riid == __uuidof(IUnknown))
        *ppv = static_cast<IUnknown*>(static_cast<IAudioProcessingObject*>(this));
    else if (riid == __uuidof(IAudioProcessingObject))
        *ppv = static_cast<IAudioProcessingObject*>(this);
    else if (riid == __uuidof(IAudioProcessingObjectRT))
        *ppv = static_cast<IAudioProcessingObjectRT*>(this);
    else if (riid == __uuidof(IAudioProcessingObjectConfiguration))
        *ppv = static_cast<IAudioProcessingObjectConfiguration*>(this);
    else if (riid == __uuidof(IAudioSystemEffects))
        *ppv = static_cast<IAudioSystemEffects*>(this);
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) VeyraApoEfx::AddRef()
{
    return ref_.fetch_add(1, std::memory_order_relaxed) + 1;
}

STDMETHODIMP_(ULONG) VeyraApoEfx::Release()
{
    const ULONG count = ref_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0)
        delete this;
    return count;
}

// ---- IAudioProcessingObject ------------------------------------------------

STDMETHODIMP VeyraApoEfx::Reset()
{
    if (locked_)
        chain_.prepare(sampleRate_, static_cast<int>(maxFrameCount_));
    haveApplied_ = false;
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::GetLatency(HNSTIME* pTime)
{
    if (!pTime)
        return E_POINTER;
    const double sr = sampleRate_ > 0.0f ? sampleRate_ : 48000.0;
    *pTime = static_cast<HNSTIME>(chain_.latencySamples() * 10000000.0 / sr);
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::GetRegistrationProperties(APO_REG_PROPERTIES** ppRegProps)
{
    if (!ppRegProps)
        return E_POINTER;

    auto* props = static_cast<APO_REG_PROPERTIES*>(CoTaskMemAlloc(sizeof(APO_REG_PROPERTIES)));
    if (!props)
        return E_OUTOFMEMORY;

    props->clsid = CLSID_VeyraApoEfx;
    props->Flags = APO_FLAG_DEFAULT;
    wcscpy_s(props->szFriendlyName, ARRAYSIZE(props->szFriendlyName), L"Veyra Output Effect");
    wcscpy_s(props->szCopyrightInfo, ARRAYSIZE(props->szCopyrightInfo), L"Veyra Sounds (GPLv3)");
    props->u32MajorVersion = 0;
    props->u32MinorVersion = 2;
    props->u32MinInputConnections = 1;
    props->u32MaxInputConnections = 1;
    props->u32MinOutputConnections = 1;
    props->u32MaxOutputConnections = 1;
    props->u32MaxInstances = 0xFFFFFFFF;
    props->u32NumAPOInterfaces = 1;
    props->iidAPOInterfaceList[0] = __uuidof(IAudioSystemEffects);

    *ppRegProps = props;
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::Initialize(UINT32 /*cbDataSize*/, BYTE* /*pbyData*/)
{
    // Connect to the service's shared parameter block if present. Absence is
    // non-fatal: the APO simply runs with defaults until the service appears.
    if (paramRegion_.open(veyra::ipc::kSharedParametersName, sizeof(VeyraSharedParameters)))
        sharedParams_ = static_cast<const VeyraSharedParameters*>(paramRegion_.data());
    return S_OK;
}

HRESULT VeyraApoEfx::validateFloatFormat(IAudioMediaType* format)
{
    if (!format)
        return E_POINTER;

    const WAVEFORMATEX* wf = format->GetAudioFormat();
    if (!wf)
        return E_INVALIDARG;

    if (wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && wf->wBitsPerSample == 32)
        return S_OK;

    if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        wf->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
    {
        const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wf);
        // Compare only Data1 to avoid pulling in the KSDATAFORMAT GUID symbol.
        if (ext->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT && wf->wBitsPerSample == 32)
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP VeyraApoEfx::IsInputFormatSupported(IAudioMediaType* /*pOppositeFormat*/,
                                                 IAudioMediaType* pRequestedInputFormat,
                                                 IAudioMediaType** ppSupportedInputFormat)
{
    if (ppSupportedInputFormat)
        *ppSupportedInputFormat = nullptr;

    if (validateFloatFormat(pRequestedInputFormat) != S_OK)
        return APOERR_FORMAT_NOT_SUPPORTED;

    if (ppSupportedInputFormat)
    {
        pRequestedInputFormat->AddRef();
        *ppSupportedInputFormat = pRequestedInputFormat;
    }
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::IsOutputFormatSupported(IAudioMediaType* /*pOppositeFormat*/,
                                                  IAudioMediaType* pRequestedOutputFormat,
                                                  IAudioMediaType** ppSupportedOutputFormat)
{
    if (ppSupportedOutputFormat)
        *ppSupportedOutputFormat = nullptr;

    if (validateFloatFormat(pRequestedOutputFormat) != S_OK)
        return APOERR_FORMAT_NOT_SUPPORTED;

    if (ppSupportedOutputFormat)
    {
        pRequestedOutputFormat->AddRef();
        *ppSupportedOutputFormat = pRequestedOutputFormat;
    }
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::GetInputChannelCount(UINT32* pu32ChannelCount)
{
    if (!pu32ChannelCount)
        return E_POINTER;
    *pu32ChannelCount = channelCount_;
    return S_OK;
}

// ---- IAudioProcessingObjectConfiguration -----------------------------------

STDMETHODIMP VeyraApoEfx::LockForProcess(UINT32 u32NumInputConnections,
                                         APO_CONNECTION_DESCRIPTOR** ppInputConnections,
                                         UINT32 u32NumOutputConnections,
                                         APO_CONNECTION_DESCRIPTOR** ppOutputConnections)
{
    if (u32NumInputConnections < 1 || u32NumOutputConnections < 1 ||
        !ppInputConnections || !ppOutputConnections)
        return E_INVALIDARG;

    IAudioMediaType* fmt = ppOutputConnections[0]->pFormat;
    if (const WAVEFORMATEX* wf = fmt ? fmt->GetAudioFormat() : nullptr)
    {
        channelCount_ = wf->nChannels;
        sampleRate_ = static_cast<float>(wf->nSamplesPerSec);
    }

    maxFrameCount_ = ppOutputConnections[0]->u32MaxFrameCount;

    scratchL_.reset(new (std::nothrow) float[maxFrameCount_ ? maxFrameCount_ : 1]);
    scratchR_.reset(new (std::nothrow) float[maxFrameCount_ ? maxFrameCount_ : 1]);
    if (!scratchL_ || !scratchR_)
        return E_OUTOFMEMORY;

    chain_.prepare(sampleRate_, static_cast<int>(maxFrameCount_));
    haveApplied_ = false;
    refreshParametersFromShared();

    locked_ = true;
    return S_OK;
}

STDMETHODIMP VeyraApoEfx::UnlockForProcess()
{
    scratchL_.reset();
    scratchR_.reset();
    locked_ = false;
    return S_OK;
}

// ---- IAudioProcessingObjectRT (real-time; no alloc, no locks) ---------------

void VeyraApoEfx::refreshParametersFromShared() noexcept
{
    if (!sharedParams_)
        return;

    VeyraParamsPayload p;
    if (!veyra::ipc::readParameters(sharedParams_, p))
        return; // writer thrashing — keep last applied

    if (haveApplied_ && std::memcmp(&p, &appliedParams_, sizeof(p)) == 0)
        return; // unchanged

    appliedParams_ = p;
    haveApplied_ = true;

    veyra::dsp::DspParameters dp;
    dp.bypass = p.bypass != 0;
    dp.monoMode = p.monoMode != 0;
    dp.balance = p.balance;
    for (int i = 0; i < 10; ++i)
        dp.eqBandsDb[static_cast<size_t>(i)] = p.eqBandsDb[i];
    dp.bassBoostDb = p.bassBoostDb;
    dp.trebleDb = p.trebleDb;
    dp.compressionAmount = p.compressionAmount;
    dp.exciterAmount = p.exciterAmount;
    dp.saturationAmount = p.saturationAmount;
    dp.saturationMode = (int) p.saturationMode;
    dp.multibandWidth = p.multibandWidth;
    dp.transientAmount = p.transientAmount;
    dp.bassEnhanceAmount = p.bassEnhanceAmount;
    dp.stereoWidth = p.stereoWidth;
    dp.volumeGain = p.volumeGain;
    dp.crossfeedAmount = p.crossfeedAmount;
    dp.virtualizationAmount = p.virtualizationAmount;
    dp.nightModeAmount = p.nightModeAmount;
    dp.loudnessMatch = p.loudnessMatch != 0;
    dp.loudnessTargetLufs = p.loudnessTargetLufs;
    dp.equalLoudness = p.equalLoudness != 0;
    dp.referenceMode = p.referenceMode != 0;
    dp.headphoneSafe = p.headphoneSafe != 0;
    dp.nonlinearOversampling = p.nonlinearOversampling != 0;
    dp.limiterCeilingDb = p.limiterCeilingDb;
    chain_.setParameters(dp);
}

STDMETHODIMP_(void) VeyraApoEfx::APOProcess(UINT32 u32NumInputConnections,
                                            APO_CONNECTION_PROPERTY** ppInputConnections,
                                            UINT32 u32NumOutputConnections,
                                            APO_CONNECTION_PROPERTY** ppOutputConnections)
{
    if (u32NumInputConnections == 0 || u32NumOutputConnections == 0)
        return;

    APO_CONNECTION_PROPERTY* in = ppInputConnections[0];
    APO_CONNECTION_PROPERTY* out = ppOutputConnections[0];
    const UINT32 frames = in->u32ValidFrameCount;

    if (in->u32BufferFlags != BUFFER_VALID)
    {
        out->u32ValidFrameCount = frames;
        out->u32BufferFlags = in->u32BufferFlags; // propagate silence/invalid
        return;
    }

    auto* src = reinterpret_cast<float*>(in->pBuffer);
    auto* dst = reinterpret_cast<float*>(out->pBuffer);

    if (channelCount_ == 2 && scratchL_ && frames <= maxFrameCount_)
    {
        refreshParametersFromShared();

        for (UINT32 i = 0; i < frames; ++i)
        {
            scratchL_[i] = src[2 * i];
            scratchR_[i] = src[2 * i + 1];
        }
        chain_.processStereo(scratchL_.get(), scratchR_.get(), static_cast<int>(frames));
        for (UINT32 i = 0; i < frames; ++i)
        {
            dst[2 * i] = scratchL_[i];
            dst[2 * i + 1] = scratchR_[i];
        }
    }
    else
    {
        // Non-stereo or not locked: clean passthrough.
        std::memcpy(dst, src, static_cast<size_t>(frames) * channelCount_ * sizeof(float));
    }

    out->u32ValidFrameCount = frames;
    out->u32BufferFlags = BUFFER_VALID;
}

STDMETHODIMP_(UINT32) VeyraApoEfx::CalcInputFrames(UINT32 u32OutputFrameCount)
{
    return u32OutputFrameCount; // 1:1, no resampling
}

STDMETHODIMP_(UINT32) VeyraApoEfx::CalcOutputFrames(UINT32 u32InputFrameCount)
{
    return u32InputFrameCount;
}
