"""Capture raw USB consoles around one Task011 BLE trial."""
import argparse
from datetime import datetime, timezone
import pathlib
import re
import subprocess
import sys
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
stop = threading.Event()
errors = []
error_lock = threading.Lock()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("relay", "baseline"), default="relay")
    parser.add_argument(
        "--control",
        choices=("isolation", "sensitivity", "relay"),
        help="C-side evidence check; defaults to isolation for baseline, relay otherwise",
    )
    parser.add_argument("--a-port", default="COM14")
    parser.add_argument("--b-port", default="COM15")
    parser.add_argument("--c-port", default="COM16")
    parser.add_argument("--scan-seconds", type=float, default=25)
    parser.add_argument("--run-id")
    return parser.parse_args()


def console(role, port, path):
    try:
        import serial

        device = serial.Serial(port=None, baudrate=115200, timeout=0.3)
        device.dtr = False
        device.rts = False
        device.port = port
        device.open()
        with device, path.open(
            "x", encoding="utf-8"
        ) as output:
            buffer = bytearray()
            while not stop.is_set():
                buffer.extend(device.read(256))
                while b"\n" in buffer:
                    index = buffer.index(10)
                    line = bytes(buffer[:index]).rstrip(b"\r")
                    del buffer[:index + 1]
                    record = (
                        f"{role}_CONSOLE utc="
                        f"{datetime.now(timezone.utc).isoformat(timespec='milliseconds')} "
                        f"monotonic_ns="
                        f"{time.monotonic_ns()} "
                        f"{line.decode('utf-8', 'replace')}"
                    )
                    output.write(record + "\n")
                    output.flush()
                    print(record, flush=True)
            if buffer:
                record = (
                    f"{role}_CONSOLE_PARTIAL utc="
                    f"{datetime.now(timezone.utc).isoformat(timespec='milliseconds')} "
                    f"monotonic_ns={time.monotonic_ns()} "
                    f"{buffer.decode('utf-8', 'replace')}"
                )
                output.write(record + "\n")
                output.flush()
                print(record, flush=True)
    except Exception as error:
        with error_lock:
            errors.append(f"{role} {port}: {error!r}")
        print(f"{role}_CONSOLE_ERROR port={port} error={error!r}", flush=True)


def main():
    args = parse_args()
    if args.scan_seconds <= 0:
        raise SystemExit("--scan-seconds must be positive")
    control = args.control or (
        "isolation" if args.mode == "baseline" else "relay"
    )
    if (args.mode == "baseline") != (control != "relay"):
        raise SystemExit("--control must match --mode")
    run_id = args.run_id or time.strftime("%Y%m%d-%H%M%S")
    if re.fullmatch(r"[A-Za-z0-9_-]+", run_id) is None:
        raise SystemExit("--run-id may contain only letters, digits, '_' and '-'")

    from task009_console_integrity import analyze_console

    roles = [("A", args.a_port)]
    if args.mode == "relay":
        roles.append(("B", args.b_port))
    roles.append(("C", args.c_port))
    pc_log = ROOT / "docs" / f"task011-{run_id}-pc.log"
    expected_logs = [
        ROOT / "docs" / f"task011-{run_id}-{role.lower()}-console.log"
        for role, _ in roles
    ]
    existing_logs = [path for path in [pc_log, *expected_logs] if path.exists()]
    if existing_logs:
        raise FileExistsError(
            "Refusing to overwrite existing evidence: "
            + ", ".join(str(path) for path in existing_logs)
        )

    threads = []
    logs = {}
    for role, port in roles:
        path = ROOT / "docs" / f"task011-{run_id}-{role.lower()}-console.log"
        logs[role] = path
        thread = threading.Thread(
            target=console, args=(role, port, path), daemon=True
        )
        thread.start()
        threads.append(thread)
    command = [
        sys.executable,
        str(ROOT / "tests" / "ble_pc_controlled_two_hop.py"),
        "--mode",
        args.mode,
        "--source",
        "E5:12:76:D2:56:EA",
        "--relay",
        "C8:51:08:6E:08:C0",
        "--destination",
        "E2:DB:6A:54:BB:71",
        "--scan-seconds",
        str(args.scan_seconds),
    ]
    result = None
    output = ""
    error_output = ""
    try:
        time.sleep(3)
        if errors:
            raise RuntimeError(
                "Refusing to start BLE trial because USB console capture failed: "
                + "; ".join(errors)
            )
        result = subprocess.run(
            command,
            cwd=ROOT,
            timeout=120,
            check=False,
            capture_output=True,
            text=True,
        )
        output = result.stdout
        error_output = result.stderr
    finally:
        time.sleep(2)
        stop.set()
        for thread in threads:
            thread.join(timeout=4)

    for role, path in logs.items():
        print(f"{role}_CONSOLE_LOG {path}", flush=True)
        if path.exists():
            lines = [
                line.split(" ", 2)[-1]
                for line in path.read_text(encoding="utf-8").splitlines()
            ]
            print(
                f"{role}_CONSOLE_INTEGRITY {analyze_console(lines)}", flush=True
            )
    with pc_log.open("x", encoding="utf-8") as output_file:
        output_file.write(output)
        output_file.write("\n--- STDERR ---\n")
        output_file.write(error_output)
    print(f"PC_LOG {pc_log}", flush=True)
    for line in output.splitlines():
        print(line, flush=True)
    for line in error_output.splitlines():
        print(f"PC_STDERR {line}", flush=True)
    if errors:
        raise RuntimeError("USB console capture failed: " + "; ".join(errors))
    if result is None:
        raise RuntimeError("BLE trial process did not return a result")
    print(f"TASK011_{args.mode.upper()}_EXIT {result.returncode}", flush=True)
    from task011_isolation_evidence import evaluate_trial

    c_lines = logs["C"].read_text(encoding="utf-8").splitlines()
    b_lines = (
        logs["B"].read_text(encoding="utf-8").splitlines()
        if "B" in logs
        else []
    )
    failures = evaluate_trial(
        args.mode,
        control,
        output.splitlines(),
        c_lines,
        b_lines,
        result.returncode,
    )
    if failures:
        for failure in failures:
            print(f"TASK011_EVIDENCE_FAIL {failure}", flush=True)
        raise SystemExit(1)
    print(f"TASK011_EVIDENCE_PASS control={control}", flush=True)


if __name__ == "__main__":
    main()
