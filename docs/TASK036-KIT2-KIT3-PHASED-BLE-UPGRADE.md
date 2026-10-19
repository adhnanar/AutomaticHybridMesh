# TASK036 – Phased Kit3 → Kit2 BLE firmware update candidate (PC-only, NOT flashed)

## Scope
Design and prepare (not deploy) one general-policy BLE candidate image for Kit3/C and then Kit2/B, with opt-in inbox, Android discovery name and no-echo DISCOVER. Kit1 LoRa RX image is untouched. **No** flash, reset, USB/serial, hardware, power, RF or LoRa TX operation; no install/download; no git stage/commit.

**Build status: NOT BUILT – BLOCKED by sandbox.** `C:\zephyrproject\.venv\Scripts\python.exe` is "not recognized" in this sandbox (file exists; same block as Task032/033/034 agent runs); system Python 3.12 has no `west` (`No module named west`), also with the venv `site-packages` on `PYTHONPATH`. No retry through other routes. Therefore **no candidate UF2 and no SHA256 exist**; the hash table below lists only pre-existing artifacts. The independent reviewer must build (command in §6).

## Files changed
- `tests/task036_kit2_kit3_candidate.conf` (new, 3 lines, see §2)
- `docs/TASK036-KIT2-KIT3-PHASED-BLE-UPGRADE.md` (this), `docs/PROGRESS.md` entry.
- No `src/`, Kconfig, CMake or prj.conf change. A failed build log was deleted (no artifacts remain).

## 1. Source / role findings (read from code, not from docs)
| Fact | Evidence |
|---|---|
| Deployed topology images only let role **A** (`eb34ed25a5a4`, Kit1) originate text | `ble_mesh_topology_can_originate_text()` → `role == A`; `ble_mesh_relay_handle_phone_write()` returns `-EACCES` otherwise (phone write fails with ATT "unlikely error"). |
| Deployed topology images **reject any TEXT whose source ≠ A** | `ble_mesh_topology_filter_text()` memcmp to `node_a_id` → `REJECTED`; B also needs TTL==3, C TTL==2. |
| ⇒ **TASK035's "write text Kit2→Kit3 using the topology image" is wrong.** Kit2 (role B, `0c13dae125c8`) and Kit3 (role C, `ee07145a5e02`) cannot originate, and would drop each other's text even if they could. Only a general-policy image on **both** ends gives Kit2↔Kit3 text. |
| General policy = `CONFIG_BLE_MESH_TEST_TOPOLOGY` unset (default n) | `process_received_packets()` else-branch: dedup only; every remote TEXT → GATT TX notify, inbox (if enabled), relay with TTL-1. Origination: any node, TTL 3, 14-byte max (`BLE_MESH_MAX_TEXT_LEN`). No A/B/C ID check; the B-ID `#error` is topology-only. |
| Candidate image is **identical for Kit2 and Kit3** | No per-kit config; node ID and name derive from FICR at runtime. One UF2, one hash. |
| Inbox is fed only by remote TEXT that passed dedup (no local echo) | `ble_mesh_relay.c` guarded by `CONFIG_BLE_INBOX`. |

## 2. Exact candidate configuration
Base: root `prj.conf` (BT, BT_PERIPHERAL, BT_OBSERVER, name `HybridMesh` – replaced by dynamic name below). Board `xiao_ble/nrf52840`. Overlay `tests/task036_kit2_kit3_candidate.conf`:
```
CONFIG_BLE_INBOX=y
CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME=y
CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y
```
Must NOT be set: `CONFIG_BLE_MESH_TEST_TOPOLOGY`, `CONFIG_BLE_MESH_TEST_B_ID_*` (reviewer: grep the built `.config`). `BT_MAX_CONN=1` (from earlier `.config`): **one phone per kit at a time**. `BT_MAX_PAIRED=0`, no bonding; GATT is unauthenticated.

