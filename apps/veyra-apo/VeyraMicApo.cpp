#include "VeyraMicApo.h"

#include <cstring>
#include <immintrin.h>

#ifndef APOERR_FORMAT_NOT_SUPPORTED
#define APOERR_FORMAT_NOT_SUPPORTED HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#endif

extern std::atomic<LONG> g_apoObjectCount; // dllmain.cpp

using veyra::ipc::VeyraMicParamsPayload;
using veyra::ipc::VeyraMicSharedParameters;

VeyraMicApo::VeyraMicApo()
{
    g_apoObjectCount.fetch_add(1, std::memory_order_relaxed);
}

VeyraMicApo::~VeyraMicApo()
{
    g_apoObjectCount.fetch_sub(1, std::memory_order_relaxed);
}

// ---- IUnknown --------------------------------------------------------------

STDMETHODIMP VeyraMicApo::QueryInterface(REFIID riid, void** ppv)
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
    else if (riid == __uuidof(IAudioSystemEffects2))
        *ppv = static_cast<IAudioSystemEffects2*>(this);
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) VeyraMicApo::AddRef()
{
    return ref_.fetch_add(1, std::memory_order_relaxed) + 1;
}

STDMETHODIMP_(ULONG) VeyraMicApo::Release()
{
    const ULONG count = ref_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0)
        delete this;
    return count;
}

// ---- IAudioProcessingObject ------------------------------------------------

STDMETHODIMP VeyraMicApo::Reset()
{
    if (locked_)
    {
        voice_.prepare(sampleRate_);
        voice_.reset();
        rnnoise_.prepare(sampleRate_);
    }
    haveApplied_ = false;
    return S_OK;
}

STDMETHODIMP VeyraMicApo::GetLatency(HNSTIME* pTime)
{
    if (!pTime)
        return E_POINTER;
    *pTime = 0; // the voice chain adds no lookahead latency
    return S_OK;
}

STDMETHODIMP VeyraMicApo::GetRegistrationProperties(APO_REG_PROPERTIES** ppRegProps)
{
    if (!ppRegProps)
        return E_POINTER;

    auto* props = static_cast<APO_REG_PROPERTIES*>(CoTaskMemAlloc(sizeof(APO_REG_PROPERTIES)));
    if (!props)
        return E_OUTOFMEMORY;

    props->clsid = CLSID_VeyraMicApo;
    props->Flags = APO_FLAG_DEFAULT;
    wcscpy_s(props->szFriendlyName, ARRAYSIZE(props->szFriendlyName), L"Veyra Microphone Effect");
    wcscpy_s(props->szCopyrightInfo, ARRAYSIZE(props->szCopyrightInfo), L"Veyra Sounds (GPLv3)");
    props->u32MajorVersion = 0;
    props->u32MinorVersion = 6;
    props->u32MinInputConnections = 1;
    props->u32MaxInputConnections = 1;
    props->u32MinOutputConnections = 1;
    props->u32MaxOutputConnections = 1;
    props->u32MaxInstances = 0xFFFFFFFF;
    props->u32NumAPOInterfaces = 1;
    props->iidAPOInterfaceList[0] = __uuidof(IAudioSystemEffects2);

    *ppRegProps = props;
    return S_OK;
}

STDMETHODIMP VeyraMicApo::Initialize(UINT32 /*cbDataSize*/, BYTE* /*pbyData*/)
{
    if (paramRegion_.open(veyra::ipc::kSharedMicParametersName, sizeof(VeyraMicSharedParameters)))
        sharedParams_ = static_cast<const VeyraMicSharedParameters*>(paramRegion_.data());
    return S_OK;
}

