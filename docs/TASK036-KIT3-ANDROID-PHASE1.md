# Task036 Kit3 Android Phase-1 evidence — 2026-10-19

User nRF Connect screenshot 1: connected `MESH-5A5E02`, MAC E2:DB:6A:54:BB:71, hybrid service 7B3E0001 and read-only inbox 0006 present. Screenshot 2: inbox read `(0x) 01-00-00-00` (v1,count0,dropped0).

Screenshot 3 after requested `DISCOVER` test: Kit3 `Mesh-5A5E02` connected; `0003` TX shows `(0x) 52-65-61-64-79` = `Ready` (no DISCOVER echo); `0004` Discovery value `(0x) 50-45-45-52-3A-30-43-31-33-44-41-45-31-32-35-43-38` = `PEER:0C13DAE125C8` (Kit2 mesh ID); `0006` inbox stays `01-00-00-00`. PASS readable discovery and no-control-echo behaviour, subject to no actual write log visible. CCC display says `Notifications and indications disabled` on TX and Discovery, so live notify delivery NOT verified. No user approval yet to flash Kit2.

Kit1 LoRa receiver and Kit2 previous BLE topology unchanged. Phase 2 requires fresh, separate Kit2 flash approval; after flash, two-way messages and inbox should be tested with nRF Connect.
