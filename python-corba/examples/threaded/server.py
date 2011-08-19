#!/usr/bin/python

import threading
import time

import CORBA
import MateCORBA
MateCORBA.load_file('./pyt.idl')
import pyt__POA

class PyT (pyt__POA.PyT):
    def  __init__(s, i):
        s.i = i

    def op(s):
        for i in xrange(0, 100):
            print 'X'
            time.sleep(0.1)
        

def run():
    while True:
        print '.'
        time.sleep(0.1)

if __name__ == "__main__":
    orb = CORBA.ORB_init()

    pyt1 = PyT(1)
    pyt1_ref = pyt1._this()
    ior = orb.object_to_string(pyt1_ref)
    open('ior', 'w').write(ior)

    t1 = threading.Thread(target = run)
    t1.start()

    time.sleep(1)

    t2 = threading.Thread(target = orb.run)
    t2.start()

    #orb.shutdown()




