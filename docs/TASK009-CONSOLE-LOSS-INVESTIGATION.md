# Task009 console loss investigation and loss-tolerant diagnostics

Date: 2026-10-18. No hardware was flashed, reset or opened.

## Evidence reviewed
- `docs/TASK009-DIAGNOSTIC-LIVE-ACCEPTANCE.md`, `docs/task009-both-consoles-acceptance.log`.
- Built config `build-task009-diagnostics-independent/zephyr/.config` and Zephyr 4.5 `usbd_cdc_acm.c`.

## Findings (facts vs. inference)
Facts:
1. Console is the USB CDC ACM (`zephyr,console = &board_cdc_acm_uart`), printk only; `CONFIG_LOG` is not set, so Zephyr logging is not involved. `prj.conf` sets no USB/console options.
2. CDC ACM `hw-flow-control` is unset. In `cdc_acm_poll_out`, if the TX buffer (`tx_fifo.current`) has no room, the byte is **silently discarded** (not blocked) when flow control is off. `tx_fifo.current` is only replaced when an IN transfer completes, so output is lost whenever the host is not draining the endpoint. TX FIFO is 1024 B (64 B buffers); the first transfer is delayed by `USBD_CDC_ACM_TX_DELAY_MS=100`; CDC TX work runs on the system workqueue.
3. In the capture, B and C each show a full boot banner followed by exactly `Hyb` (banner + CRLF + 3 chars = 64 B, one USB packet), then nothing intact. C's later lines (`HybBLE GATT connected...`) are a splice of that fragment with a later line, i.e. bytes between were lost, not reordered.
4. C's `-11` (-EAGAIN) beacon update failure occurred while the PC held a GATT connection to C. `CONFIG_BT_MAX_CONN=1` and relay data is sent through the same connectable advertising set (`BT_LE_ADV_CONN_FAST_1`), so an active connection stops that advertising and updates fail until reconnect/restart. Timing in the log matches (connect 321827500, -11 at 321829656, disconnect 321832421) but this is consistent-with, not proven.

Inference (unproven): the dropped console bytes are consistent with CDC TX buffer starvation, but no run isolated it. The absence of B/C RX records therefore does not show whether B scanned, accepted or relayed. Radio acceptance stays OPEN.

## Corrective changes (host-tested, built, NOT flashed)
- Firmware (topology builds only): atomic `decoded_frames` / `decoded_text_frames` counters in `ble_mesh_rx_diagnostics`, incremented in the scan callback (no printing there); a 5 s `HEARTBEAT` line in `ble_mesh_relay.c` carrying a sequence number `n`, cumulative counters, uptime and an `end` terminator. Gaps in `n` quantify console loss; cumulative counters give receive evidence even if individual RX lines are lost. Default (non-topology) firmware is unchanged. Retry/advertising/beacon timing is unchanged.
- `tests/task009_console_integrity.py` + `tests/test_task009_console_integrity.py`: strict heartbeat parser, missing-index/damaged-line/counter-monotonicity analysis. `tests/task009_capture_both_consoles.py` prints this summary per console.
- `tests/ble_mesh_packet/src/main.c`: ztest extended for the new counters (compiled only).
- Not changed (deliberately): no hw-flow-control (blocking `poll_out` on the system workqueue could stall advertising/RX work), no routing/RF changes, no `prj.conf` change.

## Commands and results
- `python -m unittest discover -s tests`: 15/15 PASS; `python -m compileall tests`: PASS.
- Clean `west build` (new dirs, deployed artifact untouched): default `build-task009-hb-default` 271/271 PASS, FLASH 133472 B; topology (`TEST_TOPOLOGY=y`, `B_ID_0C13=y`) `build-task009-hb-topology` 271/271 PASS, FLASH 135264 B, RAM 30472 B, UF2 270848 B, SHA256 `19718FE4F08BDF2665AED4726867E4727982438EED71237167FA4CC4609BC189`; ztest app `build-task009-hb-ztest` PASS (compile/link only, not run).

## Rollback
Delete the new test files and revert the heartbeat/counter edits in `ble_mesh_relay.c` and `ble_mesh_rx_diagnostics.[ch]`; the deployed UF2 (SHA256 `04170F01...D142`) is unchanged on all kits.

## Next action (needs user approval)
Approval to flash `build-task009-hb-topology/zephyr/zephyr.uf2` to Kit2/B and Kit3/C (A unchanged or also, user's choice). Then: capture B/C consoles ~60 s without BLE test, check `*_CONSOLE_INTEGRITY`; then rerun acceptance with the PC test avoiding a GATT connection to C during the relay window if the -EAGAIN hypothesis is to be tested.
