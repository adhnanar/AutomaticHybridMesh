# Task014 — Seeed XIAO nRF52840 + Wio-SX1262 hardware facts (research gate)
Date: 2026-10-19.

User identified hardware as the **Seeed XIAO nRF52840 & Wio-SX1262 Kit** (Seeed SKU 102010710, SX1262 862–930 MHz). User has not yet explicitly confirmed whether each of the three connected XIAO boards has the Wio-SX1262 add-on and antenna installed or the actual RF operating country/region. No RF transmissions authorized.

Seeed manufacturer's kit wiki:
https://wiki.seeedstudio.com/xiao_nrf52840%26_wio_SX1262_kit_for_meshtastic/
Wio SX1262 module datasheet:
https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262_Module_Datasheet.pdf
Independent public pin mapping (Meshtastic Seeed XIAO nRF52840 kit variant):
https://github.com/meshtastic/firmware/discussions/9424
Official Zephyr board mappings *on this PC*, as applicable to the currently deployed xiao_ble/nrf52840 target:
C:\zephyrproject\zephyr\boards\seeed\xiao_ble\seeed_xiao_connector.dtsi
C:\zephyrproject\zephyr\boards\seeed\xiao_ble\xiao_ble_common.dtsi
C:\zephyrproject\zephyr\boards\seeed\xiao_ble\xiao_ble-pinctrl.dtsi
Installed Zephyr SX1262 binding: C:\zephyrproject\zephyr\dts\bindings\lora\semtech,sx1262.yaml (includes sx126x-base with reset/busy/dio1, optional dio2-tx-enable, dio3-tcxo-voltage and rx-enable-gpios).

Reported nRF52840 kit pinout (NOT the ESP32S3 BTB variant):
- SPI clock XIAO D8 = nRF P1.13; MISO D9 = P1.14; MOSI D10 = P1.15. Zephyr xiao_spi = &spi2, already enabled, pinctrl matches.
- SX1262 NSS/CS XIAO D4 = P0.04.
- DIO1 XIAO D1 = P0.03.
- BUSY XIAO D3 = P0.29.
- NRESET XIAO D2 = P0.28.
- RXEN/RF receive switch XIAO D5 = P0.05, while TX side may be driven by SX1262 DIO2; confirm from exact nRF kit board schematic, do not assume ESP32 variant.
- DIO3 TCXO 1.8V configuration appears in public kit variant but must be validated against Seeed schematic/installed Zephyr driver.
- **Pin conflict**: xiao_i2c=&i2c1 is enabled by default, using SDA P0.04 D4 and SCL P0.05 D5. A hardware-specific LoRa overlay must disable &i2c1 or ensure no concurrent I2C use before routing CS/RXEN on these pins. Preserve USB and BLE coexistence.
- Do NOT enable SX1262 in the default BLE build automatically or affect firmware flashed on Kit1-3.
- Do NOT emit LoRa RF until antenna connectivity, three radio modules, country, allowed center frequency, radiated power and airtime/duty cycle are confirmed. UK/EU/US settings must not be guessed from user IP.
- Use original documentation and installed Zephyr binding; do not copy code/config from legacy local project.

Task014 should build an opt-in, compile-only profile and testable scaffolding for this exact hardware, with separate documented hardware confirmation and future RF-acceptance gate.

## Operator confirmation — 2026-10-19

The user confirmed all three kits have Wio-SX1262 boards and LoRa antennas connected. RF country, channel, RF-switch wiring and TCXO details are still not established. This updates the earlier pending hardware-population confirmation only.
