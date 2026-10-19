# Task012 — Physically separated BLE A→B→C relay acceptance

Date: 2026-10-19. **Status: PoC-level isolated two-hop BLE relay acceptance PASS (5/5 B-off controls and 5/5 B-on relay trials), with limitations below.**

## Hardware and software
User manually powered on Kit2/B and positioned it to bridge fixed Kit1/A and Kit3/C after B-off baseline tests. PC verified USB serials: A COM14 `1D5EA4A525ED34EB`, B COM15 `224BC825E1DA130C`, C COM16 `B13E025E5A1407EE`. Phone not connected or used. A and C have user-approved Task012 topology UF2 `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`; B remains on Task010 topology firmware. No new firmware flash, reset, or PC-triggered power change during relay trials. Radio addresses A `E5:12:76:D2:56:EA`, B `C8:51:08:6E:08:C0`, C `E2:DB:6A:54:BB:71`.

## Positive/negative controls
1. **Direct-reception sensitivity 5/5 PASS**, B off with A/C near each other: C GATT evidence recorded A-source TTL3 REJECT with matching sequences and acceptance delta 0 for every message. Details `docs/TASK012-LIVE-GATT-SENSITIVITY.md`.
2. **A/C separation baseline 5/5 PASS**, B physically off, A/C in same *fixed positions subsequently used for relay*: PC independently observed five A TTL3 messages, but C GATT evidence decoded-text/reject/accept/duplicate deltas were all zero and C stored GATT payload stayed unchanged. Sequences 210, 226, 236, 245, 255. Details `docs/TASK012-ISOLATION-FIVE-TRIALS.md`. Previous, less-separated geometry failed because C directly decoded 23 A text packets. A/B/C geometry is user-confirmed, not measured or calibrated.
3. **B on in bridging position: 5/5 message delivery and matching relay acceptance**. PC observed A TTL3 and B TTL2 source/sequence/payload equality, C stored GATT TX equaled injected unique payload and its binary evidence recorded +1 ACCEPT at A source/seq TTL2, 0 REJECT deltas and repeated DUPLICATE events.

## B-on relay trial evidence
| PC log | Sequence | PC A TTL3/B TTL2 | C ACCEPT delta | C REJECT delta | C DUPLICATE delta | C GATT exact payload | Verdict |
|---|---:|---|---:|---:|---:|---|---|
| `docs/task012-relay-live-01.log` | 326 | both match | +1 | 0 | +10 | `HOP01138992D24` | PASS, independently re-evaluated |
| `docs/task012-relay-live-02.log` | 346 | both match | +1 | 0 | +9 | PASS | PASS |
| `docs/task012-relay-live-03.log` | 357 | both match | +1 | 0 | +11 | PASS | PASS |
| `docs/task012-relay-live-04.log` | 368 | both match | +1 | 0 | +9 | PASS | PASS |
| `docs/task012-relay-live-05.log` | 378 | both match | +1 | 0 | +10 | PASS | PASS |

The first positive run's extra PC-only wrapper erroneously required C's *last* GATT event to be ACCEPT, when the record legitimately held DUPLICATE after acceptance. It printed a failing evidence verdict despite PC acceptance PASS and C +1 accepted message, zero rejects, exact source/seq/TTL2 and correct stored payload. The wrapper was corrected to allow last ACCEPT or DUPLICATE, and trial 01 evidence was independently examined from the saved log and judged PASS. Runs 02–05 completed with corrected validator and zero reported failures.

## Independent verification and acceptance limits
- `python -m unittest discover -s tests -p test_*.py -q`: **31 tests PASS, exit0**.
- `py_compile` for live sensitivity, isolation and relay wrappers: **PASS**.
- Prior Task011 stricter B-console completeness gate was not satisfied on unreliable USB CDC stdout and is not asserted here. B's PC-observed BLE advertisement TTL2 is independent relay-forward evidence; C's own binary GATT accepts at TTL2 and no TTL3 rejections strengthen the pathway. C's received advertiser MAC is not captured on device and packets are not authenticated. A third radio or malicious forwarder is excluded only by the controlled three-board setup, not cryptographically. RF separation is empirically supported by five negative controls at this arrangement, not guaranteed under all environmental conditions.
- Task012 evidence GATT counters/last-event snapshots are stateful and can be overwritten by other traffic; controlled unique payloads/seq and repeat tests used. The 20-byte record is checksummed, not tamper-proof.
- BLE three-board *PoC-level* relay-dependence milestone achieved. **LoRa routing, hybrid switching, direct phone notification, battery usage, scale and outdoor range remain untested.**
- No more physical assistance required for this milestone. Next engineering stage should plan LoRa and BLE hybrid integration with firmware build/test gates and explicit user flash authorization.

## Conclusion
The project has demonstrated: A→C direct BLE reception with A/C close, B-off inability to deliver when A/C separated, and B-on restoration of delivery over BLE with five distinct messages at the same A/C positions. **Isolated two-hop BLE mesh relay PoC: PASS within the controlled test geometry.**
