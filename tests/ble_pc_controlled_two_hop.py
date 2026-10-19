import argparse
import asyncio
import secrets
import time
from datetime import datetime, timezone
from typing import NamedTuple

from ble_pc_air_message import (
    COMPANY_ID,
    RX_UUID,
    TX_UUID,
    TextFrame,
    decode_text_frame,
)

DEFAULT_SOURCE_A_ADDRESS = "E5:12:76:D2:56:EA"
NODE_A_ID = bytes.fromhex("eb34ed25a5a4")


def log_event(message: str) -> None:
    utc = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    timestamp_ns = time.monotonic_ns()
    print(f"utc={utc} monotonic_ns={timestamp_ns} {message}", flush=True)


def acceptance_failures(
    observations: list[tuple[str, TextFrame]],
    source: str,
    relay: str,
    destination: str,
    message: bytes,
    destination_tx: bytes,
) -> list[str]:
    source = source.upper()
    relay = relay.upper()
    destination = destination.upper()
    source_sequences = {
        frame.sequence
        for address, frame in observations
        if address == source
        and frame.source_id == NODE_A_ID
        and frame.ttl == 3
        and frame.payload == message
    }
    relay_sequences = {
        frame.sequence
        for address, frame in observations
        if address == relay
        and frame.source_id == NODE_A_ID
        and frame.ttl == 2
        and frame.payload == message
    }
    failures = []
    if not source_sequences:
        failures.append("A on-air TEXT with TTL3 and the unique payload")
    if not source_sequences.intersection(relay_sequences):
        failures.append("B on-air forwarding of the same A sequence at TTL2")
    if any(
        address == destination
        and frame.source_id == NODE_A_ID
        and frame.sequence in source_sequences
        and frame.ttl == 1
        and frame.payload == message
        for address, frame in observations
    ):
        failures.append("C must not forward the message as TTL1")
    if destination_tx != message:
        failures.append("C GATT TX must contain the unique payload")
    return failures


def baseline_failures(
    observations: list[tuple[str, TextFrame]],
    source: str,
    relay: str,
    message: bytes,
    destination_before: bytes,
    destination_after: bytes,
) -> list[str]:
    source = source.upper()
    relay = relay.upper()
    source_sequences = {
        frame.sequence
        for address, frame in observations
        if address == source
        and frame.source_id == NODE_A_ID
        and frame.ttl == 3
        and frame.payload == message
    }
    failures = []
    if not source_sequences:
        failures.append("A on-air TEXT with TTL3 and the unique payload")
    if any(
        address == relay
        and frame.source_id == NODE_A_ID
        and frame.sequence in source_sequences
        and frame.ttl == 2
        and frame.payload == message
        for address, frame in observations
    ):
        failures.append("B must not forward the payload during the B-off control")
    if destination_after != destination_before:
        failures.append("C GATT TX must remain unchanged during the B-off control")
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run a controlled relay-positive trial or a B-off PC baseline. "
            "The baseline is not proof of RF isolation; inspect C USB diagnostics."
        )
    )
    parser.add_argument(
        "--mode",
        choices=("relay", "baseline"),
        default="relay",
        help="relay requires B TTL2 and C delivery; baseline requires B absent",
    )
    parser.add_argument(
        "--source", default=DEFAULT_SOURCE_A_ADDRESS, help="A BLE address"
    )
    parser.add_argument("--relay", required=True, help="B BLE address")
    parser.add_argument("--destination", required=True, help="C BLE address")
    parser.add_argument(
        "--message",
        help="Unique ASCII payload (1-14 bytes); generated when omitted.",
    )
    parser.add_argument("--scan-seconds", type=float, default=12.0)
    return parser.parse_args()


