#pragma once
// PRIVATE IMPLEMENTATION CANDIDATE. No Core integration or runtime deployment.
// Define this macro to the existing SDK1 BandiVideo.h, never the SDK2 header.
#ifndef OPENDBO_BANDI_V1_HEADER
#define OPENDBO_BANDI_V1_HEADER "../BandiVideo.h"
#endif
#include OPENDBO_BANDI_V1_HEADER
#include <memory>

namespace opendbo_bandi_adapter {
class LegacyVideo21Adapter final : public ::IBandiVideo {
public:
    LegacyVideo21Adapter();
    ~LegacyVideo21Adapter();
    HRESULT Create(HWND window, LPVOID context = NULL,
        BVL_CONTEXT_TYPE contextType = BVL_CONTEXT_DIRECTX9) override;
    HRESULT Release() override;
    HRESULT Verify(LPCSTR id, LPCSTR key) override;
    UINT GetVersion() override;
    HRESULT Open(LPCWSTR path, BVL_OPEN_OPT option, BVL_PIXEL_FORMAT format,
        BVL_AUDIO_DEVICE audio) override;
    HRESULT Open(LPCSTR path, BVL_OPEN_OPT option, BVL_PIXEL_FORMAT format,
        BVL_AUDIO_DEVICE audio) override;
    HRESULT Open(IBandiVideoSource* source, BVL_OPEN_OPT option,
        BVL_PIXEL_FORMAT format, BVL_AUDIO_DEVICE audio) override;
    HRESULT Play(BOOL repeat) override;
    HRESULT Seek(double seconds) override;
    HRESULT Stop() override;
    HRESULT Close() override;
    HRESULT OpenAsync(LPCWSTR path, BVL_OPEN_OPT option,
        BVL_PIXEL_FORMAT format, BVL_AUDIO_DEVICE audio) override;
    HRESULT OpenAsync(LPCSTR path, BVL_OPEN_OPT option,
        BVL_PIXEL_FORMAT format, BVL_AUDIO_DEVICE audio) override;
    HRESULT OpenAsync(IBandiVideoSource* source, BVL_OPEN_OPT option,
        BVL_PIXEL_FORMAT format, BVL_AUDIO_DEVICE audio) override;
    HRESULT PlayAsync(BOOL repeat) override;
    HRESULT SeekAsync(double seconds) override;
    HRESULT StopAsync() override;
    HRESULT CloseAsync() override;
    HRESULT Pause() override;
    HRESULT Resume() override;
    HRESULT SetVolume(UINT volume) override;
    HRESULT SetAlpha(UINT alpha) override;
    BOOL IsPlaying() override;
    LPVOID GetLastFrame() override;
    HRESULT GetLastFrame(void** renderTarget) override;
    HRESULT GetVideoInfo(BVLVideoInfo& info) override;
    HRESULT ReleaseLastFrame(void* renderTarget) override;
    BOOL CheckFrame() override;
    double FramesToTime(INT frames) override;
    INT TimeToFrames(double seconds) override;

    // Proposed bridge-only hooks. Their implementation and consumer call sites
    // are part of later integration, not features supplied by SDK2 itself.
    HRESULT BeforeDeviceReset();
    HRESULT AfterDeviceReset();
    bool ConsumeCurrentEofNotification(WPARAM instance, LPARAM generation);
    bool ConsumeCurrentOpenedNotification(WPARAM instance, LPARAM generation);
private:
    LegacyVideo21Adapter(const LegacyVideo21Adapter&) = delete;
    LegacyVideo21Adapter& operator=(const LegacyVideo21Adapter&) = delete;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Internal static C++ facade factory matching only the existing caller type.
// It is not the SDK2 DLL export and must never cast that export to this type.
HRESULT CreateLegacyVideo21(DWORD requestedSdk1Version, void** legacyInterface);
}
