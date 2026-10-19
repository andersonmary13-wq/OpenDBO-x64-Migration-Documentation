# My third-party notices and provenance

I provide game-source changes and limited external-SDK patches, not complete vendor SDK packages. I preserve upstream ownership and the existing notices. I do not claim that my authorship of a fix gives me ownership of the original game, middleware, examples, interfaces, or assets.

## My license scope

I identify the upstream covered game source as GNU Affero General Public License, version 3, according to its `LICENSE` at baseline `ed30bbe393653c704130aa026baf2bcad02c31ff`. I include that license verbatim as [LICENSE_AGPL-3.0.txt](LICENSE_AGPL-3.0.txt). I offer my original covered-source contributions under the applicable AGPL terms to the extent I hold the rights to them. I retain the upstream license, copyright and warranty notices, and identify my changes separately. I do not use that statement to relicense vendor-owned material. [Upstream license](https://github.com/OpenDBO/OpenDBO-Core/blob/ed30bbe393653c704130aa026baf2bcad02c31ff/LICENSE), [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html).

I have **not confirmed a vendor redistribution grant** for the Bandi-example-derived sound implementation, the Bandi SDK headers, the RenderWare vendor source changed by the patches, or the Scaleform SDK changes needed by my local build. I disclose this uncertainty explicitly. Attribution, a zero-context diff, successful SDK verification, and a successful build do not supply a missing permission or resolve AGPL compatibility for a complete proprietary-linked product. I do not represent this package as an entirely cleared SDK bundle or a grant to redistribute third-party binaries.

## The owners and boundaries I preserve

| Component | Attribution and my use | What I am not granting or bundling |
| --- | --- | --- |
| OpenDBO game source | I preserve the upstream baseline provenance, contributor credits and AGPL notice associated with the source. I distinguish my changes from inherited code. | I do not claim ownership of all upstream work or rights to unrelated game media. |
| Existing game integration headers | `DboClient/Lib/Discord/Discord.h` retains its Daneos copyright notice; `DboClient/Lib/NtlPresentation/NtlPLRenderGroup.h` retains its NTL copyright notice. | I do not replace those notices with my pseudonym or infer a separate SDK license from the directory name. |
| Bandi Video SDK1 | Bandisoft owns the SDK1 material identified in both `BandiVideo.h` headers. I modify only the integration boundary in the patch series. | I do not bundle the complete headers or SDK1 runtime, and I do not claim a vendor redistribution grant for reproduced vendor portions. |
| Bandi Video SDK2 | Bandisoft owns the external SDK2 header, runtime and examples. My `Sdk21Native.h` includes an externally supplied `<bandivid.h>`; it does not embed that header. | I do not include the SDK2 header, DLL, registration credentials, CHM or vendor example files. A valid signature or an SDK `S_OK` result is not a redistribution license. |
| My SDK2 sound port | My `Video21Bridge/Sound/Sdk2Sound.cpp` records that I informed the implementation with Bandisoft's `BandiVideoSound_DS` and `BandiVideoSound_NULL` examples, whose notices identify Copyright 2008–2014 Bandisoft. I preserve that provenance and my subsequent lifetime, clock and concurrency work. | I have no confirmed grant to assert for public redistribution of the vendor-derived portions. I do not describe the entire file as independently authored or automatically relicensed under AGPL. |
| Criterion / RenderWare | I preserve Criterion Software Limited / Criterion Software Ltd. notices found in the pipeline and allocator sources. My changes address pointer-width, address arithmetic and related x64 behavior. | I do not bundle a complete RenderWare SDK or assert that its vendor-owned code became mine. Applicable vendor source/patch redistribution permission remains unconfirmed in my reviewed materials. |
| Scaleform GFx 3.3 | Scaleform Corp. owns the GFx SDK files. The reviewed `GImage.cpp` and `GFxTagLoaders.cpp` identify its copyright and refer licensees to a valid Scaleform Commercial License Agreement. My local archive has allocation guards in those two vendor objects; I provide a minimal zero-context guard patch for matching external sources. | I do not include complete GFx SDK source files, objects, headers, archives, keys or a commercial license. I do not claim that the minimal patch grants vendor redistribution rights or reproduces the qualified archive by itself. |
| FMOD | I preserve Firelight Technologies attribution for the external SDK/runtime. The local header identifies version 2.02.22. I distinguish my original C++ compatibility glue from Firelight's SDK files. | I do not bundle the FMOD SDK, DLL or import libraries or assert an applicable redistribution entitlement. Firelight's current terms distinguish SDK files, example code and runtime-product redistribution; the applicable agreement still matters. |
| Discord RPC | I preserve Discord's MIT notice for the external RPC implementation and distinguish it from the existing game wrapper. | I do not use its MIT license to license unrelated middleware. |
| BugTrap | I preserve its MIT notice and attribution when the dependency is used. | I do not treat a private qualification or a successful load as a substitute for its notice. |
| DevIL | I preserve the LGPL notices and version-specific obligations associated with the dependency. | I do not claim that source-only game patches discharge binary-distribution or relinking obligations for a separately distributed product. |
| Microsoft platform components | I use externally provided Windows, DirectX and compiler interfaces and tools. | I do not bundle platform SDKs or claim a new license to redistribute their files. |

## My Bandi-source permission disclosure

The mirror I inspected identifies the SDK as Bandisoft material and restricts its availability to study and gameware-engineering modification while reserving commercial licensing to Bandisoft. I do not interpret that mirror statement as an express Bandisoft grant to redistribute derived sample source. I have not obtained vendor permission for the derived sound implementation. I include this disclosure so that my credits cannot be mistaken for a confirmed redistribution license. [Inspected mirror revision and README](https://github.com/sigmaco/bandi-video-v2.1.5.19/blob/867bcf0832628357a346f6e48cd6f3627cdff155/README.md).

I keep the source warning in the sound implementation: “Do not publish vendor-derived code without determining its redistribution license.” I disclose that this remains unresolved; I do not remove the warning to make the provenance look cleared.

## My migration-source mapping

I associate the following 35 migration paths with these provenance groups. I use paths as identifiers, not as a claim that each complete file is newly mine.

- I identify these two existing SDK1 files as Bandisoft-owned header material: `DboClient/BVL/BandiVideo.h`; `DboClient/Lib/NtlMovie/BandiVideo.h`.
- I identify this new example-informed implementation separately: `DboClient/Lib/NtlMovie/Video21Bridge/Sound/Sdk2Sound.cpp`.
- I identify these nine paths as existing Criterion/RenderWare source with my local changes: `DboClient/Lib/Ntl_Plugin_World/src/pipe/p2/p2define.c`; `DboClient/Lib/Ntl_Plugin_World/src/plcore/bamemory.c`; `DboClient/Lib/Ntl_Plugin_World/world/pipe/p2/d3d9/d3d9vertexshader.c`; `DboClient/Lib/Ntl_Plugin_World/world/pipe/p2/d3d9/d3d9vertexshader.h`; `DboClient/Renderware/Include/rpskin2/d3d9/skind3d9.c`; `DboClient/Renderware/Include/rpskin2/d3d9/skind3d9instance.c`; `DboClient/Renderware/Include/rpskintoon/d3d9/skind3d9.c`; `DboClient/Renderware/Include/rpskintoon/d3d9/skind3d9instance.c`; `DboClient/Renderware/Include/rtwing/rtwing.c`.
- I identify these eight new paths as my authored bridge/interface/integration glue, with external SDK requirements and the license limits above: `DboClient/Lib/NtlMovie/Video21Bridge/Backend21.cpp`; `Backend21.h`; `LegacyVideo21Adapter.cpp`; `LegacyVideo21Adapter.h`; `Sdk21Native.h`; `Sound/Sdk2SoundFactory.h`; `Video21Integration.cpp`; `Video21Integration.h`. In this bullet, names after the first full path are relative to `DboClient/Lib/NtlMovie/Video21Bridge/`.
- I identify these fifteen paths as existing game integration or project files with my modifications and inherited notices: `DboClient/BVL/MoviePlayer.cpp`; `DboClient/Client/Addin/DboAddinManager.cpp`; `DboClient/Client/Client.vcxproj`; `DboClient/Client/Main/DboApplication.cpp`; `DboClient/Lib/Discord/Discord.h`; `DboClient/Lib/Discord/Discord.vcxproj`; `DboClient/Lib/NtlCore/NtlCore.vcxproj`; `DboClient/Lib/NtlCore/NtlDumpTarget.cpp`; `DboClient/Lib/NtlCore/NtlSystemInfo.cpp`; `DboClient/Lib/NtlMovie/NtlMovie.cpp`; `DboClient/Lib/NtlMovie/NtlMovie.vcxproj`; `DboClient/Lib/NtlMovie/NtlMovieManager.cpp`; `DboClient/Lib/NtlPresentation/NtlPLRenderGroup.cpp`; `DboClient/Lib/NtlPresentation/NtlPLRenderGroup.h`; `DboClient/Lib/NtlPresentation/NtlPLResourcePack.cpp`.

I preserve the corresponding notices for earlier baseline fixes as well. A patch to an existing vendor file can reproduce vendor code in removed lines as well as added lines; I do not call a zero-context patch a universal copyright exemption.

## My external-dependency references

I use the following primary references for the boundaries I can verify, rather than replacing the actual applicable agreement with my own terms:

- I refer to [Firelight's legal terms](https://www.fmod.com/legal) for its distinct SDK, example-code and runtime conditions. I do not certify that the current web text is the exact agreement supplied with every local file.
- I refer to [Autodesk's Scaleform source-access instructions](https://help.autodesk.com/cloudhelp/ENU/Scaleform-Help/scaleform_help/source_access.html) as access instructions for later versions, not as a public redistribution grant for GFx 3.3.
- I preserve the dependency notices for [Discord RPC](https://github.com/discord/discord-rpc/blob/v3.4.0/LICENSE), [BugTrap](https://github.com/bchavez/BugTrap/blob/master/LICENSE) and [DevIL](https://github.com/DentonW/DevIL/tree/v1.8.0).

I include no registration values, personal configuration, user profile, private absolute user path, SDK binary or game asset in this notice. I make no claim of vendor endorsement, universal runtime stability, or complete license clearance.

— NoobMaster70
