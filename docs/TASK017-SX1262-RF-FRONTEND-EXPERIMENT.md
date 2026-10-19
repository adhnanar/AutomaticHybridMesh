# TASK017 — experimental opt-in SX1262 RF frontend configuration

2026-10-19. **Verdict: PASS compile/link/device-tree generation of experimental SX1262 RF frontend variant. NOT hardware-runtime tested or RF-approved.**

## Context
The user confirmed all three XIAO nRF52840 & Wio-SX1262 kit boards have radio modules with antennas connected. User is asleep and permits PC-only development without further approvals but NOT physical flashing/transmission. The transmitter's country/region/frequency and legal airtime/power are not specified. Do not transmit or flash.

## New isolated artifact
`tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay` — a second opt-in overlay; the original Task014 minimal overlay remains unchanged. Uses Zephyr native SX126x driver, SPI2 pins D8/P1.13 SCK, D9/P1.14 MISO, D10/P1.15 MOSI. CS D4/P0.04 active-low; RESET D2/P0.28 active-low; BUSY D3/P0.29 active-high; DIO1 D1/P0.03 active-high. Disable `i2c1` (stock SDA D4/P0.04 SCL D5/P0.05). Added three *experimental* RF front-end properties:
- `rx-enable-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>`: D5 RXEN, per nRF XIAO variant hardware information and manufacturer forum notes.
- `dio2-tx-enable`: DIO2 radio-controlled TX/RX switch per module manufacturer forum explanation.
- `dio3-tcxo-voltage = <SX126X_DIO3_TCXO_1V8>`: reported nRF variant oscillator voltage; Zephyr generated DTS encodes numeric 0x2 (1.8V enum). **Not electrically measured.**
Public source references:
- https://wiki.seeedstudio.com/wio_sx1262/
- https://forum.seeedstudio.com/t/wio-sx1262-refsw-dio2-and-no-actual-schematic-for-the-module/294808/10
- https://github.com/meshtastic/firmware/discussions/9424
See source-backed Task014 reviews for hardware limitations. No code imported from Meshtastic or the legacy project.

## Independent builds/static checks
From `C:\zephyrproject`: `& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:/zephyrproject/hybrid-mesh-poc/build-task017-rf-frontend-experimental C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile --pristine=always -- -DDTC_OVERLAY_FILE=C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay`
**PASS, exit0, 201/201 steps**, FLASH 55,544B RAM 15,608B. Log `docs/task017-rf-frontend-build.log`. Standalone smoke app starts native SX126x driver at firmware boot, prints device readiness only; app does not call LoRa config/send/receive.
Inspected generated `build-task017-rf-frontend-experimental/zephyr/zephyr.dts`: SX1262 child present, `rx-enable-gpios = < &gpio0 0x5 0x0 >`, `dio2-tx-enable` set, `dio3-tcxo-voltage = < 0x2 >`, CS/RESET/BUSY/DIO1 pins match, `i2c1 status = disabled`. Generated `.config` has `CONFIG_LORA=y`, `CONFIG_LORA_SX126X_NATIVE=y`, SPI enabled, SX126x native library linked. This proves **build configuration**, NOT actual module reset/readiness, RF switch correctness or radiated communication.

## Risks, limitations, next actions
- RF front-end property values reflect manufacturer descriptions, not tested on assembled boards. Wrong switch polarity or TCXO voltage may prevent radio operation; keep this variant experimental until user approves a controlled no-transmit smoke flash / instrumented runtime checks.
- Unknown legal region, center frequencies, LBT/duty-cycle/power, and regulatory domain; never activate LoRa transmit using guessed settings.
- No flash, resets, power actions or LoRa RF transmissions. No change to root BLE application or Task012/Task010 flashed images.
- Rollback: remove ONLY the experimental second overlay and this report/progress entry, leave Task014 minimal known-good overlay intact.
