# Task012 — C console evidence loss: analysis and console-independent evidence

Date: 2026-10-19. Scope: explain why the Task011 C USB CDC console lost HB/RX_REJECT lines, add an in-firmware observable that survives console loss, build (not flash) and test. **No flash, reset, erase or power action was performed.**

## Verdict on Task011
Still INCONCLUSIVE. Missing C prints are **not** evidence that C did or did not receive A's TTL3 seq312, and RF isolation is **not** claimed either way ([TASK011-FIRST-SENSITIVITY-LIVE.md](./TASK011-FIRST-SENSITIVITY-LIVE.md)). Task010 A→B→C routing is untouched (routing/filter code unchanged; only counter calls added).

## Root cause analysis (from source and build config)
- Console is `CONFIG_UART_CONSOLE` on the Zephyr USB CDC ACM (device_next, `USBD_CDC_ACM_CLASS`, TX delay 100 ms) — see build `.config`. `printk` is a synchronous byte-by-byte `uart_poll_out`.
- In [usbd_cdc_acm.c](C:/zephyrproject/zephyr/subsys/usb/device_next/class/usbd_cdc_acm.c) `cdc_acm_poll_out` **discards bytes** ("No TX buffer available, discard data") when the TX ring buffer is full and flow control is off (default) or when called from ISR. Discard is silent (one `LOG_WRN_ONCE`, and logging is not enabled in this app). A line cut mid-way and the next line appended yields exactly the truncated/merged `RX_REJECT` fragments seen.
- Ring drains only when the host reads bulk IN packets (64 B full-speed) via the USB work queue; BLE scan/adv load, the 100 ms first-TX delay, host (PC) read stalls or serial-tool reopen all let the ring fill. Bursts (RX_REJECT ×N per duplicate advertisement, since scan duplicate filtering is off) plus 5 s HB are enough to overflow.
- The existing `HB` line is checksummed and counter-based, which detects loss but cannot restore it.
- Hence a text console is inherently lossy; I do not claim the exact byte-level overflow instant (not measured on hardware).

## Solution chosen (minimal, evidence-preserving)
Feature `ble_mesh_relay/ble_mesh_evidence` — cumulative counters and last-text-event metadata exposed by a **read-only GATT characteristic** `7b3e0005-6f2a-4c91-9a3d-2f5b6c7d8e00` (service unchanged). Properties: no console dependence, no flooding (state, not events; O(1) RAM), polled by the existing PC BLE path, wrap-safe deltas, XOR checksum, no new threads. Console printing is left as is (not increased).

20-byte record (little endian): `0 ver=1 | 1 role | 2 decoded u16 | 4 text u16 | 6 reject u16 | 8 accept u16 | 10 duplicate u16 | 12 last_seq u16 | 14 last_ttl | 15 last_event (1 REJECT,2 ACCEPT,3 DUP) | 16..18 last source id bytes 3..5 | 19 XOR(0..18)`. Source `eb34ed25a5a4` → tail `25a5a4`.

Limitation: counts only frames that pass the scan-callback decode; it proves *C's firmware saw and rejected* an A frame, not RF isolation generally. Counters are not updated for self frames. Last-* shows only the most recent text event, so reads must bracket a single injection (before/after) and no other traffic.

## Files changed
- New: [ble_mesh_evidence.c/.h](../src/features/ble_mesh_relay/ble_mesh_evidence.h), [task012_evidence.py](../tests/task012_evidence.py), [test_task012_evidence.py](../tests/test_task012_evidence.py), this doc, [docs/copilot/012-hardware-verification.prompt.md](./copilot/012-hardware-verification.prompt.md).
- Edited: [ble_gatt.c/.h](../src/features/ble_gatt/ble_gatt.h) (evidence characteristic + provider hook), [ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c) (counter calls beside the existing RX_REJECT/ACCEPT/DUPLICATE paths and decode), [CMakeLists.txt](../CMakeLists.txt), [tests/ble_mesh_packet/CMakeLists.txt](../tests/ble_mesh_packet/CMakeLists.txt) and ztest [main.c](../tests/ble_mesh_packet/src/main.c) (3 new ZTESTs: layout/checksum, 300-event burst, bad arguments).

## Commands and actual results
- `python -m unittest discover -p "test_*.py"` (tests/): 31 tests OK (27 prior + 4 new).
- `west twister -p native_sim` and `west build -b native_sim`: **not runnable** here (Windows Python multiprocessing pipe error / no host `gcc`). ZTESTs therefore are **compiled and linked only, not executed**: `build-task012-ztest` (xiao_ble/nrf52840) exit 0, FLASH 82660 B.
- Clean firmware, new dirs, deployed artifacts untouched: `build-task012-default` exit 0 (FLASH 134072 B, RAM 30472 B, UF2 268288 B); `build-task012-topology` (`TEST_TOPOLOGY=y`, `B_ID_0C13=y`) exit 0 (FLASH 136108 B, RAM 30536 B, UF2 272384 B, SHA256 `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`).
- Not verified: on-air behavior, GATT read of the new characteristic, Task010 regression on hardware.

## Issues / rollback
- ZTEST execution unavailable on this host (use CI/Linux for native_sim). Adding a characteristic shifts GATT handles; phone/PC tools use UUIDs, but an nRF Connect cache may need a refresh.
- Rollback: delete the evidence module files, revert the edits above; previously flashed Task010 UF2 (SHA `2FFAA52A…7D773`) remains the deployed firmware.

## Proposed hardware verification plan (needs NEW explicit user approval to flash)
1. Approval gate: user approves flashing **only Kit1/A (COM14) and Kit3/C (COM16)** with `build-task012-topology` UF2 (SHA above). B stays physically OFF (it need not be updated for this control; for later A→B→C runs all three must carry the same build).
2. Per kit: double-tap reset to UF2, verify `INFO_UF2` Board-ID `Seeed_XIAO_nRF52840_Sense`, copy `zephyr.uf2`, confirm original serials (A `1D5EA4A525ED34EB`, C `B13E025E5A1407EE`) return. Roles stay valid only because B's ID config is `0C13`, as in Task010.
3. Smoke: scan; read `7b3e0005…` on both via `python tests\task012_evidence.py <ADDR>`; expect version 1, roles A/C, checksum OK, counters increasing slowly (decoded grows from beacons).
4. Regression with B ON for Task010 (`ble_pc_controlled_two_hop.py`, 2 runs) must still PASS and C's evidence `accept` must increase with last_ttl=2.
5. Sensitivity control, B physically OFF, A/C close: read C evidence (R0, no concurrent traffic); inject one unique A TTL3 message via `ble_pc_controlled_two_hop.py --mode baseline --control sensitivity`; wait ≥12 s; read C evidence (R1, new connection to C only after A's advert phase).
6. Pass = `sensitivity_failures(R0, R1, "25a5a4", seq)` returns `[]` (reject +≥1, accept +0, last event REJECT, tail `25a5a4`, seq = A's sequence, TTL 3). Repeat ×5 as Task011 required. Fail/unreadable = inconclusive; do not infer isolation. Keep console capture as secondary evidence only.
7. Report each run in a new Markdown file and update PROGRESS.md; stop and ask before any further flash.
