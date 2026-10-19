"""Review Task016 BLE/GATT PC logs. Measures notifications separately from PC RF sniff coverage."""
import ast
import pathlib
import re
import sys

ROOT=pathlib.Path(__file__).resolve().parents[1]
def get(pattern,text):
    m=re.search(pattern,text,re.MULTILINE)
    if not m:raise ValueError("Missing expected log field: "+pattern)
    return m.group(1)
def review(i):
    p=ROOT/"docs"/f"task016-pc-gatt-coexistence-{i:02}.log"
    text=p.read_text(encoding="utf-16")
    sent=ast.literal_eval(get(r"A_WRITE=(b'[^']+')",text))
    notifications=[ast.literal_eval(v) for v in re.findall(r"C_TX_NOTIFY=(b'[^']+')",text)]
    final=ast.literal_eval(get(r"C_FINAL_TX=(b'[^']+')",text))
    d=ast.literal_eval(get(r"C_EVIDENCE_DELTA=(\{[^}]+\})",text))
    on_air=ast.literal_eval(get(r"A_B_ON_AIR=(\[[^]]*\])",text))
    delivery_ok=sent==final and notifications.count(sent)==1 and d["accept"]==1
    relay_observed=any(a=="C8:51:08:6E:08:C0" and ttl==2 for a,seq,ttl in on_air)
    return p.name,delivery_ok,relay_observed,d["reject"],d["duplicate"]
def main():
    out=[review(i) for i in range(1,6)]
    print("TRIAL NOTIFICATION_PASS PC_B_TTL2_OBSERVED REJECTED_DIRECT_FRAMES DUPLICATES")
    for name,passed,b,rej,dup in out:
        print(f"{name} {passed} {b} {rej} {dup}")
    print(f"GATT_NOTIFICATION_PASS={sum(x[1] for x in out)}/5")
    print(f"PC_B_FORWARD_OBSERVED={sum(x[2] for x in out)}/5")
    print(f"TOTAL_C_DIRECT_REJECTS={sum(x[3] for x in out)}")
    return 0 if all(x[1] for x in out) else 1
if __name__=="__main__":sys.exit(main())
