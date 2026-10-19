#define NOMINMAX
#include "Backend21.h"
#include "Sdk21Native.h"
#include "Sound/Sdk2SoundFactory.h"
#include <bcrypt.h>
#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <new>
#pragma comment(lib,"bcrypt.lib")

namespace opendbo_bandi_adapter {
namespace sdk = opendbo_bandi_sdk21;
namespace {
std::mutex policyMutex;
ModulePolicy defaultPolicy;
bool configured=false;
std::atomic<unsigned> liveDecoders{0};
HRESULT WinError(){DWORD e=GetLastError();return HRESULT_FROM_WIN32(e?e:ERROR_GEN_FAILURE);}
bool ValidPolicy(const ModulePolicy& p){
    if(p.absoluteDll.size()<4 || p.maxFrameBytes<4 || p.maxFrameBytes>static_cast<std::size_t>(INT_MAX))return false;
    DWORD n=GetFullPathNameW(p.absoluteDll.c_str(),0,nullptr,nullptr);
    if(!n)return false;
    std::vector<wchar_t> full(n);
    if(!GetFullPathNameW(p.absoluteDll.c_str(),n,full.data(),nullptr))return false;
    if(_wcsicmp(full.data(),p.absoluteDll.c_str())!=0)return false;
    auto slash=p.absoluteDll.find_last_of(L"\\/");
    if(slash==std::wstring::npos || _wcsicmp(p.absoluteDll.c_str()+slash+1,L"bdvid64.dll")!=0)return false;
    return std::any_of(p.sha256.begin(),p.sha256.end(),[](unsigned char b){return b!=0;});
}
HRESULT HashFile(HANDLE file,std::array<unsigned char,32>& out){
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectBytes=0,got=0;std::vector<unsigned char> object;
    NTSTATUS status=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0);
    if(status<0)return HRESULT_FROM_NT(status);
    status=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectBytes),sizeof(objectBytes),&got,0);
    if(status>=0){try{object.resize(objectBytes);}catch(...){BCryptCloseAlgorithmProvider(alg,0);return E_OUTOFMEMORY;}status=BCryptCreateHash(alg,&hash,object.data(),objectBytes,nullptr,0,0);}
    HRESULT hr=status<0?HRESULT_FROM_NT(status):S_OK;
    unsigned char block[65536];
    while(SUCCEEDED(hr)){
        DWORD bytes=0;
        if(!ReadFile(file,block,sizeof(block),&bytes,nullptr)){hr=WinError();break;}
        if(!bytes)break;
        status=BCryptHashData(hash,block,bytes,0);if(status<0)hr=HRESULT_FROM_NT(status);
    }
    if(SUCCEEDED(hr)){status=BCryptFinishHash(hash,out.data(),static_cast<ULONG>(out.size()),0);if(status<0)hr=HRESULT_FROM_NT(status);}
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg,0);return hr;
}
class Reader final : public sdk::IBandiVideoFileReader {
    std::atomic<ULONG> refs_{1};
    HANDLE file_=INVALID_HANDLE_VALUE;
    std::shared_ptr<SourceOps> source_;
public:
    void Bind(std::shared_ptr<SourceOps> s){Close();source_=std::move(s);}
    void Unbind(){Close();source_.reset();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override{
        if(!out)return E_POINTER;*out=nullptr;
        if(id!=IID_IUnknown)return E_NOINTERFACE;
        *out=static_cast<sdk::IBandiVideoFileReader*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override{ULONG n=--refs_;if(!n)delete this;return n;}
    HRESULT Open(LPCWSTR path) override{
        Close();
        if(source_){
            if(!source_->read||!source_->seek||!source_->position||!source_->length)return E_INVALIDARG;
            try{return source_->seek(0)==0?S_OK:E_FAIL;}catch(...){return E_UNEXPECTED;}
        }
        if(!path||!*path)return E_INVALIDARG;
        file_=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        return file_==INVALID_HANDLE_VALUE?WinError():S_OK;
    }
    void Close() override{if(file_!=INVALID_HANDLE_VALUE){CloseHandle(file_);file_=INVALID_HANDLE_VALUE;}}
    INT Read(BYTE* out,INT count) override{
        if(count<0||(!out&&count))return -1;
        if(source_){try{int n=source_->read(out,count);return n>=0&&n<=count?n:-1;}catch(...){return -1;}}
        if(file_==INVALID_HANDLE_VALUE)return -1;
        DWORD n=0;return ReadFile(file_,out,static_cast<DWORD>(count),&n,nullptr)?static_cast<INT>(n):-1;
    }
    INT64 SetPosition(INT64 value) override{
        if(value<0)return -1;if(source_){try{return source_->seek(value);}catch(...){return -1;}}
        if(file_==INVALID_HANDLE_VALUE)return -1;
        LARGE_INTEGER in{},out{};in.QuadPart=value;
        return SetFilePointerEx(file_,in,&out,FILE_BEGIN)?out.QuadPart:-1;
    }
    INT64 GetPosition() override{
        if(source_){try{return source_->position();}catch(...){return -1;}}if(file_==INVALID_HANDLE_VALUE)return -1;
        LARGE_INTEGER zero{},out{};return SetFilePointerEx(file_,zero,&out,FILE_CURRENT)?out.QuadPart:-1;
    }
    INT64 GetFileSize() override{
        if(source_){try{return source_->length();}catch(...){return -1;}}if(file_==INVALID_HANDLE_VALUE)return -1;
        LARGE_INTEGER size{};return GetFileSizeEx(file_,&size)?size.QuadPart:-1;
    }
    ~Reader(){Close();}
};
}
HRESULT ConfigureDefaultPolicy(const ModulePolicy& p){
    try{
    if(!ValidPolicy(p))return E_INVALIDARG;
    std::lock_guard<std::mutex> lock(policyMutex);
    if(configured&&defaultPolicy.absoluteDll==p.absoluteDll&&defaultPolicy.sha256==p.sha256&&defaultPolicy.maxFrameBytes==p.maxFrameBytes)return S_OK;
    if(liveDecoders.load())return HRESULT_FROM_WIN32(ERROR_BUSY);
    defaultPolicy=p;configured=true;return S_OK;
    }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
}
HRESULT ReadDefaultPolicy(ModulePolicy& p){
    try{
    std::lock_guard<std::mutex> lock(policyMutex);
    if(!configured)return CO_E_NOTINITIALIZED;p=defaultPolicy;return S_OK;
    }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
}
struct Decoder21::Impl {
    HMODULE module=nullptr;
    sdk::IBandiVideo2* video=nullptr;
    Reader* reader=nullptr;
    IAdapterSound* sound=nullptr;
    ModulePolicy policy;
    bool verified=false,live=false;
    ~Impl(){
        if(video){video->Close();video->Release();video=nullptr;}
        if(sound){sound->Close();sound->Release();sound=nullptr;}
        if(reader){reader->Unbind();reader->Release();reader=nullptr;}
        if(module)FreeLibrary(module);
        if(live)--liveDecoders;
    }
};
Decoder21::Decoder21():impl_(new Impl){}
Decoder21::~Decoder21()=default;
HRESULT Decoder21::Initialize(const ModulePolicy& policy,HWND window){
    if(impl_->video)return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    try{
    if(!ValidPolicy(policy))return E_INVALIDARG;
    // Keep partial attempts local: every failure releases the reader/sound/
    // module and a later Initialize cannot overwrite a leaked partial state.
    auto next=std::make_unique<Impl>();
    HANDLE held=CreateFileW(policy.absoluteDll.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(held==INVALID_HANDLE_VALUE)return WinError();
    std::array<unsigned char,32> actual{};HRESULT hr=HashFile(held,actual);
    if(SUCCEEDED(hr)&&actual!=policy.sha256)hr=TRUST_E_BAD_DIGEST;
    if(SUCCEEDED(hr)){
        next->module=LoadLibraryExW(policy.absoluteDll.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!next->module)hr=WinError();
    }
    CloseHandle(held);
    if(FAILED(hr))return hr;
    auto create=reinterpret_cast<sdk::LPCREATEBANDIVIDEO2>(GetProcAddress(next->module,"CreateBandiVideo2"));
    if(!create)return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    next->reader=new(std::nothrow) Reader;
    if(!next->reader)return E_OUTOFMEMORY;
    hr=CreateAdapterSound(window,&next->sound);if(FAILED(hr))return hr;
    hr=create(BVL_VERSION,reinterpret_cast<void**>(&next->video),next->reader,next->sound);
    if(FAILED(hr))return hr;
    if(!next->video)return E_UNEXPECTED;
    next->policy=policy;next->live=true;++liveDecoders;impl_=std::move(next);return hr;
    }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
}
HRESULT Decoder21::Verify(LPCSTR id,LPCSTR key){
    if(!impl_->video)return CO_E_NOTINITIALIZED;
    HRESULT hr=impl_->video->Verify(id,key);impl_->verified=(hr==S_OK);return hr;
}
UINT Decoder21::Version()const{return impl_->video?static_cast<UINT>(impl_->video->GetVersion()):0;}
HRESULT Decoder21::Open(const std::wstring& path,std::shared_ptr<SourceOps> source,bool silent,bool async){
    if(!impl_->video)return CO_E_NOTINITIALIZED;
    if(!impl_->verified)return E_ACCESSDENIED;
    DecoderStatus status{};HRESULT hr=Status(status);if(FAILED(hr))return hr;
    if(status!=DecoderStatus::closed){hr=impl_->video->Close();if(FAILED(hr))return hr;}
    impl_->sound->Close();hr=impl_->sound->SetSilentMode(silent?TRUE:FALSE);if(FAILED(hr))return hr;
    impl_->reader->Bind(std::move(source));
    return impl_->video->Open(path.c_str(),async?TRUE:FALSE);
}
HRESULT Decoder21::Close(){
    if(!impl_->video)return CO_E_NOTINITIALIZED;
    DecoderStatus status{};HRESULT hr=Status(status);if(FAILED(hr))return hr;
    if(status!=DecoderStatus::closed)hr=impl_->video->Close();
    if(SUCCEEDED(hr)){impl_->sound->Close();impl_->reader->Unbind();}return hr;
}
HRESULT Decoder21::Play(){return impl_->video?impl_->video->Play():CO_E_NOTINITIALIZED;}
HRESULT Decoder21::Stop(){return impl_->video?impl_->video->Stop():CO_E_NOTINITIALIZED;}
HRESULT Decoder21::Pause(bool pause){return impl_->video?impl_->video->Pause(pause?TRUE:FALSE):CO_E_NOTINITIALIZED;}
HRESULT Decoder21::Seek(std::int64_t time){if(time<0)return E_INVALIDARG;return impl_->video?impl_->video->Seek(time,sdk::BVL_SEEK_TIME):CO_E_NOTINITIALIZED;}
HRESULT Decoder21::SetVolume(unsigned int volume){if(volume>255)return E_INVALIDARG;return impl_->video?impl_->video->SetVolume(static_cast<INT>(volume)):CO_E_NOTINITIALIZED;}
HRESULT Decoder21::Info(VideoInfo21& out){
    if(!impl_->video)return CO_E_NOTINITIALIZED;
    sdk::BVL_VIDEO_INFO in{};HRESULT hr=impl_->video->GetVideoInfo(in);if(FAILED(hr))return hr;
    if(in.width<=0||in.height<=0||in.fps_rate<=0||in.fps_scale<=0||in.total_time<0||in.total_frames<0)return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    out={in.width,in.height,in.fps_rate,in.fps_scale,in.samplerate,in.channels,in.total_time,in.total_frames};return hr;
}
HRESULT Decoder21::Status(DecoderStatus& out){
    if(!impl_->video)return CO_E_NOTINITIALIZED;
    sdk::BVL_STATUS status{};HRESULT hr=impl_->video->GetStatus(status);if(FAILED(hr))return hr;
    switch(status){
    case sdk::BVL_STATUS_ERROR:out=DecoderStatus::error;break;
    case sdk::BVL_STATUS_CLOSED:out=DecoderStatus::closed;break;
    case sdk::BVL_STATUS_OPENING:out=DecoderStatus::opening;break;
    case sdk::BVL_STATUS_READY:out=DecoderStatus::ready;break;
    case sdk::BVL_STATUS_PLAYING:out=DecoderStatus::playing;break;
    case sdk::BVL_STATUS_PAUSED:out=DecoderStatus::paused;break;
    case sdk::BVL_STATUS_PLAYEND:out=DecoderStatus::ended;break;
    default:return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }return hr;
}
bool Decoder21::IsNextFrame(){return impl_->video&&impl_->video->IsNextFrame()!=FALSE;}
HRESULT Decoder21::Frame(CpuFrame21& out,bool canSkip){
    VideoInfo21 info{};HRESULT hr=Info(info);if(FAILED(hr))return hr;
    std::size_t width=static_cast<std::size_t>(info.width),height=static_cast<std::size_t>(info.height);
    if(width>static_cast<std::size_t>(INT_MAX)/4)return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    std::size_t pitch=width*4;
    if(height>impl_->policy.maxFrameBytes/pitch || height>static_cast<std::size_t>(INT_MAX)/pitch)return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    CpuFrame21 next;next.width=info.width;next.height=info.height;next.pitch=static_cast<int>(pitch);
    try{next.xrgb.resize(pitch*height);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}
    sdk::BVL_FRAME frame{};frame.frame_buf=next.xrgb.data();frame.frame_buf_size=static_cast<INT>(next.xrgb.size());
    frame.pitch=next.pitch;frame.width=next.width;frame.height=next.height;frame.pixel_format=sdk::BVLPF_X8R8G8B8;
    hr=impl_->video->GetFrame(frame,canSkip?TRUE:FALSE);
    if(hr!=S_OK)return hr;
    if(frame.frame_buf!=next.xrgb.data()||frame.width!=next.width||frame.height!=next.height||frame.pitch!=next.pitch||frame.frame_buf_size!=static_cast<INT>(next.xrgb.size())||frame.pixel_format!=sdk::BVLPF_X8R8G8B8||frame.frame_time<0||frame.frame_number<0)return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    next.time100ns=frame.frame_time;next.number=frame.frame_number;out=std::move(next);return hr;
}
HRESULT Decoder21::SoundClock(std::int64_t& out){return impl_->sound?impl_->sound->GetTime(out):CO_E_NOTINITIALIZED;}
}
