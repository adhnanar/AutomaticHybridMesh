# TASK013 — independent review of LoRa transport foundation

**Reviewed:** 2026-10-19. **Verdict: PASS for isolated, not-deployed LoRa transport groundwork, with runtime and hardware gates OPEN.**

## Review scope
After user reported Copilot complete, queried `code agent ps`: **no active sessions**. The `copilot_done.txt` marker did not exist, consistent with the prior unverified VS Code/Copilot CLI hook mismatch. Reviewed every newly added C module/header in `src/features/lora_transport/`, standalone C ztest sources and CMake/Kconfig configuration, the Task013 Copilot report, and underlying packet/dedup implementation in `src/features/ble_mesh_relay/ble_mesh_packet.c/.h`. The Git working tree is primarily untracked, so no reliable historical Git diff exists; review is file-based with binary/build cross-checks.

### Implementation assessment
- Transport contract delegates raw frame send through an injected `send_frame` callback; `lora_transport_send_packet` validates configuration, reuses the existing packet encoder and propagates its errors and the backend return code. The decode path reuses the known BLE packet format.
- Four-frame bounded in-memory mock tests deterministic frame capture and send-error injection. No LoRa driver or hardware access is falsely implied.
- `lora_mesh_prepare_relay` validates packet, rejects nonforwardable TEXT/TTL <=1, checks a caller-provided dedup cache keyed on source+seq, then decrements TTL. The existing BLE application has **not** been wired to the new cache, so cross-radio deduplication is simulated only. The cache is not thread-safe; concurrent ingress will need locking.
- The dedup cache is marked on admission **before a radio send**. A future hybrid router must retain failed sends in a retry queue (without re-admitting them) or adopt an explicit transactional reservation/commit policy; otherwise retrying through `lora_mesh_prepare_relay` would be suppressed. Current code has no transmit scheduler, so this is a future-integration constraint rather than a deployed regression.
- No actual LoRa radio, driver, devicetree overlays, or specific RF region is configured. Root application and deployed BLE firmware were not modified by Task013.

## Independent verification performed
1. `C:\zephyrproject\.venv\Scripts\python.exe -m unittest discover -s tests -p 'test_*.py' -q` in the repo: **31 tests PASS**, exit0.
2. Python syntax checked the existing PC sensitivity/isolation/relay scripts: **PASS**, exit0.
3. Independently executed a **fresh clean build**:
   `& C:\zephyrproject\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task013-independent-review-ztest C:\zephyrproject\hybrid-mesh-poc\tests\lora_transport --pristine=always`.
   Result: **PASS**, exit0; **202/202** build steps; FLASH **69,416 B**, RAM **16,504 B**; ELF/UF2 generated; raw build log `docs/task013-independent-review-ztest.log`.
4. Cross-checked the preexisting Task013 default and topology UF2 binaries byte-for-byte via SHA-256 against the previously validated Task012 respective binaries:
   - Topology: **D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8** for both Task012 and Task013 (272,384 B).
   - Default: **2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14** for both Task012 and Task013 (268,288 B).
   Therefore Task013's builds have not altered the existing BLE firmware images. Copilot separately reported two clean app builds; those builds were *inspected and hash-cross-checked*, not independently rerun.
5. **C ztest cases not executed**: Windows has neither host gcc/clang on PATH nor QEMU; Copilot's native_sim Twister runs failed during manager startup. On-device execution would require a separate user-approved flash to a dedicated test board and was **not performed**.

## Acceptance and next action
- **Task013 software groundwork is accepted.** It cannot count toward Phase4 *radio* acceptance, LoRa A→B→C RF messaging, LoRa discovery, automatic BLE/LoRa selection or Android delivery.
- Before choosing a Zephyr LoRa driver or pin wiring, obtain the exact LoRa transceiver/module and matching antennas, operating region/frequency and power constraints. At least three suitable LoRa interfaces are needed for a three-device LoRa relay proof. Each XIAO nRF52840 Sense does not include LoRa internally.
- Next scoped task after hardware confirmation: validate SPI pins/CS/BUSY/DIO1/RESET, power and antenna, select compatible Zephyr driver, create hardware-only overlay and host tests, preserve the BLE path; request explicit authorization before any new firmware flash.
- `copilot_done.txt` hook remains unverified: no marker observed when Copilot was idle.
