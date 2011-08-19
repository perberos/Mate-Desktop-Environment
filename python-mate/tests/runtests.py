#!/usr/bin/env python
import glob
import os
import sys
import unittest

#SKIP_FILES = ['common', 'runtests']

if len(sys.argv) > 1:
    builddir = sys.argv[1]
    no_import_hooks = True
else:
    builddir = '..'
    no_import_hooks = False

import common
common.run_import_tests(builddir, no_import_hooks)


dir = os.path.split(os.path.abspath(__file__))[0]
os.chdir(dir)

def gettestnames():
    files = glob.glob('test*.py')
    names = map(lambda x: x[:-3], files)
    #map(names.remove, SKIP_FILES)
    return names
        
suite = unittest.TestSuite()
loader = unittest.TestLoader()

for name in gettestnames():
    suite.addTest(loader.loadTestsFromName(name))
    
testRunner = unittest.TextTestRunner()
testRunner.run(suite)
