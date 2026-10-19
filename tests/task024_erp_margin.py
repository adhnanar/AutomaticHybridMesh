"""Task024 offline worst-case UK 868.3MHz ERP envelope (NOT RF certification).
Ofcom IR2030/1/16 maximum 25 mW ERP for the selected 868.0-868.6MHz sub-band.
This calculator does not infer Seeed's antenna specification; hypothetical gains only.
"""
import argparse
import math

ERP_LIMIT_MW=25.0
ERP_LIMIT_DBM=10*math.log10(ERP_LIMIT_MW)
DIPOLE_REFERENCE_DBI=2.15

def maximum_erp_dbm(conducted_nominal_dbm, positive_power_error_db,
                    antenna_peak_gain_dbi, minimum_rf_loss_db=0.0):
    if positive_power_error_db<0 or minimum_rf_loss_db<0:
        raise ValueError("unphysical error/loss")
    return (conducted_nominal_dbm+positive_power_error_db+
            antenna_peak_gain_dbi-DIPOLE_REFERENCE_DBI-minimum_rf_loss_db)

def margin_db(conducted_nominal_dbm, positive_power_error_db,
              antenna_peak_gain_dbi, minimum_rf_loss_db=0.0):
    return ERP_LIMIT_DBM-maximum_erp_dbm(
        conducted_nominal_dbm,positive_power_error_db,
        antenna_peak_gain_dbi,minimum_rf_loss_db)

def print_table():
    print(f"UK 868.0-868.6 MHz ERP limit: {ERP_LIMIT_MW:g} mW = {ERP_LIMIT_DBM:.3f} dBm ERP")
    print("Hypothetical peak gain, NOT supplied Seeed kit antenna specification")
    print("Assumes 3.0 dB positive conducted-power uncertainty and 0.0 dB minimum chain loss.")
    print("conducted[dBm]  peak gain[dBi]  worst ERP[dBm]  margin[dB]  under_limit")
    for power in (-9,-6,0,10,14):
        for gain in (0,2,5,10,15,20):
            dbm=maximum_erp_dbm(power,3,gain)
            m=margin_db(power,3,gain)
            print(f"{power:>14} {gain:>15} {dbm:>15.2f} {m:>11.2f}  {m>=0}")
    print("WARNING: This envelope is illustrative. Hardware gain, output tolerance, antenna/feed")
    print("loss and radio-emission compliance are unverified. No transmit permission is granted.")

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    print_table()
