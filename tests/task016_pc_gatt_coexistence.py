"""PC-only Task016 BLE mesh/GATT coexistence check: no firmware flashing or LoRa use.
Subscribes to C GATT while source A message traverses physical BLE B relay.
"""
import asyncio
import secrets
import sys
from datetime import datetime,timezone

from bleak import BleakClient, BleakScanner
from ble_pc_air_message import RX_UUID, TX_UUID, COMPANY_ID, decode_text_frame
from task012_evidence import decode, delta

A="E5:12:76:D2:56:EA"
B="C8:51:08:6E:08:C0"
C="E2:DB:6A:54:BB:71"
DISCOVERY_UUID="7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00"
EVIDENCE_UUID="7b3e0005-6f2a-4c91-9a3d-2f5b6c7d8e00"

def line(s): print(datetime.now(timezone.utc).isoformat(),s,flush=True)
async def main():
    message=("N"+secrets.token_hex(6).upper()).encode("ascii")
    devices_found = await BleakScanner.discover(timeout=9,return_adv=True)
    devices={a.upper():dev for a,(dev,_adv) in devices_found.items()}
    line("DISCOVERED="+str([x for x in (A,B,C) if x in devices]))
    if any(x not in devices for x in (A,B,C)):
        line("RESULT=INCONCLUSIVE_MISSING_BLE_DEVICE");return 1
    observed=[]
    notify_text=[]
    notify_discovery=[]
    def adv(device,ad):
        frame=decode_text_frame(COMPANY_ID,ad.manufacturer_data.get(COMPANY_ID,b""),message)
        if frame and frame.source_id.hex()=="eb34ed25a5a4":
            x=(device.address.upper(),frame.sequence,frame.ttl)
            observed.append(x)
            if len(observed)==1 or x not in observed[:-1]:
                line("ON_AIR "+str(x))
    def on_text(_handle, data):
        b=bytes(data);notify_text.append(b);line("C_TX_NOTIFY="+repr(b))
    def on_discovery(_handle,data):
        b=bytes(data);notify_discovery.append(b);line("C_DISCOVERY_NOTIFY="+repr(b))
    async with BleakClient(devices[C],timeout=18) as client_c:
        before=decode(bytes(await client_c.read_gatt_char(EVIDENCE_UUID)))
        prevalue=bytes(await client_c.read_gatt_char(TX_UUID))
        line("C_PREVALUE="+repr(prevalue))
        line("C_EVIDENCE_BEFORE="+repr(before))
        await client_c.start_notify(TX_UUID,on_text)
        await client_c.start_notify(DISCOVERY_UUID,on_discovery)
        line("C_TX_AND_DISCOVERY_SUBSCRIBED")
        async with BleakScanner(adv):
            await asyncio.sleep(1)
            async with BleakClient(devices[A],timeout=15) as client_a:
                await client_a.write_gatt_char(RX_UUID,message,response=True)
                echo=bytes(await client_a.read_gatt_char(TX_UUID))
                line("A_WRITE="+repr(message)+" A_ECHO="+repr(echo))
                if echo != message:line("RESULT=FAIL_A_ECHO");return 2
            await asyncio.sleep(23)
        await asyncio.sleep(2)
        cvalue=bytes(await client_c.read_gatt_char(TX_UUID))
        after=decode(bytes(await client_c.read_gatt_char(EVIDENCE_UUID)))
        line("C_FINAL_TX="+repr(cvalue))
        line("C_EVIDENCE_AFTER="+repr(after))
        line("C_EVIDENCE_DELTA="+repr(delta(before,after)))
        line("C_TEXT_NOTIFY_COUNT="+str(len(notify_text)))
        line("C_DISCOVERY_NOTIFY_COUNT="+str(len(notify_discovery)))
        line("A_B_ON_AIR="+str(sorted(set(observed))))
        failures=[]
        if not any(x[0]==B and x[2]==2 for x in observed):
            if after.last_ttl==2 and after.last_source_tail=="25a5a4" and any(x[0]==A and x[1]==after.last_seq and x[2]==3 for x in observed) and after.accept-before.accept==1:
                line("B_ADVERTISEMENT_MISSED_BY_PC; C_RECEIVER_EVIDENCE_MATCHED_A_SOURCE_SEQ_TTL2")
            else:failures.append("NO_INDEPENDENT_RELAY_EVIDENCE")
        if cvalue != message:failures.append("NO_C_GATT_DELIVERY")
        if message not in notify_text:failures.append("NO_C_GATT_NOTIFICATION")
        if after.accept <= before.accept:failures.append("NO_C_ACCEPT_COUNTER")
        if after.reject!=before.reject:line("C_DIRECT_REJECT_DELTA="+str(after.reject-before.reject)+"; intermittent direct path observed; NOT a notification failure")
        line("VALIDATION_FAILURES="+repr(failures))
        line("RESULT="+("PASS_C_GATT_NOTIFY_WITH_BLE_RELAY" if not failures else "FAIL_OR_INCONCLUSIVE"))
        return 0 if not failures else 2

if __name__=="__main__":
    try:
        code=asyncio.run(main())
    except Exception as error:
        line("RESULT=EXCEPTION "+repr(error))
        code=1
    sys.exit(code)


