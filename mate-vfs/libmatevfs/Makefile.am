NULL =

if OS_WIN32
FNMATCH_CFLAGS = -I$(top_srcdir)/imported/fnmatch
FNMATCH_LIBS = ../imported/fnmatch/libfnmatch.la
endif

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(LIBMATEVFS_CFLAGS)			\
	$(LIBMATEVFSDAEMON_CFLAGS)		\
	$(VFS_CFLAGS)				\
	$(FNMATCH_CFLAGS)			\
	-DXDG_PREFIX=_mate_vfs_xdg		\
	-DDBUS_API_SUBJECT_TO_CHANGE		\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE                   \
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	-DMATE_VFS_PREFIX=\"$(prefix)\"	\
	-DMATE_VFS_BINDIR=\"$(bindir)\"	\
	-DMATE_VFS_DATADIR=\"$(datadir)\"	\
	-DMATE_VFS_LIBDIR=\"$(libdir)\"	\
	-DMATE_VFS_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" \
	-DMATE_VFS_SYSCONFDIR=\"$(sysconfdir)\"\
	-DG_LOG_DOMAIN=\"libmatevfs\"		\
	$(NULL)

libmatevfs_headers_to_scan_for_enums = 	\
	mate-vfs-address.h			\
	mate-vfs-application-registry.h	\
	mate-vfs-async-ops.h			\
	mate-vfs-cancellation.h		\
	mate-vfs-context.h			\
	mate-vfs-directory.h			\
	mate-vfs-drive.h			\
	mate-vfs-file-info.h			\
	mate-vfs-file-size.h			\
	mate-vfs-find-directory.h		\
	mate-vfs-handle.h			\
	mate-vfs-init.h			\
	mate-vfs-job-limit.h			\
	mate-vfs-mime.h			\
	mate-vfs-mime-deprecated.h		\
	mate-vfs-mime-handlers.h		\
	mate-vfs-mime-info-cache.h		\
	mate-vfs-mime-monitor.h		\
	mate-vfs-mime-utils.h			\
	mate-vfs-module-callback.h		\
	mate-vfs-monitor.h			\
	mate-vfs-ops.h				\
	mate-vfs-resolve.h			\
	mate-vfs-result.h			\
	mate-vfs-standard-callbacks.h		\
	mate-vfs-types.h			\
	mate-vfs-utils.h			\
	mate-vfs-volume.h			\
	mate-vfs-volume-monitor.h		\
	mate-vfs-xfer.h			\
	mate-vfs.h				\
	$(NULL)

libmatevfs_headers = 				\
	$(libmatevfs_headers_to_scan_for_enums)\
	mate-vfs-dns-sd.h			\
	mate-vfs-version.h			\
	mate-vfs-uri.h				\
	$(NULL)

export_symbols = $(LIBTOOL_EXPORT_OPTIONS)

# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
libmatevfs_built_headers = mate-vfs-enum-types.h
libmatevfs_built_cfiles = mate-vfs-enum-types.c

# Don't build the library until we have built the header that it needs:
$(OBJECTS) $(libmatevfs_2_la_OBJECTS): $(libmatevfs_built_headers)

mate-vfs-enum-types.h: @REBUILD@ $(libmatevfs_headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __LIBMATEVFS_ENUM_TYPES_H__\n#define __LIBMATEVFS_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define MATE_VFS_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "#ifdef __cplusplus\n}\n#endif\n\n#endif /* __LIBMATEVFS_ENUM_TYPES_H__ */" \
		$(libmatevfs_headers_to_scan_for_enums)) > $@

mate-vfs-enum-types.c: @REBUILD@ $(libmatevfs_headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && glib-mkenums \
			--fhead "#include <libmatevfs/mate-vfs.h>\n" \
			--fhead "#include <libmatevfs/mate-vfs-mime-handlers.h>\n" \
			--fhead "#include <glib-object.h>" \
		      	--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(libmatevfs_headers_to_scan_for_enums)) > $@


lib_LTLIBRARIES = libmatevfs-2.la

if OS_WIN32
SOCKET_LIBS = -lws2_32
else
noinst_LTLIBRARIES = libmatevfsdaemon-2.la
endif

libmatevfs_2_la_LIBADD =			\
	$(LIBMATEVFS_LIBS)			\
	$(FNMATCH_LIBS)				\
	$(SOCKET_LIBS)				\
	$(INTLLIBS)				\
	$(SELINUX_LIBS)				\
	$(NULL)

