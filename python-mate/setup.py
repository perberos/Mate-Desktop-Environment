#!/usr/bin/env python
#
# setup.py - distutils configuration for pygtk
#
# TODO:
# pygtk.spec(.in)
# win32 testing
# install *.pyc for codegen
#
"""Python Bindings for Mate.

TODO: Write a good long description"""

from distutils.command.build import build
from distutils.core import setup
import os
import sys

sys.stderr.write('''-- WARNING --
The distutils method (setup.py) of building mate-python is not
supported.  setup.py is *unmaintained* and therefore not up-to-date
with the latest developments.  Use at your own risk.
-- 
''')

try:
    import pygtk
    pygtk.require('2.0')
except (ImportError, AssertionError):
    raise SystemExit, 'ERROR: Could not find a recent version of pygtk.'

from dsextras import list_files, have_pkgconfig, GLOBAL_INC, GLOBAL_MACROS
from dsextras import InstallLib, BuildExt, PkgConfigExtension
from dsextras import Template, TemplateExtension, pkgc_version_check
from dsextras import getoutput

MAJOR_VERSION = 2
MINOR_VERSION = 5
MICRO_VERSION = 91

VERSION = "%d.%d.%d" % (MAJOR_VERSION,
                        MINOR_VERSION,
                        MICRO_VERSION)

PYGTK_REQUIRED_VERSION              = '2.0.0'
PYMATECORBA_REQUIRED_VERSION            = '2.0.0'
LIBMATE_REQUIRED_VERSION           = '2.0.0'
LIBMATEUI_REQUIRED_VERSION         = '2.0.0'
LIBMATECANVAS_REQUIRED_VERSION     = '2.0.0'
LIBMATEVFS_REQUIRED_VERSION        = '2.0.0'
LIBZVT_REQUIRED_VERSION             = '2.0.0'
MATECONF_REQUIRED_VERSION              = '1.2.0'
MATECOMPONENT_ACTIVATION_REQUIRED_VERSION  = '1.0.0'
LIBMATECOMPONENT_REQUIRED_VERSION          = '2.0.0'
LIBMATECOMPONENTUI_REQUIRED_VERSION        = '2.0.0'
LIBCAJA_REQUIRED_VERSION        = '2.0.0'
LIBPANELAPPLET_REQUIRED_VERSION     = '2.0.0'
GTKHTML2_REQUIRED_VERSION           = '2.0.0'
LIBMATEPRINT_REQUIRED_VERSION      = '2.2.0'
LIBMATEPRINTUI_REQUIRED_VERSION    = '2.2.0'

PYGTK_SUFFIX = '2.0'
PYGTK_SUFFIX_LONG = 'gtk-' + PYGTK_SUFFIX

GLOBAL_INC += ['mateconf', 'mate', 'mateprint']

DEFS_DIR    = os.path.join('share', 'pygtk', PYGTK_SUFFIX, 'defs')
CODEGEN_DIR = os.path.join('share', 'pygtk', PYGTK_SUFFIX, 'codegen')
INCLUDE_DIR = os.path.join('include', 'pygtk-%s' % PYGTK_SUFFIX)

str_version = sys.version[:3]
version = map(int, str_version.split('.'))
if version < [2, 2]:
    raise SystemExit, \
          "Python 2.2 or higher is required, %s found" % str_version

install_vfsmethod = 0
class MatePythonInstallLib(InstallLib):
    def run(self):
        self.add_template_option('VERSION', VERSION)
        self.prepare()

        self.install_template('mate-python-2.0.pc.in',
                              os.path.join(self.libdir, 'pkgconfig'))

        if install_vfsmethod:
            self.install_vfsmethod()
            
        # Modify the base installation dir
        install_dir = os.path.join(self.install_dir, PYGTK_SUFFIX_LONG)
        self.set_install_dir(install_dir)
                                          
        InstallLib.run(self)
        
    def install_vfsmethod(self):
        libdir = '/'.join(self.install_dir.split('/')[:-3])
        module_dir = os.path.join(libdir, 'mate-vfs-2.0', 'modules')
        module_file = os.path.join(self.build_dir, 'libpythonmethod.so')
        if (os.path.exists(module_file) and
            not os.path.exists(os.path.join(self.build_dir, module_file))):
            self.copy_file(module_file, module_dir)
        
