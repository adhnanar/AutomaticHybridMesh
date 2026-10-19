"""Decode and compare the Task012 binary RX evidence GATT record.

Layout mirrors src/features/ble_mesh_relay/ble_mesh_evidence.h (20 bytes).
Reading needs a short GATT connection only; it never resets or flashes.
"""
import argparse
import asyncio
from typing import NamedTuple

EVIDENCE_UUID = "7b3e0005-6f2a-4c91-9a3d-2f5b6c7d8e00"
RECORD_LEN = 20
EVENT_NAMES = {0: "NONE", 1: "REJECT", 2: "ACCEPT", 3: "DUPLICATE"}


class Evidence(NamedTuple):
    role: str
    decoded: int
    text: int
    reject: int
    accept: int
    duplicate: int
    last_seq: int
    last_ttl: int
    last_event: str
    last_source_tail: str


def decode(record: bytes) -> Evidence:
    if len(record) != RECORD_LEN:
        raise ValueError(f"evidence record must be {RECORD_LEN} bytes")
    checksum = 0
    for byte in record[:-1]:
        checksum ^= byte
    if checksum != record[-1]:
        raise ValueError("evidence checksum mismatch")
    if record[0] != 1:
        raise ValueError(f"unsupported evidence version {record[0]}")
    u16 = lambda i: record[i] | (record[i + 1] << 8)
    return Evidence(
        role=chr(record[1]), decoded=u16(2), text=u16(4), reject=u16(6),
        accept=u16(8), duplicate=u16(10), last_seq=u16(12),
        last_ttl=record[14],
        last_event=EVENT_NAMES.get(record[15], f"UNKNOWN{record[15]}"),
        last_source_tail=record[16:19].hex(),
    )


def delta(before: Evidence, after: Evidence) -> dict:
    """Counter growth modulo 2**16 between two reads."""
    return {
        name: (getattr(after, name) - getattr(before, name)) & 0xFFFF
        for name in ("decoded", "text", "reject", "accept", "duplicate")
    }


def sensitivity_failures(before: Evidence, after: Evidence, source_tail: str,
                         sequence: int, ttl: int = 3) -> list:
    """Strict check that C itself rejected a direct A frame (A, TTL3)."""
    failures = []
    growth = delta(before, after)
    if growth["reject"] < 1:
        failures.append("C reject counter did not increase")
    if growth["accept"] != 0:
        failures.append("C accepted a frame during the B-off control")
    if after.last_event != "REJECT":
        failures.append("C last text event is not REJECT")
    if after.last_source_tail != source_tail.lower():
        failures.append("C last source tail does not match A")
    if after.last_seq != sequence & 0xFFFF:
        failures.append("C last sequence does not match the A frame")
    if after.last_ttl != ttl:
        failures.append(f"C last TTL is not {ttl}")
    return failures


async def read_record(address: str) -> bytes:
    from bleak import BleakClient
    async with BleakClient(address, timeout=12) as client:
        return bytes(await client.read_gatt_char(EVIDENCE_UUID))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("address", help="BLE address of the node to read")
    args = parser.parse_args()
    print(decode(asyncio.run(read_record(args.address))))


if __name__ == "__main__":
    main()