libmatevfs_2_la_LDFLAGS =			\
	-version-info $(LIBMATEVFS_CURRENT):$(LIBMATEVFS_REVISION):$(LIBMATEVFS_AGE) \
	-no-undefined \
	$(export_symbols) \
	$(NULL)

libmatevfsdaemon_2_la_LIBADD =			\
	$(LIBMATEVFS_LIBS)			\
	$(LIBMATEVFSDAEMON_LIBS)		\
	libmatevfs-2.la			\
	$(NULL)

libmatevfsincludedir = $(includedir)/mate-vfs-2.0/libmatevfs

libmatevfsmoduleincludedir = $(includedir)/mate-vfs-module-2.0/libmatevfs

# mime-sniff-buffers and cancellabe-ops are used by modules
noinst_HEADERS =				\
	mate-vfs-async-job-map.h		\
	mate-vfs-backend.h			\
	mate-vfs-cancellable-ops.h		\
	mate-vfs-cancellation-private.h	\
	mate-vfs-cdrom.h			\
	mate-vfs-configuration.h		\
	mate-vfs-daemon-method.h		\
	mate-vfs-dbus-utils.h                  \
	mate-vfs-filesystem-type.h		\
	mate-vfs-handle-private.h		\
	mate-vfs-hal-mounts.h			\
	mate-vfs-iso9660.h			\
	mate-vfs-job-queue.h			\
	mate-vfs-job.h				\
	mate-vfs-mime-magic.h			\
	mate-vfs-mime-private.h		\
	mate-vfs-mime-sniff-buffer-private.h	\
	mate-vfs-mime-sniff-buffer.h		\
	mate-vfs-module-callback-private.h	\
	mate-vfs-monitor-private.h		\
	mate-vfs-private-utils.h		\
	mate-vfs-private.h			\
	mate-vfs-unix-mounts.h 		\
	mate-vfs-volume-monitor-client.h	\
	mate-vfs-volume-monitor-daemon.h	\
	mate-vfs-volume-monitor-private.h 	\
	xdgmime.h				\
	xdgmimecache.h				\
	xdgmimeglob.h				\
	xdgmimemagic.h				\
	xdgmimeint.h				\
	xdgmimealias.h				\
	xdgmimeparent.h				\
	xdgmimeicon.h				\
	$(NULL)



libmatevfsmoduleinclude_HEADERS =		\
	mate-vfs-cancellable-ops.h		\
	mate-vfs-inet-connection.h		\
	mate-vfs-method.h			\
	mate-vfs-mime-info.h			\
	mate-vfs-module-callback-module-api.h	\
	mate-vfs-module-shared.h		\
	mate-vfs-module.h			\
	mate-vfs-parse-ls.h			\
	mate-vfs-ssl.h				\
	mate-vfs-transform.h			\
	mate-vfs-socket-buffer.h		\
	mate-vfs-socket.h			\
	mate-vfs-pty.h				\
	$(NULL)


libmatevfsinclude_HEADERS =			\
	mate-vfs-ace.h						\
	mate-vfs-acl.h						\
	mate-vfs-application-registry.h	\
	mate-vfs-address.h			\
	mate-vfs-async-ops.h			\
	mate-vfs-cancellation.h		\
	mate-vfs-context.h			\
	mate-vfs-directory.h			\
	mate-vfs-dns-sd.h			\
	mate-vfs-drive.h			\
	mate-vfs-enum-types.h			\
	mate-vfs-file-info.h			\
	mate-vfs-file-size.h			\
	mate-vfs-find-directory.h		\
	mate-vfs-handle.h			\
	mate-vfs-init.h			\
	mate-vfs-job-limit.h			\
	mate-vfs-mime.h			\
	mate-vfs-mime-deprecated.h		\
	mate-vfs-mime-handlers.h		\
	mate-vfs-mime-info-cache.h		\
	mate-vfs-mime-monitor.h		\
	mate-vfs-mime-utils.h			\
	mate-vfs-module-callback.h		\
	mate-vfs-monitor.h			\
	mate-vfs-ops.h				\
	mate-vfs-resolve.h			\
	mate-vfs-result.h			\
	mate-vfs-standard-callbacks.h		\
	mate-vfs-types.h			\
	mate-vfs-uri.h				\
	mate-vfs-utils.h			\
	mate-vfs-version.h			\
	mate-vfs-volume-monitor.h		\
	mate-vfs-volume.h			\
	mate-vfs-xfer.h			\
	mate-vfs.h				\
	$(NULL)

