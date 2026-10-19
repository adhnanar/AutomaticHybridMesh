# TASK033 — BLE inbox history foundation (PC-only)

## Scope
Foundation for seeing the last few received short text messages after a temporary phone disconnect (nRF Connect, ATT MTU 23). Pure-C bounded ring buffer plus a defined read image. **Not** wired to the receive path and **no GATT characteristic added** (deliberately deferred, see below). Opt-in `CONFIG_BLE_INBOX`, default `n`.

## Files changed/added
- [src/features/ble_inbox/ble_inbox.h](../src/features/ble_inbox/ble_inbox.h), [ble_inbox.c](../src/features/ble_inbox/ble_inbox.c): module (no Zephyr dependency, no allocation).
- [tests/ble_inbox/](../tests/ble_inbox) : ztest suite (6 tests, native_sim) — **written, not executed**.
- [tests/test_task033_inbox_static.py](../tests/test_task033_inbox_static.py): 4 static guards.
- [Kconfig](../Kconfig): `BLE_INBOX` (default n). [CMakeLists.txt](../CMakeLists.txt): `target_sources_ifdef(CONFIG_BLE_INBOX ...)`.
- docs/PROGRESS.md entry.

## Design
- 8 slots × (seq u16, source_id[6], len u8, text[≤14]) = 23 B each; RAM ≈ 190 B. Text max 14 equals `BLE_MESH_MAX_TEXT_LEN`; source ID is the 6-byte mesh node ID of the packet source.
- **Ordering:** read image is newest first, so the first 22-byte read already shows the latest message. `seq` is a per-boot u16 counter assigned on push (wraps at 65535; compare modulo 2^16).
- **Overflow:** when full the oldest entry is overwritten and `dropped` (u16, saturating) increments. No blocking, no error.
- **Invalid input** (null, empty, >14 bytes) returns `-EINVAL`; nothing stored, never truncated.
- **Reset:** RAM only. Reboot/`ble_inbox_init` clears entries, `seq` and `dropped`; no persistence. A `seq` restarting at 0 after a reboot signals loss of history to the reader.
- **Read image (v1):** `[0]=1 version, [1]=count, [2..3]=dropped LE`, then `count × {seq LE u16, source[6], len, text[len]}`. Max 188 B (< 512 B ATT attribute limit).
- **Offsets / MTU 23:** `ble_inbox_read(inbox, offset, out, cap)` renders and copies from `offset`; Read Blob in 22-byte chunks reassembles to the exact full image (tested). Offset ≥ length returns 0. The image is rendered per call, so if a push happens between chunked reads the reader may see a torn image; clients should re-read from offset 0 if `seq` of the first record or `count/dropped` changed, or if parsing fails. Callers must serialise push/read (single mutex); the module has no locking.
- **Text content** is raw bytes, not validated as UTF-8 or printable.

## Proposed (not implemented) GATT integration
A read-only characteristic in the existing service with new UUID `7b3e0006-6f2a-4c91-9a3d-2f5b6c7d8e00` (0001–0005 are in use), read callback calling `ble_inbox_read` under a mutex, `BT_GATT_PERM_READ` only. Hooking push into the RX path touches `ble_gatt.c`/`main.c` and the baseline; held back to keep default 274/274 and Task012 hashes safe and for separate review. Notes: Android nRF Connect Read issues Read Blob for values > MTU-1 automatically.

## Security / limits
- **No acknowledgement, no delivery guarantee:** an entry means "this node received/accepted a message", not that the sender's or any other node's copy arrived; messages missed while the node was off, out of range, or deduped are never visible.
- No authentication or encryption of the inbox: the existing GATT permission model applies, so any connected central could read it (once exposed); `source_id` is unauthenticated and spoofable.
- No Android background notifications; the user must reconnect and read.
- Bounded to 8 messages; older ones are lost. RAM-only history is lost on reset/power loss.

## Commands executed and actual results
- `python -m unittest discover -s tests -p 'test_*.py' -q`: **52/52 PASS** (48 prior + 4 new).
- `python tests/task024_static_tx_check.py`: **PASS**.
- `C:\zephyrproject\.venv\Scripts\python.exe -m west`: **BLOCKED** — path not resolvable in this agent's sandbox. No firmware build (default 274/274, opt-in, or ztest) performed.
- Zephyr SDK ARM gcc syntax check: **BLOCKED** (executable not resolvable). The C module and ztest suite are therefore **uncompiled and unexecuted**; I hand-checked offsets in the tests against the layout.
- Default build is unchanged by construction (new Kconfig default n; source only added when enabled), but the 274/274 and Task012 UF2 hash reproducibility were **not re-verified**.

## Reviewer actions requested
1. With venv Python: `west build -b xiao_ble/nrf52840 -d <dir> . --pristine=always` default (expect 274/274, UF2 SHA256 `2575E713…1F14`); again with `-DCONFIG_BLE_INBOX=y` (should link-gc the unused code); and `west twister -T tests/ble_inbox -p native_sim` (or ztest build used previously).
2. Review the C for off-by-one and tearing behavior.

## Issues
Compilation and test execution blocked as above. 

## Rollback
Delete `src/features/ble_inbox`, `tests/ble_inbox`, `tests/test_task033_inbox_static.py`, and revert the `BLE_INBOX` lines in Kconfig and CMakeLists.txt.

## Next action
Independent review/build; then a separate task for the GATT read characteristic and RX-path hook. No flash, reset, USB, power, RF/LoRa TX, install/download, or git stage/commit was done.
