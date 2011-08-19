NULL =

AM_CPPFLAGS = \
	-I$(top_srcdir)/common				\
	-DAUTHDIR=\""$(authdir)"\"		 	\
	-DDATADIR=\""$(datadir)"\"		 	\
	-DGDMCONFDIR=\"$(gdmconfdir)\"			\
	-DDMCONFDIR=\""$(dmconfdir)"\"		 	\
	-DGDMLOCALEDIR=\""$(gdmlocaledir)"\"		\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLIBEXECDIR=\""$(libexecdir)"\" 		\
	-DSBINDIR=\""$(sbindir)"\"		 	\
	-DPIXMAPDIR=\""$(pixmapdir)"\"		 	\
	-DAT_SPI_REGISTRYD_DIR="\"$(AT_SPI_REGISTRYD_DIR)\""	\
	$(GTK_CFLAGS)					\
	$(XLIB_CFLAGS)					\
	$(XDMCP_CFLAGS)					\
	$(EXTRA_CHOOSER_CFLAGS)				\
	$(SIMPLE_CHOOSER_CFLAGS)			\
	$(NULL)

libexec_PROGRAMS =			\
	gdm-simple-chooser		\
	gdm-host-chooser		\
	$(NULL)

gdm_simple_chooser_SOURCES =  		\
	chooser-main.c 			\
	gdm-chooser-host.c		\
	gdm-chooser-host.h		\
	gdm-host-chooser-widget.c	\
	gdm-host-chooser-widget.h	\
	gdm-host-chooser-dialog.c	\
	gdm-host-chooser-dialog.h	\
	gdm-chooser-client.c		\
	gdm-chooser-client.h		\
	gdm-chooser-session.c		\
	gdm-chooser-session.h		\
	$(NULL)

gdm_simple_chooser_LDADD = 		\
	$(top_builddir)/common/libgdmcommon.la	\
	$(GTK_LIBS)			\
	$(EXTRA_CHOOSER_LIBS)   	\
	$(SIMPLE_CHOOSER_LIBS)		\
	$(XLIB_LIBS)			\
	$(XDMCP_LIBS)			\
	$(NULL)

gdm_host_chooser_SOURCES = 		\
	gdm-host-chooser.c 		\
	gdm-chooser-host.c		\
	gdm-chooser-host.h		\
	gdm-host-chooser-widget.c	\
	gdm-host-chooser-widget.h	\
	gdm-host-chooser-dialog.c	\
	gdm-host-chooser-dialog.h	\
	$(NULL)

gdm_host_chooser_LDADD =		\
	$(top_builddir)/common/libgdmcommon.la	\
	$(GTK_LIBS)			\
	$(EXTRA_CHOOSER_LIBS)   	\
	$(SIMPLE_CHOOSER_LIBS)		\
	$(XLIB_LIBS)			\
	$(XDMCP_LIBS)			\
	$(NULL)

EXTRA_DIST = 			\
	$(NULL)
