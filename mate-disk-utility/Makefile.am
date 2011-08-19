SUBDIRS = src data doc help po

EXTRA_DIST = \
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	mate-doc-utils.make	\
	TODO

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update

clean-local :
	rm -f *~ po/*~

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper
