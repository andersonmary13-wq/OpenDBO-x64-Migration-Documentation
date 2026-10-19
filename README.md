# My OpenDBO x64 migration source and documentation

I publish my migration source supplement on this dedicated `migration-source` branch.

- [My source changes and reconstruction instructions](source/README.md)
- [My portable build instructions](source/docs/BUILDING_X64_EN.md)
- [My tests and their limits](source/docs/TEST_RESULTS_EN.md)
- [My exact package verification](source/PACKAGE_VERIFICATION.json) and [checksums](source/SHA256SUMS.txt)
- [My licensing scope and vendor attribution](source/licenses/LICENSING_SCOPE_EN.md)
- [My 25 original documentation files, preserved at their original commit](https://github.com/andersonmary13-wq/OpenDBO-x64-Migration-Documentation/tree/2e1ba49ee97fe136da1495370b07fa02f0db5292)

I provide the actual C/C++ and Visual Studio project changes as two ordered patches against the pinned OpenDBO commit, nine complete new bridge files, the minimal external GFx guard changes and portable tools. I verified reconstruction of all 225 distinct changed source paths. I exclude complete proprietary SDKs, binary dependencies, credentials, private configuration and client assets. I disclose the unconfirmed vendor-derived redistribution permissions in my notices.

I preserve the upstream history in the recipient's own pinned checkout; this publication branch is a patch package, not a full mirror of OpenDBO history. I do not change the private game repository, the original documentation branch, [issue #138](https://github.com/OpenDBO/OpenDBO-Core/issues/138) or its existing comment through this supplement.

I tested my actual client only for a few hours on my own local server. Excessive repeated Alt+Tab could cause freezing, which I had already observed with Win32. I do not claim universal stability or that the new portable tools repeated the historical native build.

-- NoobMaster70
