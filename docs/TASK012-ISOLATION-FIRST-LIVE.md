# Task012 physically separated Kit1-to-Kit3 isolation: first baseline FAILED

Date: 2026-10-19. User indicated A and C were separated into different locations while keeping USB attached and B off. Verified only Kit1 COM14 (USB serial 1D5EA4A525ED34EB) and Kit3 COM16 (B13E025E5A1407EE) connected. Task012 GATT evidence firmware on both; B physically off.

## Test method
`tests/task012_live_isolation.py` reads C GATT evidence before/after invoking `tests/ble_pc_controlled_two_hop.py --mode baseline` with unique injected A message and 20s PC scan. Offline evidence check requires fresh PC-observed A TTL3 frame, B absent, C unchanged GATT text and zero deltas in C decoded TEXT, rejected, accepted and duplicate counters. No USB console integrity requirement, as Task012 binary GATT evidence replaces lossy printk logs. Logs: `docs/task012-isolation-live-01.log`.

## Observed result: FAIL (direct RF path persists)
A sent `HOP011074A61E2`, PC observed source eb34ed25a5a4 sequence 143 TTL3, A GATT echo PASS. C GATT TX stayed `Ready`, PC B-off baseline PASS (B absent). C evidence BEFORE: decoded 5384, text 78, reject 78, accept 0, last seq85 TTL3 REJECT; AFTER: decoded 5837, text 101, reject 101, accept 0, last seq143 TTL3 REJECT source tail 25a5a4. Delta decoded +453, TEXT +23, REJECT +23, ACCEPT +0. `ISOLATION_RESULT=FAIL_C_DETECTED_TEXT`, test process exit2. The distance/obstruction used in this setup did not isolate A from C. This **does not invalidate** Task010's prior 2/2 BLE relay acceptance, nor the Task012 5/5 proximity sensitivity controls. No further trials at same arrangement are warranted until changing separation/attenuation.

## Next action
Ask user to further separate A and C, place additional walls/attenuating barriers, or use reliable RF shielding while keeping USB and PC BLE reachable. Keep B off. Then rerun B-off negative control and obtain five clean no-C-text-delta trials. Do not claim isolation based merely on C refusing TTL3 frames; strict GATT read counters show direct reception. For later B-on relay test, maintain A/C positions and ask user to power B in a bridging position; B currently runs Task010 build, so consider compatibility and only flash changed firmware with explicit approval.
