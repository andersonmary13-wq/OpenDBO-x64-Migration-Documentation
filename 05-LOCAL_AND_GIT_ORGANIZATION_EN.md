# How I organized my local files and Git

> I preserve this document's historical state when translating it. Its pending gameplay-test statements describe the original record; my later few-hour local-server observation and the Alt+Tab limitation are recorded separately in issue #138. I replace my personal absolute paths with the placeholders `<PROJECT_ROOT>` and `<LEGACY_WORKSPACE>`; I retain the technical content.

I prepared `<PROJECT_ROOT>` as my main project folder. I checked out the private repository's `main` there and compared all 16,828 published files with their Git blobs. I preserved and corrected seven LF-to-CRLF conversions introduced by the first checkout. At the end, the source bytes and index tree matched historical commit `84501d20b9982d8f0aa6496f90cf3756419c577b`.

| Content | Where I keep it | How I treat it in Git |
| --- | --- | --- |
| Historical Win32 client sources | `Dragon Ball - Online/32bits/OpenDBO-Core` | I keep them versioned and frozen. |
| x64 client sources | `Dragon Ball - Online/64bits/OpenDBO-Core`, after validation | I will publish the stable version and continue changes on a new branch. |
| Complete 32-bit client | `client-local/32bits` | I keep it local and ignored by Git. |
| 64-bit candidate client | `client-local/64bits` | I keep it local and ignored, with its statically qualified executable and six AMD64 DLLs. My in-game test remains pending in this historical record. |
| SDKs, builds, symbols and local backups | Ignored local directories and my preserved technical archive | I version the necessary recipes and manifests while preserving the artifacts separately. |
| Project reports, scripts and documentation | Selected repository files | I explicitly commit the reviewed files. |

I copied the 633 files of my current Win32 client, including Client26, its dependencies and the data previously accessed through five junctions. I copied those data as regular physical files. The complete copy contains 10,488,136,644 logical bytes, and the executable retains SHA-256 `42d9540c0b3c40396e3c9cea0b381abb35844e9825e107f0b4fd3aeef0d22519`.

I prepared the 64-bit folder with 623 data files, totaling 10,467,815,114 logical bytes. I excluded the Win32 executable, its eight DLLs and its launcher from this preparation. I kept `CANDIDATE_NOT_READY.txt` during that stage. After completing the build and static qualification, I added the AMD64 `Client.exe` with SHA-256 `5b38017566a467534cc99f5f9eed20daa8f54eda2c66d5a9037c19951f5abe2b`, the six AMD64 DLLs and `INICIAR_CLIENTE64_CANDIDATO.cmd`. I checked the preserved data and recorded `READY_FOR_HUMAN_TEST_NOT_STABLE` before removing the previous block. I had not yet run my in-game test at the time of this record; the folder name does not replace that validation.

I checked the hashes of every source before and after copying and the hashes of every destination. I preserved file timestamps and checked distinct file identities and the absence of hardlinks and junctions in both destinations. The two trees contain independent physical copies. I kept the original files during validation.

I confirmed that Git ignores both local clients. The published history occupied about 50 MiB compressed at this check; it contains the selected sources and metadata, without the complete client, resource packages or binary SDKs.

## How I freed storage space

With authorization, I removed only the five oldest complete ZIPs from `distribuicao`: the packages dated 17 October 2026 with times `182806`, `191809`, `200217`, `202518` and `212601`. I kept complete package `213458`, every update package and every hash file. I preserved the removed packages' names, sizes, historical hashes and metadata in a receipt before deletion.

The five ZIPs totaled 52,015,129,439 logical bytes. Deletion recovered approximately 29.1 GB physically because the volume also used NTFS compression. The observed free space reached about 46.5 GB before the two local copies.

## What I still needed to complete in this historical record

I still kept `<LEGACY_WORKSPACE>` as the source of the installation in use and of my migration archive, including ongoing tests. Cleaning up the five ZIPs did not remove that source or its records. I would complete consolidation of the necessary recipes, dependencies and artifacts using checked paths while preserving my historical archive.

I had completed video integration only in the x64 sources, the new complete AMD64 build with zero errors and 573 warnings, static qualification and preparation of the separate copy. My in-game test and operational-archive consolidation still needed completion at that point. After validation, I would publish the stable x64 sources and create the new development branch. I kept community reports in English and first person, pending the satisfactory tests I had agreed to perform.
