import unittest
import pygtk; pygtk.require("2.0")
import common
import tempfile
from common import mate

class MateTest(unittest.TestCase):
    def testProgramInit(self):
        prog = mate.program_init("foo", "bar", properties={"app-libdir": "foo"})
        self.assertEqual(prog.get_property("app-libdir"), "foo")

    def testPopt(self):
        popt_table = [("text", "", None, None, 0, "Test option", "")]
        program = mate.program_init("test-case", "0.1", popt_table=popt_table)

if __name__ == '__main__':
    unittest.main()
