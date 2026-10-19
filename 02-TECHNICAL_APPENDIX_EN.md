# My technical appendix to the OpenDBO x64 migration report

I consolidate my detailed technical records and the additional local-organization records here in English. I preserve failed attempts, dated revisions and hashes; I do not assign later tests to earlier executables.

I completed the integrated AMD64 build, V7 static qualification, canonical source copy and separate candidate staging. I then played for a few hours on my own local server without other observed errors. Excessive repeated Alt+Tab can still freeze the client, as it already could on Win32. I have not established its cause or a fix. This limited observation does not certify all features, hardware, audiovisual synchronization or long-term stability.

## How I read my revision history

I performed this work on 19 October 2026, distinguishing attempts by retained UTC identifiers and hashes. An early record saying preparation, not applied or not executed remains historical; I add later results instead of changing its meaning.

I started from upstream `ed30bbe393653c704130aa026baf2bcad02c31ff` and my accumulated fixes. I preserved 17,180 baseline files and all 191 canonical correction files. My first executable used a 13-source-change freeze; the next checkpoint had 20 changed files. Integration produced 35 changed or added paths: 26 existing files changed and nine added, in 17,189 files. I preserve inherited attribution and notices rather than claiming authorship of upstream or vendor work.

## How I defined the video contracts before implementation

I began with a design, not a compiled integration. I treated SDK1 and SDK2 as different interfaces: no vtable reinterpretation, renamed DLL or invented compatible export. I used a typed C++ factory for legacy consumers and a separate call to the real `CreateBandiVideo2` factory.

I separated the legacy header, neutral contract and SDK2 declarations into translation units: namespaces do not isolate macros. I required an absolute module path, pinned hash and module ownership through decoder/callback teardown, with idempotent same-policy configuration. I added no arbitrary DLL-search fallback, fake version, verification success or feature-hiding placeholder.

I preserve Verify arguments and return the actual HRESULT. I added checked failure cleanup to `CMoviePlayer::ReadyMovie`, complementing the checked `CNtlMovie::Create` path. I did not substitute evaluation decoding for verification.

I planned one decoder-control owner, bounded commands with owned asynchronous arguments and a generation to invalidate obsolete work. Admission is not completion. I retained the non-IUnknown SDK1 source as a borrowed reference without invented AddRef/ownership. My original proposals for SDK-internal asynchronous Open and DEFAULT/SYSTEMMEM textures remain historical; measurement made me change them as described below.

## How I investigated the four installed videos

I inspected all four AVI files and both consumers, treating video as opening, playing, Pause/Seek/repeat, volume, frame/texture leases, EOF, composition and device recovery rather than final-frame count alone.

I first compiled an explicit AMD64 graph with real Windows AVI splitter, MPEG decoder, SampleGrabber and NullRenderer. All four negotiations failed with `0x80040207`. A second descriptor built from each actual MPEG sequence header was accepted by the decoder but rejected with `S_FALSE` by the splitter. I preserved both attempts without forcing contradictory types, installing codecs or claiming decoded images/EOF from failed graphs.

I then examined SDK 2.1.5.19 declarations, downloaded-file hashes and signature, separating binary authenticity, API compatibility and redistribution permission. I used the pinned DLL in isolated proofs, not vendor installers or prebuilt demos.

I initially reached expected indices/EOF without Verify using caller-owned R8G8B8 memory and instrumented silent audio, but found entirely zero pixels. I preserved the initial receipt and corrected its interpretation. Twelve beginning/middle/end PNGs were black and repeated zero-frame hashes confirmed it. Nonzero PCM did not prove exact duration or AV sync: the accelerated drain could include padding/silence after EOF.

I separately tested real verification, preserving a missing-`<string>` compile failure and its corrected test revision. Empty/invalid controls returned `E_FAIL (0x80004005)`; unchanged existing product arguments returned `S_OK`. I passed them only in memory/anonymous stdin and recorded no values, copies or derived hashes. S_OK is not legal certification of entitlement or redistribution.

I repeated R8G8B8 after verification and still got zeros, without automatically blaming licensing or a codec. I changed only the output contract to X8R8G8B8, four-byte pixels and matching pitch, keeping the AVI files, DLL and `can_skip=FALSE`. I excluded X from RGB counters.

