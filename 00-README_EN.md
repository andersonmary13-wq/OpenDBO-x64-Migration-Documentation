# My OpenDBO x64 migration documentation

I prepared these standalone English documents to accompany [my original issue #138](https://github.com/OpenDBO/OpenDBO-Core/issues/138). I preserve the published report and its complete technical appendix. I do not edit, replace, shorten or delete that issue through this documentation supplement.

I write my account in first person. I retain upstream and vendor attribution. I keep private local paths, personal repository links and confidential values out of these public copies; I record substitutions in a separate private review. I preserve the original local documents and their hashes.

## How I read these documents

I start with files 01 and 02 for the current migration report and detailed appendix. I copied those two already-published documents byte for byte. I use the remaining files as complete historical records of the design, implementation, failures, corrections, validation plans and observations at their respective revisions. I translated Portuguese records into English and adjusted internal narrative roles to my first-person voice without removing technical steps or results.

I keep historical proposals and outcomes distinct. A record saying DEFAULT textures, V6 qualification, PREPARED_ONLY or NOT_EXECUTED describes that record's revision. I do not rewrite it into the later MANAGED/V7/staged result. I do not assign every manual-plan item a PASS merely because I subsequently played locally.

I tested the x64 candidate for a few hours only on my own local server, with no other errors observed during those sessions. Excessive repeated Alt+Tab can cause freezing, which I had already encountered with Win32. I have not established a cause or a fix. I do not infer production-server stability, exhaustive feature coverage, all hardware compatibility or complete audiovisual synchronization from that local observation.

## My document index

I use relative file links for downloaded or cloned copies of this set. I will provide the actual hosted document links in a new issue comment only after publication is authorized.

| File | What I document |
| --- | --- |
| [01-MIGRATION_REPORT_EN.md](01-MIGRATION_REPORT_EN.md) | I present my migration process, changes, build results, tests and limited local play. |
| [02-TECHNICAL_APPENDIX_EN.md](02-TECHNICAL_APPENDIX_EN.md) | I retain detailed failures, refinements, test boundaries and evidence hashes. |
| [03-MIGRATION_PROCESS_AND_TESTS_EN.md](03-MIGRATION_PROCESS_AND_TESTS_EN.md) | I preserve my complete process/test record in English. |
| [04-DETAILED_TECHNICAL_RECORDS_EN.md](04-DETAILED_TECHNICAL_RECORDS_EN.md) | I preserve my full detailed technical records and revision-specific addenda. |
| [05-LOCAL_AND_GIT_ORGANIZATION_EN.md](05-LOCAL_AND_GIT_ORGANIZATION_EN.md) | I explain my source/archive/data separation, physical copies and storage accounting. |
| [06-VIDEO_ADAPTER_DESIGN_EN.md](06-VIDEO_ADAPTER_DESIGN_EN.md) | I retain my original SDK1-to-SDK2 adapter design and contracts. |
| [07-VIDEO_ADAPTER_IMPLEMENTATION_EN.md](07-VIDEO_ADAPTER_IMPLEMENTATION_EN.md) | I document the complete adapter implementation at its recorded revision. |
| [08-PHYSICAL_X64_BOOTSTRAP_ARCHITECTURE_EN.md](08-PHYSICAL_X64_BOOTSTRAP_ARCHITECTURE_EN.md) | I preserve the physical client/dependency/launcher architecture proposal. |
| [09-CANDIDATE_STAGING_INSTRUCTIONS_EN.md](09-CANDIDATE_STAGING_INSTRUCTIONS_EN.md) | I record staging admission, copying, rollback and preserved-data checks. |
| [10-VIDEO_ADAPTER_HISTORY_EN.md](10-VIDEO_ADAPTER_HISTORY_EN.md) | I retain adapter failures, queue ordering and frame-publication refinements. |
| [11-SOUND_ADAPTER_HISTORY_EN.md](11-SOUND_ADAPTER_HISTORY_EN.md) | I document initial audio implementation and underflow repair. |
| [12-SOUND_ADAPTER_V2_HISTORY_EN.md](12-SOUND_ADAPTER_V2_HISTORY_EN.md) | I retain Pause/Open corrections and the separate 901-check result. |
| [13-MANAGED_TEXTURE_V3_HISTORY_EN.md](13-MANAGED_TEXTURE_V3_HISTORY_EN.md) | I document the MANAGED-pool change and Reset with a live frame lease. |
| [14-CONSUMER_TEST_HISTORY_EN.md](14-CONSUMER_TEST_HISTORY_EN.md) | I preserve consumer fault tests, intermediate attempts and the 292-check boundary. |
| [15-MANUAL_GAME_TEST_PLAN_EN.md](15-MANUAL_GAME_TEST_PLAN_EN.md) | I retain the ten-step acceptance plan and its original unexecuted state. |
| [16-BUILD_WARNING_ANALYSIS_EN.md](16-BUILD_WARNING_ANALYSIS_EN.md) | I explain the 598-to-573 diagnostic comparison without equating diagnostics with bugs. |
| [17-WIRE_ABI_VALIDATION_PLAN_EN.json](17-WIRE_ABI_VALIDATION_PLAN_EN.json) | I preserve the complete native Win32/x64 protocol-validation plan. |
| [18-REPOSITORY_README_SNAPSHOT_EN.md](18-REPOSITORY_README_SNAPSHOT_EN.md) | I retain my authored repository README snapshot and its historical status. |
| [19-VIDEO_INVESTIGATION_HISTORY_EN.md](19-VIDEO_INVESTIGATION_HISTORY_EN.md) | I preserve DirectShow/SDK2 investigation, black-frame findings and useful-frame observations. |
| [20-SDK1_FRESH_FRAME_V2_OBSERVATIONS_EN.md](20-SDK1_FRESH_FRAME_V2_OBSERVATIONS_EN.md) | I retain the inconclusive legacy fresh-frame experiment and its limits. |
| [21-SDK1_FRESH_FRAME_V3_FOLLOWUP_EN.md](21-SDK1_FRESH_FRAME_V3_FOLLOWUP_EN.md) | I document the independent-process legacy alpha comparison. |
| [22-SDK1_D3D9_V4_HISTORY_EN.md](22-SDK1_D3D9_V4_HISTORY_EN.md) | I retain the legacy real-device texture/pool/alpha observations. |
| [23-PUBLIC_DOCUMENT_MANIFEST_EN.json](23-PUBLIC_DOCUMENT_MANIFEST_EN.json) | I identify each document, its source snapshot and its public-copy hash. |
| [24-SHA256SUMS.txt](24-SHA256SUMS.txt) | I provide SHA-256 checksums for the other 24 public files. |

## What I provide for study and reproduction

I document the upstream baseline, intentional changes, toolchain versions, dependency/API contracts, source freezing, build qualification, controlled native tests and staging procedure. I retain failed attempts and the distinction between simulated endpoints, real native APIs, real SDK/device probes and actual local play.

I provide a documentary set, not a complete build kit. I do not bundle my game source patch, test harness source, client assets, binary dependencies, external SDK implementation, symbols, dumps, credentials, user profiles or private GM reference. Some records name local scripts and receipts as provenance; those references do not mean the scripts or private artifacts are included here.

To reproduce a build or a specific native proof, I need the matching corrected source revision, the named toolchain and legitimately obtained compatible dependencies, plus the corresponding recipe or harness. I do not claim that these documents alone can reproduce an executable or every assertion automatically. I distinguish my dependency-authenticity checks from any right to redistribute a vendor SDK.

I retain my source and public-copy hashes in the manifest. I use `24-SHA256SUMS.txt` to verify the other public files; I retain that checksum file's own hash in my separate preparation receipt to avoid a self-referential checksum.

**NoobMaster70**
