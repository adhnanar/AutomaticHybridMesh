import unittest

from task011_isolation_evidence import evaluate_trial


MESSAGE = b"T011A1"
SOURCE = "eb34ed25a5a4"
SEQUENCE = 44


def pc_line(timestamp, event):
    return f"utc=2026-10-19T00:00:00.000+00:00 monotonic_ns={timestamp} {event}"


def console_line(role, timestamp, event):
    return (
        f"{role}_CONSOLE utc=2026-10-19T00:00:00.000+00:00 "
        f"monotonic_ns={timestamp} {event}"
    )


def heartbeat(role, index, text):
    body = (
        f"HB r={role} n={index} d={index * 10} t={text} "
        f"f=0 q=0 u={index * 5000}"
    )
    checksum = 0
    for character in body.encode("ascii"):
        checksum ^= character
    return f"{body} c={checksum:02X}"


def baseline_pc():
    return [
        pc_line(100, "C_GATT_PRECHECK value=b'Ready'"),
        pc_line(180, "A_GATT_CONNECT_BEGIN"),
        pc_line(200, f"A_GATT_WRITE_OK value={MESSAGE!r}"),
        pc_line(250, "A_GATT_DISCONNECTED_SCAN_CONTINUES"),
        pc_line(
            300,
            f"A_SOURCE_ON_AIR address=E5:12:76:D2:56:EA "
            f"source={SOURCE} sequence={SEQUENCE} ttl=3 payload={MESSAGE!r}",
        ),
        pc_line(1500, "C_GATT_RESULT_CONNECT_BEGIN"),
        pc_line(1600, "C_GATT_TX_DELIVERY=FAIL value=b'Ready'"),
        pc_line(
            1700,
            "B_OFF_PC_BASELINE=PASS "
            "RF_ISOLATION=NOT_ESTABLISHED_CHECK_C_USB_EVIDENCE",
        ),
    ]


def c_console(text_after, include_direct=False):
    lines = [
        console_line("C", 50, heartbeat("C", 1, 2)),
        console_line(
            "C",
            250,
            "TEST topology event=RX_REJECT role=C type=TEXT "
            f"source={SOURCE} seq={SEQUENCE} ttl=3 uptime_ms=5200 rc=0"
            if include_direct
            else "BLE mesh passive scan started",
        ),
        console_line("C", 1200, heartbeat("C", 2, text_after)),
    ]
    return lines


def relay_console(role, acceptance_event, ttl, text):
    return [
        console_line(role, 50, heartbeat(role, 1, text)),
        console_line(
            role,
            250,
            f"TEST topology event={acceptance_event} role={role} type=TEXT "
            f"source={SOURCE} seq={SEQUENCE} ttl={ttl} "
            "uptime_ms=5200 rc=0",
        ),
        console_line(role, 300, heartbeat(role, 2, text + 1)),
    ]


