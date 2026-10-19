// Private sound backend port, informed by the vendor's BandiVideoSound_DS/NULL
// examples (Copyright 2008-2014 Bandisoft). Do not publish vendor-derived code
// without determining its redistribution license. No SDK1, game or recorder.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define DIRECTSOUND_VERSION 0x0800
#include "Sdk2SoundFactory.h"
#include <dsound.h>
#include <process.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace opendbo_bandi_adapter {
namespace {
using opendbo_bandi_sdk21::IBandiVideoSoundCB;
struct Lock {
    CRITICAL_SECTION* p;
    explicit Lock(CRITICAL_SECTION& value):p(&value){EnterCriticalSection(p);}
    ~Lock(){LeaveCriticalSection(p);}
};
static HRESULT LastErrorHR(){const DWORD error=GetLastError();return error?HRESULT_FROM_WIN32(error):E_FAIL;}
static LONG VolumeDB(INT volume){
    if(volume==0)return DSBVOLUME_MIN;
    if(volume==255)return DSBVOLUME_MAX;
    // Preserve the vendor sample's monotonic 0..255 logarithmic gain mapping.
    const double floor=1e-10;
    const double gain=3000.0*std::log10((1.0-floor)*(static_cast<double>(volume)/255.0)+floor);
    return static_cast<LONG>((std::max)(-10000.0,(std::min)(0.0,gain)));
}
static INT64 FramesTime(UINT64 frames,UINT rate){
    if(!rate)return 0;
    return static_cast<INT64>(frames/rate)*10000000+
        static_cast<INT64>((frames%rate)*10000000/rate);
}

class SoundSession:public std::enable_shared_from_this<SoundSession> {
    CRITICAL_SECTION mutex_{};
    HANDLE stop_=nullptr,ready_=nullptr,thread_=nullptr;
    unsigned threadId_=0;
    HWND owner_=nullptr;
    bool silent_=true,open_=false,started_=false,paused_=false,bufferPlaying_=false;
    UINT channels_=0,rate_=0,align_=0;
    INT volume_=255;
    UINT64 epoch_=0,drainedFrames_=0,writtenFrames_=0,playedFrames_=0;
    INT64 qpcFrequency_=0,qpcAnchor_=0,clockBase_=0,lastTime_=0;
    HRESULT failure_=S_OK;
    IBandiVideoSoundCB* callback_=nullptr;
    IDirectSound8* device_=nullptr;
    IDirectSoundBuffer* buffer_=nullptr;
    DWORD capacity_=0,lastCursor_=0,writeCursor_=0;
    std::vector<BYTE> shadow_;
    INT16 pendingPCM_[2048*8]{};
    UINT pendingFrames_=0;

