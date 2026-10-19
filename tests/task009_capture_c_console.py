"""Capture C serial diagnostics while running Task 009 PC acceptance."""
import pathlib
import re
import subprocess
import sys
import threading
import time
from typing import NamedTuple

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOG = ROOT / 'docs' / 'task009-kit3-console.log'
stop = threading.Event()

_C_RX_RECORD = re.compile(
    r"TEST topology event=RX_ACCEPT role=C type=(TEXT|BEACON) "
    r"source=([0-9a-fA-F]{12}) seq=([0-9]+) ttl=([0-9]+) "
    r"uptime_ms=([0-9]+) rc=0"
)


class CReceiveEvidence(NamedTuple):
    packet_type: str
    source: bytes
    sequence: int
    ttl: int
    uptime_ms: int


def parse_c_receive_record(line: str) -> CReceiveEvidence | None:
    match = _C_RX_RECORD.fullmatch(line)
    if match is None:
        return None

    packet_type, source, sequence, ttl, uptime_ms = match.groups()
    parsed_sequence = int(sequence)
    parsed_ttl = int(ttl)
    parsed_uptime = int(uptime_ms)
    if parsed_sequence > 0xffff or not 1 <= parsed_ttl <= 3:
        return None
    if parsed_uptime > 0xffffffff:
        return None
    return CReceiveEvidence(
        packet_type,
        bytes.fromhex(source),
        parsed_sequence,
        parsed_ttl,
        parsed_uptime,
    )


class ConsoleLineBuffer:
    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, chunk: bytes) -> list[bytes]:
        self._buffer.extend(chunk)
        lines = []
        while True:
            newline = self._buffer.find(b'\n')
            if newline < 0:
                return lines
            line = bytes(self._buffer[:newline])
            del self._buffer[:newline + 1]
            if line.endswith(b'\r'):
                line = line[:-1]
            lines.append(line)

    def finish(self) -> list[bytes]:
        if not self._buffer:
            return []
        line = bytes(self._buffer)
        self._buffer.clear()
        if line.endswith(b'\r'):
            line = line[:-1]
        return [line]


def read_console():
    import serial

    with serial.Serial('COM16', 115200, timeout=0.3) as port, LOG.open(
        'w', encoding='utf-8'
    ) as output:
        line_buffer = ConsoleLineBuffer()

        def write_lines(lines: list[bytes]) -> None:
            timestamp_ms = time.monotonic_ns() // 1_000_000
            for line in lines:
                value = line.decode('utf-8', errors='replace')
                record = f'C_CONSOLE monotonic_ms={timestamp_ms} {value}'
                output.write(record + '\n')
                output.flush()
                print(record, flush=True)

        while not stop.is_set():
            write_lines(line_buffer.feed(port.read(256)))
        write_lines(line_buffer.finish())


def main():
    thread = threading.Thread(target=read_console, daemon=True)
    thread.start()
    time.sleep(2)
    cmd = [
        sys.executable,
        str(ROOT / 'tests' / 'ble_pc_controlled_two_hop.py'),
        '--source', 'E5:12:76:D2:56:EA',
        '--relay', 'C8:51:08:6E:08:C0',
        '--destination', 'E2:DB:6A:54:BB:71',
        '--scan-seconds', '20',
    ]
    try:
        result = subprocess.run(cmd, cwd=ROOT, timeout=100)
        print('ACCEPTANCE_EXIT', result.returncode, flush=True)
    finally:
        time.sleep(2)
        stop.set()
        thread.join(timeout=3)
    print('CONSOLE_LOG', LOG, flush=True)


if __name__ == '__main__':
    main()
