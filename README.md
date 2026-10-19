# Frurt — Offline Hybrid BLE + LoRa Mesh PoC

Hardware: **three Seeed XIAO nRF52840 Sense + Wio-SX1262 kits, with LoRa modules and antennas fitted**. Firmware: fresh Zephyr `xiao_ble/nrf52840` project, not copied from the legacy `C:\zephyrproject\hybrid-mesh` repository.

## Product goal
A small, battery-powered, phone-connected offline accessory enabling nearby user discovery and text messages. Local short-range transport is a custom connectionless BLE advertising/relay protocol; long-range transport will use SX1262 LoRa; future policy automatically selects/falls back between radios. Android nRF Connect supplies the existing GATT interface; **no custom mobile app** is part of this PoC. The protocol is *not* Bluetooth SIG Mesh.

## What has actually been demonstrated
- **BLE A→B→C three-kit two-hop forwarding:** 5/5 controlled relay messages accepted by C with A TTL3 and B TTL2, after 5/5 B-off separated controls and 5/5 close-distance rejection controls. See [Task012 isolated BLE acceptance](docs/TASK012-ISOLATED-TWO-HOP-FINAL-ACCEPTANCE.md).
- **PC BLE/GATT notifications while C stays connected:** 5/5 new messages generated an exact-once GATT text notification and C accepted each; PC also captured B's TTL2 advertising in 3/5 concurrent-connect runs, and direct frames sporadically leaked/rejected. See [Task016 GATT evidence](docs/TASK016-PC-GATT-NOTIFICATIONS.md).
- **PC BLE peer discovery:** writing `DISCOVER` produced C GATT discovery notifications for both other boards in 3/3 trials. See [Task016B](docs/TASK016B-PC-PEER-DISCOVERY.md).
- **Android nRF Connect:** user screenshots on Oct 18 confirmed a real GATT connection, RX write and TX readback and notification subscription settings, plus the historical demo discovery value. A separately logged Android notification event and current mesh-peer notifications have not been independently verified on Android. See [Phase2 evidence](docs/PHASE2-BLE-GATT.md).
- **SX1262 software readiness:** native Zephyr LoRa driver and separate no-transmit smoke app compile for XIAO; SPI, CS/BUSY/DIO1/RESET and I2C1 conflict resolved in an opt-in overlay. An additional *experimental* RF front-end overlay compiles with RXEN, DIO2 and DIO3 TCXO settings. Neither has been run on the radio. See [Task014](docs/TASK014-INDEPENDENT-REVIEW.md) and [Task017](docs/TASK017-SX1262-RF-FRONTEND-EXPERIMENT.md).
- **Hybrid route simulation:** opt-in C routing policy and Python reference model for BLE preference, LoRa fallback, shared packet dedup, TTL and retry. Python scenarios **13/13 PASS**; Zephyr C ztest sources compiled/linked, but **not executed**. See [Task015 independent review](docs/TASK015-INDEPENDENT-REVIEW.md).

## Current deployment
- Kit 1/A: USB COM14, BLE `E5:12:76:D2:56:EA`, Task012 BLE firmware.
- Kit 2/B: USB COM15, BLE `C8:51:08:6E:08:C0`, Task010 BLE firmware.
- Kit 3/C: USB COM16, BLE `E2:DB:6A:54:BB:71`, Task012 BLE firmware.

The **current deployed firmware does NOT operate the SX1262 LoRa radio**. No LoRa RF multi-hop, automatic on-device BLE/LoRa switching or mixed-radio delivery is claimed.

## Safe PC-only verification
From the repo on Abi:
```powershell
& C:\zephyrproject\.venv\Scripts\python.exe tests\pc_ci.py
& C:\zephyrproject\.venv\Scripts\python.exe tests\pc_ci.py --full
```
The quick command checks Python suites/syntax; `--full` also pristine-compiles BLE, the LoRa and hybrid ztest apps and both opt-in SX1262 configurations. It never flashes, resets, powers boards or transmits LoRa RF. The result is [docs/PC-CI-LATEST.md](docs/PC-CI-LATEST.md); individual logs under `docs/ci/`. A C ztest compile does **not** equal ztest runtime acceptance.

## Remaining blockers
1. **LoRa regulatory region** (country and intended legal frequency/power/airtime limits) must be confirmed by the operator; do not infer from IP/location.
2. **Explicit per-image flash approval** required before loading an SX1262 smoke, revised GATT, or hybrid firmware. No approval has been granted for these newer builds.
3. Live LoRa init/RX/TX, LoRa A→B→C RF relay, mixed BLE/LoRa route restoration, Android nRF Connect notification logs, power/range/battery tests remain open.

See [seven-phase roadmap](docs/ROADMAP.md) and [progress log](docs/PROGRESS.md) for scope and evidence. 
