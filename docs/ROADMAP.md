# Frurt — Hybrid BLE + LoRa Seven-Phase PoC Roadmap

Updated 2026-10-19. Goal: offline messaging and user discovery over automatic short-distance BLE and longer-distance LoRa mesh, presented over standard BLE GATT in Android nRF Connect (no custom app). This is a custom BLE relay rather than Bluetooth SIG Mesh.

## Phase status (evidence-bound)

| Phase | Status | Evidence and explicit outstanding gate |
|---|---|---|
| **1 — Hardware and Zephyr firmware setup** | **Partial** | Three XIAO nRF52840 Sense boards identified and flashed with BLE test firmware; operator confirmed Wio-SX1262 modules and antennas installed on all three; SX1262 radio **not initialized or tested**. |
| **2 — Android BLE GATT interface** | **Partially validated** | Android screenshots showed connection, RX write, TX readback, discovery demo/CCC subscription enabled. Separate Android notification log/event and real current peer-discovery events remain open. PC-side actual notifications tested in Phase6 work. |
| **3 — BLE peer discovery and A→B→C relay** | **PoC acceptance PASS** | Custom BLE relay physical A/C sensitivity 5/5; B-off negative 5/5; B-on relay 5/5 with TTL2 and exact C GATT payload. Note real RF isolation varies: intermittent direct-frame RX in later runs. [Task012](./TASK012-ISOLATED-TWO-HOP-FINAL-ACCEPTANCE.md). |
| **4 — SX1262 LoRa mesh** | **Software build only** | Zephyr SX1262 native driver/profile and RF frontend experimental variant compiled. No real LoRa configuration, discovery, message TX/RX or multi-hop test. [Task014](./TASK014-INDEPENDENT-REVIEW.md), [Task017](./TASK017-SX1262-RF-FRONTEND-EXPERIMENT.md). |
| **5 — Automatic hybrid route selection** | **Mock policy only** | Feature-scoped hybrid routing C code compiles; 13/13 Python reference tests. Actual C ztest cases not run; on-device mixed BLE/LoRa not integrated. [Task015](./TASK015-INDEPENDENT-REVIEW.md). |
| **6 — Phone notifications and discovery** | **PC BLE GATT verified, Android live log pending** | Exact-once C GATT text notify 5/5 during BLE mesh relay; BLE peer discovery notifications for A/B 3/3. nRF Connect live event logs and true hybrid message notification pending. [Task016](./TASK016-PC-GATT-NOTIFICATIONS.md), [Task016B](./TASK016B-PC-PEER-DISCOVERY.md). |
| **7 — Field and power tests** | **Not begun** | Full LoRa fallback, range, packet delivery ratio, latency, battery/energy, real environment/interference evaluation pending. |

## Product-level acceptance checklist

- [x] Real XIAO nRF52840 → Android nRF Connect GATT connection and RX write/TX readback.
- [x] Physical three-node BLE A→B→C relay using custom advertising payload with exact TTL/sequence evidence.
- [x] PC GATT message notifications delivered during BLE relay.
- [x] PC GATT mesh peer discovery notifications for two peers.
- [x] Zephyr SX1262 driver/overlay and hybrid software test targets compile for hardware.
- [ ] Runtime initialize SX1262 on actual kits (no flash approved for LoRa).
- [ ] Physical LoRa A→B→C forwarding with B-off isolation negative and B-on positive controls.
- [ ] Mixed BLE→LoRa and LoRa→BLE restoration under controlled link loss, duplicate suppression and TTL rules.
- [ ] Android nRF Connect live notification log from current firmware and real mesh discovery/LoRa message.
- [ ] Robust telemetry, security/authentication policy, radio-region constraints, throughput, endurance/power and battery integration.

## Immediate engineering gates

1. **Do not transmit LoRa until country/region, channel center frequency, output power, antenna rules and duty-cycle/dwell limits are confirmed.** Never infer from geolocation. SX1262 kit advertised band ≠ local legal permission.
2. Keep deployed Task012 A/C and Task010 B unchanged until user explicitly approves each new firmware flash. Software-only profiles can build while waiting.
3. Before real hybrid firmware, validate module-specific RXEN D5, DIO2 RF switch, DIO3 TCXO configuration, SPI power/pin conflicts and physical radio readiness.
4. Ensure BLE advertising/scanning/GATT concurrent with SX1262 driver does not exceed CPU/RAM/power budget. PC BLE GATT concurrency was tested; SX1262 concurrency is untested.
5. Full C ztest runtime blocked on Windows native_sim/Twister; independent Zephyr compile/link is available, Python reference tests separately run.

## Engineering rules

Fresh code only; no legacy local project source or Meshtastic firmware copied. Keep feature modules and Markdown verification per task, independent Copilot review and tested evidence. User approves flashing; no custom Android app. Documentation at [PROGRESS.md](./PROGRESS.md), [README.md](../README.md). 
