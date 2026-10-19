# TASK047 — RSSI NULL output tests

- **Scope:** Add deterministic checks for NULL packet and RSSI output pointers
  to the existing BLE mesh RSSI validation test, verifying the other output is
  preserved.
- **Files changed:** `tests/ble_mesh_packet/src/main.c`,
  `docs/TASK047-RSSI-NULL-OUTPUTS.md`, `docs/PROGRESS.md`.
- **Commands/checks:** Targeted search of the existing RSSI test found no NULL
  output-pointer coverage. Read the test and helper header. No build or test
  commands run, as requested.
- **Results:** Added two calls using the existing valid manufacturer frame and
  existing ztest assertions. Each expects `MALFORMED` and checks preservation
  of the non-NULL output.
- **Issues:** Runtime behavior remains unverified; stopped for review.
- **Rollback:** Remove the two added assertions in
  `tests/ble_mesh_packet/src/main.c`, this document, and the TASK047 entry in
  `docs/PROGRESS.md`.
- **Next action:** Independent review; run no build, tests, or hardware actions
  until requested.
