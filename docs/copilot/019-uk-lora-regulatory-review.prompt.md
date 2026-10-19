Independent review of TASK019 (UK 868.3 MHz regulatory profile) in c:\zephyrproject\hybrid-mesh-poc.
Read docs/TASK019-UK-LORA-REGULATORY-PROFILE.md, src/features/uk_lora_regulatory/*, tests/uk_lora_regulatory/*.
1. Build and run the ztest suite twice (default and -DUK868_TEST_LOCKED=ON) on qemu_cortex_m3 or native_sim; report real output.
2. Check: gate default denied, radio callback never called when gate closed/profile out-of-band/budget exhausted, reserve-before-TX, refund/overrun accounting, rolling window never forgets <1 h, 32-bit wrap, saturation, band edge + guard.
3. Visually confirm IR 2030 IR2030/1/16 duty-cycle wording (PDF pp. 20-21) is <=1% and not 0.1%.
Do not flash, transmit, or touch root CMake/GATT/firmware. Do not claim RF/regulatory compliance.
