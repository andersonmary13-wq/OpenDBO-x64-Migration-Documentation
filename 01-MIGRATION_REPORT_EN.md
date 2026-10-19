# OpenDBO client migration to 64 bits

I migrated my corrected Win32 OpenDBO client to AMD64 while preserving my working baseline and the existing game behavior. I preserved my first full x64 build, applied the later pointer fixes and integrated video compatibility only into the x64 source copy. My frozen candidate contains 17,189 files, with 26 existing files changed and nine files added against the preserved baseline. I completed my new full build with zero errors and 573 warnings, with source and dependency pins matching before and after. I passed static product qualification and prepared my separate physical candidate installation.

I have now tested this x64 candidate for a few hours, **only on my own local server**. I observed no other errors during those sessions. **Excessive repeated Alt+Tab can cause the client to freeze; I had already encountered the same behavior in my 32-bit client.** I have not established its cause or confirmed a fix, and I am not presenting it as a newly introduced x64-only regression. My local observation does not establish exhaustive feature coverage, other hardware compatibility or production-server stability.

## How I preserved my working baseline

I started from upstream revision `ed30bbe393653c704130aa026baf2bcad02c31ff` with my accumulated local fixes. I preserved the complete patch, the new files and the hashes of all 191 canonical correction files. My pre-migration operational checkpoint is `97108e5f36ebf0e5140b4067c229f57034c73c33`.

I made two independent copies of 17,180 files, totaling 654,218,992 bytes each, under `Dragon Ball - Online/32bits/OpenDBO-Core` and `Dragon Ball - Online/64bits/OpenDBO-Core`. I compared their bytes with the original inventory before changing the x64 copy. I kept the historical copy frozen and preserved Client26, its PDB and MAP, build recipes, diagnostics, dependency inputs and the previous backups.

I kept my installed Client26 as Win32. Server19 was already AMD64 before this client migration. The historical folder name describes the client baseline rather than the server architecture. I did not restart the running server for this migration work.

## How I organized and verified the repository

I created a separate private game repository, rather than using my conversation repository. I published the selected historical sources to `main` in commit `fee30079e226108e5e9ffc690f77dafd344080b8`, followed by a migration-status description in `84501d2`.

I included 16,827 reviewed source and metadata files in the first commit, covering all 191 canonical correction files. I retained external SDKs and binary inputs privately and documented those dependencies. This source archive is not a self-contained installation package.

My first staging attempt hit a Windows path-length limit. I enabled `core.longpaths` for this repository, staged exact literal paths and preserved raw bytes without line-ending conversion. I verified staged Git blob IDs against the actual files. I used Git commands with the existing Git Credential Manager authentication, then fetched the remote independently into another bare repository. The complete tree, all selected paths and all 191 canonical blobs matched. The authenticated repository metadata confirmed private visibility and `main` as the default branch.

I investigated an unexpected untracked `devmaster.xml` before treating it as a build side effect. Its bytes and preserved timestamp matched the original copy. An inherited nested ignore exception explained its Git status. My complete readback still matched all 17,180 frozen baseline files.

I am keeping the unfinished x64 candidate outside stable `main`. Once I have validated and published the x64 base, I will create the new development branch from that base and preserve the historical Win32 folder.

## Which tools and dependencies I used

I used MSBuild, MSVC v142, Windows SDK 10.0.19041.0 and the June 2010 DirectX SDK. I used PowerShell and Python for inventories, hashes, build orchestration and native test runners. I retained Client's 21 direct project references and the compiled graph of 23 projects, including transitive dependencies, and bounded compiler concurrency.

I inspected PE and COFF machine types, linker MAP files and compiler command/read/write logs. I used SymChk and the executable's RSDS identity to match the PDB. This let me connect reviewed source changes to the actual compiled objects and linked archive members.

I retained RenderWare, Scaleform GFx, FMOD, Discord RPC, BugTrap and DevIL. I rebuilt JPEG 6b and PNG 1.2.18 for the current x64 toolchain while keeping those versions, because the old binaries required incompatible C-runtime conventions. I prioritized the official DirectX headers over older simplified copies.

I preserved the original GFx archive and prepared the private allocation-guard variant required by the existing client fixes. I changed two of its 188 objects and preserved the other 186. The native GFx proof passed 48 checks. My DevIL, BugTrap and Discord API proof passed 62 checks; the real FMOD wrapper proof passed four. These results cover the exercised APIs, with complete game audio and rendering still requiring integration tests.

