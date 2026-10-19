import asyncio
from bleak import BleakScanner
TARGET={'C8:51:08:6E:08:C0','E5:12:76:D2:56:EA','E2:DB:6A:54:BB:71'}
async def main():
  def callback(dev,adv):
    if dev.address.upper() in TARGET:
      print('AIR',dev.address,'NAME',repr(adv.local_name),'MFG',[(hex(k),v.hex()) for k,v in adv.manufacturer_data.items()],'RSSI',adv.rssi,flush=True)
  async with BleakScanner(callback):
    await asyncio.sleep(13)
asyncio.run(main())
