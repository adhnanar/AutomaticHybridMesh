from pathlib import Path
root=Path(r"C:\zephyrproject\hybrid-mesh-poc")
p=root/"src/features/ble_gatt/ble_gatt.c"
s=p.read_text(encoding="utf-8")
start="    int err = rx_handler(buf, len);"
end="    (void)ble_gatt_publish_text_message(buf, len);"
assert s.count(start)==1 and s.count(end)==1
a=s.index(start)
b=s.index(end,a)
new=r"""    int err = rx_handler(buf, len);
#if defined(CONFIG_BLE_GATT_DISCOVER_NO_ECHO) && CONFIG_BLE_GATT_DISCOVER_NO_ECHO
    switch (ble_gatt_rx_classify(err, true)) {
    case BLE_GATT_RX_ACTION_ERR_LENGTH:
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    case BLE_GATT_RX_ACTION_ERR_FAILED:
        printk("BLE GATT RX message handling failed: %d\n", err);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    case BLE_GATT_RX_ACTION_CONTROL_NO_TEXT:
        return len;
    case BLE_GATT_RX_ACTION_PUBLISH_TEXT:
    default:
        break;
    }
#else
    /* Keep the previously deployed RX data path when no-echo is disabled. */
    if (err == -EMSGSIZE) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    if (err != 0) {
        printk("BLE GATT RX message handling failed: %d\n", err);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }
#endif
"""
p.write_text(s[:a]+new+s[b:],encoding="utf-8",newline="")
ci=root/"tests/pc_ci.py"
s=ci.read_text(encoding="utf-8")
assert "][:900])" in s
ci.write_text(s.replace("][:900])", "][:1800])"),encoding="utf-8",newline="")
print("Guarded opt-in code compiled in only with flag on; increased generated-DTS parser window; patch complete.")
