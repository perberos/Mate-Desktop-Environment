#! /usr/bin/env python

import MateCORBA
import CORBA
import sys

if len(sys.argv) != 2:
    print "usage: %s <message>" % sys.argv[0]
    sys.exit(1)

## this should no longer be needed, but is here as workaround to
## http://bugzilla.mate.org/show_bug.cgi?id=323201
## you also need to use this if the server is not MateCORBA2 based
MateCORBA.load_file("./echo.idl")
#import Test  # use this if the server is not MateCORBA2 based

orb = CORBA.ORB_init(sys.argv)

ior = file('iorfile').read()
echo = orb.string_to_object(ior)#._narrow(Test.Echo) # _narrow not needed with MateCORBA2 servers

if sys.argv[1] == 'quit':
    echo.quit()
else:
    def callback(retval, exc_type, exc_value, user_data):
        assert user_data == "hello"
        print "async callback", retval
        orb.shutdown(0)
    echo.echo.async([sys.argv[1]], callback, "hello")
    orb.run()

