import MateCORBA
MateCORBA.load_file('./pyt.idl')
import CORBA
orb = CORBA.ORB_init()
pyt = orb.string_to_object(open('ior').read())
pyt.op()