## How I retained existing features

I found older x64 paths that disabled or substituted functionality. I restored the full Discord implementation and declarations, the BugTrap and Discord lifecycle calls, and the DevIL screenshot conversion path. The previous screenshot path could remove a BMP without producing the intended JPEG. I also retained my display-mode preference.

I kept the full FMOD wrapper with the real AMD64 import library. I identified an older library containing `fmodstub.obj` and selected the real dependency instead. I checked the wrapper symbols and real DLL imports in the resulting binary. I corrected two stale project-header paths by pointing them to their existing files, without dropping the headers or features.

## Which pointer problems I corrected

I tested with real addresses above 4 GB. In the baseline variants, I intercepted dangerous copies or modeled invalid address calculations before dereferencing them, so I could demonstrate the defect without deliberately executing an invalid memory access.

| Problem I found | Change I made | Result I observed |
| --- | --- | --- |
| Vertex, skinning and relocation calculations narrowed pointers. | I retained pointer-width differences and address arithmetic. | I passed 39 native RtWing relocation and CONTEXT-contract checks. I rebuilt the shader calculations; those isolated checks did not exercise GPU rendering. |
| Two graphics units selected a different `d3d9pipe.h` layout. | I selected the matching local header and rebuilt the world library. | I verified the actual local include in compiler dependency logs. |
| `NtlDumpTarget` used a string address as a length and could grow its buffer by too little. | I used the string length, sufficient bounded capacity and space for the terminator. | I passed 74 combined dump/entity/callback checks; the baseline had 33 controlled failures. |
| `NtlPLRenderGroup` reduced entity addresses to 32-bit map keys. | I used `uintptr_t` consistently for insertion, lookup and removal. | I retained distinct entities with equal low address bits. |
| Two resource-pack callbacks narrowed text pointers. | I passed full-width arguments to their existing `uintptr_t` callback API. | I checked producers, interface and consumer. |
| `NtlSystemInfo` passed one-byte destinations to `%2hx`. | I parsed into six unsigned shorts, checked all values and copied six bytes while retaining selection order and cleanup. | I passed 67 native checks using the real UCRT and mocked NetAPI, without reading the machine's MAC address. |
| Pipeline cluster ordering used a 32-bit pointer sentinel, and relocation stored its difference in 32 bits. | I used `UINTPTR_MAX` and native-width integer rebasing in `p2define.c`. | I passed 131 checks; the baseline had 44 controlled failures. |
| A freelist alignment mask complemented an unsigned 32-bit value before casting it to pointer width. | I moved the cast before the complement in one `bamemory.c` expression. | I passed 5,044 checks; the baseline had 432 controlled failures. I confirmed the defect is conditional because current initialization selects `NOFREELISTS`. |

I applied the reviewed later fixes only to the private x64 source copy. My first Client executable contains the 13 source changes frozen for that build. The next checkpoint contained 20 changed files; video integration brought the candidate to 35 changed or added paths. I preserved raw before/candidate files and reversible patches. I retain the first executable as historical evidence rather than installing it as though it contained those later changes.

I rebuilt `rpworld` with the two latest RenderWare fixes, with zero warnings or errors. I verified 36 AMD64 archives containing 240 objects; the other 35 archives were byte-identical to the preceding revision. Before replacing the world-library outputs, I preserved 112 earlier files totaling 10,232,449 bytes, including objects, symbols, logs and compiler tracking records.

## How I checked data and network compatibility

I compiled native Win32 and x64 probes using the real protocol headers. I obtained matching results for 2,216 structures, 14 aliases, 64 critical offsets and ten reference packets: 2,304 compared records. Both compilers and both probes completed with zero warnings or errors. I recorded the expected four-byte/eight-byte pointer difference separately from the network-data comparison.

I included contracts used by login, world entry, items, coupons and inventory sorting. This validates the examined representation; it does not replace transport and gameplay testing.

I also surveyed headers in 12,620 installed DFF files, seven BSP files and 28,182 geometry headers. I used header reads and seeks rather than loading every file through the complete engine parser. I did not find the native-geometry/world flags that activate a particular raw D3D9 structure serialization path in the examined headers. I recorded that path's layout differences as outside this qualification. I found stale enclosing lengths in 11,036 DFF files and recorded the reader's behavior without automatically labeling them corrupt.

