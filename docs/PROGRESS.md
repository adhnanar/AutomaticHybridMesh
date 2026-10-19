# Seven-phase PoC progress

- Task049 (2026-10-19): added a pure local-acceptance classifier and compact
  table-driven ztest for the hybrid-router outcomes. No GATT/router wiring,
  callbacks, radio/configuration, build, tests, or hardware activity. Peer
  delivery is not claimed. **STOP for independent review; reviewer runs
  builds/tests.** See
  [TASK049-LOCAL-ACCEPTANCE-CLASSIFIER.md](TASK049-LOCAL-ACCEPTANCE-CLASSIFIER.md).
- Task048 (2026-10-19): specified the opt-in Phase 5 phone-ingress queue/status test gate; no implementation, build, or tests. **STOP for independent review.** See [TASK048-PHASE5-SAFE-ROUTING-GATE.md](TASK048-PHASE5-SAFE-ROUTING-GATE.md).
- Task046 (2026-10-19): extended the existing BLE RSSI validation test for NULL/short input preservation and exact -85/-86 RSSI decoding; no build/tests run. See [TASK046-RSSI-BOUNDARY-TEST.md](TASK046-RSSI-BOUNDARY-TEST.md). **STOP for independent review.**
- Task047 (2026-10-19): added deterministic BLE RSSI validation checks for NULL packet/RSSI output pointers and preservation of the other output; no build/tests run. See [TASK047-RSSI-NULL-OUTPUTS.md](TASK047-RSSI-NULL-OUTPUTS.md). **STOP for review.**

- [ ] Task042 — PC-only opt-in `CONFIG_HYBRID_ROUTER_POLICY` RSSI feed: private
  no-radio router init (zeroed ops, LoRa TX unauthorized) and RX-worker call to
  `hybrid_router_ble_peer_seen`. Not built/tested. **STOP for review.** See
  [TASK042-RSSI-ROUTER-FEED.md](./TASK042-RSSI-ROUTER-FEED.md).

- [ ] Task041 — PC-only BLE RX RSSI queue preservation. Store scanner RSSI
  alongside successfully decoded manufacturer mesh frames and carry it through
  the private queue to the RX worker. No routing, deduplication, GATT,
  advertising, phone, or malformed-frame behavior changes. No build/tests,
  hardware, flash, reset, or RF activity. **STOP for independent review.** See
  [TASK041-BLE-RSSI-QUEUE.md](./TASK041-BLE-RSSI-QUEUE.md).

- [ ] Task040 — PC-only hybrid BLE integration contract. Documented the
  existing BLE receive/GATT/router callback boundary, RSSI semantics,
  Android-text routing and connected-phone advertising backpressure; defined
  the smallest follow-up code change and five deterministic acceptance tests.
  Documentation only: no implementation, build/test, hardware, flash, reset,
  USB/power, download/install, or RF activity. **STOP for independent review.**
  See [TASK040-HYBRID-BLE-INTEGRATION-CONTRACT.md](./TASK040-HYBRID-BLE-INTEGRATION-CONTRACT.md).

- [ ] Task039 — PC-only C decision-policy runtime tests and simulator parity.
  Extended the shared vectors for inclusive BLE freshness/RSSI boundaries,
  added a C ztest for weak reports and peer-lost behavior, and aligned the
  Python simulator's retry limit with the C router (three unsuccessful
  retries). Python router reference tests passed 15/15, policy simulator
  tests 16/16, repository discovery 55/55, `py_compile` passed, and the
  static TX check passed. Existing Task012 and Task038 default UF2 artifacts
  have the same SHA-256
  (`2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`);
  existing configs show LoRa and the Task038 router policy disabled. The C
  ztest build stopped before compilation because sandbox access to
  `C:\Users\adhna\zephyr-sdk-1.0.1\cmake\Zephyr-sdkConfig.cmake` was denied.
  **C runtime tests remain unexecuted and acceptance is OPEN.** No Task036
  BLE or Kit1 setup was changed; no hardware, flash, reset, or RF operation
  occurred. **STOP for independent review.** See
  [TASK039-C-POLICY-RUNTIME-TESTS.md](./TASK039-C-POLICY-RUNTIME-TESTS.md)
  for exact commands/results, limitations, rollback, and next action.

- [ ] Task038 — PC-only hybrid policy C foundation. Adapted the isolated
  `hybrid_router` to BLE-first selection, recent RSSI-qualified BLE peer
  state, explicit externally granted LoRa TX authorization, and a distinct
  blocked-but-queued outcome. Added default-off `CONFIG_HYBRID_ROUTER_POLICY`
  and shared policy vectors consumed by Zephyr ztests and Python parity tests.
  The focused policy suite passed 16/16, router reference suite 14/14,
  repository Python suite 55/55, `task024_static_tx_check.py` passed, and
  changed Python sources passed `py_compile`. C ztests and firmware builds
  were not run: system Python lacks `west`, no workspace venv was present,
  and checking the external venv path
  was sandbox-denied. Existing locked config still reports
  `# CONFIG_LORA is not set`; no Kconfig TX lock was changed. No radio, flash,
  reset, or hardware operation occurred. The C ztest and default-image
  configuration are unverified; no binary hash is claimed. **STOP for
  independent review.** See
  [TASK038-HYBRID-POLICY-C-FOUNDATION.md](./TASK038-HYBRID-POLICY-C-FOUNDATION.md).

- [ ] Task031 — PC-only opt-in BLE Android discovery-name foundation. Added a
  default-off `Mesh-<node-ID-suffix>` advertising name and formatter tests;
  retained the existing Task018 default-off DISCOVER no-echo implementation.
  Task012 baseline UF2 hashes are unchanged. The Zephyr test/build attempt was
  blocked by a sandbox SDK access denial; fresh configurations also failed
  before generation because ZEPHYR_BASE was not set in the environment. No C
  tests executed and no candidate firmware produced. Cleanup of partial build
  dirs was also denied. Android evidence is OPEN. **STOP for independent review.**
  See [TASK031-BLE-ANDROID-UX-FOUNDATION.md](./TASK031-BLE-ANDROID-UX-FOUNDATION.md).

- [x] Task030 — PC-only BLE/Android nRF Connect discovery + text messaging gap
  analysis. Independent review corrected deployment state: Kit1/COM14 runs
  Task022 LoRa RX-only, while Kit2/3 retain BLE topology firmware. The A→B→C
  Android demo is NOT possible on that arrangement; Kit1 BLE restoration needs
  explicit user approval. Android behavior remains unverified. See
  [TASK030-BLE-ANDROID-POC-GAP-ANALYSIS.md](./TASK030-BLE-ANDROID-POC-GAP-ANALYSIS.md)
  and [TASK030-INDEPENDENT-REVIEW.md](./TASK030-INDEPENDENT-REVIEW.md).

- [ ] Task020 — PC-only UK LoRa policy hardening: sticky clock-fault reset
  contract, reservation settlement, token wrap, and ERP arithmetic hardened
  with focused ztests. The feature remains standalone and the default gate
  remains locked. Both configured ninja rebuilds were blocked by sandbox
  access denial to the installed external Zephyr SDK; ztest execution and
  physical airtime upper bound remain OPEN. Software-only reservation is not
  authority to integrate or transmit. No board interaction occurred. See
  [TASK020-UK-LORA-POLICY-HARDENING.md](./TASK020-UK-LORA-POLICY-HARDENING.md)
  for actual commands/results, review verdict, constraints, rollback, and next
  action.

- [ ] Phase 1 — Hardware & Firmware Setup (Kit 1 boot verified; radio and remaining kit checks incomplete)
- [ ] Phase 2 — Android BLE Connection
- [ ] Phase 3 — BLE Mesh (custom advertising relay implemented; radio acceptance pending)
- [ ] Phase 4 — LoRa Mesh (transport groundwork implemented; module and radio validation pending)
- [ ] Phase 5 — Automatic Hybrid Routing
- [ ] Phase 6 — Phone Notifications & Discovery
- [ ] Phase 7 — Real-World Testing

## Latest verification

- 2026-10-19: TASK014 adds an independent, opt-in Zephyr app and explicit
  overlay for the Seeed XIAO nRF52840 + Wio-SX1262 SKU 102010710. The profile
  maps SPI2, CS, RESET, BUSY and DIO1; disables default I2C1 because D4/D5
  conflict with its pins; and uses Zephyr's experimental native SX126x
  driver in a standalone boot-readiness smoke target. The default BLE app,
  root config/CMake, and topology code are unchanged; no LoRa API call to
  configure, receive or send is made by the smoke app. Independent upstream
  binding/Kconfig research confirmed driver properties, but Seeed's
  published schematic/forum materials do not establish module-side
  RXEN polarity, DIO2 RF switch wiring, or TCXO voltage/timing sufficiently
  to configure those signals safely, so they are left unset with TODOs.
  The operator subsequently confirmed Wio-SX1262 modules and antennas on
  all three boards and identified the UK as the intended region; exact
  allowed RF settings and manufacturer switch/TCXO details remain open.
  Clean default/profile builds, generated DTS/Kconfig inspection, and
  driver-link verification were **not run**: the sandbox denied access to
  the installed Zephyr checkout outside the workspace. No flash, board
  action, or RF activity occurred. The profile is authored but build and
  hardware acceptance remain OPEN; see
  [TASK014-XIAO-SX1262-OVERLAY.md](./TASK014-XIAO-SX1262-OVERLAY.md) for
  exact intended commands, actual results, limitations, rollback, and
  operator questions, and the
  [copy-ready TASK014 validation prompt](./copilot/014-xiao-sx1262-hardware-profile.prompt.md).

- 2026-10-19: TASK013 Phase 4 LoRa transport groundwork implemented in an
  isolated feature and standalone ztest app. Reuses the existing 24-byte
  packet encoding, adds an injected frame-send contract and bounded
  in-memory mock, and tests shared-cache duplicate admission, TTL decrement,
  and send-error propagation. Existing BLE sources, active routing, root
  application CMake, and default devicetree/config remain unchanged; no LoRa
  module is confirmed and the feature is not linked into the application.
  Clean default and topology XIAO builds passed (FLASH/RAM 134,072/30,472 B
  and 136,108/30,536 B); standalone LoRa ztest app compiled/linked 202/202
  steps. Twister `native_sim` did not execute tests: both normal and `-j 1`
  attempts stopped before execution with `FileNotFoundError: [WinError 2]`
  in its multiprocessing manager. Existing Python suite passed 31/31. No
  device access, power, reset, flash, or RF test; no hardware or runtime
  LoRa claim. Cross-radio cache sharing is tested as a caller-shared contract
  only, not wired into BLE. See
  [TASK013-LORA-TRANSPORT-FOUNDATION.md](./TASK013-LORA-TRANSPORT-FOUNDATION.md)
  for audit, exact commands/results, hardware checklist, limitations,
  independent review, rollback, and next step. Ask for the exact external
  module and wiring before any driver or radio-resource integration. The
  copy-ready follow-up is
  [013 phase 4 module confirmation prompt](./copilot/013-phase4-lora-module-confirmation.prompt.md).

- 2026-10-19: Task011 prepared a PC-only B-off baseline mode, raw
  UTC/monotonic BLE and A/B/C USB console capture, and an offline evidence
  evaluator for sensitivity/isolation/relay controls. Source audit confirms C
  filters A-origin TEXT to TTL2, while firmware discards the BLE advertiser
  address; therefore Task010's 2/2 result does not prove A-C RF isolation.
  Baseline success is not accepted without intact C receive logs and a
  zero decoded-text heartbeat delta; positive trials require B receive/queue,
  observed TTL2 from B's BLE address, C TTL2 acceptance/publication and no
  direct A/TTL3 reject. Python tests passed 27/27 and changed files passed
  `py_compile`. No physical test, serial access, reset, power change, flash or
  firmware build was performed. See
  [TASK011-TRUE-TWO-HOP-ISOLATION.md](./TASK011-TRUE-TWO-HOP-ISOLATION.md)
  for commands, results, limitations, rollback and requested setup.

