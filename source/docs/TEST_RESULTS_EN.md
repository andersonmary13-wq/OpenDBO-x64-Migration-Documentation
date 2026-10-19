# My source reconstruction and test results

I distinguish my historical migration tests from the new checks on this public source package. I retain the detailed chronological records in [my original documentation snapshot](https://github.com/andersonmary13-wq/OpenDBO-x64-Migration-Documentation/tree/2e1ba49ee97fe136da1495370b07fa02f0db5292); I do not rewrite earlier failures into later successes.

## My historical product results

| Scope | What I observed | What I do not infer |
| --- | --- | --- |
| Final native client build | I completed Release x64 with zero compiler errors and 573 warnings; the build took 19 minutes and 10.94 seconds. | I do not equate warnings with unique bugs or a successful link with complete game coverage. |
| Final executable | I recorded `Client.exe`, 14,668,288 bytes, SHA-256 `5b38017566a467534cc99f5f9eed20daa8f54eda2c66d5a9037c19951f5abe2b`. | I do not promise the same executable hash across different dependency archives, tools, timestamps or build environments. |
| Static qualification | I passed my V7 qualification against the unchanged final executable after correcting the verifier's mistaken RenderWare input assumption. | I do not describe that verifier correction as a new product-code fix. |
| Native sound fixture | I passed 901 checks with my corrected sound port; the earlier candidate had seven controlled failures. | I do not treat a controlled sound fixture as complete audiovisual synchronization testing. |
| Facade fixture | I passed 152 checks; the earlier candidate had two controlled failures. | I do not treat the fake backend as the real vendor SDK. |
| Consumer fault fixture | I recorded the final 292-check result and retained the separate 274-check/82-failure earlier attempt. | I do not compare different conditional check sets as if they were identical runs. |
| D3D9 texture probes | I retained separate 210-check groups: the alpha experiment used six cases by 35 checks, and the MANAGED Reset experiment used five cases by 42 checks. | I do not merge those distinct experiments into one universal device test. |
| Win32/x64 wire ABI | I matched 2,304 records: 2,216 protocol structures, 14 aliases, 64 critical offsets and 10 golden byte sequences. | I did not exercise transport, login, databases or gameplay through this header/byte-layout proof. |
| Local game sessions | I played for a few hours only on my own local server without other observed errors. | I did not establish production stability, exhaustive feature coverage or compatibility with all hardware. |
| Excessive repeated Alt+Tab | I could encounter freezing, which I had also encountered with Win32. | I have not established the cause, a new x64 regression, or a fix. |

## My checks on this public package

I reconstructed both patch stages in an isolated temporary copy of the required upstream inputs. I checked all 191 baseline after-images, then all 35 migration after-images. I compared the final union of **225 distinct paths** with my tested x64 source bytes. I also checked all nine complete bridge files against the same frozen source hashes.

I checked both zero-context patch stages with Git before applying them. I restored the documented encodings and line endings before checking raw hashes. This detects output changes that a successful fuzzy textual patch would miss.

I also ran the public applicator against a new isolated sparse checkout of the actual pinned upstream commit. My first attempt correctly refused input outside its original raw-byte list, but that list was too strict for the clean CRLF representation produced by upstream Git attributes. I retained that failed attempt. I then admitted only an equivalent line-ending representation with the same encoding/BOM, the exact pinned text projection and successful restoration of the recorded before-image hash. I did not change either frozen patch or the source manifest.

My repeated real-upstream check passed without changing the target's bytes, index or clean status. I then explicitly applied the series on that temporary checkout's dedicated work branch and matched all 225 final raw file hashes. The tested applicator SHA-256 is `9652f23db890fcfe1ccc1ff65f66d91cbc786b2bf65d6bbcc7cedf3da2dc0771`.

I test the public applicator separately for its branch/base, path, hash and rollback behavior. I record the exact results and public payload hashes in the package verification records. I treat these checks as package reconstruction tests, not as a new native client build or a new game session.

I passed 17 synthetic Git/applicator tests and 11 build-guard tests. These cover protected branches, wrong base, dirty/index/untracked input, rejected paths and pins, default check behavior, explicit two-stage application, rollback, equivalent line endings, synthetic PE/COFF/archive metadata and source/project contracts. I also passed Python syntax checks, PowerShell parsing, XML parsing and the default no-build paths. I preserved the earlier fixture and environment failures separately rather than treating those attempts as passes.

I checked the minimal external GFx guard patch against both source files' before/after raw hashes without changing the original SDK inputs or executing a compiler or SDK. I did not repeat the historical 48-assertion headless proof for the new portable preparation tool. I did not rerun the full native client build or V7 qualification for the new portable build wrapper.

I include a portable adaptation of my actual native wire ABI harness. I retain its generated structure sizes, opcodes, offsets, static assertions and golden byte comparisons. I separate validation of this adaptation's preparation from the earlier native 2,304-record run; I do not claim a new native run unless I record it explicitly.

I do not execute the private game, restart a server, edit the private source repository, or change issue #138 while preparing this publication. I exclude proprietary dependencies and private registration inputs, so I do not claim that this public package alone can produce the previously tested executable.

-- NoobMaster70
