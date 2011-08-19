#! /usr/bin/env python
try:
    import matevfs
except ImportError:
    import mate.vfs as matevfs

import atexit
import termios
import sys
from optparse import OptionParser
import gc

def _restore_term_mode(fd, attr):
    termios.tcsetattr(fd, termios.TCSADRAIN, attr)

def set_non_canonical(fd):
    old = termios.tcgetattr(fd)
    atexit.register(_restore_term_mode, fd, old)
    new = termios.tcgetattr(fd)
    new[3] = new[3] & ~termios.ICANON
    termios.tcsetattr(fd, termios.TCSADRAIN, new)


parser = OptionParser(usage="usage: %prog [options] source-uri dest-uri")
options, args = parser.parse_args()

if len(args) != 2:
    parser.error("wrong number of arguments")

src = matevfs.URI(args[0])
dst = matevfs.URI(args[1])

def progress_info_cb(info, data):
    assert data == 0x1234
    try:
        print "%s: %f %%\r" % (info.target_name,
                               info.bytes_copied/float(info.bytes_total)*100),
    except Exception, ex:
        pass
    return True
set_non_canonical(sys.stdout)
matevfs.xfer_uri(source_uri=src, target_uri=dst,
                  xfer_options=matevfs.XFER_DEFAULT,
                  error_mode=matevfs.XFER_ERROR_MODE_ABORT,
                  overwrite_mode=matevfs.XFER_OVERWRITE_MODE_ABORT,
                  progress_callback=progress_info_cb, data=0x1234)
