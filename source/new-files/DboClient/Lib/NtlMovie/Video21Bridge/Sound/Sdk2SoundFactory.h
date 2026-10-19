#pragma once
// Private migration candidate. SDK2-only interface, never include SDK1 here.
#include "../Sdk21Native.h"

namespace opendbo_bandi_adapter {
struct IAdapterSound : opendbo_bandi_sdk21::IBandiVideoSound {
    // Change only between Close and Open; retains the same SDK2/Verify instance.
    virtual HRESULT SetSilentMode(BOOL silent)=0;
};
// Returns one caller-owned reference; does not open a device or start a thread.
HRESULT CreateAdapterSound(HWND owner,IAdapterSound** out);
}
