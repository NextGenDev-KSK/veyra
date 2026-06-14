#pragma once

// Veyra output (SFX/EFX-style) Audio Processing Object.
//
// Implements the core APO COM interfaces directly against the Windows SDK (no
// WDK base class), so it builds with only the base SDK. It runs the shared
// veyra::dsp::DspChain over the endpoint's float mix and reads its parameters
// from the service via a lock-free shared-memory seqlock. APOProcess is
// allocation- and lock-free; scratch buffers are sized in LockForProcess.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmreg.h>
#include <audioenginebaseapo.h>
#include <audioapotypes.h>
#include <audiomediatype.h>

#include <atomic>
#include <memory>

#include "chain/DspChain.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/SharedParameters.h"

// {7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}
class __declspec(uuid("7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44")) VeyraApoEfx;
extern const CLSID CLSID_VeyraApoEfx;

class VeyraApoEfx final
    : public IAudioProcessingObjectRT
    , public IAudioProcessingObject
    , public IAudioProcessingObjectConfiguration
    , public IAudioSystemEffects
{
public:
    VeyraApoEfx();

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
    ~VeyraApoEfx();

    static HRESULT validateFloatFormat(IAudioMediaType* format);
    void refreshParametersFromShared() noexcept;

    std::atomic<ULONG> ref_{1};

    bool   locked_ = false;
    UINT32 channelCount_ = 2;
    float  sampleRate_ = 48000.0f;
    UINT32 maxFrameCount_ = 0;

    veyra::dsp::DspChain        chain_;
    std::unique_ptr<float[]>    scratchL_;
    std::unique_ptr<float[]>    scratchR_;

    veyra::ipc::SharedMemoryRegion          paramRegion_;
    const veyra::ipc::VeyraSharedParameters* sharedParams_ = nullptr;
    veyra::ipc::VeyraParamsPayload           appliedParams_{};
    bool                                     haveApplied_ = false;
};
