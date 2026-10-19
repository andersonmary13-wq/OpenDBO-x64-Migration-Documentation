# How I reproduce the source and native build

I publish a source patch series and my portable build recipes, not a self-contained game or SDK distribution. I keep the upstream history in my own clone. I do not attach licensed SDKs, registration values, client assets, profiles or binary libraries. The 35 migration paths alone are **not** all the inputs needed to build the product.

## How I prepare independent source trees

I use Python 3.10 or newer and Git for Windows. I first obtain a legitimate upstream checkout at the exact tested base `ed30bbe393653c704130aa026baf2bcad02c31ff`. I create my own dedicated branch, for example `x64-migration`, and keep the checkout clean, including its index and untracked files. The tool does not fetch, create branches, commit, push or change my Git configuration.

```powershell
python tools/apply_migration.py <CORE_CHECKOUT> --check --manifest-sha256 <RELEASE_MANIFEST_SHA256>
python tools/apply_migration.py <CORE_CHECKOUT> --apply --branch x64-migration --manifest-sha256 <RELEASE_MANIFEST_SHA256>
```

I use `--stage baseline` in a second independent checkout when I need the corrected Win32 baseline for protocol comparisons. The default `--stage all` checks/applies the 191-file baseline stage followed by the 35-path migration stage. I do not apply one stage and then attempt the complete series again on that dirty tree.

I check exact raw hashes, the explicit LF projection and the BOM/newline restoration recipe. For Git's LF/CRLF checkout conversion I also require the same encoding/BOM, the exact pinned projection, and reconstruction of the original raw hash through its recorded before-format recipe. I do not accept altered content, a changed BOM/encoding or arbitrary normalized source. Both zero-context patches are checked in temporary copies of the whitelisted union before the checkout can be written. The default writes no checkout or index bytes. Explicit application does not stage changes; it retains an external backup and attempts byte-exact rollback on failure. I keep other editors and build processes out of that tree while applying. An unresolved rollback requires manual reconciliation from the retained backup; I do not call a partial application successful.

## Which external inputs I need

I used MSVC v142, Windows SDK 10.0.19041.0 and the June 2010 DirectX SDK. I require native AMD64 versions of the dependencies and the genuine APIs, not renamed Win32 DLLs, Discord stubs, an FMOD stub or a fake verification result.

I supply these external inputs through explicit paths and a hash-pinned local dependency contract:

- A lawfully obtained compatible Scaleform GFx SDK and its AMD64 archive/renderer. I apply my two minimal guard changes and separately qualify the result.
- JPEG 6b and PNG 1.2.18 built for the current x64 C-runtime/toolchain conventions. The historical migration rebuilt them; I do not bundle their source trees or binaries here.
- The real AMD64 FMOD import library and the existing first-party `fmod_cpp_wrapper.cpp`, compiled as an AMD64 static wrapper. That wrapper already exists at the tested upstream path `DboClient/Lib/NtlSound/fmod/fmod_cpp_wrapper.cpp`; I do not substitute `fmodstub.obj` or the historical shim.
- Genuine AMD64 Discord RPC, BugTrap/BugTrapU and DevIL dependencies, with their applicable notices and licenses retained.
- The SDK1 interface headers already required by the legacy callers and a legitimately obtained compatible Bandi Video SDK 2.1 header/library. I do not bundle either SDK. I retain the real `CreateBandiVideo2` API and verification call. Any required verification input remains the operator's responsibility; no script here scrapes it from product source or stores it in environment variables.

I do not treat authenticity, API compatibility, user authorization to publish my changes, and vendor redistribution rights as the same question.

## How I reproduce the GFx guard changes

I include `dependencies/gfx-allocation-guards.patch` with **zero context**, not full vendor files, and exact before/after hashes in `gfx-guards-manifest.json`. My changes affect only `GImage::CreateImage` and the lossless bitmap loader: one image-object allocation, two returned-image checks and five temporary-buffer allocations. I retain the existing empty-image/skip behavior on failure.

```powershell
python tools/prepare_gfx_guards.py --sdk-root <MATCHING_EXTERNAL_GFX_SDK>
python tools/prepare_gfx_guards.py --sdk-root <MATCHING_EXTERNAL_GFX_SDK> --prepare --output-root <NEW_GUARDED_SOURCE_DIRECTORY>
```

