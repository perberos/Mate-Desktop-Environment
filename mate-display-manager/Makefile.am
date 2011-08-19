NULL =

SUBDIRS = 			\
	data			\
	common 			\
	daemon 			\
	docs			\
	gui 			\
	utils 			\
	po 			\
	tests 			\
	$(NULL)

# add these when help gets added back
# omf-install

EXTRA_DIST = \
	MAINTAINERS		\
	ChangeLog		\
	README			\
	mate-doc-utils.make	\
	xmldocs.make		\
	omf.make		\
	$(NULL)

DISTCLEANFILES = \
	mate-doc-utils.make	\
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'