- 2026-10-18: Implemented Task 009 test-topology-only feature diagnostics and
  focused tests. B/C TEXT records now correlate event, type, source, sequence,
  TTL, and firmware uptime; advertising updates are explicitly reported as
  API results, failed relay updates retain the queue head and remain
  rate-limited, and C receive decode/queue-drop counters are reported from
  work context. Accepted C TEXT records the GATT publish return; TX storage
  remains independent of notification availability. Removed topology
  per-beacon accept logging. No topology policy, 500 ms text hold, five-second
  beacon interval, or one-second retry interval changed. All host unit tests
  passed 11/11 and changed Python files passed `py_compile`. Clean default
  firmware build passed (FLASH 133,472 B, RAM 30,408 B; UF2 267,264 B,
  SHA-256 `20039A78FB135B567667EC2BB2A3CC836710C092974B9021CE4280428CA47AD0`);
  separate clean topology-enabled build passed (FLASH 134,916 B, RAM 30,408
  B; UF2 270,336 B, SHA-256
  `04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142`).
  Existing `ble_mesh_packet` ztest app compiled and linked 203/203 steps,
  but Twister statically filtered native_sim and executed zero C test cases,
  even with forced platform selection. No board was flashed, reset, or
  accessed; no live BLE or delivery success is claimed. Phase 3 remains
  OPEN. Physical retry requires explicit approval for board interaction, and
  flashing requires separate explicit approval. See
  [TASK009-FEATURE-DIAGNOSTICS.md](./TASK009-FEATURE-DIAGNOSTICS.md) for
  changed files, exact commands/results, runtime-test limitation, rollback,
  and next action.

- 2026-10-18: Follow-up review of the corrected Task 009 recapture supersedes
  the immediately preceding entry's statement that no A on-air frame was
  observed: this run did observe A source `eb34ed25a5a4`, sequence 154, TTL 3,
  with fresh payload `HOP00971595B15`. It did not observe matching B TTL2 in
  the PC scan; C GATT TX remained `Ready`. C topology `ACCEPT` console
  fragments remain concatenated/interleaved despite host-side newline
  buffering and do not identify type, sequence, or TTL. Source review finds
  B retains queued text and retries failed advertisement-data updates, but
  this run has no B console to distinguish receive, queue, update, or scanner
  outcomes. C's `-11` was a beacon update during an active GATT connection and
  does not establish a text failure. No firmware changes, tests/build, serial
  access, or hardware operations were performed. Phase 3 remains open. See
  [TASK009-RECAPTURE-REVIEW.md](./TASK009-RECAPTURE-REVIEW.md) and the
  copy-ready [011 diagnostic prompt](./copilot/011-task009-feature-diagnostics.prompt.md)
  for the minimal feature-level diagnostics/tests proposed.

- 2026-10-18: Reviewed the new independent Task 009 capture. The fresh payload
  `HOP0095F2CC5ED` was written to A and echoed by A, but the PC scanner
  recorded no matching A TTL3 or B TTL2 frame, and C's final GATT TX read was
  still `Ready`. The C console contains truncated/interleaved topology
  `ACCEPT` fragments without complete packet type/sequence/timestamp; they
  cannot be treated as fresh text evidence. `-11` is `-EAGAIN` from the GATT
  helper when a connection is active, consistent with the adjacent C GATT
  connection log; it was a beacon update and does not prove a fresh text
  transmission failure. Fixed the diagnostic harness's 300 ms `readline()`
  partial-record handling with newline buffering and monotonic timestamps;
  acceptance events now use the same monotonic clock. Added tests ensuring
  split UART records remain intact and prior-run frames with a different
  payload do not satisfy fresh acceptance. Python tests passed 10/10, bytecode
  compilation passed, and the Zephyr build command exited 0 (`ninja: no work
  to do`; no firmware source changed). No hardware was accessed or flashed.
  Phase 3 remains open pending a corrected timestamped capture and actual
  A→B→C evidence. See
  [TASK009-LIVE-DIAGNOSTIC-CAPTURE.md](./TASK009-LIVE-DIAGNOSTIC-CAPTURE.md)
  for exact evidence, commands/results, issue assessment, rollback, and next
  action.

- 2026-10-18: Task 009 live acceptance was run after the three kits were
  flashed under user approval. A's GATT write/source echo passed; the PC
  observed A's source `eb34ed25a5a4`, sequence 33, TTL 3 and B forwarding the
  same source/sequence/payload at TTL 2. C's GATT TX read remained `Ready`,
  so controlled A→B→C acceptance **failed** and Phase 3 remains open. The
  available PC log does not contain C-side scan, packet acceptance/rejection,
  role, or GATT publication diagnostics. Source inspection finds that the
  configured C policy should accept A/TTL2, store the text in GATT TX, and
  not relay it; root cause is therefore undetermined, not established as a
  topology-predicate defect. No source/configuration was changed and no
  hardware was accessed during this investigation; no tests/build were run.
  See [TASK009-LIVE-ACCEPTANCE-INVESTIGATION.md](./TASK009-LIVE-ACCEPTANCE-INVESTIGATION.md)
  for raw observations, scan/GATT analysis, proposed feature-level diagnostics
  and tests, rollback, and next action. This live result supersedes earlier
  progress entries stating that Task 009 had not yet been flashed; those
  entries record the status at their respective review times.

- 2026-10-18: Task 009 controlled two-hop policy implemented, disabled by
  default, with explicit compile-time B-ID selection, reject-before-dedup
  predicate, work-context diagnostics, C ztests, and PC-side A→B→C acceptance
  script. Independent review confirmed feature-level organization, the
  default-off setting, and filter-before-dedup path. A fresh independent
  production build passed 268/268 (FLASH 133,344 B, RAM 30,408 B; UF2
  266,752 B, SHA256
  `68FCD334628272F092BB094244B67D8B0514B27C88FD736627D96699C3FBA350`). The
  existing independent-build log is incomplete; rerunning in
  `build-independent-009` failed during Ninja recompaction with
  `Permission denied`; a pristine retry in
  `build-independent-009-review` passed. Test-policy build is also recorded
  as PASS (268/268; test-only UF2 269,312 B, SHA256
  `093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`); its
  selected B ID by itself was compile-only evidence and did not establish
  physical Kit 2/3 mapping at that review point; the later live identity
  evidence below resolves the mapping. Python tests independently reran PASS
  6/6 and acceptance scripts
  passed `py_compile`. C ztest app compile PASS (200/200), but Twister
  `native_sim` selected 0 configurations: no C cases executed. No Task 009
  board flash or physical BLE test occurred. Shared Task 008 evidence confirms
  cross-board relay but not forced two-hop; separated tests show inconsistent
  endpoint delivery. At that review point Kit 2/3 BLE-to-mesh-ID mapping was
  unresolved; the later live identity evidence below resolves it.
  Phase 3 acceptance remains pending. See
  [PHASE3-CONTROLLED-TWO-HOP.md](./PHASE3-CONTROLLED-TWO-HOP.md) for the
  implementation record and
  [TASK009-INDEPENDENT-REVIEW.md](./TASK009-INDEPENDENT-REVIEW.md) for
  reconciled review, commands, hardware evidence, blockers, and next actions.

- 2026-10-18: Task 009 physical mapping independently established from live
  BLE manufacturer beacon IDs matched against USB serial FICR suffixes:
  COM14/Kit 1/E5 → `eb34ed25a5a4`, COM15/Kit 2/C8 → `0c13dae125c8`,
  COM16/Kit 3/E2 → `ee07145a5e02`. This resolves the prior B/C identity
  blocker without reset or flash. The clean topology-enabled build in
  `build-test-topology-verified` completed 268/268 steps (FLASH 134,496 B,
  RAM 30,408 B; UF2 269,312 B). Generated configuration confirms
  `CONFIG_BLE_MESH_TEST_TOPOLOGY=y` and
  `CONFIG_BLE_MESH_TEST_B_ID_0C13=y`; UF2 SHA256 is
  `093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83`.
  A non-pristine verification build returned exit 0 (`ninja: no work to do`).
  This is build evidence only. No Task 009 firmware has been flashed, and
  controlled A→B→C physical acceptance remains pending separate explicit
  flash authorization. See
  [TASK009-HARDWARE-IDENTITY.md](./TASK009-HARDWARE-IDENTITY.md) and
  [TASK009-INDEPENDENT-REVIEW.md](./TASK009-INDEPENDENT-REVIEW.md).

## Verification log

- 2026-10-18: Task 008 reliable Phase 3 mesh transmit correction implemented and independently reviewed. The relay now peeks the pending text queue, only removes the head after `ble_gatt_update_mesh_advertisement` succeeds, and retains/retries it after `-EAGAIN` or other errors using a 1-second delay (no busy loop or blocking workqueue). The disconnect advertising restart now restores the latest requested mesh frame instead of replacing it with base data. Retry logs are rate-limited; the 500 ms text hold and 5-second idle beacon schedule remain. GATT TX echo, TTL, and deduplication code were not changed. Updated `tests/ble_pc_air_message.py` to separately report source GATT echo, matching on-air text frames by node address, and remote GATT TX delivery; added three host decoder unit tests. Clean build command `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always` completed successfully, 267/267 steps; FLASH 133,344 B, RAM 30,408 B, UF2 266,752 B. `& C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s C:\zephyrproject\hybrid-mesh-poc\tests -p test_ble_pc_air_message.py -v` passed 3/3 tests; `py_compile` passed. No board flashed. BLE on-air and remote relay acceptance remain unverified. Details, independent review verdict, rollback, actual commands/results and next step: [PHASE3-RELIABLE-MESH-TRANSMIT.md](./PHASE3-RELIABLE-MESH-TRANSMIT.md).

- 2026-10-18: Phase 3 connection recovery patch implemented for the reconnection/lifecycle issue. The BLE GATT layer now logs connection and disconnect events, tracks active connections, defers mesh advertisement updates while connected, and restarts legacy advertising after a disconnect with a bounded delayable-work retry path instead of calling host APIs from callback context. The conservative fix preserves the custom GATT service UUIDs and packet logic without claiming a proven root cause. Pristine `xiao_ble/nrf52840` build evidence: `Set-Location C:\zephyrproject; & .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always` completed with exit code `0`. No board was flashed or reset. Remaining hardware validation and radio acceptance are still pending. Details and build evidence: [PHASE3-CONNECTION-RECOVERY.md](./PHASE3-CONNECTION-RECOVERY.md).

- 2026-10-18: Phase 3 custom BLE advertising relay implemented for Zephyr 4.5.0-rc2. A pristine `xiao_ble/nrf52840` build completed with exit 0 (131,268 B flash, 30,344 B RAM; UF2 and ELF generated); the packet ztest application also compiled and linked for the board. Six parser/TTL/dedup tests are present but not executed: Twister ran zero configurations, and direct `native_sim` configuration failed because host `gcc` is not installed. No flash or radio validation was performed. SIG Mesh was rejected for this phase because its provisioning/AppKey/GATT Proxy model does not fit the generic nRF Connect GATT-only interface; the implemented relay is custom and not SIG Mesh. Independent source/config review: compile and bounded logic PASS; radio acceptance PENDING. Details, exact commands, test results, rollback, and next action: [PHASE3-BLE-MESH.md](./PHASE3-BLE-MESH.md).

- 2026-10-18: Phase 2 modular BLE GATT clean build verified for `xiao_ble/nrf52840`. The final command was `& C:\zephyrproject\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always`; exit code `0`, all `262/262` steps completed, with `zephyr.uf2`, `zephyr.elf`, and `zephyr.hex` generated. No Phase 2 flash or Android test has been performed. Phase 2 remains incomplete pending physical nRF Connect validation. Details and UUIDs: [PHASE2-BLE-GATT.md](./PHASE2-BLE-GATT.md).

