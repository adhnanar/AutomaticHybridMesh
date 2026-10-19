# Task040 — Hybrid BLE integration contract

**Scope:** PC-only API contract for feeding BLE mesh receive RSSI to the C
hybrid router and routing Android GATT text through it. Documentation only:
no implementation, build, test, flash, hardware, or RF activity.

## Current API facts

- [`ble_mesh_relay.c`](../src/features/ble_mesh_relay/ble_mesh_relay.c)
  receives advertisements in `scan_receive(address, rssi, advertising_type,
  advertising_data)`. It currently discards `rssi`. Its parser accepts only
  the configured manufacturer ID and successfully decoded mesh frames, then
  queues a `struct ble_mesh_packet`; the RSSI and advertiser address are not
  retained. `process_received_packets()` owns the existing receive-side
  deduplication, local GATT publication, inbox recording, and legacy relay
  queueing.
- [`ble_gatt.h`](../src/features/ble_gatt/ble_gatt.h) defines
  `ble_gatt_rx_handler_t` and `ble_gatt_set_rx_handler()`. The GATT RX
  `write_rx()` callback invokes that handler; [`main.c`](../src/main.c)
  registers `ble_mesh_relay_handle_phone_write()`.
- `ble_mesh_relay_handle_phone_write()` currently validates the Android text,
  builds a local TEXT packet, and queues it to the legacy relay queue. It
  does not call the hybrid router.
- [`hybrid_router.h`](../src/features/hybrid_router/hybrid_router.h) exposes
  `hybrid_router_ble_peer_seen/lost()`, `hybrid_router_send_local()`,
  `hybrid_router_receive()`, and `hybrid_router_poll()`. Router APIs, its
  shared dedup cache, and callbacks must be serialized by the integration
  owner; callbacks must not re-enter the router. The router does not own
  radios or GATT.
- `hybrid_router_ble_peer_seen(router, now_ms, rssi_dbm)` accepts RSSI at or
  above `HYBRID_ROUTER_BLE_MIN_RSSI_DBM` (-85 dBm). A report stays fresh
  through the inclusive `HYBRID_ROUTER_BLE_FRESH_MS` limit (10,000 ms).
  This API stores one aggregate reachability bit/time, not peer identity.
- `ble_gatt_update_mesh_advertisement()` copies the encoded frame into
  GATT-owned advertising data, then returns `-EAGAIN` while any phone
  connection is active. Advertising restart is deferred until disconnect.
  A successful advertising-data update means the controller accepted the
  data update; it is not an over-the-air delivery acknowledgement.

## Integration contract

### Ownership and serialization

1. The relay feature owns one `hybrid_router`, its `ble_mesh_dedup_cache`,
   sequence counter, serialized event/work context, and pending retry work.
   Initialize the router from `ble_mesh_relay_init()`; keep router state
   private to that feature.
2. Extend the BLE RX queue item to own a copy of the decoded packet and its
   RSSI. Carry RSSI through `bt_data_parse()` context from `scan_receive()`.
   Queue only successfully decoded mesh manufacturer frames; arbitrary
   advertisements and malformed frames never establish router reachability.
   The worker, not the scanner callback, invokes router APIs.
3. The same serialized worker/work context owns `hybrid_router_receive()`,
   `hybrid_router_send_local()`, `hybrid_router_poll()`, BLE peer
   seen/lost updates, and the shared dedup cache. The BLE callback and GATT
   write callback enqueue work rather than concurrently entering the router.
4. Router operation callbacks borrow frame/packet memory only for the
   duration of the callback. The router copies retry frames into its own
   pending queue. The BLE GATT layer owns its copied advertising value;
   delivery code must copy any packet data it retains. The existing inbox
   service copies accepted text into its ring under its own mutex.

### BLE receive and Android ingress events

- **BLE RX event:** `scan_receive()` receives `(address, rssi, type, data)`.
  After manufacturer-ID and packet decode succeed, enqueue
  `{packet, rssi_dbm}`. In the worker, call
  `hybrid_router_ble_peer_seen(&router, k_uptime_get_32(), rssi_dbm)` and
  process that frame through `hybrid_router_receive(...,
  HYBRID_ROUTER_LINK_BLE, encoded_frame, length, now_ms)`. Preserve the
  existing accepted-text side effects in the router `deliver` callback:
  GATT TX publication, optional inbox copy, and peer/discovery event
  publication. Do not also run legacy dedup/relay for the same frame.
- **RSSI meaning:** the RSSI belongs to the received advertisement, but the
  mesh `source_id` identifies the packet origin and can be multi-hop. The
  router API has no peer/address parameter, so this is evidence of a recent
  qualifying BLE mesh advertisement, not a destination-specific link metric
  or proof that the origin is the immediate radio neighbor.
- **Phone text event:** retain the registered callback chain
  `write_rx()` -> `ble_gatt_rx_handler_t` ->
  `ble_mesh_relay_handle_phone_write()`. The handler creates a local TEXT
  packet with the relay-owned node ID, sequence, and initial TTL, then calls
  `hybrid_router_send_local()`. Return the existing GATT success result for
  accepted/sent/queued outcomes so accepted text keeps its current GATT
  acknowledgement and TX-text echo behavior. Invalid input and queue-full
  outcomes remain errors; do not describe a queued packet as delivered.
- **Router operations:** `ble_send(ctx, frame, length)` calls
  `ble_gatt_update_mesh_advertisement()` and propagates its error, including
  `-EAGAIN`. `deliver(ctx, packet)` performs the current local accepted-text
  side effects only. A LoRa operation is not permitted to transmit unless a
  separate external authorization is explicitly supplied; initialize
  authorization false and provide no route that can enable it in this task.
