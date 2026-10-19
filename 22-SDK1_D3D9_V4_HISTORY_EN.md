# My SDK1 D3D9 alpha observation — V4

> Historical context: I preserve this complete record as a historical revision. Pending, not-yet-integrated, or untested statements below describe that revision at the time. I keep later qualification separate and do not retroactively broaden these results. The named local evidence artifacts are references, not attachments to this document.

I preserved the V3 MEMORYBUFFER source, runner, receipt and qualification byte-for-byte before preparing a distinct V4 harness. I did not replace that earlier observation or rerun the known crashing R565 format.

I created a new hidden Win32 HWND and a real HAL Direct3D9 device in each of six separate processes. I opened the unchanged local BVL32 library and logo AVI with the unchanged product Verify arguments transported only in memory/stdin and with NULL audio. I requested X8 and A8, each with SetAlpha 0, 128 and 255. I sought to9seconds and discarded stale leases until at least three distinct render advances and a timestamp of9.2seconds. All six actually reached seven distinct advances at9.199999999999978seconds.

I observed the actual texture description before reading pixels. In all six cases the native SDK returned X8R8G8B8(format22), managed pool, usage0,960×540—even when I requested A8. I copied its texture color and alpha with fixed-function SELECTARG(TEXTURE) into my own A8 render target, with blending, alpha testing, sRGB conversion and dithering disabled. I did not Present or draw to an external window. I called real GetRenderTargetData, locked the system-memory surface, and hashed RGB and BGRA bytes.

Every native case exited0. All six RGB and BGRA hashes matched the earlier MEMORYBUFFER V3 observation, with45155nonzero RGB bytes and every readback alpha byte255. SetAlpha, native device creation and GetRenderTargetData returned S_OK. I retained the actual Close-after-Stop result0x85861027 rather than replacing it with success. The unchanged SDK header produced its existing GetVersionExA deprecation warning; compilation had no errors.

This measures the returned texture for one asset and advancing timestamp. I did not infer the product's final screen blending, arbitrary SDK SetAlpha semantics, a visual fade, audio synchronization, license entitlement or complete adapter parity. I made no Core, SDK, installed client, driver or operating-system changes. The separate V4 qualification records the exact per-case HRESULTs, timestamps, input/source/executable pins and channel hashes.
