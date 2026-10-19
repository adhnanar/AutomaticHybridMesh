"""Python reference model of src/features/hybrid_router (behavioral spec only).

This is NOT the C implementation and executing it proves nothing about the C
code. It exists to pin down intended behavior and run randomized invariants on
the policy. Mirrors the constants and decision order of hybrid_router.c.
"""

from dataclasses import dataclass, field

MASK = 0xFFFFFFFF
MAX_HOPS = 3
PACKET_MAX = 24
HEADER = 10
DEDUP_SIZE = 16
DEDUP_WINDOW_MS = 30000
PENDING_MAX = 4
RETRY_DELAY_MS = 1000
MAX_RETRIES = 3
BLE_FRESH_MS = 10000
BLE_MIN_RSSI_DBM = -85
TYPE_BEACON, TYPE_TEXT = 0x11, 0x12
BLE, LORA, NONE = "ble", "lora", None


def decode(frame):
    """Return (type, src, seq, ttl, payload) or None, like ble_mesh_packet_decode."""
    if frame is None or not HEADER <= len(frame) <= PACKET_MAX:
        return None
    t, src, seq, ttl = frame[0], bytes(frame[1:7]), frame[7] | frame[8] << 8, frame[9]
    payload = bytes(frame[HEADER:])
    if not 1 <= ttl <= MAX_HOPS:
        return None
    if t == TYPE_BEACON and payload:
        return None
    if t == TYPE_TEXT and not payload:
        return None
    if t not in (TYPE_BEACON, TYPE_TEXT):
        return None
    return t, src, seq, ttl, payload


def encode(src, seq, ttl, payload=b"hi"):
    return bytes([TYPE_TEXT]) + src + bytes([seq & 0xFF, seq >> 8, ttl]) + payload


@dataclass
class Dedup:
    entries: list = field(default_factory=lambda: [None] * DEDUP_SIZE)
    nxt: int = 0

    def accept(self, src, seq, now):
        for i, e in enumerate(self.entries):
            if e and e[0] == src and e[1] == seq:
                if (now - e[2]) & MASK < DEDUP_WINDOW_MS:
                    return False
                self.entries[i] = (src, seq, now)
                return True
        self.entries[self.nxt] = (src, seq, now)
        self.nxt = (self.nxt + 1) % DEDUP_SIZE
        return True


@dataclass
class Router:
    ble_send: callable
    lora_send: callable
    cache: Dedup = field(default_factory=Dedup)
    lora_available: bool = False
    lora_tx_authorized: bool = False
    allow_same_medium: bool = True
    ble_reachable: bool = False
    ble_seen: int = 0
    pending: list = field(default_factory=list)
    delivered: list = field(default_factory=list)
    deliver_enabled: bool = True
    retries_dropped: int = 0

    def ble_fresh(self, now):
        return self.ble_reachable and (now - self.ble_seen) & MASK <= BLE_FRESH_MS

    def ble_peer_seen(self, now_ms, rssi_dbm):
        if rssi_dbm >= BLE_MIN_RSSI_DBM:
            self.ble_reachable = True
            self.ble_seen = now_ms

    def ble_peer_lost(self):
        self.ble_reachable = False

    def _candidates(self, ingress, now):
        order = [BLE, LORA]
        ok = {
            BLE: self.ble_fresh(now),
            LORA: self.lora_available and self.lora_tx_authorized,
        }
        return [
            link for link in order
            if ok[link] and (link != ingress or self.allow_same_medium)
        ]

    def _transmit(self, frame, ingress, now):
        cands = self._candidates(ingress, now)
        if not cands:
            if (not self.ble_fresh(now) and self.lora_available
                    and not self.lora_tx_authorized):
                return "blocked_lora", None
            return "no_link", None
        for i, link in enumerate(cands):
            send = self.ble_send if link == BLE else self.lora_send
            if send(frame):
                return ("sent_fallback" if i else "sent"), link
        return "failed", None

    def _route(self, frame, ingress, now):
        res, via = self._transmit(frame, ingress, now)
        if res in ("sent", "sent_fallback"):
            return res, via
        if len(self.pending) >= PENDING_MAX:
            return "queue_full", None
        self.pending.append([frame, ingress, 0, (now + RETRY_DELAY_MS) & MASK])
        if res == "blocked_lora":
            return "blocked_lora_queued", None
        return ("no_link_queued" if res == "no_link" else "queued_retry"), None

    def send_local(self, src, seq, ttl, now):
        frame = encode(src, seq, ttl)
        if decode(frame) is None:
            return "invalid", None
        snap = (list(self.cache.entries), self.cache.nxt)
        if not self.cache.accept(src, seq, now):
            return "duplicate", None
        res = self._route(frame, NONE, now)
        if res[0] == "queue_full":
            self.cache.entries, self.cache.nxt = snap
        return res

    def receive(self, ingress, frame, now):
        pkt = decode(frame)
        if pkt is None:
            return "invalid", None
        t, src, seq, ttl, payload = pkt
        if t != TYPE_TEXT:
            return "not_forwarded", None
        if not self.cache.accept(src, seq, now):
            return "duplicate", None
        if self.deliver_enabled:
            self.delivered.append((src, seq))
        if ttl <= 1:
            return "not_forwarded", None
        return self._route(encode(src, seq, ttl - 1, payload), ingress, now)

    def poll(self, now):
        sent, keep = 0, []
        for rec in self.pending:
            frame, ingress, attempts, due = rec
            if (now - due) & MASK >= 0x80000000:  # not due (signed diff < 0)
                keep.append(rec)
                continue
            res, _ = self._transmit(frame, ingress, now)
            if res.startswith("sent"):
                sent += 1
            elif attempts + 1 >= MAX_RETRIES:
                self.retries_dropped += 1
            else:
                rec[2], rec[3] = attempts + 1, (now + RETRY_DELAY_MS) & MASK
                keep.append(rec)
        self.pending = keep
        return sent
