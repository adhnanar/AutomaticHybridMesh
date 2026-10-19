"""Quantify USB console line loss from Task 009 firmware HEARTBEAT records."""
import re
from typing import Iterable, NamedTuple

_HEARTBEAT = re.compile(
    r"TEST topology event=HEARTBEAT role=([ABC]) n=([0-9]+) "
    r"decoded=([0-9]+) text=([0-9]+) decode_fail=([0-9]+) "
    r"rx_drop=([0-9]+) uptime_ms=([0-9]+) end"
)


_COMPACT = re.compile(
    r"HB r=([ABC]) n=([0-9]+) d=([0-9]+) t=([0-9]+) f=([0-9]+) "
    r"q=([0-9]+) u=([0-9]+) c=([0-9A-F]{2})"
)


class Heartbeat(NamedTuple):
    role: str
    index: int
    decoded: int
    text: int
    decode_fail: int
    rx_drop: int
    uptime_ms: int


class ConsoleIntegrity(NamedTuple):
    heartbeats: int
    missing_indices: int
    damaged_topology_lines: int
    counters_monotonic: bool
    last_decoded: int
    last_text: int


def parse_heartbeat(line: str) -> Heartbeat | None:
    compact = _COMPACT.fullmatch(line)
    if compact is not None:
        *fields, check = compact.groups()
        body = line[: line.rindex(" c=")]
        xor = 0
        for char in body.encode("ascii"):
            xor ^= char
        if int(check, 16) != xor:
            return None
        role, *numbers = fields
        values = [int(value) for value in numbers]
        if any(value > 0xffffffff for value in values):
            return None
        return Heartbeat(role, *values)
    match = _HEARTBEAT.fullmatch(line)
    if match is None:
        return None
    role, *numbers = match.groups()
    values = [int(value) for value in numbers]
    if any(value > 0xffffffff for value in values):
        return None
    return Heartbeat(role, *values)


def analyze_console(lines: Iterable[str]) -> ConsoleIntegrity:
    beats: list[Heartbeat] = []
    damaged = 0
    for line in lines:
        beat = parse_heartbeat(line)
        if beat is not None:
            beats.append(beat)
        elif (("TEST topology" in line and "uptime_ms=" not in line)
              or (line.startswith("HB ") and " c=" not in line)
              or (line.startswith("HB ") and parse_heartbeat(line) is None)):
            damaged += 1

    missing = 0
    monotonic = True
    for previous, current in zip(beats, beats[1:]):
        if current.index > previous.index:
            missing += current.index - previous.index - 1
        else:
            monotonic = False
        if (current.decoded < previous.decoded
                or current.text < previous.text):
            monotonic = False
    return ConsoleIntegrity(
        len(beats), missing, damaged, monotonic,
        beats[-1].decoded if beats else 0,
        beats[-1].text if beats else 0,
    )
