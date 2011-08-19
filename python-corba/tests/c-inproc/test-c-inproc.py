import sys
import MateCORBA

sys.path.append('.libs')
MateCORBA.load_typelib('./TestCall_module')

import CORBA, PyMateCORBA, PyMateCORBA__POA
import cTestCall

class PyTestCall(PyMateCORBA__POA.TestCall):
    def op1(self, other):
        this = self._this()
        print "  Python impl of TestCall.op1 invoked"
        other.op2(this)
    def op2(self, other):
        print "  Python impl of TestCall.op2 invoked"
        other.op3()
    def op3(self):
        this = self._this()
        print "  Python impl of TestCall.op3 invoked"

orb = CORBA.ORB_init(sys.argv)
poa = orb.resolve_initial_references('RootPOA')

# create the C objref
c_objref = cTestCall.create_TestCall(orb)

# create the python objref
py_servant = PyTestCall()
py_objref = py_servant._this()

# create a python servant that delegates to a C object reference :)
py_delegate_servant = PyMateCORBA__POA.TestCall(c_objref)
py_delegate_objref = py_delegate_servant._this()

print "Calling py_objref.op1(py_objref)"
py_objref.op1(py_objref)
print

print "Calling c_objref.op1(c_objref)"
c_objref.op1(c_objref)
print

print "Calling py_objref.op1(c_objref)"
py_objref.op1(c_objref)
print

print "Calling c_objref.op1(py_objref)"
c_objref.op1(py_objref)
print

print "Calling py_delegate_objref.op1(py_objref)"
py_delegate_objref.op1(py_objref)
print
