"""
Some sample code demonstrating how to integrate python errors with MATE bug-buddy.

Code from bug #346106 (Fernando Herrera)
"""
import sys

def bug_catcher(exctype, value, tb):
    import traceback
    import tempfile
    import os
    if exctype is not KeyboardInterrupt:
        msg = "".join(traceback.format_exception(exctype, value, tb))
        print >> sys.stderr, msg
        fd, name = tempfile.mkstemp()
        try:
            os.write(fd,msg)
            os.system("bug-buddy --include=\"%s\" --appname=\"%s\"" % (name, sys.argv[0]))
        finally:
            os.unlink(name)
    raise SystemExit


sys.excepthook = bug_catcher

raise ValueError
