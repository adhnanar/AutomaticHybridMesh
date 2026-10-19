"""Verify already built Frurt artifacts without rebuilding or touching hardware."""
import hashlib
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
EXPECT={
    "build-ci-default":"2575E7133E4B1795EF4D3A88C2B1115919F90E39A0E803C4FB10342E880D1F14",
    "build-ci-topology":"D5E9EF84540C50E28F79AA47D78591F5E2592B871E3658C61269D896640FEEB8",
}
BAD=[]
def check(label,ok,detail=""):
    print(f"{'PASS' if ok else 'FAIL'} {label} {detail}",flush=True)
    if not ok:BAD.append(label)
for name,h in EXPECT.items():
    p=ROOT/name/"zephyr"/"zephyr.uf2"
    actual=hashlib.sha256(p.read_bytes()).hexdigest().upper() if p.exists() else "MISSING"
    check(name+"_sha256",actual==h,actual)
    c=(ROOT/name/"zephyr"/".config").read_text()
    check(name+"_legacy_noecho_off","# CONFIG_BLE_GATT_DISCOVER_NO_ECHO is not set" in c)
for name in ("build-ci-sx1262-basic","build-ci-sx1262-rf_frontend"):
    p=ROOT/name/"zephyr"
    dts=(p/"zephyr.dts").read_text()
    cfg=(p/".config").read_text()
    offset=dts.index("i2c1: xiao_i2c: i2c@")
    node=dts[offset:dts.index("\n\t\t};",offset)]
    check(name+"_spi_lora", "sx1262: sx1262@0" in dts and "CONFIG_LORA_SX126X_NATIVE=y" in cfg)
    check(name+"_no_i2c_pin_conflict",'status = "disabled"' in node)
    check(name+"_native_driver", (p/"drivers/lora/native/sx126x/libdrivers__lora__native__sx126x.a").exists())
    if name.endswith("_rf_frontend"):
        check("rf_frontend_properties", all(t in dts for t in ("rx-enable-gpios","dio2-tx-enable","dio3-tcxo-voltage")))
for name in ("build-ci-lora-ztest","build-ci-hybrid-ztest"):
    p=ROOT/name/"zephyr"/"zephyr.elf"
    check(name+"_compile_link",p.exists())
for name in ("build-task018-noecho-independent","build-task018-gatt-policy-ztest-independent"):
    p=ROOT/name/"zephyr"/"zephyr.elf"
    check(name+"_compile_link",p.exists())
opt=(ROOT/"build-task018-noecho-independent"/"zephyr"/".config")
if opt.exists():
    check("optin_noecho_config","CONFIG_BLE_GATT_DISCOVER_NO_ECHO=y" in opt.read_text())
print(f"ARTIFACT_AUDIT_RESULT={'PASS' if not BAD else 'FAIL'} failed={BAD!r}",flush=True)
sys.exit(bool(BAD))
