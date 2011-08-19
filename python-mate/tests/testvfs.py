import unittest
import pygtk; pygtk.require("2.0")
import common
import tempfile
from common import matevfs

class VfsTest(unittest.TestCase):
    def testUri(self):
        uri=matevfs.URI("http://www.mate.org/index.html")
        self.assertEqual(uri.scheme, "http")
        self.assertEqual(uri.host_name, "www.mate.org")
        self.assertEqual(uri.short_name, "index.html")

    def testRead(self):
        tmpfile = tempfile.NamedTemporaryFile()
        written = "Tristitia"
        tmpfile.write(written)
        tmpfile.flush()
        read = matevfs.read_entire_file("file:" + tmpfile.name)
        self.assertEqual(read, written)


if __name__ == '__main__':
    unittest.main()
