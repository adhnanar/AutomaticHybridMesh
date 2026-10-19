# Task009 diagnostic firmware deployment and first live acceptance

Date: 2026-10-18

## Deployment
User explicitly approved Task009 diagnostic UF2 installation to all 3 boards. Exact verified source UF2: `build-task009-diagnostics-independent/zephyr/zephyr.uf2`, 270336 bytes, SHA256 `04170F01F5BE435219D12E69702DCEC60CA3909CC604AF1ACD0E7CAC87B7D142`. After user double-reset each kit, verified UF2 bootloader Board-ID Seeed_XIAO_nRF52840_Sense and copied the same UF2. Each kit reenumerated with its original COM/USB identity: Kit1/A COM14 1D5EA4A525ED34EB; Kit2/B COM15 224BC825E1DA130C; Kit3/C COM16 B13E025E5A1407EE. No other flash performed.

## Live test
Ran `tests/task009_capture_both_consoles.py`, which opens B COM15 and C COM16 serial, then invokes `tests/ble_pc_controlled_two_hop.py` with A E5:12:76:D2:56:EA, B C8:51:08:6E:08:C0, C E2:DB:6A:54:BB:71, 25-second scan. Evidence: `docs/task009-both-consoles-acceptance.log`, `docs/task009-b-diagnostic-console.log`, `docs/task009-c-diagnostic-console.log`.

Result **FAIL**. New unique message `HOP0098C47AF54` written and echoed by A. PC observed A advertisement source eb34ed25a5a4 sequence 49 TTL3 payload identical. No matching B TTL2 observation; C GATT read remained `Ready`. PC runner exited 1. B serial produced only Zephyr boot line and trailing partial `Hyb`; C serial produced boot line, partial `Hyb` and GATT connected/disconnected messages plus beacon advertising update -11. Neither console produced an intact correlatable Task009 RX_ACCEPT / relay advertising state line. These absences do not prove specific radio-path failure because the USB console output was incomplete. No C delivery/notification passed; radio acceptance OPEN.

## Follow-up and caution
Investigate USB serial console transport and boot diagnostic truncation alongside relay scheduling/advertising. Verify logging without assuming malformed lines are valid packet evidence. Capture independently by B/C console without simultaneous BLE test if needed, ensure USB DTR/console settings and source logs, then controlled repetition. Do not treat API success as on-air proof, do not alter routing/RF timing based on this data alone. Any new changed firmware flash needs renewed explicit user approval.