- 2026-10-18: Clean build verified for `xiao_ble/nrf52840` in the fresh project. Command used: `west build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always`. Exit code `0`; all `192/192` build steps completed, and `zephyr.uf2`, `zephyr.elf`, and `zephyr.hex` were generated.
- 2026-10-18: USB / serial enumeration was inspected. Only a Bluetooth virtual COM port (`Standard Serial over Bluetooth link (COM6)`) was present; there was no actual Seeed XIAO nRF52840 USB device or USB-CDC board enumerated. No physical board identity or boot evidence could be confirmed.
- 2026-10-18: Zephyr support was checked non-destructively. The installed tree includes `xiao_ble` support and the generic `semtech_sx1262mb2das` shield, but no physical Wio-SX1262 or XIAO board was present, so no pin mapping or radio support could be validated.
- 2026-10-18: Phase 1 stopped at the physical-connection gate because a real board is absent. No flashing, no bootloader changes, and no radio configuration were performed.
- Detailed audit and exact commands: [PHASE1-BASELINE.md](./PHASE1-BASELINE.md).

## Current blocker

Phase 1 remains incomplete: Kit 1 boot was verified after an approved Phase 1 flash, but radio functions and remaining kits have not been tested. The Phase 2 GATT image has only compile/link evidence and requires separate user approval before flashing and nRF Connect validation.


- 2026-10-18: With user approval, verified XIAO nRF52840 UF2 bootloader identity on D:, copied fresh zephyr.uf2, observed COM14 re-enumerate and Zephyr 4.5.0-rc2 boot message plus beginning of new app startup line. Kit 1 boot verified; radio functions and remaining kits not yet tested.

- 2026-10-18: Independent Phase 2 source review and pristine Zephyr build PASS (262/262, exit 0); user approved UF2 flash to Kit 1, board ID verified, copy completed, Zephyr boot observed on COM14. Android nRF Connect advertising/GATT tests pending; Phase 2 not complete.

- 2026-10-18: Android nRF Connect screenshot independently confirms connected GATT peripheral and exact Phase 2 service/RX/TX UUIDs. Messaging and discovery notification acceptance tests pending.

- 2026-10-18: Android screenshot confirms RX Hello and TX Hello with TX notifications enabled; notification delivery and DISCOVER event remain pending explicit verification.

- 2026-10-18: Android screenshot confirms RX DISCOVER, TX DISCOVER, discovery `DEMO: no mesh peers`, and both notification CCCs enabled. Phase 2 GATT command-response demonstrated; individual notification delivery log still pending for strict proof.

- 2026-10-18: Kit 2 connected on COM10 (VID:PID 2886:8044), existing firmware reports Device ID 2 and LoRa initialized; no radio link verified and no firmware changed. Kit 1 COM14 absent in latest enumeration; confirm USB/power before 2-kit BLE tests.

- 2026-10-18: Rechecked USB after user reconnected Kit 2: Kit 1 COM14 VID:PID 2FE3:0004 serial 1D5EA4A525ED34EB and Kit 2 COM10 VID:PID 2886:8044 serial 224BC825E1DA130C are simultaneously present, both Windows PnP status OK. Kit 2 still runs legacy LoRa firmware; Phase 3 BLE mesh code and OTA tests not yet performed.

- 2026-10-18: With explicit user approval, flashed Kit 2 Phase 2 BLE GATT UF2 (SHA256 67765A9E27C7C5DC93F65F5CF7B902EB0F304DAF990A61780FD2F519F0DAD623). Kit 2 entered UF2 as COM9, drive F: XIAO-SENSE; INFO_UF2.TXT board ID Seeed_XIAO_nRF52840_Sense, bootloader 0.6.1. UF2 copied to F:\zephyr.uf2; auto-reset and reenumeration to COM15 VID:PID 2FE3:0004, serial 224BC825E1DA130C. Serial COM15 reported Zephyr OS v4.5.0-rc2 boot banner. Kit 1 remained COM14. This verifies boot of new firmware, not BLE peer communication or Phase 3 mesh.

- 2026-10-18: Independent Phase 3 review: pristine build 267/267 PASS; UF2 262656 B, flash 131268 B, RAM 30344 B. Reviewed BLE relay/GATT source and architecture. Packet tests compiled but not executed; physical coexistence/peer messaging/multi-hop unverified. Verdict: build-ready for user-approved controlled flash and two-kit tests, not Phase 3 acceptance. See docs/PHASE3-INDEPENDENT-REVIEW.md.

- 2026-10-18: All three kits simultaneously detected: Kit 1 COM14 (2FE3:0004, serial 1D5EA4A525ED34EB), Kit 2 COM15 (2FE3:0004, serial 224BC825E1DA130C), Kit 3 COM13 (2886:8044, serial B13E025E5A1407EE); all PnP OK. Kit 3 non-destructive 115200 serial inspection reported existing legacy LoRa Mesh Phase 4 firmware, Device ID 3 and LoRa initialized. No Kit 3 flash approved or performed.

- 2026-10-18: User approved flashing Phase 3 to all three kits. Kit 1 double-reset by user, entered F: UF2, COM4 serial 1D5EA4A525ED34EB; verified INFO_UF2.TXT Board-ID Seeed_XIAO_nRF52840_Sense and firmware SHA256 9B02AC7D075678DCE411D7DA7AD83F3023856C49ED57F321F4131AC9534B1C5C. Copied 262656-byte Phase 3 UF2 to F:\zephyr.uf2 successfully; Kit 1 reenumerated COM14 VID:PID 2FE3:0004 same serial. Runtime BLE tests pending. Kits 2 and 3 not yet flashed.

- 2026-10-18: Kit 2 user double-reset, entered F: UF2 with COM9 serial 224BC825E1DA130C; verified INFO_UF2.TXT Board-ID Seeed_XIAO_nRF52840_Sense and Phase 3 UF2 SHA256 9B02AC7D075678DCE411D7DA7AD83F3023856C49ED57F321F4131AC9534B1C5C. UF2 copy succeeded and Kit 2 reenumerated COM15 VID:PID 2FE3:0004 with same serial. No BLE radio acceptance test yet; Kit 3 not flashed.

- 2026-10-18: Kit 3 user double-reset, entered F: UF2, COM12 serial B13E025E5A1407EE; verified INFO_UF2.TXT Board-ID Seeed_XIAO_nRF52840_Sense and Phase 3 UF2 SHA256 9B02AC7D075678DCE411D7DA7AD83F3023856C49ED57F321F4131AC9534B1C5C. UF2 copy succeeded and Kit 3 reenumerated COM16 VID:PID 2FE3:0004 same serial. All three kits now flashed Phase 3; COM14/15/16 serial read each showed Zephyr v4.5.0-rc2 boot banner and partial application startup text. Peer radio discovery, messages, Android notifications and 3-hop acceptance still pending.

- 2026-10-18: User Android nRF Connect screenshot after Phase 3 deployment shows connected HYBRIDMESH, RX write `DISCOVER`, TX echo `DISCOVER`, Discovery characteristic `PEER:EE07145A5E02`, both TX and Discovery CCC notifications enabled. At that point, evidence showed at least one BLE peer detected and exposed through GATT; exact kit mapping, notification event timing, inter-kit text delivery, and multi-hop remained unverified.

- 2026-10-18: Android observed first HYBRIDMESH address C8:51:08:6E:08:C0 advertises but connection fails even after phone Bluetooth restart. Second HYBRIDMESH address E5:12:76:D2:56:EA connects; TX characteristic reads `Ready` rather than earlier `HELLO123`; BLE text relay unproven. All three USB COM14/15/16 enumerate. Copilot task 007 requested to investigate connection lifecycle, advertising restart and scan/advertising coexistence; no new firmware flash authorized or performed.

- 2026-10-18: Independent Task 007 pristine build log completed 267/267 and produced 266240-byte UF2 (flash 132876 B, RAM 30408 B); patched firmware NOT flashed. Windows MediaTek Bluetooth adapter PnP OK. Installed bleak 3.0.2 into C:\zephyrproject\.venv and ran non-destructive PC scan tests/ble_pc_scan.py; scan detected 50 BLE devices, one named HybridMesh at E2:DB:6A:54:BB:71 RSSI -73 dBm. Other two not identified in this single scan; further testing required. No firmware changes on kits.

- 2026-10-18: With Android disconnected, ran tests/ble_pc_diagnostic.py using Windows Bleak 3.0.2 against OLD Phase 3 firmware (Task 007 fix not flashed). Three 9s scans each found only HybridMesh E2:DB:6A:54:BB:71 (RSSI -68/-70/-69); PC connected, verified custom service, read TX `Ready` and Discovery `PEER:EB34ED25A5A4`, and disconnected cleanly. Other boards not separately visible; BLE radio/multi-hop messaging not yet proven. No reset/flash.

- 2026-10-18: User single-reset all three boards. PC Bleak diagnostic three scans now found all three addresses C8:51:08:6E:08:C0, E5:12:76:D2:56:EA and E2:DB:6A:54:BB:71. All three connected and disconnected successfully via Windows BLE GATT; custom service present; TX read `Ready` on each. Discovery reads respectively `PEER:EE07145A5E02`, `PEER:0C13DAE125C8`, `PEER:0C13DAE125C8`. Old Phase 3 firmware still running. This proves one GATT connection per device after reset, not repeated reconnection or mesh message delivery.

- 2026-10-18: Automated follow-up after three successful PC GATT connect/disconnect operations: tests/ble_pc_relay_test.py scanned twice for 7s and found NONE of the previously observed three HybridMesh addresses, so reconnection and relay write could not proceed. Independent 12s scan tests/ble_pc_scan.py found 65 other BLE devices but zero HybridMesh. This is strong evidence old Phase 3 firmware loses discoverable advertising after connection/disconnection (not conclusively root-caused). Task 007 patch clean-builds but remains unflashed. Ask explicit user permission before deploying patch to any board.

- 2026-10-18: User approved Task 007 UF2 flash Kit 1 ONLY. Verified F: XIAO-SENSE bootloader board ID Seeed_XIAO_nRF52840_Sense, COM14 absent and COM15/16 present, UF2 SHA256 1E69DCD554648CDF6CACC4C6CBF71D7BC580BA1E64CA6DC6BB7E4717A9837A21 (266240 bytes). Copy to F:\zephyr.uf2 succeeded, COM14 reenumerated original serial 1D5EA4A525ED34EB. Kits 2/3 not flashed. Subsequent PC scan saw only E5:12:76:D2:56:EA, which passed three reconnections and GATT write/read FRURT07; follow-up scan found only E5 again, TX FRURT07, discovery inactive. Physical Kit 1 to BLE address mapping NOT established; Task 007 reconnect fix NOT yet validated on known Kit 1. No multi-kit relay verified.

- 2026-10-18: User reset ONLY Kit 1 COM14 after Task 007 flash. Only HybridMesh E5:12:76:D2:56:EA observed in three scans, and its TX value reverted from previously PC-written FRURT07 to `Ready`, strongly mapping this BLE address to Kit 1 (not cryptographic proof). PC connected/read/disconnected successfully. Targeted tests/ble_pc_kit1_reconnect.py ran six independent 6-second scan -> GATT connect -> TX read -> disconnect rounds against E5; all 6/6 VISIBLE and CONNECTED with TX `Ready`, DISCONNECTED successfully, no further reset. Evidence Task 007 patch improves reconnection on Kit 1. Kits 2/3 still old firmware and not advertising in current scan; inter-kit mesh text and Android reconnection still pending.

