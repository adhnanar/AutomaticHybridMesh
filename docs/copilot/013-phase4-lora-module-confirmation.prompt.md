# TASK014 — Phase 4 LoRa module confirmation and backend planning

Continue in `C:\zephyrproject\hybrid-mesh-poc`. Read `AGENTS.md`,
`docs/ROADMAP.md`, `docs/TASK012-ISOLATED-TWO-HOP-FINAL-ACCEPTANCE.md`, and
`docs/TASK013-LORA-TRANSPORT-FOUNDATION.md` first.

## Goal

Prepare the next Phase 4 implementation only after the user confirms the exact
external LoRa module and hardware wiring. The three boards are Seeed XIAO
nRF52840 Sense; none has confirmed built-in LoRa, and no external module is
currently confirmed.

## Required stop gate

Do not choose a transceiver, assign pins, add devicetree/Kconfig radio
resources, or implement a radio driver based on assumptions. If the exact
module orderable part number, SPI/CS/BUSY/DIO1/RESET wiring, regulator/power,
antenna, region/frequency plan, duty-cycle/airtime limits, or user approval is
missing, document the missing information and stop to ask the user.

## Constraints

- Preserve completed BLE relay behavior and the existing default/deployed
  image; do not wire a backend into the application until the exact hardware
  is confirmed and build/test effects are understood.
- Do not flash, erase, reset, power-cycle, or interact with physical devices.
- Do not import source, overlays, configuration, or implementation from
  `C:\zephyrproject\hybrid-mesh`.
- Keep the driver and configuration feature-scoped. Do not enable unsupported
  devicetree resources or allocate LoRa pins in the default build.
- Keep Android nRF Connect over BLE GATT as the phone interface; no custom
  mobile app.
- The packet/mock work is simulation and compile evidence only. The new
  caller-shared dedupe policy is not wired to live BLE.
- Update the task report and `docs/PROGRESS.md` with exact files, commands,
  actual results, issues, limitations, rollback, next action, and review.
- Do not stage or commit all untracked workspace files.

## After confirmation

Audit the selected module's exact datasheet and Zephyr driver support. Propose
a feature-local backend and explicit pin/power configuration, then implement
only after checking BLE scanning, advertising, and GATT coexistence. Add
automated send/receive, error, dedupe, TTL, and hardware-independent tests.
Separate mock/compile evidence from any radio validation. Do not claim
physical LoRa success without actual authorized test evidence.
