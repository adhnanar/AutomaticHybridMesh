# Task036 — Kit3 phase-1 flash, 2026-10-19

User explicitly approved installing the independently reviewed Task036 BLE candidate on Kit3/COM16 only and confirmed entering its UF2 bootloader. Workspace: `C:\zephyrproject\hybrid-mesh-poc`.

Before copy: COM14 and COM15 enumerated; COM16 absent after user double-reset. F:\INFO_UF2.TXT identified `Board-ID: Seeed_XIAO_nRF52840_Sense`. Verified exact candidate SHA256 `36D83CB8901B885B42A44597B266B01DF90382E1C3CB9B7548BAD9643262D5AF`; verified Kit3 Task012 rollback binary SHA256 `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`.

Rechecked matching source hash, board ID, ports immediately before `Copy-Item` candidate -> `F:\zephyr.uf2`; **copy SUCCESS**, exit0. Post-copy COM14, COM15, and COM16 present, UF2 bootloader drive not present: **Kit3 firmware handoff and USB reenumeration PASS**. These checks do NOT prove that BLE advertising/GATT and Android inbox are functional; phase-1 Android nRF Connect smoke test pending. No flash/reset to Kit1/Kit2, no LoRa TX. Kit3 expected new name `Mesh-5A5E02`, service UUID 7B3E0001-6F2A-4C91-9A3D-2F5B6C7D8E00, inbox 0006 read-only.

**WAITING FOR USER**: Use nRF Connect Android to scan Kit3 and validate name, GATT inbox/read and DISCOVER no-echo. Do not flash Kit2 until phase-1 acceptance and separate authorization.
