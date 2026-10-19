"""Read B and C heartbeat consoles for 60 seconds without BLE GATT test."""
import pathlib, serial, threading, time
from task009_console_integrity import analyze_console
root=pathlib.Path(__file__).resolve().parents[1]
duration=60
def observe(role, port):
    path=root/'docs'/f'task009-{role.lower()}-heartbeat-idle.log'
    lines=[]
    with serial.Serial(port,115200,timeout=0.2) as dev, path.open('w',encoding='utf-8') as out:
        buf=bytearray()
        end=time.monotonic()+duration
        while time.monotonic()<end:
            buf.extend(dev.read(256))
            while b'\n' in buf:
                i=buf.index(10)
                line=bytes(buf[:i]).rstrip(b'\r').decode('utf-8','replace')
                del buf[:i+1]
                out.write(f'monotonic_ms={time.monotonic_ns()//1000000} {line}\n')
                out.flush()
                lines.append(line)
        if buf:
            out.write(f'PARTIAL {buf.decode("utf-8","replace")}\n')
    print(f'{role}_IDLE_LOG={path}',flush=True)
    print(f'{role}_IDLE_INTEGRITY={analyze_console(lines)}',flush=True)
    print(f'{role}_IDLE_RECORDS={len(lines)}',flush=True)
threads=[threading.Thread(target=observe,args=(role,port)) for role,port in [('B','COM15'),('C','COM16')]]
for thread in threads:thread.start()
for thread in threads:thread.join()
