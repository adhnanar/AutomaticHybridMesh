"""TASK035 pure-logic automatic BLE/LoRa transport-selection policy simulator.

SIMULATION ONLY. No firmware, radio, serial or BLE access. The "LoRa" medium is a
Python list; nothing here can emit RF. LoRa use is gated by an explicit,
independently supplied authorization object that defaults to NOT authorized.
Wire format reuses the 24-byte BLE mesh packet (10-byte header, 14-byte text).
"""

from dataclasses import dataclass, field

MASK = 0xFFFFFFFF
HEADER, PACKET_MAX = 10, 24
MAX_TEXT = PACKET_MAX - HEADER
MAX_HOPS = 3
TYPE_TEXT = 0x12
DEDUP_SIZE, DEDUP_WINDOW_MS = 16, 30000
BLE_FRESH_MS = 10000
BLE_NEAR_RSSI_DBM = -85
PENDING_MAX = 4
RETRY_MS = 1000
MAX_RETRIES = 3

BLE, LORA = "ble", "lora"

# Observable delivery states. SENT_* means "handed to a transport", never end-to-end ack.
ST_SENT_BLE = "SENT_BLE"
ST_SENT_LORA = "SENT_LORA"
ST_DELIVERED_LOCAL = "DELIVERED_LOCAL"
ST_QUEUED_NO_ROUTE = "QUEUED_NO_ROUTE"
ST_BLOCKED_LORA = "BLOCKED_LORA_NOT_AUTHORIZED"
ST_RETRY_EXHAUSTED = "RETRY_EXHAUSTED"
ST_DUPLICATE = "DUPLICATE"
ST_TTL_END = "TTL_END_NOT_FORWARDED"
ST_REJ_SIZE = "REJECTED_TOO_LARGE"
ST_REJ_EMPTY = "REJECTED_EMPTY"
ST_REJ_INVALID = "REJECTED_INVALID"
ST_QUEUE_FULL = "QUEUE_FULL"
SENT_STATES = {ST_SENT_BLE, ST_SENT_LORA}


@dataclass
class LoraAuthorization:
    """Independent TX authorization. Default: none. Never set by the router itself."""
    tx_authorized: bool = False
    hardware_present: bool = True


def encode(src, seq, ttl, text):
    return bytes([TYPE_TEXT]) + src + bytes([seq & 0xFF, seq >> 8, ttl]) + text


