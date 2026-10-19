# TASK050 GATT local-write status classifier (PC-only)

**Scope:** pure helper `hybrid_router_result_to_gatt_status()`, test-only callable. No router/phone/GATT/radio wiring.

**Files:** `src/features/hybrid_router/hybrid_router_local_acceptance.h`, `tests/hybrid_router/src/main.c` (new table-driven `test_local_acceptance_gatt_status_mapping`), `docs/PROGRESS.md`.

**Mapping**
| Router result | Status |
|---|---|
| SENT, SENT_FALLBACK, QUEUED_RETRY, NO_LINK_QUEUED, LORA_BLOCKED_QUEUED | 0 |
| QUEUE_FULL | -ENOSPC |
| INVALID | -EINVAL |
| DUPLICATE | -EALREADY (not new acceptance) |
| NOT_FORWARDED | -ENOTSUP (not new acceptance) |
| unknown value | -EINVAL (fail closed) |

DUPLICATE/NOT_FORWARDED use distinct codes so a caller cannot confuse them with acceptance or malformed input. Zero is returned only when `hybrid_router_result_is_locally_accepted()` is true.

**Commands executed:** none (no build/tests; reviewer runs them). **Results:** unverified.

**Issues:** the exact ATT error translation of these codes is deferred to wiring.

**Rollback:** remove the helper, the new ztest and the PROGRESS line.

**Next action:** independent review (build `build-task015-hybrid-router-ztest`, run tests).
