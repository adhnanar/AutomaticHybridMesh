# Task 009 independent review and shared evidence reconciliation

## Scope and verdict

Independently reviewed the Task 009 controlled A → B → C test policy, its
test/build evidence, the previously captured Task 008 hardware observations,
and compliance with [AGENTS.md](../AGENTS.md). No firmware source, configuration,
or hardware was changed in this review.

**Verdict: PASS for feature organization, default-off policy, independent
production and verified topology builds, Python checks, and physical
Kit-to-ID mapping. Phase 3 acceptance remains OPEN.** No C ztest runtime
result or Task 009 physical acceptance is established.

## Findings

- **Feature-level architecture: PASS.** Relay and topology implementation,
  including its public headers, are grouped under
  `src/features/ble_mesh_relay/`. The C ztest application and its CMake
  configuration are under `tests/ble_mesh_packet/`; the PC acceptance runner
  and Python evidence tests are in the existing `tests/` suite. The topology's
  Zephyr Kconfig symbols are in the root `Kconfig`, and the phase-specific
  documentation is in `docs/PHASE3-CONTROLLED-TWO-HOP.md`. This is consistent
  with the project rule to organize by feature where practical and does not
  introduce a monolithic `main.c`.
- **Every-task Markdown rule: PASS.** The phase report records scope, changed
  files, commands and actual results, issues, rollback, and next action;
  `docs/PROGRESS.md` records current status and leaves Phase 3 acceptance open.
  This review and the progress entry have now been synchronized with the
  additional independent results below.
- **Default production behavior: PASS.** A fresh production build's
  `zephyr/.config` states `# CONFIG_BLE_MESH_TEST_TOPOLOGY is not set`.
  The controlled policy is gated by that Kconfig option.
- **Controlled path logic: PASS by source inspection.** In test mode, the
  receive path filters TEXT before the common deduplication, peer publication,
  and GATT TX work. Role B accepts A's TTL 3 packet and queues one relay;
  role C rejects A's TTL 3 packet and accepts TTL 2 without relaying.
  Beacons follow the existing path. TTL is not authentication and does not
  prove physical relay identity or RF isolation.
- **No implementation defect found in this scoped review.** The remaining
  gaps are evidence and environment blockers, not established source defects.

## Independent commands and actual results

### Clean production build

First reran the clean build in the earlier `build-independent-009` directory:

```powershell
Set-Location C:\zephyrproject
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-independent-009 C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

- **FAIL, exit 1.** CMake reconfiguration failed when Ninja reported
  `failed recompaction: Permission denied`. The exact cause of that filesystem/
  Ninja failure is undetermined. CMake also reported Dtc missing; this warning
  was not the fatal error.
- The pre-existing
  [`task009-independent-build.log`](./task009-independent-build.log) ends
  during configuration and contains no successful completion result. It is
  therefore not counted as build-pass evidence; the retry outcomes above
  supersede its incomplete transcript.

Retried pristine in a separate build directory:

```powershell
Set-Location C:\zephyrproject
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-independent-009-review C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

- **PASS, exit 0, 268/268 steps.** FLASH 133,344 B; RAM 30,408 B; UF2
  266,752 bytes. SHA256:
  `68FCD334628272F092BB094244B67D8B0514B27C88FD736627D96699C3FBA350`.
- The generated configuration confirms `CONFIG_BLE_MESH_TEST_TOPOLOGY` is
  unset. The missing-Dtc configure notice remained non-fatal.
- This is compile/link evidence only; no flash or runtime behavior is implied.

### Python tests

```powershell
Set-Location C:\zephyrproject
& .\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p 'test_*.py' -v
& .\.venv\Scripts\python.exe -m py_compile C:\zephyrproject\hybrid-mesh-poc\tests\ble_pc_controlled_two_hop.py C:\zephyrproject\hybrid-mesh-poc\tests\test_ble_pc_controlled_two_hop.py
```

- **PASS**, unittest exit 0, 6/6 tests: three advertisement decoder tests and
  three controlled-two-hop acceptance-evidence tests.
- **PASS**, `py_compile` exit 0 for the acceptance runner and its tests.

### Verified topology build and identity

The live evidence in
[TASK009-HARDWARE-IDENTITY.md](./TASK009-HARDWARE-IDENTITY.md) matches
manufacturer beacon IDs to the final six USB serial bytes in reverse order:
COM14 / Kit 1 / E5:12:76:D2:56:EA → `eb34ed25a5a4`; COM15 / Kit 2 /
C8:51:08:6E:08:C0 → `0c13dae125c8`; COM16 / Kit 3 /
E2:DB:6A:54:BB:71 → `ee07145a5e02`. The match establishes A=Kit 1,
B=Kit 2, C=Kit 3 without resetting or flashing the devices.

Inspected the completed clean build transcript at
[`task009-verified-topology-build.log`](./task009-verified-topology-build.log):
it records 268/268 steps, FLASH 134,496 B, RAM 30,408 B, and generation of a
269,312-byte UF2. The generated `zephyr/.config` confirms
`CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and
`CONFIG_BLE_MESH_TEST_B_ID_0C13=y`. The UF2 SHA256 is
`093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`.
Because the captured clean transcript did not include the invoking shell's
exit status, a non-pristine verification build was run in the same build
directory; it returned exit code 0 (`ninja: no work to do`). The transcript's
missing-Dtc notice was non-fatal. This verifies compile/link/configuration
only; it does not establish radio behavior or authorize flashing.

Verification command executed from `C:\zephyrproject`:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-test-topology-verified C:\zephyrproject\hybrid-mesh-poc
```