def decode(frame):
    if frame is None or not HEADER < len(frame) <= PACKET_MAX:
        return None
    ttl = frame[9]
    if frame[0] != TYPE_TEXT or not 1 <= ttl <= MAX_HOPS:
        return None
    return bytes(frame[1:7]), frame[7] | frame[8] << 8, ttl, bytes(frame[HEADER:])


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
class Policy:
    """Per-node policy. ble_send/lora_send return True if handed to the transport."""
    ble_send: callable
    lora_send: callable
    auth: LoraAuthorization = field(default_factory=LoraAuthorization)
    cache: Dedup = field(default_factory=Dedup)
    peers: dict = field(default_factory=dict)  # peer -> (last_seen_ms, rssi)
    pending: list = field(default_factory=list)  # [frame, retries, due_ms]
    states: list = field(default_factory=list)  # observable audit log
    inbox: list = field(default_factory=list)  # local delivery (phone GATT feed)
    lora_tx_attempts: int = 0

    def ble_peer_seen(self, peer, rssi, now):
        self.peers[peer] = (now, rssi)

    def ble_peer_lost(self, peer):
        self.peers.pop(peer, None)

    def ble_near(self, now):
        return any((now - t) & MASK <= BLE_FRESH_MS and r >= BLE_NEAR_RSSI_DBM
                   for t, r in self.peers.values())

    def lora_usable(self):
        return self.auth.hardware_present and self.auth.tx_authorized

    def _log(self, state, seq=None):
        self.states.append((state, seq))
        return state

    def _try(self, link, frame):
        if link == LORA:
            self.lora_tx_attempts += 1
            return bool(self.lora_send(frame))
        return bool(self.ble_send(frame))

    def _transmit(self, frame, now):
        """BLE if verified near, else LoRa only if authorized; failover either way."""
        tried = False
        for link in (BLE, LORA):
            if link == BLE and not self.ble_near(now):
                continue
            if link == LORA and not self.lora_usable():
                continue
            tried = True
            if self._try(link, frame):
                return ST_SENT_BLE if link == BLE else ST_SENT_LORA
        if not tried and self.auth.hardware_present and not self.auth.tx_authorized:
            return ST_BLOCKED_LORA  # only a LoRa route could exist, and it is forbidden
        return ST_QUEUED_NO_ROUTE

    def _route(self, frame, now, seq):
        st = self._transmit(frame, now)
        if st in SENT_STATES:
            return self._log(st, seq)
        if len(self.pending) >= PENDING_MAX:
            return self._log(ST_QUEUE_FULL, seq)
        self.pending.append([frame, 0, (now + RETRY_MS) & MASK])
        return self._log(st, seq)

    def send_local(self, src, seq, text, now):
        if not text:
            return self._log(ST_REJ_EMPTY, seq)
        if len(text) > MAX_TEXT:
            return self._log(ST_REJ_SIZE, seq)  # never truncate silently
        if not self.cache.accept(src, seq, now):
            return self._log(ST_DUPLICATE, seq)
        return self._route(encode(src, seq, MAX_HOPS, text), now, seq)

    def receive(self, ingress, frame, now):
        pkt = decode(frame)
        if pkt is None:
            return self._log(ST_REJ_INVALID)
        src, seq, ttl, text = pkt
        if not self.cache.accept(src, seq, now):
            return self._log(ST_DUPLICATE, seq)
        self.inbox.append((src, seq, text, ingress))
        self._log(ST_DELIVERED_LOCAL, seq)
        if ttl <= 1:
            return self._log(ST_TTL_END, seq)
        return self._route(encode(src, seq, ttl - 1, text), now, seq)

    def poll(self, now):
        keep = []
        for rec in self.pending:
            frame, retries, due = rec
            seq = frame[7] | frame[8] << 8
            if (now - due) & MASK >= 0x80000000:
                keep.append(rec)
                continue
            st = self._transmit(frame, now)
            if st in SENT_STATES:
                self._log(st, seq)
            elif retries + 1 >= MAX_RETRIES:
                self._log(ST_RETRY_EXHAUSTED, seq)
            else:
                rec[1] = retries + 1
                rec[2] = (now + RETRY_MS) & MASK
                keep.append(rec)
        self.pending = keep


class Sim3:
    """Nodes A,B,C. BLE links A-B and B-C (A-C out of BLE range); optional LoRa A-C
    over a purely virtual medium. Each node has an Android GATT endpoint (inbox)."""

    def __init__(self, lora_auth=()):
        self.now = 0
        self.ble_up = {("A", "B"): True, ("B", "C"): True}
        self.lora_air = []  # virtual medium only; never real RF
        self.nodes = {}
        for n in "ABC":
            auth = LoraAuthorization(tx_authorized=n in lora_auth)
            self.nodes[n] = Policy(lambda f, n=n: self._ble(n, f),
                                   lambda f, n=n: self._lora(n, f), auth)
        self.src = {n: n.encode() * 6 for n in "ABC"}
        self.seq = 0

    def refresh_beacons(self, rssi=-60):
        for (a, b), up in self.ble_up.items():
            if up:
                self.nodes[a].ble_peer_seen(b, rssi, self.now)
                self.nodes[b].ble_peer_seen(a, rssi, self.now)

    def _ble(self, n, frame):
        ok = False
        for (a, b), up in self.ble_up.items():
            if up and n in (a, b):
                self.nodes[b if n == a else a].receive(BLE, frame, self.now)
                ok = True
        return ok

    def _lora(self, n, frame):
        self.lora_air.append((n, frame))
        if n not in ("A", "C"):
            return False
        self.nodes["C" if n == "A" else "A"].receive(LORA, frame, self.now)
        return True

    def gatt_write(self, node, text):
        """Android nRF Connect writes text to a node's GATT characteristic."""
        self.seq += 1
        return self.nodes[node].send_local(self.src[node], self.seq, text, self.now)
