# Task025 independent software review — 2026-10-19

Status: locked-build verified; complete acceptance pending C runtime and concurrent-use review. No board operations performed.

Independent checks on Abi:
- 45 Python tests passed.
- 13 hybrid-router Python tests passed.
- Static locked-image checker passed.
- Clean Zephyr xiao_ble/nrf52840 build 204/204 passed, using the experimental SX1262 overlay. FLASH 56836 B; RAM 16888 B; UF2 114176 bytes.
- Locked UF2 SHA256: 30C8A5356C11D238C9A9C607561A31B6F367221DA536A62467A8E178126E8D63. Not flashed.

Findings: durable store logic is written and the locked firmware compiles. The C power-interruption tests have not been executed. A possible two-caller race exists because the RAM consumed flag and flash state check/write/readback are not protected by an exclusive lock. The real hardware flash adapter and antenna compliance evidence are also outstanding. The default production build remains disabled from transmitting.

## Concurrent-caller review
`frurt_os_fire()` checks and updates `c->consumed` without an atomic operation. `frurt_os_store_consume()` reads ARMED, writes a fixed consumed word, then reads it back. In a two-caller interleaving, both callers can observe ARMED before either write; both may then verify the same consumed marker and each invoke the radio callback. The current tests only model sequential reboots. This is an unproven safety property, not a demonstrated hardware failure. Require either exclusive serialization enforced by the implementation or an explicit single-thread contract that cannot be violated, with an interleaving regression test.

## Next review gates
Execute C tests on a supported host toolchain or working emulator; verify real persistent flash write semantics and reserved page; verify antenna and regulatory documentation. No new flash or radio test authorized.

## Independent C ztest compilation and execution attempt
Clean `qemu_cortex_m3` locked test target compiled and linked **144/144 PASS**. Both `store_tests.c` and `reboot_tests.c` were compiled. `west build -t run` failed with Windows exit code `0xC0000135` from the SDK QEMU executable, indicating a missing runtime dependency. **No C test case was executed**, so power-interruption test results are still unverified. No software installation or device operation performed.
