"""Offline tests for the Task024 static TX check and airtime cross-check."""
import importlib.util
import pathlib
import shutil
import tempfile
import unittest

HERE = pathlib.Path(__file__).parent


def load(name):
    s = importlib.util.spec_from_file_location(name, HERE / f"{name}.py")
    m = importlib.util.module_from_spec(s)
    s.loader.exec_module(m)
    return m


chk = load("task024_static_tx_check")
air = load("task024_airtime_check")


class StaticCheck(unittest.TestCase):
    def test_repo_passes(self):
        self.assertEqual(chk.run(), [])

    def test_stray_send_detected(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            shutil.copytree(chk.ROOT / "src", root / "src")
            shutil.copytree(chk.ROOT / "tests", root / "tests",
                            ignore=shutil.ignore_patterns("__pycache__", "build*"))
            (root / "src/features/uk_lora_one_shot/evil.c").write_text("void f(){lora_send(0,0,0);}")
            self.assertTrue(chk.check_tokens(root))

    def test_task025_store_gate_tamper_detected(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            for sub in ("src/features/uk_lora_one_shot", "src/features/uk_lora_regulatory",
                        "tests/sx1262_uk_one_shot"):
                shutil.copytree(chk.ROOT / sub, root / sub,
                                ignore=shutil.ignore_patterns("__pycache__", "build*"))
            self.assertEqual(chk.check_defaults(root), [])
            h = root / "src/features/uk_lora_one_shot/uk_lora_one_shot_store.h"
            h.write_text(h.read_text().replace("FRURT_UK_ONE_SHOT_STORE_ACCEPTED 0",
                                               "FRURT_UK_ONE_SHOT_STORE_ACCEPTED 1"))
            cm = root / "tests/sx1262_uk_one_shot/CMakeLists.txt"
            cm.write_text(cm.read_text() + "\ntarget_compile_definitions(app PRIVATE FRURT_UK_ONE_SHOT_STORE_ACCEPTED=1)\n")
            errs = chk.check_defaults(root)
            self.assertTrue(any("store accepted default" in e for e in errs))
            self.assertTrue(any("must not define STORE_ACCEPTED" in e for e in errs))

    def test_comment_mention_ignored(self):
        self.assertNotIn("lora_send", chk.strip_comments("/* lora_send */ // lora_send_async\n"))


class Airtime(unittest.TestCase):
    def test_ten_byte_value(self):
        self.assertAlmostEqual(air.airtime_s(10), 0.144384, places=6)
        self.assertEqual(air.airtime_us_with_margin(10), 151604)

    def test_far_below_one_percent_hour(self):
        self.assertLess(air.airtime_us_with_margin(10), 36_000_000 * 0.01)


if __name__ == "__main__":
    unittest.main()
