# Task019 — UK LoRa region evidence / implementation handoff
Updated 2026-10-19. User explicitly confirmed the devices will operate in the **United Kingdom**.

## Regulatory evidence
Official Ofcom short-range-device guidance: https://www.ofcom.org.uk/spectrum/radio-equipment/short-range-devices
Official UK interface requirements IR 2030: https://www.ofcom.org.uk/siteassets/resources/documents/spectrum/interface-requirements/ir-2030.pdf
Read current IR2030/1/16 (PDF pages 19–20): non-specific SRD 868.0–868.6 MHz, max **25 mW ERP**, radio access / interference mitigation equivalent to designated standards, **or** alternative **<= 1% duty cycle**. Nearby sub-bands have different rules. Do not extrapolate from generic 'EU868' settings to all frequencies.
Ofcom 2026 SRD exemption regulations updates: https://www.ofcom.org.uk/spectrum/frequencies/notice-of-proposals-to-make-wireless-telegraphy-exemption-amendment-regulations-2025

## Proposed PC-only profile, not a radiating authorization
- Jurisdiction: UK.
- Proposed center: 868,300,000 Hz; proposed 125 kHz LoRa bandwidth, nominal occupied center±62.5 kHz (must validate actual modulation, emission mask and frequency accuracy for compliance). All nodes must share a chosen identical profile.
- ERP regulatory cap for that class: 25 mW ERP; **not** an SX1262 output register setting. Effective radiated output depends on configured dBm, RF front-end losses and antenna gain. Before physical TX, check antenna documentation, module reference design and measured/guaranteed maximum ERP including tolerances.
- Duty-cycle alternative: <= 1% across relevant operating budget; a rolling-hour software scheduler can conservatively cap *all* transmitted LoRa airtime (new messages, relay advertisements, retries and acknowledgments) at 36 s per hour for one device. Need account for radio airtime including preambles. This engineering guideline isn't by itself sufficient to guarantee legal compliance.
- No RF TX permission from this document. Must validate suitable radio parameter/antenna compliance, correct RF switch and TCXO, actual driver readiness, transmit power, airtime control, duty cycle, equipment/regulatory obligations, and explicit flash authorization first.
- Keep current Task012 A/C and Task010 B BLE firmware unchanged. Experimental radio overlays build but have never been executed on physical SX1262.

## Assigned implementation
Copilot Task019: isolated UK config/constants, hardware transmission gate default denied, monotonic airtime-budget tests and independent review. No firmware flash/LoRa send. `docs/TASK019-UK-LORA-REGULATORY-PROFILE.md` will contain Copilot's result when ready.