if not pkgc_version_check('pygtk-2.0', 'PyGTK', PYGTK_REQUIRED_VERSION):
    raise SystemExit, "Aborting"
pygtkincludedir = getoutput('pkg-config --variable pygtkincludedir pygtk-2.0')
codegendir = getoutput('pkg-config --variable codegendir pygtk-2.0')
defsdir = getoutput('pkg-config --variable defsdir pygtk-2.0')

GLOBAL_INC.append(pygtkincludedir)
GTKDEFS = [os.path.join(defsdir, 'pango-types.defs'),
           os.path.join(defsdir, 'gdk-types.defs'),
           os.path.join(defsdir, 'gtk-types.defs')]
MATECOMPONENTDEFS = [os.path.join('matecomponent', 'matecomponent-types.defs'),
              os.path.join('matecomponent', 'matecomponentui-types.defs')]
CANVASDEFS = [os.path.join('mate', 'canvas.defs')]
sys.path.append(codegendir)
try:
    from override import Overrides
except ImportError:
    raise SystemExit, \
'Could not find code generator in %s, do you have installed pygtk correctly?'

have_pymatecorba = pkgc_version_check('pymatecorba-2', 'PyMateCORBA.',
                                 PYMATECORBA_REQUIRED_VERSION)
if have_pymatecorba:
    includedir = getoutput('pkg-config --variable includedir pymatecorba-2')
    GLOBAL_INC.append(os.path.join(includedir, 'pymatecorba-2')) 

libmate = TemplateExtension(name='mate', pkc_name='libmate-2.0',
                             pkc_version=LIBMATE_REQUIRED_VERSION,
                             output='mate._mate',
                             defs='mate/mate.defs',
                             sources=['mate/matemodule.c',
                                      'mate/mate.c'],
                             register=GTKDEFS+['mate/mate.defs'],
                             override='mate/mate.override')

libmateui = TemplateExtension(name='ui', pkc_name='libmateui-2.0',
                               pkc_version=LIBMATEUI_REQUIRED_VERSION,
                               output='mate.ui',
                               defs='mate/ui.defs',
                               sources=['mate/uimodule.c',
                                        'mate/ui.c'],
                               register=GTKDEFS+MATECOMPONENTDEFS + \
                                        ['mate/mate.defs',
                                         'mate/ui.defs'],
                               override='mate/ui.override')

libmatecanvas = TemplateExtension(name='canvas',
                                   pkc_name='libmatecanvas-2.0',
                                   pkc_version=LIBMATECANVAS_REQUIRED_VERSION,
                                   output='mate.canvas',
                                   defs='mate/canvas.defs',
                                   sources=['mate/canvasmodule.c',
                                            'mate/canvas.c'],
                                   register=['mate/canvas.defs'],
                                   override='mate/canvas.override')

libmatevfs = PkgConfigExtension(name='mate.vfs',
                                 pkc_name='mate-vfs-2.0',
                                 pkc_version=LIBMATEVFS_REQUIRED_VERSION,
                                 sources=['mate/vfs-dir-handle.c',
                                          'mate/vfs-file-info.c',
                                          'mate/vfs-handle.c',
                                          'mate/vfsmodule.c',
                                          'mate/vfs-uri.c',
                                          'mate/vfs-context.c'])

libzvt = TemplateExtension(name='zvt',
                           pkc_name='libzvt-2.0',
                           pkc_version=LIBZVT_REQUIRED_VERSION,
                           output='mate.zvt',
                           defs='mate/zvt.defs',
                           sources=['mate/zvtmodule.c',
                                    'mate/zvt.c'],
                           register=['mate/zvt.defs'],
                           override='mate/zvt.override')

