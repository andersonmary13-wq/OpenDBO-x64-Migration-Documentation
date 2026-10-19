# My SDK1 fresh-frame observations

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

My first Win32 baseline is preserved at `sdk1-baseline-20261019T170241Z-2419fb6e`. Before my refinement I copied its source, runner and receipt byte for byte to `sdk1-before-fresh-v2`. I did not repeat the known R565 Open access violation.

I created `sdk1_baseline_v2.cpp` and `run_sdk1_baseline_v2.py` as separate files. I retained actual old BVL32.dll, its actual header, a hidden probe-owned HWND, memory output context and NULL audio. I read only the unchanged existing product Verify arguments in memory and stdin; I did not put identifiers/keys in logs, files, argv or a new credential store.

I moved GetVersion after Create, drained old leases, primed a frame near time0, then sought9 seconds. I rejected target leases whose CurrentRenderTime was outside9..9.25 and recorded the time before/after GetLastFrame. I measured raw/color hashes and byte-alpha values for requests0/128/255 in both X8/A8 formats, reversing the order in a second pass. I released each lease before the next Seek.

The native build succeeded with one C4996 warning from the unchanged legacy GetVersionExA header. Both formats exited0 in `sdk1-baseline-v2-20261019T171206Z-921e56d4`. Those exit codes qualify the observed API/timestamp traversal, not an alpha law. All target timestamps were9, yet the first alpha0 returned nonzero RGB and the later repeated seeks returned black even at alpha255. I retained this inconsistency and did not infer that alpha premultiplies RGB or is ignored generally. The memory DIB alpha bytes were always255; that fact alone says nothing about D3D compositor behavior.

The actual GetVersion after Create returned0. I kept the observed negative/rounding conversions and negative Close HRESULT unchanged. No game, physical sound device, GPU presentation, OS codec registration, source/runtime edit or licensing entitlement assertion was part of this refinement.
