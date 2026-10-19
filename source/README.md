# My OpenDBO x64 migration source changes

I publish my source changes against **OpenDBO-Core `ed30bbe393653c704130aa026baf2bcad02c31ff`**. I preserve my accumulated Win32 corrections before applying my x64 migration. I include the actual C/C++ and Visual Studio project changes, the nine new video/audio bridge files, a checked patch applicator and portable reproduction tools. I retain the original authors' credits, including the work I adapted from [pull request #136](https://github.com/OpenDBO/OpenDBO-Core/pull/136).

## What I publish

| File or directory | What I provide |
| --- | --- |
| [0001-baseline191.patch](0001-baseline191.patch) | I preserve the 191 changed paths in my corrected Win32 baseline. |
| [0002-x64-migration35.patch](0002-x64-migration35.patch) | I apply the 26 modified paths and nine new files of my tested x64 source revision. |
| [new-files](new-files/DboClient/Lib/NtlMovie/Video21Bridge) | I provide complete copies of my nine bridge files for direct study; the second patch already creates them. |
| [source-export-manifest.json](source-export-manifest.json) | I pin the upstream commit, both stages, encodings, line endings, before/after hashes and the final 225-path union. |
| [tools](tools) | I provide the guarded applicator, native build runner and dependency preparation tools. |
| [My build instructions](docs/BUILDING_X64_EN.md), [build properties](tools/Client64.Output.props) and [dependency changes](dependencies) | I provide portable build configuration and the minimal external GFx guard patch. |
| [tests](tests) | I provide my portable native Win32/x64 wire ABI proof. |
| [My test results](docs/TEST_RESULTS_EN.md) | I separate the historical product tests from the checks I performed on this source package. |
| [My licensing scope](licenses/LICENSING_SCOPE_EN.md) and [third-party notices](licenses/THIRD_PARTY_NOTICES.md) | I identify attribution, excluded dependencies and the unconfirmed permission for vendor-derived portions. |

I publish this as a **patch series in a separate public branch**, rather than a full mirror of the upstream tree. A full fork would also copy historical SDK payloads and registration definitions that I exclude from this publication. I preserve the upstream connection with an exact commit pin and two ordered deltas. When I apply these changes in my own upstream clone, I retain that clone's original Git history. I do not describe this package repository itself as a full OpenDBO fork.

## How I reconstruct the source

I start with a fresh upstream checkout at the pinned commit and create a separate work branch. I keep this package outside that checkout. I use Python 3 and Git; I run the applicator's `--help` first for the exact supported options.

```powershell
git clone https://github.com/OpenDBO/OpenDBO-Core.git OpenDBO-Core-x64
git -C OpenDBO-Core-x64 switch --detach ed30bbe393653c704130aa026baf2bcad02c31ff
git -C OpenDBO-Core-x64 switch -c x64-migration
python source/tools/apply_migration.py --help
python source/tools/apply_migration.py OpenDBO-Core-x64 --check
python source/tools/apply_migration.py OpenDBO-Core-x64 --apply --branch x64-migration
```

I perform the applicator's default check before its explicit `--apply` mode. I use a clean checkout at the exact base; I do not force patches onto an unrelated or already modified tree. I inspect and commit the resulting changes in my own work branch. The applicator does not commit or stage them for me.

I use the wrapper because these are **zero-context patches over a normalized text projection**. I remove only a leading UTF-8 BOM, decode UTF-16LE where recorded, and normalize CRLF to LF for patching. I then restore the recorded encoding and line endings and check the raw output hashes. Running plain `git apply` against arbitrary checkout bytes does not perform that procedure. I do not substitute fuzzy patch success for byte equivalence.

I keep the two stages separate because `DboApplication.cpp` changes in both. Their combined result contains **225 distinct changed paths**, not 226. Twenty-five migration-only upstream files differ from my preserved Win32 copy only in line endings; the manifest explicitly records the accepted upstream representation before the second stage.

## How I build and test

I follow [my build instructions](docs/BUILDING_X64_EN.md) with legally obtained, matching external dependencies. I use the real x64 dependency interfaces and retain the existing game features. I do not replace missing licensed components with success stubs or claim that an empty library reproduces my tested client.

I do not bundle a game client, assets, SDK implementations, vendor DLLs/libraries, registration values, accounts, database contents, private deployment configuration or my GM command reference. A matching source reconstruction is distinct from a successful build with my privately qualified dependencies. I document the remaining dependency preparation requirements explicitly.

I retain [my original report and technical appendix](https://github.com/OpenDBO/OpenDBO-Core/issues/138) and the [25 published documentation files at their original commit](https://github.com/andersonmary13-wq/OpenDBO-x64-Migration-Documentation/tree/2e1ba49ee97fe136da1495370b07fa02f0db5292). I have not changed that issue, its comments, the private game repository, or the original documentation branch to publish this source supplement.

I tested my x64 client only on my own local server for a few hours. Excessive repeated Alt+Tab could freeze it, as it already could with Win32. I have not established the cause or a fix. I do not present this package as a production stability certificate or as permission to redistribute proprietary dependencies.

-- NoobMaster70
