# My technical records of the x64 migration

> I preserve this record's historical status statements in this full translation. Its V6-pending statements and later V7/staging addendum describe those stages; I record my subsequent limited local-play observation separately in issue #138. I do not retrospectively change an earlier pending state into a test result.

I consolidate in this document the designs, attempts, repairs and tests that I recorded in private files during the migration. I retain the earlier documents and receipts with their original bytes. I do not turn an earlier hypothesis into a measured result or attribute later tests to an earlier executable.

**I record this closing status on 19 October 2026:** I completed the full V6 build with zero errors and 573 warnings. I have not yet completed static qualification of that product: the first V6 analysis stopped because it assumed a separate `rpworld` library rather than the composition used by the actual build, and I am preparing revision V7 of that analysis. I do not classify the candidate as stable, installed, ready to launch or approved inside the game. I retain the Win32 client and Server19 in use.

I use the receipts' dates and UTC identifiers to order the stages below. I report the observations as my own and retain the technical names of classes, APIs, files and tools so that they can be checked. I do not include SDK verification arguments, credentials, vendor code or binaries in this text.

## How I defined the contracts before implementation

On 19 October 2026, I began with a private design, before compilation or integration. I preserved that state in `operacao/privado/bandi-sdk21-adapter/DESIGN.md`. I treated the SDK1 and SDK2 interfaces as different interfaces: I did not convert one vtable into the other, rename the new DLL as if it were the old one or invent a compatible export.

I planned a typed C++ factory for SDK1 consumers and a separate call to the real `CreateBandiVideo2` factory. I isolated the two SDK headers in different translation units because a namespace does not isolate macros. I retained absolute-path loading, DLL verification and the module reference until closing and releasing its objects and callbacks.

I specified that every `Verify` call should preserve its received arguments and return the actual HRESULT. I did not accept failure as success or use evaluation decoding as a substitute for verification. I also identified that I needed to check the result in `CMoviePlayer::ReadyMovie`, in addition to the path that already checked it in `CNtlMovie::Create`.

I planned one owner of decoder control, a bounded queue, owned arguments for asynchronous commands and a generation to invalidate old notifications and operations. I kept admission of an operation separate from its completion. I preserved the SDK1 source contract as a borrowed reference, without inventing `AddRef` or ownership of an object that is not IUnknown.

I recorded proposals in the first design that I later had to correct through measurement: I had planned to open asynchronously inside the SDK too and to use DEFAULT textures with SYSTEMMEM staging. I retain those proposals only as design history. I describe their later corrections at the corresponding stages without presenting them as the current policy.

## How I investigated the four videos without changing the game

On 19 October 2026, I read the four AVI files and the existing consumers. I identified that preserving video required more than reaching EOF: I needed to retain opening, playback, pause, seek, repeat, volume, texture, frame borrowing, notifications, composition and device recovery.

I first compiled an AMD64 proof using a real Windows AVI splitter, MPEG decoders, SampleGrabbers and NullRenderers. I kept the graph explicit, without installing a codec or accepting an arbitrary fallback. I preserved the four negotiation failures `0x80040207` in `operacao/privado/video64-probe/run-20261019T161512Z-2cb93f9c`. I did not obtain EOF, video hashes or functional equivalence from that attempt.

I prepared a second variant with a descriptor built from each video's actual header and required acceptance on both sides of the connection. I obtained acceptance from the decoder but rejection with `S_FALSE` from the splitter. I preserved `run-v2-20261019T161927Z-4f456865` and did not force a contradictory type or register codecs in the system.

I then examined the origin, signature and declarations of SDK 2.1.5.19. I kept DLL authenticity, API compatibility and redistribution permission separate. I used only the pinned DLL in the authorized experiments; I did not run vendor installers or ready-made examples.

I began the SDK2 proof without `Verify`, with instrumented silent audio and caller-owned R8G8B8 video memory. I reached the expected indices and EOF in all four files, but later found completely zeroed buffers. I preserved the initial receipt and wrote a corrective addendum: I could not approve useful images from frame counts and EOF alone.

