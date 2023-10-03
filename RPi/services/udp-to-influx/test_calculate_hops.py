import unittest
from udp_to_influx import CalculateNbrOfHops

class TestCalculateNbrOfHops(unittest.TestCase):

    def test_root_address(self):
        # Testing for scenario with root_address provided
        root = "2001:db8::1"
        obj = CalculateNbrOfHops(root)

        obj.calc_hops("2001:db8::2", "2001:db8::1")
        self.assertEqual(obj.calc_hops("2001:db8::3", "2001:db8::2"), 2)

        obj.calc_hops("2001:db8::4", "2001:db8::3")
        self.assertEqual(obj.calc_hops("2001:db8::5", "2001:db8::4"), 4)

    def test_no_root_address(self):
        # Testing for scenario with no root_address provided
        # Implicit 2001:db8::1 root
        obj = CalculateNbrOfHops(None)
        obj.calc_hops("2001:db8::2", "2001:db8::1")
        self.assertEqual(obj.calc_hops("2001:db8::3", "2001:db8::2"), 2)

        obj.calc_hops("2001:db8::4", "2001:db8::3")
        self.assertEqual(obj.calc_hops("2001:db8::5", "2001:db8::4"), 4)

    def test_invalid_path(self):
        # Testing for scenario with more than 30 hops
        root = "2001:db8::1"
        obj = CalculateNbrOfHops(root)

        for i in range(2, 33):
            obj.calc_hops(f"2001:db8::{i}", f"2001:db8::{i - 1}")

        self.assertEqual(obj.calc_hops("2001:db8::33", "2001:db8::32"), -1)

    def test_loop_detection(self):
        # Testing for loop in hops
        root = None
        obj = CalculateNbrOfHops(root)

        obj.calc_hops("2001:db8::2", "2001:db8::1")
        obj.calc_hops("2001:db8::3", "2001:db8::2")
        obj.calc_hops("2001:db8::4", "2001:db8::3")
        obj.calc_hops("2001:db8::2", "2001:db8::4")  # Introducing loop
        self.assertEqual(obj.calc_hops("2001:db8::5", "2001:db8::4"), -1)

if __name__ == '__main__':
    unittest.main()
