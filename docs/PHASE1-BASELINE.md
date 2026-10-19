# Phase 1 hardware audit — baseline and blocker

Status: blocked at the physical-connection gate. The project scaffolding builds, but no actual XIAO nRF52840 board or radio kit is present in this environment, so hardware verification cannot continue without the user connecting the hardware.

## Scope

This audit covers the fresh project only and does not reuse any legacy code. It verifies:

- the build artifacts produced by the installed Zephyr toolchain,
- the system USB/serial enumeration for a real board,
- the Zephyr board and shield support in the installed tree,
- a three-kit checklist to be filled when hardware is physically present,
- the blocker condition that stops Phase 1 if the board is absent.

## Environment inspected

- Host machine: Windows 11 / PowerShell
- Project: `C:\zephyrproject\hybrid-mesh-poc`
- Zephyr tree: `C:\zephyrproject\zephyr`
- Zephyr SDK: `C:\Users\adhna\zephyr-sdk-1.0.1`
- Build tool: `west 1.5.0`
- Board target confirmed in the build: `xiao_ble/nrf52840`
- Build artifacts present: `build/zephyr/zephyr.uf2`, `build/zephyr/zephyr.elf`, `build/zephyr/zephyr.hex`

## Build artifact verification

Command executed:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

Result: exit code `0`; all `192/192` build steps completed and the output generated the firmware image. The build artifact timestamps confirm the files were produced on 2026-10-18 at 15:13 local time.

Command executed:

```powershell
$build = 'C:\zephyrproject\hybrid-mesh-poc\build\zephyr'; Get-ChildItem "$build\zephyr.uf2", "$build\zephyr.elf", "$build\zephyr.hex" | Select-Object FullName,LastWriteTime,Length | Format-Table -AutoSize
```

Evidence:

```text
FullName                                                 LastWriteTime          Length
--------                                                 -------------          ------
C:\zephyrproject\hybrid-mesh-poc\build\zephyr\zephyr.uf2 10/18/2026 3:13:24 PM   90112
C:\zephyrproject\hybrid-mesh-poc\build\zephyr\zephyr.elf 10/18/2026 3:13:24 PM 1305496
C:\zephyrproject\hybrid-mesh-poc\build\zephyr\zephyr.hex 10/18/2026 3:13:24 PM  126378
```

This confirms a successful clean build for the installed board target only. It does not prove that a physical board is connected to the host.

## Physical USB / serial inspection

Command executed:

```powershell
Get-CimInstance Win32_PnPEntity | Where-Object {$_.Name -match 'COM|USB|Nordic|XIAO|nRF|Serial|Bluetooth'} | Select-Object Name,PNPDeviceID,Manufacturer,Description | Format-Table -AutoSize
```

Command executed:

```powershell
$ErrorActionPreference='SilentlyContinue'; Write-Host '=== USB / Serial devices ==='; Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'USB|BTHENUM|VID_|PID_' -or $_.Name -match 'COM|USB|Serial|Bluetooth|XIAO|Nordic|nRF' } | Select-Object Name,InstanceId,Present | Format-Table -AutoSize; Write-Host '=== Serial ports ==='; [System.IO.Ports.SerialPort]::GetPortNames() | ForEach-Object { $_ }; Write-Host '=== Win32 serial devices ==='; Get-WmiObject Win32_SerialPort | Select-Object Name,DeviceID,PNPDeviceID | Format-Table -AutoSize
```

Actual evidence from the host:

```text
Name                                                            PNPDeviceID
----                                                            -----------
Standard Serial over Bluetooth link (COM6)                      BTHENUM\{00001101-0000-1000-8000-00805F9B34FB}_LOCAL...
```

and the serial-port list yielded only COM6 as a Bluetooth serial link; there were no entries corresponding to a Seeed XIAO BLE, nRF52840 USB CDC ACM device, or any USB serial device that can be treated as a physical board.

Conclusion: the environment has a Bluetooth virtual COM port, not an actual XIAO nRF52840 board connected over USB. No physical board evidence is present.

## Zephyr support research (non-destructive)

Command executed:

```powershell
Select-String -Path 'C:\zephyrproject\zephyr\boards' -Pattern 'xiao_ble|xiao.*nrf52840|Wio-SX1262|wio.*sx1262|semtech_sx1262mb2das|sx1262.*shield' -CaseSensitive:$false
```

Evidence inspected:

- `C:\zephyrproject\zephyr\boards\seeed\xiao_ble\doc\index.rst` confirms support for `xiao_ble` / `xiao_ble/nrf52840`.
- `C:\zephyrproject\zephyr\boards\shields\semtech_sx1262mb2das\doc\index.rst` confirms a generic `Semtech SX1262MB2DAS LoRa Shield` exists and is enabled with `--shield semtech_sx1262mb2das`.
- There is no installed Zephyr board definition named `Wio-SX1262` by itself; the relevant installed support is the generic Semtech SX1262 shield, not a verified board-level mapping for a specific Wio-SX1262 module.