I added nonzero PCM counters and three image observations per AVI in a separate variant. I obtained twelve black PNGs and confirmed the repeated hashes of zeroed frames. I observed nonzero audio samples, but did not use the amount of PCM pulled as proof of the original duration or synchronization: my silent drain could include padding or silence after the end.

I then tested the real verification call, keeping the existing arguments only in memory and anonymous stdin. I preserved a compile failure in the proof itself, caused by the missing `<string>` include, and its correction in a new file. I obtained `E_FAIL (0x80004005)` for empty and invalid controls and `S_OK` for the unchanged product call. I recorded only the results, without saving arguments, values or their hashes. I did not interpret `S_OK` as legal certification of licensing or redistribution rights.

I repeated the R8G8B8 proof after real verification and still obtained zeroed pixels. I preserved the failure without automatically attributing it to licensing or a missing codec. I then changed only the output contract to X8R8G8B8, with four bytes per pixel, a corresponding pitch and exclusion of the X byte from the RGB counters. I retained `can_skip=FALSE`, the same AVI files and the same DLL.

| File I decoded in X8 | Frames I observed through EOF | Frames where I measured nonzero RGB |
| --- | ---: | ---: |
| `logo.avi` | 550 | 548 |
| `opening.avi` | 4,580 | 4,580 |
| `Age1000.avi` | 4,585 | 4,585 |
| `The_base_of_Tenkaichi.avi` | 7,042 | 6,987 |

I checked the indices, monotonic times and per-frame hashes. I opened the twelve beginning, middle and ending PNGs and observed coherent images and transitions. I did not visually classify every one of the 57 zero-RGB frames in the table or remove a watermark. I preserved the qualification in `operacao/privado/video64-probe/sdk2-x8-native-observation-qualification-v1.json`. I did not derive physical audio playback, complete audiovisual synchronization or equivalence of the in-game consumers from this test.

## How I implemented and corrected the adapter's audio

On 19 October 2026, I implemented DirectSound and a silent mode in the same `IBandiVideoSound` object, allowing mode changes only while closed and preserving the already-verified SDK2 object. I kept SDK1 out of that translation unit. I returned an owned reference from the factory, without opening a device or starting a thread during its creation.

I used the vendor's audio examples as private dependencies, retaining attribution and license notices. I implemented a DirectSound PCM ring with the caller's HWND, `DSSCL_NORMAL`, logarithmic volume from 0 to 255, a clock based on the played cursor, pause, restart, buffer restoration, and event/thread shutdown. I implemented silent mode with real QPC, subtracting pauses and draining only the frames due according to the clock. I did not use the accelerated decoding-proof clock as the production clock or replace volume with `E_NOTIMPL`.

I retained callback references during the session, invoked callbacks outside locks and preserved independent state to allow Close or the last Release during a callback without self-join. I retained the shutdown order: I close and release the SDK before releasing the sound callback and module. I did not claim that I can forcibly terminate an external callback that never returns.

I preserved the first proof and its intermediate results of 867 checks. I later found that my first candidate could continue playing old PCM when the decoder returned zero samples. I preserved the earlier bytes, stopped the buffer on underflow, retained its clock and repositioned the next write. I applied the same stronger oracle to the preceding baseline and obtained two safe failures; with the correction I obtained 879 passing checks.

I later found PCM loss when Pause occurred during `GetPcmData`: I had already consumed a block but could discard it when changing the generation. I prepared V2 separately and retained that PCM in session storage while paused, delivering it before pulling another block. I retained Stop, Start and Close as changes to the stream identity.

I also reproduced two concurrent Opens that could replace the published session and leave the displaced worker without its owner. I added atomic admission before closing or waiting for the previous owner, rejected concurrent or reentrant Open as Busy and used a ticket to prevent publication after Close during initialization. I kept Join outside lifecycle locks and preserved the SDK2 object and its verification.

