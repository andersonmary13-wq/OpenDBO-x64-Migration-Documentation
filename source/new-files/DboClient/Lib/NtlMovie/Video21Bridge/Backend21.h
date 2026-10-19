#pragma once
// Neutral boundary: this header must never include either vendor video header.
#include <windows.h>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace opendbo_bandi_adapter {
struct ModulePolicy {
    std::wstring absoluteDll;
    std::array<unsigned char,32> sha256{};
    std::size_t maxFrameBytes = 256u*1024u*1024u;
};
HRESULT ConfigureDefaultPolicy(const ModulePolicy&);
HRESULT ReadDefaultPolicy(ModulePolicy&);
struct SourceOps {
    std::function<int(unsigned char*,int)> read;
    std::function<std::int64_t(std::int64_t)> seek;
    std::function<std::int64_t()> position;
    std::function<std::int64_t()> length;
};
enum class DecoderStatus { error,closed,opening,ready,playing,paused,ended };
struct VideoInfo21 {
    int width=0,height=0,fpsRate=0,fpsScale=0,sampleRate=0,channels=0;
    std::int64_t duration100ns=0,totalFrames=0;
};
struct CpuFrame21 {
    int width=0,height=0,pitch=0;
    std::int64_t time100ns=0,number=0;
    std::vector<unsigned char> xrgb;
};
class Decoder21 final {
public:
    Decoder21();
    ~Decoder21();
    HRESULT Initialize(const ModulePolicy&,HWND);
    HRESULT Verify(LPCSTR,LPCSTR);
    UINT Version() const;
    HRESULT Open(const std::wstring&,std::shared_ptr<SourceOps>,bool silent,bool async);
    HRESULT Close();
    HRESULT Play();
    HRESULT Stop();
    HRESULT Pause(bool);
    HRESULT Seek(std::int64_t);
    HRESULT SetVolume(unsigned int);
    HRESULT Info(VideoInfo21&);
    HRESULT Status(DecoderStatus&);
    bool IsNextFrame();
    HRESULT Frame(CpuFrame21&,bool canSkip);
    HRESULT SoundClock(std::int64_t&);
private:
    Decoder21(const Decoder21&)=delete;
    Decoder21& operator=(const Decoder21&)=delete;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
