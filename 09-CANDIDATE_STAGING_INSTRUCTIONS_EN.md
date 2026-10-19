# My x64 candidate staging procedure

> I translate this historical pre-staging procedure without converting its pending states into later successes. I retain its original V6 admission requirement as historical; I describe the subsequent V7 result and actual staging separately in issue #138.

I prepared the bootstrap plan and a separate copying script. I had not yet staged the physical client when I wrote this procedure. I keep both scripts PREPARED_ONLY by default; I do not start the game, log in or control services through them.

I generate an active plan only after checking the build's exit0 and static V6 qualification from the same generation. I bind the EXE, six AMD64 DLLs, symbols, freeze and dependency contract through the receipt hashes. I keep cwd at the physical x64 data folder without automatic-login arguments.

Before copying, I check every individual hash of the 623 preserved files. I do not replace ConfigOptions, SystemEnv, profiles, packs or screenshots with defaults. I accept only destination names listed in the plan; if an approved target already exists, I save and verify its backup before replacing it. I refuse an unexpected file rather than overwrite it.

I prepare the seven PEs and launcher in a temporary folder inside the destination itself. I retain `CANDIDATE_NOT_READY.txt` while copying and performing readbacks. I check all 623 hashes again after copying and mark `READY_FOR_HUMAN_TEST_NOT_STABLE` only when every check passes. I keep human gameplay validation pending in this historical procedure.

If the operation fails, I restore the block and approved backups and remove only new files that still have exactly the hashes I wrote. I preserve a changed or unknown file and record the need for reconciliation. I retain partial artifacts privately for diagnosis, without recursive cleanup or an automatic attempt to start the product.

I had already exercised the tool contracts using private synthetic files; I do not treat that exercise as physical-client installation or a replacement for real rendering, network, audio or gameplay tests. I use `--execute --root-authorized` only after explicit authorization of the concrete operation and its pinned plan.

**NoobMaster70**