## What my first complete x64 build produced

I required source and dependency checks before invoking the compiler. An early preparation attempt stopped at the two stale project-header paths; I retained that attempt and corrected the paths in the x64 copy.

My first complete Release x64 Client build finished with **zero errors and 598 warnings**. It produced a **14,576,128-byte** AMD64 executable, machine `0x8664`, with SHA-256 `e8cea6a1358d0ad4b1273615a8151d8c4cfe069858dfa60532abdb88f6f88b5b`. I preserved the EXE, PDB, MAP and build logs.

I checked its 18 direct imports and the immediate import closure of the six local images, including the examined Windows-library symbols and forwarders. I matched its PDB with SymChk and checked the real Discord, BugTrap, DevIL, FMOD and guarded graphics objects. I inventoried and triaged 376 unique warning diagnostics, distinguishing actual address truncation from intentionally encoded small integer tokens in the investigated paths. Warning debt remains; this triage does not mean I eliminated every warning or executed every affected path. I also recorded pre-existing limitations, including the already disabled offline Lua compilation wrapper, without presenting them as new migration failures.

## How I investigated video playback

I confirmed that the old x64 header expected `BVL64.dll` and the 1.1.0.5 `CreateBandiVideo` interface. I retained working video playback as a migration requirement while investigating a compatible dependency.

I identified MPEG-1-compatible video and MPEG Layer II audio in the four installed AVI files. I tested an AMD64 DirectShow graph using real Windows filters. The direct video connection failed because the AVI splitter's offered type did not match the MPEG-1 decoder's accepted type. In a second test, I built a descriptor from the real MPEG sequence header: the decoder accepted it, but the splitter did not. I preserved both failures without forcing that connection or installing system codecs.

