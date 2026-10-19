"""Behavioral/property tests of the Python reference model (not the C code)."""

import os
import random
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import hybrid_router_ref as ref  # noqa: E402

SRC = b"\x01\x02\x03\x04\x05\x06"


def make(ble_ok=True, lora_ok=True):
    sent = {"ble": [], "lora": []}
    state = {"ble": ble_ok, "lora": lora_ok}

    def tx(name):
        def f(frame):
            sent[name].append(frame)
            return state[name]
        return f

    r = ref.Router(tx("ble"), tx("lora"))
    return r, sent, state


class ScenarioTests(unittest.TestCase):
    def test_fallback_once_same_ttl(self):
        r, sent, st = make(ble_ok=False)
        r.ble_reachable, r.lora_available = True, True
        r.lora_tx_authorized = True
        res, via = r.send_local(SRC, 1, 3, 10)
        self.assertEqual((res, via), ("sent_fallback", ref.LORA))
        self.assertEqual(sent["ble"], sent["lora"])
        self.assertEqual(sent["lora"][0][9], 3)

    def test_relay_prefers_ble_and_decrements_once(self):
        r, sent, _ = make()
        r.ble_reachable, r.lora_available = True, True
        r.lora_tx_authorized = True
        res, via = r.receive(ref.BLE, ref.encode(SRC, 5, 3), 10)
        self.assertEqual((res, via), ("sent", ref.BLE))
        self.assertEqual(sent["ble"][0][9], 2)
        self.assertEqual(sent["lora"], [])

    def test_unauthorized_lora_is_explicitly_blocked_and_not_called(self):
        r, sent, _ = make()
        r.lora_available = True
        self.assertEqual(
            r.send_local(SRC, 40, 3, 0)[0], "blocked_lora_queued"
        )
        self.assertEqual(sent["lora"], [])

    def test_ttl_one_delivered_not_forwarded(self):
        r, sent, _ = make()
        r.ble_reachable, r.lora_available = True, True
        self.assertEqual(r.receive(ref.LORA, ref.encode(SRC, 6, 1), 1)[0], "not_forwarded")
        self.assertEqual(len(r.delivered), 1)
        self.assertEqual(sent["ble"] + sent["lora"], [])

    def test_cross_media_duplicate(self):
        r, sent, _ = make()
        r.ble_reachable, r.lora_available = True, True
        f = ref.encode(SRC, 7, 3)
        r.receive(ref.BLE, f, 1)
        self.assertEqual(r.receive(ref.LORA, f, 2)[0], "duplicate")
        self.assertEqual(len(r.delivered), 1)
        self.assertEqual(len(sent["ble"]) + len(sent["lora"]), 1)

    def test_retry_preserves_frame_and_is_bounded(self):
        r, sent, st = make(ble_ok=False, lora_ok=False)
        r.ble_reachable, r.lora_available = True, True
        r.lora_tx_authorized = True
        res, _ = r.receive(ref.BLE, ref.encode(SRC, 8, 3), 100)
        self.assertEqual(res, "queued_retry")
        first = sent["lora"][0]
        st["lora"] = True
        self.assertEqual(r.poll(200), 0)
        self.assertEqual(r.poll(1100), 1)
        self.assertEqual(sent["lora"][-1], first)
        self.assertEqual(first[9], 2)

    def test_retry_exhaustion(self):
        r, _, _ = make(lora_ok=False)
        r.lora_available = True
        r.lora_tx_authorized = True
        r.send_local(SRC, 9, 2, 0)
        for t in (1000, 2000, 3000):
            r.poll(t)
        self.assertEqual((r.pending, r.retries_dropped), ([], 1))

    def test_queue_full_and_wraparound(self):
        r, _, _ = make()
        t0 = 0xFFFFFF00
        for i in range(ref.PENDING_MAX):
            self.assertEqual(r.send_local(SRC, 100 + i, 3, t0)[0], "no_link_queued")
        self.assertEqual(r.send_local(SRC, 200, 3, t0)[0], "queue_full")
        r.lora_available = True
        r.lora_tx_authorized = True
        self.assertEqual(r.poll(0xFFFFFF10), 0)
        self.assertEqual(r.poll((t0 + ref.RETRY_DELAY_MS) & ref.MASK), ref.PENDING_MAX)

    def test_stale_ble_and_wrap_freshness(self):
        r, _, _ = make()
        r.ble_peer_seen(0xFFFFFFF0, -60)
        self.assertTrue(r.ble_fresh(4))
        self.assertFalse(r.ble_fresh((0xFFFFFFF0 + ref.BLE_FRESH_MS + 1) & ref.MASK))

    def test_rssi_and_freshness_boundaries_match_c_router(self):
        r, sent, _ = make()
        r.ble_peer_seen(1000, -86)
        self.assertFalse(r.ble_fresh(1000))
        self.assertEqual(r.send_local(SRC, 30, 3, 1000)[0], "no_link_queued")
        self.assertEqual(sent["ble"], [])

        r.ble_peer_seen(2000, -85)
        self.assertTrue(r.ble_fresh(12000))
        r.ble_peer_seen(11000, -95)
        self.assertTrue(r.ble_fresh(12000))
        self.assertFalse(r.ble_fresh(12001))
        r.ble_peer_lost()
        self.assertFalse(r.ble_fresh(12000))

    def test_local_queue_full_allows_same_seq_retry(self):
        r, sent, _ = make()
        for i in range(ref.PENDING_MAX):
            self.assertEqual(r.send_local(SRC, 100 + i, 3, 0)[0], "no_link_queued")
        self.assertEqual(r.send_local(SRC, 200, 3, 0)[0], "queue_full")
        # Not admitted: a same-seq echo is not suppressed as a duplicate.
        r.lora_available = True
        r.lora_tx_authorized = True
        self.assertEqual(r.poll(1000), 1 + 3)  # drain all four slots
        self.assertEqual(r.send_local(SRC, 200, 3, 1100)[0], "sent")
        self.assertEqual(sent["lora"][-1][7], 200)
        # Now admitted: resend and own echo are duplicates.
        self.assertEqual(r.send_local(SRC, 200, 3, 1200)[0], "duplicate")
        self.assertEqual(r.receive(ref.LORA, ref.encode(SRC, 200, 3), 1300)[0],
                         "duplicate")

    def test_queue_full_rollback_keeps_earlier_admissions(self):
        r, _, _ = make()
        for i in range(ref.PENDING_MAX):
            r.send_local(SRC, 100 + i, 3, 0)
        self.assertEqual(r.send_local(SRC, 100, 3, 1)[0], "duplicate")
        self.assertEqual(r.send_local(SRC, 100, 3, 2)[0], "duplicate")
        self.assertEqual(r.send_local(SRC, 200, 3, 3)[0], "queue_full")
        self.assertEqual(r.send_local(SRC, 100, 3, 4)[0], "duplicate")

    def test_null_deliver_not_counted_but_relays(self):
        r, sent, _ = make()
        r.ble_reachable, r.lora_available = True, True
        r.lora_tx_authorized = True
        r.deliver_enabled = False
        res, via = r.receive(ref.BLE, ref.encode(SRC, 11, 3), 1)
        self.assertEqual((res, via, r.delivered), ("sent", ref.BLE, []))
        self.assertEqual(sent["ble"][0][9], 2)
        self.assertEqual(r.receive(ref.LORA, ref.encode(SRC, 11, 3), 2)[0], "duplicate")
        res, _ = r.receive(ref.BLE, ref.encode(SRC, 12, 1), 3)
        self.assertEqual(res, "not_forwarded")

    def test_malformed(self):
        r, sent, _ = make()
        r.ble_reachable, r.lora_available = True, True
        good = ref.encode(SRC, 1, 3)
        bad = [None, b"", good[:5], good + b"x" * 20, good[:9] + b"\x00" + good[10:],
               good[:9] + b"\x09" + good[10:], b"\x55" + good[1:]]
        for f in bad:
            self.assertEqual(r.receive(ref.BLE, f, 0)[0], "invalid")
        self.assertEqual((r.delivered, sent["ble"], sent["lora"]), ([], [], []))


