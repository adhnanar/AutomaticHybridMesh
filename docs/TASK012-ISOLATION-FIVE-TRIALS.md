# Task012 B-off A-to-C isolation baseline — five successes at revised geometry

Date: 2026-10-19. User moved A and C farther apart after prior B-off isolation failure (C then decoded 23 direct TEXT frames). Kit2/B remained powered off; PC USB enumeration saw only Kit1/A COM14 serial 1D5EA4A525ED34EB and Kit3/C COM16 serial B13E025E5A1407EE. Approved Task012 topology firmware installed on A/C, unmodified Task010 firmware on powered-off B.

Method: `tests/task012_live_isolation.py` for five separate, newly generated A messages and PC on-air evidence, plus read-only C GATT evidence snapshots before/after each trial. Logs `docs/task012-isolation-live-02.log` through `-06.log`. For every repeat: PC saw A TTL3 exact source+sequence+payload, C GATT stored TX `Ready` remained unchanged, B not discovered, new C GATT counters readable with valid record checksums, and C text/reject/accept/duplicate counters unchanged. All five runs exited 0 and printed `ISOLATION_RESULT=PASS_NO_C_TEXT_DECODE_IN_TRIAL`.

| Log | A source sequence TTL3 | C decoded delta | C TEXT delta | C REJECT delta | C ACCEPT delta | Outcome |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `02` | 210 | 0 | 0 | 0 | 0 | PASS |
| `03` | 226 | 0 | 0 | 0 | 0 | PASS |
| `04` | 236 | 0 | 0 | 0 | 0 | PASS |
| `05` | 245 | 0 | 0 | 0 | 0 | PASS |
| `06` | 255 | 0 | 0 | 0 | 0 | PASS |

**B-off baseline: 5/5 PASS** following previous close-range sensitivity GATT control 5/5 PASS (docs/TASK012-LIVE-GATT-SENSITIVITY.md). C's total decoded-frame count remained constant at 7509 across the five test intervals, so it did not see any decoded mesh traffic at all; this is consistent with isolation but does not independently prove the C BLE scanning subsystem was healthy throughout. Successful C GATT reads demonstrate device/application responsiveness, not scan RX liveness. Require positive B-on restoration and C GATT ACCEPT delta / matching sequence/source/TTL2 with A/C geometry unchanged before accepting a true B-dependent two-hop RF relay. The RF separation is empirical, not calibrated or guaranteed over time; uncharacterized interferers/geometry remain limitations.

Next physical assistance: leave Kit1 and Kit3 fixed. Power on Kit2/B by reconnecting USB and position it approximately between the two nodes or where it is within BLE reach of both; keep B accessible to PC. Check B's original serial COM15 and BLE address C8:51:08:6E:08:C0. B remains on Task010 topology firmware, matching packet format but without Task012 GATT evidence; use B on-air TTL2 and console diagnostics plus C's Task012 evidence characteristic. First attempt smoke relay without flashing B; user approval would be necessary for any changed B firmware. No flash, resets or physical power actions by PC were used in this baseline.
