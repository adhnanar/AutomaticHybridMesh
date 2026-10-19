# Task049 — Phase 5 local acceptance classifier

## Scope

Add a pure, header-only classifier for synthetic `hybrid_router_result` values
used to represent local GATT acceptance. Queued and sent outcomes listed below
classify as locally accepted; queue-full, invalid, and duplicate outcomes do
not represent new acceptance. This result is not evidence of peer delivery.

No router send, phone/GATT wiring, callbacks, radio source/configuration,
hardware, flash, reset, or LoRa transmission is included.

## Files changed

- `src/features/hybrid_router/hybrid_router_local_acceptance.h` — pure
  result-to-local-acceptance helper.
- `tests/hybrid_router/src/main.c` — compact table-driven ztest covering the
  accepted and non-accepted result classes.
- `docs/TASK049-LOCAL-ACCEPTANCE-CLASSIFIER.md` — task record.
- `docs/PROGRESS.md` — Task049 progress entry.

## Commands executed

No shell, build, or test commands were run. Builds and tests are left to the
independent reviewer as requested.

## Actual results

The helper maps `QUEUED_RETRY`, `NO_LINK_QUEUED`,
`LORA_BLOCKED_QUEUED`, `SENT`, and `SENT_FALLBACK` to locally accepted.
`QUEUE_FULL`, `INVALID`, `DUPLICATE`, and `NOT_FORWARDED` map to not accepted.
The helper does not invoke or inspect the router and does not claim peer
delivery. The added ztest was not executed.

## Issues and review status

No known issues from the scoped source inspection. Independent review and
reviewer-run build/tests are pending; this is not an acceptance verdict.

## Rollback

Remove `src/features/hybrid_router/hybrid_router_local_acceptance.h`, remove
its include and `test_local_acceptance_result_classification` from
`tests/hybrid_router/src/main.c`, and remove the Task049 entry from
`docs/PROGRESS.md` and this task record.

## Next action

Stop for independent review. Keep Phase 5 acceptance and any GATT/router
integration open until separately reviewed and tested.
