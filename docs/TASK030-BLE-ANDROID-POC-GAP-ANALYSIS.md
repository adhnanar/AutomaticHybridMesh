# Task030 — Offline BLE discovery + text messaging via Android nRF Connect: gap analysis

2026-10-19. **PC-only static analysis. No source/config/test changed, nothing flashed, reset, transmitted (BLE or LoRa), no USB/power action, no installs, no git stage/commit.** Repo: `C:\zephyrproject\hybrid-mesh-poc`. Status: **awaiting independent review.**

## Scope
Re-center the PoC on a user-visible offline demo: phone discovers nearby Frurt nodes and exchanges short text through Android nRF Connect (no custom app), using the stable firmware unchanged. This document inventories what the code really exposes, defines the smallest truthful demo, and lists gaps. Sources read: `AGENTS.md`, `README.md`, `docs/PROGRESS.md`, `docs/TASK029-INDEPENDENT-REVIEW.md`, `docs/PHASE2-BLE-GATT.md`, `docs/PHASE3-BLE-MESH.md`, `docs/PHASE3-CONNECTION-RECOVERY.md`, `docs/TASK016-PC-GATT-NOTIFICATIONS.md`, `docs/TASK018-DISCOVER-CONTROL-NO-ECHO.md`, `src/main.c`, `src/features/ble_gatt/*`, `src/features/ble_mesh_relay/{ble_mesh_relay.c,ble_mesh_packet.*,ble_mesh_topology.c,ble_mesh_evidence.h,ble_mesh_scan_policy.h}`, `prj.conf`, `Kconfig`, `build-task012-*/zephyr/.config`.

## Files changed / commands executed
- Created this file; appended a Task030 entry to `docs/PROGRESS.md`. No code, test or config edits, so no Python/static test run was required (none edited).
- Commands: read-only file listing/greps; `Get-FileHash` of the two existing Task012 UF2s (no board access).
- Verified hashes of the on-disk Task012 artifacts match those recorded in Task018: default `2575E713…0D1F14`, topology `D5E9EF84…FEEB8`.

## What is deployed vs what is in the tree
README: Kit1/A `E5:12:76:D2:56:EA` and Kit3/C `E2:DB:6A:54:BB:71` run the Task012 **topology** UF2; Kit2/B `C8:51:08:6E:08:C0` runs Task010 topology firmware (per Task012 acceptance doc). So **the boards currently carry the test-only A→B→C topology policy**, not the default root image. Consequences below are labelled *[topology]* or *[default]*. The Task018 no-echo option is built but **not deployed**.

## GATT inventory (from `ble_gatt.c`)
Device name `HybridMesh` (`CONFIG_BT_DEVICE_NAME`, identical on every node). Scan response always carries the 128-bit service UUID and the name; the primary advertising PDU carries the service UUID when idle and switches to manufacturer data (company ID `0xFFFF`, mesh frame) while a mesh frame is held. Connectable legacy advertising, `BT_MAX_CONN=1`, `BT_MAX_PAIRED=0`, no SMP/security, ATT MTU 23 (payload max 20 bytes).

| Item | UUID (as shown in nRF Connect) | Properties | Behaviour |
|---|---|---|---|
| Service | `7B3E0001-6F2A-4C91-9A3D-2F5B6C7D8E00` | primary | — |
| RX | `7B3E0002-6F2A-4C91-9A3D-2F5B6C7D8E00` | Write, Write w/o response | ≤20 B accepted by GATT; relay handler then enforces ≤14 B text (else ATT error "invalid attribute length"). Exact ASCII `DISCOVER` = control command. |
| TX | `7B3E0003-6F2A-4C91-9A3D-2F5B6C7D8E00` | Read, Notify (+CCC) | **Single last-message buffer (≤20 B)**. Updated by (a) local RX text write (echo) and (b) any accepted received mesh text. |
| Discovery | `7B3E0004-6F2A-4C91-9A3D-2F5B6C7D8E00` | Read, Notify (+CCC) | **Single last-event buffer**. Initial `discovery inactive`; then `PEER:<12 hex node ID>` or `BLE: no peers`. |
| Evidence | `7B3E0005-6F2A-4C91-9A3D-2F5B6C7D8E00` | Read | 20-byte binary counters (version, role, decoded/text/reject/accept/duplicate, last seq/ttl/event, XOR checksum). Debug aid, not user-readable. |

TX and Discovery use separate CCC descriptors (`0x2902`); nRF Connect's "subscribe" button writes it.

