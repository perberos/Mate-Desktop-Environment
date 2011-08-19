'''Simple test case that jams everything_server and everything_client together
so that they run in the same address space.'''

import sys, unittest
import MateCORBA, CORBA

import everything_server

orb = CORBA.ORB_init(sys.argv)
factory_servant = everything_server.MyFactory()
factory_objref  = factory_servant._this()
factory_servant._default_POA().the_POAManager.activate()

import everything_client
everything_client.factory = factory_objref

EverythingTestCase = everything_client.EverythingTestCase

unittest.main()