    static unsigned __stdcall Entry(void* opaque){
        std::unique_ptr<std::shared_ptr<SoundSession>> transfer(static_cast<std::shared_ptr<SoundSession>*>(opaque));
        std::shared_ptr<SoundSession> session=*transfer;transfer.reset();
        session->Run();return 0;
    }
    INT64 QpcTimeLocked()const{
        if(!started_ || paused_)return clockBase_;
        LARGE_INTEGER now{};if(!QueryPerformanceCounter(&now) || qpcFrequency_<=0)return clockBase_;
        const INT64 ticks=(std::max)(INT64(0),now.QuadPart-qpcAnchor_);
        const long double delta=static_cast<long double>(ticks)*10000000.0L/qpcFrequency_;
        const long double total=static_cast<long double>(clockBase_)+delta;
        return total>static_cast<long double>(INT64_MAX)?INT64_MAX:static_cast<INT64>(total);
    }
    HRESULT UpdatePositionLocked(){
        if(!bufferPlaying_)return S_OK;
        DWORD cursor=0;const HRESULT hr=buffer_->GetCurrentPosition(&cursor,nullptr);
        if(FAILED(hr))return hr;
        if(cursor>=capacity_ || cursor%align_)return E_FAIL;
        const DWORD delta=(cursor+capacity_-lastCursor_)%capacity_;
        playedFrames_+=delta/align_;lastCursor_=cursor;
        // Never advance the audio clock beyond samples actually submitted.
        if(playedFrames_>writtenFrames_)playedFrames_=writtenFrames_;
        lastTime_=(std::max)(lastTime_,FramesTime(playedFrames_,rate_));
        if(writtenFrames_ && playedFrames_==writtenFrames_){
            // A starved/finished decoder must not loop stale PCM from the ring.
            // Resume future data at its actual next write position, same clock.
            HRESULT stopped=buffer_->Stop();if(FAILED(stopped))return stopped;
            bufferPlaying_=false;
            stopped=buffer_->SetCurrentPosition(writeCursor_);if(FAILED(stopped))return stopped;
            lastCursor_=writeCursor_;
        }
        return S_OK;
    }
    HRESULT RestoreLocked(){
        HRESULT hr=buffer_->Restore();if(FAILED(hr))return hr;
        void* first=nullptr;void* second=nullptr;DWORD firstBytes=0,secondBytes=0;
        hr=buffer_->Lock(0,capacity_,&first,&firstBytes,&second,&secondBytes,0);
        if(FAILED(hr))return hr;
        const bool valid=firstBytes+secondBytes==capacity_ && (!firstBytes || first) && (!secondBytes || second);
        if(valid){
            if(firstBytes)std::memcpy(first,shadow_.data(),firstBytes);
            if(secondBytes)std::memcpy(second,shadow_.data()+firstBytes,secondBytes);
        }
        const HRESULT unlock=buffer_->Unlock(first,firstBytes,second,secondBytes);
        if(!valid)return E_FAIL;if(FAILED(unlock))return unlock;
        hr=buffer_->SetCurrentPosition(lastCursor_);if(FAILED(hr))return hr;
        return bufferPlaying_?buffer_->Play(0,0,DSBPLAY_LOOPING):S_OK;
    }
    HRESULT WriteLocked(const INT16* pcm,UINT frames){
        const DWORD bytes=frames*align_;
        if(!bytes || bytes>=capacity_)return E_INVALIDARG;
        void* first=nullptr;void* second=nullptr;DWORD firstBytes=0,secondBytes=0;
        HRESULT hr=buffer_->Lock(writeCursor_,bytes,&first,&firstBytes,&second,&secondBytes,0);
        if(hr==DSERR_BUFFERLOST){
            hr=RestoreLocked();if(FAILED(hr))return hr;
            hr=buffer_->Lock(writeCursor_,bytes,&first,&firstBytes,&second,&secondBytes,0);
        }
        if(FAILED(hr))return hr;
        const bool valid=firstBytes+secondBytes==bytes && (!firstBytes || first) && (!secondBytes || second);
        if(valid){
            if(firstBytes)std::memcpy(first,pcm,firstBytes);
            if(secondBytes)std::memcpy(second,reinterpret_cast<const BYTE*>(pcm)+firstBytes,secondBytes);
        }
        const HRESULT unlock=buffer_->Unlock(first,firstBytes,second,secondBytes);
        if(!valid)return E_FAIL;if(FAILED(unlock))return unlock;
        const DWORD tail=(std::min)(bytes,capacity_-writeCursor_);
        std::memcpy(shadow_.data()+writeCursor_,pcm,tail);
        if(bytes>tail)std::memcpy(shadow_.data(),reinterpret_cast<const BYTE*>(pcm)+tail,bytes-tail);
        writeCursor_=(writeCursor_+bytes)%capacity_;writtenFrames_+=frames;
        if(!bufferPlaying_){
            hr=buffer_->Play(0,0,DSBPLAY_LOOPING);if(FAILED(hr))return hr;bufferPlaying_=true;
        }
        return S_OK;
    }
    HRESULT ResetBufferLocked(){
        if(!buffer_)return S_OK;
        HRESULT hr=buffer_->Stop();if(FAILED(hr))return hr;
        bufferPlaying_=false;lastCursor_=writeCursor_=0;
        hr=buffer_->SetCurrentPosition(0);if(FAILED(hr))return hr;
        std::fill(shadow_.begin(),shadow_.end(),BYTE(0));
        void* first=nullptr;void* second=nullptr;DWORD firstBytes=0,secondBytes=0;
        hr=buffer_->Lock(0,capacity_,&first,&firstBytes,&second,&secondBytes,0);
        if(hr==DSERR_BUFFERLOST)return RestoreLocked();
        if(FAILED(hr))return hr;
        const bool valid=firstBytes+secondBytes==capacity_ && (!firstBytes || first) && (!secondBytes || second);
        if(valid){if(firstBytes)std::memset(first,0,firstBytes);if(secondBytes)std::memset(second,0,secondBytes);}
        const HRESULT unlock=buffer_->Unlock(first,firstBytes,second,secondBytes);
        return valid?unlock:E_FAIL;
    }
    void FailLocked(HRESULT hr){
        if(SUCCEEDED(hr))hr=E_FAIL;failure_=hr;started_=false;paused_=false;++epoch_;
        if(buffer_){buffer_->Stop();bufferPlaying_=false;}
    }
    void Pump(){
        // Callback is never called under a session/lifecycle lock. It may Close,
        // Stop, Pause or Release the adapter; this session has independent life.
        for(unsigned batch=0;batch<8;++batch){
            IBandiVideoSoundCB* callback=nullptr;UINT request=0;UINT64 epoch=0;
            {
                Lock guard(mutex_);
                if(!open_ || !started_ || paused_ || FAILED(failure_))return;
                if(pendingFrames_){
                    const UINT frames=pendingFrames_;pendingFrames_=0;
                    const HRESULT hr=WriteLocked(pendingPCM_,frames);
                    if(FAILED(hr)){FailLocked(hr);return;}
                    continue;
                }
                if(silent_){
                    const INT64 now=QpcTimeLocked();
                    const UINT64 target=static_cast<UINT64>(now/10000000)*rate_+
                        static_cast<UINT64>(now%10000000)*rate_/10000000;
                    if(target<=drainedFrames_)return;
                    request=static_cast<UINT>((std::min)(UINT64(2048),target-drainedFrames_));
                }else{
                    const HRESULT hr=UpdatePositionLocked();if(FAILED(hr)){FailLocked(hr);return;}
                    const UINT64 queued=writtenFrames_-playedFrames_;
                    const UINT64 capacityFrames=capacity_/align_;
                    if(queued>=capacityFrames-1)return;
                    request=static_cast<UINT>((std::min)(UINT64(2048),capacityFrames-1-queued));
                }
                epoch=epoch_;callback=callback_;if(callback)callback->AddRef();
            }
            if(!callback)return;
            INT16 pcm[2048*8]{};INT received=-1;
            try{received=callback->GetPcmData(pcm,static_cast<INT>(request));}catch(...){received=-1;}
            callback->Release();
            {
                Lock guard(mutex_);
                if(!open_ || !started_ || epoch_!=epoch)return;
                if(received<0 || static_cast<UINT>(received)>request){FailLocked(E_FAIL);return;}
                if(!received)return;
                if(silent_)drainedFrames_+=static_cast<UINT>(received);
                else if(paused_){
                    // Pause does not change decoder stream identity: retain the
                    // already-consumed PCM, then submit it before the next pull.
                    pendingFrames_=static_cast<UINT>(received);
                    std::memcpy(pendingPCM_,pcm,pendingFrames_*align_);return;
                }else{const HRESULT hr=WriteLocked(pcm,static_cast<UINT>(received));if(FAILED(hr)){FailLocked(hr);return;}}
            }
        }
    }
    void Run(){
        const HANDLE starts[]={stop_,ready_};
        if(WaitForMultipleObjects(2,starts,FALSE,INFINITE)!=WAIT_OBJECT_0+1)return;
        while(WaitForSingleObject(stop_,2)==WAIT_TIMEOUT){
            try{Pump();}catch(...){Lock guard(mutex_);FailLocked(E_FAIL);}
        }
    }
public:
    SoundSession(HWND owner,bool silent,INT volume):owner_(owner),silent_(silent),volume_(volume){InitializeCriticalSection(&mutex_);}
    ~SoundSession(){
        RequestClose();
        if(thread_ && GetCurrentThreadId()!=threadId_)WaitForSingleObject(thread_,INFINITE);
        if(buffer_)buffer_->Release();if(device_)device_->Release();
        if(callback_)callback_->Release();
        if(thread_)CloseHandle(thread_);if(ready_)CloseHandle(ready_);if(stop_)CloseHandle(stop_);
        DeleteCriticalSection(&mutex_);
    }
    HRESULT Initialize(INT channels,INT samplerate,IBandiVideoSoundCB* callback){
        if(channels<1 || channels>8 || samplerate<1 || samplerate>384000 || !callback)return E_INVALIDARG;
        channels_=static_cast<UINT>(channels);rate_=static_cast<UINT>(samplerate);align_=channels_*sizeof(INT16);
        LARGE_INTEGER frequency{};
        if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0)return E_FAIL;qpcFrequency_=frequency.QuadPart;
        stop_=CreateEventW(nullptr,TRUE,FALSE,nullptr);ready_=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        if(!stop_ || !ready_)return LastErrorHR();
        callback->AddRef();callback_=callback;
        if(!silent_){
            if(!owner_ || !IsWindow(owner_))return E_INVALIDARG;
            HRESULT hr=DirectSoundCreate8(nullptr,&device_,nullptr);if(FAILED(hr))return hr;
            hr=device_->SetCooperativeLevel(owner_,DSSCL_NORMAL);if(FAILED(hr))return hr;
            WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=static_cast<WORD>(channels_);
            format.nSamplesPerSec=rate_;format.wBitsPerSample=16;format.nBlockAlign=static_cast<WORD>(align_);format.nAvgBytesPerSec=rate_*align_;
            capacity_=(std::max)(UINT(8),rate_/5)*align_;shadow_.resize(capacity_,0);
            DSBUFFERDESC descriptor{};descriptor.dwSize=sizeof(descriptor);
            descriptor.dwFlags=DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLVOLUME;
            descriptor.dwBufferBytes=capacity_;descriptor.lpwfxFormat=&format;
            hr=device_->CreateSoundBuffer(&descriptor,&buffer_,nullptr);if(FAILED(hr))return hr;
            hr=ResetBufferLocked();if(FAILED(hr))return hr;
            hr=buffer_->SetVolume(VolumeDB(volume_));if(FAILED(hr))return hr;
        }
        open_=true;
        auto* transfer=new(std::nothrow)std::shared_ptr<SoundSession>(shared_from_this());if(!transfer)return E_OUTOFMEMORY;
        thread_=reinterpret_cast<HANDLE>(_beginthreadex(nullptr,0,&Entry,transfer,0,&threadId_));
        if(!thread_){delete transfer;return E_OUTOFMEMORY;}
        return S_OK;
    }
    void Publish(){SetEvent(ready_);}
    void RequestClose(){
        Lock guard(mutex_);open_=false;started_=false;paused_=false;pendingFrames_=0;++epoch_;
        if(buffer_){buffer_->Stop();bufferPlaying_=false;}
        if(stop_)SetEvent(stop_);
    }
    void Join(){if(thread_ && GetCurrentThreadId()!=threadId_)WaitForSingleObject(thread_,INFINITE);}
    HRESULT Start(){
        Lock guard(mutex_);if(!open_)return E_UNEXPECTED;
        ++epoch_;failure_=S_OK;pendingFrames_=0;clockBase_=lastTime_=0;drainedFrames_=writtenFrames_=playedFrames_=0;
        LARGE_INTEGER now{};if(!QueryPerformanceCounter(&now))return E_FAIL;qpcAnchor_=now.QuadPart;
        const HRESULT hr=ResetBufferLocked();if(FAILED(hr)){FailLocked(hr);return hr;}
        started_=true;paused_=false;return S_OK;
    }
    HRESULT Stop(){
        Lock guard(mutex_);if(!open_)return E_UNEXPECTED;
        ++epoch_;started_=false;paused_=false;pendingFrames_=0;clockBase_=lastTime_=0;drainedFrames_=writtenFrames_=playedFrames_=0;
        const HRESULT hr=ResetBufferLocked();if(FAILED(hr))failure_=hr;return hr;
    }
    HRESULT Pause(BOOL paused){
        Lock guard(mutex_);if(!open_ || !started_)return E_UNEXPECTED;
        if((paused!=FALSE)==paused_)return S_OK;
        // Pause/resume preserves the stream; Stop/Start/Close alone change epoch.
        if(paused){
            if(silent_)clockBase_=QpcTimeLocked();
            else{const HRESULT hr=UpdatePositionLocked();if(FAILED(hr)){FailLocked(hr);return hr;}if(buffer_){const HRESULT stop=buffer_->Stop();if(FAILED(stop))return stop;}}
            paused_=true;
        }else{
            LARGE_INTEGER now{};if(!QueryPerformanceCounter(&now))return E_FAIL;qpcAnchor_=now.QuadPart;
            paused_=false;
            if(bufferPlaying_){const HRESULT hr=buffer_->Play(0,0,DSBPLAY_LOOPING);if(FAILED(hr)){FailLocked(hr);return hr;}}
        }
        return S_OK;
    }
    HRESULT GetTime(INT64& time){
        Lock guard(mutex_);time=0;if(!open_)return E_UNEXPECTED;
        if(silent_)time=QpcTimeLocked();
        else{
            if(!paused_){const HRESULT hr=UpdatePositionLocked();if(FAILED(hr)){FailLocked(hr);return hr;}}
            time=lastTime_;
        }
        return failure_;
    }
    HRESULT SetVolume(INT volume){
        Lock guard(mutex_);if(volume<0 || volume>255)return E_INVALIDARG;
        volume_=volume;return buffer_?buffer_->SetVolume(VolumeDB(volume)):S_OK;
    }
};

