"""Task024 static proof that the LOCKED one-shot image has no reachable TX call.

Checks (source level, always) and, when a build directory is given, artefact level:
  1. lora_send/lora_send_async/lora_test_cw appear (outside comments) ONLY in
     tests/sx1262_uk_one_shot/src/radio_tx.c.
  2. CMake adds radio_tx.c only inside the FRURT_UK_ONE_SHOT_TX_ALLOW block, and
     that option defaults OFF.
  3. Locked-default macros are 0 in the feature headers.
  4. radio_tx.c refuses to compile when locked (#error).
  5. Build dir (optional): zephyr.map / object list must not contain radio_tx.c.obj,
     and CMakeCache must have FRURT_UK_ONE_SHOT_TX_ALLOW=OFF.
Usage: python tests/task024_static_tx_check.py [--build DIR]
"""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TX_TOKENS = re.compile(r"\b(lora_send|lora_send_async|lora_test_cw)\b")
ALLOWED = pathlib.Path("tests/sx1262_uk_one_shot/src/radio_tx.c")
SCAN_DIRS = ("src/features/uk_lora_regulatory", "src/features/uk_lora_one_shot",
             "tests/sx1262_uk_one_shot", "tests/sx1262_uk_tx_locked",
             "tests/uk_lora_one_shot", "tests/uk_lora_zephyr_claim_lock",
             "tests/uk_lora_regulatory")
SCAN_EXT = {".c", ".h"}


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def check_tokens(root=ROOT):
    bad = []
    for d in SCAN_DIRS:
        for f in (root / d).rglob("*"):
            if f.suffix not in SCAN_EXT:
                continue
            rel = f.relative_to(root)
            if TX_TOKENS.search(strip_comments(f.read_text(errors="ignore"))) and rel != ALLOWED:
                bad.append(str(rel))
    return bad


def check_cmake(root=ROOT):
    errs = []
    t = (root / "tests/sx1262_uk_one_shot/CMakeLists.txt").read_text()
    if not re.search(r'option\(FRURT_UK_ONE_SHOT_TX_ALLOW\s+"[^"]*"\s+OFF\)', t):
        errs.append("allow option is not default OFF")
    m = re.search(r"if\(FRURT_UK_ONE_SHOT_TX_ALLOW\)(.*?)else\(\)", t, re.S)
    if not m or "radio_tx.c" not in m.group(1):
        errs.append("radio_tx.c not in allow block")
    if "radio_tx.c" in t.replace(m.group(1) if m else "", ""):
        errs.append("radio_tx.c referenced outside allow block")
    return errs


def check_defaults(root=ROOT):
    errs = []
    h = (root / "src/features/uk_lora_one_shot/uk_lora_one_shot.h").read_text()
    if not re.search(r"#ifndef FRURT_UK_ONE_SHOT_TX_ALLOW\s+#define FRURT_UK_ONE_SHOT_TX_ALLOW 0", h):
        errs.append("one-shot default not 0")
    r = (root / "src/features/uk_lora_regulatory/uk_lora_regulatory.h").read_text()
    if not re.search(r"#ifndef UK868_ALLOW_GATE_OPEN[^#]*#define UK868_ALLOW_GATE_OPEN 0", r):
        errs.append("UK868 gate default not 0")
    rt = (root / ALLOWED).read_text()
    if not re.search(r"#if !FRURT_UK_ONE_SHOT_TX_ALLOW\s+#error", rt):
        errs.append("radio_tx.c lacks locked #error")
    # TASK025: reboot-safe store gate
    sh = (root / "src/features/uk_lora_one_shot/uk_lora_one_shot_store.h").read_text()
    if not re.search(r"#ifndef FRURT_UK_ONE_SHOT_STORE_ACCEPTED\s+#define FRURT_UK_ONE_SHOT_STORE_ACCEPTED 0", sh):
        errs.append("store accepted default not 0")
    c = (root / "src/features/uk_lora_one_shot/uk_lora_one_shot.c").read_text()
    if not re.search(r"#if FRURT_UK_ONE_SHOT_TX_ALLOW && !FRURT_UK_ONE_SHOT_STORE_ACCEPTED\s+#error", c):
        errs.append("allow path not hard-disabled without store acceptance")
    cm = (root / "tests/sx1262_uk_one_shot/CMakeLists.txt").read_text()
    if "STORE_ACCEPTED" in cm:
        errs.append("production CMake must not define STORE_ACCEPTED")
    if not re.search(r"if\(FRURT_UK_ONE_SHOT_TX_ALLOW\)\s+message\(FATAL_ERROR", cm):
        errs.append("production CMake allow option not FATAL_ERROR")
    return errs


def check_build(build):
    errs = []
    b = pathlib.Path(build)
    cache = b / "CMakeCache.txt"
    if not cache.exists():
        return ["no CMakeCache.txt in build dir"]
    c = cache.read_text(errors="ignore")
    if not re.search(r"FRURT_UK_ONE_SHOT_TX_ALLOW:BOOL=OFF", c):
        errs.append("build is not configured locked (cache)")
    for p in list(b.rglob("*.map")) + list(b.rglob("*.ninja")):
        if "radio_tx" in p.read_text(errors="ignore"):
            errs.append(f"radio_tx referenced in {p.name}")
    for p in b.rglob("radio_tx*"):
        errs.append(f"artefact exists: {p}")
    return errs


def run(build=None, root=ROOT):
    errs = [f"TX token outside radio_tx.c: {x}" for x in check_tokens(root)]
    errs += check_cmake(root) + check_defaults(root)
    if build:
        errs += check_build(build)
    return errs


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--build")
    a = ap.parse_args()
    e = run(a.build)
    for x in e:
        print("FAIL:", x)
    print("TASK024_STATIC_TX_CHECK=" + ("FAIL" if e else "PASS"))
    sys.exit(1 if e else 0)
