# Task028 — Independent review (2026-10-19)

**Verdict: ACCEPT PC-only locked software build and static tests; NOT ACCEPTED for RF transmission.** Verified VS Code Copilot idle and correct repo `C:\zephyrproject\hybrid-mesh-poc`. No hardware flash, reset, power/USB action or RF TX.

## Independently executed
- With environment `DTC_OVERLAY_FILE=C:/zephyrproject/hybrid-mesh-poc/tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262_rf_frontend.overlay`, ran `C:\zephyrproject\.venv\Scripts\python.exe -m west build -b xiao_ble/nrf52840 -d build-task028-independent-locked tests/sx1262_uk_one_shot --pristine=always`: **204/204 PASS**. FLASH 56836 bytes, RAM 16888 bytes; produced locked UF2 (114176 bytes), SHA256 `426C4D79D715E6369370E1D2ECAD371AFE0F9B7C7E4E2C9AD90F24720AFAFB7D`.
- Build cache `FRURT_UK_ONE_SHOT_TX_ALLOW:BOOL=OFF`; static source+ELF/map checker `python tests/task024_static_tx_check.py --build build-task028-independent-locked`: **PASS**.
- `python -m unittest discover -s tests -p 'test_*.py' -q`: **45/45 PASS**.
- `python -m unittest discover -s tests/hybrid_router_sim -p 'test_*.py' -q`: **13/13 PASS**.
- Read `claim_locked` / `frurt_os_fire`: lock is acquired before volatile consumed/read + persistent store consume, then released before radio callback; missing lock denies, busy denies. Integrator must guarantee one genuine shared nonrecursive lock per store, all code paths, and no concurrent reinit/lock reassignment.

## Restrictions
- Concurrent C runtime tests are WRITTEN but NOT EXECUTED. Single-threaded interleaving design alone is not a runtime proof.
- No real nRF52840 lock/store integration, no atomic flash adapter, no antenna ERP/compliance authorization; never enable production TX.
- Task028 Copilot report incorrectly said west could not execute; independent Python module invocation succeeded. Default-locked build does not compile the TX-allowed implementation path.

## Next
Implement/test a narrowly scoped Zephyr lock-adapter foundation behind TX-disabled build gates and retain simulated host tests. Continue seeking executable C runtime tests before accepting RF use.
