import unittest
import globimap as gm

class MainTest(unittest.TestCase):
    def test_instance(self):       
        m = gm.globimap()
        self.assertEqual(m, m)

    def test_configure(self):       
        m = gm.globimap()
        m.configure(12, 20)
        self.assertEqual(m.d, 12)


if __name__ == '__main__':
    unittest.main()