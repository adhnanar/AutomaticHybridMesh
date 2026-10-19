# Task023 — UK SX1262 TX preflight: closed-gate build and original antenna review

Date: 2026-10-19. **Result: PASS PC-only, NO actual transmission.**

## Hardware identification
User confirmed all three nRF52840 Wio-SX1262 kits are fitted with their **original included Seeed LoRa antennas**. This rules out user-changed antennas but does NOT establish a specified maximum antenna gain in dBi. Manufacturer nRF kit wiki (https://wiki.seeedstudio.com/xiao_nrf52840%26_wio_SX1262_kit_for_meshtastic/) gives radio type/frequency and kit configuration but does not publish a verified peak gain for this exact included antenna. Third-party 102010710 distributor listings confirm a U.FL/IPEX antenna is included (https://www.digikey.com/en/products/detail/seeed-technology-co-ltd/102010710/26553900). **Do not reuse 2 dBi specification from distinct XIAO ESP32-S3 + Wio kit**, whose pinout and antenna bundle differ. Some Seeed 868 MHz antennas are specified as 2.8 dBi or more and are NOT interchangeable assumptions (https://wiki.seeedstudio.com/lora_antenna_selection_guide/).

## Regulatory gate
Ofcom UK IR2030/1/16 (https://www.ofcom.org.uk/siteassets/resources/documents/spectrum/interface-requirements/ir-2030.pdf) specifies 868.0–868.6 MHz 25mW ERP and spectrum access mitigation or alternative max1% duty. UK test proposed center 868.3MHz, bandwidth 125k, spreading factor 9, CR4/5. A **verified worst-case antenna gain**, RF chain losses, radio output accuracy and compliance assessment are still outstanding; user confirming factory included antennas does not establish those. No RF emission permitted by this task.

## Standalone safe preflight
Created `tests/sx1262_uk_tx_locked/CMakeLists.txt`, `prj.conf`, `src/main.c`. It:
- Requires real native Zephyr SX1262 hardware overlay, yet does not transmit or call `lora_config`, `lora_send`, or `lora_send_async`.
- Compiles `src/features/uk_lora_regulatory/uk_lora_regulatory.c` with explicit `UK868_ALLOW_GATE_OPEN=0` and BUILD_ASSERT prevents enabling compile lock.
- Examines default UK868 profile and 10-byte LoRa packet airtime estimate, tests `uk868_gate_open` with compliance signoffs absent and confirms airtime reservation denied.
- Prints unique marker `FRURT_TASK023_UK_TX_LOCKED_PREFLIGHT_V1`; **not a live TX firmware**.

Independent **pristine Zephyr xiao_ble/nrf52840 compile/link 202/202 PASS**, FLASH56,788B RAM15,608B, UF2 113,664B, SHA256 `8B0FBD0A33372ECA783A4389B111D77BDAA687C320CDB52C8FBD27189C6DAAC5`. Log `docs/task023-uk-tx-locked-build.log`. Source regex `lora_send\s*\(` had zero matches. Three USB serial identities remain visible, Kit1 COM14 RX-only; Kit2 COM15 BLE; Kit3 COM16 BLE. **Preflight firmware itself has not been flashed**, and no actual SX1262 radio TX occurred.

## Next
Actual RF packet exchange is **not yet achieved**. Prepare an explicit one-shot TX firmware only after reviewing antenna/equipment conformance, worst-case ERP and selected driver power and airtime with competent evidence. Then seek specific authorization for Kit2 image flash and RF transmission, with intended LoRa payload and stop conditions; keep Kit3 unchanged. No need for physical help right now. If supplier cannot document the actual included antenna, consider an RF engineer/measurement or another manufacturer-spec'd approved antenna/assembly rather than guessing.
