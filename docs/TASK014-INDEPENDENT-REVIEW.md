# TASK014 — Independent SX1262 Profile Review

Date: 2026-10-19. **Verdict: PASS for opt-in compile/link and device-tree integration ONLY. Physical SX1262 initialization, RF switching, RX/TX and mesh integration are not yet accepted.**

## Operator and constraints
The user confirmed **all three XIAO nRF52840 + Wio-SX1262 kits have LoRa modules and antennas connected**. The exact radio country/region, legal frequency, allowed TX power/duty cycle, RF-switch details of specific module board revisions, and active LoRa radio communication are not yet confirmed. User is sleeping and requested maximum **nonphysical PC-only software work**. No firmware flash, reset, board power action or RF transmission was performed.

## Code review
- Inspected `tests/sx1262_hardware_profile/`: CMake, prj.conf, overlay, main.c. Isolated Zephyr app uses `CONFIG_LORA=y`, `CONFIG_LORA_MODULE_BACKEND_NATIVE=y`, `CONFIG_LORA_SX126X_NATIVE=y` and `CONFIG_PRINTK=y`.
- Board overlay explicitly opts into `xiao_spi=&spi2`, 1 MHz SPI radio child, CS D4/P0.04 active-low; SX1262 reset D2/P0.28 active-low, busy D3/P0.29 active-high, DIO1 D1/P0.03 active-high. The overlay disables `i2c1` because its default SDA/SCL (P0.04/P0.05) clash with radio CS and proposed RXEN.
- Existing SPI2 pad mappings used by stock XIAO nRF52840 pinctrl: SCK D8/P1.13, MISO D9/P1.14, MOSI D10/P1.15.
- Smoke app only checks `device_is_ready`; it does not call `lora_send`, `lora_recv` or `lora_config`. Zephyr driver may still reset/init chip at boot, so **no flashing is authorized**.
- No changes to root BLE application, root Kconfig, BLE source, or BLE default overlay.
- RF front-end lines were deliberately omitted from Task014 due to module-side confirmation concerns. Public Seeed Wio module documentation says DIO3 supplies the TCXO; Seeed engineer clarified DIO2 to RF-switch CTRL and RF_SW to its complementary control (RX high), while Meshtastic XIAO nRF52840 variant config documents DIO2 TX, RXEN D5, TCXO 1.8V. These are strong design leads, **not measured runtime proof for the assembled three boards**. Source references:
  - https://wiki.seeedstudio.com/wio_sx1262/
  - https://forum.seeedstudio.com/t/wio-sx1262-refsw-dio2-and-no-actual-schematic-for-the-module/294808/10
  - https://github.com/meshtastic/firmware/discussions/9424
No additional RF GPIO property added in this review.

## Independent builds and evidence
1. First opt-in build command with native Windows backslashes in `DTC_OVERLAY_FILE` **FAILED early**: CMake `Invalid character escape '\z'`; log `docs/task014-independent-sx1262-build.log`. This was a command quoting/Windows path formatting issue, not device tree or driver failure.
2. Re-ran with forward-slash overlay path and pristine build:
`C:\zephyrproject\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:/zephyrproject/hybrid-mesh-poc/build-task014-independent-sx1262 C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile --pristine=always -- -DDTC_OVERLAY_FILE=C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262.overlay`.
**PASS**: 201/201 build steps, FLASH 55,544 B, RAM 15,608 B, opt-in UF2 111,104 B. Log `docs/task014-independent-sx1262-build-pass2.log`.
3. Parsed actual `build-task014-independent-sx1262/zephyr/zephyr.dts`: `/soc/spi@40023000/sx1262@0` is `compatible=semtech,sx1262` with matching CS P0.04, reset P0.28, busy P0.29, DIO1 P0.03; `/soc/i2c@40004000` status **disabled**. Actual `.config` selects CONFIG_SPI=y, CONFIG_LORA=y, CONFIG_LORA_MODULE_BACKEND_NATIVE=y and CONFIG_LORA_SX126X_NATIVE=y; output contains linked `zephyr/drivers/lora/native/sx126x/libdrivers__lora__native__sx126x.a`.
4. Independently pristine rebuilt the **root default BLE application** (log `docs/task014-independent-default-build.log`, PASS), SHA256 UF2 `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`, byte-identical to Task012 default.
5. Independently pristine rebuilt the **root topology BLE application** (log `docs/task014-independent-topology-build.log`, PASS), SHA256 UF2 `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`, byte-identical to physically validated Task012 topology.
6. Prior Task013 31 Python unittests PASS and independent C ztest compile/link PASS. Task014 is driver/profile smoke build only, **not a radio or hardware test**.

## Limits and next actions
- Native SX126x driver is selected and linkable, but there is **no runtime `device_is_ready` test**: C smoke app never flashed or run on boards.
- Correct RXEN/DIO2 and DIO3 TCXO configuration is still a separate opt-in configuration and verification step; do not mark hardware radio operational until board-specific positive RX/TX evidence.
- RF country and frequencies must be explicitly confirmed by user before any transmission (do not derive from coarse geolocation). Do not use LoRa radio physically or flash without explicit authorization.
- Task015 can proceed on PC to implement and test a mock hybrid router without real RF, while this hardware acceptance gate remains open.
- Rollback: remove `tests/sx1262_hardware_profile/` and its Task014 doc/prompt files only. No active firmware changed.
