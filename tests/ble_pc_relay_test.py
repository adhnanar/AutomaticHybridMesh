import asyncio
from bleak import BleakClient, BleakScanner
RX='7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00'
TX='7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00'
TARGETS=['C8:51:08:6E:08:C0','E5:12:76:D2:56:EA','E2:DB:6A:54:BB:71']
async def main():
    devices={}
    for n in range(2):
        for address,(dev,adv) in (await BleakScanner.discover(timeout=7,return_adv=True)).items():
            if address.upper() in TARGETS: devices[address.upper()]=dev
    print('FOUND',list(devices),flush=True)
    for addr in TARGETS:
        if addr not in devices: continue
        for attempt in range(1,4):
            try:
                async with BleakClient(devices[addr],timeout=10) as c:
                    print('RECONNECT',addr,attempt,'PASS',flush=True)
                await asyncio.sleep(0.6)
            except Exception as e: print('RECONNECT',addr,attempt,'FAIL',repr(e),flush=True)
    src=next((x for x in TARGETS if x in devices),None)
    if not src: return
    message=b'FRURT07'
    try:
        async with BleakClient(devices[src],timeout=12) as c:
            await c.write_gatt_char(RX,message,response=True)
            print('SENT',src,repr(message),flush=True)
            print('SOURCE_TX',repr(bytes(await c.read_gatt_char(TX))),flush=True)
    except Exception as e: print('SEND_FAIL',repr(e),flush=True); return
    await asyncio.sleep(3)
    for addr in TARGETS:
        if addr==src or addr not in devices: continue
        try:
            async with BleakClient(devices[addr],timeout=12) as c:
                data=bytes(await c.read_gatt_char(TX))
                print('REMOTE_TX',addr,repr(data),'MATCH',data==message,flush=True)
        except Exception as e: print('REMOTE_FAIL',addr,repr(e),flush=True)
asyncio.run(main())
