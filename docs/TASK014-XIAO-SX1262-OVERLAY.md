# TASK014 — XIAO nRF52840 + Wio-SX1262 opt-in hardware profile

Date: 2026-10-19. **Status: profile and no-transmit smoke target authored;
clean Zephyr builds not verified in this sandbox; hardware/RF acceptance open.**

## Scope and evidence

The target is the Seeed XIAO nRF52840 (`xiao_ble/nrf52840`) plus Wio-SX1262
kit (SKU 102010710), not the ESP32-S3 variant. This is a standalone,
opt-in test application; it is not selected by the root build and is not
linked into the BLE/GATT application.

The installed build artifacts already present in the workspace show the
default board's `xiao_spi` is `spi2` and `i2c1` is enabled. Its generated
`spi2_default` pinctrl contains PSEL values `0x400002d`, `0x500002f`, and
`0x600002e`, matching SCK P1.13, MOSI P1.15, and MISO P1.14 respectively.
The user-provided kit pin map and required hardware reference identify CS D4
(P0.04), DIO1 D1 (P0.03), BUSY D3 (P0.29), and RESET D2 (P0.28). Those GPIO
assignments are used in the opt-in overlay.
Because the stock board maps `i2c1` SDA/SCL to D4/D5, the overlay disables
`i2c1` to prevent pin ownership conflicts while the profile is selected.

The Zephyr v4.5.0-rc1 upstream SX1262 binding requires reset, busy, and DIO1
GPIOs and inherits the SPI device binding. The same tag's LoRa Kconfig offers
the native backend and `LORA_SX126X_NATIVE` driver; that driver is explicitly
experimental. See:

