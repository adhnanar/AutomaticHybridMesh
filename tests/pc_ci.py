"""Frurt PC-only regression driver. No flash, reset, or LoRa transmission.

Usage: C:\\zephyrproject\\.venv\\Scripts\\python.exe tests\\pc_ci.py [--full]
Quick: Python tests, syntax and baseline artifact checks.
Full: pristine Zephyr compile/link for BLE, LoRa mock, hybrid router and SX1262 overlays.
C ztest runtime is NOT executed by this Windows runner.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import pathlib
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
WEST = ROOT.parent / ".venv" / "Scripts" / "west.exe"
BOARD = "xiao_ble/nrf52840"
EXPECT = {
    "default": "2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14",
    "topology": "D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8",
}
STAMP = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
LOG_DIR = ROOT / "docs" / "ci" / STAMP
RESULTS: list[tuple[str, str, str, str]] = []

def record(name: str, status: str, detail: str, log: str = "") -> None:
    print(f"{status}: {name}: {detail}", flush=True)
    RESULTS.append((name,status,detail,log))

def command(name: str, argv: list[str], *, timeout: int = 700,
            cwd: pathlib.Path = ROOT) -> bool:
    path = LOG_DIR / f"{name}.log"
    print("RUN "+name+": "+" ".join(argv),flush=True)
    started=time.monotonic()
    try:
        with path.open("w",encoding="utf-8") as f:
            result=subprocess.run(argv,cwd=str(cwd),stdout=f,
                                  stderr=subprocess.STDOUT,timeout=timeout)
        ok=result.returncode == 0
        record(name,"PASS" if ok else "FAIL",
               f"exit={result.returncode}, elapsed={time.monotonic()-started:.1f}s",
               path.relative_to(ROOT).as_posix())
        return ok
    except (OSError,subprocess.TimeoutExpired) as exc:
        record(name,"FAIL",repr(exc),path.relative_to(ROOT).as_posix())
        return False

def build(name: str, app: pathlib.Path, *,
          extras: tuple[str,...]=(), overlay: pathlib.Path|None=None,
          compare_hash: str|None=None) -> None:
    dest=ROOT/f"build-ci-{name}"
    argv=[str(WEST),"build","-b",BOARD,"-d",str(dest),str(app),
          "--pristine=always"]
    props=list(extras)
    if overlay:
        props.append("-DDTC_OVERLAY_FILE="+overlay.as_posix())
    if props:argv+=["--",*props]
    if not command(name,argv,cwd=ROOT.parent):return
    if compare_hash:
        uf2=dest/"zephyr"/"zephyr.uf2"
        digest=hashlib.sha256(uf2.read_bytes()).hexdigest().upper()
        record(name+"_hash","PASS" if digest==EXPECT[compare_hash] else "FAIL",
               f"sha256={digest}, expected Task012 {compare_hash}")
    if overlay:
        dts=(dest/"zephyr"/"zephyr.dts").read_text(encoding="utf-8")
        conf=(dest/"zephyr"/".config").read_text(encoding="utf-8")
        i2c_disabled=('i2c1: xiao_i2c: i2c@' in dts and
                      'status = "disabled"' in dts[dts.find('i2c1: xiao_i2c: i2c@'):][:1800])
        native=('CONFIG_LORA_SX126X_NATIVE=y' in conf and
                "sx1262@0" in dts and i2c_disabled)
        record(name+"_dts","PASS" if native else "FAIL",
               "SX1262 native driver and disabled I2C1 generated DTS")
        if "rf_frontend" in overlay.name:
            frontend=('rx-enable-gpios' in dts and
                      'dio2-tx-enable' in dts and
                      'dio3-tcxo-voltage' in dts)
            record(name+"_rf_frontend_props",
                   "PASS" if frontend else "FAIL","RF frontend DT properties compiled")

def run() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--full",action="store_true",help="Run all clean Zephyr builds")
    args=parser.parse_args()
    LOG_DIR.mkdir(parents=True,exist_ok=True)
    command("python-existing",
        [sys.executable,"-m","unittest","discover","-s","tests","-p","test_*.py","-q"])
    command("python-hybrid-model",
        [sys.executable,"-m","unittest","discover","-s",
         "tests/hybrid_router_sim","-p","test_*.py","-q"])
    scripts=["tests/task012_live_relay.py","tests/task012_live_isolation.py",
             "tests/task012_live_sensitivity.py","tests/task016_pc_gatt_coexistence.py",
             "tests/task016_pc_discovery_notifications.py","tests/task016_review_logs.py"]
    command("python-compile",
            [sys.executable,"-m","py_compile",*scripts])
    if args.full:
        build("default",ROOT,compare_hash="default")
        build("topology",ROOT,extras=("-DCONFIG_BLE_MESH_TEST_TOPOLOGY=y",
              "-DCONFIG_BLE_MESH_TEST_B_ID_0C13=y"),compare_hash="topology")
        build("lora-ztest",ROOT/"tests"/"lora_transport")
        build("hybrid-ztest",ROOT/"tests"/"hybrid_router")
        p=ROOT/"tests"/"sx1262_hardware_profile"
        build("sx1262-basic",p,overlay=p/"xiao_nrf52840_wio_sx1262.overlay")
        rf=p/"xiao_nrf52840_wio_sx1262_rf_frontend.overlay"
        if rf.is_file():
            build("sx1262-rf_frontend",p,overlay=rf)
    report=ROOT/"docs"/"PC-CI-LATEST.md"
    rows=["# PC-only Frurt regression run",f"Run (UTC): {STAMP}",
          f"Mode: {'full' if args.full else 'quick'}",
          "No flash, reset, physical power or LoRa RF actions performed.",
          "All Zephyr C ztests are **compile/link-only**; their cases were not run.",
          "","| Stage | Verdict | Evidence |","|---|---|---|"]
    for name,status,detail,log in RESULTS:
        value=detail.replace("|","/")
        if log:value+=f"; log: {log}"
        rows.append(f"| {name} | {status} | {value} |")
    report.write_text("\n".join(rows)+"\n",encoding="utf-8")
    print("REPORT "+str(report),flush=True)
    return 0 if all(r[1]=="PASS" for r in RESULTS) else 1

if __name__=="__main__":
    sys.exit(run())