I compared V1 and V2 using the same 901-check oracle: I obtained seven safe failures and 894 passes in V1, against 901 passes in V2. I compiled the production object and proof with `/W4 /WX`, without warnings or errors. I used real threads, events, locks, UCRT and SDK2/Windows declarations, but kept the DirectSound endpoints, owner and QPC as explicit simulations. I did not use that proof as a physical-audio or synchronization test.

I also preserved an artifact error: my 7,229-byte V2 patch did not end with LF on the last context line. I retained that patch and issued V2b at 7,230 bytes, correcting only the final LF. I checked application and reversal byte for byte without changing the CPP or the 901 tests. I retained the old revisions before activating V2 as a private input to the adapter.

## How I measured the old alpha and time behavior

On 19 October 2026, I ran my own Win32 program against the existing SDK1 DLL, with a hidden HWND, NULL audio and unchanged verification arguments only in memory/stdin. I preserved the first attempt, including an RGB565 failure inside Open. I did not repeat that already-failed case or treat it as an obligation to reproduce a crash in the adapter.

I observed finite negative values in time conversions and truncation of frame counts toward zero. I moved version reading until after Create and retained the actual zero result and negative Close HRESULTs. I retained the existing deprecation warning in the old header without changing that SDK.

I prepared a second alpha measurement by draining leases and checking `CurrentRenderTime` after Seek. I still found an inconsistent result: some repeated seeks returned black even at alpha 255. I preserved V2 and did not use zero exit codes as proof of an alpha rule.

I then prepared V3 with an independent process and Open for every X8/A8 combination with alpha 0/128/255. I waited for at least three distinct render advances and at least 9.2 seconds without filtering frames by color. I observed seven advances and `9.199999999999978` in all six cases. I obtained identical memory pixels, 45,155 nonzero RGB bytes and alpha 255 in all six cases.

I prepared V4 in independent Win32 processes with an owned hidden HAL D3D9 device, still without physical audio or the game. I requested X8 and A8 with the three alpha values and observed the actual texture description: I received X8R8G8B8, MANAGED pool, usage zero and 960 by 540 in every case, even when requesting A8. I copied color and alpha from the texture into an owned render target with `SELECTARG(TEXTURE)`, without blending or additional conversions, and read the pixels with `GetRenderTargetData`.

I obtained the same RGB/BGRA hashes as the memory measurement, alpha 255 and `S_OK` from SetAlpha and readback in all six cases. I preserved the `0x85861027` return from Close after Stop. I limited the conclusion to the measured frame, format and values: I did not assert a universal rule for screen composition, every frame, every format or audiovisual synchronization.

I recorded the D3D9 measurement in `operacao/privado/video64-probe/sdk1-d3d9-alpha-v4-qualification.json`, SHA-256 `29fcdb4e01db68977f787082171cc84396682093d48b7ebdefa39f2694d44d9b`.

## How I corrected the queue and frame publication

On 19 October 2026, I compiled the first bridge translation units for AMD64 and retained the result as compilation without promoting it to execution. I encountered LNK1101 during the first link of a proof; I preserved the attempt and a refused process-stop request whose identity no longer matched. I repeated the same proof in the authorized execution context and obtained an executable. I did not treat that repetition as isolated proof of the cause of LNK1101.

I implemented all 30 virtual methods of the SDK1 interface, the file reader, borrowed source, bounded queue, frame memory, conversions, leases and notifications. I retained the separation between the SDK1 header, neutral contract and SDK2 backend. I preserved the first facade results and extended coverage without attributing the new cases to the earlier proofs.

I found an actual failure when opening asynchronously in both the facade queue and the SDK. I observed initialization, verification, texture, pause and reset, but Close followed by OpenAsync, SeekAsync and PlayAsync did not reach the expected EOF. I checked that the backend's synchronous sequence reached the last frames. I kept OpenAsync nonblocking for the caller, but changed to synchronous Open inside the already-asynchronous worker. I preserved the failed attempt and the later result of the same path.

I also found that a frame could return from a callback after StopAsync or CloseAsync and be published as current. I began checking the generation after decode and before publication and the new lease. I preserved leases already returned to the consumer. I compared the baseline and V2 using the same facade oracle: I obtained two safe failures in 152 baseline checks and none in V2's 152.