libmatevfs_win32_sources =			\
	mate-vfs-dllmain.c			\
	$(NULL)

libmatevfs_unix_sources =			\
	mate-vfs-cdrom.c			\
	mate-vfs-daemon-method.c		\
	mate-vfs-dbus-utils.c			\
	mate-vfs-dns-sd.c			\
	mate-vfs-module-callback-marshall.c	\
	mate-vfs-parse-ls.c			\
	mate-vfs-pty.c				\
	mate-vfs-volume-monitor-client.c	\
	mate-vfs-volume-ops.c			\
	$(NULL)

if OS_WIN32
libmatevfs_platform_sources = $(libmatevfs_win32_sources)
else
libmatevfs_platform_sources = $(libmatevfs_unix_sources)
endif

libmatevfs_2_la_SOURCES =			\
	mate-vfs-ace.c						\
	mate-vfs-acl.c						\
	mate-vfs-application-registry.c	\
	mate-vfs-address.c			\
	mate-vfs-async-job-map.c		\
	mate-vfs-async-ops.c			\
	mate-vfs-cancellable-ops.c	 	\
	mate-vfs-cancellation.c		\
	mate-vfs-configuration.c		\
	mate-vfs-context.c			\
	mate-vfs-directory.c			\
	mate-vfs-drive.c			\
	mate-vfs-file-info.c			\
	mate-vfs-filesystem-type.c		\
	mate-vfs-find-directory.c		\
	mate-vfs-handle.c			\
	mate-vfs-inet-connection.c		\
	mate-vfs-init.c			\
	mate-vfs-job-queue.c			\
	mate-vfs-job.c				\
	mate-vfs-method.c			\
	mate-vfs-mime-handlers.c		\
	mate-vfs-mime-info.c			\
	mate-vfs-mime-info-cache.c		\
	mate-vfs-mime-monitor.c		\
	mate-vfs-mime-sniff-buffer.c		\
	mate-vfs-mime.c			\
	mate-vfs-mime-magic.c			\
	mate-vfs-module-callback-module-api.c	\
	mate-vfs-module-callback-private.c	\
	mate-vfs-module-callback.c		\
	mate-vfs-module-shared.c		\
	mate-vfs-module.c			\
	mate-vfs-monitor.c			\
	mate-vfs-open-fd.c			\
	mate-vfs-ops.c				\
	mate-vfs-private-utils.c		\
	mate-vfs-private.c			\
	mate-vfs-resolve.c			\
	mate-vfs-result.c			\
	mate-vfs-socket-buffer.c		\
	mate-vfs-socket.c			\
	mate-vfs-ssl.c				\
	mate-vfs-transform.c			\
	mate-vfs-uri.c				\
	mate-vfs-utils.c			\
	mate-vfs-volume-monitor.c		\
	mate-vfs-volume.c			\
	mate-vfs-xfer.c			\
	xdgmimemagic.c				\
	xdgmime.c				\
	xdgmimecache.c				\
	xdgmimeglob.c				\
	xdgmimeint.c				\
	xdgmimealias.c				\
	xdgmimeparent.c				\
	xdgmimeicon.c				\
	$(libmatevfs_built_cfiles)		\
	$(libmatevfs_platform_sources)		\
	$(NULL)


# The daemon uses all interfaces in mate-vfs-cdrom
# and uses _mate_vfs_filesystem_volume_name, which all
# only use static data.
#
libmatevfsdaemon_2_la_SOURCES =		\
	mate-vfs-filesystem-type.c		\
	mate-vfs-cdrom.c			\
	mate-vfs-hal-mounts.c			\
	mate-vfs-volume-monitor-daemon.c	\
	mate-vfs-volume-monitor.c	 	\
	mate-vfs-unix-mounts.c			\
	$(NULL)

# Platform specific includes
libmatevfsplatformincludedir = $(libdir)/mate-vfs-2.0/include
libmatevfsplatforminclude_HEADERS =		\
	mate-vfs-file-size.h

# TESTS = check-headers.pl
EXTRA_DIST = check-headers.pl $(libmatevfs_win32_sources)

BUILT_SOURCES = \
	mate-vfs-enum-types.h			\
	mate-vfs-enum-types.c			\
	mate-vfs-version.h			\
	$(NULL)
CLEANFILES = $(BUILT_SOURCES)

