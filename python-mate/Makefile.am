AUTOMAKE_OPTIONS = 1.6

SUBDIRS = 

if BUILD_DOCS
SUBDIRS += docs
endif

if BUILD_MATE
SUBDIRS += mate
endif

if BUILD_MATECOMPONENT_ANY
SUBDIRS += matecomponent
endif

if BUILD_MATECONF
SUBDIRS += mateconf
endif

if BUILD_MATEVFS
SUBDIRS += matevfs
endif

if BUILD_MATECANVAS
SUBDIRS += matecanvas
endif

SUBDIRS += tests



pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-python-2.0.pc

EXTRA_DIST = \
  INSTALL.WAF \
  waf \
  wscript \
  mate-python.spec \
  mate-python.spec.in \
  mate-python-2.0.pc.in \
  PKG-INFO \
  PKG-INFO.in \
  ChangeLog.pre-2-0 \
  setup.py \
  examples/bug-buddy-integration.py \
  examples/matecomponent/matecomponentui/MateComponent_Sample_Hello.xml \
  examples/matecomponent/matecomponentui/hello.py \
  examples/matecomponent/echo/README \
  examples/matecomponent/echo/echo-client.py \
  examples/matecomponent/unknown.py \
  examples/matecomponent/matecomponent-application.py \
  examples/canvas/canvas-example.py \
  examples/canvas/canvas-curve.py \
  examples/mateconf/basic-mateconf-app.py \
  examples/mateconf/simple-controller.py \
  examples/mateconf/simple-view.py \
  examples/vfs/shell.py \
  examples/vfs/sync-xfer.py \
  examples/vfs/async-xfer.py \
  examples/vfs/async.py \
  examples/vfs/cancellation.py \
  examples/vfs/pygvfsmethod/README \
  examples/vfs/pygvfsmethod/pyfs.conf \
  examples/vfs/pygvfsmethod/pyfs.py \
  examples/popt/popt.py \
  examples/popt/goption.py


$(srcdir)/ChangeLog:
	-rm -f $(srcdir)/ChangeLog
	$(srcdir)/generate-ChangeLog

dist-hook: dist-hook-local $(srcdir)/ChangeLog

.PHONY : dist-hook-local
