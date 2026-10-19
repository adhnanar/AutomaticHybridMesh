"""Task021 PC-only UART diagnostic log capture (AFTER separately approved flash).

Never flashes, resets, reboots, changes GPIO, starts LoRa TX or writes serial data.
Chooses board via stable USB serial number, not volatile COM port label.
"""
import argparse
import pathlib
import sys
import time
from datetime import datetime, timezone

import serial
from serial.tools.list_ports import comports

SERIALS={
    "A":"1D5EA4A525ED34EB",
    "B":"224BC825E1DA130C",
    "C":"B13E025E5A1407EE",
}
MARKER="FRURT_TASK021_NO_TX_DIAGNOSTIC"
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--kit",choices=tuple(SERIALS),default="A")
    ap.add_argument("--seconds",type=int,default=45)
    ap.add_argument("--log",default=None)
    args=ap.parse_args()
    if not 1<=args.seconds<=300:
        ap.error("--seconds must be 1..300")
    ports=[p for p in comports() if (p.serial_number or "").upper()==SERIALS[args.kit]]
    if len(ports)!=1:
        print("NO_UNIQUE_USB_DEVICE",args.kit,[(p.device,p.serial_number) for p in ports]);return 2
    port=ports[0].device
    log=pathlib.Path(args.log or f"docs/task021-live-{args.kit.lower()}.log")
    log.parent.mkdir(parents=True,exist_ok=True)
    print(f"PASS USB_IDENTITY kit={args.kit} serial={SERIALS[args.kit]} port={port}",flush=True)
    received=[]
    start=time.monotonic()
    with serial.Serial(port,baudrate=115200,timeout=0.5) as ser,log.open("w",encoding="utf-8") as out:
        # Opening serial port may assert DTR on some hosts; do not manipulate reset.
        while time.monotonic()-start<args.seconds:
            raw=ser.readline()
            if not raw:continue
            value=raw.decode("utf-8",errors="replace").strip()
            line=datetime.now(timezone.utc).isoformat()+" "+value
            print(line,flush=True)
            out.write(line+"\n")
            received.append(value)
    marker=any(MARKER in x for x in received)
    ready=any("TASK021_SX1262_DEVICE_READY=1" in x for x in received)
    failed=any("TASK021_SX1262_DEVICE_READY=0" in x for x in received)
    if not marker:
        print("RESULT=INCONCLUSIVE_NO_TASK021_BOOT_MARKER; may have connected after startup");return 1
    if failed:
        print("RESULT=FAIL_RADIO_DRIVER_INIT");return 2
    if ready:
        print("RESULT=PASS_DRIVER_INIT_ONLY; NOT SPI SILICON PROOF OR LORA RF TEST");return 0
    print("RESULT=INCONCLUSIVE_NO_DRIVER_READY_STATUS");return 1
if __name__=="__main__":sys.exit(main())
