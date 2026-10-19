# Copilot Agent — Phase 1 hardware audit and safe next steps

Read AGENTS.md and docs/ROADMAP.md. Work ONLY in this fresh project. Do not reuse old firmware.

1. Verify the existing successful build artifacts and report their board target and timestamps.
2. Inspect present USB devices/serial ports and determine whether an actual XIAO nRF52840 is connected. Do not mistake Bluetooth virtual COM ports for the board.
3. Inspect available Zephyr XIAO board definitions and non-destructively research whether Wio-SX1262 shield is supported in the installed tree. Do not invent pin mappings.
4. Create a reproducible hardware checklist for three kits, including board ID, USB port, LoRa antenna, firmware image and actual boot evidence.
5. Prepare a fresh Git repository and .gitignore if safe, keeping build output ignored; never modify legacy project or commit secrets. No push.
6. MANDATORY: Update docs/PHASE1-BASELINE.md and docs/PROGRESS.md with every change, exact commands, actual evidence, blockers, rollback and next action.
7. Do not flash, erase or change bootloaders. If hardware is absent, stop at the physical-connection gate and report what the user must connect.
