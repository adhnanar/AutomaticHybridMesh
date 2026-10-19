# Task009 heartbeat deployment and independent hardware retest

Date: 2026-10-18

## Firmware deployment
User approved heartbeat diagnostic UF2 for Kit2 and Kit3 only. Kit1 was deliberately left unchanged. Kit3 entered UF2 with COM16 absent, COM14/15 still connected, F: XIAO-SENSE with Board-ID Seeed_XIAO_nRF52840_Sense verified. Verified source UF2 SHA256 19718FE4F08BDF2665AED4726867E4727982438EED71237167FA4CC4609BC189, 270848 bytes, copied successfully to F:\zephyr.uf2. Original Kit3 COM16 serial B13E025E5A1407EE recovered; all 3 COM ports present.

## Idle console test (no BLE GATT)
Ran tests/task009_heartbeat_observe.py, opened COM15 and COM16 for 60 seconds. Logs docs/task009-b-heartbeat-idle.log and docs/task009-c-heartbeat-idle.log, summary docs/task009-heartbeat-idle-summary.log. Each yielded only a 64-byte Zephyr boot banner; 0 complete heartbeat records. Exit 0 (capture script); console integrity cannot establish a healthy trace.

## Live acceptance with both consoles
Ran tests/task009_capture_both_consoles.py with 25-second scan; logs docs/task009-hb-live-acceptance.log and docs/task009-b-diagnostic-console.log, docs/task009-c-diagnostic-console.log. A GATT write and echo PASS message HOP009DD6A7E59; A E5 over-air message source eb34ed25a5a4 sequence 288 TTL3 observed. Matching B TTL2 not observed by PC scanner; C GATT TX read `Ready` FAIL; PC acceptance exit 1. B and C USB console heartbeat strings were merged and truncated. The incomplete strings show decoded cumulative counts from 10 to 12 and text=0 for both roles; treat these as indications only, not intact validated records. Integrity reported B heartbeats=0, damaged=2; C heartbeats=0, damaged=4. Both lack correlation of an RX stage to A seq288. Console transport is still lossy. No confirmed end-to-end BLE delivery.

## Next work
Investigate why A TTL3 is visible to PC but B/C count no text frames; differentiate short advertisement hold, connection-driven advertising interruptions, and scan callback decode/transport from a real RF miss. Capture well-formed counters/telemetry without relying on USB CDC printk. Review BLE legacy advertising payload size and scanner filter/cross-node behavior, and avoid connecting to C during relay observation. Do not flash modified firmware without renewed explicit approval.