- [SX1262 binding](https://github.com/zephyrproject-rtos/zephyr/blob/v4.5.0-rc1/dts/bindings/lora/semtech%2Csx1262.yaml)
- [SX126x shared binding](https://github.com/zephyrproject-rtos/zephyr/blob/v4.5.0-rc1/dts/bindings/lora/semtech%2Csx126x-base.yaml)
- [LoRa backend Kconfig](https://github.com/zephyrproject-rtos/zephyr/blob/v4.5.0-rc1/drivers/lora/Kconfig)
- [Native SX126x Kconfig](https://github.com/zephyrproject-rtos/zephyr/blob/v4.5.0-rc1/drivers/lora/native/sx126x/Kconfig)
- [Seeed kit product reference](https://wiki.seeedstudio.com/xiao_nrf52840%26_wio_SX1262_kit_for_meshtastic/)
- [Seeed-published Wio-SX1262-for-XIAO schematic PDF](https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf)
- [Seeed forum discussion of missing internal module RF-switch details](https://forum.seeedstudio.com/t/wio-sx1262-refsw-dio2-and-no-actual-schematic-for-the-module/294808)

The product reference identifies the kit and its radio band but does not
establish this project's lawful region or channel plan. The Seeed schematic
PDF endpoint returned raw PDF data rather than a readable/rendered schematic
in this environment; a Seeed forum discussion reports that the public
document does not expose the module's internal RF-switch implementation.
Consequently, the available evidence is not sufficient to make safe
RXEN/DIO2/TCXO configuration assumptions.
The current upstream driver supports `rx-enable-gpios`,
`dio2-tx-enable`, and `dio3-tcxo-voltage`; driver support alone does not
prove the module wiring. Accordingly these properties are intentionally
omitted.

### Open hardware TODOs

- **TODO — RXEN:** D5/P0.05 is reported as RXEN in the supplied pin map, but
  its module-side connection and asserted polarity have not been verified
  from a sufficiently detailed manufacturer circuit.
- **TODO — DIO2/RF switch:** Do not set `dio2-tx-enable` until Seeed confirms
  the Wio-SX1262 module RF-switch implementation and DIO2 connection.
- **TODO — DIO3/TCXO:** Do not select a TCXO voltage or startup delay until
  the actual module oscillator supply and timing are confirmed. Do not infer
  1.8 V from generic SX1262 designs or unverified third-party board variants.
- **Board population — operator confirmed:** All three XIAO boards have
  Wio-SX1262 modules and antennas attached. This is a user confirmation,
  not physical inspection or radio validation.
- **RF region — operator identified:** United Kingdom. The specific lawful
  frequency/channel plan, bandwidth/data rate, transmit timing/power, and
  airtime constraints for this deployment are not yet selected or validated.
  None is configured here.

## Files changed

- Added `tests/sx1262_hardware_profile/CMakeLists.txt`, an independent
  Zephyr app build target.
- Added `tests/sx1262_hardware_profile/prj.conf` selecting Zephyr's native
  experimental SX126x driver only for this app.
- Added `tests/sx1262_hardware_profile/xiao_nrf52840_wio_sx1262.overlay`
  with SPI2, CS, RESET, BUSY, DIO1 and the conflicting I2C disable.
- Added `tests/sx1262_hardware_profile/src/main.c`, a boot-time device
  readiness smoke app. The driver initializes during Zephyr startup; this
  app reports readiness and never calls LoRa configure/receive/send APIs.
- Updated `docs/PROGRESS.md`.
- Added the copy-ready follow-up prompt
  `docs/copilot/014-xiao-sx1262-hardware-profile.prompt.md`.

Root `CMakeLists.txt`, `prj.conf`, BLE sources, BLE topology policy, and
existing firmware configurations were not edited. No automatic overlay
selection was added. No legacy project source or configuration was used.

## Build and static verification commands

The intended clean commands, from `C:\zephyrproject`, are:

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task014-default C:\zephyrproject\hybrid-mesh-poc --pristine=always
```

This compares a clean default app build without the radio profile.

```powershell
& .\.venv\Scripts\west.exe build -b xiao_ble/nrf52840 -d C:\zephyrproject\hybrid-mesh-poc\build-task014-sx1262 C:\zephyrproject\hybrid-mesh-poc\tests\sx1262_hardware_profile --pristine=always -- -DDTC_OVERLAY_FILE=C:\zephyrproject\hybrid-mesh-poc\tests\sx1262_hardware_profile\xiao_nrf52840_wio_sx1262.overlay
```

This explicitly opts in to the test app and non-default overlay. After a
successful build, inspect
`build-task014-sx1262\zephyr\zephyr.dts` for `sx1262@0`, `spi2`, the GPIOs,
and disabled `i2c1`; inspect `build-task014-sx1262\zephyr\.config` and
`zephyr\drivers\lora` build output for the native driver selection/object.
Compare the default build's `zephyr.dts` and `.config` to ensure it has no
enabled SX1262 device or selected LoRa driver. Build topology separately
using the existing topology Kconfig selections if that image is required
for a binary comparison.

### Actual commands and results in this session

- Read the repository's `AGENTS.md`, `docs/TASK013-INDEPENDENT-REVIEW.md`,
  `docs/TASK014-XIAO-SX1262-HARDWARE-REFERENCE.md`, and
  `docs/PROGRESS.md`: **completed**.
- Inspected the existing Task012/013 generated DTS and Kconfig artifacts
  inside the workspace: **completed**. They confirm stock `xiao_spi` is
  SPI2 with PSEL values for SCK P1.13, MOSI P1.15, and MISO P1.14, stock
  I2C1 is enabled on D4/D5, and the existing default build has `CONFIG_LORA`
  unset.
- Ran an inline Python static check from PowerShell over the new overlay,
  app/config and existing generated board DTS: **11/11 PASS**. It checked
  I2C1 disable, SPI host and its existing PSEL values, CS, RESET, BUSY, DIO1
  pin declarations, native driver Kconfig, absence of unresolved
  RXEN/DIO2/TCXO settings, and absence of LoRa TX/RX/config API calls from
  the smoke app. This is source-level checking, not DTS preprocessing or
  compilation.
- Queried official Zephyr v4.5.0-rc1 binding, driver and Kconfig source, and
  Seeed's published references: **completed**. Results support the declared
  binding/Kconfig facts; the schematic fetch was not renderable and
  module-side RF-switch details remain unresolved.
- Attempted to read the installed checkout at
  `C:\zephyrproject\zephyr`; PowerShell returned **Access is denied** due to
  the sandbox. As required by the sandbox policy, no retry through another
  command/tool was attempted for that path.
- In response to the operator confirmation request, the user confirmed
  modules and antennas on all three boards and specified the United Kingdom
  as the intended operating region. They report sandbox policy can be
  updated, but access was not changed in this execution; no denied-path
  retry or build was attempted.
- Clean default and overlay `west build` commands above: **NOT RUN**. The
  installed Zephyr checkout/west environment is outside the permitted
  workspace, and local access to that checkout was denied. Therefore no
  build success, generated overlay DTS, Kconfig driver linkage, or image
  comparison is claimed.
- No flash, erase, reset, power, board access, BLE activity, or RF activity
  was performed.

## Limitations and verdict

The overlay profile is authored but not build-verified. Static source review
cannot substitute for the required clean default and opt-in builds, generated
DTS inspection, driver object linkage, and pin-conflict checks. Keep
Task014's build acceptance **OPEN** until those commands run in an
environment permitted to access the installed Zephyr tree.

The overlay configures only enough known wiring for a compile/init smoke
target. Because the radio driver can reset and initialize the transceiver at
boot, this app must not be flashed/run until the operator explicitly
authorizes board action. The application makes no RF transmit call and sets
no RF region or radio parameters.
It is not the three-node hardware proof and does not establish RF readiness.

## Rollback

Remove only the added `tests/sx1262_hardware_profile/` directory and
`docs/copilot/014-xiao-sx1262-hardware-profile.prompt.md`, then revert this
report's entry in `docs/PROGRESS.md`. The root BLE app/configuration was not
changed. If the named `build-task014-*` output directories are created in a
later verification, they are disposable build outputs; do not remove any
other build directory as part of rollback.

## Next action

First rerun the two clean builds and generated-file/linkage inspections in a
workspace that can read the installed Zephyr tree. The operator has confirmed
modules/antennas on all three boards and identified the UK; keep the exact UK
frequency/channel and airtime/power rules unselected until they are
authoritatively established. Separately obtain manufacturer circuit details
for RXEN polarity, RF switch/DIO2 routing, and TCXO voltage/startup delay. Do
not enable RF switching or configure/transmit LoRa until those facts and
region constraints are established and the operator explicitly authorizes
board/RF action.

## Independent review addendum — 2026-10-19

Later independent clean builds on Abi passed: SX1262 profile 201/201, default BLE 273/273, Python tests 31/31. Generated devicetree and driver linkage checked. See docs/TASK014-INDEPENDENT-REVIEW.md for evidence and hashes. User confirmed three LoRa modules and antennas are connected. Radio runtime, RF switch/TCXO and regional settings remain unverified.
