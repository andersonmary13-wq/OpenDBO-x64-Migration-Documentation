#define NOMINMAX
#include "Video21Integration.h"
#include "Backend21.h"
#include <d3d9.h>
#include <tchar.h>
#include <cassert>
#define OPENDBO_BANDI_V1_HEADER "../BandiVideo.h"
#include "LegacyVideo21Adapter.h"
#include <algorithm>
#include <new>
#include <string>
#include <vector>

namespace opendbo_bandi_adapter {
HRESULT ConfigureInstalledVideo21Policy()try{
    std::vector<wchar_t> path(512);
    for(;;){
        SetLastError(ERROR_SUCCESS);
        DWORD size=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
        DWORD error=GetLastError();
        if(!size)return HRESULT_FROM_WIN32(error?error:ERROR_GEN_FAILURE);
        if(size<path.size()&&error!=ERROR_INSUFFICIENT_BUFFER){
            std::wstring value(path.data(),size);auto slash=value.find_last_of(L"\\/");
            if(slash==std::wstring::npos)return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            ModulePolicy policy;policy.absoluteDll=value.substr(0,slash+1)+L"bdvid64.dll";
            const unsigned char digest[32]={0x83,0x47,0xe8,0x60,0x8c,0x25,0x19,0x80,0x2f,0x64,0x18,0x3e,0xbd,0x06,0x5f,0xb5,0xf4,0xcb,0x8d,0x24,0x46,0x4a,0x3f,0x39,0xc8,0xab,0x51,0x66,0xd0,0x0c,0xe5,0x18};
            std::copy(digest,digest+32,policy.sha256.begin());
            return ConfigureDefaultPolicy(policy);
        }
        if(path.size()>=32768)return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        path.resize((std::min)(path.size()*2,std::size_t(32768)));
    }
}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_UNEXPECTED;}
// Only the typed x64 factory above is admitted by the two migrated consumers.
// A foreign SDK1 object must not be passed to these bridge-only hooks.
bool IsCurrentVideo21Eof(IBandiVideo* video,WPARAM instance,LPARAM generation){
    return video&&static_cast<LegacyVideo21Adapter*>(video)->ConsumeCurrentEofNotification(instance,generation);
}
HRESULT BeforeVideo21DeviceReset(IBandiVideo* video){return video?static_cast<LegacyVideo21Adapter*>(video)->BeforeDeviceReset():E_POINTER;}
HRESULT AfterVideo21DeviceReset(IBandiVideo* video){return video?static_cast<LegacyVideo21Adapter*>(video)->AfterDeviceReset():E_POINTER;}
}
