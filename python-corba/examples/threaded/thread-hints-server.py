#!/usr/bin/python

import threading
import time

import MateCORBA
import CORBA
import PortableServer
MateCORBA.load_file('./pyt.idl')
import pyt__POA

class PyT (pyt__POA.PyT):
    def  __init__(self, num):
        self.num = num
        self.level = 0
        self._level_lock = threading.Lock()

    def _default_POA(self):
        return threaded_poa

    def op(self):

        self._level_lock.acquire()
        self.level += 1
        level = self.level
        self._level_lock.release()
        
        for i in xrange(0, 20):
            print 'X', self.num, level
            time.sleep(0.2)

        self._level_lock.acquire()
        self.level -= 1
        level = self.level
        self._level_lock.release()

        return 1

def run():
    while True:
        print '.'
        time.sleep(2)

_objects = dict()

if __name__ == "__main__":
    orb = CORBA.ORB_init(orb_id="matecorba-io-thread")
    rootpoa = orb.resolve_initial_references("RootPOA")
    thread_policy = rootpoa.create_thread_policy(PortableServer.ORB_CTRL_MODEL);
    global threaded_poa
    threaded_poa = rootpoa.create_POA("ThreadedPOA", rootpoa.the_POAManager, [thread_policy])
    threaded_poa.set_thread_hint(MateCORBA.THREAD_HINT_PER_REQUEST)

    t1 = threading.Thread(target=run)
    t1.start()

    for num in range(1, 3):
        pyt1 = PyT(num)
        _objects[num] = pyt1 # stash object in a dict to keep it alive
        pyt1_ref = pyt1._this()
        ior = orb.object_to_string(pyt1_ref)
        open('ior%i' % num, 'w').write(ior)

    time.sleep(1)
    orb.run()
    orb.shutdown()