- **Near/blocked behavior:** a fresh qualifying BLE report makes BLE the
  first-choice route. A successful BLE data update yields router `SENT`
  (or `SENT_FALLBACK`), not proof of RF delivery. If BLE is stale/unavailable
  and the LoRa path is present/available but authorization is false, require
  `HYBRID_ROUTER_LORA_BLOCKED_QUEUED`; keep the frame pending and never call
  the LoRa send callback. Publish the explicit status event
  `BLOCKED:LORA_UNAUTH` through `ble_gatt_publish_discovery_event()` (19
  bytes, within `BLE_GATT_MAX_VALUE_LEN`). Pending storage is bounded
  (`HYBRID_ROUTER_PENDING_MAX`) and unsuccessful retries are bounded by
  `HYBRID_ROUTER_MAX_RETRIES`; report queue-full or eventual retry-drop
  outcomes rather than implying indefinite retention. If no LoRa transport
  path exists, the router correctly reports `NO_LINK_QUEUED` instead; do not
  mislabel that router result as LoRa authorization state.
- **Connected-phone limitation:** the Android phone is connected while it
  sends its GATT write. In that state `ble_gatt_update_mesh_advertisement()`
  returns `-EAGAIN`, even if a mesh peer was recently heard. The router must
  treat the BLE callback as failed/queued, not sent, and retry from serialized
  work. Disconnect permits advertising restart using the cached latest
  payload; a later accepted update still proves only controller acceptance.
  Thus “peer near” does not guarantee an immediate BLE advertisement while
  the phone remains connected, and the present API cannot promise over-air
  delivery.

## Smallest following code change

After this contract is reviewed, make one focused change in
`src/features/ble_mesh_relay/ble_mesh_relay.c`: add the router-owned state and
serialized work adapter; carry RSSI with each accepted BLE RX queue item;
route accepted BLE frames and Android writes through the router; retain the
existing GATT/inbox delivery effects in `deliver`; and retry router-pending
work without enabling LoRa TX. Keep the legacy path intact when the
hybrid-router policy is disabled. Do not alter radio configuration or
advertising/connection policy in that change.

## Five deterministic acceptance tests

Use C-level fake callbacks and synthetic frames only; no device or RF is
needed. These are proposed tests, not tests run for Task040.

1. **RSSI filtering and source events:** a valid mesh manufacturer frame at
   -85 dBm reports BLE seen; at -86 dBm it does not refresh reachability.
   Non-mesh advertisements and malformed frames do not report seen or enter
   the router.
2. **Freshness boundary:** a qualifying report routes BLE at exactly
   `seen_ms + 10,000`; at `seen_ms + 10,001`, BLE is stale. Include unsigned
   millisecond wraparound within the router's documented span.
3. **Android text to nearby peer:** invoke the registered RX handler with
   valid text while BLE is fresh; assert one BLE callback, zero LoRa callback,
   decoded frame has local source ID, allocated sequence, TEXT type, expected
   TTL and unchanged payload, and GATT write succeeds. Treat the GATT TX text
   echo as acceptance only, not delivery proof.
4. **Unauthorized LoRa is explicitly blocked:** with BLE stale and a
   LoRa-capable test adapter present but authorization false, assert
   `HYBRID_ROUTER_LORA_BLOCKED_QUEUED`, one pending frame, zero LoRa callback
   invocations, and exact status `BLOCKED:LORA_UNAUTH`. Keep the adapter
   fake/non-transmitting.
5. **Phone-connected BLE backpressure:** make the fake BLE callback return
   `-EAGAIN` to model the active-phone guard; assert no sent outcome and a
   pending retry. After the fake guard is cleared, advance to the retry time
   and assert one accepted BLE callback and one completed retry. This verifies
   router handling of the API condition, not a physical connection or RF send.

## Task record

- **Files changed:** this contract and one concise entry in `docs/PROGRESS.md`;
  no source, configuration, or test files.
- **Commands executed:** read-only, focused workspace inspection:
  `Get-ChildItem -Name`; `Get-ChildItem -Name src`;
  `Get-ChildItem -Name tests`; `Get-ChildItem -Name src\features`;
  `Get-ChildItem -Name tests\hybrid_router; Get-ChildItem -Name tests\ble_inbox`;
  focused `Get-ChildItem -Recurse -File` listings of the BLE and hybrid-router
  feature directories; targeted `rg` searches for receive/RSSI, GATT RX,
  router entry points, and advertising/connection symbols in those API files.
- **Actual results:** TASK039 review verdict and open C-runtime acceptance
  were read. Source inspection confirmed the current BLE callback discards
  RSSI, phone writes use the legacy relay queue, the hybrid router is not
  wired into those paths, and GATT advertisement update returns `-EAGAIN`
  while a phone is connected. No builds or tests were run; no hardware, flash,
  reset, USB, power, downloads, installs, or RF activity occurred.
- **Issues/limitations:** the router's BLE reachability is aggregate, not
  destination-aware; advertising update acceptance is not air-delivery
  evidence; actual C acceptance remains open until the five proposed tests
  are implemented and run in a suitable nonphysical environment.
- **Rollback:** remove this document and its single Task040 progress entry;
  no implementation state needs rollback.
- **Next action:** independent review of this contract. Do not start the code
  change or advance acceptance until reviewed; keep all LoRa TX disabled.

**Review status:** awaiting independent review. This document records a
proposed integration contract, not implementation or test evidence.
