Utilitiesdir = $(datadir)/applications

SUBDIRS = data help libmateui-deprecated

INCLUDES = 	\
	$(MATE_UTILS_CFLAGS)					\
	$(GIO_UNIX_CFLAGS)					\
	-I$(srcdir)/libmateui-deprecated			\
	-I$(top_srcdir)/libeggsmclient			\
	-DG_DISABLE_DEPRECATED					\
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"			\
	-DDATADIR=\""$(datadir)"\"				\
	-DGREP_COMMAND=\""$(GREP_COMMAND)"\"

libmateui_deprecated_LIB = $(builddir)/libmateui-deprecated/libmateui-deprecated.la
libeggsmclient_LIB = $(top_builddir)/libeggsmclient/libeggsmclient.la

bin_PROGRAMS = mate-search-tool

schemadir = $(MATECONF_SCHEMA_FILE_DIR)
schema_in_file = mate-search-tool.schemas.in
schema_DATA = $(schema_in_file:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

mate_search_tool_SOURCES =     \
	gsearchtool-support.c   \
	gsearchtool-support.h   \
	gsearchtool-callbacks.c \
	gsearchtool-callbacks.h \
	gsearchtool.c	        \
	gsearchtool.h

mate_search_tool_LDADD =            \
	$(MATE_UTILS_LIBS)          \
	$(GIO_UNIX_LIBS)             \
	$(libmateui_deprecated_LIB) \
	$(libeggsmclient_LIB)

man_MANS = mate-search-tool.1

Utilities_in_files = mate-search-tool.desktop.in 
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(Utilities_in_files) \
	$(Utilities_DATA) \
	$(schema_in_file) \
	$(man_MANS)

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/gsearchtool/$(schema_DATA) ; \
	fi
else
install-data-local:
endif

CLEANFILES = 				\
	mate-search-tool.desktop	\
	$(schema_DATA)

disthook:
	cd $(distdir) ; rm -f $(CLEANFILES)
