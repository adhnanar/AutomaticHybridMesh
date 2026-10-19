import unittest

import task012_evidence as ev


def build(decoded=2, text=1, reject=1, accept=0, dup=0, seq=0x138, ttl=3,
          event=1, tail=b"\x25\xa5\xa4", role=b"C"):
    body = bytearray([1, role[0]])
    for value in (decoded, text, reject, accept, dup, seq):
        body += value.to_bytes(2, "little")
    body += bytes([ttl, event]) + tail
    checksum = 0
    for byte in body:
        checksum ^= byte
    return bytes(body + bytes([checksum]))


class EvidenceTests(unittest.TestCase):
    def test_decode(self):
        record = ev.decode(build())
        self.assertEqual(record.role, "C")
        self.assertEqual(record.last_seq, 0x138)
        self.assertEqual(record.last_event, "REJECT")
        self.assertEqual(record.last_source_tail, "25a5a4")

    def test_bad_checksum_length_version(self):
        good = bytearray(build())
        good[3] ^= 1
        with self.assertRaises(ValueError):
            ev.decode(bytes(good))
        with self.assertRaises(ValueError):
            ev.decode(b"\x01")
        bad_version = bytearray(build())
        bad_version[0] = 2
        bad_version[19] ^= 3
        with self.assertRaises(ValueError):
            ev.decode(bytes(bad_version))

    def test_delta_wraps(self):
        a = ev.decode(build(reject=65535))
        b = ev.decode(build(reject=2))
        self.assertEqual(ev.delta(a, b)["reject"], 3)

    def test_sensitivity_pass_and_fail(self):
        before = ev.decode(build(reject=0, seq=0, event=0))
        after = ev.decode(build(reject=1))
        self.assertEqual(
            ev.sensitivity_failures(before, after, "25A5A4", 0x138), [])
        self.assertTrue(ev.sensitivity_failures(after, after, "25a5a4", 0x138))
        wrong = ev.decode(build(accept=1, event=2))
        self.assertTrue(ev.sensitivity_failures(before, wrong, "25a5a4", 0x138))


if __name__ == "__main__":
    unittest.main()
