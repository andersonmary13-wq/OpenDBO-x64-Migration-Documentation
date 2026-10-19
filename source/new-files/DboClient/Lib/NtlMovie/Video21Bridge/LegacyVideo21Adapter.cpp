#define NOMINMAX
#ifndef OPENDBO_BANDI_V1_HEADER
#define OPENDBO_BANDI_V1_HEADER "../BandiVideo.h"
#endif
#include <windows.h>
#include <tchar.h>
#include <cassert>
#include <d3d9.h>
#include "LegacyVideo21Adapter.h"
#include "Backend21.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_map>
#pragma comment(lib,"gdi32.lib")

namespace opendbo_bandi_adapter {
namespace {
constexpr std::size_t MaxCommands=64,MaxFrameSlots=4;
constexpr double TicksPerSecond=10000000.0;
std::atomic<std::uint64_t> nextNotification{1};
std::uint64_t NewNotification(){return nextNotification.fetch_add(1);}
HRESULT SecondsToTicks(double value,std::int64_t& ticks){
    if(!std::isfinite(value)||value<0)return E_INVALIDARG;
    long double n=static_cast<long double>(value)*10000000.0L;
    // MSVC long double is 64-bit double: converting INT64_MAX rounds UP to
    // 2^63, so compare against the exclusive bound before the integer cast.
    if(n>=9223372036854775808.0L)return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    ticks=static_cast<std::int64_t>(n);return S_OK;
}
std::wstring Wide(const char* p){
    if(!p)return {};
    int n=MultiByteToWideChar(CP_ACP,0,p,-1,nullptr,0);if(!n)return {};
    std::vector<wchar_t> temp(n);if(!MultiByteToWideChar(CP_ACP,0,p,-1,temp.data(),n))return {};
    return std::wstring(temp.data());
}
struct MemoryFrame final : IBVLMemoryBuffer {
    HBITMAP bitmap=nullptr;
    unsigned char* pixels=nullptr;
    HBITMAP GetHBitmap()const override{return bitmap;}
    BYTE* GetRasterBuffer()const override{return pixels;}
    ~MemoryFrame(){if(bitmap)DeleteObject(bitmap);}
};
struct FrameSlot {
    IDirect3DTexture9* texture=nullptr;
    std::unique_ptr<MemoryFrame> memory;
    unsigned borrows=0;
    int width=0,height=0,pitch=0;
    BVL_PIXEL_FORMAT format=BVLPF_X8R8G8B8;
    unsigned alpha=255;
    std::uint64_t serial=0;
    bool retired=false;
    void* Key()const{return memory?static_cast<IBVLMemoryBuffer*>(memory.get()):static_cast<void*>(texture);}
    ~FrameSlot(){if(texture)texture->Release();}
};
bool ValidFormat(BVL_PIXEL_FORMAT f){return f==BVLPF_X8R8G8B8||f==BVLPF_A8R8G8B8||f==BVLPF_R5G6B5;}
bool ValidOption(BVL_OPEN_OPT o){return o==BVL_OPEN_OPT_NONE||o==BVL_OPEN_OPT_AUTOPLAY||o==BVL_OPEN_OPT_AUTOPLAY_REPEAT;}
}
struct LegacyVideo21Adapter::Impl {
    struct Task {std::uint64_t epoch;std::function<HRESULT(Decoder21&)> run;std::shared_ptr<std::promise<HRESULT>> completion;bool recordResult;};
    LegacyVideo21Adapter* owner;
    std::mutex queueMutex,stateMutex,frameMutex;
    std::condition_variable wake;
    std::deque<Task> commands;
    std::thread worker;
    std::atomic<bool> terminating{false},created{false},opening{false};
    std::atomic<std::uint64_t> epoch{1},notification{NewNotification()};
    HWND window=nullptr;
    DWORD renderThread=0;
    IDirect3DDevice9* device=nullptr;
    BVL_CONTEXT_TYPE context=BVL_CONTEXT_DIRECTX9;
    BVL_PIXEL_FORMAT requestedFormat=BVLPF_X8R8G8B8;
    BVL_OPEN_OPT option=BVL_OPEN_OPT_NONE;
    DecoderStatus status=DecoderStatus::closed;
    VideoInfo21 info;
    HRESULT lastResult=S_OK;
    HRESULT openedResult=S_OK;
    unsigned volume=255,alpha=255,decodeErrors=0;
    bool repeat=false,eofPosted=false,openedPosted=false,eofConsumed=false,openedConsumed=false,reset=false;
    std::uint64_t activeNotification=notification.load();
    std::int64_t clockBase=0,clockAtBase=0,renderTime=0;
    UINT eofMessage=0,openedMessage=0;
    std::vector<std::unique_ptr<FrameSlot>> frames;
    std::shared_ptr<CpuFrame21> lastFrame;
    std::uint64_t frameSerial=0;

