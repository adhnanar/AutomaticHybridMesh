"""Evaluate captured Task011 PC and USB-console evidence without device access."""
import ast
import re
from typing import NamedTuple, Sequence

from task009_console_integrity import analyze_console, parse_heartbeat

_PC_PREFIX = re.compile(r"^utc=\S+ monotonic_ns=([0-9]+) (.*)$")
_CONSOLE_PREFIX = re.compile(
    r"^[ABC]_CONSOLE utc=\S+ monotonic_ns=([0-9]+) (.*)$"
)
_CONSOLE_PARTIAL_PREFIX = re.compile(
    r"^[ABC]_CONSOLE_PARTIAL utc=\S+ monotonic_ns=([0-9]+) (.*)$"
)
_PC_FRAME = re.compile(
    r"(A_SOURCE_ON_AIR|B_RELAY_ON_AIR|C_ON_AIR|OTHER_NODE_ON_AIR) "
    r"address=([0-9A-F:]+) source=([0-9a-fA-F]{12}) "
    r"sequence=([0-9]+) ttl=([0-9]+) payload=(b'.*')$"
)
_CONSOLE_EVENT = re.compile(
    r"TEST topology event=(RX_ACCEPT|RX_REJECT|RX_DUPLICATE|"
    r"GATT_PUBLISH|RELAY_QUEUE_OK) role=([ABC]) type=(TEXT|BEACON) "
    r"source=([0-9a-fA-F]{12}) seq=([0-9]+) ttl=([0-9]+) "
    r"uptime_ms=([0-9]+) rc=(-?[0-9]+)$"
)
_PC_BYTES = re.compile(r"^(?:A_GATT_WRITE_OK value=|C_GATT_PRECHECK value=|"
                       r"C_GATT_TX_DELIVERY=(?:PASS|FAIL) value=)(b'.*')$")
_EVIDENCE_EVENT_PREFIXES = (
    "TEST topology event=RX_ACCEPT",
    "TEST topology event=RX_REJECT",
    "TEST topology event=RX_DUPLICATE",
    "TEST topology event=GATT_PUBLISH",
    "TEST topology event=RELAY_QUEUE_OK",
)


class AirFrame(NamedTuple):
    label: str
    address: str
    source: bytes
    sequence: int
    ttl: int
    payload: bytes


class ConsoleEvent(NamedTuple):
    event: str
    role: str
    packet_type: str
    source: bytes
    sequence: int
    ttl: int
    uptime_ms: int
    result: int


def _pc_records(lines: Sequence[str]) -> list[tuple[int, str]]:
    records = []
    for line in lines:
        match = _PC_PREFIX.fullmatch(line.strip())
        if match:
            records.append((int(match.group(1)), match.group(2)))
    return records


def _console_records(lines: Sequence[str]) -> list[tuple[int, str]]:
    records = []
    for line in lines:
        match = _CONSOLE_PREFIX.fullmatch(line.rstrip("\r\n"))
        if match:
            records.append((int(match.group(1)), match.group(2)))
    return records


def _bytes_value(event: str) -> bytes | None:
    match = _PC_BYTES.fullmatch(event)
    if match is None:
        return None
    try:
        value = ast.literal_eval(match.group(1))
    except (SyntaxError, ValueError):
        return None
    return value if isinstance(value, bytes) else None


def _air_frames(records: Sequence[tuple[int, str]]) -> list[AirFrame]:
    frames = []
    for _, line in records:
        match = _PC_FRAME.search(line)
        if match is None:
            continue
        label, address, source, sequence, ttl, payload_value = match.groups()
        try:
            payload = ast.literal_eval(payload_value)
        except (SyntaxError, ValueError):
            continue
        if not isinstance(payload, bytes):
            continue
        frames.append(
            AirFrame(
                label,
                address.upper(),
                bytes.fromhex(source),
                int(sequence),
                int(ttl),
                payload,
            )
        )
    return frames


def _console_events(records: Sequence[tuple[int, str]]) -> list[ConsoleEvent]:
    events = []
    for _, line in records:
        match = _CONSOLE_EVENT.fullmatch(line)
        if match is None:
            continue
        event, role, packet_type, source, sequence, ttl, uptime, result = (
            match.groups()
        )
        events.append(
            ConsoleEvent(
                event,
                role,
                packet_type,
                bytes.fromhex(source),
                int(sequence),
                int(ttl),
                int(uptime),
                int(result),
            )
        )
    return events


def _damaged_receive_records(lines: Sequence[str]) -> int:
    complete_records = sum(
        line.startswith(_EVIDENCE_EVENT_PREFIXES)
        and _CONSOLE_EVENT.fullmatch(line) is None
        for _, line in _console_records(lines)
    )
    partial_records = 0
    for line in lines:
        match = _CONSOLE_PARTIAL_PREFIX.fullmatch(line.rstrip("\r\n"))
        if match is None:
            continue
        content = match.group(2)
        if content.startswith("HB ") or content.startswith(
            _EVIDENCE_EVENT_PREFIXES
        ):
            partial_records += 1
    return complete_records + partial_records


