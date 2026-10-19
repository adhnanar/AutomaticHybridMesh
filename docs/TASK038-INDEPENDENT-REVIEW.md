# Task038 independent review — 2026-10-19

**Verdict: ACCEPT PC-only opt-in C routing foundation, NOT a working on-device hybrid transport.** Copilot idle, verified workspace `C:\zephyrproject\hybrid-mesh-poc`. BLE-first C route selection uses recent RSSI validation (10 s/-85 dBm assumed threshold), LoRa send callback only when separately authorized; production LoRa TX remains prohibited. The router is not wired to application radios or phone GATT. C Zephyr ztest runtime tests written but unexecuted; simulator queue expiry and C retry count differ, and candidate policy is aggregate-peer rather than destination-aware.

Independent executions using `C:\zephyrproject\.venv\Scripts\python.exe`:
- `-m unittest discover -s tests\hybrid_policy_sim -p test_*.py -q`: **16/16 PASS**.
- `-m unittest discover -s tests\hybrid_router_sim -p test_*.py -q`: **14/14 PASS**.
- `-m unittest discover -s tests -p test_*.py -q`: **55/55 PASS**.
- `tests/task024_static_tx_check.py`: **PASS**.
- Pristine root default `python -m west build -b xiao_ble/nrf52840 -d build-task038-independent-default . --pristine=always`, process-local `ZEPHYR_BASE=C:\zephyrproject\zephyr`: **274/274 PASS**, UF2 SHA256 `2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14`, exact stable Task012 default.
- Opt-in `CONFIG_HYBRID_ROUTER_POLICY=y` in process-local `EXTRA_CONF_FILE=tests/task038_hybrid_policy_optin.conf`, pristine `-d build-task038-independent-policy`: **275/275 PASS**, `.config` confirms option, UF2 SHA256 **same as Task012 default** because router still unused/garbage-collected by linker. Thus compile tested but not runtime-integrated.

**No board flashing, reset, USB, power, BLE/LoRa RF or LoRa TX operations.** Kit1 remains LoRa RX, Kits2/3 retain proven Task036 BLE. Next: use PC-only isolated test harness or C ztests to validate actual C decision logic, then design explicit RX/GATT integration behind default-off safety gates; never enable real LoRa TX without separate authorization.
