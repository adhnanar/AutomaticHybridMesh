# Task010 scan duplicate-filter root cause and heartbeat hardening

Date: 2026-10-18. No hardware flashed, reset or opened.

## Root cause (source-grounded)
1. `ble_mesh_relay_init` started the scan with `BT_LE_SCAN_PASSIVE_CONTINUOUS`, which sets `BT_LE_SCAN_OPT_FILTER_DUPLICATE` (`zephyr/include/zephyr/bluetooth/bluetooth.h`). The host forwards it as `filter_dup` (`host/scan.c`).
2. The Zephyr LL filter (`controller/hci/hci.c`: `dup_found`, `is_dup_or_update`) matches on advertiser address + PDU type bit only. For legacy advertising it returns "duplicate" for every later ADV_IND (`else if (adv_type != PDU_ADV_TYPE_EXT_IND) return true;`). Payload is never compared. The table has `CONFIG_BT_CTLR_DUP_FILTER_LEN`=16 entries, round-robin evicted, and is cleared only when scanning is (re)enabled.
3. Every node advertises its beacon (address fixed, ADV_IND) and later swaps the same set's data to a TEXT frame. Once B/C had seen A/B's beacon, the TEXT ADV_IND from the same address was dropped inside the controller, before `scan_receive`. This matches `decoded` rising slowly (10 -> 12, only via table eviction/new addresses) while `text=0`, and the PC (no LL filter) seeing A TTL3. Nodes never reach the code that logs RX_ACCEPT, so B never relays TTL2 and C never gets the message.
4. 500 ms hold vs scan: scan window equals interval (continuous, 30 ms), adv interval is <=60 ms, so 500 ms already gave ~8+ events. The hold is not the primary cause; it was raised to 1000 ms (>=10 events at the max interval) for margin only.
5. USB CDC loss (carried from TASK009-CONSOLE-LOSS-INVESTIGATION): `cdc_acm_poll_out` drops bytes when the TX buffer is full and flow control is off. Not provable without hardware; mitigation is to make each heartbeat one atomic USB packet.

## Changes (feature `ble_mesh_relay`)
- New [ble_mesh_scan_policy.h](../src/features/ble_mesh_relay/ble_mesh_scan_policy.h): scan params without duplicate filter; hold/interval constants. Used by [ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c) (scan start, hold).
- New [ble_mesh_heartbeat.c/.h](../src/features/ble_mesh_relay/ble_mesh_heartbeat.h): compact `HB r=B n=1 d=2 t=0 f=0 q=0 u=5000 c=XX` line (<64 B, XOR checksum) replacing the long heartbeat printk.
- [task009_console_integrity.py](../tests/task009_console_integrity.py) parses/validates the compact format and the legacy one; checksum mismatch counts as damaged.
- Tests: 3 ztests added in [tests/ble_mesh_packet/src/main.c](../tests/ble_mesh_packet/src/main.c) (no duplicate filter, hold covers >=10 adv events, heartbeat fits 64 B + checksum); 1 Python test.
- CMake lists updated (app and ztest).

## Commands and results
- `python -m unittest discover -s tests`: 16/16 PASS.
- Clean `west build -p always` xiao_ble/nrf52840: topology (`build-task009-scanfix-topology`, B_ID_0C13) PASS, FLASH 135336 B, RAM 30472 B, UF2 270848 B, SHA256 `2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773`; default (`build-task009-scanfix-default`) PASS, FLASH 133472 B; ztest app (`build-task009-scanfix-ztest`) PASS compile/link.
- NOT verified: ztests were not executed (native_sim cannot be configured on this Windows host; no host gcc/cl). No RF evidence.

## Issues
- Fix is code-grounded, not hardware-confirmed. Another pitfall: unfiltered scanning raises scan-callback rate; rx queue (8) drops are counted via `q=`.

## Rollback
Revert the scan start to `BT_LE_SCAN_PASSIVE_CONTINUOUS`, hold to 500 ms, restore old heartbeat printk, delete the three new files and test additions.

## Next action (needs explicit approval)
Flash `build-task009-scanfix-topology/zephyr/zephyr.uf2` to Kit1/A, Kit2/B and Kit3/C (A also scans and the same filter affects any node receiving relays; all three should match). Then idle 60 s with compact-HB integrity check, and rerun acceptance without connecting to C during the relay window.
