# My process for migrating the client to 64 bits

> I distinguish the verification of all 35 changed or added source pins from compiled-source provenance: I check actual compilation/archive/link chains for compiled sources, compiler-read dependencies for relevant headers, and frozen metadata for project files. I retain the historical wording below without implying that headers or project files each generate an object file.

> I prepared this complete translation for review only; I have not authorized or performed its publication. I retain the source record's sequence, technical scope and historical pending statements. Later limited local-play observations belong to the separate issue #138 record and do not retrospectively expand the earlier tests. I use `<WORKSPACE>` for my local project root, `<PRIVATE_EVIDENCE_ROOT>` for my private technical evidence directory, and `<PRIVATE_REPOSITORY_URL>` for my private repository address.

I began this migration from my latest locally corrected OpenDBO base, preserving the Win32 client and the server in use. My goal is to deliver a functional AMD64 client with the existing corrections and behavior, publish the validated base, and then continue development on a new branch. In this record, I describe what I have already done, the problems I found, the test results, and what I still need to complete.

**Status of this revision on October 19, 2026:** I published the 32-bit client's source history in my private repository and preserved the first complete x64 build. I then corrected the pointer problems I found and integrated the video adaptation only into the x64 copy. I verified that the 17,180 historical files remained intact and checked the migration's 35 new or changed paths in a candidate tree containing 17,189 files. I completed the new full build with zero errors and 573 warnings, statically qualified the product, and prepared the separate physical copy for my in-game test. I have not yet completed that test or published the candidate as stable.

## 1 How I preserved the existing base

I used upstream revision `ed30bbe393653c704130aa026baf2bcad02c31ff` and my accumulated local corrections as the starting point. I recorded the 191 canonical files in that set, including the new files, and preserved the complete patch and source hashes. The operational checkpoint preceding the migration is `97108e5f36ebf0e5140b4067c229f57034c73c33`.

I created two independent copies of 17,180 files, each containing 654,218,992 bytes, under `Dragon Ball - Online/32bits/OpenDBO-Core` and `Dragon Ball - Online/64bits/OpenDBO-Core`. I checked both copies' bytes against the source inventory before making changes. I kept the historical copy frozen and made the adaptations in the 64-bit copy. I also preserved the Client26 executable, its symbols and diagnostic files, build recipes, local dependencies, and backups of the previous version.

I retained the installation used for gameplay. The installed Client26 remains Win32, with SHA-256 `42d9540c0b3c40396e3c9cea0b381abb35844e9825e107f0b4fd3aeef0d22519`. Server19 was already AMD64 before this migration. The historical folder name `32bits` identifies the client base, not the server architecture. I did not restart the server to perform the work described here.

## 2 How I organized GitHub

I created my private Dragon Ball Online repository (`<PRIVATE_REPOSITORY_URL>`), separate from the conversations repository. I published the historical archive on `main` in commit `fee30079e226108e5e9ffc690f77dafd344080b8`, then added the migration-status description in commit `84501d2`.

I selected 16,827 files for the first commit: audited sources and metadata, including the 191 canonical correction files. I preserved the required external binaries and SDKs locally; publishing the sources does not create a self-contained installation package. I recorded the dependencies in `DEPENDENCIES.json` and retained the selected files' authorship and license notices.

I encountered a path-length limit during the first `git add`. I enabled `core.longpaths` only in the new repository and repeated the selection using literal paths. I also configured the selection to preserve the original bytes without converting line endings. Before sending it, I compared the staged blob identifiers with the file bytes.

I used Git through the terminal with Git Credential Manager and the existing authentication. After the push, I independently read back the remote in a separate bare repository: the entire tree, all 16,827 paths, and the 191 canonical blobs matched. The authenticated API confirmed private visibility and the default branch `main`.

I found `devmaster.xml` appearing as an untracked file and investigated before attributing it to the build. I confirmed that the file was already part of the original copy and retained the same bytes and date. The cause was an inherited exception in the internal `.gitignore`. The complete verification confirmed that none of the 17,180 preserved files had been contaminated.

