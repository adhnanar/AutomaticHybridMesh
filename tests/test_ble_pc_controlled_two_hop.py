import unittest

from ble_pc_air_message import TextFrame
from ble_pc_controlled_two_hop import acceptance_failures, baseline_failures


class ControlledTwoHopAcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.source = "E5:12:76:D2:56:EA"
        self.relay = "C8:51:08:6E:08:C0"
        self.destination = "E2:DB:6A:54:BB:71"
        self.message = b"HOP00912345678"
        self.source_frame = TextFrame(
            bytes.fromhex("eb34ed25a5a4"), 31, 3, self.message
        )
        self.relay_frame = TextFrame(
            bytes.fromhex("eb34ed25a5a4"), 31, 2, self.message
        )

    def test_requires_same_sequence_forwarded_at_ttl2_and_c_gatt_tx(self):
        observations = [
            (self.source, self.source_frame),
            (self.relay, self.relay_frame),
        ]

        self.assertEqual(
            acceptance_failures(
                observations, self.source, self.relay, self.destination,
                self.message, self.message,
            ),
            [],
        )

    def test_direct_a_ttl3_and_source_echo_are_not_two_hop_evidence(self):
        failures = acceptance_failures(
            [(self.source, self.source_frame)],
            self.source,
            self.relay,
            self.destination,
            self.message,
            self.message,
        )

        self.assertIn(
            "B on-air forwarding of the same A sequence at TTL2", failures
        )

    def test_previous_run_frames_do_not_match_the_unique_payload(self):
        old_message = b"HOP009OLD0001"
        old_source_frame = self.source_frame._replace(
            sequence=33, payload=old_message
        )
        old_relay_frame = self.relay_frame._replace(
            sequence=33, payload=old_message
        )

        failures = acceptance_failures(
            [
                (self.source, old_source_frame),
                (self.relay, old_relay_frame),
            ],
            self.source,
            self.relay,
            self.destination,
            self.message,
            self.message,
        )

        self.assertIn(
            "A on-air TEXT with TTL3 and the unique payload", failures
        )
        self.assertIn(
            "B on-air forwarding of the same A sequence at TTL2", failures
        )

    def test_rejects_unrelated_relay_sequence_and_ttl1_forward(self):
        observations = [
            (self.source, self.source_frame),
            (self.relay, self.relay_frame._replace(sequence=32)),
            (
                self.destination,
                self.relay_frame._replace(ttl=1),
            ),
        ]

        failures = acceptance_failures(
            observations,
            self.source,
            self.relay,
            self.destination,
            self.message,
            b"old",
        )

        self.assertEqual(len(failures), 3)
        self.assertIn(
            "B on-air forwarding of the same A sequence at TTL2", failures
        )
        self.assertIn("C must not forward the message as TTL1", failures)
        self.assertIn("C GATT TX must contain the unique payload", failures)

    def test_b_off_baseline_requires_fresh_a_and_unchanged_c(self):
        failures = baseline_failures(
            [(self.source, self.source_frame)],
            self.source,
            self.relay,
            self.message,
            b"Ready",
            b"Ready",
        )
        self.assertEqual(failures, [])

    def test_b_off_baseline_rejects_b_forwarding_and_c_change(self):
        failures = baseline_failures(
            [
                (self.source, self.source_frame),
                (self.relay, self.relay_frame),
            ],
            self.source,
            self.relay,
            self.message,
            b"Ready",
            self.message,
        )
        self.assertEqual(len(failures), 2)
        self.assertIn(
            "B must not forward the payload during the B-off control", failures
        )
        self.assertIn(
            "C GATT TX must remain unchanged during the B-off control",
            failures,
        )

    def test_b_off_baseline_requires_matching_fresh_a_frame(self):
        failures = baseline_failures(
            [(self.source, self.source_frame._replace(payload=b"OLD"))],
            self.source,
            self.relay,
            self.message,
            b"Ready",
            b"Ready",
        )
        self.assertEqual(
            failures, ["A on-air TEXT with TTL3 and the unique payload"]
        )


if __name__ == "__main__":
    unittest.main()
