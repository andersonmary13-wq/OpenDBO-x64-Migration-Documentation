# My physical x64 bootstrap — historical private proposal, without installation

> I preserve this proposal's original preparation state in this English copy. Its V6 and pending-test statements are historical; I record later V7 qualification, staging and limited local play separately in issue #138. I replace only my personal absolute destination path with `<PROJECT_ROOT>/client-local/64bits`.

I prepared this plan for `<PROJECT_ROOT>/client-local/64bits`. I checked that the physical copy already contains 623 data files and `CANDIDATE_NOT_READY.txt`, without EXE/DLL/PDB files. I keep the 32-bit client independent, with 633 regular files. I did not move assets, create links or reuse the 32-bit executable during this stage.

I recorded native V6 build session 98768, which was still running when I prepared this proposal. I did not start the product through this document. I separate compilation, static qualification, staging and human testing; I still kept gameplay validation pending for the x64 candidate at that point.

## My source tree and integration

I checked that the integrated freeze contains 17,189 files and 35 intentional deltas, with declared preservation of every byte in the historical 32-bit tree, including 191 canonical correction source files. I divide the 35 deltas into 15 video-integration files (six existing and nine new), nine RenderWare pointer/memory files, five memory/identity/resource-callback files and six project/external-resource integration entries. I retain the authoritative list and hashes in the freeze; this text does not replace them.

I kept both existing video consumers: `CMoviePlayer` and `CNtlMovie`/`CNtlMovieManager`. I adapted the x64 integration to use a typed factory and keep SDK1 and SDK2 separated by translation unit. In `Video21Integration.cpp`, I obtain the Unicode EXE path and require the sibling `bdvid64.dll` with its qualified SHA. I do not reinterpret_cast the SDK2 factory into the SDK1 interface or use a fake HMODULE or fabricated Verify result.

I check Verify's HRESULT in `CMoviePlayer` and validate the EOF generation/instance. I keep its original `ReleaseHandler` intact: it marks deferred release. I use RAII for the lease and COM references in `CNtlMovie::UpdateRenderTexture`; my reset callbacks preserve the Close/reopen/seek flow. I deliver a MANAGED/usage0/X8 texture through the bridge, as observed in SDK1, and I had already exercised a live lease through Reset on an owned device. I do not treat that as validation of the complete product's event loop, scenes or audio.

I prepared `Client64.Output.v2.props` to keep native outputs in private directories, redirect MIDL to `IntDir/midl` without removing generation, and disable the post-build FMOD copy into the `DragonBall` source folder. I retain explicit dependency-binary staging in the pipeline. I passed the isolated `NtlXMLLoader` build; its tracker does not show its C++ sources consuming the new `_h.h/_i.c` files, so I do not claim that read.

## My byte-for-byte data preservation

I preserve all of `pack` (454 files), `movie` (four), `font` (four), `cursor` (36), `ts` (four), `ScreenShot` (104), `user` (13) and the four existing files in the data/config/log root. I do not replace local profiles, SystemEnv or `ConfigOptions.xml` with source defaults or another character's settings. I retain the 623-file copy's own manifest as my preservation reference.

I checked that `CDboGlobal::Create` reads `ConfigOptions.xml` through its relative path; `LoadConfigOptions` uses `/config_options/op` attributes for the address/port and also reads localization/BugTrap options. I did not print their values. I do not rewrite any IP, account, password or graphics option through this proposal. I preserve the asset package without treating preservation as certification of every possible native stream or binary layout between 32 and 64 bits.

## My proposed binaries for the x64 folder

After confirming exit0 and qualifying that same artifact, I plan to copy only `Client.exe` and the six exact AMD64 DLLs from contract V3:

| Destination name | My qualified source |
| --- | --- |
| `DevIL.dll` | I use AMD64 DevIL staging. |
| `BugTrap-x64.dll` | I use AMD64 BugTrap staging. |
| `BugTrapU-x64.dll` | I use AMD64 Unicode BugTrap staging. |
| `discord-rpc.dll` | I use AMD64 Discord staging. |
| `fmod.dll` | I use real AMD64 FMOD, with the static compatibility wrapper linked. |
| `bdvid64.dll` | I use SDK2 with its signature/origin and SHA checked, retaining real Verify. |

I do not carry `fmod_shim.dll`, 32-bit BVL or any other DLL from the Win32 client folder. I retain PDB/MAP files and receipts privately for diagnosis; I do not need them in the data root.

I confirmed that `d3dx9_43.dll` installed in `C:/Windows/System32` is AMD64 (SHA `84b900dbd7fa978d6e0caee26fc54f2f61d92c9c75d10b35f00e3e82cd1d67b4`). I found the `SysWOW64` copy to be I386. I plan to resolve the installed x64 module without copying the I386 one or installing system components at this stage. I require VCRuntime/UCRT and the remaining imports to be included in the final artifact's local static qualification.

## My launcher and admission checks

I preserved the original launcher's contract: it changes directory into the data folder and calls `Client.exe` without arguments. I retain that contract, with cwd equal to the physical x64 folder and an empty argument vector. I found optional automatic-login support in the parser, but I include neither credentials nor those arguments in this proposal. I will enter the game through the normal human flow.

I prepared `prepare_bootstrap64.py` to remain PREPARED_ONLY by default. Even `--plan` only reads data and writes a private plan; I include no mode for copying, installing, removing, executing processes or starting the game in that planner. I require native metadata exit0, V6 qualification from the same generation (seven roots, imports and symbols), freeze/contract pins, a destination still without PEs and preservation of all 623 data files. I also keep the launcher template outside the client folder and retain its block while `CANDIDATE_NOT_READY.txt` exists.

Before staging, I revalidate those admission checks immediately before copying and read back the seven PEs and their closure at the final local path. I leave the sources and 32-bit copy untouched and explicitly release a human test only afterward. I retain the blocking marker until all checks finish. I did not operate services, SQL, an active launcher or a game process during this preparation.

I retain upstream and external-SDK credits, licenses and provenance. I do not attribute authorship of those components to myself through this bootstrap plan.

**NoobMaster70**