I used the actual class, SDK1 header, GDI, threads, futures and PostMessage in that facade, but a simulated neutral decoder, including the reentrant callback. I kept a separate 29-check proof of the reader, policy and file hash, with extracted bodies and real file/CNG APIs. I did not present these proofs as SDK decoding or execution of the game.

I later aligned the output formats with what I measured in SDK1: I retained X8/alpha 255 for both 32-bit-format requests, without inventing a pixel effect that I had not observed. I validated and retained the requested SetAlpha value and preserved the distinction between requested format and the texture actually exposed. I obtained one failure in the preceding A8 case under the new format oracle and 210 passing checks in six V2 cases of 35, using real SDK2 and D3D9.

## How I replaced the historical DEFAULT pool with the current MANAGED pool

On 19 October 2026, my first implementation still used a DEFAULT destination texture fed through SYSTEMMEM staging and `UpdateTexture`. I described that state in the preserved `IMPLEMENTATION.md`. I do not present it as the current behavior.

I measured that SDK1 returned MANAGED/usage zero and confirmed that the global callback manager ignores callback return values. I concluded that returning Busy would not prove that reset would be prevented while a frame was borrowed. I preserved the DEFAULT implementation and prepared V3 separately.

I changed to uploading directly through `LockRect` on the MANAGED texture, retained suspension of new borrowing during reset and preserved leases already issued. I retained the memory context's reset behavior and preserved the original `CMoviePlayer::ReleaseHandler` as deferred-release marking; I did not replace it with synchronous Close.

I obtained two safe failures from V2 DEFAULT under the same new oracle. I obtained 210 passing V3 checks in five cases of 42: four AVI files with NULL audio and one logo case with real DirectSound at volume zero. I retained a lease during an actual D3D9 Reset on an owned device, read the same useful texture after reset and returned the lease. I also retained the 152 facade checks as a separate regression.

I did not add that group of 210 to the format/alpha group of 210 as a measure of total coverage. I preserved the earlier 165 real tests as a historical V1 stage with four AVI files and one silent DirectSound case. I did not reclassify them as V3 tests, complete real-time playback, audible audio or approval inside the game.

## How I tested the consumers and their error returns

On 19 October 2026, I prepared integration in six existing files and nine new ones, retained the originals and checked the Win32 paths. I compiled four bridge translation units separately and extracted 13 actual consumer and global-manager methods for a native controlled-failure proof.

I compiled the native SDK1 declarations and integrated x64 wrapper with the bodies of `CNtlMovie`, `CMoviePlayer` and `CNtlDeviceRestoreHandler`. I kept D3D9 interfaces as declared simulations of HRESULTs, references and state, without pretending to establish a COM ABI or GPU execution. I used invented verification values only in controlled fixture branches, without accessing credentials or the commercial DLL.

I preserved the first compile failure in my test adaptation: I had expanded a pointer typedef in a way that changed a multiple-variable declaration and needed to make the actual `void**` overload compatible with the simulated texture. I corrected only the proof's adapter without modifying the extracted product bodies. I retained intermediate current results of 270 and 272 checks before adding manager and three-state-restoration coverage.

I reproduced a safe baseline failure using counters and quarantine: after GetSurfaceLevel failed, I detected the frame lease and the reference obtained from the old render target without their return. I checked that the revision returns the frame exactly once on error paths and balances the GetRenderTarget, GetTexture and GetSurfaceLevel references. I checked that all three restorations—render target, texture and viewport—are attempted even when one fails.

I tested reuse of the owned render target, Delete, device callbacks, Busy release, later cleanup and modeled reentrancy during Draw/Present. I checked that the consumer retains the identity needed until returning the lease and that the native void-returning manager does not turn Busy into a reset veto. I kept the real MANAGED/Reset proof in its separate receipt.

I obtained 292 passing current checks. I executed 274 checks on the baseline and obtained 82 safe failures; I did not execute paths there that would dereference null or already-cleared pointers. I added 18 checks only on safe paths of the corrected revision, without calling them 18 complete new cases.

