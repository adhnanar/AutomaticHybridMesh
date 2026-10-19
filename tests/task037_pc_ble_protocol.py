import asyncio
from bleak import BleakClient
K2='C8:51:08:6E:08:C0'; K3='E2:DB:6A:54:BB:71'
def u(n):return f'7b3e{n:04x}-6f2a-4c91-9a3d-2f5b6c7d8e00'
async def test(label,address):
 events=[]
 print('TEST_START',label,flush=True)
 async with BleakClient(address,timeout=22) as c:
  async def nope():pass
  def cb(sender, data):events.append((str(sender.uuid),bytes(data).hex('-')));print('NOTIFY',label,events[-1],flush=True)
  for n in (3,4):await c.start_notify(u(n),cb)
  before=bytes(await c.read_gatt_char(u(6)))
  try:
   await c.write_gatt_char(u(2),b'DISCOVER',response=True)
   print('DISCOVER_WRITE_PASS',label,flush=True)
  except Exception as e:print('DISCOVER_WRITE_ERROR',label,type(e).__name__,str(e),flush=True)
  await asyncio.sleep(1.0)
  tx=bytes(await c.read_gatt_char(u(3)))
  inbox=bytes(await c.read_gatt_char(u(6)))
  print('DISCOVER_NO_ECHO',label,tx!=b'DISCOVER', 'TX',tx.decode(errors='replace'),'INBOX_UNCHANGED',before==inbox,flush=True)
  try:
   await c.write_gatt_char(u(2),b'123456789012345',response=True)
   print('OVERSIZE_UNEXPECTED_ACCEPT',label,flush=True)
  except Exception as e:print('OVERSIZE_REJECT',label,type(e).__name__,str(e)[:160],flush=True)
  await c.stop_notify(u(3));await c.stop_notify(u(4))
 print('TEST_DONE',label,'EVENT_COUNT',len(events),flush=True)
async def main():
 for label, address in [('Kit2',K2),('Kit3',K3)]:
  try:await test(label,address)
  except Exception as e:print('TEST_ERROR',label,type(e).__name__,str(e),flush=True)
asyncio.run(main())
