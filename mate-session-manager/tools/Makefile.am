bin_PROGRAMS = mate-session-save

AM_CPPFLAGS =					\
	$(MATE_SESSION_CFLAGS)			\
	$(DBUS_GLIB_CFLAGS)			\
	$(MATECONF_FLAGS)				\
	-DLOCALE_DIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

mate_session_save_SOURCES =			\
	mate-session-save.c

mate_session_save_LDADD =			\
	$(SM_LIBS)				\
	$(ICE_LIBS)				\
	$(MATE_SESSION_LIBS)			\
	$(DBUS_GLIB_LIBS)			\
	$(MATECONF_LIBS)

-include $(top_srcdir)/git.mk