mateconf = TemplateExtension(name='mateconf',
                          pkc_name='mateconf-2.0',
                          pkc_version=MATECONF_REQUIRED_VERSION,
                          output='mate.mateconf',
                          defs='mateconf/mateconf.defs',
                          sources=['mateconf/mateconfmodule.c',
                                   'mateconf/mateconf-fixes.c',
                                   'mateconf/mateconf.c'],
                          register=['mateconf/mateconf.defs'],
                          override='mateconf/mateconf.override')

matecomponent_activation = PkgConfigExtension(name='matecomponent.activation',
                           pkc_name='matecomponent-activation-2.0',
                           pkc_version=MATECOMPONENT_ACTIVATION_REQUIRED_VERSION,
                           sources=['matecomponent/activationmodule.c'])

libmatecomponent = TemplateExtension(name='matecomponent',
                              pkc_name='libmatecomponent-2.0',
                              pkc_version=LIBMATECOMPONENT_REQUIRED_VERSION,
                              output='matecomponent._matecomponent',
                              defs='matecomponent/matecomponent.defs',
                              sources=['matecomponent/matecomponentmodule.c',
                                       'matecomponent/matecomponent-arg.c',
                                       'matecomponent/matecomponent.c'],
                              register=MATECOMPONENTDEFS + \
                                       ['matecomponent/matecomponentui.defs'],
                              load_types='matecomponent/matecomponent-arg-types.py',
                              override='matecomponent/matecomponent.override')

libmatecomponentui = TemplateExtension(name='matecomponentui',
                                pkc_name='libmatecomponentui-2.0',
                                pkc_version=LIBMATECOMPONENTUI_REQUIRED_VERSION,
                                output='matecomponent.ui',
                                defs='matecomponent/matecomponentui.defs',
                                sources=['matecomponent/matecomponentuimodule.c',
                                         'matecomponent/matecomponentui.c'],
                                register=GTKDEFS + CANVASDEFS + \
                                         MATECOMPONENTDEFS + \
                                         ['matecomponent/matecomponentui.defs'],
                                load_types='matecomponent/matecomponent-arg-types.py',
                                override='matecomponent/matecomponentui.override')

libcaja = TemplateExtension(name='caja',
                                pkc_name='libcaja',
                                pkc_version=LIBCAJA_REQUIRED_VERSION,
                                output='mate.caja',
                                defs='mate/caja.defs',
                                sources=['mate/cajamodule.c',
                                         'mate/caja.c'],
                                register=['mate/caja.defs'],
                                override='mate/caja.override')

libmatepanelapplet = TemplateExtension(name='applet',
                                pkc_name='libmatepanelapplet-2.0',
                                pkc_version=LIBPANELAPPLET_REQUIRED_VERSION,
                                output='mate.applet',
                                defs='mate/applet.defs',
                                sources=['mate/appletmodule.c',
                                         'mate/applet.c'],
                                register=['mate/applet.defs'],
                                override='mate/applet.override')

gtkhtml2 = TemplateExtension(name='gtkhtml2',
                           pkc_name='libgtkhtml-2.0',
                           pkc_version=GTKHTML2_REQUIRED_VERSION,
                           output='gtkhtml2',
                           defs='gtkhtml2/gtkhtml2.defs',
                           sources=['gtkhtml2/gtkhtml2module.c',
                                    'gtkhtml2/gtkhtml2.c'],
                           register=['gtkhtml2/gtkhtml2.defs'],
                           override='gtkhtml2/gtkhtml2.override')

libmateprint = TemplateExtension(name='print',
                           pkc_name='libmateprint-2.2',
                           pkc_version=LIBMATEPRINT_REQUIRED_VERSION,
                           output='mateprint._print',
                           defs='mateprint/print.defs',
                           sources=['mateprint/printmodule.c',
                                    'mateprint/print.c'],
                           register=GTKDEFS + ['mateprint/print.defs'],
                           override='mateprint/print.override')

libmateprintui = TemplateExtension(name='printui',
                           pkc_name='libmateprintui-2.2',
                           pkc_version=LIBMATEPRINTUI_REQUIRED_VERSION,
                           output='mateprint.printui',
                           defs='mateprint/printui.defs',
                           sources=['mateprint/printuimodule.c',
                                    'mateprint/printui.c'],
                           register=GTKDEFS + CANVASDEFS + \
                                    ['mateprint/print.defs',
                                     'mateprint/printui.defs'],
                           override='mateprint/printui.override')

