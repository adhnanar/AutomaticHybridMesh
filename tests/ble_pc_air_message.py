import argparse
import asyncio
import time
from typing import NamedTuple

COMPANY_ID = 0xFFFF
TEXT_TYPE = 0x12
HEADER_LEN = 10
MAX_FRAME_LEN = 24
RX_UUID = "7b3e0002-6f2a-4c91-9a3d-2f5b6c7d8e00"
TX_UUID = "7b3e0003-6f2a-4c91-9a3d-2f5b6c7d8e00"
DEFAULT_SOURCE = "C8:51:08:6E:08:C0"
DEFAULT_REMOTES = [
    "E5:12:76:D2:56:EA",
    "E2:DB:6A:54:BB:71",
]


class TextFrame(NamedTuple):
    source_id: bytes
    sequence: int
    ttl: int
    payload: bytes


def decode_text_frame(
    company_id: int, data: bytes, expected_payload: bytes
) -> TextFrame | None:
    if (
        company_id != COMPANY_ID
        or len(data) < HEADER_LEN
        or len(data) > MAX_FRAME_LEN
        or data[0] != TEXT_TYPE
    ):
        return None

    ttl = data[9]
    payload = data[HEADER_LEN:]
    if ttl < 1 or ttl > 3 or payload != expected_payload:
        return None

    return TextFrame(
        source_id=data[1:7],
        sequence=int.from_bytes(data[7:9], "little"),
        ttl=ttl,
        payload=payload,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Distinguish GATT source echo, on-air mesh text, and remote GATT delivery."
    )
    parser.add_argument("--source", default=DEFAULT_SOURCE)
    parser.add_argument(
        "--remote", action="append", dest="remotes",
        help="Remote BLE address to inspect; may be repeated.",
    )
    parser.add_argument(
        "--message",
        help="ASCII test payload (1-14 bytes); defaults to a unique AIR008 payload.",
    )
    parser.add_argument("--scan-seconds", type=float, default=8.0)
    return parser.parse_args()


async def run_test(args: argparse.Namespace, message: bytes) -> None:
    from bleak import BleakClient, BleakScanner

    source = args.source.upper()
    remotes = [address.upper() for address in (args.remotes or DEFAULT_REMOTES)]
    found = await BleakScanner.discover(timeout=7, return_adv=True)
    devices = {
        address.upper(): device
        for address, (device, _advertisement) in found.items()
    }
    if source not in devices:
        raise RuntimeError(f"SOURCE_NOT_FOUND {source}")

    observed = []
    source_notifications = []

    def on_advertisement(device, advertisement):
        frame = decode_text_frame(
            COMPANY_ID,
            advertisement.manufacturer_data.get(COMPANY_ID, b""),
            message,
        )
        if frame is None:
            return
        observation = (device.address.upper(), frame)
        observed.append(observation)
        label = "SOURCE_ON_AIR" if observation[0] == source else "OTHER_NODE_ON_AIR"
        print(
            f"{label} address={observation[0]} "
            f"packet_source={frame.source_id.hex()} sequence={frame.sequence} "
            f"ttl={frame.ttl} payload={frame.payload!r}",
            flush=True,
        )

    def on_source_tx(_sender, data):
        value = bytes(data)
        source_notifications.append(value)
        print(f"SOURCE_TX_NOTIFY value={value!r}", flush=True)

    async with BleakScanner(on_advertisement):
        await asyncio.sleep(1)
        async with BleakClient(devices[source], timeout=12) as client:
            await client.start_notify(TX_UUID, on_source_tx)
            await client.write_gatt_char(RX_UUID, message, response=True)
            print(f"GATT_WRITE_OK source={source} value={message!r}", flush=True)
            source_tx = bytes(await client.read_gatt_char(TX_UUID))
            print(
                f"SOURCE_GATT_ECHO={'PASS' if source_tx == message else 'FAIL'} "
                f"value={source_tx!r}",
                flush=True,
            )
            await asyncio.sleep(0.5)
        await asyncio.sleep(args.scan_seconds)

    print(f"SOURCE_TX_NOTIFICATION_COUNT {len(source_notifications)}", flush=True)
    print(f"SOURCE_ON_AIR_COUNT {sum(addr == source for addr, _ in observed)}", flush=True)
    print(
        f"OTHER_NODE_ON_AIR_COUNT {sum(addr != source for addr, _ in observed)}",
        flush=True,
    )

    for address in remotes:
        if address == source:
            continue
        device = devices.get(address)
        if device is None:
            print(f"REMOTE_NOT_FOUND {address}", flush=True)
            continue
        try:
            async with BleakClient(device, timeout=12) as client:
                value = bytes(await client.read_gatt_char(TX_UUID))
                delivered = value == message
                print(
                    f"REMOTE_GATT_DELIVERY={'PASS' if delivered else 'FAIL'} "
                    f"address={address} value={value!r}",
                    flush=True,
                )
        except Exception as error:
            print(f"REMOTE_GATT_READ_FAILED {address}: {error!r}", flush=True)

    print(f"AIR_TEXT_COUNT {len(observed)}", flush=True)


def main() -> None:
    args = parse_args()
    if args.scan_seconds <= 0:
        raise SystemExit("--scan-seconds must be positive")
    try:
        message = (
            args.message.encode("ascii")
            if args.message is not None
            else f"AIR008-{time.strftime('%H%M%S')}".encode("ascii")
        )
    except UnicodeEncodeError as error:
        raise SystemExit("--message must contain ASCII characters") from error
    if not message or len(message) > 14:
        raise SystemExit("--message must contain 1-14 bytes")

    asyncio.run(run_test(args, message))


if __name__ == "__main__":
    main()
