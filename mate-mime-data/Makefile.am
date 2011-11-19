SUBDIRS = man po

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = mate-mime-data-2.0.pc

NULL =

keys_in_files = mate-vfs.keys.in
keys_files = $(keys_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

mimedir = $(datadir)/mime-info
mime_DATA = $(keys_files) mate-vfs.mime

applicationsdir = $(datadir)/application-registry
applications_DATA = mate-vfs.applications

sysconf_DATA = mate-vfs-mime-magic

EXTRA_DIST = \
	autogen.sh \
	check-mime.pl \
	mate-mime-data.spec \
	mate-mime-data-zip.in \
	mate-mime-data-2.0.pc.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(keys_in_files) \
	$(mime_DATA) \
	$(sysconf_DATA) \
	$(applications_DATA)

CLEANFILES = $(keys_files)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update
