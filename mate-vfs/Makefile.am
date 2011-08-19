SUBDIRS_MATE=doc schemas

SUBDIRS =					\
	imported				\
	libmatevfs				\
	modules					\
	daemon					\
	schemas					\
	devel-docs				\
	doc					\
	programs				\
	test					\
	po

EXTRA_DIST =					\
	HACKING					\
	MAINTAINERS 				\
	TODO					\
	autogen.sh				\
	mate-vfs.spec				\
	mate-vfs-zip.in			\
	mate-vfs-2.0.pc.in			\
	mate-vfs-module-2.0.pc.in		\
	gtk-doc.make				\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in			\
	xdgmime.win32.diff

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-vfs-2.0.pc mate-vfs-module-2.0.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DISTCLEANFILES = intltool-extract intltool-merge intltool-update
