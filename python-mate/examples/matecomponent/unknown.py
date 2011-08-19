#! /usr/bin/env python
import pygtk; pygtk.require("2.0")

import matecomponent
import MateComponent
import MateComponent__POA
import CORBA, sys
import gc

gc.set_debug(gc.DEBUG_LEAK)

orb = CORBA.ORB_init(sys.argv)
matecomponent.activate()

class PersistStream(MateComponent__POA.PersistStream, matecomponent.UnknownBaseImpl):
    def __init__(self):
        matecomponent.UnknownBaseImpl.__init__(self)

    # implementations of methods/attributes of "Interface" go here

    def load(self, content_type):
	print "PersistStream::load(content_type='%s')" % content_type

    def save(self, content_type):
	print "PersistStream::save(content_type='%s')" % content_type


ps = PersistStream()

def foo(*args):
    print args

listener = matecomponent.Listener(foo)
listener.add_interface(ps.get_matecomponent_object())

del ps
## How is it that the PersistStream servant instance is kept alive, I
## hear you ask?
##     Well, there's a signal connection from the 'destroy' signal of
## the ForeignObject to a bound method of the servant instance.  A
## bound method object contains an implicit reference to the instance
## to which it is bound.  On the other hand, the servant contains a
## reference to the ForeignObject.  This way, these two objects keep
## each other alive with mutual references.
##     When the 'destroy' signal is fired (when matecomponent reference count
## drops to zero), the bound method in the servant is called, and it
## deletes the reference it has to the ForeignObject, thus destroying
## the signal connection (closure), consequently releasing the last
## reference to the servant.  At least that's my theory on how things
## work..


print "server-side boundary"
listener = listener.corba_objref() # from now on we work on client-side
gc.collect()
print "client-side"

print "query PersistStream"
ps = listener.queryInterface("IDL:MateComponent/PersistStream:1.0")
print ps
print "unref PersistStream"
if ps is not None:
    ps.unref()
gc.collect()

print ">>unref listener start"
# this should trigger final ref count -> destroy
listener.unref()
del listener
print "<<unref listener end"
gc.collect()

