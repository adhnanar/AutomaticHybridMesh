# Frurt — latest PC-only continuous integration review

2026-10-19. **PASS for software builds, Python tests and verified artifacts. Physical LoRa and C runtime tests remain unverified.**

| Check | Verified result |
|---|---|
| Existing Python suite | 31/31 PASS |
| Hybrid router Python reference | 13/13 PASS |
| Root BLE default/topology firmware | Incrementally rebuilt after Task018 correction; both match validated Task012 UF2 SHA256 exactly |
| Standalone LoRa transport C test | Clean compile/link PASS; no runtime |
| Standalone hybrid router C test | Clean compile/link PASS; no runtime |
| Basic and experimental SX1262 overlays | Clean compile/link PASS with native SX1262 driver |
| Task018 optional no-echo GATT firmware | Separate pristine compile/link PASS; feature enabled in Kconfig, **not flashed** |
| Task018 GATT policy C test app | Independent pristine compile/link PASS (205/205), **not executed** |
| Artifact checker | 15/15 PASS: `tests/task018_verify_artifacts.py` |

**Audit trail:** The first full `tests/pc_ci.py --full` pass completed all six build targets successfully, but flagged default/topology checksum changes and two false-negative SX1262 DTS checks. We fixed the feature-OFF compile-time guard and extended the DTS parser window, then incrementally rebuilt both BLE images, recovering original validated Task012 checksums byte-for-byte. No second *full pristine* 6-target pass occurred after these corrections. Its original logs remain `docs/ci/20261019T030959Z/`. Subsequent root rebuild logs: `docs/task018-off-default-rebuild.log`, `docs/task018-off-topology-rebuild.log`. New pristine Task018 opt-in and policy builds: `docs/task018-noecho-independent.log`, `docs/task018-gatt-policy-ztest-independent.log`.

See `docs/TASK014-INDEPENDENT-REVIEW.md`, `docs/TASK015-INDEPENDENT-REVIEW.md`, `docs/TASK017-SX1262-RF-FRONTEND-EXPERIMENT.md`, and `docs/TASK018-DISCOVER-CONTROL-NO-ECHO.md`.

**Safety:** PC-only; no new firmware flashed to A/B/C, no resets or LoRa RF TX. Actual SX1262 initialization/RX/TX, mixed physical radio routing and updated GATT no-echo delivery remain pending future specifically approved hardware tests, plus explicit user country/channel choice.
