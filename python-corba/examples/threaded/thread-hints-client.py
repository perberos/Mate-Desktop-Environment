import sys
import threading
import time
import MateCORBA
import CORBA

## this should no longer be needed, but is here as workaround to
## http://bugzilla.mate.org/show_bug.cgi?id=323201
MateCORBA.load_file('./pyt.idl')

orb = CORBA.ORB_init(orb_id="matecorba-io-thread")

for num in [int(x) for x in sys.argv[1:]]:
    pyt = orb.string_to_object(open('ior%i' % num).read())
    t1 = threading.Thread(target=pyt.op)
    t1.start()

