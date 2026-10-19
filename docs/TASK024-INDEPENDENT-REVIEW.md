# Task024 — Independent review of UK LoRa one-shot transmitter

Reviewed 2026-10-19 after Copilot reported completion and VS Code was reopened at `C:\zephyrproject\hybrid-mesh-poc`.

**Verdict: ACCEPT locked PC-only software/test foundation. DO NOT DEPLOY OR ENABLE LoRa TX.** The user has not authorized a specific Kit2 firmware flash; verified antenna peak gain / RF power calibration and compliance are still missing. All three USB identities checked and original BLE images are unchanged.

## Verified implementation
- Examined `src/features/uk_lora_one_shot/uk_lora_one_shot.h/.c`, isolated `tests/sx1262_uk_one_shot/{CMakeLists.txt,src/main.c,src/radio_tx.c}`, code/report and Python static checks.
- `FRURT_UK_ONE_SHOT_TX_ALLOW` default OFF in CMake; both it and `UK868_ALLOW_GATE_OPEN` are forced to 0 in the locked application and feature source compile requires both locks match.
- Only allowed variant includes `src/radio_tx.c` with real `lora_config`/`lora_send` calls. Locked build does not compile that source. Static source + ELF/map check passed. Native SX1262 Zephyr driver itself contains TX APIs by design; the app never invokes them in the locked image.
- The locked build uses Kit1 receiver-matching 868.3MHz, BW125k, SF9, CR4/5, preamble8, private sync and CRC enabled (Zephyr lora_modem_config packet_crc_disable defaults to false). Payload 10 bytes, margined airtime 151,604 microseconds.
- Gate requires operator region, documented ERP, equipment compliance, **verified** antenna gain; none of the latter are currently documented. Candidate -9 dBm conducted is tentative, NOT proof of <25mW ERP.
- One-shot state latches consumed before callback; fail counts full reserved airtime. **Important fail-safe deficiency for future TX**: state and budget are volatile and reset on reboot; an allow image can transmit again after each reboot. Restrict future transmit-capable build until reboot-persistent one-shot disarm or an equally strong independent hardware/operational control exists. A 30-second abort delay is not a reliable substitute.

## Independent verification
1. Executed `west build -b xiao_ble/nrf52840 -d build-task024-independent-locked tests/sx1262_uk_one_shot --pristine=always -- -DDTC_OVERLAY_FILE=tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay` with full paths adapted: **PASS 203/203** compile/link; FLASH 56,836B, RAM 16,888B, UF2 114,176B. Log: `docs/task024-independent-locked-pristine.log`.
2. UF2 SHA256 `D2EC03A68E6BB860BC2BDBB22817BD59EFB28D27C46F7DEA78112549F9803439` identical to Copilot's locked build. Generated `CMakeCache.txt` confirms `FRURT_UK_ONE_SHOT_TX_ALLOW:BOOL=OFF`.
3. `python tests/task024_static_tx_check.py --build build-task024-independent-locked`: **PASS**. Scope source/main + ELF/map, not a guarantee against future added modules.
4. `python -m unittest discover -s tests -p 'test_task024*.py' -q`: **13/13 PASS**. Full repository Python discovery **44/44 PASS**; independent hybrid Python reference suite **13/13 PASS**. Initial test invocation `python -m unittest tests.test_task024_static_tx_check ...` failed due to tests directory not being an importable Python package; rerunning correct discovery form passed.
5. `python tests/task024_airtime_check.py`: PASS, 10B frame raw 144.384ms, +5% 151,604us.
6. Both validated Task012 BLE default and topology UF2 hashes unchanged: default `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`, topology `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`.
7. All three kits still enumerate on Abi, Kit1 COM14 original USB serial `1D5EA4A525ED34EB` running RX, Kit2 COM15 `224BC825E1DA130C` BLE, Kit3 COM16 `B13E025E5A1407EE` BLE.
8. **C ztest runtime NOT executed**. Zephyr SDK Windows QEMU launcher `qemu-system-arm.exe --version` fails exit `-1073741515` (0xC0000135, missing DLL/dependency), no host GCC/Clang/Zig. Copilot's C ztest compilation and user's Python test success cannot substitute for C runtime acceptance.

## Open gates
**No flash/reset/LoRa TX during this review.** True antenna gain/power tolerance and RF front-end loss/equipment compliance must be established for specific supplied antenna assembly; UK Ofcom 868.0-868.6MHz 25mW ERP / 1% duty option not equivalent to blanket radio authorization. One-shot TX reboot rearming must be corrected, and C tests should execute on an available host/emulator before advancing. Separately authorize any future Kit2 test image flash and RF transmission.

Rollback: remove only Task024 feature/test paths and docs, retain BLE and Task021/022 hardware firmware. Further improvement delegated to Task025; keep TX compile locked and do not enable transmitter.
