# Phase 3 connection recovery task 007

## Scope

This fix targets the Phase 3 BLE reconnect issue reported for the Android nRF Connect phone flow that can connect to the HYBRIDMESH peripheral, write `DISCOVER` and `HELLO123`, receive peer discovery, and connect to a second device, but then fails to reconnect to the original device at address `C8:51:08:6E:08:C0` after the phone Bluetooth stack is restarted. The task explicitly does not claim a proven root cause without hardware evidence.

The implementation follows Zephyr 4.5 Bluetooth guidance for legacy connectable advertising, passive scanning, advertising-data updates, and connection lifecycle handling. It adds explicit diagnostics and a conservative advertising restart path without calling illegal host APIs from callback context.

## Files changed

- `src/features/ble_gatt/ble_gatt.c` — added connection/disconnection callbacks, active-connection tracking, advertising restart scheduling, and guarded `bt_le_adv_update_data` calls while a phone connection is active.
- `src/features/ble_mesh_relay/ble_mesh_relay.c` — added explicit scan-start logging for passive peer discovery and clearer start-up diagnostics.
- `docs/PROGRESS.md` — recorded the recovery patch and build evidence.

## Implementation notes

- Kept the existing custom GATT service UUIDs, RX/TX/discovery behavior, and peer-relay packet logic intact.
- Avoided race-prone host API usage in the callback path by deferring the re-advertise action to a delayable work item.
- Guarded mesh advertisement updates (`bt_le_adv_update_data`) when a phone connection is still active; the radio stack may reject update calls while connected, and the code now reports that state instead of trying to continue in an illegal sequence.
- Added connection/disconnection logs and advertising restart logs to make the lifecycle observable without making unproven claims about the underlying Android/phone issue.
- Preserved the existing modular feature layout and did not flash any board or alter bootloaders.

## Commands executed

From `C:\zephyrproject`:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

## Actual results

The build completed successfully with exit code `0`.

Evidence captured from the command output:

- `Board: xiao_ble, qualifiers: nrf52840`
- `-- Build files have been written to: C:/zephyrproject/hybrid-mesh-poc/build`
- `-- west build: building application`
- final status: `completed with exit code 0`

This is compile evidence only. It does not prove BLE peer reconnect behavior or radio coexistence on physical hardware.

## Remaining radio tests and open issues

- Live nRF Connect reconnect validation on actual XIAO boards is still pending.
- Passive peer scanning and connectable advertising coexistence on the physical nRF52840 still requires user-approved hardware verification.
- Host-side `native_sim`/Twister execution remains unavailable in this Windows environment because the required native toolchain is not installed.
- No claim is made that this patch fixes the original reconnect problem in the field; it only hardens the lifecycle and makes the failure mode observable and recoverable under the documented Zephyr constraints.

## Rollback

If needed, revert the changes in:

- `src/features/ble_gatt/ble_gatt.c`
- `src/features/ble_mesh_relay/ble_mesh_relay.c`

This is a non-destructive runtime logic patch and does not require bootloader or board reset actions.

## Next action

With explicit user approval, the next step is a controlled, non-destructive hardware verification: flash the patched firmware to the test XIAO, confirm the Android phone can reconnect after Bluetooth restart, and observe the new connection/disconnection logs. If the radio still fails to reconnect, the logs will indicate whether the failure is in the advertisement lifecycle or elsewhere in the phone/controller interaction.
