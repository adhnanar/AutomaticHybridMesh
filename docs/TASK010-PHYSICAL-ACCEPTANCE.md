# Task010 physical three-kit BLE mesh acceptance — PASS

Date: 2026-10-18

## Approved firmware deployment
User explicitly approved Task010 scan duplicate-filter fix on all three XIAO nRF52840 Sense kits, and individually double-reset each kit. Verified each F: XIAO-SENSE UF2 bootloader, Board-ID Seeed_XIAO_nRF52840_Sense; copied `build-task009-scanfix-topology/zephyr/zephyr.uf2` (270848 bytes, SHA256 `2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773`) successfully to each. Post-flash USB reconnects validated for A COM14 serial 1D5EA4A525ED34EB, B COM15 serial 224BC825E1DA130C, C COM16 serial B13E025E5A1407EE.

## Test method
Used `tests/task009_capture_both_consoles.py`, which invokes `tests/ble_pc_controlled_two_hop.py` with A BLE E5:12:76:D2:56:EA, B BLE C8:51:08:6E:08:C0, C BLE E2:DB:6A:54:BB:71 and 25 s scan. PC sends unique message to A GATT, verifies A echo, scans on-air A TTL3 and B TTL2 same original source/sequence/payload, then reads C TX via GATT to confirm exact message. Simultaneous USB console capture on COM15/16 monitors receive diagnostics. A directly received frame alone is not sufficient.

## Results
1. **PASS**: evidence `docs/task010-live-acceptance-first.log`. GATT write/echo PASS `HOP009C459BA42`; PC observed A TTL3 source eb34ed25a5a4 sequence 52 and B TTL2 same source/sequence/payload; C GATT TX `HOP009C459BA42` PASS; `CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS`, runner exit0. B valid heartbeat sample n=31 decoded=2845 text=3; C heartbeat n=11 decoded=1345 text=34; C console contains RX_ACCEPT substrings but long diagnostic lines interleaved/corrupted. Final integrity: B 7 complete heartbeats, missing indices 2, damaged topology lines 2; C 6 complete, missing 2, damaged 1.
2. **PASS**: evidence `docs/task010-live-acceptance-second.log`. GATT write/echo PASS `HOP0094B70A7C9`; PC observed A TTL3 source eb34ed25a5a4 sequence 68 and B TTL2 same source/sequence/payload; C GATT TX `HOP0094B70A7C9` PASS; `CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS`, runner exit0. Final integrity: B 8 heartbeats, missing 1, damaged 1, last_text 46; C 7 heartbeats, missing 2, damaged 1, last_text 58.

## Interpretation and remaining limitations
The controlled **three-board two-hop BLE text relay PoC acceptance has passed in 2/2 repeated live runs**. Task010 duplicate-filter change is strongly supported as the earlier missing-TEXT root cause: earlier B/C heartbeat text=0 despite other decoded frames, now their decoded text counts increase and B TTL2/C delivery are externally verified. This is a PoC milestone, not proof of reliability at scale, absence of nearby interference, BLE direct-path exclusion, or completion of LoRa/hybrid routing/Android notification. The controlled acceptance proves B relayed on-air and C stored the message; C may also hear A's direct frame in this co-located setup, and log fragments are not adequate to certify all C receive paths independently. USB CDC console still drops/interleaves long printk lines even though compact heartbeats provide parsable evidence. Continue isolation/distance tests, GATT notification/UI discovery, and LoRa routing later. No additional firmware flash performed after these tests.
