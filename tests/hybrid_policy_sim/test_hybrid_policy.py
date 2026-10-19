import os
import random
import re
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
from hybrid_policy import *  # noqa: E402,F401,F403


def texts(sim, n):
    return [i[2] for i in sim.nodes[n].inbox]


class PolicyTests(unittest.TestCase):
    def test_c_router_policy_vectors_match_python_policy(self):
        vectors_path = os.path.join(
            os.path.dirname(__file__), "..", "hybrid_router", "policy_vectors.h"
        )
        with open(vectors_path, encoding="ascii") as source:
            vector_source = source.read()
        pattern = re.compile(
            r'X\("([^"]+)",\s*(\d+)U,\s*(-?\d+),\s*(true|false),\s*'
            r'(true|false),\s*(-?\d+),\s*(-?\d+),\s*\\\s*'
            r'(HYBRID_ROUTER_[A-Z_]+),\s*(HYBRID_ROUTER_LINK_[A-Z]+)\)',
            re.DOTALL,
        )
        vectors = pattern.findall(vector_source)
        self.assertEqual(len(vectors), 9)
        result_to_state = {
            "HYBRID_ROUTER_LORA_BLOCKED_QUEUED": ST_BLOCKED_LORA,
            "HYBRID_ROUTER_NO_LINK_QUEUED": ST_QUEUED_NO_ROUTE,
            "HYBRID_ROUTER_SENT_FALLBACK": ST_SENT_LORA,
        }

        for (name, age_ms, rssi_dbm, lora_available, lora_authorized,
             ble_rc, lora_rc, c_result, c_link) in vectors:
            calls = {"ble": 0, "lora": 0}

            def ble_send(_frame):
                calls["ble"] += 1
                return int(ble_rc) == 0

            def lora_send(_frame):
                calls["lora"] += 1
                return int(lora_rc) == 0

            policy = Policy(
                ble_send,
                lora_send,
                auth=LoraAuthorization(
                    tx_authorized=(lora_authorized == "true"),
                    hardware_present=(lora_available == "true"),
                ),
            )
            if int(rssi_dbm) >= BLE_NEAR_RSSI_DBM:
                policy.ble_peer_seen("peer", int(rssi_dbm), 0)

            actual = policy.send_local(
                b"\x01" * 6, 1, b"x", int(age_ms)
            )
            expected = result_to_state.get(c_result)
            if expected is None:
                expected = (
                    ST_SENT_BLE if c_link == "HYBRID_ROUTER_LINK_BLE"
                    else ST_SENT_LORA
                )
            with self.subTest(vector=name):
                self.assertEqual(actual, expected)
                if c_link == "HYBRID_ROUTER_LINK_BLE":
                    self.assertEqual(calls, {"ble": 1, "lora": 0})
                elif c_link == "HYBRID_ROUTER_LINK_LORA":
                    self.assertGreaterEqual(calls["lora"], 1)
                else:
                    self.assertEqual(calls, {"ble": 0, "lora": 0})

    def test_ble_multihop_and_all_phones_see_message(self):
        s = Sim3()
        s.refresh_beacons()
        self.assertEqual(s.gatt_write("A", b"hello"), ST_SENT_BLE)
        self.assertEqual(texts(s, "B"), [b"hello"])
        self.assertEqual(texts(s, "C"), [b"hello"])  # A->B->C, 2 hops
        self.assertEqual(texts(s, "A"), [])  # origin echo deduped, not re-delivered
        self.assertEqual(s.lora_air, [])

    def test_ttl_decrements_and_stops(self):
        s = Sim3()
        s.refresh_beacons()
        s.gatt_write("A", b"x")
        # A sent ttl3, B relayed ttl2, C relayed ttl1 back to B which dedups it
        self.assertIn((ST_DUPLICATE, 1), s.nodes["B"].states)
        end = encode(b"Z" * 6, 7, 1, b"end")
        self.assertEqual(s.nodes["B"].receive(BLE, end, 0), ST_TTL_END)
        self.assertEqual(texts(s, "B")[-1], b"end")  # delivered locally, not forwarded
        self.assertEqual(len(s.nodes["C"].inbox), 1)

    def test_no_ble_no_auth_is_blocked_not_delivered(self):
        s = Sim3()  # nobody authorized, no beacons -> no verified-near peer
        self.assertEqual(s.gatt_write("A", b"hi"), ST_BLOCKED_LORA)
        self.assertEqual(texts(s, "C"), [])
        self.assertEqual(s.lora_air, [])
        self.assertEqual(s.nodes["A"].lora_tx_attempts, 0)
        self.assertEqual(len(s.nodes["A"].pending), 1)
        self.assertEqual(s.nodes["A"].poll(999), None)
        self.assertEqual(len(s.nodes["A"].pending), 1)
        for t in (1000, 2000, 3000):
            self.assertEqual(s.nodes["A"].poll(t), None)
        self.assertEqual(s.nodes["A"].pending, [])
        self.assertEqual(
            [state for state, _ in s.nodes["A"].states].count(ST_RETRY_EXHAUSTED),
            1,
        )

    def test_unauthorized_never_transmits_over_many_polls(self):
        s = Sim3()
        s.gatt_write("A", b"hi")
        for t in range(0, 30000, 500):
            s.now = t
            s.nodes["A"].poll(t)
        self.assertEqual(s.lora_air, [])
        self.assertIn(ST_RETRY_EXHAUSTED, [x[0] for x in s.nodes["A"].states])
        self.assertEqual(s.nodes["A"].pending, [])

    def test_lora_used_only_when_authorized_and_ble_not_near(self):
        s = Sim3(lora_auth=("A",))
        self.assertEqual(s.gatt_write("A", b"far"), ST_SENT_LORA)
        self.assertEqual(texts(s, "C"), [b"far"])
        self.assertEqual(len(s.lora_air), 1)

    def test_weak_or_stale_ble_not_verified_near(self):
        s = Sim3()
        s.nodes["A"].ble_peer_seen("B", -95, 0)  # too weak
        self.assertEqual(s.gatt_write("A", b"a"), ST_BLOCKED_LORA)
        s.nodes["A"].ble_peer_seen("B", -60, 0)
        s.now = BLE_FRESH_MS + 1  # stale
        self.assertEqual(s.gatt_write("A", b"b"), ST_BLOCKED_LORA)

    def test_ble_preferred_over_authorized_lora(self):
        s = Sim3(lora_auth=("A", "B", "C"))
        s.refresh_beacons()
        self.assertEqual(s.gatt_write("A", b"near"), ST_SENT_BLE)
        self.assertEqual(s.lora_air, [])

    def test_failover_ble_to_lora_when_authorized(self):
        s = Sim3(lora_auth=("A",))
        s.refresh_beacons()
        s.ble_up[("A", "B")] = False  # link dropped but beacon still fresh
        self.assertEqual(s.gatt_write("A", b"fo"), ST_SENT_LORA)
        self.assertEqual(texts(s, "C"), [b"fo"])

    def test_failover_ble_failure_unauthorized_queues_not_delivered(self):
        s = Sim3()
        s.refresh_beacons()
        s.ble_up[("A", "B")] = False
        self.assertEqual(s.gatt_write("A", b"q"), ST_QUEUED_NO_ROUTE)
        self.assertEqual(s.lora_air, [])
        s.ble_up[("A", "B")] = True
        s.now += 1000
        s.nodes["A"].poll(s.now)  # retry delivers over recovered BLE
        self.assertEqual(texts(s, "C"), [b"q"])

    def test_cross_transport_dedup(self):
        s = Sim3(lora_auth=("A",))
        s.refresh_beacons()
        frame = encode(s.src["A"], 99, 3, b"dup")
        s.nodes["C"].receive(LORA, frame, 0)
        self.assertEqual(s.nodes["C"].receive(BLE, frame, 5), ST_DUPLICATE)
        self.assertEqual(len(s.nodes["C"].inbox), 1)

    def test_size_limits_no_truncation(self):
        s = Sim3()
        s.refresh_beacons()
        self.assertEqual(s.gatt_write("A", b"x" * MAX_TEXT), ST_SENT_BLE)
        self.assertEqual(s.gatt_write("A", b"x" * (MAX_TEXT + 1)), ST_REJ_SIZE)
        self.assertEqual(s.gatt_write("A", b""), ST_REJ_EMPTY)
        self.assertEqual(texts(s, "C"), [b"x" * MAX_TEXT])

    def test_invalid_frames_rejected(self):
        p = Sim3().nodes["A"]
        for f in (None, b"", bytes(10), encode(b"A" * 6, 1, 0, b"x"), encode(b"A" * 6, 1, 4, b"x"),
                  b"\x99" + bytes(12), encode(b"A" * 6, 1, 1, b"x" * 15)):
            self.assertEqual(p.receive(BLE, f, 0), ST_REJ_INVALID)
        self.assertEqual(p.inbox, [])

    def test_queue_full_explicit(self):
        s = Sim3()
        res = [s.gatt_write("A", b"m") for _ in range(PENDING_MAX + 1)]
        self.assertEqual(res[-1], ST_QUEUE_FULL)

    def test_hardware_absent_is_queued_not_blocked(self):
        s = Sim3()
        s.nodes["A"].auth.hardware_present = False
        self.assertEqual(s.gatt_write("A", b"h"), ST_QUEUED_NO_ROUTE)

    def test_random_invariants_never_tx_without_auth(self):
        rng = random.Random(35)
        for _ in range(200):
            s = Sim3()  # no authorization anywhere
            for _ in range(40):
                s.now += rng.randrange(0, 700)  # total < dedup window
                op = rng.randrange(4)
                if op == 0:
                    s.refresh_beacons(rng.choice([-60, -95]))
                elif op == 1:
                    s.ble_up[rng.choice([("A", "B"), ("B", "C")])] = rng.random() < 0.5
                elif op == 2:
                    s.gatt_write(rng.choice("ABC"), bytes(rng.randrange(0, 20)))
                else:
                    for n in s.nodes.values():
                        n.poll(s.now)
            self.assertEqual(s.lora_air, [])
            for n in s.nodes.values():
                self.assertEqual(n.lora_tx_attempts, 0)
                keys = [(i[0], i[1]) for i in n.inbox]
                self.assertEqual(len(keys), len(set(keys)))  # delivered at most once


if __name__ == "__main__":
    unittest.main()
