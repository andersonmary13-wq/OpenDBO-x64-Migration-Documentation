# My comparison of the full V6 build warnings

I read both complete logs without running compilers, an SDK, or the client. I checked that the first ended with 598 warnings and zero errors and the new one with 573 warnings and zero errors. Each log repeats all diagnostics in its summary: I counted 598/573 compilation occurrences, not the 1,196/1,146 repeated lines.

I compared a multiset by file, code, message, and project, ignoring only line/column to accommodate source shifts. I found 25 removed occurrences and no added signature. I retained the details and original positions in comparison.json, with hashes of both logs.

I attribute the removed diagnostics to the specific recorded locations: NtlDumpTarget (2), the MAC scan in NtlSystemInfo (6), NtlPLRenderGroup (6), NtlPLResourcePack (4), p2define (2), and both BandiVideo.h copies (5 GetVersionExA warnings). I do not interpret this as 25 corrected bugs: one cast can generate C4311 and C4302, and one scan call generates six type warnings. I did not find a warning for the bamemory mask correction in the earlier log; I rely on its separate native proof.

I found no warnings in the four new Video21Bridge CPPs. I retained the C4996 warning at NtlMovieManager.cpp:30, as well as the two integer-tag warnings in NtlFlasher already classified separately. I did not classify the remaining 573 warnings as harmless or declare gameplay, ABI, audio, or video stable on the basis of this comparison.
