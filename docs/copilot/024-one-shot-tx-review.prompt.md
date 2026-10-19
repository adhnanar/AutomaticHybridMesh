Independently review TASK024 in C:\zephyrproject\hybrid-mesh-poc (never the parent). Do not flash, reset, or emit RF; do not git add/commit.
1. Read docs/TASK024-ONE-SHOT-TX-SAFETY-REVIEW.md, src/features/uk_lora_one_shot, tests/sx1262_uk_one_shot, tests/uk_lora_one_shot.
2. Run the ztests (west build -b qemu_cortex_m3 tests/uk_lora_one_shot, with and without -DONE_SHOT_TEST_ALLOW=ON, then -t run) on a machine whose QEMU works, or run tests/uk_lora_one_shot/host/run_host_tests.py with TASK024_HOST_CC set.
3. Rebuild the locked app pristine for xiao_ble/nrf52840 with the rf_frontend overlay; compare SHA256 with the doc; run python tests/task024_static_tx_check.py --build <dir>.
4. Confirm no allow-mode artefact exists and lora_send occurs only in tests/sx1262_uk_one_shot/src/radio_tx.c.
5. Append a verdict to the doc and docs/PROGRESS.md.