I also preserved an error in my first result qualifier: I had required zero warnings from both compilations, but the baseline had a C4100 for the legacy `lParam` parameter. I corrected only the expectation and retained the baseline with one warning and the revision with zero warnings, both without compilation errors. I did not change or rerun the binaries for that metadata correction.

I kept the limits explicit: I simulated the origin and validation of the EOF token, callback-list hosting, geometry, global objects and D3D9 setters. I did not certify the application's complete event order, screen composition, driver-held references or tolerance of arbitrary destruction during a callback. I used the receipt `operacao/evidencias/fase28-video21-consumers-validacao.json` to bind those results to the exact sources.

## How I preserved the two physical data copies

On 19 October 2026, I separately inventoried 131 regular files in `cliente` and five permitted junctions: cursor, font, movie, pack and ts, resolved only to their corresponding destinations in `OpenDBO.Client`. I did not follow links generically. I found no empty directories requiring separate inclusion.

I prepared a bounded plan, checked the policies and made both physical copies with 4-MiB buffers and exclusive creation. I checked source SHA before and after, destination SHA and dates, `nlink=1` and distinct identities between corresponding files. I did not use hardlinks, destination junctions, deletion, moving or overwriting of originals.

I preserved 633 files in the complete Win32 copy, with 10,488,136,644 logical bytes and the original I386/PE32 Client26. I preserved 623 data files in the x64 copy, with 10,467,815,114 logical bytes, plus the `CANDIDATE_NOT_READY.txt` marker. I excluded the Win32 EXE, eight DLLs and launcher from that x64 copy. I retained existing configurations and profiles as private data without publishing their values.

I treated the result as proof of copy integrity without opening the game or asserting x64 functionality. I keep that stage's marker and absence of executable products separate from future qualified staging. I do not use the first historical x64 EXE as a substitute for a final product.

## How I integrated the sources and corrected the build recipe

On 19 October 2026, I applied the video revision only to the x64 copy. I checked that stage's final set: 35 paths different from the baseline, with 26 existing files changed and nine new files, in a tree of 17,189 files. I preserved the historical 17,180 files of the 32-bit tree. I did not attribute those changes to the first x64 EXE, which retains its own historical revision.

I expanded the freeze to reject extra files and found a copy of `fmod.dll` and four MIDL products in the source tree. I preserved the five products and the attempt before removing them from the candidate tree. I did not delete or rewrite a file belonging to the historical inventory to conceal that occurrence.

I corrected the private recipe to use `/out` in the MIDL intermediate directory and retained active generation. I also corrected the selection of Client's PostBuildEvent that copied FMOD into source. I retained explicit staging of the AMD64 DLL through the dependency contract without removing audio from the product.

I preserved the old recipe and ran an isolated MIDL proof and a complete NtlXMLLoader build. I obtained zero errors and zero warnings in both, without recreating the five files in source. I checked that the generated directory was available to the compiler, but did not claim that C++ read the four new products: my tracker did not show that consumption.

I then completed the full V6 build with zero errors and 573 warnings. I distinguish that result from the static analysis that I still need to complete. I preserved the V6 analysis that stopped because of its `rpworld` expectation and am preparing revision V7 to check the actual composition without retroactively changing the previous attempt. I do not yet assert PE, symbols, closure or final readiness of this new product merely from the build's zero exit code.

## How I separate what I tested from what is still outstanding