- 2026-10-18: User explicitly approved Task 007 flash to Kits 2 and 3. User double-reset Kit 2 only. Confirmed F: XIAO-SENSE UF2 board ID Seeed_XIAO_nRF52840_Sense, COM15 absent while COM14 and COM16 present. Task 007 UF2 SHA256 1E69DCD554648CDF6CACC4C6CBF71D7BC580BA1E64CA6DC6BB7E4717A9837A21. Copied to F:\zephyr.uf2 successfully, Kit 2 reenumerated COM15 original serial 224BC825E1DA130C. Kit 3 not yet flashed.

- 2026-10-18: Kit 3 Task 007 UF2 flashed after user double-reset; F: XIAO-SENSE board ID verified, COM16 absent while COM14/15 present, UF2 hash 1E69DCD554648CDF6CACC4C6CBF71D7BC580BA1E64CA6DC6BB7E4717A9837A21, copy succeeded, COM16 returned with serial B13E025E5A1407EE. All three boards now on Task 007 firmware. PC three 9s BLE scans each detected E2:DB:6A:54:BB:71, C8:51:08:6E:08:C0, E5:12:76:D2:56:EA. All three connected, had custom GATT service, TX `Ready`, discovery peer, and disconnected. Automated tests/ble_pc_relay_test.py then found all 3, 9/9 reconnect cycles passed (3 per address). PC wrote FRURT07 to C8 and source TX echoed FRURT07, but remote TX read `Ready` on E5 and E2 after 3s. Thus cross-device text forwarding NOT verified (could be scan/advertise scheduling or message handling); no BLE multi-hop proof. Do not claim Phase 3 complete.

- 2026-10-18: Phase 3 relay diagnosis: Reviewed ble_gatt.c/ble_mesh_relay.c. `ble_gatt_update_mesh_advertisement` returns -EAGAIN when active_connections>0; the then-current advertising worker dequeued packets without checking the update result, and disconnect advertising restart used base advertising data. PC Bleak callback air probe observed manufacturer 0xffff beacon frames from all 3 devices. PC concurrent scan+GATT write `AIRPROBE` to C8 succeeded, but no text advertisement was observed in the subsequent 8s (`AIR_TEXT_COUNT 0`). This supported the lost/deferred TX hypothesis but did not prove root cause (Windows scanning coexistence possible). Task 008 implemented the queue and restart correction; clean build and host tests are documented above. No further firmware flash authorized; ask user approval before flashing and measuring hardware acceptance.

- 2026-10-18: User explicitly approved flashing Task 008 to all three kits. Kit 1 user double-reset; F: XIAO-SENSE Board-ID Seeed_XIAO_nRF52840_Sense verified; COM14 absent while COM15/16 present. Task 008 UF2 SHA256 4589A06099429CFF5BBAFBBC365589833D426B96D23A287CD584654D73AAD53D (266752 bytes). Copied to F:\zephyr.uf2 successfully; Kit 1 reenumerated COM14 original serial 1D5EA4A525ED34EB. Kits 2/3 not yet updated to Task 008; hardware message relay acceptance pending.

- 2026-10-18: User double-reset Kit 2 for approved Task 008 deployment. Verified F: XIAO-SENSE Board-ID Seeed_XIAO_nRF52840_Sense; COM15 absent while COM14/16 present; Task 008 UF2 SHA256 4589A06099429CFF5BBAFBBC365589833D426B96D23A287CD584654D73AAD53D (266752 bytes). Copy to F:\zephyr.uf2 succeeded with exit code 0, bootloader F: disappeared, but COM15 did NOT reenumerate after repeated checks. COM14/16 remain. Kit 2 firmware transfer succeeded, runtime USB boot not yet verified; request user single reset Kit 2 before proceeding. Kit 3 not yet flashed Task 008.

- 2026-10-18: After user single-reset Kit 2, COM15 reenumerated with original USB serial 224BC825E1DA130C, VID:PID 2FE3:0004; COM14/16 remained present. Task 008 Kit 2 runtime USB recovery verified after reset. Kit 3 Task 008 deployment still pending.

- 2026-10-18: User double-reset Kit 3 under prior explicit Task 008 all-three flash approval. Verified F: XIAO-SENSE UF2 Board-ID Seeed_XIAO_nRF52840_Sense, COM16 absent and COM14/15 present. Task 008 UF2 SHA256 4589A06099429CFF5BBAFBBC365589833D426B96D23A287CD584654D73AAD53D (266752 bytes). Copy to F:\zephyr.uf2 succeeded, Kit 3 reenumerated COM16 original USB serial B13E025E5A1407EE. All three boards now flashed Task 008; Phase 3 BLE relay radio acceptance testing pending.

- 2026-10-18: Task 008 all-three live radio acceptance tests after firmware deployment. PC `tests/ble_pc_air_message.py --message AIR008K3 --scan-seconds 10`: GATT write/source notification/echo PASS to C8:51:08:6E:08:C0; other node E2:DB:6A:54:BB:71 advertised same text source 0c13dae125c8 sequence 26 TTL 2 (2 observations), remote E2 GATT TX PASS, E5 GATT TX `Ready` FAIL. Second independent run `--source E5:12:76:D2:56:EA --remote C8:51:08:6E:08:C0 --remote E2:DB:6A:54:BB:71 --message AIR008B --scan-seconds 10`: source GATT write/notify PASS; E2 advertised same text source eb34ed25a5a4 sequence 57 TTL 2 (6 observations); BOTH C8 and E2 remote GATT TX PASS. This proves real BLE inter-board relay advertisements and all-three-device message delivery in second run, but NOT an isolated forced two-hop A→B→C path because direct RF reception is possible. Phase 3 remains open for explicit multi-hop isolation and robustness tests. No further flash needed for these tests.

- 2026-10-18: Post-Task008 regression `tests/ble_pc_relay_test.py` detected all 3 HybridMesh devices, completed 9/9 BLE reconnection cycles (3 per device). GATT wrote FRURT07 to C8; remote E2 TX FRURT07 PASS, remote E5 TX retained prior AIR008B (not matching FRURT07). Thus relay works but delivery to every peer is not consistent. Need controlled forced two-hop and reliability characterization before Phase 3 completion.

- 2026-10-18: User physically separated kits with Kit 2 between Kit 1/3 (exact BLE address mapping for Kits 2/3 not yet established). Post-separation test source E5 message SEP3A01: source on-air TTL3 count2, C8 forwarded on-air TTL2 count2 and GATT delivery PASS, E2 GATT FAIL (retained FRURT07). Reverse test source E2 message SEP3B02: source on-air TTL3 count4, C8 remote GATT PASS, E5 remote GATT FAIL (retained SEP3A01), no other-node on-air frame captured. No proven two-hop endpoint delivery; likely separation/radio reachability and/or intermittent relay scheduling. Must not mark Phase 3 complete. Need physical kit-to-BLE mapping and controlled direct-link isolation.

- 2026-10-18: Resolved Task009 physical kit mapping without reset/flash: live BLE beacon manufacturer IDs E5=eb34ed25a5a4, C8=0c13dae125c8, E2=ee07145a5e02; the immutable IDs match reversed suffixes of physical USB serials COM14 Kit1, COM15 Kit2, COM16 Kit3. Verified A=Kit1 E5, B=Kit2 C8, C=Kit3 E2. See docs/TASK009-HARDWARE-IDENTITY.md. Started separate clean test-mode build with B=0c13; awaiting completion. No flash performed, approval required.

- 2026-10-18: User explicitly approved Task009 flashing to all three kits. Kit1 double-reset by user; COM14 disappeared, COM15/16 remained; F: XIAO-SENSE UF2 Board-ID Seeed_XIAO_nRF52840_Sense verified. Test-topology UF2 269312 bytes SHA256 093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83 copied successfully to F:\zephyr.uf2. Kit1 COM14 reenumerated with original USB serial 1D5EA4A525ED34EB. Kit2/3 remain Task008 pending user bootloader entry; Task009 physical acceptance pending.

- 2026-10-18: Task009 Kit2 deployment under explicit all-three approval: user double-reset Kit2; COM15 disappeared, COM14/16 remained. Verified F: XIAO-SENSE UF2 Board-ID Seeed_XIAO_nRF52840_Sense. Verified UF2 269312 bytes SHA256 093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83 copied to F:\zephyr.uf2 successfully. Kit2 COM15 reenumerated with original serial 224BC825E1DA130C; Kit1 COM14 and Kit3 COM16 remained. Kit3 Task009 flash and physical acceptance pending.

- 2026-10-18: Under user approval, Kit3 Task009 deployment: COM16 absent, COM14/15 remained; F: XIAO-SENSE Board-ID verified. Copied test UF2 SHA256 093F3568635F8B60021BA0FC6A140FC5E5DD21D56743F9F5E6E2D1DFB2F54C83 (269312 bytes), successful; COM16 reenumerated original serial B13E025E5A1407EE. All 3 kits now flashed Task009. First live `tests/ble_pc_controlled_two_hop.py --source E5:12:76:D2:56:EA --relay C8:51:08:6E:08:C0 --destination E2:DB:6A:54:BB:71 --scan-seconds 15` failed acceptance: A GATT write/echo PASS, A TTL3 on air source eb34ed25a5a4 sequence 33 PASS, B C8 TTL2 same source/sequence PASS, C GATT TX remained `Ready` FAIL. Raw evidence docs/task009-live-acceptance.log. Phase3 OPEN; investigate C delivery. No further flash authorized without fresh approval if firmware changes.

- 2026-10-18: Following Copilot Task009 investigation, independently captured Kit3 COM16 serial during fresh PC acceptance (no flash). `tests/task009_capture_c_console.py`, logs `docs/task009-kit3-console.log` and `docs/task009-live-with-console.log`. C console showed `TEST topology ACCEPT role=C local=EE07145A5E02` with source snippets 0C13... and EB34..., confirming at least some C topology accept events, but lines truncated/interleaved and not correlated to fresh message sequence. Fresh A GATT write/echo PASS for HOP0095F2CC5ED; no matching A/B on-air observations captured in PC runner; C TX `Ready` FAIL; acceptance exit 1. Console also reported BLE mesh beacon advertisement update failed -11. Need source/sequence-correlated receive diagnostics and scan/advertisement reliability analysis; no Phase3 completion claim.

- 2026-10-18: Independent verification after Copilot diagnostic capture changes: unittest discover 10/10 PASS, py_compile PASS. Ran corrected COM16 capture harness with Task009 acceptance, evidence docs/task009-recapture-verified.log and docs/task009-kit3-console.log. A GATT write/echo PASS payload HOP00971595B15; A E5 on-air source eb34ed25a5a4 seq154 TTL3 observed; no matching B TTL2 on-air; C GATT TX Ready FAIL, acceptance exit 1. C console showed corrupted/interleaved topology accept fragments and beacon advertising update -11; cannot link accepts to fresh TEXT sequence. Phase3 remains OPEN; no firmware changes or flash. Request Copilot investigate B relay emission, C diagnostics and console record integrity with precise evidence.

- 2026-10-18: Independently validated Copilot Task009 feature diagnostics. Python unittest 11/11 PASS; changed capture/parser/acceptance py_compile PASS; inspected new feature-level relay queue/receive counter/GATT TX storage integration. Clean independent Zephyr topology build 271/271 PASS, FLASH 134916 B, RAM 30408 B, UF2 270336 B, SHA256 04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142 (matches Copilot artifact). Config TEST_TOPOLOGY=y, B_ID_0C13=y; production build disables topology policy. C ztests compiled but not run. COM14/15/16 present. Report docs/TASK009-DIAGNOSTICS-INDEPENDENT-VERIFICATION.md; log docs/task009-diagnostics-independent-build.log. No flash; fresh approval required to deploy new diagnostic UF2. Phase3 controlled A->B->C OPEN.