    explicit Impl(LegacyVideo21Adapter* p):owner(p){}
    ~Impl(){Shutdown();frames.clear();if(device)device->Release();}
    void SaveResult(HRESULT hr){std::lock_guard<std::mutex> lock(stateMutex);lastResult=hr;}
    HRESULT Enqueue(std::function<HRESULT(Decoder21&)> fn,bool async,std::uint64_t expected,
                    std::function<void(std::uint64_t&)> admitted={},bool recordResult=true){
        if(std::this_thread::get_id()==worker.get_id()&&!async)return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
        try{
        auto done=std::make_shared<std::promise<HRESULT>>();auto future=done->get_future();
        {std::lock_guard<std::mutex> lock(queueMutex);
            if(terminating.load()||!worker.joinable())return CO_E_NOTINITIALIZED;
            if(commands.size()>=MaxCommands)return BVERR_ASYNCQUEUE_FULL;
            // Allocate the queue node before changing cancellation/notification
            // state: an allocation failure must not cancel the old operation.
            commands.push_back({expected,std::move(fn),done,recordResult});
            if(admitted)admitted(expected);
            commands.back().epoch=expected;}
        wake.notify_one();
        if(async)return S_OK; // successful queue admission, not SDK completion
        return future.get();
        }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
    }
    HRESULT Send(std::function<HRESULT(Decoder21&)> fn,bool async=false,bool recordResult=true){
        if(!created.load())return CO_E_NOTINITIALIZED;
        return Enqueue(std::move(fn),async,epoch.load(),{},recordResult);
    }
    void RetireFrames(){
        std::lock_guard<std::mutex> lock(frameMutex);
        lastFrame.reset();
        for(auto& p:frames)p->retired=true;
        // Retire only. COM/GDI resources are recycled by Borrow on the owning
        // render thread, never while a decoder/source callback admits a task.
    }
    bool HasBorrows(){std::lock_guard<std::mutex> lock(frameMutex);for(auto& p:frames)if(p->borrows)return true;return false;}
    void Shutdown(){
        if(!worker.joinable())return;
        terminating.store(true);++epoch;notification.store(NewNotification());wake.notify_all();
        if(worker.get_id()!=std::this_thread::get_id())worker.join();
        created.store(false);
    }
    void Notify(bool terminal,std::uint64_t tag,HRESULT result=S_OK){
        // Publish state BEFORE PostMessage: the owning UI may consume the
        // notification immediately on another thread. Failed posts are retried
        // only while this exact generation is still current.
        {std::lock_guard<std::mutex> lock(stateMutex);
            if(tag!=notification.load()||tag!=activeNotification)return;
            bool& posted=terminal?eofPosted:openedPosted;
            if(posted)return;
            posted=true;
            if(terminal)eofConsumed=false;else{openedConsumed=false;openedResult=result;}}
        WPARAM value=terminal?reinterpret_cast<WPARAM>(owner):static_cast<WPARAM>(result);
        if(!PostMessageW(window,terminal?eofMessage:openedMessage,value,static_cast<LPARAM>(tag))){
            HRESULT error=HRESULT_FROM_WIN32(GetLastError());
            std::lock_guard<std::mutex> lock(stateMutex);
            if(tag==activeNotification&&tag==notification.load()){
                (terminal?eofPosted:openedPosted)=false;lastResult=error;}}
    }
    void Poll(Decoder21& decoder){
        DecoderStatus current{};HRESULT hr=decoder.Status(current);
        if(FAILED(hr)){SaveResult(hr);return;}
        {std::lock_guard<std::mutex> lock(stateMutex);status=current;}
        if(current!=DecoderStatus::ended||decoder.IsNextFrame())return;
        bool loop=false;std::uint64_t tag=0;
        {std::lock_guard<std::mutex> lock(stateMutex);loop=repeat;tag=activeNotification;}
        if(tag!=notification.load())return;
        if(loop){
            hr=decoder.Seek(0);if(SUCCEEDED(hr))hr=decoder.Play();
            if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(stateMutex);clockBase=0;clockAtBase=0;renderTime=0;status=DecoderStatus::playing;eofPosted=false;}
            SaveResult(hr);return;
        }
        Notify(true,tag);
    }
    void Main(){
        Decoder21 decoder;
        while(true){
            Task task{};bool have=false;
            {std::unique_lock<std::mutex> lock(queueMutex);
                wake.wait_for(lock,std::chrono::milliseconds(5),[this]{return terminating.load()||!commands.empty();});
                if(!commands.empty()){task=std::move(commands.front());commands.pop_front();have=true;}
                else if(terminating.load())break;}
            if(have){
                HRESULT hr=E_ABORT;
                if(!terminating.load()&&task.epoch==epoch.load()){
                    try{hr=task.run(decoder);}catch(const std::bad_alloc&){hr=E_OUTOFMEMORY;}catch(...){hr=E_UNEXPECTED;}
                    if(task.recordResult&&task.epoch==epoch.load())SaveResult(hr);
                }
                task.completion->set_value(hr);
            }
            if(created.load()&&!opening.load()&&!terminating.load())Poll(decoder);
        }
        decoder.Close();
    }
    HRESULT Start(const ModulePolicy& policy,HWND hwnd){
        terminating.store(false);
        window=hwnd;eofMessage=RegisterWindowMessageW(L"WM_BVNOTIFY_EOF");openedMessage=RegisterWindowMessageW(L"WM_BVNOTIFY_OPENED");
        if(!eofMessage||!openedMessage)return HRESULT_FROM_WIN32(GetLastError());
        try{worker=std::thread([this]{Main();});}catch(...){return E_OUTOFMEMORY;}
        HRESULT hr=Enqueue([policy,hwnd](Decoder21& d){return d.Initialize(policy,hwnd);},false,epoch.load());
        if(SUCCEEDED(hr))created.store(true);else Shutdown();return hr;
    }
    HRESULT DoOpen(std::wstring path,std::shared_ptr<SourceOps> source,BVL_OPEN_OPT opt,BVL_PIXEL_FORMAT format,BVL_AUDIO_DEVICE audio,bool async){
        if(!ValidOption(opt)||!ValidFormat(format)||(audio!=BVL_AUDIO_DEVICE_NULL&&audio!=BVL_AUDIO_DEVICE_DSOUND))return E_INVALIDARG;
        if(path.empty()&&!source)return E_INVALIDARG;
        if(!created.load())return CO_E_NOTINITIALIZED;
        auto admittedTag=std::make_shared<std::uint64_t>(0);
        auto fn=[this,path,source,opt,format,audio,admittedTag](Decoder21& d){
            const std::uint64_t tag=*admittedTag;
            // This task already runs off the caller thread. Wait for the
            // SDK's synchronous completion here; a second SDK async queue can
            // reset the source after our following Seek has returned S_OK.
            HRESULT hr=d.Open(source?L"<BandiVideoSource>":path,source,audio==BVL_AUDIO_DEVICE_NULL,false);
            if(SUCCEEDED(hr)){
                const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
                while(true){
                    if(tag!=epoch.load()||terminating.load()){d.Close();hr=E_ABORT;break;}
                    DecoderStatus s{};hr=d.Status(s);if(FAILED(hr))break;
                    if(s==DecoderStatus::ready||s==DecoderStatus::playing||s==DecoderStatus::paused)break;
                    if(s!=DecoderStatus::opening){hr=E_FAIL;break;}
                    if(std::chrono::steady_clock::now()>deadline){d.Close();hr=BVERR_TIMEOUT;break;}
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            VideoInfo21 next{};
            if(SUCCEEDED(hr))hr=d.Info(next);
            if(SUCCEEDED(hr))hr=d.SetVolume(volume);
            if(SUCCEEDED(hr)&&opt!=BVL_OPEN_OPT_NONE)hr=d.Play();
            if(tag==epoch.load()){
                opening.store(false);
                if(SUCCEEDED(hr)){
                    {std::lock_guard<std::mutex> lock(stateMutex);info=next;requestedFormat=format;option=opt;repeat=(opt==BVL_OPEN_OPT_AUTOPLAY_REPEAT);status=opt==BVL_OPEN_OPT_NONE?DecoderStatus::ready:DecoderStatus::playing;clockBase=clockAtBase=renderTime=0;activeNotification=notification.load();eofPosted=false;openedPosted=false;eofConsumed=false;openedConsumed=false;decodeErrors=0;}
                }
                // SDK1 OPENED carries its completion HRESULT in WPARAM. The
                // bridge-only generation cookie is carried in LPARAM.
                {std::lock_guard<std::mutex> lock(stateMutex);activeNotification=notification.load();openedPosted=false;openedConsumed=false;}
                Notify(false,notification.load(),hr);
            }
            return hr;
        };
        return Enqueue(std::move(fn),async,epoch.load(),[this,admittedTag](std::uint64_t& expected){
            expected=++epoch;*admittedTag=expected;notification.store(NewNotification());opening.store(true);RetireFrames();
        });
    }
    HRESULT DoPlay(BOOL looping,bool async){
        if(!created.load())return CO_E_NOTINITIALIZED;
        return Enqueue([this,looping](Decoder21& d){HRESULT hr=d.Play();if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(stateMutex);repeat=(looping!=FALSE);activeNotification=notification.load();eofPosted=false;eofConsumed=false;status=DecoderStatus::playing;}return hr;},async,epoch.load(),[this](std::uint64_t&){notification.store(NewNotification());});
    }
    HRESULT DoSeek(double seconds,bool async){
        std::int64_t ticks=0;HRESULT hr=SecondsToTicks(seconds,ticks);if(FAILED(hr))return hr;
        if(!created.load())return CO_E_NOTINITIALIZED;
        return Enqueue([this,ticks](Decoder21& d){HRESULT hr=d.Seek(ticks);if(SUCCEEDED(hr)){std::int64_t clock=0;d.SoundClock(clock);std::lock_guard<std::mutex> lock(stateMutex);clockBase=ticks;clockAtBase=clock;renderTime=ticks;activeNotification=notification.load();eofPosted=false;eofConsumed=false;}return hr;},async,epoch.load(),[this](std::uint64_t&){notification.store(NewNotification());RetireFrames();});
    }
    HRESULT DoStop(bool close,bool async){
        if(!created.load())return CO_E_NOTINITIALIZED;
        return Enqueue([this,close](Decoder21& d){HRESULT hr=close?d.Close():d.Stop();if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(stateMutex);status=close?DecoderStatus::closed:DecoderStatus::ready;repeat=false;eofPosted=false;eofConsumed=false;openedPosted=false;openedConsumed=false;clockBase=clockAtBase=renderTime=0;activeNotification=notification.load();}return hr;},async,epoch.load(),[this](std::uint64_t& expected){expected=++epoch;notification.store(NewNotification());opening.store(false);RetireFrames();});
    }
    HRESULT Borrow(void** out){
        if(!out)return E_POINTER;*out=nullptr;if(!created.load())return CO_E_NOTINITIALIZED;
        const std::uint64_t generation=epoch.load();
        if(context==BVL_CONTEXT_DIRECTX9&&GetCurrentThreadId()!=renderThread)return RPC_E_WRONG_THREAD;
        if(opening.load())return S_FALSE;
        {std::lock_guard<std::mutex> lock(frameMutex);if(reset)return D3DERR_DEVICELOST;}
        auto frame=std::make_shared<CpuFrame21>();bool decoded=false;
        HRESULT hr=Send([frame,&decoded](Decoder21& d){if(!d.IsNextFrame())return S_FALSE;HRESULT hr=d.Frame(*frame,true);decoded=(hr==S_OK);return hr;});
        if(generation!=epoch.load())return E_ABORT;
        if(FAILED(hr)){std::lock_guard<std::mutex> lock(stateMutex);++decodeErrors;return hr;}
        std::lock_guard<std::mutex> lock(frameMutex);
        if(generation!=epoch.load())return E_ABORT;
        if(decoded){lastFrame=frame;++frameSerial;{std::lock_guard<std::mutex> state(stateMutex);renderTime=frame->time100ns;}}
        if(!lastFrame)return S_FALSE;
        FrameSlot* slot=nullptr;BVL_PIXEL_FORMAT format;unsigned useAlpha;
        {std::lock_guard<std::mutex> state(stateMutex);format=requestedFormat;useAlpha=255;}
        for(auto& p:frames)if(!p->retired&&p->serial==frameSerial&&p->format==format&&p->alpha==useAlpha){slot=p.get();break;}
        if(!slot){
            for(auto& p:frames)if(!p->borrows){slot=p.get();break;}
            if(!slot){if(frames.size()>=MaxFrameSlots)return HRESULT_FROM_WIN32(ERROR_BUSY);frames.emplace_back(new FrameSlot);slot=frames.back().get();}
            hr=Fill(*slot,*lastFrame,format,useAlpha);if(FAILED(hr))return hr;
            slot->serial=frameSerial;slot->retired=false;
        }
        // A source callback or a driver callback can admit a replacement while
        // Frame/upload is in progress. Never republish a cancelled generation.
        // Already-returned leases remain valid until their normal return.
        if(generation!=epoch.load()){lastFrame.reset();for(auto& p:frames)p->retired=true;return E_ABORT;}
        if(slot->borrows==(std::numeric_limits<unsigned>::max)())return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        ++slot->borrows;if(slot->texture)slot->texture->AddRef();*out=slot->Key();return S_OK;
    }
    HRESULT Fill(FrameSlot& slot,const CpuFrame21& frame,BVL_PIXEL_FORMAT format,unsigned useAlpha){
        // All decoding is SDK2 X8. Pixel selection below is explicit legacy
        // output conversion, never an SDK2 enum cast or unsupported RGB24 decode.
        if(frame.width<=0||frame.height<=0||frame.width>INT_MAX/4||frame.pitch<frame.width*4||
            static_cast<std::size_t>(frame.height)>frame.xrgb.size()/static_cast<std::size_t>(frame.pitch))return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        int bpp=format==BVLPF_R5G6B5?2:4;
        std::size_t row=static_cast<std::size_t>(frame.width)*bpp;
        if(row>static_cast<std::size_t>(INT_MAX)-3)return E_INVALIDARG;
        int pitch=static_cast<int>((row+3)&~std::size_t(3));
        if(frame.height<=0||static_cast<std::size_t>(frame.height)>static_cast<std::size_t>(INT_MAX)/pitch)return E_INVALIDARG;
        std::vector<unsigned char> pixels(static_cast<std::size_t>(pitch)*frame.height,0);
        for(int y=0;y<frame.height;++y)for(int x=0;x<frame.width;++x){
            const unsigned char* src=frame.xrgb.data()+static_cast<std::size_t>(y)*frame.pitch+x*4;
            unsigned char* dst=pixels.data()+static_cast<std::size_t>(y)*pitch+x*bpp;
            if(bpp==2){unsigned v=((src[2]>>3)<<11)|((src[1]>>2)<<5)|(src[0]>>3);dst[0]=static_cast<unsigned char>(v);dst[1]=static_cast<unsigned char>(v>>8);}
            else{dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=static_cast<unsigned char>(useAlpha);}
        }
        // Free-slot replacement is exception-safe and cannot overwrite an
        // outstanding borrow. Destination only becomes visible after success.
        auto next=std::make_unique<FrameSlot>();next->width=frame.width;next->height=frame.height;next->pitch=pitch;next->format=format;next->alpha=useAlpha;
        if(context==BVL_CONTEXT_MEMORYBUFFER){
            struct BitmapInfo {BITMAPINFOHEADER header;DWORD masks[3];} bmi{};
            bmi.header.biSize=sizeof(BITMAPINFOHEADER);bmi.header.biWidth=frame.width;bmi.header.biHeight=-frame.height;bmi.header.biPlanes=1;bmi.header.biBitCount=static_cast<WORD>(bpp*8);
            bmi.header.biCompression=bpp==2?BI_BITFIELDS:BI_RGB;bmi.masks[0]=0xF800;bmi.masks[1]=0x07E0;bmi.masks[2]=0x001F;
            next->memory=std::make_unique<MemoryFrame>();void* bits=nullptr;
            next->memory->bitmap=CreateDIBSection(nullptr,reinterpret_cast<BITMAPINFO*>(&bmi),DIB_RGB_COLORS,&bits,nullptr,0);
            if(!next->memory->bitmap||!bits)return HRESULT_FROM_WIN32(GetLastError());
            next->memory->pixels=static_cast<unsigned char*>(bits);std::memcpy(bits,pixels.data(),pixels.size());
        }else{
            if(!device)return CO_E_NOTINITIALIZED;
            D3DFORMAT df=bpp==2?D3DFMT_R5G6B5:D3DFMT_X8R8G8B8;
            // The measured SDK1 texture contract is MANAGED/usage0. Lock the
            // managed backing store directly; the runtime uploads it as needed.
            // A caller-owned lease therefore remains valid across D3D9 Reset.
            HRESULT hr=device->CreateTexture(frame.width,frame.height,1,0,df,D3DPOOL_MANAGED,&next->texture,nullptr);if(FAILED(hr))return hr;
            D3DLOCKED_RECT locked{};hr=next->texture->LockRect(0,&locked,nullptr,0);if(FAILED(hr))return hr;
            if(locked.Pitch<pitch||!locked.pBits){next->texture->UnlockRect(0);return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);}
            for(int y=0;y<frame.height;++y)std::memcpy(static_cast<unsigned char*>(locked.pBits)+static_cast<std::size_t>(y)*locked.Pitch,pixels.data()+static_cast<std::size_t>(y)*pitch,row);
            hr=next->texture->UnlockRect(0);if(FAILED(hr))return hr;
        }
        if(slot.texture)slot.texture->Release();slot.memory=std::move(next->memory);slot.texture=next->texture;next->texture=nullptr;
        slot.width=next->width;slot.height=next->height;slot.pitch=next->pitch;slot.format=format;slot.alpha=useAlpha;return S_OK;
    }
};

