# Task011 first physical sensitivity control — inconclusive

Date: 2026-10-19. User stated Kit2/B powered off. Verified only Kit1/A COM14 serial 1D5EA4A525ED34EB and Kit3/C COM16 serial B13E025E5A1407EE enumerated. No phone connection required.

## Run
`tests/task009_capture_both_consoles.py --mode baseline --control sensitivity --a-port COM14 --c-port COM16 --scan-seconds 20 --run-id task011-sensitivity-live-01`. Evidence `docs/task011-sensitivity-invocation-01.log` and unique per-run raw logs (see task011 run directory). Unique message `HOP0114261CAA4`; A GATT write/echo PASS; A on-air source eb34ed25a5a4 sequence 312 TTL3 observed. C precheck and final GATT value both `Ready`; PC B-off baseline accepted (`B_OFF_PC_BASELINE=PASS RF_ISOLATION=NOT_ESTABLISHED_CHECK_C_USB_EVIDENCE`), subprocess exit0.

## Strict evidence verdict: INCONCLUSIVE
Offline evaluator emitted `TASK011_EVIDENCE_FAIL C heartbeat indices are missing`, `C console has damaged topology or heartbeat records`, `C console has incomplete receive, GATT, or heartbeat records`, and `C did not log the direct A/TTL3 sensitivity frame`. C console contained truncated/merged `RX_REJECT role=C` fragments but no complete same-sequence record. Thus there is no valid C-side sensitivity proof, and no RF-isolation claim. Need improve CDC diagnostic robustness or observational strategy while retaining strict criteria. No reset, power toggling or firmware flash performed.

## Next
Keep Kit2 off and A/C connected. Investigate why Zephyr CDC console breaks heartbeat and receive lines under BLE radio load; prepare fix or sound alternative, host tests/build, then seek new approval before changed firmware flash. Do not count this run as one of five accepted sensitivity trials.
