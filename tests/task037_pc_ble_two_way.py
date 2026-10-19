import asyncio
from bleak import BleakClient
def uuid(n): return '7b3e%04x-6f2a-4c91-9a3d-2f5b6c7d8e00'%n
K2='C8:51:08:6E:08:C0'
K3='E2:DB:6A:54:BB:71'
def decode(img):
 if len(img)<4:return 'SHORT'
 pos=4; records=[]
 for _ in range(img[1]):
  if pos+9>len(img):return 'TRUNCATED '+img.hex('-')
  seq=int.from_bytes(img[pos:pos+2],'little');source=img[pos+2:pos+8].hex().upper(); length=img[pos+8];pos+=9
  if pos+length>len(img):return 'TRUNCATED '+img.hex('-')
  records.append((seq,source,img[pos:pos+length].decode('utf8','replace')));pos+=length
 return {'version':img[0],'count':img[1],'dropped':int.from_bytes(img[2:4],'little'),'records':records}
async def inbox(label,address):
 async with BleakClient(address,timeout=22) as c:
  b=bytes(await c.read_gatt_char(uuid(6)))
  print('INBOX',label,b.hex('-'),decode(b),flush=True)
  return b
async def send(label,address,payload):
 async with BleakClient(address,timeout=22) as c:
  await c.write_gatt_char(uuid(2),payload.encode(),response=True)
  tx=bytes(await c.read_gatt_char(uuid(3)))
  print('SENT_LOCAL',label,repr(payload),'TX',tx.decode('utf8','replace'),flush=True)
 print('SENDER_DISCONNECTED',label,flush=True)
async def main():
 await inbox('Kit2-BEFORE',K2)
 await send('Kit3',K3,'BACK32')
 await asyncio.sleep(4.0)
 await inbox('Kit2-AFTER',K2)
 await inbox('Kit3-BEFORE',K3)
 await send('Kit2',K2,'PC23')
 await asyncio.sleep(4.0)
 await inbox('Kit3-AFTER',K3)
asyncio.run(main())
