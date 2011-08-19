noinst_LTLIBRARIES =					\
	libtotemscrsaver.la

common_defines = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATECONF_PREFIX=\""/apps/totem"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DBINDIR=\""$(bindir)"\"			\
	-DLOGO_PATH=DATADIR\"\"G_DIR_SEPARATOR_S\"totem\"G_DIR_SEPARATOR_S\"totem_logo.png\"	\
	$(DISABLE_DEPRECATED)

libtotemscrsaver_la_SOURCES =				\
	totem-scrsaver.c				\
	totem-scrsaver.h

libtotemscrsaver_la_CFLAGS =				\
	$(common_defines)				\
	$(DEPENDENCY_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(DBUS_CFLAGS)					\
	$(AM_CFLAGS)

libtotemscrsaver_la_LDFLAGS =				\
	$(AM_LDFLAGS)

libtotemscrsaver_la_LIBADD =				\
	$(DBUS_LIBS)					\
	$(XTEST_LIBS)