| File I decoded in X8 | Frames I observed through EOF | Frames where I measured nonzero RGB |
| --- | ---: | ---: |
| `logo.avi` | 550 | 548 |
| `opening.avi` | 4,580 | 4,580 |
| `Age1000.avi` | 4,585 | 4,585 |
| `The_base_of_Tenkaichi.avi` | 7,042 | 6,987 |

I checked contiguous indices, monotonic timestamps and per-frame hashes and inspected twelve coherent PNGs/transitions. I did not classify all 57 zero-RGB frames individually or remove a watermark. This does not establish physical audio, full AV sync or cross-decoder pixel equivalence inside the game.

## How I implemented and repaired adapter audio

I implemented DirectSound and silent operation in one `IBandiVideoSound`, allowing mode changes only while closed to preserve the verified SDK2 object. SDK1 stays outside that TU. My factory returns one owned reference without opening a device or starting a thread.

I retained attribution/notices for private vendor DS/NULL references. I implemented a secondary PCM ring with caller HWND, `DSSCL_NORMAL`, logarithmic volume 0..255, played-cursor clock, Pause/resume, Stop/Start reset, shadow Restore and event/thread cleanup without changing the primary format. Silent mode uses QPC minus pauses and bounded pulls of only due sample frames; volume is validated/retained, not E_NOTIMPL. I did not use accelerated test-byte consumption as production time.

I retain session callback references, invoke outside locks and keep independent session state for callback-driven Close/final Release without self-join or UAF. I close/release the SDK before sound callback/module teardown. I cannot safely force-terminate a callback that never returns.

I preserved 867 early checks. I then found stale ring PCM could continue playing after zero returned samples. I preserved source/tests, stopped on underflow, preserved the clock and reanchored fresh writes. The stronger common 879-check oracle exposed two safe earlier-candidate failures; the repair passed all 879.

I later found Pause during GetPcmData could discard an already-consumed chunk when changing generation. V2 retains pending PCM while paused and delivers it before another pull; Stop/Start/Close still change stream identity. Concurrent Opens could also replace the published session and orphan its worker. I added atomic admission, Busy for concurrent/reentrant Open, and a ticket preventing publication after Close during initialization, keeping Join outside lifecycle locks and verification intact.

I compared V1/V2 with the same 901-check oracle: V1 passed 894 with seven safe failures, V2 passed 901. Production object and fixture compiled `/W4 /WX`, zero warnings/errors. Threads, events, locks, UCRT and SDK2/Windows declarations are real; DirectSound endpoints, owner and QPC inputs are controlled, not physical audio/AV-sync qualification.

I preserved a 7,229-byte V2 patch missing final-context LF and issued V2b at 7,230 bytes, changing only LF. Raw forward/reverse application matched; CPP and 901 results were unchanged, not rerun. I preserved revisions before activating V2.

## How I measured legacy time and alpha behavior

I used an independent Win32 SDK1 harness with owned hidden HWND, NULL audio and unchanged verification arguments only in memory/stdin. I preserved an RGB565 failure inside Open without repeating the crash, attributing it to gameplay or requiring the adapter to imitate it.

I measured finite negative time conversion and frame truncation toward zero. At the exercised 30 fps, FramesToTime(-2/-1) was approximately -0.0666667/-0.0333333; TimeToFrames(-1/0.05/1.05) returned -30/1/31. I did not clamp those finite negatives. A before-Create version result was not valid initialization evidence; after Create I observed zero. I retained negative Close results, including `0x85861027` after Stop, and the existing header deprecation warning rather than inventing versions or unconditional success.

I preserved an inconclusive V2 alpha experiment: draining leases/checking CurrentRenderTime after Seek still allowed repeated black output even at alpha255. Zero process exits did not establish fresh-frame alpha equivalence.

I then used independent Open/processes for X8/A8 requested formats times alpha0/128/255, waiting for at least three distinct advances and 9.2s without selecting by color. All six actually reached seven advances and `9.199999999999978`: identical memory pixels, 45,155 nonzero RGB bytes, alpha255.

