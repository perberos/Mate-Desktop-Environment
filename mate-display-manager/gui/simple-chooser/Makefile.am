NULL =

AM_CPPFLAGS = \
	-I$(top_srcdir)/common				\
	-DAUTHDIR=\""$(authdir)"\"		 	\
	-DDATADIR=\""$(datadir)"\"		 	\
	-DMDMCONFDIR=\"$(mdmconfdir)\"			\
	-DDMCONFDIR=\""$(dmconfdir)"\"		 	\
	-DMDMLOCALEDIR=\""$(mdmlocaledir)"\"		\
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
	mdm-simple-chooser		\
	mdm-host-chooser		\
	$(NULL)

mdm_simple_chooser_SOURCES =  		\
	chooser-main.c 			\
	mdm-chooser-host.c		\
	mdm-chooser-host.h		\
	mdm-host-chooser-widget.c	\
	mdm-host-chooser-widget.h	\
	mdm-host-chooser-dialog.c	\
	mdm-host-chooser-dialog.h	\
	mdm-chooser-client.c		\
	mdm-chooser-client.h		\
	mdm-chooser-session.c		\
	mdm-chooser-session.h		\
	$(NULL)

mdm_simple_chooser_LDADD = 		\
	$(top_builddir)/common/libmdmcommon.la	\
	$(GTK_LIBS)			\
	$(EXTRA_CHOOSER_LIBS)   	\
	$(SIMPLE_CHOOSER_LIBS)		\
	$(XLIB_LIBS)			\
	$(XDMCP_LIBS)			\
	$(NULL)

mdm_host_chooser_SOURCES = 		\
	mdm-host-chooser.c 		\
	mdm-chooser-host.c		\
	mdm-chooser-host.h		\
	mdm-host-chooser-widget.c	\
	mdm-host-chooser-widget.h	\
	mdm-host-chooser-dialog.c	\
	mdm-host-chooser-dialog.h	\
	$(NULL)

mdm_host_chooser_LDADD =		\
	$(top_builddir)/common/libmdmcommon.la	\
	$(GTK_LIBS)			\
	$(EXTRA_CHOOSER_LIBS)   	\
	$(SIMPLE_CHOOSER_LIBS)		\
	$(XLIB_LIBS)			\
	$(XDMCP_LIBS)			\
	$(NULL)

EXTRA_DIST = 			\
	$(NULL)
