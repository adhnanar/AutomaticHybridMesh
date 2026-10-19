import asyncio
from bleak import BleakClient

SVC = '7b3e0001-6f2a-4c91-9a3d-2f5b6c7d8e00'
def char(n): return '7b3e%04x-6f2a-4c91-9a3d-2f5b6c7d8e00' % n
KITS = [('Kit3','E2:DB:6A:54:BB:71'),('Kit2','C8:51:08:6E:08:C0')]

async def main():
 for name, address in KITS:
  print('CONNECT_START',name,address,flush=True)
  try:
   async with BleakClient(address,timeout=22.0) as c:
    print('CONNECTED',name,c.is_connected,flush=True)
    chars={str(ch.uuid).lower() for svc in c.services for ch in svc.characteristics}
    services={str(s.uuid).lower() for s in c.services}
    print('SERVICE_VALID',name,SVC in services,flush=True)
    for n in range(2,7):print('CHAR_PRESENT',name,n,char(n) in chars,flush=True)
    for n in [3,4,6]:
     try:
      data=await asyncio.wait_for(c.read_gatt_char(char(n)),timeout=12)
      print('READ',name,n,bytes(data).hex('-'),flush=True)
     except Exception as e: print('READ_ERROR',name,n,type(e).__name__,str(e),flush=True)
  except Exception as e:
   print('CONNECT_ERROR',name,type(e).__name__,str(e),flush=True)

asyncio.run(main())
