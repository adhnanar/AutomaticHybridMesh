import asyncio
from bleak import BleakScanner

async def main():
    print('PC_BLE_SCAN_START', flush=True)
    devices = await BleakScanner.discover(timeout=12.0, return_adv=True)
    for address, (dev, adv) in devices.items():
        name = dev.name or adv.local_name or ''
        if 'Mesh' in name or 'MESH' in name or 'Hybrid' in name or any('7b3e0001' in str(u).lower() for u in adv.service_uuids):
            print('FRURT_DEVICE', repr(name), address, 'rssi', adv.rssi, 'uuids', adv.service_uuids, flush=True)
    print('PC_BLE_SCAN_DONE count', len(devices), flush=True)

asyncio.run(main())
