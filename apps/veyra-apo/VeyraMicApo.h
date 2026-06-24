#pragma once

// Veyra microphone (capture) Audio Processing Object.
//
// Mirrors the render-side VeyraApoEfx but runs the mono veyra::dsp::VoiceChain
// over the capture stream, reading its parameters from the service's mic
// shared-memory block. Mics are mono in the overwhelming majority of cases;
// non-mono formats fall through as a clean passthrough. APOProcess is
// allocation- and lock-free.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmreg.h>
#include <audioenginebaseapo.h>
#include <audioapotypes.h>
#include <audiomediatype.h>

#include <atomic>

#include "veyra/RnnoiseDenoiser.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/SharedParameters.h"
#include "voice/VoiceChain.h"

// {B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}
class __declspec(uuid("B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12")) VeyraMicApo;
extern const CLSID CLSID_VeyraMicApo;

class VeyraMicApo final
    : public IAudioProcessingObjectRT
    , public IAudioProcessingObject
    , public IAudioProcessingObjectConfiguration
    , public IAudioSystemEffects
{
public:
    VeyraMicApo();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // IAudioProcessingObject
    STDMETHOD(Reset)() override;
    STDMETHOD(GetLatency)(HNSTIME* pTime) override;
    STDMETHOD(GetRegistrationProperties)(APO_REG_PROPERTIES** ppRegProps) override;
    STDMETHOD(Initialize)(UINT32 cbDataSize, BYTE* pbyData) override;
    STDMETHOD(IsInputFormatSupported)(IAudioMediaType* pOppositeFormat,
                                      IAudioMediaType* pRequestedInputFormat,
                                      IAudioMediaType** ppSupportedInputFormat) override;
    STDMETHOD(IsOutputFormatSupported)(IAudioMediaType* pOppositeFormat,
                                       IAudioMediaType* pRequestedOutputFormat,
                                       IAudioMediaType** ppSupportedOutputFormat) override;
    STDMETHOD(GetInputChannelCount)(UINT32* pu32ChannelCount) override;

    // IAudioProcessingObjectRT
    STDMETHOD_(void, APOProcess)(UINT32 u32NumInputConnections,
                                 APO_CONNECTION_PROPERTY** ppInputConnections,
                                 UINT32 u32NumOutputConnections,
                                 APO_CONNECTION_PROPERTY** ppOutputConnections) override;
    STDMETHOD_(UINT32, CalcInputFrames)(UINT32 u32OutputFrameCount) override;
    STDMETHOD_(UINT32, CalcOutputFrames)(UINT32 u32InputFrameCount) override;

    // IAudioProcessingObjectConfiguration
    STDMETHOD(LockForProcess)(UINT32 u32NumInputConnections,
                              APO_CONNECTION_DESCRIPTOR** ppInputConnections,
                              UINT32 u32NumOutputConnections,
                              APO_CONNECTION_DESCRIPTOR** ppOutputConnections) override;
    STDMETHOD(UnlockForProcess)() override;

private:
    ~VeyraMicApo();

    static HRESULT validateFloatFormat(IAudioMediaType* format);
    void refreshParametersFromShared() noexcept;

    std::atomic<ULONG> ref_{1};

    bool   locked_ = false;
    bool   denormalsSet_ = false; // FTZ+DAZ set once on the audiodg capture thread
    UINT32 channelCount_ = 1;
    float  sampleRate_ = 48000.0f;
    UINT32 maxFrameCount_ = 0;

    veyra::dsp::VoiceChain voice_;
    veyra::RnnoiseDenoiser rnnoise_; // default mic NS when available; else VoiceChain's

    veyra::ipc::SharedMemoryRegion              paramRegion_;
    const veyra::ipc::VeyraMicSharedParameters* sharedParams_ = nullptr;
    veyra::ipc::VeyraMicParamsPayload           appliedParams_{};
    bool                                        haveApplied_ = false;
};
