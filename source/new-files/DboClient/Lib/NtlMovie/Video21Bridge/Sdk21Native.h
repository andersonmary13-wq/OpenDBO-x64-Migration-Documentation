#pragma once
// SDK2-only translation units. Never include this together with SDK1's
// BandiVideo.h: preprocessor macros are not isolated by a C++ namespace.
#include <windows.h>
#include <unknwn.h>
#include <cassert>
#include <tchar.h>
namespace opendbo_bandi_sdk21 {
#include <bandivid.h>
}