I keep the x64 candidate outside `main` while validations remain outstanding. After validating and publishing the x64 version, I will create the new development branch from that base. I will develop future functionality on that branch and preserve the historical folder.

## 3 Which tools and dependencies I used

I built with MSBuild, MSVC v142, Windows SDK 10.0.19041.0, and the June 2010 DirectX SDK. I used PowerShell and Python to prepare inventories, verify hashes, and run the builds and native proofs. I retained Client's 21 direct references and the compiled graph of 23 projects, including their transitive dependencies, and limited compiler concurrency to control resource use.

I checked the PE/COFF architecture of executables, DLLs, objects, and libraries. I examined MAP files and compiler read, write, and command records to connect the sources to the objects actually compiled and the libraries selected by the linker. I used SymChk and the executable's RSDS identity to check the PDB match.

I retained RenderWare, Scaleform GFx, FMOD, Discord RPC, BugTrap, DevIL, and the other dependencies required for the existing behavior. I rebuilt JPEG 6b and PNG 1.2.18 for x64 with the current compiler while preserving their versions, because the old files depended on C-runtime conventions that did not work with the new link. I used the official DirectX headers instead of the older simplified headers that produced type incompatibilities.

I preserved the original GFx archive and prepared a private variant with the two allocation corrections already required by the client. Of the archive's 188 objects, I changed two and preserved the other 186. The native GFx proof passed 48 checks. The DevIL, BugTrap, and Discord API proofs passed 62 checks, and the real FMOD-wrapper proof passed four. Those proofs exercised the specified APIs; I still need to validate the complete audio and visual experience inside the game.

## 4 How I preserved functionality during the port

I found old x64 paths that substituted or disabled functionality. I restored compilation of the real Discord implementation and its complete headers. I retained the application's BugTrap and Discord initialization and shutdown calls.

I restored the DevIL path used for screenshots. The previous x64 path could delete the BMP without converting it to JPEG. I also retained my display preference by removing the x64-specific override that forced a different display-mode choice.

I retained the complete FMOD wrapper and the real AMD64 import library. I identified an old archive containing `fmodstub.obj` and removed it from the link selection in favor of the real dependency. I checked the wrapper symbols in the MAP and the real DLL imports; I did not use that stub to obtain an apparently completed build.

I corrected two old header paths in the project files, pointing them to the files that actually existed. I did not remove a header or resource to bypass those preparation failures.

## 5 Which pointer problems I found

I tested the algorithms with real addresses above 4 GB and compared the results with the preceding version. In the proofs of the old versions, I intercepted dangerous operations before they accessed invalid addresses. This let me demonstrate data loss without deliberately causing invalid memory accesses.

| Problem I found | How I corrected it | Result I obtained |
| --- | --- | --- |
| I found pointer conversions in RenderWare vertex, skinning, and relocation calculations. | I used pointer-compatible differences and sizes while retaining the algorithms' logic. | I obtained 39 native checks of RtWing relocation and the CONTEXT contract. I rebuilt the shader calculations, still without a GPU-rendering proof. |
| I found two translation units compiling against a different copy of `d3d9pipe.h`. | I made the include select the local header matching the implementation and the public interface in use. | I confirmed local resolution in the actual compiler records and rebuilt the library. |
| I found a string address being used as a length in `NtlDumpTarget`. Buffer growth could also be insufficient. | I used the string length, reserved enough space for the data and terminator, and protected the calculation against overflow. | I obtained 74 passing checks in the set covering the dump, entity, and callback adjustments; the previous version had 33 controlled failures. |
| I found entity pointers reduced to 32-bit keys in `NtlPLRenderGroup`. | I used `uintptr_t` as the identity in the map and its lookup and removal operations. | I preserved entities with identical lower 32 bits but different complete addresses. |
| I found text pointers reduced to `unsigned int` in two packing callbacks. | I retained the complete address when passing arguments to the API that already accepted `uintptr_t`. | I checked the producers, the interface, and the existing consumer. |
| I found `%2hx` writing into one-byte destinations in `NtlSystemInfo`. | I read the values into six `unsigned short` elements, validated the complete read, and copied six bytes. I retained interface-selection order and released the buffer on every return path. | I obtained 67 passing checks with real UCRT and simulated NetAPI, without reading the computer's MAC address. |
| I found the `0xFFFFFFFF` sentinel limiting cluster-address sorting and a relocation offset stored in 32 bits. | I used `UINTPTR_MAX` and native-width address arithmetic in `p2define.c`. | I obtained 131 passing checks; the previous version had 44 controlled failures. |
| I found an alignment mask complemented in 32 bits before conversion to a pointer. | I performed the conversion before the complement in one `bamemory.c` expression. | I obtained 5,044 passing checks; the previous version had 432 controlled failures. I confirmed that the defect is conditional because the current initialization uses `NOFREELISTS`. |

