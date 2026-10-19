"""Task012 PC-only B-off GATT evidence sensitivity trial. No firmware changes."""
import asyncio, pathlib, re, subprocess, sys
from datetime import datetime, timezone
from task012_evidence import decode, read_record, sensitivity_failures, delta

ROOT=pathlib.Path(__file__).resolve().parents[1]
C="E2:DB:6A:54:BB:71"
def stamp(): return datetime.now(timezone.utc).isoformat()
def emit(text): print(stamp(), text, flush=True)
before=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_BEFORE={before!r}')
cmd=[sys.executable,str(ROOT/'tests'/'ble_pc_controlled_two_hop.py'),'--mode','baseline',
     '--source','E5:12:76:D2:56:EA','--relay','C8:51:08:6E:08:C0',
     '--destination',C,'--scan-seconds','20']
result=subprocess.run(cmd,cwd=str(ROOT),capture_output=True,text=True,timeout=120)
print(result.stdout,flush=True)
if result.stderr: print(result.stderr,flush=True)
emit(f'BASELINE_EXIT={result.returncode}')
after=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_AFTER={after!r}')
emit(f'C_COUNTER_DELTAS={delta(before,after)!r}')
matches=re.findall(r'A_SOURCE_ON_AIR .*?source=eb34ed25a5a4 sequence=(\d+) ttl=3',result.stdout,re.I)
if not matches:
    emit('SENSITIVITY_RESULT=INCONCLUSIVE NO_A_TTL3_ON_AIR')
    sys.exit(1)
sequence=int(matches[-1])
failures=sensitivity_failures(before,after,'25a5a4',sequence)
emit(f'OBSERVED_A_SEQUENCE={sequence}')
emit(f'SENSITIVITY_FAILURES={failures!r}')
if result.returncode!=0 or failures:
    emit('SENSITIVITY_RESULT=INCONCLUSIVE_OR_FAIL')
    sys.exit(1)
emit('SENSITIVITY_RESULT=PASS_GATT_C_DIRECT_TTL3_REJECTION')