| Scope I recorded | Result I retain | Limit I preserve |
| --- | --- | --- |
| I negotiated two explicit DirectShow graphs. | I preserved connection and type-acceptance failures. | I did not obtain qualified decoding through that path. |
| I decoded the four AVI files in X8 through SDK2. | I checked frames, EOF, hashes, nonzero RGB and twelve images. | I did not derive physical audio, AV sync or complete pixel equivalence between decoders. |
| I tested SoundProxy V2. | I obtained 901 passes against seven safe V1 failures under the same oracle. | I kept controlled device and clock endpoints in the proof. |
| I tested facade V2 and the reader. | I obtained 152 passes against two safe baseline failures and kept the 29 reader checks separate. | I used a simulated decoder in the facade; I did not count that proof as real SDK or game execution. |
| I measured six SDK1 combinations and aligned format/alpha in V2. | I obtained six real cases of 35 checks, totaling 210, in addition to the pinned earlier measurements. | I did not generalize the measured frame to all screen composition. |
| I corrected the pool policy in V3. | I obtained five real cases of 42 checks, totaling 210, and two safe V2 failures. | I did not add that group to the alpha group or the historical 165 as overall coverage. |
| I tested the integrated consumers with controlled failures. | I obtained 292 current passes and 82 safe failures in 274 baseline checks. | I kept D3D9 interfaces and parts of the host explicitly simulated. |
| I copied the local clients. | I checked 633 Win32 files and 623 x64 data files plus the marker. | I did not execute the game as part of that copy proof. |
| I compiled the V6 product. | I obtained zero errors and 573 warnings. | I still need to complete static qualification and game testing. |

I retain the manual plan in `operacao/privado/fase28-documentacao/revisao-assets-audit-v6-20261019/roteiro-testes-jogo-x64-v2-primeira-pessoa.md`, with its ten stages marked NOT EXECUTED. I will record passes only for paths that I actually exercise, with the observed revision, duration and repetitions. I do not infer PASS from a build, silence in a log or a folder name.

I keep community publication of the migration conditional on satisfactory technical results and the agreed in-game tests. I do not publish through this document. I keep technical feasibility, module authenticity and licensing/redistribution conditions separate.

## How I bind this narrative to the preserved proofs

I checked the following private source documents before this consolidation. I retain their historical states and bytes; I use this new text as the current narrative without replacing those files.

| Document I preserved | SHA-256 I checked |
| --- | --- |
| `operacao/privado/bandi-sdk21-adapter/DESIGN.md` | `2eff1f6f7981c7f41c95c678780619c6a99c0ab782b04c82ae8ae5cc4f9e3831` |
| `operacao/privado/bandi-sdk21-adapter/IMPLEMENTATION.md` | `65db0a1e0d978727dd1c75b384d9491be7fb58612fb0c3c213ca6c8be00ce480` |
| `operacao/privado/bandi-sdk21-adapter/HISTORY.md` | `53f4be7cae01e20b2688a349dfaf2c6f8919f3ecaf95193d3bacefc0598e0e6b` |
| `operacao/privado/bandi-sdk21-adapter/sound/HISTORY.md` | `c3db6b7e150c63913d62c49dde2aa4a30c32c92fae048634a6332212dff421d7` |
| `operacao/privado/bandi-sdk21-adapter/sound/v2/HISTORY.md` | `1ca921dcdd20dfb2c69a9e5b299a9d7404ddeaf38cb00c3a8af9cfdefe26e27b` |
| `operacao/privado/bandi-sdk21-adapter/pool-v3/HISTORY.md` | `79aeaad2100e64d9a94b2c45b5eb968f62ea92a02b617cb2374434e92d2fce51` |
| `operacao/privado/bandi-sdk21-adapter/core-integration/tests/HISTORY_CONSUMERS_V1.md` | `ebca5fcac09da5cbee7933ac87933ab8e6e223784a8d27b191ba20362ded4199` |
| `operacao/privado/fase28-client-local/COPY_HISTORY_V1.md` | `7da5f2772d386ac130efdb718241317886646e9e350e179eefe246a299eb1bde` |
| `operacao/privado/video64-probe/HISTORY.md` | `555c3c64e04bc94137b2fa00b981cc5f4a685e1ea9fcbf7f17d76d1289375850` |
| `operacao/privado/video64-probe/SDK1_FRESH_OBSERVATIONS.md` | `2db98b910f7a97de9ef34b6c37ef9419e795f9f6dbb0b6963f7342ee3fc581a4` |
| `operacao/privado/video64-probe/SDK1_FRESH_V3_FOLLOWUP.md` | `833dcc857d055fb30499c8a2051cc0ab183c3fc4ecb48feae88e71748502134d` |
| `operacao/privado/video64-probe/SDK1_D3D9_V4_HISTORY.md` | `e59cd1721ff4397dd70be13e5dcb7f654cf77d95f42d8f66a590b485e5f8b235` |

