"""PC-only GATT DISCOVER command / discovery notification test.
No firmware flash, hardware reset, or LoRa RF transmission.
"""
import asyncio
import sys
from datetime import datetime, timezone

from bleak import BleakClient, BleakScanner
from ble_pc_air_message import RX_UUID, TX_UUID

C="E2:DB:6A:54:BB:71"
DISC="7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00"
def line(x):print(datetime.now(timezone.utc).isoformat(),x,flush=True)

async def run():
    found=await BleakScanner.discover(timeout=8,return_adv=True)
    devices={x.upper():dev for x,(dev,ad) in found.items()}
    if C not in devices:
        line("DISCOVERY_RESULT=INCONCLUSIVE_C_UNDISCOVERED");return 1
    events=[]
    text=[]
    def on_discovery(_s, data):
        value=bytes(data)
        events.append(value)
        line("DISC_NOTIFY="+repr(value))
    def on_text(_s,data):
        value=bytes(data)
        text.append(value)
        line("TEXT_NOTIFY="+repr(value))
    async with BleakClient(devices[C],timeout=15) as client:
        await client.start_notify(DISC,on_discovery)
        await client.start_notify(TX_UUID,on_text)
        prior=bytes(await client.read_gatt_char(DISC))
        line("DISC_BEFORE="+repr(prior))
        await client.write_gatt_char(RX_UUID,b"DISCOVER",response=True)
        line("DISCOVER_WRITE_OK")
        await asyncio.sleep(2)
        after=bytes(await client.read_gatt_char(DISC))
        line("DISC_AFTER="+repr(after))
        line("DISC_EVENT_COUNT="+str(len(events)))
        line("TEXT_EVENT_COUNT="+str(len(text)))
        recognized=[v for v in events if v.startswith(b"PEER:") or v==b"BLE: no peers"]
        failures=[]
        if not recognized:failures.append("NO_DISCOVERY_NOTIFICATION")
        if recognized and after!=recognized[-1]:failures.append("GATT_READBACK_NOT_LAST_DISC_EVENT")
        line("VALIDATION_FAILURES="+repr(failures))
        line("DISCOVERY_RESULT="+("PASS" if not failures else "FAIL"))
        return 0 if not failures else 2

if __name__=="__main__":
    try:sys.exit(asyncio.run(run()))
    except Exception as e:
        line("DISCOVERY_RESULT=EXCEPTION "+repr(e))
        sys.exit(1)
