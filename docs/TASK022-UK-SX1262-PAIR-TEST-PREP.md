# Task022 — UK SX1262 radio-pair PoC, stage 1 RX only

Date: 2026-10-19. User has authorized proceeding toward a controlled UK LoRa TX/RX proof, but **this stage does not transmit**.

## Verified configuration
- Three Seeed XIAO nRF52840 Sense + Wio-SX1262 kits; Kit1 on COM14 already running Task021 v2 SX1262 driver diagnostic and physically reported DEVICE_READY=1 repeatedly. Kits2/3 retain the original BLE firmware.
- Ofcom IR2030/1/16 UK license-exempt 868.0–868.6MHz band, 25mW ERP and either designated-standard equivalent spectrum access/interference mitigation or duty-cycle <=1%. Source: https://www.ofcom.org.uk/siteassets/resources/documents/spectrum/interface-requirements/ir-2030.pdf
- RX-only test image: `tests/sx1262_uk_pair/`; XIAO Zephyr native driver, opt-in tested experimental RF switch overlay, LoRa RX modem config **868.3MHz, 125kHz, SF9, CR4/5, preamble 8, private sync**. No LoRa send calls or TX command in this program. Serial log prints `FRURT_TASK022_UK868_RX_ONLY_V1`, driver ready, configuration result, and loop `TASK022_RX_WAIT_NO_PACKET`/packet bytes with RSSI/SNR.
- Fresh XIAO `west build` **201/201 PASS**, FLASH55944B RAM15608B, UF2 112128B SHA256 `28C2852D6C714D428D0269ECAF48BC3E8ACEEAFFF4D672E69BAEDECD738C4644`. Raw build: `docs/task022-uk-receiver-build.log`. **Not yet flashed or runtime-tested**.
- Receiver role can be installed on Kit1 without disrupting Kits2/3. The previously flashed Task021 remains restorable; BLE Task012 original UF2 also preserved.

## Radio-transmit gate
- No transmitter image built/installed yet; no LoRa RF transmissions occurred in Task022 stage1.
- Ofcom power limit is effective radiated power (ERP), not requested SX1262 register output. Antenna gain, front-end losses, configured output power and device conformity must be reviewed before real TX. Using a generic manufacturer default up to +22dBm would be unsafe under this band limit.
- Intended later TX test: one bounded payload from Kit2 to Kit1 at 868.3 MHz with conservative power and verified airtime budget. Requires TX image review and explicit board-specific flashing step; no unrestricted radio beaconing.

## Next practical step
Request user to double-press RESET on Kit1 only for UF2 bootloader. After confirming drive board ID and UF2 checksum, copy RX-only firmware and inspect COM14 status. This tests RX configuration readiness without any transmitted LoRa signal. For first RF delivery, prepare separately gated Kit2 transmitter after antenna/ERP assessment; do not use unverified stock TX output.