I repeated all six with an owned hidden HAL D3D9 device. Every returned texture was X8R8G8B8, MANAGED, usage0, 960x540 even when requesting A8. I copied through SELECTARG(TEXTURE), no blending, into an owned A8 target and used GetRenderTargetData. RGB/BGRA hashes matched memory, alpha255 and SetAlpha/readback S_OK. I limit the conclusion to the measured frame/formats/values, not every frame, format, final screen composition or audible synchronization.

## How I repaired ordering and frame publication

I preserved initial AMD64 TU compilation as compilation only. The first proof link failed LNK1101; I retained it and a refused process-stop request whose identity no longer matched. Repeating in the authorized context linked successfully, without independently establishing why LNK1101 occurred.

I implemented all 30 legacy methods, file/borrowed-source readers, bounded queue, conversions, frame memory/leases and notifications across the separated headers/backend. I retained earlier facade results, including 144 checks, without assigning later cases to them.

I found double-asynchronous Open broke ordering: initialization, verification, texture, Pause/Reset could work, but Close→OpenAsync→SeekAsync→PlayAsync missed expected EOF while the synchronous backend reached final frames. I retained async caller admission but made SDK Open synchronous inside its already-asynchronous worker, preserving the failure and later result.

I also found a source callback could admit StopAsync/CloseAsync during decode and allow a cancelled frame to be republished. I recheck generation after decode, before CPU publication and before a new lease; already-issued leases stay valid until returned. The same facade oracle improved from 152 checks/two safe failures to 152/no failures.

I used the real class/SDK1 declarations, GDI, threads, futures and PostMessage, but a simulated neutral decoder/reentrant callback. I retained 29 separate extracted-body reader/policy/hash checks with real file/CNG APIs, not real-SDK decoding or gameplay.

I aligned both 32-bit-format requests to measured X8/alpha255 output, validating/storing SetAlpha without an invented pixel effect and retaining requested-versus-returned format. An earlier A8 case exposed the mismatch; six corrected real SDK2/D3D9 cases of 35 checks passed: 210.

## How I replaced the historical DEFAULT texture policy

I originally implemented a DEFAULT destination texture with a SYSTEMMEM upload texture and `UpdateTexture`. I retain that implementation and its design notes as history. I do not describe DEFAULT as the final pool policy.

I measured MANAGED/usage-zero textures from the old SDK and checked the global device-release dispatcher. Its `ReleaseHandler` returns void and ignores each callback's result. A consumer returning Busy therefore does not prove that a global Reset will be vetoed while a frame is leased. I retained the DEFAULT implementation before preparing V3.

I changed upload to direct `LockRect` on a MANAGED texture. I suspend new borrowing during reset and preserve outstanding leases. I retained the MEMORYBUFFER context's existing reset policy and preserved the original `CMoviePlayer::ReleaseHandler` deferred-release marking instead of replacing it with synchronous Close.

I exercised an actual D3D9 Reset with a live lease in an owned device, read useful pixels from that same texture afterward and then returned the lease. My V3 proof passed five cases of 42 checks, totaling 210: the four AVI files with NULL audio and one logo case with real DirectSound at volume zero. The preceding DEFAULT variant had two safe failures in the new pool oracle. Its early conditional path executed 39 checks, so I do not present that smaller baseline count as an equal number of complete cases.

I also retained the 152 facade checks as a separate regression suite. I retained an earlier real-adapter result of 165 checks across four AVI cases and a muted DirectSound case as V1 history. Those sampled/seek-based experiments exercised upload/readback, Pause, Seek, leases, recovery and EOF; they were not complete real-time audiovisual playbacks.

I keep the two totals of 210 distinct. One means six format/alpha cases of 35 checks; the other means five MANAGED/Reset cases of 42 checks. Neither is a second full decode of all four soundtracks, and their sum is not a measure of total feature coverage.

## How I checked the consumers and their failure paths

I integrated six existing files and nine new files, retained originals/Win32-selected paths, compiled four bridge TUs and extracted 13 actual consumer/global-manager methods for native fault testing.

I used SDK1 declarations and the integrated wrapper with CNtlMovie, CMoviePlayer and CNtlDeviceRestoreHandler bodies, but controlled D3D9 HRESULT/reference/state endpoints rather than a measured COM ABI or GPU. Invented Verify values stayed in controlled fixture branches without reading credentials or loading the commercial DLL.

