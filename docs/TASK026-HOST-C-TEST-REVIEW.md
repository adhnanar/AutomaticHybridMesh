# TASK026 — Host C regression test execution review (2026-10-19)

**Verdict: BLOCKED. Task025 C regression tests were NOT compiled or executed. No C pass is claimed.** No install, download, PATH change, flash, reset, RF TX or USB/power change was performed. Production TX stays hard-disabled; no TX-enabled image was created.

## Scope
Find an already-installed compiler/runtime to run `tests/uk_lora_one_shot/host/run_host_tests.py` equivalents (locked and fake-radio ALLOW), and review reboot/interrupted-write invariants and hidden TX routes.

## Files changed
- Added [tests/uk_lora_one_shot/host/run_host_tests_qemu.py](../tests/uk_lora_one_shot/host/run_host_tests_qemu.py): fallback runner (Zephyr SDK ARM gcc + picolibc semihosting + `qemu-system-arm -M mps2-an385`, same sources/defines as `run_host_tests.py`). **Never successfully run**.
- This file; [docs/PROGRESS.md](PROGRESS.md) appended.

## Toolchain inspection (read-only)
- PATH: no `gcc`, `clang`, `cl`, `tcc`, `cc`; `zig`/`ziglang` not installed in Python envs; no WSL distro (`wsl` reports Windows Subsystem for Linux not installed); no Docker; no MSVC/MinGW/MSYS2/LLVM directories under `C:\` / Program Files; PlatformIO/Arduino/Cargo dirs contain no C compiler.
- Only C compiler found: Zephyr SDK `C:\Users\adhna\zephyr-sdk-1.0.1\gnu\arm-zephyr-eabi\bin\arm-zephyr-eabi-gcc.exe` (14.3.0, **ARM cross-compiler**, cannot make a native Windows exe). Runtime candidate: SDK `hosttools\qemu\qemu-system-arm.exe`.
- Runner attempt: `python tests\uk_lora_one_shot\host\run_host_tests_qemu.py` → `PermissionError: [WinError 5] Access is denied` when launching the SDK gcc. Direct invocation of the gcc and of `C:\zephyrproject\.venv\Scripts\python.exe` from this session also failed ("not recognized") although `Test-Path` is True and files exist. This is a **sandbox execution restriction** on paths outside the repo (also noted in Task024/025). Not retried via other means.
- Even if gcc ran, prior reviews (Task024/025) recorded `qemu-system-arm` exits `0xC0000135` (missing DLL), so the QEMU path is doubly blocked.

## Minimal user approval needed (pick one; nothing installed by me)
1. Allow the sandbox to execute `C:\Users\adhna\zephyr-sdk-1.0.1\gnu\arm-zephyr-eabi\bin\*` (read+exec) AND fix the missing QEMU DLL (identify with a dependency walker; likely SDK-bundled runtime DLL dir not on PATH — a process-local PATH addition in the runner would suffice), then run `python tests\uk_lora_one_shot\host\run_host_tests_qemu.py`; **or**
2. Approve `pip install ziglang` into a venv (≈80 MB), then `python tests\uk_lora_one_shot\host\run_host_tests.py` (existing runner, native exe); **or**
3. Approve installing any native MinGW-w64/LLVM compiler and set `TASK024_HOST_CC`.

## Source review (static, by reading; not a runtime proof)
Reviewed `uk_lora_one_shot_store.c`, `uk_lora_one_shot.c`, CMake files.
- **Reboot**: state is read from durable store each `frurt_os_fire`; only `ARMED` (valid magic/version/CRC/non-erased token and consume word still `0xFFFFFFFF`) permits TX. BLANK, CORRUPT, IO error, CONSUMED all deny. Volatile `consumed` latch is only an extra layer.
- **Order**: consume word programmed and read back/re-validated **before** `uk868_guarded_send`; any write/verify failure returns `ERR_STORE`/no radio callback (fail closed). Volatile latch set before the store commit, so even failed attempts cannot retry in the same boot.
- **Interrupted writes**: a torn arm record fails CRC → CORRUPT (deny). A torn consume word: any bit cleared makes word ≠ `0xFFFFFFFF` → CONSUMED (deny); an untouched word means no consume occurred and no TX happened because TX follows verified readback. Tear-then-reboot therefore cannot yield a second transmission. Residual: crash after consume commit but before/during radio means the shot is lost (intended, fail-safe).
- **Limitations** (not mitigated): relies on flash word-program bits only clearing (1→0) and on a real adapter providing atomic/monotonic behaviour; no adapter exists; concurrent `frurt_os_fire` callers (non-atomic latch/state check) remain unprotected — needs a lock if multithreaded. `frurt_os_store_state` treats only first 16 bytes for blank detection; fine.
- **Hidden TX route**: `grep` of `src`/`tests` shows `frurt_os_fire`/radio code only in the allow-gated paths; `radio_tx.c` is compiled only in the allow branch; [tests/sx1262_uk_one_shot/CMakeLists.txt](../tests/sx1262_uk_one_shot/CMakeLists.txt) executes `message(FATAL_ERROR ...)` immediately when `FRURT_UK_ONE_SHOT_TX_ALLOW` is ON (the following `WARNING`/sources are unreachable). C preprocessor `#error`s require both locks equal and `FRURT_UK_ONE_SHOT_STORE_ACCEPTED` for ALLOW. `STORE_ACCEPTED=1` is set only in host test CMake `tests/uk_lora_one_shot/CMakeLists.txt` (fake radio, no LoRa driver). Finding: that Zephyr unit-test CMake can enable ALLOW with a fake radio — acceptable (no driver linked) but must never be flashed to a board with SX1262 hardware.

## Commands executed
`Get-Command gcc,clang,cl,tcc,cc`; bounded directory listings/recursive filename scans; `wsl -l -v`; `python tests\uk_lora_one_shot\host\run_host_tests_qemu.py` (access denied). Prior independent evidence from Task025 review (not repeated): locked build 204/204, static check, 45+13 Python PASS.

## Issues / rollback / next
- Issue: C runtime verification still open (blocker above). Rollback: delete `run_host_tests_qemu.py` and `build-task026-host/` (build dir left empty; untracked).
- Next: user chooses approval option 1–3, then run the matching runner and record both locked and ALLOW outputs. Keep flashing/TX forbidden.