class AdapterSound final:public IAdapterSound {
    std::atomic<ULONG> references_{1};
    std::atomic<bool> opening_{false};
    UINT64 generation_=0;
    CRITICAL_SECTION lifecycle_{};
    HWND owner_;
    bool silent_=false;
    INT volume_=255;
    std::shared_ptr<SoundSession> session_;
    std::shared_ptr<SoundSession> Current(){Lock guard(lifecycle_);return session_;}
public:
    explicit AdapterSound(HWND owner):owner_(owner){InitializeCriticalSection(&lifecycle_);}
    ~AdapterSound(){Close();DeleteCriticalSection(&lifecycle_);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{
        if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown)return E_NOINTERFACE;
        *out=static_cast<IAdapterSound*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef()override{return ++references_;}
    ULONG STDMETHODCALLTYPE Release()override{const ULONG remaining=--references_;if(!remaining)delete this;return remaining;}
    HRESULT SetSilentMode(BOOL silent)override{Lock guard(lifecycle_);if(session_ || opening_.load())return E_UNEXPECTED;silent_=silent!=FALSE;return S_OK;}
    HRESULT Open(INT channels,INT samplerate,IBandiVideoSoundCB* callback)override{
        // The admission flag precedes Close and remains set across the unlocked
        // Join. A concurrent/reentrant Open cannot replace an unpublished owner.
        if(opening_.exchange(true))return HRESULT_FROM_WIN32(ERROR_BUSY);
        struct Admission {std::atomic<bool>& flag;~Admission(){flag.store(false);}} admission{opening_};
        try{
            std::shared_ptr<SoundSession> old,candidate;UINT64 ticket=0;
            {
                Lock guard(lifecycle_);ticket=++generation_;
                old.swap(session_);if(old)old->RequestClose();
            }
            if(old)old->Join(); // Never hold a lifecycle lock while joining.
            HRESULT result=E_FAIL;
            {
                Lock guard(lifecycle_);
                if(generation_!=ticket)return HRESULT_FROM_WIN32(ERROR_CANCELLED);
                candidate=std::make_shared<SoundSession>(owner_,silent_,volume_);
                result=candidate->Initialize(channels,samplerate,callback);
                if(SUCCEEDED(result) && generation_==ticket){
                    session_=candidate;candidate->Publish();return S_OK;
                }
                // An AddRef observer may Close reentrantly during Initialize.
                if(SUCCEEDED(result))result=HRESULT_FROM_WIN32(ERROR_CANCELLED);
                candidate->RequestClose();
            }
            candidate->Join();return result;
        }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_FAIL;}
    }
    void Close()override{
        std::shared_ptr<SoundSession> old;
        {Lock guard(lifecycle_);++generation_;old.swap(session_);if(old)old->RequestClose();}
        if(old)old->Join();
        // Own-callback Close cannot self-join: old session owns all worker state
        // until the callback returns. DirectSound was already stopped above.
    }
    HRESULT Start()override{auto s=Current();return s?s->Start():E_UNEXPECTED;}
    HRESULT Stop()override{auto s=Current();return s?s->Stop():E_UNEXPECTED;}
    HRESULT Pause(BOOL pause)override{auto s=Current();return s?s->Pause(pause):E_UNEXPECTED;}
    HRESULT GetTime(INT64& value)override{auto s=Current();if(!s){value=0;return E_UNEXPECTED;}return s->GetTime(value);}
    HRESULT SetVolume(INT volume)override{
        if(volume<0 || volume>255)return E_INVALIDARG;
        std::shared_ptr<SoundSession> s;
        {Lock guard(lifecycle_);volume_=volume;s=session_;}
        return s?s->SetVolume(volume):S_OK;
    }
};
}
HRESULT CreateAdapterSound(HWND owner,IAdapterSound** out){
    if(!out)return E_POINTER;*out=nullptr;
    try{*out=new AdapterSound(owner);return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_FAIL;}
}
}
