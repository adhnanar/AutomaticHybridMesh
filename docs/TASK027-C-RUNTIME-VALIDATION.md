# TASK027 — C runtime validation (2026-10-19)

**Status: BLOCKED; neither C suite was compiled or executed.** This was a PC-only attempt using existing local software. No package was installed or downloaded, and no flash, reset, USB/device operation, or RF transmission was performed.

## Scope

Continue Task026 by checking whether the installed Zephyr SDK QEMU can run locally, without changing the global PATH or installing software. If usable, run the existing QEMU test runner for both the default locked policy and the fake-radio ALLOW policy. Keep the production TX path disabled.

## Files changed

- Added this validation record.
- Appended the Task027 result to [PROGRESS.md](PROGRESS.md).
- No source, configuration, runner, or test files were changed.

## Checks and actual results

- Confirmed the working directory was `C:\zephyrproject\hybrid-mesh-poc`.
- Read [AGENTS.md](../AGENTS.md), [TASK025-INDEPENDENT-REVIEW.md](TASK025-INDEPENDENT-REVIEW.md), and [TASK026-HOST-C-TEST-REVIEW.md](TASK026-HOST-C-TEST-REVIEW.md).
- Direct invocation of the installed `arm-zephyr-eabi-gcc.exe --version` succeeded and reported Zephyr SDK GCC 14.3.0.
- The process PATH did not contain the Zephyr SDK QEMU or GNU ARM tool directories. A process-local-only PATH prefix containing those existing directories was tried; this did not make the QEMU invocation usable. No machine/user PATH was modified.
- An initial read-only SDK inventory found QEMU-bundled DLLs `libgcrypt-20.dll`, `libgpg-error-0.dll`, `libiconv-2.dll`, `libintl-8.dll`, `libjpeg-8.dll`, and `libpixman-1-0.dll`. This inventory did not identify which DLL (if any) is missing.
- QEMU execution attempts in this session did not produce a usable version output. Subsequent inspection of `C:\Users\adhna\zephyr-sdk-1.0.1\hosttools\qemu\qemu-system-arm.exe` and its parent directory was explicitly denied by the sandbox. The prior Task026 report records QEMU exit `0xC0000135` (`-1073741515`, missing runtime dependency); the exact missing DLL remains unverified here.
- Consequently, `tests\uk_lora_one_shot\host\run_host_tests_qemu.py` was **not run**. Neither `locked (ALLOW=0)` nor `allow_fake_radio (ALLOW=1)` was compiled or executed; no C pass is claimed. The runner and test sources remain unchanged.
- No rollback was needed; no build output was created by this attempt.

## Commands executed

- PowerShell inspection of the current process PATH, SDK directory and DLL inventory.
- Direct `arm-zephyr-eabi-gcc.exe --version`.
- Direct QEMU `--version` attempt, then a second `--version` attempt with a temporary process-only PATH containing the SDK QEMU and ARM compiler directories.
- Read-only `Test-Path`, `Get-Item`, directory listing, and attempted SDK `objdump -p` inspection for QEMU imports. The sandbox denied access to the QEMU executable path during follow-up inspection; no workaround or alternate-language retry was attempted.
- Read-only inspection of the existing QEMU runner, prior review records, and project progress.

## Issues and rollback

- **Blocker:** the sandbox denied access to the existing SDK QEMU executable/directory during follow-up inspection and execution. The reported missing-runtime status does not disclose the exact missing DLL, and the SDK DLL inventory alone cannot prove the dependency set is complete.
- No implementation or configuration change needs rollback. If this record itself must be removed, delete only `docs/TASK027-C-RUNTIME-VALIDATION.md` and remove only the Task027 entry appended to `docs/PROGRESS.md`.

## Next action

Stop for independent review as requested. For a later runtime-validation attempt, first make the existing QEMU path `C:\Users\adhna\zephyr-sdk-1.0.1\hosttools\qemu` readable/executable in the sandbox (including read access needed to inspect installed dependencies); then rerun the existing QEMU runner and record separate locked and fake-radio ALLOW results. Do not download/install dependencies or enable real radio transmission.