class Task011IsolationEvidenceTests(unittest.TestCase):
    def test_b_off_isolation_requires_zero_c_text_delta(self):
        failures = evaluate_trial(
            "baseline",
            "isolation",
            baseline_pc(),
            c_console(text_after=2),
        )
        self.assertEqual(failures, [])

    def test_b_off_isolation_fails_if_c_decodes_a_direct_frame(self):
        failures = evaluate_trial(
            "baseline",
            "isolation",
            baseline_pc(),
            c_console(text_after=3, include_direct=True),
        )
        self.assertIn("C logged direct A/TTL3 reception", failures)
        self.assertIn(
            "C decoded-text counter increased or is unmeasurable", failures
        )

    def test_sensitivity_control_requires_exact_reject_and_counter_delta(self):
        failures = evaluate_trial(
            "baseline",
            "sensitivity",
            baseline_pc(),
            c_console(text_after=3, include_direct=True),
        )
        self.assertEqual(failures, [])

    def test_sensitivity_control_does_not_pass_with_counter_only(self):
        failures = evaluate_trial(
            "baseline",
            "sensitivity",
            baseline_pc(),
            c_console(text_after=3),
        )
        self.assertIn("C did not log the direct A/TTL3 sensitivity frame", failures)

    def test_isolation_fails_on_incomplete_or_gapped_c_capture(self):
        lines = c_console(text_after=2)
        lines.pop()
        failures = evaluate_trial("baseline", "isolation", baseline_pc(), lines)
        self.assertTrue(any("heartbeat" in failure for failure in failures))

    def test_isolation_fails_on_partial_receive_line(self):
        lines = c_console(text_after=2)
        lines.append(
            console_line(
                "C",
                1300,
                "TEST topology event=RX_REJECT role=C type=TEXT "
                f"source={SOURCE} seq={SEQUENCE} ttl=3 "
                "uptime_ms=9000 rc=0",
            ).replace("C_CONSOLE utc=", "C_CONSOLE_PARTIAL utc=", 1)
        )
        failures = evaluate_trial("baseline", "isolation", baseline_pc(), lines)
        self.assertIn(
            "C console has incomplete receive, GATT, or heartbeat records",
            failures,
        )

    def test_relay_requires_both_console_paths_and_matching_air_frame(self):
        pc_lines = [
            pc_line(100, "C_GATT_PRECHECK value=b'Ready'"),
            pc_line(180, "A_GATT_CONNECT_BEGIN"),
            pc_line(200, f"A_GATT_WRITE_OK value={MESSAGE!r}"),
            pc_line(250, "A_GATT_DISCONNECTED_SCAN_CONTINUES"),
            pc_line(
                300,
                f"A_SOURCE_ON_AIR address=E5:12:76:D2:56:EA "
                f"source={SOURCE} sequence={SEQUENCE} ttl=3 "
                f"payload={MESSAGE!r}",
            ),
            pc_line(
                400,
                f"B_RELAY_ON_AIR address=C8:51:08:6E:08:C0 "
                f"source={SOURCE} sequence={SEQUENCE} ttl=2 "
                f"payload={MESSAGE!r}",
            ),
            pc_line(500, "CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS"),
            pc_line(
                600,
                f"C_GATT_TX_DELIVERY=PASS value={MESSAGE!r}",
            ),
        ]
        c_lines = relay_console("C", "RX_ACCEPT", 2, 1) + [
            console_line(
                "C",
                300,
                "TEST topology event=GATT_PUBLISH role=C type=TEXT "
                f"source={SOURCE} seq={SEQUENCE} ttl=2 "
                "uptime_ms=5500 rc=0",
            )
        ]
        b_lines = relay_console("B", "RX_ACCEPT", 3, 1) + [
            console_line(
                "B",
                300,
                "TEST topology event=RELAY_QUEUE_OK role=B type=TEXT "
                f"source={SOURCE} seq={SEQUENCE} ttl=2 "
                "uptime_ms=5500 rc=0",
            )
        ]
        self.assertEqual(
            evaluate_trial("relay", "relay", pc_lines, c_lines, b_lines), []
        )

    def test_relay_fails_when_c_also_hears_direct_a(self):
        pc_lines = [
            pc_line(100, "C_GATT_PRECHECK value=b'Ready'"),
            pc_line(180, "A_GATT_CONNECT_BEGIN"),
            pc_line(200, f"A_GATT_WRITE_OK value={MESSAGE!r}"),
            pc_line(250, "A_GATT_DISCONNECTED_SCAN_CONTINUES"),
            pc_line(
                300,
                f"A_SOURCE_ON_AIR address=E5:12:76:D2:56:EA "
                f"source={SOURCE} sequence={SEQUENCE} ttl=3 "
                f"payload={MESSAGE!r}",
            ),
            pc_line(
                400,
                f"B_RELAY_ON_AIR address=C8:51:08:6E:08:C0 "
                f"source={SOURCE} sequence={SEQUENCE} ttl=2 "
                f"payload={MESSAGE!r}",
            ),
            pc_line(500, "CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS"),
            pc_line(
                600,
                f"C_GATT_TX_DELIVERY=PASS value={MESSAGE!r}",
            ),
        ]
        c_lines = relay_console("C", "RX_ACCEPT", 2, 1) + [
            console_line(
                "C",
                350,
                "TEST topology event=RX_REJECT role=C type=TEXT "
                f"source={SOURCE} seq={SEQUENCE} ttl=3 "
                "uptime_ms=5400 rc=0",
            )
        ]
        b_lines = relay_console("B", "RX_ACCEPT", 3, 1) + [
            console_line(
                "B",
                300,
                "TEST topology event=RELAY_QUEUE_OK role=B type=TEXT "
                f"source={SOURCE} seq={SEQUENCE} ttl=2 "
                "uptime_ms=5500 rc=0",
            )
        ]
        failures = evaluate_trial("relay", "relay", pc_lines, c_lines, b_lines)
        self.assertIn("C also decoded direct A/TTL3 during relay trial", failures)


if __name__ == "__main__":
    unittest.main()
