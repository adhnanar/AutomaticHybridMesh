# Task048 — Phase 5 safe phone-ingress gate

**Scope:** PC-only acceptance specification for opt-in phone ingress queue
handling. No route dispatch, source/config/test changes, physical LoRa callback,
build, test, flash, reset, or RF activity.

## Required behavior

- With a phone connected, BLE advertising update may return `-EAGAIN`. Treat
  that as a failed immediate BLE attempt. If the bounded pending queue admits
  the phone packet, return the existing successful GATT write acknowledgement
  and record `HYBRID_ROUTER_QUEUED_RETRY`; do not report `SENT` or delivery.
  If admission fails with `HYBRID_ROUTER_QUEUE_FULL`, return a GATT error.
- GATT write success is authoritative only for local acceptance into the
  bounded queue. A successful BLE callback means only that the controller
  accepted the advertising-data update; neither it, the GATT TX echo, nor queue
  admission proves peer or over-the-air delivery. No delivery acknowledgement
  is defined by this gate.
- Retain the router bounds: four pending packets, 1,000 ms retry delay, and at
  most three failed retries after the initial attempt. On exhaustion, clear
  the pending entry and count/report a retry drop; never retain indefinitely.
- Report `BLOCKED:LORA_UNAUTH` through
  `ble_gatt_publish_discovery_event()` only when a LoRa path is configured and
  available but TX authorization is false and no usable BLE route remains.
  Return/record `HYBRID_ROUTER_LORA_BLOCKED_QUEUED`; the fake LoRa callback
  must not be invoked. A connected-phone BLE `-EAGAIN` with BLE otherwise
  available is `QUEUED_RETRY`, not LoRa-blocked. If no LoRa path exists, use
  `HYBRID_ROUTER_NO_LINK_QUEUED`, not an authorization status.

## Smallest next source change

After this gate is reviewed, limit the first implementation to
`src/features/ble_mesh_relay/ble_mesh_relay.c` and its focused C test: add the
opt-in phone-ingress queue/admission and outcome mapping in the serialized
feature context. Exercise only fake callbacks. Do not call
`hybrid_router_send_local()` or otherwise dispatch routes in that change; do
not add a physical LoRa operation or enable TX. Defer route dispatch to a
separate reviewed task.

## Synthetic acceptance cases

1. **Connected-phone backpressure:** fake BLE callback returns `-EAGAIN`;
   successful pending admission yields GATT success and
   `QUEUED_RETRY`/pending count 1, with no sent or delivered assertion.
2. **Admission versus delivery:** verify GATT success means accepted into the
   local queue only; no delivery is recorded. Verify queue-full yields GATT
   error and no false acceptance.
3. **Bounds and expiry:** admit four synthetic packets, reject the fifth as
   queue-full; fail each initial attempt and at most three 1,000 ms-spaced
   retries, then verify pending count returns to zero and retry-drop is
   recorded.
4. **Unauthorized LoRa:** with BLE unavailable and a fake LoRa path available
   but unauthorized, verify `LORA_BLOCKED_QUEUED`, exact
   `BLOCKED:LORA_UNAUTH`, one bounded pending packet, and zero LoRa callback
   calls. With no LoRa path, verify `NO_LINK_QUEUED` and no blocked event.
5. **No delivery inference:** a fake BLE callback returning success may record
   controller-data acceptance only; verify no peer-delivery result is emitted
   without an actual protocol acknowledgement.

All cases use synthetic packets and non-transmitting fakes. They do not verify
physical connections, radio operation, or RF delivery.

## Task record

- **Files changed:** this specification and one `docs/PROGRESS.md` entry.
- **Commands executed:** focused reads of `AGENTS.md` item 13,
  `docs/TASK047-INDEPENDENT-REVIEW.md`,
  `docs/TASK040-HYBRID-BLE-INTEGRATION-CONTRACT.md`, and
  `docs/PROGRESS.md`; targeted searches and reads of the hybrid-router
  constants/status/retry implementation.
- **Actual results:** confirmed contract behavior for connected-phone
  `-EAGAIN`, BLE controller acceptance versus delivery, explicit unauthorized
  LoRa status, and bounded queue/retry values (4, 1,000 ms, 3). No builds,
  tests, hardware access, flash, reset, or RF activity.
- **Issues/limitations:** this is a proposed test gate, not implementation or
  acceptance evidence; route dispatch and physical LoRa behavior remain out of
  scope.
- **Rollback:** remove this document and its single Task048 progress entry.
- **Next action:** stop for independent review. Do not implement route dispatch
  until the review accepts this gate.

**Review verdict:** awaiting independent review.
