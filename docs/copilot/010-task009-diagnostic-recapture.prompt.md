# Task 009 timestamped diagnostic recapture

Run one evidence-only Task 009 A→B→C diagnostic retry using
`tests/task009_capture_c_console.py`, which captures C's serial output and runs
`tests/ble_pc_controlled_two_hop.py`.

Requirements:

- Do not flash, erase, reset, or alter any board unless the user gives fresh,
  explicit approval for that action.
- Keep the payload unique for this run. Preserve the separate C console log
  and combined output with monotonic timestamps.
- Report A GATT write/echo, matching A TTL3 advertisement, matching B TTL2
  advertisement with the same source and sequence, and C GATT TX read as
  separate observations. Do not treat old payloads/sequences, a GATT echo, or
  an `ACCEPT` console fragment as proof of fresh on-air delivery.
- If the runner fails, state precisely which evidence is absent. Do not change
  firmware policy or advertising timing without complete C role, packet type,
  source, sequence, TTL, and GATT publication evidence.
- Save the raw logs, document commands and actual results in Markdown, and
  update `docs/PROGRESS.md`. Leave Phase 3 open unless the controlled
  acceptance criteria all pass.
