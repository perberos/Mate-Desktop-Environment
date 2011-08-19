import os
import sys

#if not os.environ.has_key('DIST_CHECK'):
#    sys.path = ['..', '../matevfs', '../matecanvas'] + sys.path


modules = [
    "matecomponent",
    "matevfs",
    "matecomponent.ui",
    "matecomponent",
    "mateconf",
    "mate",
    "mate.ui",
    "matecanvas",
]

def run_import_tests(builddir, no_import_hooks):
    if not no_import_hooks:
        import ltihooks
        ltihooks.install()

    for item in modules:
        if isinstance(item, tuple):
            module, dirname = item
        else:
            module = item
            dirname = item
        #sys.path.insert(0, os.path.join(builddir, dirname))
        print "Trying to import module %s... " % (module,),
        try:
            mod = __import__(module) # try to import the module to catch undefined symbols
        except ImportError, ex:
            if ex.args[0].startswith("No module named"):
                print "not found"
            else:
                raise
        else:
            print "ok."
            globals()[module] = mod

    if not no_import_hooks:
        ltihooks.uninstall()