def _partial_firmware_lines(lines: Sequence[str]) -> list[str]:
    partial = []
    for line in lines:
        match = _CONSOLE_PARTIAL_PREFIX.fullmatch(line.rstrip("\r\n"))
        if match is not None:
            partial.append(match.group(2))
    return partial


def _trial_identity(
    pc_records: Sequence[tuple[int, str]],
    source_address: str,
    message: bytes,
) -> tuple[int, int, int, int, int] | None:
    connect_times = [
        timestamp
        for timestamp, line in pc_records
        if line == "A_GATT_CONNECT_BEGIN"
    ]
    write_times = [
        timestamp
        for timestamp, line in pc_records
        if line.startswith("A_GATT_WRITE_OK ")
        and _bytes_value(line) == message
    ]
    disconnect_times = [
        timestamp
        for timestamp, line in pc_records
        if line == "A_GATT_DISCONNECTED_SCAN_CONTINUES"
    ]
    source_frames = [
        (timestamp, frame)
        for timestamp, line in pc_records
        for frame in _air_frames([(timestamp, line)])
        if frame.label == "A_SOURCE_ON_AIR"
        and frame.address == source_address.upper()
        and frame.source == bytes.fromhex("eb34ed25a5a4")
        and frame.ttl == 3
        and frame.payload == message
        and disconnect_times
        and timestamp > disconnect_times[-1]
    ]
    if not connect_times or not write_times or not disconnect_times or not source_frames:
        return None
    if not connect_times[-1] <= write_times[0] <= disconnect_times[-1]:
        return None
    matching_sequences = {frame.sequence for _, frame in source_frames}
    if len(matching_sequences) != 1:
        return None
    return (
        connect_times[-1],
        write_times[0],
        disconnect_times[-1],
        source_frames[0][0],
        next(iter(matching_sequences)),
    )