- 2026-10-18: Under fresh user approval for Task009 diagnostic UF2 on all three kits, flashed Kit1 only. User double-reset Kit1: COM14 disappeared while COM15/16 stayed present; F: XIAO-SENSE INFO_UF2.TXT verified Board-ID Seeed_XIAO_nRF52840_Sense. Verified source build-task009-diagnostics-independent/zephyr/zephyr.uf2 SHA256 04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142 (270336 bytes). Copy to F:\zephyr.uf2 succeeded exit 0, COM14 reenumerated with original serial 1D5EA4A525ED34EB; COM15/16 remained connected. Kit2 and Kit3 await sequential diagnostic flash, followed by hardware acceptance. No Phase3 pass claimed.

- 2026-10-18: Under user's explicit approval for Task009 diagnostic firmware deployment to all three kits, flashed Kit2. User double-reset Kit2: COM15 disappeared; COM14/16 stayed enumerated. Verified F: XIAO-SENSE bootloader Board-ID Seeed_XIAO_nRF52840_Sense. Verified source UF2 SHA256 04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142. F:\zephyr.uf2 copy succeeded, Kit2 reenumerated COM15 with original USB serial 224BC825E1DA130C; COM14/16 remain. Kit3 pending; live diagnostics not yet run.

- 2026-10-18: User approved Task009 diagnostic UF2 to all three kits. Kit3 COM16 entered bootloader while COM14/COM15 remained connected; F: XIAO-SENSE INFO_UF2.TXT Board-ID Seeed_XIAO_nRF52840_Sense confirmed. Approved UF2 SHA256 04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142 successfully copied F:\zephyr.uf2. Kit3 COM16 reenumerated original serial B13E025E5A1407EE. All three kits now on Task009 diagnostic firmware, physical acceptance pending.

- 2026-10-18: After all 3 Task009 diagnostic firmware installations and USB COM reconnection, executed first live dual-console acceptance via tests/task009_capture_both_consoles.py. Payload HOP0098C47AF54 A GATT write and echo PASS, A source eb34ed25a5a4 seq49 TTL3 seen on air; matching B TTL2 NOT observed; C GATT TX Ready FAIL; acceptance exit1. COM15/COM16 console logs incomplete (only boot/partial lines and C GATT/advertising -11). No intact correlated RX_ACCEPT/relay stage found, so root cause remains unknown. Evidence docs/task009-both-consoles-acceptance.log, task009-b-diagnostic-console.log, task009-c-diagnostic-console.log. Report docs/TASK009-DIAGNOSTIC-LIVE-ACCEPTANCE.md. Phase3 OPEN; further firmware changes require new flash approval.

- 2026-10-18: Task009 console-loss investigation (docs/TASK009-CONSOLE-LOSS-INVESTIGATION.md). Source review: console is USB CDC printk (no CONFIG_LOG); CDC poll_out discards bytes without flow control; logs show 64-byte truncation 'Hyb' plus spliced lines; C -EAGAIN beacon failure coincided with PC GATT connection (BT_MAX_CONN=1). Added topology-only HEARTBEAT + decode counters and host integrity analyzer. Python 15/15 PASS; clean default/topology/ztest builds PASS (topology UF2 SHA256 19718FE4...C189, not flashed). No hardware touched; Phase3 OPEN; flash of new UF2 needs user approval.

- 2026-10-18: User explicitly approved heartbeat Task009 diagnostic topology UF2 for Kit2/B and Kit3/C only (Kit1/A untouched). Kit2 COM15 disappeared while COM14/16 stayed connected after user double-reset. Verified F: XIAO-SENSE UF2 bootloader and Board-ID Seeed_XIAO_nRF52840_Sense. Approved heartbeat UF2 SHA256 19718FE4F08BDF2665AED4726867E4727982438EED71237167FA4CC4609BC189 copied successfully to F:\zephyr.uf2. Original Kit2 USB serial 224BC825E1DA130C reenumerated at COM15, COM14/COM16 still present. Kit3 heartbeat deployment pending; no new physical acceptance result yet.

- 2026-10-18: User-approved Task009 heartbeat UF2 Kit3 deployment complete. Kit3 F: UF2 bootloader Board-ID confirmed, SHA256 19718FE4F08BDF2665AED4726867E4727982438EED71237167FA4CC4609BC189 transferred successfully, original COM16 recovered; Kit1 unchanged, Kit2 previously flashed same heartbeat image. Idle 60-second dual-console capture yielded only boot banner each (0 complete heartbeat records). Live A->B->C retry with heartbeat images: A GATT write/echo PASS HOP009DD6A7E59; A TTL3 seq288 over-air observed, matching B TTL2 absent, C GATT Ready FAIL. Partial B/C heartbeat strings report decoded 10->12 and text=0, but invalid/corrupted lines mean not fully validated. See docs/TASK009-HEARTBEAT-HARDWARE-RETEST.md and task009-hb-live-acceptance.log. Phase3 OPEN; investigate text reception/advertising and lossy CDC; no further flash authorized.

- 2026-10-18: Task010 root cause found in source: scan used BT_LE_SCAN_OPT_FILTER_DUPLICATE; Zephyr LL dup filter drops later legacy ADV_IND from a known address regardless of payload, so TEXT after a beacon never reached B/C (decoded grows only by eviction, text=0). Fix: unfiltered scan, 1000 ms hold, compact checksummed <64 B heartbeat. Python 16/16 PASS; clean topology/default/ztest builds PASS (ztests compiled, not executed). Topology UF2 SHA256 2FFAA52A...D773 NOT flashed; needs approval. Hardware result unverified, Phase3 OPEN. See docs/TASK010-SCAN-DUPLICATE-FILTER-FIX.md.

- 2026-10-18: User explicitly approved Task010 BLE duplicate-filter fix topology UF2 for all 3 kits. Kit1/A flashed successfully after user double-reset: COM14 absent with COM15/16 present, UF2 drive F: XIAO-SENSE and INFO_UF2 Board-ID Seeed_XIAO_nRF52840_Sense confirmed. Verified SHA256 2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773 for build-task009-scanfix-topology/zephyr/zephyr.uf2; copy to F:\zephyr.uf2 succeeded, COM14 reenumerated serial 1D5EA4A525ED34EB, COM15/16 remained connected. Kit2/3 Task010 flashing pending; Phase3 acceptance not yet retested.

- 2026-10-18: Under user's explicit approval for Task010 BLE scan duplicate-filter topology firmware to all three kits, flashed Kit2/B. User double-reset Kit2; COM15 absent while COM14/16 remained, F: XIAO-SENSE INFO_UF2 verified Board-ID Seeed_XIAO_nRF52840_Sense. Source UF2 SHA256 2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773 verified. UF2 copy to F:\zephyr.uf2 succeeded; Kit2 COM15 recovered with original serial 224BC825E1DA130C, alongside COM14 and COM16. Kit3 Task010 flash pending, live two-hop retest pending; Phase3 OPEN.

- 2026-10-18: Under user's explicit Task010 all-three-kit flash approval, Kit3/C reset into UF2: COM16 absent and COM14/15 present, F: XIAO-SENSE Board-ID Seeed_XIAO_nRF52840_Sense verified. Exact topology UF2 SHA256 2FFAA52A810514C27F63823271F8F24EAE5B6A531359F0DDA94E6E95B007D773 copied to F:\zephyr.uf2 successfully. Kit3 restored COM16 USB serial B13E025E5A1407EE, COM14 and COM15 still present. All three kits now on Task010 scan duplicate-filter fix. Live A->B->C acceptance pending.

- 2026-10-18: Task010 new scan duplicate-filter fix now physically flashed on all 3 boards and **controlled A->B->C BLE relay acceptance PASSED in 2/2 independent runs**. Run1 A GATT echo PASS, A seq52 TTL3 and B seq52 TTL2 identical source/message HOP009C459BA42 observed on-air, C GATT exact payload PASS, runner exit0; evidence docs/task010-live-acceptance-first.log. Run2 A seq68 TTL3 and B seq68 TTL2 identical source/message HOP0094B70A7C9, C GATT exact payload PASS, runner exit0; evidence docs/task010-live-acceptance-second.log. B/C HEARTBEAT text counters now >0 vs old zero, although USB CDC still mangles some printk lines. See docs/TASK010-PHYSICAL-ACCEPTANCE.md. BLE controlled 3-node two-hop PoC milestone PASSED; broader RF isolation, LoRa, hybrid routing and phone notifications NOT yet validated.

- 2026-10-19: Configured VS Code Local Copilot completion signal: `.github/hooks/notify-chatgpt.json` with `UserPromptSubmit` removing `copilot_done.txt` and `Stop` writing `FINISHED at <ISO8601>`, and `.vscode/settings.json` enabling `chat.useHooks` and user-requested `chat.tools.autoApprove`. JSON loaded and both PowerShell commands manually tested successfully (marker created then removed); true VS Code agent lifecycle invocation remains to be verified. IMPORTANT: VS Code Local native PascalCase hooks differ from Copilot CLI hooks (camelCase with version 1). Completion marker does not actively push notifications; it is read when a watcher polls. No firmware changes/flashes.

- 2026-10-19: Independent Task011 completion review after user reported Copilot done. `code agent ps` found no active Copilot sessions; Task011 report docs/TASK011-TRUE-TWO-HOP-ISOLATION.md and new PC scripts/tests exist. Independently ran Python unittest discovery: 27/27 PASS exit0; py_compile on Task011 modified runners/evidence module PASS exit0; --help runner works. No firmware changes/flashing or new RF isolation test. `copilot_done.txt` was absent despite agent idle, so Stop-hook invocation by `code chat`/Copilot CLI remains UNVERIFIED (manual PowerShell hook execution had been verified earlier); do not infer live hook success. Await user physical setup for sensitivity control (B powered off, A/C close), then fixed A/C isolation baseline and relay control. Strict RX console integrity gates can result in inconclusive trials due to continuing CDC console data loss.

- 2026-10-19: Kit2/B user-confirmed powered off, USB scan shows only A COM14 and C COM16. Ran Task011 first PC sensitivity trial `task011-sensitivity-live-01`: A GATT write+echo PASS HOP0114261CAA4, A on air TTL3 seq312 observed, C TX Ready unchanged; B-off PC baseline PASS but **strict physical sensitivity evidence INCONCLUSIVE** due missing/damaged C heartbeat/receive logs and lack of intact C same-sequence reject. See docs/TASK011-FIRST-SENSITIVITY-LIVE.md and docs/task011-sensitivity-invocation-01.log. No firmware flashed or reset. Do not proceed with RF isolation claim until instrumentation trustworthy.

- 2026-10-19: Task012 C-console evidence. Source analysis: Zephyr USB CDC ACM poll_out silently discards bytes when TX ring is full (no flow control), explaining truncated/merged lines; Task011 stays INCONCLUSIVE, no RF-isolation claim. Added read-only GATT binary evidence characteristic 7b3e0005 (counters + last reject seq/TTL/source tail, checksummed) with host decoder/tests. Python unittest 31/31 OK; ZTESTs compiled/linked only (native_sim unavailable on this host); clean default and topology firmware builds exit 0 (topology UF2 SHA256 D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8). NOT flashed; Task010 deployed firmware unchanged; hardware plan awaits new explicit approval. See docs/TASK012-C-CONSOLE-EVIDENCE.md.

- 2026-10-19: Independent review of completed Copilot Task012. Agent idle, completion marker absent (Copilot CLI Stop-hook not proven). Read docs/TASK012-C-CONSOLE-EVIDENCE.md and new BLE mesh evidence/GATT host decoder code. Independently ran Python unittest discovery 31/31 PASS (exit0), py_compile Task012 modules PASS, inspected topology UF2 SHA256 D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8, 272384 bytes; Kit1 COM14 and Kit3 COM16 only (Kit2 off). New 20-byte read-only GATT evidence characteristic intended to replace lossy CDC console for C rejected/accepted TEXT counters and last-event metadata. Still no hardware GATT characteristic read, so hardware result unproven. No flashing/reset. To verify Task011 B-off sensitivity, ask new user approval to flash Task012 UF2 to A/C only; B stays off. Later three-board regression needs coordinated firmware/approval.

