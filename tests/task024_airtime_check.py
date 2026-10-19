"""Task024 independent LoRa time-on-air calculator (Semtech AN1200.13 formula).

Written separately from src/features/uk_lora_regulatory (floating point, no
shared code) so the C integer implementation can be cross-checked. Offline only.
"""
import math

MARGIN_PERMILLE = 50  # must equal UK868_AIRTIME_MARGIN_PERMILLE


def airtime_s(payload, sf=9, bw_hz=125000, cr=1, preamble=8, crc=True, explicit=True):
    tsym = (2 ** sf) / bw_hz
    de = 1 if tsym > 0.016 else 0
    ih = 0 if explicit else 1
    num = 8 * payload - 4 * sf + 28 + (16 if crc else 0) - 20 * ih
    n_pay = 8 + max(math.ceil(num / (4 * (sf - 2 * de))) * (cr + 4), 0)
    return (preamble + 4.25 + n_pay) * tsym


def airtime_us_with_margin(payload, **kw):
    return math.ceil(airtime_s(payload, **kw) * 1e6 * (1000 + MARGIN_PERMILLE) / 1000)


if __name__ == "__main__":
    for n in (10, 16, 24):
        print(f"payload={n:>3}B raw={airtime_s(n)*1e3:.3f} ms "
              f"margined={airtime_us_with_margin(n)} us")
    one_hour_cap_us = 36_000_000
    print(f"1% of one hour = {one_hour_cap_us} us; "
          f"10 B margined fraction = {airtime_us_with_margin(10)/one_hour_cap_us*100:.4f}%")
