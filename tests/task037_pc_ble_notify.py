import asyncio
from bleak import BleakClient
K2='C8:51:08:6E:08:C0';K3='E2:DB:6A:54:BB:71'
def u(n):return f'7b3e{n:04x}-6f2a-4c91-9a3d-2f5b6c7d8e00'
async def main():
 events=[]
 async with BleakClient(K3,timeout=22) as rx:
  def cb(_,data):events.append(bytes(data));print('KIT3_TX_NOTIFY',bytes(data).hex('-'),flush=True)
  await rx.start_notify(u(3),cb)
  print('KIT3_RECEIVER_CONNECTED',flush=True)
  try:
   async with BleakClient(K2,timeout=22) as tx:
    print('KIT2_SENDER_CONNECTED',flush=True)
    await tx.write_gatt_char(u(2),b'NOTIFY24',response=True)
    print('KIT2_WRITE_OK',flush=True)
  except Exception as e:print('KIT2_SENDER_ERROR',type(e).__name__,str(e),flush=True)
  print('KIT2_SENDER_DISCONNECTED',flush=True)
  await asyncio.sleep(6)
  blob=bytes(await rx.read_gatt_char(u(6)))
  print('KIT3_INBOX',blob.hex('-'),flush=True)
  print('LIVE_NOTIFY_MATCH',b'NOTIFY24' in events,flush=True)
  await rx.stop_notify(u(3))
asyncio.run(main())