## Mesh/protocol facts that matter for a phone user
- Frame (`ble_mesh_packet.h`): type `0x11` beacon / `0x12` text, 6-byte source ID, u16 sequence, TTL (max 3), **≤14 text bytes**, no fragmentation. Beacon every 5 s; text held ~1 s per advertising update. Unauthenticated, unencrypted, unaddressed (flood broadcast).
- Node ID = nRF52 FICR DEVICEID (e.g. A = `EB34ED25A5A4`), **not the BLE MAC** shown by nRF Connect, and not user-assigned. No profile/alias/handle exists anywhere in code.
- Peer table: 8 entries, 60 s expiry, fed by beacons and text. A `PEER:` notification fires only when a peer is *new or expired*; a phone connecting later sees nothing until it writes `DISCOVER` (which re-publishes all current peers back-to-back; each overwrites the single Discovery value).
- Phone-connected node cannot advertise mesh frames: `ble_gatt_update_mesh_advertisement` returns `-EAGAIN` while a connection is active; the relay retries every 1 s (queue depth 4) and the frame goes out only after the phone **disconnects** and advertising restarts. Scanning/receiving continues while connected (Task016 evidence with a PC).
- *[topology]* only role A may originate text (B/C RX text write fails with ATT "unlikely error"); only A-sourced frames are accepted; B accepts TTL3 only and relays; C accepts TTL2 only. `DISCOVER` works on every node. Reverse direction or C→A is **not possible** on deployed images.
- *[default]* every node originates, accepts and relays any text (TTL3→1) with 16-entry/30 s dedup. **No live evidence found in the reviewed docs of this build being used phone-to-phone**; it is software/compile evidence plus shared code paths.
- *[deployed, no-echo off]* writing text or `DISCOVER` to RX also sets and notifies TX with the written bytes. The sender's own phone therefore sees its own text (and the literal `DISCOVER`) on TX.

## Implemented vs missing (for "discovery + text messaging")
| Capability | State | Evidence |
|---|---|---|
| Phone connects, writes RX, reads TX, enables notify (nRF Connect) | Implemented; user screenshots Oct 18 (older firmware) | README, Phase2 doc |
| Nearby-node discovery visible to phone | Implemented as `PEER:<id>` notification / read; PC 3/3 PASS (`DISCOVER`) | Task016B; **not verified on Android with current firmware** |
| Text from phone to remote node's phone-visible TX | Implemented in BLE advert path, A→B→C only on deployed images; PC-driven 5/5 | Task012/016; **no Android evidence** |
| Remote text arrives as GATT notification | PC 5/5 exactly-once | Task016 |
| Inbox / history | **Missing** (one 20-byte slot, overwritten, lost on reset) | code |
| Sender identity / profile / name | **Missing** (TX carries bare text; discovery gives opaque hex ID) | code |
| Outbox / delivery status / ack / retries | **Missing** (fire-and-forget advert; queue depth 4; no feedback) | code |
| Bidirectional multi-node messaging on deployed images | **Missing** (topology lock) | `ble_mesh_topology.c` |
| Send while phone stays connected | **Missing/limited** (`-EAGAIN` until disconnect) | `ble_gatt.c` |
| Unique device name per node | **Missing** | `prj.conf` |
| Security (pairing, encryption, auth) | **Missing** | `MAX_PAIRED=0` |
| Phone background/alert notifications | **Not provided; unverified** | see below |
| LoRa path | Not operated by deployed firmware | README |

## Smallest truthful demonstration with existing firmware (no reflash)
Goal: show *a phone user sees nearby nodes and a text sent from one node's phone appears on another node's phone*, honestly limited to the A→B→C topology.

Phone 1 = sender (node A). Phone 2 = receiver (node C), or one phone with two sequential connections (phone connection limit is per board, not per phone).

Preparation (human, nRF Connect for Android): scan; identify boards by **MAC** (all are named `HybridMesh`): A `E5:12:76:D2:56:EA`, B `C8:51:08:6E:08:C0`, C `E2:DB:6A:54:BB:71`. Physical placement must keep A and C out of direct range, with B between (Task012 found isolation empirical, not guaranteed; direct leaks are rejected by C).

