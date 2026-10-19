# My independent-process SDK1 alpha comparison

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

I preserved the entire V2 source/runner/receipt and inconclusive addendum byte for byte under `sdk1-before-isolated-v3`. I then performed one bounded refinement: I ran a new process/Open for each X8/A8 and alpha0/128/255 combination. I did not repeat R565, install a codec, open a game or reuse a rendered frame from another case.

I waited for at least three distinct CurrentRenderTime advances after Seek9, and accepted the measurement only at or after9.2 seconds. I did not filter acceptance by pixel color. Each case actually advanced seven times and reached9.199999999999978; all six observations contained45155 nonzero color bytes, equal color/BGRA hashes and alpha bytes255. This establishes equal MEMORYBUFFER pixels for the requested alpha values in this bounded, same-frame comparison. I do not generalize that observation to D3D compositing or other video frames.

The native compilation succeeded with the one unchanged legacy GetVersionExA deprecation warning. I retained all native HRESULTs, including GetVersion after Create returning0 and the negative Close result. The runner supplied only the unchanged original Verify arguments through memory/stdin and redacted them before any output could be saved. The original DLL, AVI, headers, client32 and runtime stayed unchanged.

The result is recorded in `sdk1-baseline-v3-20261019T171502Z-2b25b5b7` and `sdk1-fresh-alpha-timing-v3-qualification.json`. I stopped this line of investigation after the bounded V3 result; I did not attempt an unapproved D3D9 device test or claim complete visual/AV/feature equivalence.