I applied these latest candidates only to the x64 copy, after the proofs and reviews. I preserved the preceding files and reversible patches. The first x64 `Client.exe` contains the first build's 13 source changes. The next checkpoint had 20 changed files; after the video integration, the tree frozen for the new build had 35 paths different from the base: 26 modified existing files and nine new files. I retain the first build as historical evidence rather than installing it as though it contained those later changes.

I rebuilt `rpworld` with the two latest RenderWare adjustments. That rebuild completed without warnings or errors. I checked 36 libraries and 240 AMD64 objects, with the other 35 libraries identical to the preceding revision. Before replacing the library results, I preserved 112 previous files containing 10,232,449 bytes, including objects, symbols, logs, and compiler records.

## 6 How I checked data formats and protocols

I compared Win32 and x64 proof executables built with the real protocol headers. I obtained equivalence for 2,216 structures, 14 aliases, 64 critical offsets, and ten reference packets, totaling 2,304 compared records. Both compilers and both programs completed successfully without warnings or errors. I recorded the expected pointer-size difference, four versus eight bytes, outside the network-data comparison.

I included the contracts used by login, world entry, items, coupons, and inventory sorting. This comparison validates the examined data representation; transport and in-game action tests remain necessary.

I also examined the headers of 12,620 DFF and seven BSP installed resources, including 28,182 geometry headers. I performed this analysis by reading and searching headers rather than loading every file through the engine's complete parser. In the examined headers, I did not find the native geometry or world flags that would activate raw serialization of certain D3D9 structures. I recorded that this serialization path differs between Win32 and x64 layouts and remains outside this qualification. I found legacy container lengths in 11,036 DFF files and recorded the reader's behavior without automatically classifying them as corruption.

## 7 How I ran the first complete x64 build

I prepared a check of sources and dependencies before starting any compiler. The first preparation attempt stopped because of the two old header paths. I preserved that attempt and corrected the paths in the x64 copy.

I ran the complete Release x64 build with all Client references. It finished with **zero errors and 598 warnings**, producing an executable of **14,576,128 bytes**, with PE AMD64 `0x8664` and SHA-256 `e8cea6a1358d0ad4b1273615a8151d8c4cfe069858dfa60532abdb88f6f88b5b`. I preserved the EXE, PDB, MAP, and logs.

I checked the 18 direct imports and immediate resolution of the six local images, including the required symbols and forwarders in the examined Windows libraries. I confirmed the PDB identity with SymChk. I confirmed the real Discord, BugTrap, DevIL, and FMOD implementations and the protected graphics objects in the MAP. This static qualification does not replace execution of the game.

I inventoried and triaged the 598 warnings, which produced 376 unique diagnostics. I distinguished actual truncated addresses from small integer identifiers intentionally stored as tokens in the investigated paths. I also recorded preexisting limitations, such as the already disabled offline Lua-compilation wrapper, without automatically attributing them to the migration. This triage does not mean that every warning was eliminated or every path was executed.

## 8 How I investigated and tested video

I confirmed in the project header that the old x64 path expected `BVL64.dll`, with the `CreateBandiVideo` interface from version 1.1.0.5. I did not find a compatible official distribution in the initial search, and I retained video playback as a migration requirement.

I examined the four installed AVI files and identified compatible MPEG-1 video and MPEG Layer II audio. I tried an AMD64 DirectShow graph with the real Windows filters. Direct video connection failed because the AVI splitter offered a type that the MPEG-1 decoder did not accept. In a second proof, I constructed the descriptor from the video's actual header: the decoder accepted it, but the splitter did not. I preserved both failures and neither forced the connection nor installed system codecs.