def evaluate_trial(
    mode: str,
    control: str,
    pc_lines: Sequence[str],
    c_lines: Sequence[str],
    b_lines: Sequence[str] = (),
    pc_exit: int = 0,
    source_address: str = "E5:12:76:D2:56:EA",
    relay_address: str = "C8:51:08:6E:08:C0",
) -> list[str]:
    """Return evidence failures; an empty list means the requested control passed."""
    if mode not in ("baseline", "relay"):
        return ["unknown trial mode"]
    if control not in ("isolation", "sensitivity", "relay"):
        return ["unknown evidence control"]
    if (mode == "baseline") != (control != "relay"):
        return ["trial mode and evidence control do not match"]

    failures = []
    if pc_exit != 0:
        failures.append("PC BLE runner did not pass")
    pc_records = _pc_records(pc_lines)
    c_records = _console_records(c_lines)
    c_firmware_lines = [line for _, line in c_records]
    c_firmware_lines.extend(_partial_firmware_lines(c_lines))
    message = next(
        (
            value
            for _, line in pc_records
            if line.startswith("A_GATT_WRITE_OK ")
            and (value := _bytes_value(line)) is not None
        ),
        None,
    )
    if message is None:
        failures.append("unique A GATT write evidence is missing")
        return failures

    identity = _trial_identity(pc_records, source_address, message)
    if identity is None:
        failures.append("exactly one fresh A/TTL3 source sequence was not observed")
        return failures
    connect_time, _, disconnect_time, source_frame_time, sequence = identity

    if control in ("isolation", "sensitivity"):
        if "B_OFF_PC_BASELINE=PASS" not in "\n".join(
            line for _, line in pc_records
        ):
            failures.append("B-off PC baseline did not pass")
        before_values = [
            _bytes_value(line)
            for _, line in pc_records
            if line.startswith("C_GATT_PRECHECK ")
        ]
        after_values = [
            _bytes_value(line)
            for _, line in pc_records
            if line.startswith("C_GATT_TX_DELIVERY=")
        ]
        if not before_values or not after_values or before_values[-1] != after_values[-1]:
            failures.append("C GATT TX did not remain unchanged")
    else:
        relay_frames = [
            (timestamp, frame)
            for timestamp, line in pc_records
            for frame in _air_frames([(timestamp, line)])
            if frame.label == "B_RELAY_ON_AIR"
            and frame.address == relay_address.upper()
            and frame.source == bytes.fromhex("eb34ed25a5a4")
            and frame.sequence == sequence
            and frame.ttl == 2
            and frame.payload == message
            and timestamp > source_frame_time
        ]
        if not relay_frames:
            failures.append("matching B-address TTL2 advertisement is missing")
        if not any(
            line == "CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS"
            for _, line in pc_records
        ):
            failures.append("controlled A-to-B-to-C PC acceptance did not pass")

    heartbeats = []
    for timestamp, line in c_records:
        heartbeat = parse_heartbeat(line)
        if heartbeat is not None and heartbeat.role == "C":
            heartbeats.append((timestamp, heartbeat))
    integrity = analyze_console(c_firmware_lines)
    if len(heartbeats) < 2:
        failures.append("fewer than two complete C heartbeat records")
    if not integrity.counters_monotonic:
        failures.append("C heartbeat counters are not monotonic")
    if integrity.missing_indices != 0:
        failures.append("C heartbeat indices are missing")
    if integrity.damaged_topology_lines != 0:
        failures.append("C console has damaged topology or heartbeat records")
    if _damaged_receive_records(c_lines) != 0:
        failures.append(
            "C console has incomplete receive, GATT, or heartbeat records"
        )

    events = _console_events(c_records)
    matching_c = [
        event
        for event in events
        if event.role == "C"
        and event.packet_type == "TEXT"
        and event.source == bytes.fromhex("eb34ed25a5a4")
        and event.sequence == sequence
    ]

    if control in ("isolation", "sensitivity"):
        observation_end = next(
            (
                timestamp
                for timestamp, line in pc_records
                if line.startswith("C_GATT_RESULT_CONNECT_BEGIN")
            ),
            None,
        )
        before = [
            heartbeat
            for timestamp, heartbeat in heartbeats
            if timestamp <= connect_time
        ]
        during_after = [
            heartbeat
            for timestamp, heartbeat in heartbeats
            if timestamp > source_frame_time
            and (observation_end is None or timestamp < observation_end)
        ]
        if not before or not during_after:
            failures.append("C heartbeats do not span the RF observation")
            text_delta = None
        else:
            text_delta = during_after[-1].text - before[-1].text
            if during_after[-1].index <= before[-1].index:
                failures.append("C heartbeat did not advance during the trial")

        direct_events = [
            event
            for event in matching_c
            if event.event == "RX_REJECT" and event.ttl == 3
        ]
        if control == "isolation":
            if direct_events:
                failures.append("C logged direct A/TTL3 reception")
            if text_delta is None or text_delta != 0:
                failures.append("C decoded-text counter increased or is unmeasurable")
            if any(
                event.event in ("RX_ACCEPT", "RX_REJECT", "RX_DUPLICATE")
                for event in matching_c
            ):
                failures.append("C logged a receive event for the isolated payload")
        else:
            if not direct_events:
                failures.append("C did not log the direct A/TTL3 sensitivity frame")
            if text_delta is None or text_delta <= 0:
                failures.append("C decoded-text counter did not increase")
    else:
        b_events = _console_events(_console_records(b_lines))
        b_records = _console_records(b_lines)
        b_firmware_lines = [line for _, line in b_records]
        b_heartbeats = [
            parse_heartbeat(line)
            for _, line in b_records
            if parse_heartbeat(line) is not None
            and parse_heartbeat(line).role == "B"
        ]
        b_integrity = analyze_console(b_firmware_lines)
        if len(b_heartbeats) < 2:
            failures.append("fewer than two complete B heartbeat records")
        if not b_integrity.counters_monotonic:
            failures.append("B heartbeat counters are not monotonic")
        if b_integrity.missing_indices != 0:
            failures.append("B heartbeat indices are missing")
        if b_integrity.damaged_topology_lines != 0:
            failures.append("B console has damaged topology or heartbeat records")
        if _damaged_receive_records(b_lines) != 0:
            failures.append(
                "B console has incomplete receive, relay, or heartbeat records"
            )
        if not any(
            event.role == "B"
            and event.event == "RX_ACCEPT"
            and event.source == bytes.fromhex("eb34ed25a5a4")
            and event.sequence == sequence
            and event.ttl == 3
            for event in b_events
        ):
            failures.append("B did not log accepting the A/TTL3 frame")
        if not any(
            event.role == "B"
            and event.event == "RELAY_QUEUE_OK"
            and event.source == bytes.fromhex("eb34ed25a5a4")
            and event.sequence == sequence
            and event.ttl == 2
            for event in b_events
        ):
            failures.append("B did not log queueing the matching TTL2 relay")
        if not any(
            event.event == "RX_ACCEPT" and event.ttl == 2
            for event in matching_c
        ):
            failures.append("C did not log accepting the matching TTL2 frame")
        if not any(
            event.event == "GATT_PUBLISH"
            and event.ttl == 2
            and event.result == 0
            for event in matching_c
        ):
            failures.append("C did not log successful TTL2 GATT publication")
        if any(
            event.event == "RX_REJECT" and event.ttl == 3
            for event in matching_c
        ):
            failures.append("C also decoded direct A/TTL3 during relay trial")

    return failures
