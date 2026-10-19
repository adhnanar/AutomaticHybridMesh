import asyncio
from bleak import BleakScanner,BleakClient
ADDRESS='E5:12:76:D2:56:EA'
TX='7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00'
async def main():
    for i in range(1,7):
        try:
            found=await BleakScanner.discover(timeout=6,return_adv=True)
            device=found.get(ADDRESS)
            print('ROUND',i,'VISIBLE',bool(device),flush=True)
            if not device: continue
            async with BleakClient(device[0],timeout=12) as client:
                data=bytes(await client.read_gatt_char(TX))
                print('ROUND',i,'CONNECTED',client.is_connected,'TX',repr(data),flush=True)
            print('ROUND',i,'DISCONNECTED',flush=True)
        except Exception as e: print('ROUND',i,'ERROR',repr(e),flush=True)
        await asyncio.sleep(1)
asyncio.run(main())