## 3. Identity, UUID, names
| Item | Kit2/B | Kit3/C | Kit1/A |
|---|---|---|---|
| Mesh ID (FICR) | `0c13dae125c8` | `ee07145a5e02` | `eb34ed25a5a4` |
| Name now (deployed) | `HybridMesh` | `HybridMesh` (same → ambiguous) | n/a (LoRa image) |
| Candidate name `Mesh-` + `node_id[3..5]` (derived by hand from `ble_gatt_android_name_format`; **unverified on air**) | `Mesh-E125C8` | `Mesh-5A5E02` | would be `Mesh-25A5A4` |
Names are unique among the three kits. Service `7B3E0001-6F2A-4C91-9A3D-2F5B6C7D8E00`; RX 0002, TX 0003, Discovery 0004, Evidence 0005, **Inbox 0006 (read-only, new, no collision)**; all share suffix `-6F2A-4C91-9A3D-2F5B6C7D8E00`. Name and UUID are in the scan response (service UUID + name; adv PDU holds flags+mesh manufacturer data when a text is pending).
Android caveat: identity address is FICR-derived and unchanged, so Android may cache the old GATT table/name (no Service Changed). Before testing: Android Bluetooth off/on, nRF Connect "Refresh services"/clear cache, or forget device; otherwise 0006 may not appear.

## 4. Phone-connected advertising constraints (from code)
- GATT advertising is connectable (`BT_LE_ADV_CONN_FAST_1`); with `BT_MAX_CONN=1` it stops while a phone is connected.
- Mesh TEXT/beacon frames are sent by updating that advertisement (`ble_gatt_update_mesh_advertisement`). While connected it returns `-EAGAIN`; the packet stays at the queue head and is retried every 1 s. **A sender whose phone stays connected never transmits the text.** After disconnect, advertising restarts with the pending frame and each text is held `BLE_MESH_TEXT_ADV_HOLD_MS`=1000 ms (≥10 adv events at ≤60 ms) – a single 1 s window per text, no ACK/retry.
- Receiver keeps scanning while connected (observer is independent of the connection; no code stops scan), so a connected receiver phone still gets the TX notify (0003, if subscribed) and the inbox (0006). Its relay (TTL 2) is held until that phone disconnects, then advertised once; the sender ignores its own source ID, so this is harmless but means a stale relay frame can appear after disconnect.
- ATT MTU 23: inbox image (≤188 B) is read with Read Blob; per-connection snapshot protects against torn reads (Task034; unproven on hardware).
- Dedup (src+seq, 16 entries, 30 s) covers beacons too; sequence counter restarts at 0 on reboot. **Wait ≥35 s after either kit boots before the first text**, and use a unique text each time.
- Max 14 bytes; `DISCOVER` is a control command (no-echo → not stored in TX).

## 5. Compatibility matrix during the phases
| State | Kit2 | Kit3 | Kit3→Kit2 text | Kit2→Kit3 text | Notes |
|---|---|---|---|---|---|
| Today | Task010 topology (B) | Task012 topology (C) | impossible (origin denied) | impossible | |
| **Phase 1** | Task010 topology | **candidate** | Kit3 can originate; **Kit2 drops it (RX_REJECT)** | impossible (Kit2 cannot originate) | Kit3 inbox cannot receive real text; only Kit2 beacons. Smoke test only. |
| **Phase 2** | **candidate** | candidate | works | works | Both directions testable. |
Phase 1 therefore validates only Kit3 standalone behaviour (name, UUID, inbox read of an empty 4-byte image `01 00 00 00`, phone write accepted, no DISCOVER echo, Kit2 unaffected). A negative control is expected: Kit2's TX characteristic must **not** show Kit3's text. Beacons still interoperate (filter applies to TEXT only).

