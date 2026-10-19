"""Task022 read-only serial capture from known Kit1 USB CDC COM14."""
import time
import serial
from pathlib import Path
from serial.tools.list_ports import comports
ports=[p for p in comports() if (p.serial_number or "").upper()=="1D5EA4A525ED34EB"]
if len(ports)!=1:
    raise SystemExit("Kit1 USB identity absent or ambiguous")
port=ports[0].device
path=Path("docs/task022-kit1-rx-live.log")
print("TASK022_KIT1_PORT="+port,flush=True)
with serial.Serial(port,115200,timeout=0.5) as s,path.open("w",encoding="utf-8") as f:
    start=time.monotonic()
    while time.monotonic()-start<32:
        line=s.readline().decode(errors="replace").strip()
        if line:
            print(line,flush=True)
            f.write(line+"\n")
            f.flush()
print("TASK022_CAPTURE_COMPLETE",flush=True)
