NULL =

AM_CPPFLAGS = \
	-I.						\
	-I..						\
	-DLOCALSTATEDIR=\""$(localstatedir)"\" 		\
	-DMDM_SCREENSHOT_DIR=\""$(MDM_SCREENSHOT_DIR)"\"\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	$(UTILS_CFLAGS)					\
	$(CANBERRA_GTK_CFLAGS)				\
	$(GTK_CFLAGS)					\
	$(XLIB_CFLAGS)					\
	$(COMMON_CFLAGS)				\
	$(NULL)

edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@MDM_PID_FILE[@]|$(MDM_PID_FILE)|g'

bin_PROGRAMS = \
	mdmflexiserver		\
	mdm-screenshot		\
	$(NULL)

mdmflexiserver_SOURCES =	\
	mdmflexiserver.c	\
	$(NULL)

mdmflexiserver_LDADD =		\
	$(GTK_LIBS)		\
	$(COMMON_LIBS)		\
	$(NULL)

mdm_screenshot_SOURCES =	\
	mdm-screenshot.c	\
	$(NULL)

mdm_screenshot_LDADD =		\
	$(GTK_LIBS)		\
	$(CANBERRA_GTK_LIBS)	\
	$(XLIB_LIBS)		\
	$(COMMON_LIBS)		\
	$(NULL)

CLEANFILES = 			\
	$(NULL)

DISTCLEANFILES = 		\
	$(NULL)

