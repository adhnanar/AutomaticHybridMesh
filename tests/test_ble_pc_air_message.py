import unittest

from ble_pc_air_message import COMPANY_ID, TEXT_TYPE, decode_text_frame


class DecodeTextFrameTests(unittest.TestCase):
    def setUp(self):
        self.payload = b"AIR008-123456"
        self.frame = (
            bytes([TEXT_TYPE])
            + bytes([1, 2, 3, 4, 5, 6])
            + bytes([0x34, 0x12, 2])
            + self.payload
        )

    def test_decodes_matching_text_advertisement(self):
        decoded = decode_text_frame(COMPANY_ID, self.frame, self.payload)

        self.assertIsNotNone(decoded)
        self.assertEqual(decoded.source_id, bytes([1, 2, 3, 4, 5, 6]))
        self.assertEqual(decoded.sequence, 0x1234)
        self.assertEqual(decoded.ttl, 2)
        self.assertEqual(decoded.payload, self.payload)

    def test_rejects_non_text_or_unrelated_advertisements(self):
        self.assertIsNone(decode_text_frame(0x1234, self.frame, self.payload))
        self.assertIsNone(
            decode_text_frame(COMPANY_ID, bytes([0x11]) + self.frame[1:], self.payload)
        )
        self.assertIsNone(
            decode_text_frame(COMPANY_ID, self.frame, b"another message")
        )

    def test_rejects_invalid_frame_bounds_and_ttl(self):
        self.assertIsNone(decode_text_frame(COMPANY_ID, b"\x12", self.payload))
        self.assertIsNone(
            decode_text_frame(
                COMPANY_ID, self.frame[:9] + b"\x00" + self.payload, self.payload
            )
        )
        self.assertIsNone(
            decode_text_frame(
                COMPANY_ID, self.frame[:9] + b"\x04" + self.payload, self.payload
            )
        )
        self.assertIsNone(
            decode_text_frame(COMPANY_ID, self.frame + b"x" * 15, self.payload)
        )


if __name__ == "__main__":
    unittest.main()