This is important because the project must not invent pin mappings or assume a LoRa adapter is connected. The Zephyr tree supports the generic SX1262 shield, but not a physically identified board in this host.

## Three-kit hardware checklist (reproducible template)

Because the physical kit is absent, this checklist remains intentionally blank until a real board is connected.

| Kit | Board ID | USB port / COM | LoRa antenna attached? | Firmware image | Boot evidence |
|---|---|---|---|---|---|
| Kit 1 | TBD — actual Seeed XIAO BLE or board revision | TBD | TBD | `zephyr.uf2` or debug image | TBD, requires USB mass storage or serial console |
| Kit 2 | TBD — actual Seeed XIAO BLE or board revision | TBD | TBD | `zephyr.uf2` or debug image | TBD, requires USB mass storage or serial console |
| Kit 3 | TBD — actual Seeed XIAO BLE or board revision | TBD | TBD | `zephyr.uf2` or debug image | TBD, requires USB mass storage or serial console |

Each kit must be verified by physical inspection before any board-specific configuration is added to the application.

## Blocker and stop condition

The Phase 1 gate is blocked because no actual XIAO nRF52840 board is connected to the host. The environment shows only a Bluetooth virtual COM port and no USB CDC ACM / board enumeration for a Seeed board.

This means:

- no safe board identity can be validated,
- no board-specific boot verification can occur,
- no flashing or radio validation is permitted,
- no LoRa pin mapping or shield mapping may be assumed.

## Rollback and project state

The only project modifications made during this audit were:

- initialization of a fresh Git repository in `C:\zephyrproject\hybrid-mesh-poc`,
- creation of `.gitignore` to ignore build output,
- documentation updates in `docs/PHASE1-BASELINE.md` and `docs/PROGRESS.md`.

No firmware was flashed, no bootloader was changed, and no legacy project content was modified.

## Next action required from the user

Connect one or more real Seeed XIAO nRF52840 boards or a known compatible USB board and verify that the host enumerates a board-specific USB CDC ACM device or UF2 mass storage device. If the user connects a board, we will continue Phase 1 by checking:

1. board ID and USB port,
2. bootloader mode evidence,
3. LoRa radio or shield presence,
4. firmware image and serial boot logs,
5. readiness for Phase 2 BLE work.

Do not continue to Phase 2 until the hardware is physically present and verified.


## Kit 1 approved UF2 flash — 2026-10-18
- User explicitly approved flashing Kit 1, then manually entered bootloader by double-reset.
- UF2 removable drive D: label `XIAO-SENSE`; `INFO_UF2.TXT` identified `Model: Seeed XIAO nRF52840`, `Board-ID: Seeed_XIAO_nRF52840_Sense`, UF2 Bootloader 0.6.1 and S140 7.3.0.
- Verified source `build/zephyr/zephyr.uf2` and exact board ID before copying it to `D:\zephyr.uf2` with PowerShell `Copy-Item`. Copy completed; drive disconnected and USB CDC COM14 returned. No full-chip erase or bootloader replacement.
- Serial read at COM14/115200 after reboot returned `*** Booting Zephyr OS build v4.5.0-rc2-76-ga915b2f12675 ***\r\nHyb` (64 bytes). This confirms Zephyr boot and the beginning of the new project status output. A subsequent serial read was empty because the one-time boot message had already passed; full line remains to be captured on a fresh reset.
- Outcome: Phase 1 basic firmware build, UF2 flashing, Zephyr boot VERIFIED on Kit 1. LoRa, BLE, antennas, Kits 2/3 and full serial message still unverified.
- Next: ask Copilot to plan BLE GATT Phase 2 while completing remaining Phase 1 physical/radio checks. Do not declare Phase 1 fully complete.

## Kit 2 initial USB/serial inspection — 2026-10-18
- User connected Kit 2. Windows detected USB Serial Device `COM10`, USB VID:PID `2886:8044`, serial `224BC825E1DA130C`; Windows device status OK. Kit 1 COM14 not present in this enumeration; physical connection/power state needs clarification.
- Non-destructive 115200 baud read on COM10 returned `Starting LoRa Mesh Phase 4...`, `Internal flash storage ready!`, `Reserved message IDs: 3001 to 4000`, `Device ID: 2`, `LoRa initialized!`, and sample send instructions.
- This is existing/legacy firmware. Startup text is not proof of over-the-air LoRa messaging, and is not part of the fresh PoC. No flash, reset, or source import performed.
- Next: determine whether Kit 1 is still connected, then prepare fresh project firmware for Kit 2 only after separate explicit approval and bootloader identity verification. Phase 3 BLE mesh requires design review of coexistence with Android GATT.
