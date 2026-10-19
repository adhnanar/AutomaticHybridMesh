# Task021 — No-transmit SX1262 driver initialization diagnostic

Date: 2026-10-19. **PC software acceptance: PASS build/link/static verification. Hardware radio check pending an explicitly approved test firmware flash.**

## Why
User confirmed three XIAO nRF52840 Sense + Wio-SX1262 kits with antennas fitted; USB devices remain A COM14 serial `1D5EA4A525ED34EB`, B COM15 `224BC825E1DA130C`, C COM16 `B13E025E5A1407EE`. The validated BLE A/C Task012 and B Task010 firmware must remain unchanged unless the user approves a test image flash.

## Implementation
Standalone source `tests/sx1262_no_tx_diagnostic/` (CMakeLists, prj.conf, src/main.c) uses the existing experimental opt-in overlay at `tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay`. It links Zephyr 4.5 native SX126x driver, requires enabled SX1262 DTS and RXEN, DIO2 and DIO3 TCXO properties via BUILD_ASSERT, then prints boot marker `FRURT_TASK021_NO_TX_DIAGNOSTIC v1`, radio device name, and `TASK021_SX1262_DEVICE_READY=1/0`, result and a 30-second alive heartbeat. No calls to `lora_config`, `lora_send`, receive API, GATT or advertising. **Zephyr's native SX126x driver does initialize hardware during boot, so later approved flashing may toggle RESET and SPI internally; device_is_ready does not prove silicon ID, RX signal quality, usable TCXO or RF link.**

Companion read-only PC harness: `tests/task021_pc_serial_capture.py --kit A --seconds 45` automatically locates correct USB CDC port by unique serial ID and reads logs; never flashes, transmits, or sends text commands; warns if boot marker missed. UART open on Windows may manipulate DTR depending on driver defaults; therefore use after conscious firmware deployment only.

## Verification
Fresh independent `west build -b xiao_ble/nrf52840 -d C:/zephyrproject/hybrid-mesh-poc/build-task021-no-tx-diagnostic C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_no_tx_diagnostic --pristine=always -- -DDTC_OVERLAY_FILE=C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay`.
**PASS 201/201 compile/link steps**, FLASH 55,748 bytes, RAM 15,608 bytes, UF2 111,616 bytes. Build log `docs/task021-no-tx-diagnostic-build.log`.
Generated UF2 SHA-256: `FC28283847C6217CA0CC4BFDE308E05EEB15764A8A8FD1DEFD32B7BA8B8EDAB2`.
Native SX126x driver linked; generated DTS contains `rx-enable-gpios`, `dio2-tx-enable`, `dio3-tcxo-voltage`; native LORA and SPI Kconfig enabled. Diagnostic source scan showed no LoRa TX/config function invocations. PC serial script py_compile PASS.
Existing BLE topology UF2 hash remains `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8` (Task012 validated build).

## Limitations and next physical gate
No GPIO diagnostic reads of RESET/BUSY/DIO1 and no SPI status command were added: modifying lines already claimed by native driver could introduce risk or misleading evidence. The test reports driver initialization only, not genuine chip response. C test executable has not been run on a kit. Kit B remains Task010, A/C Task012.
The next safe step is user authorization **for Kit1/A only** to flash exactly this Task021 UF2 and capture USB CDC logs from PC. This temporarily removes Kit1's BLE mesh role until re-flashing its original, approved Task012 image; backup path must remain available. Because UF2 bootloader mode requires physical reset double-click, ask user to double-click Kit1 RESET only AFTER explicit flash authorization and leave B/C untouched.
No LoRa TX should occur; local UK operation identified but actual Ofcom conformity and TX power/airtime pending. Do not expand to kit B/C or LoRa RF without separate approval.
