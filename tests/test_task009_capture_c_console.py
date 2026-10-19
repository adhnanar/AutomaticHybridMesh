import unittest

from task009_capture_c_console import (
    ConsoleLineBuffer,
    parse_c_receive_record,
)


class ConsoleLineBufferTests(unittest.TestCase):
    def test_preserves_partial_record_until_newline_arrives(self):
        lines = ConsoleLineBuffer()

        self.assertEqual(lines.feed(b"TEST topology ACCEPT role=C sour"), [])
        self.assertEqual(
            lines.feed(b"ce=ee07145a5e02 seq=42 ttl=2\r\n"),
            [b"TEST topology ACCEPT role=C source=ee07145a5e02 seq=42 ttl=2"],
        )

    def test_extracts_multiple_lines_and_retains_trailing_fragment(self):
        lines = ConsoleLineBuffer()

        self.assertEqual(
            lines.feed(b"first record\nsecond par"),
            [b"first record"],
        )
        self.assertEqual(
            lines.feed(b"t\nthird record\n"),
            [b"second part", b"third record"],
        )
        self.assertEqual(lines.finish(), [])

    def test_flushes_final_unterminated_record(self):
        lines = ConsoleLineBuffer()

        self.assertEqual(lines.feed(b"final fragment"), [])
        self.assertEqual(lines.finish(), [b"final fragment"])
        self.assertEqual(lines.finish(), [])

    def test_accepts_only_complete_c_receive_identity_records(self):
        record = (
            "TEST topology event=RX_ACCEPT role=C type=TEXT "
            "source=ee07145a5e02 seq=42 ttl=2 uptime_ms=123456 rc=0"
        )

        self.assertEqual(
            parse_c_receive_record(record),
            ("TEXT", bytes.fromhex("ee07145a5e02"), 42, 2, 123456),
        )
        self.assertIsNone(parse_c_receive_record(record + record))
        self.assertIsNone(parse_c_receive_record(record[:-8]))
        self.assertIsNone(
            parse_c_receive_record(record.replace("type=TEXT ", ""))
        )
        self.assertIsNone(
            parse_c_receive_record(record.replace("role=C", "role=B"))
        )
        self.assertIsNone(
            parse_c_receive_record(record.replace("seq=42", "seq=65536"))
        )
        self.assertIsNone(
            parse_c_receive_record(record.replace("ttl=2", "ttl=0"))
        )


if __name__ == "__main__":
    unittest.main()
