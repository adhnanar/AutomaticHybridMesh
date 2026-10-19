"""Static guards for the Task033 inbox foundation (C is not executed here)."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
INBOX_H = (ROOT / "src/features/ble_inbox/ble_inbox.h").read_text()
INBOX_C = (ROOT / "src/features/ble_inbox/ble_inbox.c").read_text()


def define(text, name):
    return int(re.search(rf"#define {name} (\d+)U", text).group(1))


class InboxStaticTests(unittest.TestCase):
    def test_text_len_matches_mesh_packet(self):
        pkt = (ROOT / "src/features/ble_mesh_relay/ble_mesh_packet.h").read_text()
        max_len = define(pkt, "BLE_MESH_PACKET_MAX_LEN")
        hdr = define(pkt, "BLE_MESH_PACKET_HEADER_LEN")
        self.assertEqual(define(INBOX_H, "BLE_INBOX_MAX_TEXT_LEN"), max_len - hdr)
        self.assertEqual(define(INBOX_H, "BLE_INBOX_SOURCE_ID_LEN"),
                         define(pkt, "BLE_MESH_NODE_ID_LEN"))

    def test_image_fits_gatt_attribute_limit(self):
        slots = define(INBOX_H, "BLE_INBOX_SLOTS")
        total = 4 + slots * (9 + define(INBOX_H, "BLE_INBOX_MAX_TEXT_LEN"))
        self.assertLessEqual(total, 512)

    def test_no_dynamic_allocation_or_zephyr_dependency(self):
        for text in (INBOX_H, INBOX_C):
            self.assertNotRegex(text, r"\b(malloc|calloc|realloc|free|k_malloc)\b")
            self.assertNotIn("zephyr/", text)

    def test_default_off_and_gatt_integration_guarded(self):
        kconfig = (ROOT / "Kconfig").read_text()
        block = kconfig.split("config BLE_INBOX")[1]
        self.assertIn("default n", block)
        cmake = (ROOT / "CMakeLists.txt").read_text()
        self.assertIn("target_sources_ifdef(CONFIG_BLE_INBOX", cmake)
        self.assertNotIn("ble_inbox", cmake.split("target_sources_ifdef")[0])
        self.assertNotIn("ble_inbox", (ROOT / "src/main.c").read_text())
        for rel in ("src/features/ble_gatt/ble_gatt.c",
                    "src/features/ble_mesh_relay/ble_mesh_relay.c"):
            lines = (ROOT / rel).read_text().splitlines()
            guard = 0
            for line in lines:
                s = line.strip()
                if s.startswith("#if defined(CONFIG_BLE_INBOX)"):
                    guard += 1
                elif guard and s == "#endif":
                    guard -= 1
                elif guard == 0 and ("ble_inbox_service_" in s
                                     or "BT_UUID_HYBRID_INBOX" in s
                                     or "ble_inbox_service.h" in s):
                    self.fail(f"unguarded inbox reference in {rel}: {s}")

    def test_uuid_0006_unique_and_read_only(self):
        svc = (ROOT / "src/features/ble_inbox/ble_inbox_service.h").read_text()
        self.assertIn("0x7b3e0006, 0x6f2a, 0x4c91, 0x9a3d, 0x2f5b6c7d8e00", svc)
        gatt = (ROOT / "src/features/ble_gatt/ble_gatt.c").read_text()
        self.assertEqual(len(re.findall(r"0x7b3e0006", gatt + svc)), 1)
        self.assertEqual(len(re.findall(r"BT_GATT_CHARACTERISTIC\(BT_UUID_HYBRID_INBOX,"
                                        r"\s*BT_GATT_CHRC_READ,\s*BT_GATT_PERM_READ,"
                                        r"\s*ble_inbox_service_read, NULL,\s*NULL\)", gatt)), 1)

    def test_record_only_after_filter_at_publish_point(self):
        relay = (ROOT / "src/features/ble_mesh_relay/ble_mesh_relay.c").read_text()
        body = relay.split("static void process_received_packets")[1]
        body = body.split("static bool advertisement_parse")[0]
        self.assertEqual(body.count("ble_inbox_service_record("), 1)
        rec = body.index("ble_inbox_service_record(")
        self.assertLess(body.index("memcmp(packet.source_id, local_node_id"), rec)
        self.assertLess(body.index("packet.type != BLE_MESH_PACKET_TEXT"), rec)
        self.assertLess(body.index("ble_mesh_dedup_accept"), rec)
        self.assertLess(body.index("BLE_MESH_TOPOLOGY_DUPLICATE"), rec)
        gatt = (ROOT / "src/features/ble_gatt/ble_gatt.c").read_text()
        self.assertNotIn("ble_inbox_service_record", gatt)

    def test_service_uses_mutex_and_per_connection_snapshot(self):
        c = (ROOT / "src/features/ble_inbox/ble_inbox_service.c").read_text()
        self.assertIn("K_MUTEX_DEFINE(inbox_mutex)", c)
        self.assertIn("snapshots[CONFIG_BT_MAX_CONN]", c)
        self.assertIn("bt_conn_index", c)
        self.assertEqual(c.count("k_mutex_lock"), c.count("k_mutex_unlock"))
        self.assertNotRegex(c, r"\b(malloc|calloc|realloc|k_malloc)\b")
        self.assertRegex(INBOX_C, r"BLE_INBOX_IMAGE_MAX_LEN <= UINT8_MAX")


if __name__ == "__main__":
    unittest.main()
