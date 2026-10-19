# Task012 live GATT evidence and direct-path sensitivity controls

Date: 2026-10-19. User explicitly approved flashing Task012 topology UF2 to Kit1/A and Kit3/C only; Kit2/B kept physically off. Verified both UF2 bootloaders and restored original serial identities (A COM14 `1D5EA4A525ED34EB`, C COM16 `B13E025E5A1407EE`). UF2 SHA256 `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8` identical on both deployments. GATT evidence characteristic `7b3e0005-6f2a-4c91-9a3d-2f5b6c7d8e00` read/decoded successfully on A and C, role/checksum valid; no phone used.

## Test setup and method
With Kit2 OFF and A/C near each other, `tests/task012_live_sensitivity.py` performs a read-only C GATT evidence snapshot, invokes `tests/ble_pc_controlled_two_hop.py --mode baseline` for a unique A text frame with 20 s BLE scan, then reads C GATT evidence and validates direct rejection via `task012_evidence.sensitivity_failures`. Stored logs: `docs/task012-sensitivity-live-01.log` through `-05.log`. Each baseline observed A TTL3 matching source, sequence and payload, C GATT TX stayed `Ready`, and B absent. On run 1, script falsely printed inconclusive because of a test wrapper regex escape error, not a device failure; independently reparsed original saved log and ran `sensitivity_failures` with snapshots: zero failures, A sequence36. Corrected wrapper before runs 2-5.

## Independent outcomes
| Run | A sequence TTL3 | C reject delta | C accept delta | C last event/source/TTL | Verdict |
| --- | ---: | ---: | ---: | --- | --- |
| 01 | 36 | 18 | 0 | REJECT, A tail 25a5a4, TTL3 | PASS (re-evaluated saved GATT/PC evidence) |
| 02 | 55 | 15 | 0 | REJECT, A tail 25a5a4, TTL3 | PASS |
| 03 | 67 | 14 | 0 | REJECT, A tail 25a5a4, TTL3 | PASS |
| 04 | 76 | 16 | 0 | REJECT, A tail 25a5a4, TTL3 | PASS |
| 05 | 85 | 15 | 0 | REJECT, A tail 25a5a4, TTL3 | PASS |

**5/5 direct-path sensitivity controls PASS.** This validates that C hears the direct A message in close proximity and correctly rejects it under topology policy. It does NOT establish A/C RF isolation or prove B-dependent relaying after physical separation. Task010 prior 2/2 three-board relay successes remain on earlier firmware; B was deliberately not flashed with Task012 and is off. Next requires user-assisted placement and a B-off A-to-C RF isolation baseline with same GATT before/after instrumentation, then positive B-on with unchanged A/C placement and future B firmware approval if needed. No additional flash performed.
