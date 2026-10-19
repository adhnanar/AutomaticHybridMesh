"""Offline Task024 ERP-envelope arithmetic tests (not hardware testing)."""
import importlib.util
import pathlib
import unittest
spec=importlib.util.spec_from_file_location(
    "erp",pathlib.Path(__file__).with_name("task024_erp_margin.py"))
erp=importlib.util.module_from_spec(spec)
spec.loader.exec_module(erp)

class ErpMarginTests(unittest.TestCase):
    def test_limit_is_25mw(self):
        self.assertAlmostEqual(erp.ERP_LIMIT_DBM,13.9794000867,places=6)
    def test_gain_increases_erp(self):
        self.assertAlmostEqual(
            erp.maximum_erp_dbm(-9,3,5)-erp.maximum_erp_dbm(-9,3,0),5)
    def test_more_line_loss_lowers_erp(self):
        self.assertAlmostEqual(
            erp.maximum_erp_dbm(-9,3,5,0)-erp.maximum_erp_dbm(-9,3,5,2),2)
    def test_low_power_hypothetical_15dbi_margin(self):
        self.assertGreater(erp.margin_db(-9,3,15),0)
    def test_high_power_hypothetical_gain_can_exceed(self):
        self.assertLess(erp.margin_db(14,3,5),0)
    def test_equal_to_limit(self):
        self.assertAlmostEqual(
            erp.margin_db(erp.ERP_LIMIT_DBM-3-2.15,3,4.30),0,places=6)
    def test_negative_error_rejected(self):
        with self.assertRaises(ValueError):
            erp.maximum_erp_dbm(-9,-1,2)
    def test_negative_min_loss_rejected(self):
        with self.assertRaises(ValueError):
            erp.maximum_erp_dbm(-9,1,2,-0.01)

if __name__=="__main__":
    unittest.main()
