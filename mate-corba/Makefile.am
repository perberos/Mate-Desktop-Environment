DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

SUBDIRS=linc2 src include test docs

bin_SCRIPTS=matecorba2-config

EXTRA_DIST =		\
	MateCORBA2.m4	\
	MateCORBA.spec	\
	matecorba2-zip.in	\
	HACKING		\
	MAINTAINERS	\
	README.win32	\
	Makefile.shared \
	gtk-doc.make	\
	MateCORBA2.doap

rpms: distcheck
	rpm -ta $(top_builddir)/@PACKAGE@-@VERSION@.tar.gz

m4datadir=$(datadir)/aclocal
m4data_DATA=MateCORBA2.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	MateCORBA-2.0.pc \
	MateCORBA-CosNaming-2.0.pc \
	MateCORBA-imodule-2.0.pc
