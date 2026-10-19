# My managed-pool refinement

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

I preserved candidate V2 and prepared this separate V3 overlay after reviewing the SDK1 measurement: both requested 32-bit output formats expose a MANAGED usage-0 X8 texture. The earlier DEFAULT implementation required reset admission that the global callback dispatcher does not honor. I replaced staging/UpdateTexture with a direct managed-texture LockRect upload and retained outstanding D3D frame leases across reset. MEMORYBUFFER reset behavior is unchanged.

The same new native oracle safely records the prior DEFAULT candidate failing the pool and borrowed-lease admission checks. The final candidate passes five real SDK/device cases (42 checks each) and the existing 152-check facade regression. No Core application occurred.