## 6. Independent comparison / commands for reviewer
Pre-existing artifacts (recomputed with `Get-FileHash` this task; all configs read from each build's `.config`):
| Role | Build dir | SHA256 | Size |
|---|---|---|---|
| **Kit2 deployed (rollback)** Task010 topology B_ID_0C13 | `build-task009-scanfix-topology` | `2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773` | 270848 |
| **Kit3 deployed (rollback)** Task012 topology B_ID_0C13 | `build-task012-topology` (= ci/013/014 topology) | `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8` | 272384 |
| Kit1 deployed Task022 LoRa RX-only (unchanged) | `build-task022-uk-receiver` | `28C2852D6C714D428D0269ECAF48BC3E8ACEEAFFF4D672E69BAEDECD738C4644` | 112128 |
| General default (no opt-ins), reference | `build-task034-independent-default` | `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14` | 268288 |
| Inbox only (Task034) | `build-task034-independent-inbox` | `8A23CF3261114E9DB52D5DDDFECADB66E2B2B690FAEE2EB50D3C8A1AD080799C` | 269824 |
| Android name + no-echo only (Task032) | `build-task032-independent-android` | `9380BC59C3ED86AA63CF70C4A587C42DC6FFFF36DE149DEDEF55AE550C33A7B1` | 269312 |
| **Candidate (all three opt-ins)** | `build-task036-kit23-candidate` | **NOT BUILT** | – |
Note: `build-task033-independent-inbox` has `BLE_INBOX=y` but the same hash as the default image – a stale/invalid artifact; do not use it as inbox evidence.
Reviewer build (process-local env, run outside this sandbox):
```
cd C:\zephyrproject\hybrid-mesh-poc
$env:ZEPHYR_BASE='C:\zephyrproject\zephyr'
C:\zephyrproject\.venv\Scripts\python.exe -m west build -p always -b xiao_ble/nrf52840 -d build-task036-kit23-candidate . -- -DEXTRA_CONF_FILE=C:/zephyrproject/hybrid-mesh-poc/tests/task036_kit2_kit3_candidate.conf
Get-FileHash build-task036-kit23-candidate\zephyr\zephyr.uf2 -Algorithm SHA256
Select-String build-task036-kit23-candidate\zephyr\.config -Pattern 'BLE_INBOX|ANDROID_DISCOVERY|NO_ECHO|TEST_TOPOLOGY|TEST_B_ID|BT_MAX_CONN|BT_DEVICE_NAME_DYNAMIC'
```
Expected: `BLE_INBOX=y`, `ANDROID_DISCOVERY_NAME=y`, `NO_ECHO=y`, `BT_DEVICE_NAME_DYNAMIC=y`, topology/B_ID not set, UF2 differs from every hash above. Record the hash here and in PROGRESS.

## 7. Per-kit risk
**Kit3 (Phase 1, first):** only receiver-capable/inbox-capable kit while Kit2 still rejects its text → no end-to-end proof; Android GATT cache may hide 0006 or show old name; loses the Task012 topology evidence-counter semantics (role letter in evidence becomes `?`, since role is topology-only); if the image fails to boot Kit3 needs UF2 rollback (bootloader untouched, UF2 drag-drop only). Moderate-low.
**Kit2 (Phase 2):** loses the B relay role used for the accepted A→B→C PoC; that three-node test would then be impossible until Kit2/Kit3 are rolled back (and Kit1 BLE restored – not authorized). Topology policy and general policy cannot coexist in one network: a general node floods TEXT from anyone, deployed topology nodes drop non-A. Moderate. Do Phase 2 only after Phase 1 is reviewed clean.
**Both:** unauthenticated GATT write/read and unauthenticated source IDs; single 1 s advertisement window per text with no ACK, so loss is possible (retry with a new text); inbox is RAM-only (lost on reset), 8 slots; one phone per kit (`BT_MAX_CONN=1`); user-facing "success" = text visible, not delivery guarantee. **Kit1:** unchanged, LoRa RX-only; no LoRa TX, no RF scope change.

## 8. Realistic Android B/C test (each direction separate; run after Phase 2 unless stated)
Pre: both kits powered ≥35 s, Android cache cleared, two Android sessions/phones with nRF Connect (one per kit; BT_MAX_CONN=1). Texts ≤14 bytes, unique per run (e.g. `K2T1`, `K3T1`). Evidence: screenshots + notes; no claim without them.
**Direction D1 Kit2 → Kit3**
1. Receiver phone: connect `Mesh-5A5E02`, enable notifications on 0003 (and 0004), read 0006 (expect `01 00 00 00` or previous entries). **Stay connected.**
2. Sender phone: connect `Mesh-E125C8`, write ASCII `K2T1` to 0002 (expect write success; a `DISCOVER` write must not echo). **Then DISCONNECT** (releases advertising; Kit2 then advertises the frame for ~1 s).
3. Receiver phone: observe 0003 notification `K2T1`; re-read 0006 → count 1, source ID `0C13DAE125C8`, text `K2T1`. Pass = both. Optionally disconnect/reconnect receiver and re-read to confirm RAM inbox persistence.
**Direction D2 Kit3 → Kit2** – repeat with roles swapped (`K3T1`; expected source `EE07145A5E02`).
Negative/variant checks: (a) sender phone left connected → no delivery expected (`-EAGAIN` hold) – confirms the constraint; (b) receiver not connected during send, connect afterwards and read 0006 – inbox-after-the-fact; (c) write while topology image still on the peer (Phase 1) → no delivery, expected.
Phase 1 (Kit3 only) subset: discovery name, UUID 0001/0006 present, 0006 read = `01 00 00 00`, write accepted on 0002, no DISCOVER echo, Kit2 TX unchanged.

## 9. User approval steps (nothing below has been done)
1. Independent review of this doc, conf and the reviewer's own build + hash (rule 10); candidate hash recorded.
2. **Approval A:** flash candidate to **Kit3 only** (user double-resets, confirm INFO_UF2 Board-ID `Seeed_XIAO_nRF52840_Sense`, verify SHA256 before copy). Run Phase 1 subset. Kit1/Kit2 untouched.
3. Review Phase 1 evidence; keep acceptance open.
4. **Approval B (separate):** flash the same UF2 to **Kit2**; run D1 and D2. Acknowledge the loss of the topology A→B→C test setup.
5. Kit1 is not flashed in any step; no LoRa TX authorization is implied.

## 10. Rollback
Per kit, same procedure: user double-reset → UF2 drive → copy the original UF2 and verify its hash: Kit2 `2FFAA52A…7D773` (`build-task009-scanfix-topology\zephyr\zephyr.uf2`), Kit3 `D5E9EF84…FEEB8` (`build-task012-topology\zephyr\zephyr.uf2`). Verify the files are still present and unmodified before any flash. Repo rollback: delete `tests/task036_kit2_kit3_candidate.conf`, this doc, and the PROGRESS entry.

## Commands executed and actual results
- Reads: AGENTS.md, TASK030/034 reviews, TASK035 doc, `ble_mesh_topology.*`, `ble_mesh_relay.c`, `ble_gatt.c`, `ble_inbox_service.c`, `ble_gatt_android_name.c`, Kconfig, prj.conf, existing `.config`/UF2 hashes.
- `python -m west …` via venv: **not recognized (sandbox)**; system python: `No module named west`. No build, no UF2/hash for candidate.
- `python -m unittest discover -s tests -p "test_*.py"`: **55/55 OK**; `python tests\task024_static_tx_check.py`: **PASS** (unchanged code, sanity only).
- No hardware, flash, RF, install, git staging.

## Issues
- Candidate build and Android name/inbox behaviour unverified; derived names (`Mesh-E125C8`/`Mesh-5A5E02`) are hand-computed.
- Earlier "Kit2 = Task010 / Kit3 = Task012 deployed" comes from PROGRESS flash records; no live readback was done (no hardware access allowed).
- Scanning-while-connected on a Kit is assumed from code and earlier coexistence tests, not proven for the inbox image.

## Next action
**WAITING FOR:** independent review and an independent west build of the candidate (venv python is sandbox-blocked for this agent). No flash without Approval A/B.
