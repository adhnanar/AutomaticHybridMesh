# Task016 — PC BLE mesh delivery with concurrent GATT text notifications

2026-10-19. **PC notification acceptance: 5/5 PASS** with no firmware modification, reset, LoRa transmission, app installation or phone. This is not an Android notification/background acceptance or independent per-trial LoRa/RF-isolation proof.

## Setup and method
Kit1/A `E5:12:76:D2:56:EA`, Kit2/B `C8:51:08:6E:08:C0`, Kit3/C `E2:DB:6A:54:BB:71` remained USB connected, at the prior A/C separated and B bridging geometry. Only existing Task012 A/C and Task010 B BLE firmware used. Windows PC Bleak first scans A/B/C, then subscribes to C's `7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00` GATT TX and `7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00` discovery CCCs with C connected. During this subscription, PC writes one unique text message to A's `7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00`, scans on-air BLE packets, reads the final C TX and Task012 evidence counters before disconnecting. Test harness `tests/task016_pc_gatt_coexistence.py`; review harness `tests/task016_review_logs.py`. Five raw logs `docs/task016-pc-gatt-coexistence-01..05.log`.

## Independent review of raw PC logs
`python tests/task016_review_logs.py` exit 0:
| Trial | Exact C TX notification and final stored payload | C accept increment | B TTL2 advertisement independently captured by PC | C rejected text increment |
|---|---|---|---|---|
| 01 | PASS | +1 | PC missed B; C last seq/TTL2 from A-source & +1 accept | 0 |
| 02 | PASS | +1 | YES | 0 |
| 03 | PASS | +1 | YES | +2 |
| 04 | PASS | +1 | YES | +3 |
| 05 | PASS | +1 | PC missed B; C last event overwritten by later TTL3 reject | +1 |

**5/5 live GATT notifications PASS.** Each unique payload appeared exactly once as a C TX notification and matched the final C GATT read, with C's binary evidence ACCEPT delta +1 for each trial. Duplicate frames were received but did not create duplicate notifications. No C discovery notification occurred during these five text trials; discovery characteristic CCC subscription itself worked but discovery delivery remains untested. No phone was connected; nRF Connect display/background behavior remains open.

**RF limitations:** During simultaneous PC advertising scan plus two GATT links, Windows saw B's TTL2 advertisement in 3/5 trials (A TTL3 source also scanned). Missing a B advertisement is *PC observation loss*, not evidence B failed to relay. C's evidence ACCEPT increment and exact notification validate delivery, but C only retains the **last** mesh event. In 3/5 trials, C rejection deltas rose by 2/3/1, showing intermittent directly received and topology-rejected text frames despite prior B-off isolation baseline. Physical RF isolation is empirical and not permanent; in trial05 last event was TTL3 rejection so last-sequence/TTL cannot independently prove the accepted frame's TTL. Prior five B-off isolation and five B-on PC-observed TTL2 acceptance trials remain separate independent baseline evidence.

**Script grading notes:** original trial01 command exited 2 solely because PC did not independently capture B's advertisement despite C accepted/notification, and trial03 exited 2 solely because its strict RF-isolation check rejected +2 direct frames; trial05 exited 2 solely because the strict B-advertisement/last-event check was inconclusive. These command statuses must not be reported as full strict relay-proof PASS. The independent offline log validator classifies the narrower but important **GATT notification objective** correctly as 5/5 PASS.

## Next
- No flashing/physical actions.
- More aggressive PC-only test: subscribe to C discovery GATT CCC while triggering controlled peer lost/new advertisement via PC GATT connection to B (only if normal network disruption is acceptable); independent check with PC logs.
- Later connect Android nRF Connect and manually confirm display/notifications; no custom app, no Android background alert guarantee.
- For hybrid delivery, integrate SX1262 radio via opt-in firmware after country/frequency and flashing approval; cross-radio message delivery not established by this test.