I preserved the initial fixture compile failure: expanding a pointer typedef changed a multiple-variable declaration, and the real void** overload required a bridge to the fake texture. I fixed only test type adaptation, retaining product bodies and intermediate 270/272-check results before global-manager/restoration coverage.

I safely reproduced baseline lease/old-render-target-reference leakage after GetSurfaceLevel failure using counters/quarantine. I checked one frame return on error, balanced GetRenderTarget/GetTexture/GetSurfaceLevel references, and independent render-target/texture/viewport restore attempts even after one fails.

I tested owned-target reuse, Delete, device callbacks, Busy/later cleanup and modeled Draw/Present reentrancy while retaining the identity needed to return a lease. The actual void manager ignores callback results; Busy is no reset veto. Actual MANAGED/Reset remains separately tested.

I passed 292 current checks; the baseline executed 274 with 82 safe failures, skipping null/already-cleared-pointer dereferences. Eighteen extra current assertions depend on safe corrected paths, not eighteen complete new features. I corrected a metadata qualifier that wrongly required zero warnings in both: baseline had one legacy C4100 for lParam, current zero, both compile-success. I neither changed nor reran binaries for that correction.

I explicitly modeled EOF-token origin/validation, callback-list hosting, geometry, globals and D3D setters. I did not qualify every application event order, driver reference or arbitrary callback-time destruction, or retroactively enlarge this fixture using the separate device/local-play observations.

## How I bound the integration to actual build inputs

I retained typed factory creation, the installed-module policy, checked verification cleanup, generation-aware EOF handling and RAII frame/surface ownership. I did not cast the new SDK factory to the old interface.

I integrated through the movie library and both consumers, supplying SDK2 as an external dependency. The four adapter translation units use no inherited PCH and build only for x64; I preserved the selected Win32 behavior and legacy signatures. I retained explicit CNG/DirectSound dependencies rather than stubs. I checked the 17,189-file canonical source copy against its freeze separately from playable data and executable readiness.

## How I corrected generated outputs without removing MIDL

I expanded the source guard to reject extras and found fmod.dll plus four MIDL products absent from the historical 17,180-file inventory. I preserved them and the failed guard before removing only those extras, not inventoried historical files.

I traced MIDL: type-library output was private but the remaining output directory unspecified. I redirected /out to the private intermediate directory with preserved names/include availability, keeping generation enabled. FMOD copying belonged to Client; the old suppression targeted NtlSound only. I selected the correct Client event and retained verified AMD64 staging instead of removing audio. A preserved old FMOD timestamp was not proof of copy time.

I retained the old recipe and ran MIDL-only then isolated full NtlXMLLoader builds: zero warnings/errors, no regenerated source extras. The generated directory was available, but CL tracking did not show consumers reading the new _h.h/_i.c files; I did not claim that consumption. I used the reviewed private output/intermediate recipe for the full build while preserving the earlier guard failure.

## How I kept the product and verifier histories separate

I preserve the first complete AMD64 build as historical: zero errors, 598 warnings, 14,576,128-byte EXE, SHA-256 `e8cea6a1358d0ad4b1273615a8151d8c4cfe069858dfa60532abdb88f6f88b5b`. Its static check covered 18 direct imports and the then-applicable six-image local closure, not later fixes.

I completed the integrated Release x64 build in 19 minutes and 10.94 seconds using at most two compiler workers: zero errors, 573 warnings and a 14,668,288-byte EXE, SHA-256 `5b38017566a467534cc99f5f9eed20daa8f54eda2c66d5a9037c19951f5abe2b`. I retained EXE, PDB, MAP and build/tracking logs and checked source/dependency pins around the build.

I normalized paths and line numbers when comparing warnings. I found 25 removed diagnostics and none added: 20 associated with dump, MAC, entity identity, callback and cluster corrections, and five with the replaced wrappers' old Windows-version path. I do not equate diagnostics with bugs: a conversion can yield two and one MAC scan yielded six. The four new bridge translation units had zero warnings; the remaining 573 are warning debt. My earlier inventory found 376 unique diagnostics among 598 occurrences, distinguishing actual address loss from bounded integer tokens and retaining pre-existing limitations such as the disabled offline Lua compile wrapper. I did not execute every warning site.

