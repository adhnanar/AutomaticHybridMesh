# Copilot Agent — Phase 1 fresh firmware setup
You are working ONLY in C:\zephyrproject\hybrid-mesh-poc. Read AGENTS.md and docs/ROADMAP.md first.

TASK: Establish a fresh Zephyr firmware skeleton for Phase 1 with feature-level modules (e.g. features/system_status, features/ble, features/lora, features/diagnostics) and small main entrypoint. Reuse installed Zephyr toolchain only, NEVER legacy application code, overlays, configs or firmware. Inspect exact board target and radio support before generating board-specific files.

1. Inspect Zephyr version, board definitions and SDK. Do not assume ESP32; user may have XIAO nRF52840 + Wio-SX1262 kits.
2. Implement minimal boot/status feature and modular CMake layout. Avoid enabling unverified radio pins or features.
3. Attempt a clean build for a VERIFIED board target, capture command and output. Never flash or erase automatically.
4. MANDATORY: Create/update docs/PHASE1-BASELINE.md and docs/PROGRESS.md with files changed, exact build command, real outcome, errors, rollback and next action. Every later Copilot task must likewise update at least one Markdown file.
5. If hardware identity or target cannot be confirmed, stop at a safe scaffold and explicitly document the blocker. Do not invent test results.

Acceptance: documented, reproducible clean build; no claim of hardware operation until flashed and observed.
