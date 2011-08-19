
NULL =

gdu-gtk-enumtypes.h: gdu-gtk-enums.h gdu-gtk-enumtypes.h.template
	( top_builddir=`cd $(top_builddir) && pwd`; \
	 cd $(srcdir) && glib-mkenums --template gdu-gtk-enumtypes.h.template gdu-gtk-enums.h ) > \
	   gdu-gtk-enumtypes.h.tmp && mv gdu-gtk-enumtypes.h.tmp gdu-gtk-enumtypes.h

gdu-gtk-enumtypes.c: gdu-gtk-enums.h gdu-gtk-enumtypes.c.template
	( top_builddir=`cd $(top_builddir) && pwd`; \
	 cd $(srcdir) && glib-mkenums --template gdu-gtk-enumtypes.c.template gdu-gtk-enums.h ) > \
	   gdu-gtk-enumtypes.c.tmp && mv gdu-gtk-enumtypes.c.tmp gdu-gtk-enumtypes.c

BUILT_SOURCES =            	\
	gdu-gtk-enumtypes.c     \
	gdu-gtk-enumtypes.h     \
	$(NULL)

EXTRA_DIST =                         	\
	gdu-gtk-enumtypes.h.template    \
	gdu-gtk-enumtypes.c.template    \
	$(NULL)

lib_LTLIBRARIES=libgdu-gtk.la

libgdu_gtkincludedir=$(includedir)/mate-disk-utility/gdu-gtk

libgdu_gtkinclude_HEADERS =              				\
	gdu-gtk.h							\
	gdu-gtk-types.h							\
	gdu-gtk-enums.h							\
	gdu-gtk-enumtypes.h						\
	gdu-time-label.h						\
	gdu-pool-tree-view.h						\
	gdu-pool-tree-model.h						\
	gdu-size-widget.h						\
	gdu-create-linux-md-dialog.h					\
	gdu-ata-smart-dialog.h						\
	gdu-spinner.h							\
	gdu-volume-grid.h						\
	gdu-details-table.h						\
	gdu-details-element.h						\
	gdu-error-dialog.h						\
	gdu-confirmation-dialog.h					\
	gdu-button-element.h						\
	gdu-button-table.h						\
	gdu-dialog.h							\
	gdu-edit-partition-dialog.h					\
	gdu-format-dialog.h						\
	gdu-partition-dialog.h						\
	gdu-create-partition-dialog.h					\
	gdu-create-linux-lvm2-volume-dialog.h				\
	gdu-edit-name-dialog.h						\
	gdu-disk-selection-widget.h					\
	gdu-add-component-linux-md-dialog.h				\
	gdu-add-pv-linux-lvm2-dialog.h				\
	gdu-edit-linux-md-dialog.h					\
	gdu-edit-linux-lvm2-dialog.h					\
	gdu-drive-benchmark-dialog.h					\
	gdu-connect-to-server-dialog.h					\
	$(NULL)

libgdu_gtk_la_SOURCES =                 	               				\
	gdu-gtk.h				gdu-gtk.c				\
	gdu-gtk-enums.h									\
	gdu-gtk-enumtypes.h			gdu-gtk-enumtypes.c			\
	gdu-gtk-types.h									\
	gdu-time-label.h			gdu-time-label.c			\
	gdu-pool-tree-view.h			gdu-pool-tree-view.c			\
	gdu-pool-tree-model.h			gdu-pool-tree-model.c			\
	gdu-size-widget.h			gdu-size-widget.c			\
	gdu-create-linux-md-dialog.h		gdu-create-linux-md-dialog.c		\
	gdu-ata-smart-dialog.h			gdu-ata-smart-dialog.c			\
	gdu-spinner.h				gdu-spinner.c				\
	gdu-volume-grid.h			gdu-volume-grid.c			\
	gdu-details-table.h			gdu-details-table.c			\
	gdu-details-element.h			gdu-details-element.c			\
	gdu-error-dialog.h			gdu-error-dialog.c			\
	gdu-confirmation-dialog.h		gdu-confirmation-dialog.c		\
	gdu-button-element.h			gdu-button-element.c			\
	gdu-button-table.h			gdu-button-table.c			\
	gdu-dialog.h				gdu-dialog.c				\
	gdu-edit-partition-dialog.h		gdu-edit-partition-dialog.c		\
	gdu-format-dialog.h			gdu-format-dialog.c			\
	gdu-partition-dialog.h			gdu-partition-dialog.c			\
	gdu-create-partition-dialog.h		gdu-create-partition-dialog.c		\
	gdu-create-linux-lvm2-volume-dialog.h	gdu-create-linux-lvm2-volume-dialog.c	\
	gdu-edit-name-dialog.h			gdu-edit-name-dialog.c			\
	gdu-disk-selection-widget.h		gdu-disk-selection-widget.c		\
	gdu-add-component-linux-md-dialog.h	gdu-add-component-linux-md-dialog.c	\
	gdu-add-pv-linux-lvm2-dialog.h		gdu-add-pv-linux-lvm2-dialog.c		\
	gdu-edit-linux-md-dialog.h		gdu-edit-linux-md-dialog.c		\
	gdu-edit-linux-lvm2-dialog.h		gdu-edit-linux-lvm2-dialog.c		\
	gdu-drive-benchmark-dialog.h		gdu-drive-benchmark-dialog.c		\
	gdu-connect-to-server-dialog.h		gdu-connect-to-server-dialog.c		\
	$(NULL)

libgdu_gtk_la_CPPFLAGS = 				\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	-DG_LOG_DOMAIN=\"libgdu-gtk\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)					\
	-DGDU_API_IS_SUBJECT_TO_CHANGE			\
	-DGDU_GTK_API_IS_SUBJECT_TO_CHANGE		\
	-DGDU_GTK_COMPILATION

libgdu_gtk_la_CFLAGS = 					\
	$(GLIB2_CFLAGS)					\
	$(GOBJECT2_CFLAGS)				\
	$(GIO2_CFLAGS)					\
	$(GIO_UNIX2_CFLAGS)				\
	$(DBUS_GLIB_CFLAGS)				\
	$(MATE_KEYRING_CFLAGS)				\
	$(GTK2_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)					\
	$(LIBATASMART_CFLAGS)				\
	$(AVAHI_UI_CFLAGS)				\
	$(NULL)

libgdu_gtk_la_LIBADD = 					\
	$(GLIB2_LIBS)					\
	$(GIO2_LIBS)					\
	$(GIO_UNIX2_LIBS)				\
	$(DBUS_GLIB_LIBS)				\
	$(MATE_KEYRING_LIBS)				\
	$(GTK2_LIBS)					\
	$(INTLLIBS)					\
	$(LIBATASMART_LIBS)				\
	$(AVAHI_UI_LIBS)				\
	$(NULL)

libgdu_gtk_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		        -export-dynamic -no-undefined -export-symbols-regex '(^gdu_.*)'

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdu-gtk.pc

CLEANFILES = $(BUILT_SOURCES) $(pkgconfig_DATA)

EXTRADIST =				\
	gdu-gtk-enumtypes.h.template	\
	gdu-gtk-enumtypes.c.template	\
	$(NULL)

clean-local :
	rm -f *~ $(BUILT_SOURCES)

