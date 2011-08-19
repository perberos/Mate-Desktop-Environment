NULL =

AM_CPPFLAGS = \
	-I.						\
	-I..						\
	-DLOCALSTATEDIR=\""$(localstatedir)"\" 		\
	-DGDM_SCREENSHOT_DIR=\""$(GDM_SCREENSHOT_DIR)"\"\
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
	-e 's|@GDM_PID_FILE[@]|$(GDM_PID_FILE)|g'

bin_PROGRAMS = \
	gdmflexiserver		\
	gdm-screenshot		\
	$(NULL)

gdmflexiserver_SOURCES =	\
	gdmflexiserver.c	\
	$(NULL)

gdmflexiserver_LDADD =		\
	$(GTK_LIBS)		\
	$(COMMON_LIBS)		\
	$(NULL)

gdm_screenshot_SOURCES =	\
	gdm-screenshot.c	\
	$(NULL)

gdm_screenshot_LDADD =		\
	$(GTK_LIBS)		\
	$(CANBERRA_GTK_LIBS)	\
	$(XLIB_LIBS)		\
	$(COMMON_LIBS)		\
	$(NULL)

CLEANFILES = 			\
	$(NULL)

DISTCLEANFILES = 		\
	$(NULL)

