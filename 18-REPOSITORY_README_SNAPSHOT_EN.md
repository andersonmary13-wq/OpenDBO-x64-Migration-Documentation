# Dragon Ball - Online

> I translate my preserved README snapshot from 2026-10-19T18:59:58Z, not the latest `main`. I retain its historical build-pending and not-ready statements rather than replacing them with later results. I prepared this copy for review only, without publishing it.

I keep the game sources, the preserved Win32 client history, and records of my AMD64 migration in this private repository.

## My historical base

I preserved the Win32 client base with the accumulated changes through Client26 and Server19 under `Dragon Ball - Online/32bits/OpenDBO-Core`. I use `32bits` to identify the historical client; the Server19 executables were already AMD64 before this migration.

I published 16,827 selected files in commit `fee30079e226108e5e9ffc690f77dafd344080b8` and checked the complete tree and the 191 canonical correction files through an independent GitHub readback. I keep that folder frozen as a historical record.

I described the external SDKs and binary files in `DEPENDENCIES.json`. I preserve those dependencies locally; the published source archive is not a self-contained installation package. I recorded the original selection in `PUBLICATION-MANIFEST.json` and `PUBLICATION-PATHS.txt`. I retain `STATE32.json` as the preparation receipt preceding the first push, without interpreting its publication fields as the current state.

## My migration to 64 bits

I completed the first full Release x64 build on October 19, 2026, with zero errors and 598 warnings. I preserved its 14,576,128-byte AMD64 executable, with SHA-256 `e8cea6a1358d0ad4b1273615a8151d8c4cfe069858dfa60532abdb88f6f88b5b`, as the historical result of that stage.

I then corrected the pointer problems I found and integrated video/audio compatibility with BandiVideo SDK 2.1.5.19 while retaining the game's old interface. I checked that the 17,180 historical files remained intact and froze a candidate tree of 17,189 files containing 26 modified existing files and nine new files. I started the new complete build with those changes; the first preserved executable does not contain the later changes.

I compared 2,304 Win32/x64 protocol-representation records and ran the native proofs described in my reports. I still need, at this snapshot's stage, to qualify the new complete product and finish in-game tests, including rendering, audio, videos, and the already corrected functionality. I did not deliver or publish a stable x64 version at this stage.

I keep the x64 candidate outside `main` while completing validation. After publishing the stable base, I will create the new development branch from it and make subsequent changes on that branch. I will preserve the 32-bit history.

## How I organize my files and records

I keep the complete clients under `client-local/32bits` and `client-local/64bits`, excluded from Git. I prepared two independent physical copies and retain `CANDIDATE_NOT_READY.txt` in the x64 copy until completing AMD64-product preparation. I version the reviewed sources, recipes, and documents while preserving resources, binary SDKs, personal settings, and build artifacts locally.

I write my reports, README, manuals, and explanatory records in first person. I preserve attempts, the problems I found, corrections, test results, and limitations, with dated revisions and hashes. I will publish the migration reports to the community in English after satisfactory technical results and final in-game tests.

I have already published my [local-adjustment and issue #135 follow-up report](https://github.com/OpenDBO/OpenDBO-Core/issues/137). I confirmed the Client26 map and interface in earlier tests; I still keep the in-game statistical validation of Legendary-item promotion pending.

Signed: NoobMaster70