I retain the original V6 qualification failure as HISTORICAL. PE/import/symbol checks had passed, but my verifier incorrectly required the standalone graphics build's `rpworld.lib`; the complete Client build legitimately rebuilt and selected another `rpworld` through `Ntl_Plugin_World`. I preserved the failed tool and four partial outputs, which were not a complete qualification. I corrected the verifier in V7 without recompiling or changing product source to satisfy the wrong assumption.

For each changed compiled source, I bound the source to actual compiler command/read/write groups, AMD64 COFF objects, librarian command/read/write outputs and matching object-payload SHA inside an archive. I checked relevant header dependencies in compiler read records and project metadata against the frozen recipe; headers and project files are not individual object files. Each modified compiled source requires at least one final selected chain. I retained available unselected archives without borrowing same-basename MAP attribution. Independent readback checked representative complete-build and standalone world-object chains; only the former were final full-path link inputs. Dead stripping means archive membership does not prove every corrected function survives or executes.

I then passed V7 on the same native output: 20 Client direct imports, seven explicit local images, 1,528 examined symbol references, real SDK2 factory/CNG/DirectSound imports, matching local PDB/RSDS identity and provenance for all 35 changed or added paths. My closure examines eager imports and required export forwarders with the Windows API-set model; it does not certify all optional Windows components or future dynamic plugins. These remain static proofs, not audible playback or gameplay certification.

## How I preserved source publication and checkout integrity

I retained a separate private source repository with reviewed game/first-party sources, project inputs, fixes, notices and dependency metadata. I did not discard first-party compatibility TUs merely because they were near SDK files. Vendor SDKs, binaries, personal runtime configuration and assets remain outside that source archive; it is not a self-contained redistribution package.

I independently fetched and compared the first 16,827 selected paths and all 191 canonical blobs and confirmed private visibility/default `main`. I preserved the first path-length staging failure, enabled repository-local `core.longpaths`, used exact literal paths and compared raw blobs instead of accepting EOL conversion. Authentication used existing legitimate Git Credential Manager, without extracting a token.

I checked out 16,828 published files after the status-document addition, preserved and corrected seven LF-to-CRLF conversions, and matched working bytes/index to the historical commit. An untracked `devmaster.xml` had original bytes/timestamp and an inherited nested ignore exception, not proven build contamination; all 17,180 baseline files still matched.

I keep clients, SDKs, builds, symbols and backups ignored, with recipes/manifests retained separately. The selected compressed history was about 50 MiB at that check, excluding full clients, packs and binary SDKs. I do not claim the stable x64 base or all dependencies have been publicly distributed.

## How I made two independent physical client copies

I inventoried 131 original regular files and resolved only five permitted junctions: cursor, font, movie, pack and ts, each to its corresponding data root. I did not traverse arbitrary links; no empty directories required separate preservation.

I used 4-MiB buffers and exclusive creation, checked source SHA before/after and destination SHA/timestamps, and verified `nlink=1`, distinct identities and no destination hardlinks/junctions. I did not move, delete or overwrite originals to manufacture two copies.

| Copy I preserved | Result I recorded | Meaning I retain |
| --- | --- | --- |
| I made the full Win32 copy. | I checked 633 files and 10,488,136,644 logical bytes, including Client26/dependencies. | I retain an independent I386/PE32 baseline. |
| I made the x64 data copy. | I checked 623 data files and 10,467,815,114 logical bytes plus a not-ready marker. | I excluded the Win32 EXE, eight DLLs and old launcher at that stage. |
| I staged the qualified candidate later. | I added its EXE, six AMD64 DLLs and dedicated launcher and rechecked the 623 data files. | I marked it ready for a human test, not universally stable. |

I preserved private configuration, profiles, screenshots and cursor/font/movie/pack/ts/user data byte-exactly, without publishing their contents or replacing defaults. I retained the relative configuration/movie paths and separate ignored `client-local/32bits` and `client-local/64bits` folders. Server19 was already AMD64; staging did not restart it or remove the original operational archive.

I also retained the authorized cleanup record: I removed only five oldest full ZIPs dated 17 October 2026 at 182806, 191809, 200217, 202518 and 212601, keeping full package 213458, updates and hash files. I preserved names, sizes, historical hashes and metadata before deletion. Their 52,015,129,439 logical bytes recovered approximately 29.1 GB physically because of NTFS compression; free space was about 46.5 GB before the two copies. I distinguish those quantities and did not delete the operational client.

