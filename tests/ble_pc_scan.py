import asyncio
from bleak import BleakScanner

async def main():
    found = await BleakScanner.discover(timeout=12, return_adv=True)
    print('TOTAL_FOUND', len(found))
    for address, (device, adv) in found.items():
        name = adv.local_name or device.name or ''
        if 'HYBRID' in name.upper():
            print('HYBRIDMESH', address, repr(name), 'RSSI', adv.rssi)

asyncio.run(main())