The command returned **exit code 0** (`ninja: no work to do`). The artifact
hash was checked with:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath C:\zephyrproject\hybrid-mesh-poc\build-test-topology-verified\zephyr\zephyr.uf2
```

The generated configuration was checked for
`CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and
`CONFIG_BLE_MESH_TEST_B_ID_0C13=y`.

### C ztests

The existing Task 009 record reports the C ztest application compiled and
linked for `xiao_ble/nrf52840` (200/200 build steps). The recorded Twister
`native_sim` attempt selected no runnable configuration (0 built, 0 executed);
the available host compiler was reported missing. These C cases have **not**
run. Compiling the test firmware is not a C test pass.

## Shared historical hardware evidence

These observations predate Task 009 and came from Task 008 firmware. They show
useful BLE relay behavior but do not count as Task 009 acceptance.

| Test | Observed result | What it establishes / does not establish |
|---|---|---|
| `AIR008B`, source E5:12:76:D2:56:EA | Another node, E2:DB:6A:54:BB:71, advertised the same A-originated text at TTL 2; both other GATT TX endpoints received it. | Cross-board distribution succeeded. It was not a forced two-hop test; direct RF reception was not excluded. |
| `SEP3A01`, source E5 | C8:51:08:6E:08:C0 advertised the text at TTL 2; its GATT delivery passed. E2's GATT TX did not match and retained `FRURT07`. | A relay advertisement was observed, but endpoint delivery was incomplete. |
| `SEP3B02`, source E2 | C8 GATT delivery passed; E5 GATT TX did not match and retained `SEP3A01`. No other-node TTL 2 advertisement was captured. | Reverse-direction endpoint delivery was also incomplete; this is not evidence of an A → B → C path. |
| USB / BLE identity | Live BLE manufacturer beacon IDs were matched to USB serial FICR suffixes: COM14 / Kit 1 / E5:12:76:D2:56:EA → `eb34ed25a5a4`; COM15 / Kit 2 / C8:51:08:6E:08:C0 → `0c13dae125c8`; COM16 / Kit 3 / E2:DB:6A:54:BB:71 → `ee07145a5e02`. The last 12 serial digits encode the six-byte ID in reverse byte order. | Physical identity and A/B/C selection are established without reset or flash. This is not Task 009 runtime or two-hop acceptance evidence. |

No board was flashed, reset, erased, or tested over BLE for Task 009. The
Task 009 physical acceptance runner was **not run**. The Task 009 policy is
not established as running on the kits, and no end-to-end A → B → C physical
acceptance is claimed.

## Blockers and next action

1. Obtain an environment in which the C ztests actually execute, or preserve
   their runtime status as pending.
2. Obtain separate explicit authorization before flashing Task 009 firmware.
   Then configure the verified A/B/C roles and run the documented PC
   acceptance test with a physically controlled path that excludes direct
   A-to-C reception.
3. Keep Phase 3 incomplete until B's matching TTL 2 advertisement and C's
   GATT delivery are observed under that controlled setup.

## Original independent review change record

- Changed only `docs/TASK009-INDEPENDENT-REVIEW.md` and
  `docs/PROGRESS.md`; no firmware, Kconfig, test, or hardware changes.
- The independent build generated validation artifacts under
  `build-independent-009-review/`; these are build outputs, not implementation
  changes.
- Rollback for this review is limited to restoring the prior contents of those
  two Markdown files. There is no firmware or board rollback associated with
  this review.
- The physical Kit 2/Kit 3 mapping blocker is resolved by the live evidence in
  [TASK009-HARDWARE-IDENTITY.md](./TASK009-HARDWARE-IDENTITY.md), and the
  verified topology build is recorded above. Physical acceptance remains
  pending; do not flash without separate user approval.

## 2026-10-18 identity/build evidence update

- **Scope:** reconcile the Task009 report and Phase 3 report with the attached
  independent hardware identity evidence; inspect the completed verified
  topology build; preserve the separate flash-authorization gate.
- **Files changed:** `docs/TASK009-INDEPENDENT-REVIEW.md`,
  `docs/PHASE3-CONTROLLED-TWO-HOP.md`, and `docs/PROGRESS.md`. The attached
  `docs/TASK009-HARDWARE-IDENTITY.md` and captured build log were inspected,
  not modified.
- **Commands and actual results:** inspected
  `docs/task009-verified-topology-build.log` (268/268 steps, FLASH 134,496 B,
  RAM 30,408 B, UF2 269,312 B); checked the generated Kconfig values for the
  test policy and Kit 2 B ID; hashed the UF2
  (`093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`);
  reran the configured build non-pristinely and observed exit code 0. No
  Python or C tests were rerun, no board was flashed, and no physical
  acceptance test was run.
- **Issues:** the original clean-build transcript did not capture the shell
  exit status; the same configured build directory passed an explicit
  non-pristine verification. C ztest runtime and physical A→B→C acceptance
  remain pending.
- **Rollback:** documentation-only; restore the previous contents of the
  three changed Markdown files. No firmware, configuration, build output, or
  hardware rollback is needed.
- **Next action:** run C ztests in a supported host environment; do not flash
  without separate explicit user approval. After approval, run the documented
  controlled BLE acceptance test and keep Phase 3 open until its required
  on-air and GATT evidence is collected.