## How I guarded the candidate staging transaction

I separated a read-only plan from copying; both tools do nothing by default. Active admission binds the actual build/qualification generation, source freeze, dependency hashes and fixed destination. I do not launch, log in, query SQL or control services through staging.

I selected the real EXE and six qualified AMD64 DLLs: DevIL, both staged BugTrap x64 variants, Discord RPC, FMOD and `bdvid64.dll`. I separately checked installed AMD64 `d3dx9_43.dll`, not its I386 counterpart. I did not carry BVL32, local x86 D3DX or an FMOD shim into the candidate. The launcher keeps cwd at the physical data root and has no automatic-login arguments.

I retain the not-ready marker through copying/readback, back up explicitly approved old targets, reject unexpected overwrites/reparse paths and refuse a destination outside the fixed root. My seven historical transaction scenarios used real synthetic copies, fsync, Windows rename/rollback and 623 individual synthetic hashes each: success, copy failure, commit failure, receipt-publication failure, target appearing after admission, approved-old-file rollback and an unowned pre-existing READY. I preserved a fixture failure whose receipt-search glob was one directory too shallow, correcting the test rather than claiming a product fix.

I repaired two narrower issues. A foreign local DLL/PYD could appear after planning and escape an owned-name-only check, potentially shadowing the checked system D3DX. I added an early executable-image guard before metadata/history writes. READY ownership had also been assigned only after publication/readback, so a readback failure after rename could leave false readiness during rollback. I calculate the exact marker bytes/SHA beforehand and attempt owned READY removal independently of not-ready restoration.

I used the same five-contract oracle: the retained earlier tool had five safe failures and the revision passed five. Three cases call real admission with foreign DLL/PYD files and prove refusal before writes. Two exercise actual synthetic copy/rename/rollback with READY readback or launch-block restoration faults; admission/protected-data are explicitly replaced there, not claimed as new real-product/623-data acceptance tests. A failed not-ready restoration still requires reconciliation. I preserve revisions and do not relabel the seven older scenarios as reruns of the final tool.

## How I updated the early pending states after qualification

I preserve the original V6-complete/qualification-pending account as HISTORICAL, followed by V7 PASS on the unchanged executable, canonical copy of all 17,189 sources and physical staging. The aggregate data hash before/after staging matched: `b59a1f80650e39e5926ab2869bb674066134ba3ecc37fdb50ddcc188a9131d44`.

I removed the not-ready marker only after checks and recorded `READY_FOR_HUMAN_TEST_NOT_STABLE`, retaining symbols/receipts privately. Staging did not start the client or restart the server. My later few-hour session is a separate chronological observation, not retroactive gameplay approval of preparation records.

## How I keep the measured test groups distinct

I retain 901 sound checks, 152 facade checks, 29 reader/policy checks, 165 historical real-adapter checks, the two different 210-check groups and 292 consumer checks as separate results with the boundaries stated above. I do not sum them into a coverage score, reassign old results to a new executable or equate assertions with independent complete features.

I retained a ten-step manual game-test plan whose preparation revision said NOT EXECUTED. I do not convert all ten entries to PASS because I subsequently played locally. I only assign an individual result when I know which path, revision, duration and repetitions were actually exercised. Silence in a log and a folder name are not substitutes for that observation.

I keep login/world entry, map/NPC selection, sorting, coupons, pet/fuel/vehicle, combat, quests, mail/trade, audio/video and reconnect as distinct possible test scopes. The existence of a plan for those scopes does not mean I completed each of them during the few-hour session. I also do not turn the already-observed Win32 Alt+Tab freeze into a new migration-only defect without evidence.

## How I retain verifiable evidence without publishing private contents

I retain source inventories, frozen copies, raw before/candidate files, reversible patches, build recipes, dependency contracts, old binaries/objects, native logs, controlled captures and publication/readback receipts. I preserve failed attempts and corrective addenda. My dated document snapshots keep the narrative that existed at each point; they do not rewrite old results to match a later conclusion.

I include identifiers and hashes below to distinguish the principal records, not to imply that private SDK binaries or runtime data are downloadable from this issue. I do not attach verification arguments, user profiles, personal configurations, licensed SDK source or vendor binaries. A record's hash identifies its bytes; it does not independently establish that every claim inside it is correct.