I then evaluated the [preserved Bandi Video SDK 2.1.5.19](https://github.com/sigmaco/bandi-video-v2.1.5.19) supplied during this investigation. I checked the downloaded files against their Git blobs. Windows validated the Bandisoft Authenticode signature and timestamp on `bdvid64.dll`. I confirmed its actual `CreateBandiVideo2` export and the changed interface, including caller-owned CPU frame buffers. I implemented an explicit compatibility layer while retaining the new SDK's actual name and API.

I initially obtained correct frame counts and EOF with RGB24 buffers, but pixel inspection showed entirely black images. I retained that attempt and corrected its interpretation in an addendum: frame progression alone did not prove useful decoded images. The normal existing product verification returned `S_OK`; invalid controls returned `E_FAIL`. I recorded those results without including the verification arguments in the reports. RGB24 remained black in the examined case after verification.

I repeated the test with the game's `X8R8G8B8` output format and obtained useful images through EOF in all four files. I recorded per-frame hashes, contiguous indices and monotonic timestamps and visually examined 12 extracted PNGs.

| File I tested | Frames through EOF | Frames with nonzero RGB |
| --- | ---: | ---: |
| `logo.avi` | 550 | 548 |
| `opening.avi` | 4,580 | 4,580 |
| `Age1000.avi` | 4,585 | 4,585 |
| `The_base_of_Tenkaichi.avi` | 7,042 | 6,987 |

I observed fades and transitions among the 12 PNGs I examined. I did not visually classify every one of the 57 zero-RGB frames counted in the table. I observed nonzero PCM, but the accelerated silent test clock and drain do not establish exact complete audio duration, AV synchronization or device playback. I am retaining those as integration requirements.

I implemented all 30 legacy methods, including synchronous/asynchronous opening, command ordering, repeat, pause, volume, alpha, source readers, time conversion, D3D9 textures, frame leases, cleanup and EOF notification. I retained real SDK verification and both DirectSound and silent modes. I integrated the adapter into both game consumers and included it in the qualified complete build. I retain the runtime limits described below. The external SDK remains a private dependency; a valid binary signature and API result do not establish redistribution rights.

I ran an isolated facade proof with a hidden window, real Windows GDI, threads and messages, and an explicitly simulated decoder. I passed 144 checks covering the exercised contracts, including queues, asynchronous failures, frame leases and stale notifications. I passed another 29 checks for backend reading, hashing and module-policy validation. These results do not establish real SDK decoding or rendering.

While implementing the new audio path, I found that overlapping opens could leave an old session running, and a pause during a PCM callback could discard an already consumed chunk. I preserved the first implementation and compared it with the corrected revision under the same native oracle: the new revision passed 901 checks; the old revision had seven controlled failures. I reviewed callback ordering, open admission and cancellation before activating the revision in the private adapter. Those checks use controlled callbacks; physical audio and AV synchronization remain pending.

I also ran a separate Win32 program against the old DLL to measure legacy behavior without changing the installed client. I confirmed that time conversions retain finite negative values and truncate frame counts toward zero. My first alpha test did not establish that a new frame had arrived after seeking, so I retained it without claiming alpha equivalence. The RGB565 case failed inside the old DLL's open operation in that program. I recorded that test limitation without attributing it to gameplay or requiring the adapter to reproduce the failure.

I refined that measurement with independent processes and seven render-time advances before capture. All six X8/A8 cases with alpha 0, 128 and 255 produced identical memory pixels with alpha 255. I repeated it using my own hidden D3D9 device: the old DLL returned an X8 MANAGED texture with alpha 255 in all six cases, with hashes matching the memory frame. I used those observations to align the adapter, without claiming complete game-compositing equivalence from that probe.

I also exercised an earlier adapter revision with all four AVI files, D3D9 texture upload/readback, pause, seek, frame leases, device recovery and EOF, including a DirectSound case at volume zero. I passed 165 checks in those five cases and retained that historical result. These sampled/seek-based tests are separate from the earlier complete CPU decoding runs and do not establish full real-time AV playback.

I corrected two defects in my adapter implementation. I had queued asynchronous opening both in the facade and the SDK, allowing a subsequent seek to run before opening completed. I retained asynchronous admission in the facade and used synchronous SDK opening inside its worker to preserve command ordering. Independent review also found that a frame returning from a source callback after Close or Stop could be published as current. I rechecked its generation before publication. The same oracle improved from 152 checks with two controlled failures to 152 with none.

I also found a device-recovery difference: the old client received MANAGED textures, while my first candidate created DEFAULT textures. I confirmed that the global release-handler dispatcher ignores callback return values, so returning Busy would not prevent reset. I preserved that candidate and corrected the implementation to retain the old SDK's observed MANAGED pool. My real-device proof passed 210 checks across five cases of 42 checks, using the four AVI files and a muted DirectSound case. I confirmed successful D3D9 reset with a frame still leased and useful readback from the same texture afterward.

I separately passed six format/alpha cases of 35 checks each, also totaling 210. I compared X8/A8 and alpha 0, 128 and 255 with the observed old-SDK behavior: X8 output with alpha 255. These two groups exercise different contracts. I do not present them as two complete audiovisual playback runs or add their counts as a measure of feature coverage.

I extracted 13 actual consumer and global-manager methods into a native controlled-failure fixture using native SDK1 declarations and the integrated x64 wrapper, with controlled D3D9 interfaces. The corrected integration passed 292 checks. The preceding baseline executed 274 checks with 82 controlled failures; 18 additional checks were conditional on safe paths in the corrected implementation. The corrected fixture compiled with zero warnings; the baseline compilation had one C4100 warning. These checks cover contracts, ordering and reference handling rather than complete game execution or the real driver's behavior.

I reviewed the six existing files and nine new integration files, checked their hashes and applied them only to the x64 source tree. I retained the Win32 paths and legacy interface signature, isolating the new SDK in separate translation units. My new complete build uses that frozen source set.

## How I corrected generated-file destinations

I expanded source verification to reject extra files as well as changes to inventoried files. This found five generated products in the candidate source tree: a copied `fmod.dll` and four MIDL outputs. I preserved those products and the stopped attempt before removing them from the source tree. They were absent from the historical 17,180-file inventory.

I traced the MIDL commands and found that the type-library destination was already private, but the remaining output directory was unspecified. I directed `/out` to the project's private intermediate directory while retaining MIDL generation. I also corrected the FMOD copy-event selection: the event belonged to Client, while the previous selection addressed NtlSound only. I now stage the actual AMD64 DLL through the verified dependency contract, retaining the dependency in the product.

I preserved the previous recipe and tested the new one with an isolated MIDL run and a complete NtlXMLLoader build. Both completed with zero warnings and errors without recreating those five files in the source tree. I recorded that the existing C++ sources still use their previously selected headers; generating the four MIDL products does not establish that those products are consumed by C++. The complete build recipe now checks for extra files in the frozen source tree as well.

## What my new complete build produced

I ran the integrated Release x64 build with the corrected output recipe and at most two concurrent compiler workers. MSBuild completed in 19 minutes and 10.94 seconds with **zero errors and 573 warnings**. I obtained a **14,668,288-byte** AMD64 `Client.exe`, SHA-256 `5b38017566a467534cc99f5f9eed20daa8f54eda2c66d5a9037c19951f5abe2b`. I rechecked all 17,189 source files and dependency pins and staged the six actual AMD64 DLLs only in the private build directory, preserving my running installation.

I compared the warnings with the first build's 598, normalizing paths and line numbers. I found 25 removed diagnostics and none added: 20 removals related to the dump, MAC, entity-identity, callback and cluster changes, and five to the old Windows-version path in the replaced video wrappers. I did not equate 25 diagnostics with 25 distinct defects: a cast can produce two diagnostics, and one MAC scan produced six. I confirmed zero warnings in the four new video-bridge translation units. I retain the remaining 573 warnings as explicit warning debt.

My first qualification attempt for this new product passed the PE, import and symbol checks but stopped at the `p2define.c` provenance chain. I had required the `rpworld.lib` produced by the isolated graphics rebuild, while the complete Client build also reconstructs that library through Ntl_Plugin_World and selects its new output. I preserved the attempt and traced the actual compiler command, object, archiver command and MAP. I confirmed the corrected source's chain to the selected library and corrected the verifier to check that actual chain while recording the earlier archive as unselected. I did not change the product to satisfy my verifier's incorrect assumption.

I reviewed and ran the corrected verifier and passed static product qualification. I checked Client's 20 direct imports and the immediate closure of seven local images, examining 1,528 symbol references. I confirmed the actual `CreateBandiVideo2` export, CNG and DirectSound imports and the matching local PDB with SymChk. I connected the changed compiled sources to actual compilation and archiving records, checked relevant headers in compiler read records and checked project metadata against the frozen recipe. I verified all 35 changed or added source paths against the reviewed freeze, distinguishing selected libraries from preserved outputs unused by this link. I do not present static results as gameplay execution or stability certification.

I prepared my separate physical candidate installation with that actual `Client.exe`, its six qualified AMD64 DLLs and a launcher. I checked the 623 data files before and after staging, preserving my profiles, configuration and assets. I removed `CANDIDATE_NOT_READY.txt` only after those checks and recorded `READY_FOR_HUMAN_TEST_NOT_STABLE`. I retained PDB, MAP and receipts privately. I did not launch the Client or restart my server during staging. I use the installed Windows AMD64 D3DX9 dependency, which I checked separately from its I386 counterpart.

## How I preserve the investigation

I retain source inventories, frozen copies, raw before/candidate files, reversible patches, recipes, dependency contracts, previous binaries and objects, build logs, native test results, hashes, controlled captures and publication receipts. I preserve failed attempts and corrective addenda. I will preserve dated, hashed snapshots of this report as it develops.

I keep the detailed technical artifacts privately and link the report's results to the corresponding evidence receipts. I will review the community version without including credentials, private runtime data or external SDK binaries.

## My local test and remaining limits

I ran the x64 candidate for a few hours on my own local server. I did not observe any other errors during those sessions. This is a limited local observation, not an exhaustive check of every feature, hardware configuration or reconnect path. I have not tested public or production servers as part of this report.

Excessive repeated Alt+Tab can still freeze the client. I had already encountered the same behavior on Win32, so I am not presenting it as a newly introduced x64-only regression. I have not established its cause or confirmed a fix.

I am sharing my migration process, the technical documentation and measured test results with that limitation. I keep native and static proofs tied to their revisions; I do not infer complete rendering, audiovisual synchronization or long-term stability from these few hours of play. I preserve the Win32 archive and keep the stable-publication/development-branch transition separate from this experience report.

My earlier local-fix follow-up is available as [issue 137](https://github.com/OpenDBO/OpenDBO-Core/issues/137), following [issue 135](https://github.com/OpenDBO/OpenDBO-Core/issues/135). Those reports document my existing baseline fixes; I am not presenting their upstream overlaps as new original work in this migration.

— NoobMaster70
