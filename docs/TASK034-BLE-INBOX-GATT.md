# Task034 — BLE inbox GATT + mesh RX integration (PC-only, opt-in)

Date: 2026-10-19. Status: **implemented, statically checked; NOT compiled for Zephyr, C tests NOT executed.**

## Scope
Integrate the Task033 bounded inbox with the real accepted mesh RX path and a read-only GATT characteristic for nRF Connect. `CONFIG_BLE_INBOX` stays default `n`.

## Design
- **UUID** `7B3E0006-6F2A-4C91-9A3D-2F5B6C7D8E00`, read-only, in the existing hybrid service. Collision check: grep of src/tests/docs found 0001–0005 only (RX/TX/Discovery/Evidence + service); 0006 appeared only in the Task033 proposal text.
- **Recorded data:** source ID (6 bytes) + accepted text (<=14 bytes) + seq, in `process_received_packets` ([ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c)) immediately next to `ble_gatt_publish_text_message`, i.e. after the local-source drop (`continue`), topology filter/reject/duplicate, dedup, and the `type == TEXT` check. Local phone-written echo (`write_rx`) and DISCOVER/control frames never reach the inbox.
- **Locking:** one `k_mutex` ([ble_inbox_service.c](../src/features/ble_inbox/ble_inbox_service.c)) guards push, snapshot take and snapshot clear.
- **Torn Read Blob prevention:** per-connection frozen fixed-size snapshot (`CONFIG_BT_MAX_CONN` x 190 B). Read at offset 0 freezes a new snapshot; offset>0 serves the frozen bytes with no inbox access; offset>0 with no snapshot or beyond length -> ATT Invalid Offset. Snapshot cleared on disconnect. Pure-C policy in [ble_inbox.c](../src/features/ble_inbox/ble_inbox.c) (`ble_inbox_snapshot_*`).
- **Image format** (unchanged from Task033): `ver, count, dropped(LE16)`, then newest-first `{seq LE16, src[6], len, text}`; max 188 B (static-asserted <=255).
- **Default build:** the service definition is duplicated under `#if CONFIG_BLE_INBOX / #else`; the `#else` branch is the original text, all other hooks are `#if`-guarded, sources only added via `target_sources_ifdef`. Preprocessed default code should be unchanged.

## Limits (not claimed)
Android nRF Connect must manually Read (Read Blob at MTU 23); no notifications, no background/status-bar alerts. RAM only, lost on reset; unauthenticated; no delivery/ack guarantee; max 8 messages, oldest overwritten (`dropped` counter saturates). A client that restarts at offset 0 mid-sequence gets a fresh snapshot (its earlier blob sequence is abandoned). Two clients never interfere (separate slots). `bt_conn_index` slot reuse relies on the disconnect clear.

## Files changed
- Modified: [Kconfig](../Kconfig), [CMakeLists.txt](../CMakeLists.txt), [ble_gatt.c](../src/features/ble_gatt/ble_gatt.c), [ble_mesh_relay.c](../src/features/ble_mesh_relay/ble_mesh_relay.c), [ble_inbox.c](../src/features/ble_inbox/ble_inbox.c)/[.h](../src/features/ble_inbox/ble_inbox.h), [tests/ble_inbox/src/main.c](../tests/ble_inbox/src/main.c), [tests/test_task033_inbox_static.py](../tests/test_task033_inbox_static.py) (replaced obsolete "no GATT reference" check with guard/UUID/record-point/mutex checks).
- New: [ble_inbox_service.c/.h](../src/features/ble_inbox/ble_inbox_service.h).

## Commands and actual results
- `python -m unittest discover -s tests -p 'test_*.py' -q` (system Python): **55/55 OK** (was 52; +3 net from rewritten static checks).
- `python tests/task024_static_tx_check.py`: **PASS**.
- `C:\zephyrproject\.venv\Scripts\python.exe -m west build ...` with process-local `ZEPHYR_BASE`: **BLOCKED** — sandbox reports the venv python "not recognized" (file exists; same as Task032/033 agent runs). System Python has no `west`; no arm-gcc/gcc on PATH. So: default and `EXTRA_CONF_FILE=tests/task033_inbox_optin.conf` builds, native_sim ztest (3 new snapshot tests written), and the Task012 UF2 hash (`2575E713…1F14`) are **NOT verified**.
- No flash/reset/RF/LoRa/USB/install/stage/commit.

## Issues / risks for review
Unverified compile of `ble_inbox_service.c`, the duplicated service macro and `K_MUTEX_DEFINE` use; ATT callbacks hold a mutex with `K_FOREVER` (same pattern as existing reads). Must reproduce both builds and compare default UF2 hash independently.

## Rollback
Revert the files above (or simply leave `CONFIG_BLE_INBOX` off, the default); delete the two new service files.

## Next action
Independent review: run default + opt-in west builds, ztest on native_sim, hash check. No hardware use without authorization.
