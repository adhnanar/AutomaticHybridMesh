import unittest

from task009_console_integrity import analyze_console, parse_heartbeat


def beat(n, decoded=0, text=0):
    return (
        f"TEST topology event=HEARTBEAT role=B n={n} decoded={decoded} "
        f"text={text} decode_fail=0 rx_drop=0 uptime_ms={n * 5000} end"
    )


class ConsoleIntegrityTests(unittest.TestCase):
    def test_parses_only_complete_heartbeats(self):
        line = beat(3, 10, 1)
        self.assertEqual(parse_heartbeat(line), ("B", 3, 10, 1, 0, 0, 15000))
        self.assertIsNone(parse_heartbeat(line[:-7]))
        self.assertIsNone(parse_heartbeat("Hyb" + line))
        self.assertIsNone(parse_heartbeat(line.replace("role=B", "role=X")))

    def test_counts_missing_heartbeat_indices(self):
        result = analyze_console([beat(0), beat(1), beat(4, 5, 1)])
        self.assertEqual(result.heartbeats, 3)
        self.assertEqual(result.missing_indices, 2)
        self.assertTrue(result.counters_monotonic)
        self.assertEqual((result.last_decoded, result.last_text), (5, 1))

    def test_flags_damaged_topology_lines_and_counter_regression(self):
        result = analyze_console(
            ["TEST topology event=RX_AC", beat(1, 9), beat(2, 4)]
        )
        self.assertEqual(result.damaged_topology_lines, 1)
        self.assertFalse(result.counters_monotonic)

    def test_compact_heartbeat_checksum(self):
        body = "HB r=C n=7 d=12 t=3 f=0 q=0 u=35000"
        xor = 0
        for char in body.encode():
            xor ^= char
        line = f"{body} c={xor:02X}"
        self.assertEqual(parse_heartbeat(line), ("C", 7, 12, 3, 0, 0, 35000))
        self.assertIsNone(parse_heartbeat(line.replace("d=12", "d=13")))
        result = analyze_console([line, line[:-4], "HB r=C n=8 d=1"])
        self.assertEqual(result.heartbeats, 1)
        self.assertEqual(result.damaged_topology_lines, 2)

    def test_empty_console_reports_no_evidence(self):
        result = analyze_console([])
        self.assertEqual(result.heartbeats, 0)
        self.assertEqual(result.missing_indices, 0)


if __name__ == "__main__":
    unittest.main()