| Evidence I preserve | SHA-256 I recorded |
| --- | --- |
| My original adapter design | `2eff1f6f7981c7f41c95c678780619c6a99c0ab782b04c82ae8ae5cc4f9e3831` |
| My earlier implementation narrative | `65db0a1e0d978727dd1c75b384d9491be7fb58612fb0c3c213ca6c8be00ce480` |
| My adapter history | `53f4be7cae01e20b2688a349dfaf2c6f8919f3ecaf95193d3bacefc0598e0e6b` |
| My original sound history | `c3db6b7e150c63913d62c49dde2aa4a30c32c92fae048634a6332212dff421d7` |
| My sound V2 history | `1ca921dcdd20dfb2c69a9e5b299a9d7404ddeaf38cb00c3a8af9cfdefe26e27b` |
| My MANAGED V3 history | `79aeaad2100e64d9a94b2c45b5eb968f62ea92a02b617cb2374434e92d2fce51` |
| My consumer-fixture history | `ebca5fcac09da5cbee7933ac87933ab8e6e223784a8d27b191ba20362ded4199` |
| My physical-copy history | `7da5f2772d386ac130efdb718241317886646e9e350e179eefe246a299eb1bde` |
| My video-probe history | `555c3c64e04bc94137b2fa00b981cc5f4a685e1ea9fcbf7f17d76d1289375850` |
| My initial SDK1 observations | `2db98b910f7a97de9ef34b6c37ef9419e795f9f6dbb0b6963f7342ee3fc581a4` |
| My SDK1 V3 follow-up | `833dcc857d055fb30499c8a2051cc0ab183c3fc4ecb48feae88e71748502134d` |
| My SDK1 D3D9 V4 history | `e59cd1721ff4397dd70be13e5dcb7f654cf77d95f42d8f66a590b485e5f8b235` |
| My sound V2 candidate | `5a45c4ca6f2c3eef68659f739e7ef2efe64b4472d9fe013c92ead8f3189090f2` |
| My retained sound V2 review | `a11f09210fe6f87856b9ed7731f99513238bbb7bbd190b05ab4a1d6a0605828b` |
| My SDK2 format/alpha result | `8074973be0944128ea58a28c0861ca759122369baac355f392402770dd80b73e` |
| My MANAGED/Reset result | `4d70d8576572cf9d2db6fffa3a1962667ff877f5d4387e2f26d22d07b70e24af` |
| My final integration candidate | `191e80527d1f8f584bb9e5a174847d008d965edf4f52b1ecef2f2510a6f1ed78` |
| My raw consumer result | `1cf4374de0c23d80974e178ca5ecac3be23ec2dc96f5f3071c950670d78e5354` |
| My consumer qualification | `086acb6be44651df0e84c386dc75c53bb03952658d367e5d9fb8eca0b66c295b` |
| My physical-copy qualification | `4dc219543e0fa4c301703d485c9e9629482cfa7eb70e20e604b8b0d2fe3098f7` |
| My corrected output recipe | `5297fcdc8eee82140ea20b0b315ed969d08fd70dc8c36b244b728ddff14ff290` |
| My SDK1 D3D9 measurement | `29fcdb4e01db68977f787082171cc84396682093d48b7ebdefa39f2694d44d9b` |
| My complete V7 qualification | `25f7e51c6136a0a07e001ca6efbf9cf095ebd3675cce6ce434b3f608b8d1cea3` |
| My actual staging receipt | `424a745741adcbefca975d41fb2141b9ccc1ff651a55d0eeefdfdc3f6201eeff` |

I retain SDK authenticity, interface compatibility and licensing/redistribution as separate matters. I preserve original copyrights and notices. I do not publish an external SDK or claim a right to redistribute it merely because its signature verifies, its factory resolves or its normal verification call succeeds.

I am publishing this as an account of the work, the retained failures, the fixes and the limited local result. I keep my earlier local-fix follow-up separate in [issue 137](https://github.com/OpenDBO/OpenDBO-Core/issues/137). I do not call the migration universally stable, claim that every manual-plan entry passed or announce an Alt+Tab fix that I have not established.

**NoobMaster70**
