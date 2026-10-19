"""Static regressions for the isolated Task029 Zephyr claim-lock adapter."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
FEATURE = ROOT / "src/features/uk_lora_one_shot"
TEST_APP = ROOT / "tests/uk_lora_zephyr_claim_lock"


class ZephyrClaimLock(unittest.TestCase):
    def test_lock_uses_atomic_nonblocking_state_transitions(self):
        source = (FEATURE / "uk_lora_zephyr_claim_lock.c").read_text()
        header = (FEATURE / "uk_lora_zephyr_claim_lock.h").read_text()
        self.assertIn("atomic_cas(&lock->state, CLAIM_LOCK_AVAILABLE, CLAIM_LOCK_HELD)", source)
        self.assertIn("atomic_cas(&lock->state, CLAIM_LOCK_UNINITIALIZED, CLAIM_LOCK_INITIALIZING)",
                      source)
        self.assertIn("atomic_set(&lock->state, CLAIM_LOCK_AVAILABLE)", source)
        self.assertIn("atomic_cas(&lock->state, CLAIM_LOCK_HELD, CLAIM_LOCK_AVAILABLE)", source)
        self.assertIn("struct frurt_os_lock_ops ops;", header)
        self.assertNotIn("k_mutex", source + header)
        self.assertNotIn("k_sem", source + header)

    def test_compile_app_is_isolated_from_production_and_radio_tx(self):
        app_cmake = (TEST_APP / "CMakeLists.txt").read_text()
        app_source = (TEST_APP / "src/main.c").read_text()
        root_cmake = (ROOT / "CMakeLists.txt").read_text()
        production_cmake = (ROOT / "tests/sx1262_uk_one_shot/CMakeLists.txt").read_text()
        self.assertIn("uk_lora_zephyr_claim_lock.c", app_cmake)
        self.assertNotIn("uk_lora_zephyr_claim_lock.c", root_cmake)
        self.assertNotIn("uk_lora_zephyr_claim_lock.c", production_cmake)
        self.assertNotIn("radio_tx.c", app_cmake)
        self.assertNotIn("lora_send", app_source + app_cmake)
        self.assertNotIn("FRURT_UK_ONE_SHOT_TX_ALLOW=1", app_cmake)
        self.assertIn("FRURT_UK_ONE_SHOT_TX_ALLOW=0", production_cmake)

    def test_compile_app_exercises_shared_lock_api(self):
        app_source = (TEST_APP / "src/main.c").read_text()
        self.assertIn("frurt_os_zephyr_claim_lock_init(&store_claim_lock)", app_source)
        self.assertIn("frurt_os_zephyr_claim_lock_ops(&store_claim_lock)", app_source)
        self.assertIn("shared_ops->ctx != &store_claim_lock", app_source)


if __name__ == "__main__":
    unittest.main()