LegacyVideo21Adapter::LegacyVideo21Adapter():impl_(new Impl(this)){}
LegacyVideo21Adapter::~LegacyVideo21Adapter()=default;
HRESULT LegacyVideo21Adapter::Create(HWND window,LPVOID context,BVL_CONTEXT_TYPE type)try {
    if(impl_->created.load())return BVERR_ALREADY_CREATED;
    if(!window||(type!=BVL_CONTEXT_DIRECTX9&&type!=BVL_CONTEXT_MEMORYBUFFER))return E_INVALIDARG;
    if(type==BVL_CONTEXT_DIRECTX9&&!context)return E_POINTER;
    ModulePolicy policy;HRESULT hr=ReadDefaultPolicy(policy);if(FAILED(hr))return hr;
    impl_->context=type;impl_->renderThread=GetCurrentThreadId();
    if(type==BVL_CONTEXT_DIRECTX9){impl_->device=static_cast<IDirect3DDevice9*>(context);impl_->device->AddRef();}
    hr=impl_->Start(policy,window);if(FAILED(hr)&&impl_->device){impl_->device->Release();impl_->device=nullptr;}return hr;
}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Release()try {
    if(std::this_thread::get_id()==impl_->worker.get_id())return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
    if(impl_->context==BVL_CONTEXT_DIRECTX9&&GetCurrentThreadId()!=impl_->renderThread)return RPC_E_WRONG_THREAD;
    if(impl_->HasBorrows())return HRESULT_FROM_WIN32(ERROR_BUSY);
    impl_->Shutdown();delete this;return S_OK;
}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Verify(LPCSTR id,LPCSTR key)try {return impl_->Send([id,key](Decoder21& d){return d.Verify(id,key);});}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
UINT LegacyVideo21Adapter::GetVersion()try{auto value=std::make_shared<UINT>(0);HRESULT hr=impl_->Send([value](Decoder21& d){*value=d.Version();return S_OK;},false,false);return FAILED(hr)?0:*value;}catch(...){return 0;}
HRESULT LegacyVideo21Adapter::Open(LPCWSTR p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen(p,nullptr,o,f,a,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Open(LPCSTR p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen(Wide(p),nullptr,o,f,a,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
namespace {
std::shared_ptr<SourceOps> WrapSource(IBandiVideoSource* source){
    if(!source)return {};auto out=std::make_shared<SourceOps>();
    out->read=[source](unsigned char* p,int n){return source->Read(p,n);};out->seek=[source](std::int64_t n){return source->SetPosition(n);};out->position=[source]{return source->GetPosition();};out->length=[source]{return source->GetLength();};return out;
}
}
HRESULT LegacyVideo21Adapter::Open(IBandiVideoSource* p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen({},WrapSource(p),o,f,a,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Play(BOOL r)try {return impl_->DoPlay(r,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Seek(double s)try {return impl_->DoSeek(s,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Stop()try {return impl_->DoStop(false,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Close()try {return impl_->DoStop(true,false);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::OpenAsync(LPCWSTR p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen(p,nullptr,o,f,a,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::OpenAsync(LPCSTR p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen(Wide(p),nullptr,o,f,a,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::OpenAsync(IBandiVideoSource* p,BVL_OPEN_OPT o,BVL_PIXEL_FORMAT f,BVL_AUDIO_DEVICE a)try {if(!p)return E_POINTER;return impl_->DoOpen({},WrapSource(p),o,f,a,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::PlayAsync(BOOL r)try {return impl_->DoPlay(r,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::SeekAsync(double s)try {return impl_->DoSeek(s,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::StopAsync()try {return impl_->DoStop(false,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::CloseAsync()try {return impl_->DoStop(true,true);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Pause()try {return impl_->Send([this](Decoder21& d){HRESULT hr=d.Pause(true);if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(impl_->stateMutex);impl_->status=DecoderStatus::paused;}return hr;});}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::Resume()try {return impl_->Send([this](Decoder21& d){HRESULT hr=d.Pause(false);if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(impl_->stateMutex);impl_->status=DecoderStatus::playing;}return hr;});}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::SetVolume(UINT volume)try {if(volume>255)return E_INVALIDARG;return impl_->Send([this,volume](Decoder21& d){HRESULT hr=d.SetVolume(volume);if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(impl_->stateMutex);impl_->volume=volume;}return hr;});}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::SetAlpha(UINT alpha)try {if(alpha>255)return E_INVALIDARG;if(!impl_->created.load())return CO_E_NOTINITIALIZED;std::lock_guard<std::mutex> lock(impl_->stateMutex);impl_->alpha=alpha;return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
BOOL LegacyVideo21Adapter::IsPlaying(){if(!impl_->created.load()||impl_->opening.load())return FALSE;std::lock_guard<std::mutex> lock(impl_->stateMutex);return impl_->status==DecoderStatus::playing?TRUE:FALSE;}
LPVOID LegacyVideo21Adapter::GetLastFrame(){void* p=nullptr;return SUCCEEDED(GetLastFrame(&p))?p:nullptr;}
HRESULT LegacyVideo21Adapter::GetLastFrame(void** p)try {return impl_->Borrow(p);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::ReleaseLastFrame(void* p)try {
    if(impl_->context==BVL_CONTEXT_DIRECTX9&&GetCurrentThreadId()!=impl_->renderThread)return RPC_E_WRONG_THREAD;
    if(!p)return E_POINTER;std::lock_guard<std::mutex> lock(impl_->frameMutex);
    for(auto& slot:impl_->frames)if(slot->Key()==p){if(!slot->borrows)return E_INVALIDARG;--slot->borrows;if(slot->texture)slot->texture->Release();return S_OK;}return E_INVALIDARG;
}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
BOOL LegacyVideo21Adapter::CheckFrame()try{
    if(!impl_->created.load()||impl_->opening.load())return FALSE;
    auto yes=std::make_shared<bool>(false);HRESULT hr=impl_->Send([yes](Decoder21& d){*yes=d.IsNextFrame();return S_OK;},false,false);return SUCCEEDED(hr)&&*yes?TRUE:FALSE;
}catch(...){return FALSE;}
HRESULT LegacyVideo21Adapter::GetVideoInfo(BVLVideoInfo& out)try {
    std::memset(&out,0,sizeof(out));if(!impl_->created.load())return CO_E_NOTINITIALIZED;if(impl_->opening.load())return BVERR_NOT_OPENED;
    {std::lock_guard<std::mutex> lock(impl_->stateMutex);if(FAILED(impl_->lastResult))return impl_->lastResult;}
    auto clock=std::make_shared<std::int64_t>(0);HRESULT clockResult=impl_->Send([clock](Decoder21& d){return d.SoundClock(*clock);},false,false);
    std::lock_guard<std::mutex> lock(impl_->stateMutex);
    out.Format=impl_->requestedFormat;out.Playflag=impl_->option;out.width=impl_->info.width;out.height=impl_->info.height;out.TotalPlayTime=impl_->info.duration100ns/TicksPerSecond;
    out.ErrorFrames=impl_->decodeErrors;out.CurrentRenderTime=impl_->renderTime/TicksPerSecond;
    // Silent/no-audio sources can legitimately have no opened sound device.
    // Their last decoded timestamp remains meaningful without inventing a
    // sound clock or converting that optional failure into a video failure.
    out.CurrentPlayTime=(SUCCEEDED(clockResult)?impl_->clockBase+std::max<std::int64_t>(0,*clock-impl_->clockAtBase):impl_->renderTime)/TicksPerSecond;
    switch(impl_->status){case DecoderStatus::playing:out.Status=BVL_STATUS_PLAYING;break;case DecoderStatus::paused:out.Status=BVL_STATUS_PAUSED;break;case DecoderStatus::closed:out.Status=BVL_STATUS_CREATED;break;default:out.Status=BVL_STATUS_OPENED;break;}return S_OK;
}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
double LegacyVideo21Adapter::FramesToTime(INT frames){std::lock_guard<std::mutex> lock(impl_->stateMutex);if(impl_->info.fpsRate<=0||impl_->info.fpsScale<=0)return -1;return static_cast<double>(frames)*impl_->info.fpsScale/impl_->info.fpsRate;}
INT LegacyVideo21Adapter::TimeToFrames(double seconds){std::lock_guard<std::mutex> lock(impl_->stateMutex);if(!std::isfinite(seconds)||impl_->info.fpsRate<=0||impl_->info.fpsScale<=0)return -1;long double n=static_cast<long double>(seconds)*impl_->info.fpsRate/impl_->info.fpsScale;if(n>INT_MAX||n<INT_MIN)return -1;return static_cast<INT>(n);}
HRESULT LegacyVideo21Adapter::BeforeDeviceReset()try {if(GetCurrentThreadId()!=impl_->renderThread)return RPC_E_WRONG_THREAD;std::lock_guard<std::mutex> lock(impl_->frameMutex);if(impl_->context!=BVL_CONTEXT_DIRECTX9){for(auto& p:impl_->frames)if(p->borrows)return HRESULT_FROM_WIN32(ERROR_BUSY);impl_->frames.clear();}impl_->reset=true;return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
HRESULT LegacyVideo21Adapter::AfterDeviceReset()try {if(GetCurrentThreadId()!=impl_->renderThread)return RPC_E_WRONG_THREAD;std::lock_guard<std::mutex> lock(impl_->frameMutex);impl_->reset=false;return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
bool LegacyVideo21Adapter::ConsumeCurrentEofNotification(WPARAM p,LPARAM tag){if(p!=reinterpret_cast<WPARAM>(this)||static_cast<std::uint64_t>(tag)!=impl_->notification.load())return false;std::lock_guard<std::mutex> lock(impl_->stateMutex);if(!impl_->eofPosted||impl_->eofConsumed||impl_->activeNotification!=static_cast<std::uint64_t>(tag))return false;impl_->eofConsumed=true;return true;}
bool LegacyVideo21Adapter::ConsumeCurrentOpenedNotification(WPARAM p,LPARAM tag){if(static_cast<std::uint64_t>(tag)!=impl_->notification.load())return false;std::lock_guard<std::mutex> lock(impl_->stateMutex);if(!impl_->openedPosted||impl_->openedConsumed||impl_->activeNotification!=static_cast<std::uint64_t>(tag)||p!=static_cast<WPARAM>(impl_->openedResult))return false;impl_->openedConsumed=true;return true;}
HRESULT CreateLegacyVideo21(DWORD version,void** out){if(!out)return E_POINTER;*out=nullptr;if(version!=BANDI_VIDEO_LIBRARY_VERSION)return BVERR_INVALID_VERSION;try{*out=static_cast<IBandiVideo*>(new LegacyVideo21Adapter);return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}}
}
