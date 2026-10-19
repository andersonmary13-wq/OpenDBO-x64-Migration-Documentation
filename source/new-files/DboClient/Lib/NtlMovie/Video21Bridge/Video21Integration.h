#pragma once
#include <windows.h>
struct IBandiVideo;
namespace opendbo_bandi_adapter {
HRESULT ConfigureInstalledVideo21Policy();
HRESULT CreateLegacyVideo21(DWORD requestedVersion,void** output);
bool IsCurrentVideo21Eof(IBandiVideo*,WPARAM,LPARAM);
HRESULT BeforeVideo21DeviceReset(IBandiVideo*);
HRESULT AfterVideo21DeviceReset(IBandiVideo*);

// These helpers own the exact reference/lease they receive, not the video or
// device. They do not require either SDK header in this declaration boundary.
template<class Video> class Video21FrameLease final {
    Video* video_;void* frame_;
public:
    Video21FrameLease(Video* video,void* frame):video_(video),frame_(frame){}
    ~Video21FrameLease(){if(video_&&frame_)video_->ReleaseLastFrame(frame_);}
    Video21FrameLease(const Video21FrameLease&)=delete;
    Video21FrameLease& operator=(const Video21FrameLease&)=delete;
};
template<class T> struct Video21ComReference final {
    T* pointer=nullptr;
    ~Video21ComReference(){if(pointer)pointer->Release();}
    Video21ComReference()=default;
    Video21ComReference(const Video21ComReference&)=delete;
    Video21ComReference& operator=(const Video21ComReference&)=delete;
};
}