I bind the main final results to the receipts below. I retain their state labels as belonging to each receipt's date: I do not rewrite “not applied” in an old experiment to make it appear that it already described the later application.

| Proof I reference | SHA-256 I checked |
| --- | --- |
| `operacao/privado/bandi-sdk21-adapter/sound/v2/sound-candidate-v2.json` | `5a45c4ca6f2c3eef68659f739e7ef2efe64b4472d9fe013c92ead8f3189090f2` |
| `operacao/evidencias/fase28-sound-sdk2-peer-network-v2.json` | `a11f09210fe6f87856b9ed7731f99513238bbb7bbd190b05ab4a1d6a0605828b` |
| `operacao/privado/bandi-sdk21-adapter/tests/real-sdk-20261019T173256Z-72403ad1/receipt.json` | `8074973be0944128ea58a28c0861ca759122369baac355f392402770dd80b73e` |
| `operacao/privado/bandi-sdk21-adapter/pool-v3/tests/real-sdk-20261019T175229Z-7a0d3e84/receipt.json` | `4d70d8576572cf9d2db6fffa3a1962667ff877f5d4387e2f26d22d07b70e24af` |
| `operacao/privado/bandi-sdk21-adapter/core-integration/candidate-v3-20261019T175413Z-cfb8631e/candidate.json` | `191e80527d1f8f584bb9e5a174847d008d965edf4f52b1ecef2f2510a6f1ed78` |
| `operacao/privado/bandi-sdk21-adapter/core-integration/tests/native-20261019T175846Z-4eaf6e6e/receipt.json` | `1cf4374de0c23d80974e178ca5ecac3be23ec2dc96f5f3071c950670d78e5354` |
| `operacao/evidencias/fase28-video21-consumers-validacao.json` | `086acb6be44651df0e84c386dc75c53bb03952658d367e5d9fb8eca0b66c295b` |
| `operacao/evidencias/fase28-client-local-copias-concluidas-20261019T174730Z.json` | `4dc219543e0fa4c301703d485c9e9629482cfa7eb70e20e604b8b0d2fe3098f7` |
| `operacao/privado/migracao64-pipeline/Client64.Output.v2.props` | `5297fcdc8eee82140ea20b0b315ed969d08fd70dc8c36b244b728ddff14ff290` |

I retain only references and hashes at these paths in this public text edition. I do not attach the private contents, personal configurations, SDKs or third-party binaries. I preserve the other reports, README and recorder outside the scope of this new edition.

## My addendum after static qualification and physical preparation

I later completed V7 static qualification of the same AMD64 executable without recompiling or changing source to work around the V6 verifier failure. I confirmed 20 direct imports, seven local images, 1,528 examined symbol references, a matching PDB and the provenance of the 35 changed or added paths. I preserved the receipt in `operacao/privado/migracao64-pipeline/qualifications/video-v7-20261019-155204-bc2115f2/qualification.json`, with SHA-256 `25f7e51c6136a0a07e001ca6efbf9cf095ebd3675cce6ce434b3f608b8d1cea3`.

I prepared the separate physical copy with the qualified executable and six AMD64 DLLs and its own launcher. I checked the 623 data files before and after copying: both aggregate inventories have SHA-256 `b59a1f80650e39e5926ab2869bb674066134ba3ecc37fdb50ddcc188a9131d44`. I preserved the receipt in `operacao/privado/migracao64-bootstrap/staging-history/20261019T185621Z-7ecc291b/receipt.json`, SHA-256 `424a745741adcbefca975d41fb2141b9ccc1ff651a55d0eeefdfdc3f6201eeff`.

I recorded the copy as `READY_FOR_HUMAN_TEST_NOT_STABLE`. I have not yet started the x64 game or completed my product gameplay, image or audio test. I keep stable publication to `main`, the new development branch and posting the community reports conditional on the agreed validation.