class PropertyTests(unittest.TestCase):
    def test_random_traffic_invariants(self):
        rng = random.Random(15)
        for _ in range(200):
            r, sent, st = make()
            now = rng.choice([0, 0xFFFFF000])
            seen = set()
            for _ in range(150):
                now = (now + rng.randint(0, 3000)) & ref.MASK
                op = rng.random()
                st["ble"], st["lora"] = rng.random() < .7, rng.random() < .7
                r.lora_available = rng.random() < .7
                r.lora_tx_authorized = rng.random() < .7
                if rng.random() < .3:
                    r.ble_reachable, r.ble_seen = True, now
                if op < .6:
                    key = (rng.randint(0, 5), rng.randint(0, 255))
                    ttl = rng.randint(1, 3)
                    frame = ref.encode(bytes([key[0]]) * 6, key[1], ttl)
                    before = len(r.delivered)
                    res, _ = r.receive(rng.choice([ref.BLE, ref.LORA]), frame, now)
                    # One delivery per accepted packet, none for duplicates.
                    self.assertEqual(len(r.delivered) - before,
                                     0 if res == "duplicate" else 1)
                    # An immediate repeat on either medium is always suppressed.
                    if res != "duplicate":
                        self.assertEqual(
                            r.receive(rng.choice([ref.BLE, ref.LORA]), frame, now)[0],
                            "duplicate")
                    seen.add(key)
                    if ttl == 1 and res != "duplicate":
                        self.assertEqual(res, "not_forwarded")
                elif op < .8:
                    r.poll(now)
                else:
                    r.receive(ref.BLE, bytes(rng.randint(0, 30)), now)
                self.assertLessEqual(len(r.pending), ref.PENDING_MAX)
            for f in sent["ble"] + sent["lora"]:
                self.assertTrue(1 <= f[9] <= 3)


if __name__ == "__main__":
    unittest.main()
