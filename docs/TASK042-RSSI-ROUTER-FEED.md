# Task042 — RSSI feed to hybrid router (opt-in)

**Scope:** under `CONFIG_HYBRID_ROUTER_POLICY` only (default n), feed the
validated BLE RX queue RSSI to `hybrid_router_ble_peer_seen`.

**File changed:** `src/features/ble_mesh_relay/ble_mesh_relay.c` (all additions
are `#if CONFIG_HYBRID_ROUTER_POLICY` guarded).
- Include `hybrid_router.h`; static `hybrid_router_state`.
- `ble_mesh_relay_init`: after `ble_mesh_dedup_init`, `hybrid_router_init` with a
  static zeroed ops struct (no `lora_send`, `ble_send`, or `deliver`
  callbacks). Init leaves LoRa TX unauthorized; nothing authorizes it. Failure
  returns the error.
- `process_received_packets`: after the self-source skip, calls
  `hybrid_router_ble_peer_seen(&state, k_uptime_get_32(), rssi)`. Frames in
  the queue were already fully decoded; only the serialized RX worker touches
  the router (no other thread calls it).

**Unchanged:** phone writes, relays, dedup use, advertising, GATT, radio
functions, default-disabled image. The router's send/receive/poll are never
called, so the shared dedup cache is not touched by it.

**Commands/results:** none (no build/tests per task rules). Unverified.

**Issues/notes:** router state is currently write-only (no consumer yet).
CMake already adds the include dir for this config.

**Rollback:** revert the guarded blocks in `ble_mesh_relay.c`, this doc and the
PROGRESS entry.

**Next:** reviewer builds default and `CONFIG_HYBRID_ROUTER_POLICY=y`, confirms
default image logic unchanged; then fake-input RSSI test.