data_files = []
ext_modules = []
py_modules = []
if libmate.can_build() or libmatecanvas.can_build():
    py_modules.append('mate.__init__')    
if libmate.can_build():
    ext_modules.append(libmate)
    data_files.append((DEFS_DIR, ('mate/mate.defs',)))
if have_pymatecorba and libmateui.can_build():
    ext_modules.append(libmateui)
    data_files.append((DEFS_DIR, ('mate/ui.defs',)))    
if libmatecanvas.can_build():
    ext_modules.append(libmatecanvas)
    data_files.append((DEFS_DIR, ('mate/canvas.defs',)))    
if libmatevfs.can_build():
    #libdir = getoutput('pkg-config --variable=libdir mate-vfs-module-2.0')
    #pythondir = os.path.join(libdir, 'mate-vfs-2.0', 'modules', 'python')
    #GLOBAL_MACROS.append(('MATE_VFS_PYTHON_DIR', '"%s"' % pythondir))
    #libpythonmethod = PkgConfigExtension(name='libpythonmethod',
    #                   pkc_name='mate-vfs-module-2.0',
    #                   pkc_version=LIBMATEVFS_REQUIRED_VERSION,
    #                   sources=['mate/mate-vfs-python-method.c'])
    #if libpythonmethod.can_build():
    #    install_vfsmethod = 1
    #    ext_modules.append(libpythonmethod)
        
    ext_modules.append(libmatevfs)
if libzvt.can_build():
    ext_modules.append(libzvt)
    data_files.append((DEFS_DIR, ('mate/zvt.defs',)))
if mateconf.can_build():
    ext_modules.append(mateconf)
    data_files.append((DEFS_DIR, ('mateconf/mateconf.defs',)))
if have_pymatecorba and matecomponent_activation.can_build():
    ext_modules.append(matecomponent_activation)
if have_pymatecorba and libmatecomponent.can_build():
    ext_modules.append(libmatecomponent)
    data_files.append((DEFS_DIR, ('matecomponent/matecomponent.defs',
                                  'matecomponent/matecomponent-types.defs')))
if have_pymatecorba and libmatecomponentui.can_build():
    ext_modules.append(libmatecomponentui)
    data_files.append((DEFS_DIR, ('matecomponent/matecomponentui.defs',
                                  'matecomponent/matecomponentui-types.defs')))
    if libcaja.can_build():
        ext_modules.append(libcaja)
        data_files.append((DEFS_DIR, ('mate/caja.defs',)))
if libmatepanelapplet.can_build():
    ext_modules.append(libmatepanelapplet)
    data_files.append((DEFS_DIR, ('mate/applet.defs',)))
if gtkhtml2.can_build():
    ext_modules.append(gtkhtml2)
    data_files.append((DEFS_DIR, ('gtkhtml2/gtkhtml2.defs',)))
if libmateprint.can_build():
    ext_modules.append(libmateprint)
    data_files.append((DEFS_DIR, ('mateprint/print.defs',)))
    py_modules.append('mateprint.__init__')
if libmateprintui.can_build():
    ext_modules.append(libmateprintui)
    data_files.append((DEFS_DIR, ('mateprint/printui.defs',)))

# Install matecomponent/__init__.py
if have_pymatecorba and (matecomponent_activation.can_build() or
                     libmatecomponent.can_build() or
                     libmatecomponentui.can_build()):
    py_modules.append('matecomponent.__init__')

doclines = __doc__.split("\n")

setup(name="mate-python",
      url='http://www.daa.com.au/~james/mate/',
      version=VERSION,
      license='LGPL',
      platforms=['yes'],
      maintainer="James Henstridge",
      maintainer_email="james@daa.com.au",
      description = doclines[0],
      long_description = "\n".join(doclines[2:]),
      py_modules=py_modules,
      ext_modules=ext_modules,
      data_files=data_files,
      cmdclass={'install_lib': MatePythonInstallLib,
                'build_ext': BuildExt })
