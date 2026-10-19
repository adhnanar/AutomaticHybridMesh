import asyncio
from bleak import BleakScanner, BleakClient
SERVICE='7b3e0001-6f2a-4c91-9a3d-2f5b6c7d8e00'
TX='7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00'
DISC='7b3e0004-6f2a-4c91-9a3d-2f5b6c7d8e00'
async def main():
    seen={}
    for i in range(3):
        found=await BleakScanner.discover(timeout=9,return_adv=True)
        for address,(dev,adv) in found.items():
            name=adv.local_name or dev.name or ''
            if 'HYBRID' in name.upper():
                seen[address]=dev
                print('SCAN',i+1,address,repr(name),'RSSI',adv.rssi,flush=True)
    print('UNIQUE_HYBRIDMESH',len(seen),flush=True)
    for address,dev in seen.items():
        try:
            print('CONNECTING',address,flush=True)
            async with BleakClient(dev,timeout=12) as client:
                print('CONNECTED',address,client.is_connected,flush=True)
                print('HAS_SERVICE',SERVICE in [str(s.uuid) for s in client.services],flush=True)
                for uuid in (TX,DISC):
                    try: print('READ',address,uuid,repr(bytes(await client.read_gatt_char(uuid))),flush=True)
                    except Exception as e: print('READ_ERROR',address,uuid,repr(e),flush=True)
            print('DISCONNECTED',address,flush=True)
        except Exception as e: print('CONNECT_ERROR',address,repr(e),flush=True)
asyncio.run(main())
