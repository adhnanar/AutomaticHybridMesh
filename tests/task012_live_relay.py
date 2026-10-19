"""Automated PC-only positive relay trial; reads C binary evidence before/after."""
import asyncio, pathlib, re, subprocess, sys
from datetime import datetime, timezone
from task012_evidence import decode, read_record, delta

ROOT=pathlib.Path(__file__).resolve().parents[1]
C="E2:DB:6A:54:BB:71"
def emit(s): print(datetime.now(timezone.utc).isoformat(),s,flush=True)
before=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_BEFORE={before!r}')
cmd=[sys.executable,str(ROOT/'tests'/'ble_pc_controlled_two_hop.py'),
 '--mode','relay','--source','E5:12:76:D2:56:EA',
 '--relay','C8:51:08:6E:08:C0','--destination',C,'--scan-seconds','25']
try:
    run=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=130)
except subprocess.TimeoutExpired as e:
    emit('RELAY_RESULT=INCONCLUSIVE PC_TIMEOUT');sys.exit(1)
print(run.stdout,flush=True)
if run.stderr:print('PC_STDERR='+run.stderr,flush=True)
emit(f'RELAY_PC_EXIT={run.returncode}')
after=decode(asyncio.run(read_record(C)))
emit(f'C_EVIDENCE_AFTER={after!r}')
growth=delta(before,after)
emit(f'C_COUNTER_DELTAS={growth!r}')
a=re.findall(r'A_SOURCE_ON_AIR .*?source=eb34ed25a5a4 sequence=(\d+) ttl=3',run.stdout,re.I)
b=re.findall(r'B_RELAY_ON_AIR .*?source=eb34ed25a5a4 sequence=(\d+) ttl=2',run.stdout,re.I)
emit(f'A_SEQ={a} B_SEQ={b}')
if run.returncode or not a or not b or not (set(a)&set(b)):
    emit('RELAY_RESULT=FAIL_OR_INCONCLUSIVE_PC_PATH');sys.exit(1)
seq=int((set(a)&set(b)).pop())
fail=[]
if after.role!='C':fail.append('C role not C')
if growth['accept']<1:fail.append('C accept counter did not rise')
if after.last_seq!=seq:fail.append('C last sequence differs')
if after.last_ttl!=2:fail.append('C last TTL not 2')
if after.last_event not in ('ACCEPT','DUPLICATE'):fail.append('C last event not ACCEPT/DUPLICATE')
if after.last_source_tail!='25a5a4':fail.append('C source tail mismatch')
if growth['reject']!=0:fail.append('C rejected text indicating possible direct route or invalid topology')
emit(f'RELAY_EVIDENCE_FAILURES={fail!r}')
if fail:emit('RELAY_RESULT=INCONCLUSIVE_OR_FAIL_GATT');sys.exit(2)
emit('RELAY_RESULT=PASS_B_TTL2_C_ACCEPT_AND_GATT_DELIVERY')