- 2026-10-19: User explicitly approved Task012 diagnostic UF2 for Kit1/A and Kit3/C only, B off. On second user double-reset Kit1 COM14 disappeared, COM16 stayed; F: XIAO-SENSE INFO_UF2 verified Board-ID Seeed_XIAO_nRF52840_Sense. SHA256 D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8 verified on build-task012-topology/zephyr/zephyr.uf2; copy F:\zephyr.uf2 succeeded, Kit1 COM14 reenumerated original serial 1D5EA4A525ED34EB alongside C COM16. Kit3 Task012 flash pending, B remains powered off. No Task012 GATT hardware acceptance claimed yet.

- 2026-10-19: Under user approval for Task012 UF2 A and C only, flashed Kit3/C after double-reset: COM16 absent, COM14 remained; F: XIAO-SENSE bootloader with Board-ID Seeed_XIAO_nRF52840_Sense. Verified UF2 SHA256 D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8; successful transfer to F:\zephyr.uf2; Kit3 COM16 reenumerated serial B13E025E5A1407EE, Kit1 COM14 present. B powered off. New GATT evidence pending live read.

- 2026-10-19: Task012 Task011 direct-path sensitivity validated on physical A/C with B OFF. New GATT binary evidence characteristic read on A and C, checksums/roles valid. Five unique A TTL3 messages observed by PC; C last rejection seq/TTL/source matches, GATT rejection deltas 18,15,14,16,15 respectively, acceptance delta zero; C TX Ready unchanged. **5/5 sensitivity trials PASS.** Trial 01 runner emitted false inconclusive due script regex typo; independently verified original saved GATT before/after snapshots and on-air seq36 with sensitivity_failures=[]; corrected script before trials 02-05. Evidence docs/TASK012-LIVE-GATT-SENSITIVITY.md, task012-sensitivity-live-01..05.log. RF isolation still NOT established; need move A/C apart with B off for isolation controls, then B-on positive. No phone; no new flash beyond user-approved A/C Task012.

- 2026-10-19: First user-separated A/C RF isolation negative control (B off) **FAILED** as C still decoded A's direct text. Only USB COM14/COM16 enumerated. PC A GATT echo PASS; A on-air seq143 TTL3 payload HOP011074A61E2 observed, C TX Ready unchanged. GATT evidence C text counter 78->101 (+23), reject 78->101 (+23), accept remains0, last seq143 TTL3 source tail25a5a4. `ISOLATION_RESULT=FAIL_C_DETECTED_TEXT`; geometry not isolating, need increased A/C RF attenuation before repeating. See docs/TASK012-ISOLATION-FIRST-LIVE.md and task012-isolation-live-01.log. No flash/reset.

- 2026-10-19: After user increased A/C RF separation with B off, **five B-off isolation baseline trials PASS** using Task012 GATT evidence snapshots and A-on-air PC observations. Logs docs/task012-isolation-live-02..06.log; A sequences 210,226,236,245,255 TTL3, C TEXT/REJECT/ACCEPT deltas all zero and C stored TX Ready unchanged across every trial. C overall decoded counter stayed at 7509, so nonreceiving scanner not ruled out solely by baselines; positive B-on restoration with fixed A/C geometry needed. Previous near-geometry trial failed (+23 rejected A packets), and 5/5 close-proximity direct sensitivity passed. See docs/TASK012-ISOLATION-FIVE-TRIALS.md. No flashing/reset. Next user to reconnect/place Kit2 midway, no need for phone.

- 2026-10-19: **Controlled RF-separated A→B→C BLE relay PoC ACCEPTANCE PASS** with only PC BLE/GATT/USB tools, no Android phone. A/C remained in positions where five B-off A TTL3 transmissions yielded C TEXT delta0 (prior 5/5 direct sensitivity with A/C close confirmed C RX and direct TTL3 rejection). User powered on/positioned B as bridge, serial COM15 original ID restored, B firmware Task010 compatible with A/C Task012. **Five of five** fresh relay messages: PC observed exact matching A source seq TTL3 and B same source seq TTL2; C GATT TX exact payload; C GATT accept delta +1 and reject delta0 each trial, last event often DUPLICATE due to repeated relay frames. Seqs 326,346,357,368,378. Trial1 validator incorrectly required last ACCEPT (was DUPLICATE after +1 accept); independently reviewed evidence and corrected validation for trials2-5, all passed. Python tests 31/31 PASS and wrapper py_compile PASS. Evidence in docs/TASK012-ISOLATED-TWO-HOP-FINAL-ACCEPTANCE.md and task012-relay-live-01..05.log. Caveat: empirical RF separation, no authenticated advertiser provenance, incomplete B console not claimed. **No additional firmware flashes.** Next: hybrid LoRa+BLE design/implementation and staged testing, explicitly seek flash approval for changes.

- 2026-10-19: **TASK013 independently reviewed and accepted for software foundation only.** Copilot idle, completion-marker absent. Reviewed 6 new LoRa transport headers/C files, ztest CMake/testcases, underlying packet code. Independently ran 31/31 Python tests PASS and fresh standalone LoRa ztest Zephyr XIAO build 202/202 steps PASS (FLASH 69416 B RAM 16504 B), raw log docs/task013-independent-review-ztest.log. C ztests compiled, **not executed** (no Windows native_sim host toolchain/QEMU); real LoRa RF not tested, module not confirmed. Hash-verified Task013 topology/default UF2s identical to Task012 deployed counterpart builds, so BLE image unchanged. Flag future atomic dedup/send retry and multi-radio concurrency design before enabling hardware. See docs/TASK013-INDEPENDENT-REVIEW.md. No hardware operations/flashes. Await exact LoRa transceiver/module and regulatory region for Phase4 physical integration.

- 2026-10-19: **TASK014 independently reviewed and ACCEPTED for compile-only SX1262 hardware profile.** VS Code Copilot agent idle, authored opt-in smoke app/overlay and feature docs. Independent clean SX1262 build 201/201 PASS with native driver linked, SPI2/D4 CS/D2 RESET/D3 BUSY/D1 DIO1 and I2C1 disabled; UF2 111104 B SHA256 51E199AFCB39D98F39E12D95B52A6B322AF9EF2F0AA49CA67C315F4A85725D5A. Independent clean default BLE build 273/273 PASS, LoRa disabled; Python tests 31/31 PASS. User confirmed 3 LoRa modules and antennas connected. RXEN/DIO2 RF switch, DIO3 TCXO, RF region/power/airtime unresolved; physical LoRa bring-up and hybrid routing OPEN. No flashing, reset or RF transmission. Evidence docs/TASK014-INDEPENDENT-REVIEW.md and task014-independent-* logs.

- 2026-10-19: Task014 independent reviewer **PASS software-only SX1262 opt-in Zephyr hardware profile**. User confirms 3 LoRa modules and antennas attached; no operating RF region specified. Source review checked SPI2 D8/9/10, CS D4, DIO1 D1, BUSY D3, RESET D2, I2C1 disabled. First pristine opt-in build failed Windows CMake backslash escaping; retried with forward slashes: native SX126x build 201/201 PASS (FLASH 55544B RAM15608B), verified generated DTS + Kconfig + driver archive. Independent pristine default and topology builds PASS, UF2 hashes match Task012 originals exactly. NO flash, RF send, board reset. Details docs/TASK014-INDEPENDENT-REVIEW.md and build logs. RF-switch RXEN/DIO2/TCXO and live radio operation remain future gates; Task015 hybrid mock-routing work launched.

- 2026-10-19: TASK015 adds a software-only hybrid BLE+LoRa router (src/features/hybrid_router) with injected mock transports. ztest app tests/hybrid_router COMPILED/LINKED for xiao_ble/nrf52840 (200/200, 9.76% flash; not flashed). C ztests NOT EXECUTED: native_sim twister crashed on this Windows host (multiprocessing WinError 2), not retried. Python reference model tests 10/10 PASS (model only, not the C code). No hardware, RF, GATT, or default build touched. Awaiting remote native_sim run and independent review. Details/limits/rollback/next plan: docs/TASK015-HYBRID-ROUTER-MOCK-ACCEPTANCE.md


- 2026-10-19: **Task016 PC BLE GATT notification coexistence PASS 5/5 (narrow objective)**: with C connected/subscribed to TX notification and discovery CCCs, inject unique A text via PC GATT; C TX notified exact payload once and C acceptance delta +1 on each of five runs, readback exact. Prior BLE B relay geometry maintained, no firmware/LoRa transmission/phone. PC independently captured B TTL2 in 3/5 while concurrently handling multiple BLE connections; strict per-trial complete radio provenance inconclusive in other 2. C reject counter rose in 3 runs (+2,+3,+1), showing intermittent direct RF leakage; no blanket RF-isolation claim. C discovery CCC subscribed but no discovery event generated. Offline validator tests/task016_review_logs.py exit0; raw logs docs/task016-pc-gatt-coexistence-01..05.log; report docs/TASK016-PC-GATT-NOTIFICATIONS.md. Android behavior still open.

- 2026-10-19: **PC BLE GATT discovery notification test 3/3 PASS** using PC subscriptions to C discovery/text CCC and RX command `DISCOVER`, no Android app. Each run returned PEER:0C13DAE125C8 (B) and PEER:EB34ED25A5A4 (A), final discovery GATT readback matched. Logs docs/task016-pc-discovery-01..03.log, report docs/TASK016B-PC-PEER-DISCOVERY.md. Found control-plane UX bug: DISCOVER command also echoed through text TX notify (because ble_gatt.c/write_rx unconditionally publishes text after successful handler). Requires software-only correction and future flash approval. No firmware/LoRa RF changes.

- 2026-10-19: **Task015B hybrid routing software-only corrections independently accepted** (no firmware flash). Copilot focused fixes: guarded local queue-full dedup rollback permits same-seq retry after capacity frees when callbacks serialized; stats.delivered/out.delivered_locally only when delivery callback invoked; cross-medium-first relay remains experimental heuristic. Python hybrid model 13/13 PASS; existing suite 31/31 PASS. Independently pristine Zephyr XIAO C ztest compile/link 200/200 PASS FLASH81116 RAM17784, raw log docs/task015b-independent-build.log. C ztests NOT executed, true RF hybrid routing untested. Independent review docs/TASK015-INDEPENDENT-REVIEW.md and amended Task015 report. Input-needed Copilot turn cancelled after code persisted, separate terminal independently built. No physical actions.

- 2026-10-19: Task017 **experimental second opt-in RF switch/TCXO overlay compile/link PASS 201/201**, XIAO nRF52840 Wio-SX1262, standalone SX native smoke build; uses RXEN D5 active-high, DIO2 RF-switch TX, DIO3 TCXO 1.8V sourced from manufacturer notes/nRF variant pin evidence, unverified physically. DTS inspected RX GPIO P0.05, dio2 flag, dio3 enum 0x2, I2C1 disabled, SX1262 native driver linked. Original Task014 minimal overlay unchanged; no board flash/RF. docs/TASK017-SX1262-RF-FRONTEND-EXPERIMENT.md, docs/task017-rf-frontend-build.log. Region/user flash authorization pending.

- 2026-10-19: Reconciled stale project README.md and seven-phase docs/ROADMAP.md with observed evidence: BLE 3-node PoC PASS, PC GATT notifications 5/5 and peer discovery 3/3, SX1262/experimental frontend compile-only, hybrid routing mock-only, Android historical GATT screenshots vs current peer notifications, and explicit RF region/flash/runtime test gates. Existing feature/phase historical reports preserved without rewriting old records. Created safe repeatable tests/pc_ci.py with quick PASS (31 existing Python, 13 hybrid Python, syntax); full pristine PC CI started, report docs/PC-CI-LATEST.md when complete. No hardware operations or LoRa transmit.