I then evaluated SDK 2.1.5.19, identified during the migration and preserved in the [SIGMA repository](https://github.com/sigmaco/bandi-video-v2.1.5.19). I checked the files against their Git blobs and checked `bdvid64.dll`'s Authenticode signature: Windows validated the Bandisoft signature and timestamp. I confirmed that the DLL exports `CreateBandiVideo2` and uses a different interface with frame buffers in caller-owned memory. I therefore prepared an explicit adaptation between the interfaces, retaining the new SDK's real name and API.

I preserved an initial attempt in which frame counts and video completion appeared correct, but pixel inspection revealed completely black frames in 24-bit RGB format. I corrected my interpretation through an addendum: counts and EOF alone were insufficient to approve the image. I also tested the SDK's normal verification: the existing product arguments returned `S_OK`, while invalid controls returned `E_FAIL`. I recorded only those results without reproducing the arguments in the reports.

I repeated the proof in the game's `X8R8G8B8` format and obtained useful images from all four complete AVI files. I recorded per-frame hashes, indices, and monotonic timestamps and examined 12 extracted PNGs for visual checking.

| File I tested | Frames through EOF | Frames with nonzero RGB |
| --- | ---: | ---: |
| `logo.avi` | 550 | 548 |
| `opening.avi` | 4,580 | 4,580 |
| `Age1000.avi` | 4,585 | 4,585 |
| `The_base_of_Tenkaichi.avi` | 7,042 | 6,987 |

I observed transitions and fades in the 12 examined PNGs. I did not visually classify every one of the 57 zero-RGB frames counted in the table. I observed nonzero PCM, but the accelerated clock and silent-test drain do not prove the audio's exact duration, sound/image synchronization, or playback on a device. At that stage, those checks remained part of integration.

I implemented compatibility for all 30 methods of the old interface: synchronous and asynchronous opening, the command queue, repeat, pause, volume, alpha, data sources, time conversions, D3D9 textures, frame borrowing and return, shutdown, and end notifications. I retained real SDK verification and the DirectSound and silent modes. I integrated this implementation into both game consumers, still subject at that stage to the new complete build and product testing. I keep the DLL and external SDK sources as private dependencies; neither the signature nor the API result certifies redistribution rights.

I ran an isolated facade proof with an invisible window, GDI, threads, and real Windows messages, using an explicitly simulated decoder. I obtained 144 passing checks for the exercised contracts, including the queue, asynchronous failures, frame leases, and legacy notifications. I obtained another 29 passing checks for backend reading, hashing, and policy. Those results are not decoding or rendering tests using the real SDK.

I found two failures while implementing the new audio path: overlapping opens could leave an old session active, and pausing during a PCM callback could discard an already consumed portion. I preserved the first implementation and compared the corrected revision under the same native oracle: the new revision passed 901 checks; the old one had seven controlled failures. I reviewed callback ordering and open admission and cancellation before activating the revision in the private adapter. The test used controlled callbacks; physical audio and synchronization remain pending.

I also ran a separate Win32 program against the old DLL without changing the installed client, to measure contracts rather than assume them. I confirmed that time conversions preserve finite negative values and truncate frame counts toward zero. I preserved an initial alpha test that did not guarantee a fresh frame after seek, so I did not use it as proof of alpha equivalence. That program's RGB565 case failed inside the old DLL's open operation; I recorded it as a limitation of that test without attributing it to gameplay or requiring the adapter to reproduce the failure.

I refined that measurement with independent processes and seven render-time advances before capture. In all six X8/A8 cases with alpha 0, 128, and 255, I obtained identical memory pixels and alpha 255. I repeated the measurement with my own invisible D3D9 device: the old DLL returned an X8 texture, MANAGED pool, and alpha 255 in all six cases, with the same hashes as the memory frame. I used that result to align the adaptation. I did not derive a conclusion about the game's entire final composition from this test.

I also exercised an earlier adapter revision with all four AVI files, D3D9 texture upload and readback, pause, seek, frame leases, device recovery, and EOF, including one DirectSound case at volume zero. I obtained 165 passing checks in those five cases. I preserved that result as a historical step. These sampled and seek-based tests are distinct from the complete decoding of the four files recorded earlier and do not establish full real-time audiovisual playback.

I found and corrected two failures in my adapter implementation. Asynchronous opening was queued by both the facade and the SDK, allowing a following seek to be applied before opening had finished. I retained asynchronous admission in the facade and performed synchronous opening inside its worker to preserve command ordering. During independent review, I also found a frame that could return from a callback after Close or Stop and be published as current. I checked the generation before publishing that frame. The same oracle improved from 152 checks with two controlled failures to 152 without failures.

I also identified a device-recovery difference: the old client received MANAGED textures, while my initial candidate created DEFAULT textures. I confirmed in the code that the global dispatcher ignores release-callback return values; returning Busy would not prevent reset. I preserved the preceding candidate and corrected the implementation to retain the MANAGED pool observed in the old SDK. In the real proof using all four AVI files and one muted DirectSound case, I obtained 210 passing checks across five cases of 42 checks. I confirmed successful D3D9 reset with a frame still leased and useful readback from the same texture afterward.

I separately ran six format/alpha cases with 35 checks each, also totaling 210. I compared X8/A8 and alpha 0, 128, and 255 with the observed old-SDK behavior: X8 output with alpha 255. This group and the preceding MANAGED group test different contracts; I did not present them as two complete audiovisual playback runs or add their counts as a measure of feature coverage.

I extracted 13 actual consumer and global-manager methods for a native controlled-failure test using native SDK1 declarations and the integrated x64 wrapper, with controlled D3D9 interfaces. The corrected integration passed 292 checks; the preceding baseline executed 274 checks with 82 controlled failures. The 18 additional checks were conditional on safe paths in the corrected implementation. I obtained zero warnings in that corrected compilation and one C4100 warning in the baseline compilation. This test checks contracts, ordering, and references; it does not replace complete consumer execution in the game or tests using the real driver.

I reviewed the six existing files and nine new integration files, checked their hashes, and applied the revision only to the x64 tree. I retained the Win32 paths and legacy interface signature, isolating the new SDK in its own translation units. My new complete build uses that frozen set.

## 9 How I corrected generated-file destinations

I expanded source checking to detect extra files as well as changes to inventoried files. This barrier found five generated products in the candidate tree: a copied `fmod.dll` and four MIDL outputs. I preserved those five products and the stopped attempt before removing them from the source tree. They were absent from the historical 17,180-file inventory.

I traced MIDL in the logs and confirmed that the type-library destination was already private, but the other outputs' directory was unspecified. I configured `/out` to use the project's private intermediate directory while keeping MIDL generation enabled. I also corrected the FMOD copy-event selection: the event belonged to Client, while the previous selection addressed NtlSound only. I now stage the actual AMD64 DLL through the verified dependency contract without removing it from the product.

I preserved the old recipe and validated the new one with an isolated MIDL run and a complete NtlXMLLoader build. Both finished with zero warnings and errors without recreating those five source-tree files. I recorded that the existing C++ sources continue using the headers already selected by the project; I did not present MIDL generation as proof that those four new products were consumed by C++. The complete-build recipe now also checks for extra files in the frozen source tree.

## 10 My new complete build and verification of its results

I ran the new complete Release x64 build with the integrated source set, corrected output recipe, and at most two simultaneous compilers. I completed MSBuild in 19 minutes and 10.94 seconds with **zero errors and 573 warnings**. I obtained an AMD64 `Client.exe` of **14,668,288 bytes**, with SHA-256 `5b38017566a467534cc99f5f9eed20daa8f54eda2c66d5a9037c19951f5abe2b`. I checked all 17,189 files and dependency hashes again and staged the six AMD64 DLLs in the private build directory. I retained the installation in use unchanged.

I compared the warnings with the first build's 598 after normalizing paths and line numbers. I found 25 removed diagnostics and none added. I attributed 20 removals to the dump, MAC, entity-identity, callback, and cluster adjustments; the other five came from the old Windows-version path in the replaced video wrappers. I did not present 25 diagnostics as 25 distinct defects: one conversion can produce two warnings, and the same MAC read produced six. I confirmed that the four new video-bridge translation units produced no warnings. I kept the remaining debt of 573 warnings explicit.

During the first qualification of this new product, my verifier passed PE, import, and symbol checks but stopped at the `p2define.c` provenance chain. I had required the `rpworld.lib` archive from the isolated graphics rebuild, while the complete build also rebuilds that library through the Ntl_Plugin_World project and selects its new output. I preserved the attempt and investigated the compiler command, object, archive command, and MAP. I confirmed the actual chain from the corrected source to the selected library and corrected the verifier to check that chain, retaining the preceding proof as an unselected archive. I did not change the product to satisfy my verifier's incorrect expectation.

I reviewed and ran the corrected verifier, which completed static qualification successfully. I checked Client's 20 direct imports and immediate resolution of the seven local images, examining 1,528 symbol references. I confirmed the real `CreateBandiVideo2` export, the CNG and DirectSound imports, and the local PDB match with SymChk. I connected the 35 new or changed paths to the actual compilation and archive records, distinguishing selected libraries from preserved versions not used in this link. I did not treat these static results as game execution or stability testing.

I prepared the candidate installation under `<WORKSPACE>/client-local/64bits`, copying only this `Client.exe`, its six qualified AMD64 DLLs, and the `INICIAR_CLIENTE64_CANDIDATO.cmd` launcher. I checked the 623 data files before and after without replacing my profiles, settings, or resources. I removed the `CANDIDATE_NOT_READY.txt` block only after the checks and recorded `READY_FOR_HUMAN_TEST_NOT_STABLE`. I kept the PDB, MAP, and receipts private; I did not start the client or restart the server during that preparation. I use the AMD64 D3DX9 dependency already installed in Windows, which I checked separately from the I386 copy.

## 11 How I preserved results and attempts

I retained source inventories, frozen copies, forward/reverse patches, recipes, dependency contracts, previous objects and binaries, compiler logs, native proofs, hashes, controlled captures, and publication receipts. I retained failed attempts and the addenda correcting earlier conclusions. I will preserve dated, hashed snapshots of new report revisions to retain the process step by step.

I use `operacao/evidencias/fase28-*.json` for tracking receipts and `<PRIVATE_EVIDENCE_ROOT>/migracao64-pipeline`, `<PRIVATE_EVIDENCE_ROOT>/migracao64-dependencias`, `<PRIVATE_EVIDENCE_ROOT>/wire-abi-proof`, `<PRIVATE_EVIDENCE_ROOT>/video64-probe`, and `<PRIVATE_EVIDENCE_ROOT>/bandi-sdk21-adapter` for private technical artifacts. The hashes and indexes connect the reports to each attempt's actual files.

I also prepared a main project folder with versioned sources and two independent physical local clients excluded from Git. I checked 633 files in the Win32 copy and 623 data files in the x64 preparation, retaining the not-ready marker at that copy stage. With authorization, I removed only five old distribution ZIP files to free space and preserved their records. I detailed that organization in `ORGANIZACAO_LOCAL_E_GIT.md`.

## 12 What I still need to validate before publishing as stable

I completed static qualification and physical preparation of the candidate. I still need to perform my in-game test and record the applicable execution, image, and sound results. I keep the completed isolated-test results identified by revision and scope.

I will prepare the candidate in a separate installation for my in-game test. I will record my results for login, map and NPC selection, inventory and item sorting, coupons, pet, fuel, vehicle, combat, quests, mail, trade, audio, videos, and reconnection, according to the paths I exercise. I will keep pending tests and any limitations explicit.

I recorded authorization to publish the community reports **after satisfactory technical results and in-game tests**. I will prepare that publication in English and in first person, including problems, solutions, results, and caveats. My published local-fix report preceding the migration is [issue 137](https://github.com/OpenDBO/OpenDBO-Core/issues/137); the migration reports await the agreed testing condition.

I will publish the x64 version on `main` as stable only after that validation. I will then create the new branch for continued development while preserving the 32-bit folder as a historical record.
