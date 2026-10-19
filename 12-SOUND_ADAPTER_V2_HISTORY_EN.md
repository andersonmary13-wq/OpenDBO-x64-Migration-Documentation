# My SoundProxy V2 follow-up

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

I preserved the first frozen sound candidate (`sound-candidate-v1.json`, SHA256 starting `a30c61ce`) and its 879-check proof. I did not revise that historical success into coverage it did not have.

After the freeze I identified an additional defect in my new code: Pause during `GetPcmData` invalidated the operation epoch and dropped a block that the decoder had already consumed. I reported it before integration. I prepared `pause-v2` separately, leaving the active source unchanged. I retained consumed PCM in session-owned storage when the stream was paused, then submitted it before pulling new samples. Stop, Start and Close still reset stream identity and discard obsolete data.

During a separate review, I identified another defect: two overlapping Open calls could both pass the initial Close and replace one another, leaving the displaced session worker owned only by itself. I confirmed that the video facade serializes calls, but I did not depend on that as a hidden SoundProxy precondition. I therefore prepared an explicit admission/ticket fix in this V2 overlay.

I added an atomic Open admission guard. A second concurrent or reentrant Open returns `ERROR_BUSY`; it cannot replace the owner. I kept joins outside lifecycle locks. Close changes a generation ticket, so Close called during callback AddRef cancels an unpublished candidate. Mode changes remain forbidden while an Open is in progress. The SDK2 instance and its verification state are not recreated or changed by these mechanisms.

I extended the same native oracle with a real callback-triggered pause, sequential PCM payload checks, a deterministic callback barrier and two native threads, nested Open, and Close during Initialize. I used native SDK2/Windows declarations and real UCRT/threads/events/locks. DirectSound/owner/QPC endpoints remained explicit facades; no real audio hardware, SDK DLL, window or game opened.

The preserved V1 source produced 894 passes and seven safe failures out of 901 checks. The V2 overlay passed all 901. Both variants and the standalone production object compiled with `/W4 /WX`, zero warnings/errors. I retained both native results in `../tests/native-v3-20261019T170216803688Z`; I did not overwrite the 867/879-check receipts or any earlier video/license observations.

At this freeze the active `../Sdk2Sound.cpp` is still V1. I have not applied the overlay, modified Core, opened an audio device or qualified runtime AV sync. I will switch the private active input only after the planned review and integration gate. This port remains private vendor-derived code with redistribution rights unestablished; community publication is conditional on later satisfactory human gameplay testing.
