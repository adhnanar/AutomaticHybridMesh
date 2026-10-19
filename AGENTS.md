# Project operating rules
These rules apply to all work in this repository, including GitHub Copilot Agent.
1. Implement ONLY the fresh seven-phase Hybrid BLE + LoRa Mesh PoC roadmap. Do not import or copy any source, overlays, configuration or implementation from C:\zephyrproject\hybrid-mesh.
2. Feature-level code organization: each independent feature owns its source, public header, configuration, tests and docs where practical. Avoid monolithic main.c.
3. EVERY implementation task MUST create or update a Markdown file describing scope, files changed, commands executed, actual results, issues, rollback and next action. Update docs/PROGRESS.md. No exceptions.
4. Use Zephyr RTOS, west and CMake already installed; do not migrate frameworks without explicit approval.
5. Prefer automation: inspect, configure, build and test without requiring manual copying. Provide copy-ready prompts under docs/copilot for Copilot.
6. Never claim build, flash, BLE, LoRa or routing success without actual evidence. Do not flash, erase or change bootloaders without user approval.
7. nRF Connect on Android via BLE GATT is the PoC phone interface. No custom mobile app.
8. The seven phase acceptance tests are authoritative. BLE peer multi-hop and phone GATT must coexist, or record a feasible alternative.
9. Record each phase in Markdown; stop and ask for physical intervention only when necessary.
10. GitHub Copilot Agent is the primary coding agent. After EVERY Copilot task, independently inspect all changed code and configs, check project scope and modularity, review diagnostics and security/logic errors, reproduce the build/tests, verify documentation and acceptance evidence, and record a review verdict in Markdown. Do not advance automatically on Copilot's self-reported success alone; fix issues via another Copilot task and repeat the independent review until acceptable.
11. Before proceeding to a new phase or flashing, check actual artifacts, hardware prerequisites and the user's authorization. Keep unfinished acceptance checks open. The assistant may inspect remote project files and terminal results, but must not claim direct visibility of VS Code chat state unless a supported interface exposes it.
12. User-directed workflow (2026-10-19): Continue independent coding, builds, static review, documentation and nonphysical tests without stopping for routine approvals. Stop only when genuinely waiting for an explicit user physical action/authorization or a running Copilot task. When stopping, explicitly state the blocker using bold **WAITING FOR: ...**. Do not turn a missing detail into an unnecessary stop when more PC-only work can proceed. This rule never overrides Rule 6/11: no unauthorized flashing, resets, or LoRa RF emissions.

13. Copilot credit efficiency policy (user requirement, 2026-10-19): Keep Copilot tasks minimal, narrow, and implementation-only where possible. Avoid broad exploratory prompts, duplicate audits, repeated tests/builds inside Copilot, and unnecessary new Copilot sessions. Assistant should do independent checks, builds, review, and documentation directly where practical. Use brief targeted prompts that name exact files and acceptance criteria. Never compromise correctness, safety, independent review, or documentation requirements to save credits. 

