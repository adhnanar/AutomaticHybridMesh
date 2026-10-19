"""Build and run the one-shot policy tests on the PC with `zig cc` (pip install ziglang).

Both configurations are exercised: locked (default) and unlocked-with-fake-radio.
No Zephyr, no radio driver, no RF. Usage: python tests/uk_lora_one_shot/host/run_host_tests.py
"""
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / "build-task024-host"
SRC = [
    ROOT / "tests/uk_lora_one_shot/src/main.c",
    ROOT / "tests/uk_lora_one_shot/src/store_tests.c",
    ROOT / "tests/uk_lora_one_shot/src/reboot_tests.c",
    ROOT / "tests/uk_lora_one_shot/src/concurrency_tests.c",
    ROOT / "tests/uk_lora_one_shot/host/runner.c",
    ROOT / "src/features/uk_lora_one_shot/uk_lora_one_shot.c",
    ROOT / "src/features/uk_lora_one_shot/uk_lora_one_shot_store.c",
    ROOT / "src/features/uk_lora_regulatory/uk_lora_regulatory.c",
]
INC = [ROOT / "tests/uk_lora_one_shot/host", ROOT / "tests/uk_lora_one_shot/src",
       ROOT / "src/features/uk_lora_one_shot",
       ROOT / "src/features/uk_lora_regulatory"]


def run(name, allow):
    OUT.mkdir(exist_ok=True)
    exe = OUT / f"host_{name}.exe"
    cc = os.environ.get("TASK024_HOST_CC", f"{sys.executable} -m ziglang cc").split()
    allow_defs = [f"-DFRURT_UK_ONE_SHOT_STORE_ACCEPTED={allow}"]  # host-only simulated flash
    cmd = cc + ["-std=c11", "-Wall", "-Wextra", "-Werror",
           f"-DFRURT_UK_ONE_SHOT_TX_ALLOW={allow}", f"-DUK868_ALLOW_GATE_OPEN={allow}"] + allow_defs + [
           "-o", str(exe)] + [f"-I{i}" for i in INC] + [str(s) for s in SRC]
    for d in ("zigcache", "zigcache-local"):
        (OUT / d).mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(OUT / "zigcache"),
               ZIG_LOCAL_CACHE_DIR=str(OUT / "zigcache-local"))
    subprocess.run(cmd, check=True, env=env)
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    print(f"=== {name} (ALLOW={allow}) ===")
    print(r.stdout)
    return r.returncode


if __name__ == "__main__":
    rc = run("locked", 0) | run("allow_fake_radio", 1)
    print("TASK024_HOST_TESTS=" + ("PASS" if rc == 0 else "FAIL"))
    sys.exit(rc)