I compile only those two prepared translation units using the flags and include directories in the guard manifest. I remove exactly `GImage.obj` and `GFxTagLoaders.obj` from a **copy** of the original AMD64 archive and insert the two new objects. I compare all 188 object payloads, requiring exactly two changed and 186 byte-identical objects. I separately require the allocation-failure/headless proof; checking or preparing the patch does not run its historical 48 assertions and does not create a qualified archive by itself. I retain the original SDK and archive unchanged.

## My local dependency contract

I create a private JSON contract with schema `opendbo-client64-dependencies/v1`, architecture `AMD64` and `native_build_ready: true` only after its external qualification. I do not copy a private machine's contract into the public source tree. Its fields are:

- `files`: every required external input as `{path, bytes, sha256}`, with paths relative to my explicit dependency root.
- `paths`: `gfx_sdk` and `bandi_sdk2_include`, both relative directories inside that root.
- `features`: true for `Discord`, `BugTrap`, `DevIL`, `FMOD` and `BandiVideo`.
- `gfx`: `guards_preserved`, a pinned `protected_archive` at `link/libgfx.lib`, a pinned `qualification_receipt`, and the separately observed `headless_checks: 48` / `headless_failures: 0`. Preflight checks pins/AMD64 objects; these fields do not replace the native proof.
- `BandiVideo`: `actual_export: "CreateBandiVideo2"` and `verification_preserved: true`.
- `runtime_files`: six `{source: {path, bytes, sha256}, destination_name}` rows for `DevIL.dll`, `BugTrap-x64.dll`, `BugTrapU-x64.dll`, `discord-rpc.dll`, `fmod.dll` and `bdvid64.dll`.

I arrange the qualified import/static libraries under `link/` and point the SDK directories at their genuine external headers. I retain the actual dependency/API and link qualifications separately; these declarations are assertions I must substantiate, not a license grant or an automatic compatibility test.

## How I invoke the prepared native build

`tools/Build-Client64.ps1` does nothing by default. I supply all paths explicitly when I choose to build:

```powershell
./tools/Build-Client64.ps1 -Execute -SourceRoot <CORE_CHECKOUT> `
  -SourceManifest <SOURCE_EXPORT_MANIFEST> -SourceManifestSha256 <SHA256> `
  -DependencyRoot <EXTERNAL_DEPENDENCY_ROOT> -DependencyContract <LOCAL_CONTRACT> `
  -DependencyContractSha256 <SHA256> -DirectXSDK <DIRECTX_SDK_ROOT> `
  -BuildRoot <NEW_OUTPUT_DIRECTORY> -MSBuild <MSBUILD_EXE> -Python <PYTHON_EXE> -Jobs 2
```

I retain the actual 33-plugin sequence, the `rpskin2`/`rpuserdat` target aliases, all Client project references and transitive dependencies. I keep explicit Release x64/v142, `/MT`, one compiler per project and at most two project workers. The portable `Client64.Output.props`, `RenderWare64.Output.props` and `Dependency64.props` preserve private output/intermediate/PCH/tlog destinations, official DirectX include priority, MIDL output/name guards, the four x64-only/no-PCH video translation units and suppression of the old Client/NtlSound post-build source-tree copy. Runtime DLL staging stays inside the new build output; it is not an installation.

I freeze the actual source tree and external pins before compiling and check them again afterward, including unexpected generated source files. I require the actual output PE to be AMD64. I preserve logs/results even when compilation fails. The native metadata does **not** certify import closure, PDB identity, guarded-object archive/MAP selection, wire ABI, physical audio, audiovisual synchronization, gameplay or long-term stability.

I can supply `-QualificationScript <STATIC_QUALIFIER.py> -QualificationScriptSha256 <SHA256>` as a separate explicit hook. The hook receives `--native-output-manifest <.../native-output-metadata.json>`. I must use a reviewed static qualifier that checks local import closure, SymChk/PDB, the genuine SDK2 export, guarded GFx/RW object provenance and the actual selected compiler/object/archive/link/MAP chains. The historical V7 qualifier is not copied unchanged because it bound private paths and prior-run receipts. I do not claim that this new portable wrapper has repeated the historical full build or qualification merely because its parser/default and isolated contracts pass.

## How I retain the evidence boundaries

I keep the published historical results tied to their measured revisions: first native build 598 warnings, later integrated build 573 warnings, V6 verifier failure followed by V7 qualification on the same executable. My local game observation was only a few hours on my own server. Excessive repeated Alt+Tab could still freeze the client, as it already could in Win32. I do not turn source application, a portable recipe, a native exit code or a short local session into universal stability or feature-equivalence claims.
