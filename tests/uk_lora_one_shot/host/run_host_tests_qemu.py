"""Run the one-shot policy tests with the already-installed Zephyr SDK ARM gcc + QEMU.

Used when no native host C compiler exists. Compiles the same sources as run_host_tests.py
for a Cortex-M3 with picolibc semihosting and executes them under qemu-system-arm
(mps2-an385, no network, no hardware, no RF). Usage:
    python tests/uk_lora_one_shot/host/run_host_tests_qemu.py
Override the SDK location with ZEPHYR_SDK_DIR.
"""
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / "build-task026-host"
SDK = pathlib.Path(os.environ.get("ZEPHYR_SDK_DIR", pathlib.Path.home() / "zephyr-sdk-1.0.1"))
GCC = SDK / "gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc.exe"
QEMU = SDK / "hosttools/qemu/qemu-system-arm.exe"
SRC = [
    "tests/uk_lora_one_shot/src/main.c",
    "tests/uk_lora_one_shot/src/store_tests.c",
    "tests/uk_lora_one_shot/src/reboot_tests.c",
    "tests/uk_lora_one_shot/host/runner.c",
    "src/features/uk_lora_one_shot/uk_lora_one_shot.c",
    "src/features/uk_lora_one_shot/uk_lora_one_shot_store.c",
    "src/features/uk_lora_regulatory/uk_lora_regulatory.c",
]
INC = ["tests/uk_lora_one_shot/host", "tests/uk_lora_one_shot/src",
       "src/features/uk_lora_one_shot", "src/features/uk_lora_regulatory"]


def run(name, allow):
    OUT.mkdir(exist_ok=True)
    elf = OUT / f"host_{name}.elf"
    cmd = [str(GCC), "-mcpu=cortex-m3", "-mthumb", "--specs=picolibc.specs", "--oslib=semihost",
           "--crt0=hosted", "-std=c11", "-Wall", "-Wextra", "-Werror",
           f"-DFRURT_UK_ONE_SHOT_TX_ALLOW={allow}", f"-DUK868_ALLOW_GATE_OPEN={allow}",
           f"-DFRURT_UK_ONE_SHOT_STORE_ACCEPTED={allow}",
           "-Wl,--defsym=__flash=0x0", "-Wl,--defsym=__flash_size=0x400000",
           "-Wl,--defsym=__ram=0x20000000", "-Wl,--defsym=__ram_size=0x400000",
           "-Wl,--defsym=__stack_size=0x4000", "-T", "picolibc.ld",
           "-o", str(elf)] + [f"-I{ROOT / i}" for i in INC] + [str(ROOT / s) for s in SRC]
    subprocess.run(cmd, check=True, cwd=ROOT)
    r = subprocess.run([str(QEMU), "-M", "mps2-an385", "-nographic", "-nic", "none",
                        "-semihosting-config", "enable=on,target=native",
                        "-kernel", str(elf)],
                       capture_output=True, text=True, timeout=120)
    print(f"=== {name} (ALLOW={allow}) ===")
    print(r.stdout)
    return r.returncode


if __name__ == "__main__":
    rc = run("locked", 0) | run("allow_fake_radio", 1)
    print("TASK026_HOST_TESTS_QEMU=" + ("PASS" if rc == 0 else "FAIL"))
    sys.exit(rc)