1. **Discovery:** connect to C → open service `7B3E0001-…` → tap Discovery (`…0004`) subscribe (triple-arrow) → write text `DISCOVER` to RX (`…0002`, type *Text*). Expect Discovery value/notification(s) `PEER:<12 hex>` or `BLE: no peers`; TX will also show `DISCOVER` (echo, deployed images). Honest expected IDs: A `EB34ED25A5A4` plus B ID (`0C13DAE125C8` or `EE07145A5E02`; C's ID is the other). Which peers appear depends on RF range. Android display of back-to-back notifications is unverified.
2. **Receiver ready:** on C subscribe to TX (`…0003`) and keep the connection open and nRF Connect in the foreground.
3. **Send:** connect to A, write ≤14 ASCII bytes (e.g. `HELLO30`) to RX, **then disconnect** from A (needed for the advert to go out). A's own TX shows the echo.
4. **Observe:** C's TX should notify `HELLO30` once (PC-verified behaviour). If missed, read TX once on C. Dismiss/overwrite semantics: only the latest message is retained.
5. Negative checks worth recording: write 15 bytes → rejected; write to B or C → rejected *[topology]*; read evidence char only for debugging.

This demonstrates discovery and one-directional two-hop text only. It must **not** be described as bidirectional chat, a mesh, or a delivered inbox.

## Cross-device relay feasibility with phones
- Relay itself is phone-independent (connectionless BLE advertising); Task012 PC-driven 5/5 two-hop and Task016 5/5 notification coexistence show the mechanism works with a GATT link active on C.
- Phone→A→B→C was never exercised with Android; the PC used `write` then `disconnect`, which matches the required procedure above.
- Coexistence of phone GATT + mesh (rule 8) has PC evidence only; Android connection-recovery after phone Bluetooth restart (`C8:51:08:6E:08:C0`) remains an open field issue (PHASE3-CONNECTION-RECOVERY.md).
- Full BLE peer multi-hop between *two phones in both directions* requires lifting the topology lock (default image) plus new work below; it is not claimable now.

## nRF Connect (Android) notification limitations
- GATT notifications appear only as value updates/log lines inside an active nRF Connect connection; this repo has **no evidence of Android alerts, vibration, status-bar notifications or background delivery**, and none must be claimed. nRF Connect is a developer tool; behaviour when the screen is off or app backgrounded is OS-dependent and unverified.
- Android GATT notifications are lost if the link drops; the board retains only the latest TX value in RAM, and a reconnect read returns just that.
- Long/binary values are displayed per the selected format (Text/Byte array); the 14-byte limit counts UTF-8 bytes.
- Only one phone can be connected per board (`BT_MAX_CONN=1`); the connected phone prevents that board from transmitting.

## Prioritized next implementation tasks (all need approval before any flash)
1. **P0 – Android evidence run on existing images** (needs user/phone): execute the procedure above, capture nRF Connect screenshots/log export, record in a Task doc. No firmware change.
2. **P1 – Truthful single-slot UX fixes, opt-in Kconfig, default off** (PC build + review): enable `BLE_GATT_DISCOVER_NO_ECHO` in a candidate image (already built, Task018); add a per-node unique `BT_DEVICE_NAME`/short suffix from node ID; format discovery events with a node label (`PEER:<id> rssi`). Must keep Task012 hashes reproducible when off.
3. **P1 – Sender-prefixed TX payload** (`<id4>:<text>` within 20 B, so text ≤ ~9 bytes) and a PC-testable Python/C pure formatter with unit tests (extends `tests/hybrid_router_sim` style).
4. **P2 – Small inbox characteristic** (ring of last N messages with sequence, readable by nRF Connect) + host-testable ring buffer; lets a reconnecting phone read missed messages.
5. **P2 – Transmit-while-connected**: allow advert updates during a connection (extended/second advertising set or non-connectable set) so the sender phone need not disconnect; requires hardware validation and `BT_MAX_CONN` review.
6. **P3 – Default-image bidirectional two-phone test** (A↔C via B) with deduplication; physical test plan.
7. **P3 – Security**: authentication/encryption or at least pairing/whitelist; payload integrity; document threat model.
8. Android background-notification behaviour: only investigate if an evidence run shows a need; otherwise state "foreground nRF Connect only".

## Issues / risks
- All Android claims are inherited from older screenshots; current-firmware Android behaviour is unverified.
- Deployed topology images constrain the demo; test IDs are hard-coded in `ble_mesh_topology.c`.
- RF isolation of A/C is empirical (Task016 saw direct-frame leakage, rejected by C).

## Rollback
Documentation only: delete this file and the Task030 line in `docs/PROGRESS.md`.

## Next action
Independent review of this analysis (rule 10), then user-approved Android evidence run (P0). **Review verdict: pending.**
