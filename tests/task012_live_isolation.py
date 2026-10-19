"""Task012 B-off physical isolation trial; strict GATT RX evidence, no firmware flash."""
import asyncio, pathlib, re, subprocess, sys
from datetime import datetime, timezone
from task012_evidence import decode, read_record, delta

ROOT=pathlib.Path(__file__).resolve().parents[1]
C="E2:DB:6A:54:BB:71"
def emit(text): print(datetime.now(timezone.utc).isoformat(), text, flush=True)
before=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_BEFORE={before!r}')
cmd=[sys.executable,str(ROOT/'tests'/'ble_pc_controlled_two_hop.py'),'--mode','baseline',
     '--source','E5:12:76:D2:56:EA','--relay','C8:51:08:6E:08:C0',
     '--destination',C,'--scan-seconds','20']
result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120)
print(result.stdout,flush=True)
if result.stderr: print(result.stderr,flush=True)
emit(f'BASELINE_EXIT={result.returncode}')
after=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_AFTER={after!r}')
growth=delta(before,after)
emit(f'C_COUNTER_DELTAS={growth!r}')
matches=re.findall(r'A_SOURCE_ON_AIR .*?source=eb34ed25a5a4 sequence=(\d+) ttl=3',result.stdout,re.I)
if result.returncode or not matches:
    emit('ISOLATION_RESULT=INCONCLUSIVE_OR_FAILED_PC_BASELINE')
    sys.exit(1)
emit('OBSERVED_A_SEQUENCE='+matches[-1])
if before.role != 'C' or after.role != 'C':
    emit('ISOLATION_RESULT=INVALID_ROLE');sys.exit(1)
if growth['text'] or growth['reject'] or growth['accept'] or growth['duplicate']:
    emit('ISOLATION_RESULT=FAIL_C_DETECTED_TEXT')
    sys.exit(2)
if any(getattr(before,n)!=getattr(after,n) for n in ('last_seq','last_ttl','last_event','last_source_tail')):
    emit('ISOLATION_RESULT=INCONCLUSIVE_LAST_EVENT_CHANGED')
    sys.exit(1)
emit('ISOLATION_RESULT=PASS_NO_C_TEXT_DECODE_IN_TRIAL')
