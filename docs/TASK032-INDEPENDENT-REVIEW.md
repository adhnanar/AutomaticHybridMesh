# Task032 independent firmware-build review — 2026-10-19

**Verdict: PC-side firmware compilation ACCEPTED for default and Android opt-in; no Android/hardware acceptance.** Correct workspace `C:\zephyrproject\hybrid-mesh-poc`; Copilot idle. Copilot's own environment blocked `python.exe`, but independent execution using venv Python `-m west` succeeded. All builds are test artifacts, NOT flashed.

## Independently executed
- `C:\zephyrproject\.venv\Scripts\python.exe -m west build -b xiao_ble/nrf52840 -d build-task032-independent-default . --pristine=always` with process-local `ZEPHYR_BASE=C:\zephyrproject\zephyr`: **PASS 274/274**, FLASH 134072 bytes, RAM 30472 bytes, UF2 SHA256 `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14` — EXACT match to stable Task012 default UF2.
- Opt-in `tests/task032_android_optin.conf` with `CONFIG_BLE_GATT_ANDROID_DISCOVERY_NAME=y` and `CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y`, passed as process-local `EXTRA_CONF_FILE=tests/task032_android_optin.conf`; pristine `python -m west build -b xiao_ble/nrf52840 -d build-task032-independent-android .`: **PASS 274/274**, FLASH 134576 bytes, RAM 30472 bytes, UF2 SHA256 `9380BC59C3ED86AA63CF70C4A587C42DC6FFFF36DE149DEDEF55AE550C33A7B1`. Build `.config` confirms both options and `CONFIG_BT_DEVICE_NAME_DYNAMIC=y`.
- `python -m unittest discover -s tests -p 'test_*.py' -q`: **48/48 PASS**. `python tests/task024_static_tx_check.py`: **PASS**.
- Original `build-task012-topology/zephyr/zephyr.uf2` hash unchanged `D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8`; Kit1 Task022 receive-only UF2 unchanged `28C2852D6C714D428D0269ECAF48BC3E8ACEEAFFF4D672E69BAEDECD738C4644`.

## Restrictions
- No compiled C formatter tests were executed and no Android nRF Connect behavior verified. Device-name payload fills exactly the 31-byte legacy scan response limit; adding more AD elements requires redesign.
- Kit1 COM14 still runs RX-only LoRa and is not BLE A; Kit2/3 retain topology BLE. Do not flash any board without explicit user authorization; keep LoRa TX blocked.
- Opt-in image builds but is not accepted for physical use without a board-specific rollback/acceptance plan. Future priority is UX message history/discovery evidence, PC-only until authorization.