- 2026-10-19: **Task018 optional DISCOVER no-echo firmware software acceptance PASS**, not deployed. Initial full PC CI compiled all 6 Zephyr targets, revealed default/topology UF2 hash changes and false DTS checks. Guarded new RX classification behind feature-ON compile flag and extended SX1262 DTS audit window; incremental root default/topology rebuilds exit0 now match Task012 validated UF2 hashes byte-for-byte. Independent pristine opt-in no-echo firmware build exit0 (CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y), independent pristine GATT policy C ztest compile/link 205/205 PASS (not executed). Artifact audit 15/15 PASS, Python 31/31 baseline and 13/13 hybrid model PASS. No flash/reset/LoRa RF. See docs/TASK018-DISCOVER-CONTROL-NO-ECHO.md and docs/PC-CI-LATEST.md. Updated no-echo behavior needs separately approved flash/runtime test, and LoRa country/channel remains unconfirmed.

- 2026-10-19: User confirmed RF operating country **United Kingdom**. Checked official Ofcom IR2030/1/16: 868.0–868.6MHz, 25mW ERP cap, appropriate interference mitigation or alternative <=1% duty-cycle. Proposed 868.3MHz center and 125kHz BW for future controlled tests, not permission to transmit. No hardware flash/RF. Assigned Task019 software-only UK region channel/ERP/duty-airtime gating with Tx disabled by default; pending Copilot review. Regulatory/source handoff docs/TASK019-UK-REGULATORY-SOURCE.md.
- 2026-10-19: **Task019 UK regulatory profile code + ztest written, NOT built or executed** (sandbox blocked west/venv, ARM gcc, zig; no host compiler). Opt-in `src/features/uk_lora_regulatory/`, `tests/uk_lora_regulatory/`; gate default denied and compile-locked, 30 s/h rolling budget, band+25 kHz guard. IR2030/1/16 re-read from PDF (≤1% attribution across PDF pp.20-21 needs human confirmation); Ofcom Apr-2026 item is a proposal only. No firmware/root CMake changes, no RF. Details and uncertainties: docs/TASK019-UK-LORA-REGULATORY-PROFILE.md. Awaiting toolchain access + independent review.

- 2026-10-19: **Task019 UK LoRa regulatory gate independently built and reviewed** after Copilot completion. Official Ofcom IR2030/1/16 868.0–868.6MHz, 25mW ERP and alternative <=1% duty restriction confirmed; 868.3MHz/125kHz remains proposed only. Feature-scoped TX gate default compile-locked OFF, 30s/h airtime reservation across all classes; no root app/radio wiring. Independent clean Zephyr XIAO C ztest builds mock 199/199 PASS, locked 199/199 PASS, then reviewer latched clock fault fail-closed and amended test, incremental rebuild both PASS. **16 C test cases compiled/linked, NOT executed**. Existing Python 31/31 and hybrid 13/13 PASS, deployed BLE UF2 hashes unchanged. See docs/TASK019-INDEPENDENT-REVIEW.md; device compliance, radiated power, live LoRa/hybrid proof and user-specific flash permission open. No flash/RF.

- 2026-10-19: **Task019 UK LoRa safety profile independently reviewed: three Zephyr targets COMPILE/LINK PASS, C runtime blocked.** XIAO mock 199/199, XIAO locked 199/199, qemu_cortex_m3 140/140, all incremental rebuilds after reviewer patch PASS; QEMU execution failed on Windows 0xc0000135 missing native dependency, no C case executed. Found serious compile-time lock bypass through mutable g.open in original code; fixed uk868_gate_is_open to unconditional FALSE when UK868_ALLOW_GATE_OPEN=0 and added locked-test spoof scenario. Existing Python 31/31 and hybrid reference 13/13 PASS. Noted airtime overrun, clock fault recovery, concurrency, ERP/antenna and Ofcom IR table page-break attribution remain open before RF. No flash/transmit. Docs/TASK019-INDEPENDENT-REVIEW.md, build/test logs task019-*.

- 2026-10-19: Task020 UK868 policy hardening independent review: C clock_fault latched fail-closed, invalid settlement retains pending token, token ID wrap avoids live collisions, ERP arithmetic clamped 64-bit, matching C ztests added. Independently compiled XIAO locked and mock target incremental builds PASS, pristine QEMU Cortex-M3 140/140 compile/link PASS, existing Python 31/31 + hybrid 13/13 PASS. C test runtime NOT executed because bundled Windows QEMU fails 0xc0000135. Airtime bound not calibrated, no legal/RF acceptance. No flash/reset/RF action. docs/TASK020-INDEPENDENT-REVIEW.md and task020-independent-* build logs.

- 2026-10-19: Prepared Task021 standalone NO-LoRa-TX SX1262 driver init diagnostic for future explicitly approved Kit1 test. Three USB serial IDs verified online COM14/15/16. Copilot editor prompt blocked; prepared independent standalone Zephyr app + PC read-only serial capture script tests/task021_pc_serial_capture.py selecting stable USB serial, no on-board commands. Pristine west XIAO + experimental RXEN/DIO2/TCXO overlay 201/201 build PASS; FLASH55748 RAM15608 UF2 111616 sha256 FC28283847C6217CA0CC4BFDE308E05EEB15764A8A8FD1DEFD32B7BA8B8EDAB2. DTS and native SX126x driver linked. No LoRa send/config in source; app device_is_ready is ONLY initialization status, not chip silicon RX evidence. BLE Task012 image hash unchanged. No flash/reset/power/RF TX. See docs/TASK021-SX1262-RECEIVE-ONLY-DIAGNOSTIC.md. Next requires explicit authorization to flash this image to Kit1/A only, with temporary BLE service interruption; then physical double-press Reset to enter UF2 bootloader.

- 2026-10-19: User approved Task021 Kit1 only, double-reset allowed UF2 bootloader F: XIAO-SENSE (Board-ID Seeed_XIAO_nRF52840_Sense). Verified original Task021 v1 UF2 SHA256 FC28283847C6217CA0CC4BFDE308E05EEB15764A8A8FD1DEFD32B7BA8B8EDAB2 and copied to F:\zephyr.uf2 successfully; Kit1 same USB serial 1D5EA4A525ED34EB returned as COM14, B/C untouched. Read-only serial capture saw repeated TASK021_ALIVE_NO_TX=1, proving application running, but failed to capture one-time TASK021_SX1262_DEVICE_READY boot message. User single-pressed reset while serial capture active; Windows disconnect caused pyserial ClearCommError and capture ended, boot readiness still UNKNOWN. Source revised v2 to print marker and DEVICE_READY every 10s; incremental Zephyr build exit0, UF2 SHA256 2525D42E302C1E6007EFE5B25743872DD3AB292C114453E24319989C6DA2036B, 112128 bytes, **NOT FLASHED pending explicit v2 approval**. No LoRa RF TX or Kit2/3 changes.

- 2026-10-19: **Task021 v2 Kit1 SX1262 driver initialization physical acceptance PASS (narrow)**. User explicitly approved Kit1 v2 flash, double-reset entered verified F: XIAO-SENSE bootloader Board-ID Seeed_XIAO_nRF52840_Sense, copied exact UF2 SHA256 2525D42E302C1E6007EFE5B25743872DD3AB292C114453E24319989C6DA2036B, Kit1 USB identity 1D5EA4A525ED34EB returned COM14. Automated serial tests/task021_pc_serial_capture.py for 32 s reported marker FRURT_TASK021_NO_TX_DIAGNOSTIC v2 and repeated TASK021_SX1262_DEVICE_READY=1 three times, exit0 PASS_DRIVER_INIT_ONLY. Log docs/task021-kit1-v2-live.log. Device readiness shows Zephyr driver init status only, NOT actual SPI silicon ID or LoRa RX/TX, no RF transmission. Kit2 COM15 / Kit3 COM16 untouched. Task021 v2 temporarily replaces Kit1 BLE mesh application; restoration later requires approved flash.

- 2026-10-19: User approved progression toward controlled UK SX1262 TX/RX test. Verified Ofcom IR2030/1/16 UK 868.0–868.6MHz, max25mW ERP, <=1% duty-cycle alternative. Copilot Task022 requested but editor blocked input; prepared independent RX-only standalone Zephyr SX1262 role using 868.3MHz/125k/SF9/CR4-5/private sync; no lora_send or RF TX. Fresh clean build 201/201 PASS, FLASH55944 RAM15608 UF2 SHA256 28C2852D6C714D428D0269ECAF48BC3E8ACEEAFFF4D672E69BAEDECD738C4644. All three kit serial IDs still visible. RX image ready for Kit1 flash after bootloader physical button action. TX image/power assessment still open, no RF transmissions. docs/TASK022-UK-SX1262-PAIR-TEST-PREP.md.

- 2026-10-19: User entered Kit1 UF2 bootloader for Task022 RX-only test. Independently verified F: INFO_UF2.TXT Board-ID Seeed_XIAO_nRF52840_Sense and expected receiver UF2 SHA256 28C2852D6C714D428D0269ECAF48BC3E8ACEEAFFF4D672E69BAEDECD738C4644. Flash copy F:\zephyr.uf2 succeeded; Kit1 identity serial 1D5EA4A525ED34EB returned COM14. PC script tests/task022_capture_kit1_rx.py captured three TASK022_RX_WAIT_NO_PACKET messages across 32 seconds without RX errors, log docs/task022-kit1-rx-live.log. Interpretation: Zephyr RX loop is active and returns timeouts without errors, but initial TASK022_RX_CONFIG_RC startup event missed, no LoRa test packet received yet. Kit2/3 unmodified; NO RF TX. TX firmware, power/antenna ERP compliance review pending.

- 2026-10-19: User confirms stock/original included LoRa antennas on all 3 XIAO nRF52840 + Wio-SX1262 kits. Independent Seeed nRF kit docs do NOT yield verified antenna peak gain (2dBi published for different ESP32-S3 kit must not be imported). UK 868.3MHz IR2030/1/16 25mW ERP, <=1% duty-cycle alternative remains planning target. Task023 independently created TX-locked hardware preflight, explicitly UK868_ALLOW_GATE_OPEN=0, no radio TX APIs; pristine xiao_ble/nrf52840 Zephyr compile 202/202 PASS, FLASH56788 RAM15608, UF2 113664 SHA256 8B0FBD0A33372ECA783A4389B111D77BDAA687C320CDB52C8FBD27189C6DAAC5. No flash, no LoRa TX; Kit1 RX mode, Kit2/3 BLE mode. Antenna ERP compliance remains open before actual RF send. docs/TASK023-UK-ANTENNA-AND-LOCKED-TX-PREFLIGHT.md.

- 2026-10-19 afternoon: User accidentally closed VS Code windows. Reconnected Abi via Remote Desktop Commander; device online with Kit1 COM14 serial 1D5EA4A525ED34EB (LoRa RX-only image), Kit2 COM15 224BC825E1DA130C and Kit3 COM16 B13E025E5A1407EE still detected. Found resumed Copilot task incorrectly running with cwd C:\zephyrproject, stopped that turn. Reopened VS Code with `code -r C:\zephyrproject\hybrid-mesh-poc`; confirmed actual VS Code window title `Welcome - hybrid-mesh-poc - Visual Studio Code`, terminal cwd C:\zephyrproject\hybrid-mesh-poc and AGENTS.md. Restarted Task024 one-shot UK TX safety work with Copilot CLI; `code agent ps` confirms active session cwd file:///c%3A/zephyrproject/hybrid-mesh-poc. No physical board actions, no flash, no LoRa TX. Copilot independent review follows completion.