async def run_acceptance(args: argparse.Namespace, message: bytes) -> None:
    from bleak import BleakClient, BleakScanner

    source = args.source.upper()
    relay = args.relay.upper()
    destination = args.destination.upper()
    if len({source, relay, destination}) != 3:
        raise RuntimeError("A, B, and C must have distinct BLE addresses")

    found = await BleakScanner.discover(timeout=7, return_adv=True)
    devices = {
        address.upper(): device
        for address, (device, _advertisement) in found.items()
    }
    required = (
        (source, destination)
        if args.mode == "baseline"
        else (source, relay, destination)
    )
    missing = [address for address in required
               if address not in devices]
    if missing:
        raise RuntimeError(f"Required devices not found: {', '.join(missing)}")
    if args.mode == "baseline" and relay in devices:
        raise RuntimeError(
            "B is still BLE-discoverable; this baseline requires B physically "
            "powered off. The tool will not power down or reset devices."
        )

    async with BleakClient(devices[destination], timeout=12) as client:
        log_event("C_GATT_PRECHECK_CONNECT_BEGIN")
        destination_before = bytes(await client.read_gatt_char(TX_UUID))
    log_event("C_GATT_PRECHECK_DISCONNECTED")
    if destination_before == message:
        raise RuntimeError("Choose a payload different from C's existing GATT TX")
    log_event(f"C_GATT_PRECHECK value={destination_before!r}")

    observations: list[tuple[str, TextFrame]] = []
    printed = set()

    def on_advertisement(device, advertisement):
        frame = decode_text_frame(
            COMPANY_ID,
            advertisement.manufacturer_data.get(COMPANY_ID, b""),
            message,
        )
        if frame is None or frame.source_id != NODE_A_ID:
            return
        address = device.address.upper()
        observations.append((address, frame))
        key = (address, frame.source_id, frame.sequence, frame.ttl)
        if key not in printed:
            printed.add(key)
            if address == source:
                label = "A_SOURCE_ON_AIR"
            elif address == relay:
                label = "B_RELAY_ON_AIR"
            elif address == destination:
                label = "C_ON_AIR"
            else:
                label = "OTHER_NODE_ON_AIR"
            log_event(
                f"{label} address={address} source={frame.source_id.hex()} "
                f"sequence={frame.sequence} ttl={frame.ttl} "
                f"payload={frame.payload!r}"
            )

    async with BleakScanner(on_advertisement):
        await asyncio.sleep(1)
        async with BleakClient(devices[source], timeout=12) as client:
            log_event("A_GATT_CONNECT_BEGIN")
            await client.write_gatt_char(RX_UUID, message, response=True)
            log_event(f"A_GATT_WRITE_OK value={message!r}")
            source_tx = bytes(await client.read_gatt_char(TX_UUID))
            log_event(
                f"A_SOURCE_GATT_ECHO={'PASS' if source_tx == message else 'FAIL'} "
                f"value={source_tx!r}"
            )
            if source_tx != message:
                raise RuntimeError("A GATT source echo did not match the payload")
        log_event("A_GATT_DISCONNECTED_SCAN_CONTINUES")
        await asyncio.sleep(args.scan_seconds)

    async with BleakClient(devices[destination], timeout=12) as client:
        log_event("C_GATT_RESULT_CONNECT_BEGIN")
        destination_tx = bytes(await client.read_gatt_char(TX_UUID))
    log_event("C_GATT_RESULT_DISCONNECTED")
    log_event(
        f"C_GATT_TX_DELIVERY="
        f"{'PASS' if destination_tx == message else 'FAIL'} "
        f"value={destination_tx!r}"
    )

    if args.mode == "baseline":
        failures = baseline_failures(
            observations,
            source,
            relay,
            message,
            destination_before,
            destination_tx,
        )
    else:
        failures = acceptance_failures(
            observations, source, relay, destination, message, destination_tx
        )
    if failures:
        raise RuntimeError(
            "ACCEPTANCE_FAIL: " + "; ".join(failures)
            + (
                ". A clean PC baseline is not RF-isolation proof; require "
                "C-side receive diagnostics and decode-counter evidence."
                if args.mode == "baseline"
                else ". A TTL3 direct observation alone is insufficient."
            )
        )
    if args.mode == "baseline":
        log_event(
            "B_OFF_PC_BASELINE=PASS "
            "RF_ISOLATION=NOT_ESTABLISHED_CHECK_C_USB_EVIDENCE"
        )
    else:
        log_event("CONTROLLED_A_TO_B_TO_C_ACCEPTANCE=PASS")


def main() -> None:
    args = parse_args()
    if args.scan_seconds <= 0:
        raise SystemExit("--scan-seconds must be positive")
    try:
        message = (
            args.message.encode("ascii")
            if args.message is not None
            else f"HOP011{secrets.token_hex(4).upper()}".encode("ascii")
        )
    except UnicodeEncodeError as error:
        raise SystemExit("--message must contain ASCII characters") from error
    if not message or len(message) > 14:
        raise SystemExit("--message must contain 1-14 bytes")
    asyncio.run(run_acceptance(args, message))


if __name__ == "__main__":
    main()
