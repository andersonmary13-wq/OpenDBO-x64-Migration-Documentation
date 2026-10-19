# Consumer integration fault tests — first-person record

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

I froze the source pins of the private integration overlay and extracted the native consumer methods rather than reproducing their algorithms manually. I compiled the actual SDK1/BVLVideoInfo declarations, current x64 CBandiVideoLibrary wrapper and RAII helper header. I used declared test-only D3D HRESULT/reference/state facades, never a graphics device or vendor DLL.

My first compile exposed mistakes in the fixture type adapter: expanding a pointer typedef changed the second declarator, and the texture facade required an explicit void** bridge at the typed CMoviePlayer overload. I preserved that failed attempt, driver and template, then fixed only the adapter. The production bodies and overlay did not change.

The first useful run passed270checks for the current consumer and detected75safe baseline failures. I retained it. I next extracted the real void device-release manager and tested its continuation behavior, reaching272current passes. I then added explicit checks that all three restore setters are attempted even after one fails and that a second frame reuses the owned target. The final run passed292current checks; the baseline ran274checks with82safe failures. The18extra current checks cover baseline paths that would dereference null/cleared pointers and were intentionally not executed against the baseline.

I measured the surface-failure leak with counters and quarantine objects, not undefined behavior. I covered reentrant modeled Draw/Present callbacks, ReleaseBusy retention, later cleanup, EOF gating and controlled Verify/Open failure cleanup. I did not use real registration arguments. I preserved every compilation/run receipt and the previous templates/drivers.

My first closing qualifier overclaimed zero warnings for both variants and stopped safely. I retained it and corrected only that expectation: the baseline has one C4100 for its unused legacy lParam; the current consumer has no warnings. I did not alter or rerun the compiled inputs for this metadata correction.

I did not claim GPU Reset safety or DEFAULT/MANAGED equivalence from the facades. The native manager ignores callback return values, so Busy alone does not prove a veto. Real SDK/driver/pool and gameplay proofs remain separate. The final qualification read back the frozen overlay, body/header pins, compiler logs and AMD64 outputs. I made no Core, overlay, runtime, server, OS or Git changes.