HRESULT VeyraMicApo::validateFloatFormat(IAudioMediaType* format)
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
        if (ext->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT && wf->wBitsPerSample == 32)
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP VeyraMicApo::IsInputFormatSupported(IAudioMediaType* /*pOppositeFormat*/,
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

STDMETHODIMP VeyraMicApo::IsOutputFormatSupported(IAudioMediaType* /*pOppositeFormat*/,
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

STDMETHODIMP VeyraMicApo::GetInputChannelCount(UINT32* pu32ChannelCount)
{
    if (!pu32ChannelCount)
        return E_POINTER;
    *pu32ChannelCount = channelCount_;
    return S_OK;
}

// ---- IAudioProcessingObjectConfiguration -----------------------------------

STDMETHODIMP VeyraMicApo::LockForProcess(UINT32 u32NumInputConnections,
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
        sampleRate_   = static_cast<float>(wf->nSamplesPerSec);
    }
    maxFrameCount_ = ppOutputConnections[0]->u32MaxFrameCount;

    voice_.prepare(sampleRate_);
    voice_.reset();
    rnnoise_.prepare(sampleRate_);
    haveApplied_ = false;
    refreshParametersFromShared();

    locked_ = true;
    return S_OK;
}

STDMETHODIMP VeyraMicApo::UnlockForProcess()
{
    locked_ = false;
    return S_OK;
}

// ---- IAudioSystemEffects2 --------------------------------------------------

STDMETHODIMP VeyraMicApo::GetEffectsList(LPGUID* ppEffectsIds, UINT* pcEffects,
                                          HANDLE /*hEvent*/)
{
    if (!ppEffectsIds || !pcEffects)
        return E_POINTER;
    *ppEffectsIds = nullptr;
    *pcEffects    = 0;
    return S_OK;
}

// ---- IAudioProcessingObjectRT (real-time; no alloc, no locks) ---------------

void VeyraMicApo::refreshParametersFromShared() noexcept
{
    if (!sharedParams_)
        return;

    VeyraMicParamsPayload p;
    if (!veyra::ipc::readMicParameters(sharedParams_, p))
        return; // writer thrashing — keep last applied

    if (haveApplied_ && std::memcmp(&p, &appliedParams_, sizeof(p)) == 0)
        return; // unchanged

    appliedParams_ = p;
    haveApplied_   = true;

    veyra::dsp::VoiceParams vp;
    vp.enabled          = p.enabled != 0;
    vp.highPassHz       = p.highPassHz;
    vp.noiseSuppression = p.noiseSuppression;
    vp.compressionAmount = p.compressionAmount;
    vp.deEssAmount      = p.deEssAmount;
    vp.presenceDb       = p.presenceDb;
    vp.outputGainDb     = p.outputGainDb;
    vp.sideToneLevel    = p.sideToneLevel;
    vp.agc              = p.agc != 0;
    // RNNoise (when active) is the noise-suppression stage; disable the chain's
    // expander so they don't fight. Otherwise VoiceChain's suppressor is used.
    if (rnnoise_.active())
        vp.noiseSuppression = 0.0f;
    voice_.setParams(vp);
}

STDMETHODIMP_(void) VeyraMicApo::APOProcess(UINT32 u32NumInputConnections,
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

    if (!denormalsSet_)
    {
        _mm_setcsr(_mm_getcsr() | 0x8040u); // FTZ+DAZ: prevents subnormal CPU stalls
        denormalsSet_ = true;
    }

    if (channelCount_ == 1 && locked_ && frames <= maxFrameCount_)
    {
        refreshParametersFromShared();
        if (dst != src)
            std::memcpy(dst, src, static_cast<size_t>(frames) * sizeof(float));
        rnnoise_.processMono(dst, static_cast<int>(frames)); // default NS (no-op if unavailable)
        voice_.processMono(dst, static_cast<int>(frames));
    }
    else
    {
        // Non-mono or not locked: clean passthrough.
        if (dst != src)
            std::memcpy(dst, src, static_cast<size_t>(frames) * channelCount_ * sizeof(float));
    }

    out->u32ValidFrameCount = frames;
    out->u32BufferFlags = BUFFER_VALID;
}

STDMETHODIMP_(UINT32) VeyraMicApo::CalcInputFrames(UINT32 u32OutputFrameCount)
{
    return u32OutputFrameCount;
}

STDMETHODIMP_(UINT32) VeyraMicApo::CalcOutputFrames(UINT32 u32InputFrameCount)
{
    return u32InputFrameCount;
}