- 2026-10-19: **TASK024 one-shot UK868 TX safety implementation (PC-only, NO RF/flash).** New feature src/features/uk_lora_one_shot + app tests/sx1262_uk_one_shot (compile-time lock default OFF, two locks forced equal, lora_send only in radio_tx.c compiled only in allow build, evidence gate with unverified antenna gain => denied, RX-identical config, one-shot latch, airtime reserved before send, uncertain outcomes kept charged). Independent airtime 10 B = 151604 us margined. Locked xiao_ble pristine build 203/203 PASS, FLASH56836 RAM16888 UF2 114176 SHA256 D2EC03A68E6BB860BC2BDBB22817BD59EFB28D27C46F7DEA78112549F9803439, not flashed; static no-TX check PASS; allow compile-check built then deleted. ztests compile in both configs but NOT executed (QEMU DLL missing, zig sandbox denied) - open item for independent review. 13 Python tests OK. No ERP/antenna approval; -9 dBm provisional only. See docs/TASK024-ONE-SHOT-TX-SAFETY-REVIEW.md.

- 2026-10-19: Following user report Copilot Task024 done, reopened VS Code using `code -r C:\zephyrproject\hybrid-mesh-poc` from correct repo and verified Copilot idle. Task024 default-locked one-shot TX independent review PASS compile-only: pristine Zephyr xiao_ble/nrf52840 203/203 PASS, FLASH56836 RAM16888, locked UF2 SHA256 D2EC03A68E6BB860BC2BDBB22817BD59EFB28D27C46F7DEA78112549F9803439 matches Copilot. Static no-TX checker PASS, Task024 Python 13/13 PASS, whole repo 44/44 PASS, hybrid Python 13/13 PASS; Kit1 COM14 RX, Kit2 COM15 BLE, Kit3 COM16 BLE unaffected; root BLE hashes unchanged. Windows QEMU exits 0xC0000135; C ztest cases NOT executed. No flash/LoRa TX. Critical future-TX gate: allowed one-shot re-arms after MCU reboot (volatile consumed/budget), antenna ERP and conformity not verified. See docs/TASK024-INDEPENDENT-REVIEW.md. Following Task025 should harden restart protection and test execution without enabling real TX.

- 2026-10-19: **TASK025 reboot-safe one-shot TX authorization (PC-only, NO flash/RF).** Added pure-C durable once-only store (arm token + CRC, single consume-word program, no firmware erase/arm, readback before radio; blank/corrupt/unreadable/consumed => deny), wired into frurt_os_fire before radio; ALLOW path hard-disabled (#error without FRURT_UK_ONE_SHOT_STORE_ACCEPTED, production CMake FATAL_ERROR). Regression tests written for reboot, torn/interrupted write, consumed, power-cut sweep, unknown airtime, no duplicate TX. Actual results: static check PASS, Python 45/45 and hybrid 13/13 OK. C tests NOT compiled/executed and locked XIAO image NOT rebuilt: sandbox denied west.exe/SDK, no host compiler, QEMU DLL missing. No success claimed for C/build. Real flash adapter and hardware atomicity remain an open gate. See docs/TASK025-ONE-SHOT-REBOOT-SAFETY.md. STOP for independent review.

- 2026-10-19: Task025 review results recorded in docs/TASK025-INDEPENDENT-REVIEW.md. Software-only checks passed; C runtime and concurrent-call protection remain open.
- 2026-10-19: Task025 independent QEMU locked ztest build 144/144 PASS; QEMU runtime failed 0xC0000135, C tests not executed. Review remains conditional; see docs/TASK025-INDEPENDENT-REVIEW.md.

- 2026-10-19: **TASK026 host C test execution: BLOCKED, C tests NOT executed.** No native C compiler/WSL/Docker/zig installed; only Zephyr SDK ARM gcc found and sandbox denied executing it (WinError 5); QEMU also previously fails 0xC0000135. Added fallback runner tests/uk_lora_one_shot/host/run_host_tests_qemu.py (never run). Source review of reboot/torn-write invariants and no-hidden-TX route: fail-closed, production CMake FATAL_ERROR intact; concurrency and real flash adapter remain open. No install, flash, reset or RF. User approval options in docs/TASK026-HOST-C-TEST-REVIEW.md. STOP for independent review.
- 2026-10-19: **TASK027 C runtime validation BLOCKED; neither suite compiled or executed.** Direct Zephyr SDK ARM GCC 14.3.0 invocation succeeded. Process-local PATH was inspected and temporarily prefixed with existing SDK QEMU/compiler directories, but QEMU remained unusable; follow-up access to the SDK QEMU executable/directory was denied by sandbox policy. A read-only inventory found QEMU-bundled DLLs but did not establish the exact missing dependency. No code/tests changed, no install/download, no flash/reset/device/USB/RF operation. See [TASK027-C-RUNTIME-VALIDATION.md](TASK027-C-RUNTIME-VALIDATION.md). Stop for independent review.

- 2026-10-19: **TASK028 one-shot concurrency hardening (PC-only).** Added injected non-blocking claim lock (frurt_os_attach_lock); fire() now holds it across check, consumed latch and durable consume, releases before radio; missing lock => NO_LOCK, contention => BUSY (shot not burned). New os_concurrency tests (every-store-access interleave, negative control, re-entry, storage error, restart) written. Actual: static check PASS, Python 45 OK, arm-gcc -fsyntax-only -Werror rc=0 for ALLOW=0/1. C tests NOT executed; locked firmware rebuild NOT done (west.exe blocked by sandbox). Production TX still disabled; no flash/RF/hardware. See [TASK028-ONE-SHOT-CONCURRENCY.md](TASK028-ONE-SHOT-CONCURRENCY.md). STOP for independent review.

- 2026-10-19: **TASK029 Zephyr/nRF52840 claim-lock adapter foundation (PC-only).** Added an isolated atomic-CAS lock adapter with a one-time init state machine and a separate compile-only Zephyr app; it is not part of root production CMake, the production one-shot app, or any TX path. Shared-lock-per-store, init/rebind, ISR and callback constraints are documented. Actual results: Task029 Python regressions 3/3 PASS; all tests Python 48/48 PASS; Task024 static TX check PASS; standalone `xiao_ble/nrf52840` Zephyr compile/link 192/192 PASS, adapter symbols present, `CONFIG_LORA`/`CONFIG_ZTEST` unset, no TX source reference. Compile only; the app and C runtime tests were not run. TX remains hard-disabled; no flash, reset, RF, hardware, install/download or global PATH change. See [TASK029-ZEPHYR-CLAIM-LOCK.md](TASK029-ZEPHYR-CLAIM-LOCK.md). **STOP for independent review.**

- 2026-10-19: **TASK032 Android BLE name build validation: BLOCKED, no Zephyr build or C test run.** Sandbox denied executing C:\zephyrproject\.venv\Scripts\python.exe (system Python lacks west). Python 48/48 PASS, Task024 static check PASS; static review: name uses node_id[3..5] matching relay FICR derivation, scan response is exactly 31 bytes, boot order OK. Task012/Task022 UF2 hashes unchanged. No code change, flash, reset, RF or hardware. See [TASK032-ANDROID-BLE-BUILD-VALIDATION.md](TASK032-ANDROID-BLE-BUILD-VALIDATION.md). **STOP for independent review.**

- 2026-10-19: **TASK033 BLE inbox history foundation (PC-only).** Added pure-C bounded 8-slot newest-first ring buffer + offset-readable image (MTU-23 safe) under src/features/ble_inbox, opt-in CONFIG_BLE_INBOX default n; no GATT/RX hook yet (proposed UUID 7b3e0006 documented). Actual: Python 52/52 PASS, Task024 static PASS. west build and ARM gcc BLOCKED in agent sandbox; C module and ztest suite NOT compiled/executed; 274/274 and Task012 hash not re-verified. No flash/reset/RF/stage/commit. See [TASK033-BLE-INBOX-FOUNDATION.md](TASK033-BLE-INBOX-FOUNDATION.md). **STOP for independent review.**

- 2026-10-19: **TASK034 BLE inbox GATT + mesh RX integration (PC-only, opt-in CONFIG_BLE_INBOX default n).** Read-only characteristic 7b3e0006 (no collision), fed at the accepted-remote-TEXT point in mesh RX (source ID + text only), mutex-protected, per-connection frozen snapshot against torn Read Blob. Actual: Python 55/55 OK, Task024 static PASS. west/venv python BLOCKED by sandbox, no ARM compiler: default/opt-in builds, ztest and Task012 hash NOT verified. No flash/reset/RF/stage/commit. See [TASK034-BLE-INBOX-GATT.md](TASK034-BLE-INBOX-GATT.md). **STOP for independent review.**

- 2026-10-19: **TASK035 automatic BLE/LoRa transport-selection policy (PC-only, simulation).** Python policy + 3-node/Android-GATT simulator in tests/hybrid_policy_sim: BLE if verified near, LoRa only if independently authorized (default not), else explicit BLOCKED/QUEUED, never false delivery; cross-transport dedup/TTL, failover, 14-byte limit, observable states. Actual: new 15/15 OK, existing 55/55 OK, Task024 static PASS. Python model only; no firmware/C change, no build, flash, reset, RF or stage/commit. Acceptance matrix and gating in [TASK035-AUTOMATIC-HYBRID-ROUTING.md](TASK035-AUTOMATIC-HYBRID-ROUTING.md). **STOP for independent review.**


- 2026-10-19: **TASK036 phased Kit3 then Kit2 BLE candidate design (PC-only, NOT built, NOT flashed).** Finding: deployed topology B/C cannot originate text and drop non-A source, so TASK035's Kit2->Kit3 write via topology image is wrong; general-policy image needed on both kits (Kit3-first phase only smoke-tests Kit3; two-way needs both). Candidate overlay tests/task036_kit2_kit3_candidate.conf (BLE_INBOX, ANDROID_DISCOVERY_NAME, DISCOVER_NO_ECHO; topology off). Hand-derived names Mesh-5A5E02 (Kit3), Mesh-E125C8 (Kit2). Build BLOCKED (venv python sandbox-denied, system python no west): no UF2/SHA256. Python 55/55 OK, Task024 static PASS. Rollback hashes: Kit2 2FFAA52A...7D773, Kit3 D5E9EF84...FEEB8. See [TASK036-KIT2-KIT3-PHASED-BLE-UPGRADE.md](TASK036-KIT2-KIT3-PHASED-BLE-UPGRADE.md). No flash/reset/RF/stage/commit. **STOP for independent review.**

- Task043 (2026-10-19): added deterministic RSSI/freshness boundary test in tests/hybrid_router; not built/run; awaiting review. See docs/TASK043-RSSI-POLICY-TEST.md.
- Task044 (2026-10-19): added exact blocked/queued result, pending-count, and zero BLE/LoRa send assertions for stale BLE with unauthorized LoRa; no build/tests run, native_sim not retried. See [TASK044-BLOCKED-ASSERTIONS.md](TASK044-BLOCKED-ASSERTIONS.md). **STOP for independent review.**
- Task045 (2026-10-19): extracted the BLE manufacturer/RX decode gate into a deterministic seam; tests assert malformed/non-mesh rejection and preservation of scan callback RSSI. No build/tests run (reviewer handles execution); no routing dispatch or hardware/RF actions. See [TASK045-RSSI-VALIDATION.md](TASK045-RSSI-VALIDATION.md). **STOP for independent review.**

- Task050 (2026-10-19): added pure `hybrid_router_result_to_gatt_status()` (accepted->0, QUEUE_FULL->-ENOSPC, INVALID->-EINVAL, DUPLICATE->-EALREADY, NOT_FORWARDED->-ENOTSUP) with table-driven ztest; no wiring, not built/run. See [TASK050-GATT-STATUS-CLASSIFIER.md](TASK050-GATT-STATUS-CLASSIFIER.md). **STOP for independent review.**
