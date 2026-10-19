# My short human test plan — x64 candidate

> I retain this as my historical manual-test plan, not a report of completed gameplay. I keep every test NOT EXECUTED and retain the original planning-stage not-ready conditions. I do not use this snapshot to describe the latest candidate's readiness or to authorize publication. I replace my private absolute project path with `<WORKSPACE>`.

**I record the initial state of every test as NOT EXECUTED.** I do not attest to product access, qualification, installation, or PASS through this plan. I prepared it for use only after final qualification and explicit delivery of the candidate by the responsible party. I preserve version v1 as history without changing its bytes.

## Before I open it

I consider `<WORKSPACE>/client-local/64bits/Client.exe` a possible destination. I treat this path as future: the tree at this planning stage contains data and `CANDIDATE_NOT_READY.txt`. **I do not start the client while that marker indicates that preparation is not ready.** I do not use the migration's first historical executable as a substitute for the final product.

I check the delivered EXE's hash and its PE Machine AMD64 `0x8664` field in the final receipt. I do not use the folder name or file size as proof of architecture. After an authorized launch, I also check the process architecture in Task Manager when that information is available, without replacing the connection between PE, hash, and receipt.

I preserve `client-local\32bits`, the originals, and Client26 and Server19 in use. I do not restart services, edit SQL, reopen completed quests, replace drivers, or change system settings to carry out this plan. I enter credentials only through the normal interface and do not include them in notes, images, or reports.

## My manual test round

| Stage | My action | What I record and check | My initial result |
| --- | --- | --- | --- |
| Identity and entry | I check the receipt, PE, and hash; I open only the delivered candidate and enter the account and character through the normal flow. | I record the process's actual path, architecture, build, time, world entry, and basic character information. I do not record passwords or tokens. | NOT EXECUTED |
| Map and NPCs | I open M for the first time and check the current area, world/region/area selectors, and NPC list; I select an NPC, close and reopen the map, and change to a supported region. | I record perceived opening time, list and selector stability, agreement between selection, marker, and pulse, what I observe retained after reopening, and behavior when returning to the current area. I do not assume persistence that I have not observed. | NOT EXECUTED |
| Sort and persistence | I use low-value items in the bag and personal bank, note positions and quantities, and test sorting and eligible stack merging. I leave and re-enter through the game. | I check for disconnection, preservation of totals, correct donor removal, and persistence of final positions after re-entry. If freezing or disconnection occurs, I record that occurrence and do not repeat the action in a sequence to force success. | NOT EXECUTED |
| Pet, fuel, and vehicle | I use the normal pet, feeding, fuel-equipment, and vehicle activation and exit flows in a valid situation. | I record the selected and active pet, consumption and observed effect, equipped fuel, persistence after context changes and re-entry, and vehicle acceptance. I record before and after states and account for the fact that these tests may consume items. | NOT EXECUTED |
| Combat and flight | I test targeting, F and normal attack, a usual skill, and movement. If my character has flight enabled, I test WW while holding the second W, the camera, and turning. | I record recognized input, camera control, movement continuation or cancellation, and the actual outcome. I do not turn a short sample into approval of every state. | NOT EXECUTED |
| Quests, mail, and trade | I check available quests through their normal flow and exercise mail and a private shop with an item and test value chosen for this round. | I record progression and the NPC's actual response, sending and receiving, and preservation of the item and value. If a quest is completed or unavailable, I record “not applicable in this round” and do not restore it through SQL. | NOT EXECUTED |
| Screenshot | I create an in-world screenshot and check the generated file. | I check that the file exists, the image is legible, and its dimensions and content are consistent; I record whether the expected conversion occurred. I avoid capturing authentication or private data. | NOT EXECUTED |
| Audio and videos | I listen to normal and environmental sounds and check volume and pause using the existing controls. I watch videos reachable through the normal flow, including entry when available, and test stop and skip. | I record useful images, sound presence, volume, completion, and screen transitions. I keep unreached videos pending and do not use short samples to approve all audiovisual synchronization. | NOT EXECUTED |
| Alt+Tab and recovery | I switch to another window and return while the flow is stable. I test a supported display change only through the normal controls. | I record focus, mouse and keyboard, image, video, and audio before and after, device recovery, and any error. I do not alter drivers, system resolution, or profiles to mask failures. | NOT EXECUTED |
| Exit and reconnection | I perform normal logout, return to the character, and close the candidate through the normal flow. | I record whether freezing occurred, which state persisted, and whether reconnection preserved items and balance. I distinguish unexpected disconnection from the exit I requested. | NOT EXECUTED |

## How I record the outcome

For each stage, I note the date and time, candidate path, hash and build, exact action, character and channel only in the private record, observation, and evidence reference. I use “as observed in this round,” “failed,” “not executed,” or “not applicable,” with the duration, repetitions, and route actually reported. I do not enter PASS because there was no message, because of an isolated screenshot, or for a test I did not perform.

If a failure occurs, I preserve the message and time for correlation with the existing diagnostics and stop that stage. I do not use this plan as authorization to terminate processes, change the 32-bit client, restart Server19, or apply new corrections during testing. I limit my final approval to the set actually exercised and keep the other items pending.
