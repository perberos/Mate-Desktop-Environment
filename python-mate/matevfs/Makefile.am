INCLUDES = $(PYTHON_INCLUDES) $(PYGTK_CFLAGS) -DPY_SSIZE_T_CLEAN


EXTRA_DIST =

pkgpythondir = $(pyexecdir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0
matevfsdir = $(pkgpyexecdir)/matevfs
matevfs_PYTHON = __init__.py

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

# libmatevfs binding
matevfs_LTLIBRARIES = _matevfs.la
_matevfs_la_CFLAGS = $(MATEVFS_CFLAGS)
_matevfs_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_matevfs
_matevfs_la_LIBADD = $(MATEVFS_LIBS) $(PYTHON_LIBS)
_matevfs_la_SOURCES = vfsmodule.c vfs-uri.c vfs-file-info.c vfs-dir-handle.c \
 vfs-handle.c pymatevfs.h pymatevfs-private.h vfs-xfer-progress-info.c \
 vfs-context.c vfs-async-handle.c vfs-volume.c vfs-drive.c vfs-volume-monitor.c

# libmatevfs binding
if BUILD_MATEVFSMATECOMPONENT
matevfs_LTLIBRARIES += matevfsmatecomponent.la
endif
matevfsmatecomponent_la_CFLAGS = $(MATEVFSMATECOMPONENT_CFLAGS)  -DMATE_VFS_INCLUDE_MATECOMPONENT
matevfsmatecomponent_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initmatevfsmatecomponent
matevfsmatecomponent_la_LIBADD = $(MATEVFSMATECOMPONENT_LIBS) $(PYTHON_LIBS)
matevfsmatecomponent_la_SOURCES = vfsmatecomponentmodule.c pymatevfsmatecomponent.h


gpincdir = $(includedir)/mate-python-2.0
gpinc_HEADERS = pymatevfs.h pymatevfsmatecomponent.h

## Python MateVFS modules
vfsmethoddir = $(libdir)/mate-vfs-2.0/modules
if BUILD_PYVFSMODULE
  vfsmethod_LTLIBRARIES = libpythonmethod.la
else
  vfsmethod_LTLIBRARIES =
endif

libpythonmethod_la_CFLAGS = $(MATE_VFS_MODULE_CFLAGS) -DMATE_VFS_PYTHON_DIR="\"$(vfsmethoddir)\""
libpythonmethod_la_SOURCES = mate-vfs-python-method.c
libpythonmethod_la_LDFLAGS = $(common_ldflags) -export-symbols-regex "vfs_module_(init|shutdown)"
libpythonmethod_la_LIBADD = $(MATE_VFS_MODULE_LIBS) $(PYTHON_EMBED_LIBS)

EXTRA_DIST += wscript
