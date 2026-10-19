# TASK014 follow-up prompt — validate the opt-in XIAO Wio-SX1262 profile

Work only in `C:\zephyrproject\hybrid-mesh-poc` and the seven-phase Hybrid
BLE + LoRa Mesh PoC. Do not read/import anything from the legacy
`C:\zephyrproject\hybrid-mesh` project.

## Objective

Verify and, if necessary, fix the independent, opt-in
`tests/sx1262_hardware_profile` Zephyr app/overlay for the Seeed XIAO
nRF52840 + Wio-SX1262 SKU 102010710 against the installed Zephyr 4.5-rc1
source. Do clean builds with and without its explicit overlay, inspect
generated DTS and Kconfig/linkage, and document actual output. Never flash,
reset, erase, power-cycle, or transmit.

## Guardrails

- Keep root BLE/GATT code, configuration, topology behavior, and firmware
  images unchanged; do not link LoRa into the BLE app.
- The exact target is `xiao_ble/nrf52840`, not ESP32-S3.
- The profile overlay must remain explicit/non-default and disable `i2c1`
  because stock D4/D5 I2C mappings conflict with CS/RXEN pins.
- Verify DIO2 RF switch, RXEN active polarity, and DIO3 TCXO against
  authoritative module circuit details before adding any of them. Current
  profile intentionally omits these unresolved wiring properties; do not
  infer them from generic designs or unrelated boards.
- Do not configure an RF region, channel/frequency, RF timing, transmit
  power, or airtime policy. Do not call LoRa send/receive/configuration APIs
  in the smoke app.
- If the SDK paths remain blocked by sandbox policy, stop and report that
  blocker; do not try alternate tools to bypass it.

## Acceptance

1. Clean default build passes with no SX1262 enabled and without modifying
   BLE application sources/configuration.
2. Clean explicit overlay profile build passes and generated
   `zephyr.dts` contains the target SX1262 SPI/CS/reset/BUSY/DIO1 mapping
   plus disabled `i2c1`.
3. Generated `.config` and build output prove the Zephyr 4.5-rc1 native
   SX126x driver is selected/linked only for the opt-in profile.
4. Report actual commands/results, unresolved hardware TODOs, rollback and
   next action in `docs/TASK014-XIAO-SX1262-OVERLAY.md`; update
   `docs/PROGRESS.md`.
