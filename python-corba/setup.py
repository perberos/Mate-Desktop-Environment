#!/usr/bin/env python
"""Python language binding for the MateCORBA2 CORBA implementation.

PyMateCORBA aims to take advantage of new features found in MateCORBA2 to make
language bindings more efficient.  This includes:
  - Use of MateCORBA2 type libraries to generate stubs
  - use of the MateCORBA_small_invoke_stub() call for operation
    invocation, which allows for short circuited invocation on local
    objects.
"""

from commands import getoutput
from distutils.core import setup
from distutils.extension import Extension
import os

from dsextras import have_pkgconfig, GLOBAL_MACROS
from dsextras import InstallLib, PkgConfigExtension

MAJOR_VERSION = 2
MINOR_VERSION = 0
MICRO_VERSION = 0

VERSION = "%d.%d.%d" % (MAJOR_VERSION,
                        MINOR_VERSION,
                        MICRO_VERSION)

MATECORBA2_REQUIRED  = '2.4.4'

GLOBAL_MACROS.append(('MATECORBA2_STUBS_API', 1))

class PyMateCORBAInstallLib(InstallLib):
    def run(self):
        self.add_template_option('MATECORBA2_REQUIRED_VERSION', MATECORBA2_REQUIRED)
        self.prepare()

        self.install_template('pymatecorba-2.pc.in',
                              os.path.join(self.libdir, 'pkgconfig'))
        InstallLib.run(self)
        
matecorba = PkgConfigExtension(name='MateCORBA',
                     pkc_name='MateCORBA-2.0',
                     pkc_version=MATECORBA2_REQUIRED,
                     sources=['src/MateCORBAmodule.c',
                              'src/pycorba-typecode.c',
                              'src/pycorba-object.c',
                              'src/pycorba-method.c',
                              'src/pycorba-marshal.c',
                              'src/pycorba-orb.c',
                              'src/pycorba-any.c',
                              'src/pycorba-exceptions.c',
                              'src/pycorba-struct.c',
                              'src/pycorba-enum.c',
                              'src/pycorba-fixed.c',
                              'src/stub-gen.c',
                              'src/pymatecorba-servant.c',
                              'src/pymatecorba-poa.c',
                              'src/pymatecorba-utils.c'])

if not matecorba.can_build():
    raise SystemExit

doclines = __doc__.split("\n")

setup(name="pymatecorba",
      version=VERSION,
      license='LGPL',
      platforms=['yes'],
      maintainer="James Henstridge",
      maintainer_email="james@daa.com.au",
      description = doclines[0],
      long_description = "\n".join(doclines[2:]),
      py_modules=['CORBA', 'PortableServer'],
      ext_modules=[matecorba],
      data_files=[('include/pymatecorba-2.0', ['src/pymatecorba.h'])],
      cmdclass={'install_lib': PyMateCORBAInstallLib})
