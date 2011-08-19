NULL =

if HAVE_SSL
SSL_DEFINES = -DNE_HAVE_SSL
else
SSL_DEFINES =
endif

INCLUDES =					\
	-DNE_HAVE_DAV				\
	$(SSL_DEFINES)                          \
	$(NEON_CFLAGS)				\
	$(LIBMATEVFS_CFLAGS)			\
	$(GSSAPI_CFLAGS)			\
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
        -I$(top_srcdir)/libmatevfs             \
        -I$(top_builddir)/libmatevfs           \
        $(NULL)

noinst_LTLIBRARIES = libneon.la

NEON_INCLUDES = \
	ne_207.h	\
	ne_acl.h	\
	ne_alloc.h	\
	ne_auth.h	\
	ne_basic.h	\
	ne_compress.h	\
	ne_dates.h	\
	ne_defs.h	\
	ne_i18n.h	\
	ne_locks.h	\
	ne_md5.h	\
	ne_private.h	\
	ne_props.h	\
	ne_redirect.h	\
	ne_request.h	\
	ne_session.h	\
	ne_socket.h	\
	ne_ssl.h	\
	ne_string.h	\
	ne_uri.h	\
	ne_utils.h	\
	ne_xml.h	\
	ne_xmlreq.h	\
	ne_matevfs.h	\
	$(NULL)


NEON_BASE_SOURCES = ne_request.c ne_session.c 	\
	ne_basic.c  ne_string.c	 		\
	ne_uri.c ne_dates.c ne_alloc.c  	\
	ne_md5.c ne_utils.c    			\
	ne_auth.c 				\
	ne_redirect.c		\
	ne_compress.c

NEON_DAV_SOURCES = \
	ne_207.c ne_xml.c \
	ne_props.c ne_locks.c ne_xmlreq.c

NEON_EXTRA_SOURCES = ne_acl.c ne_matevfs.c

libneon_la_LIBADD =			\
	$(NEON_LIBS) $(NEON_LTLIBS)	\
	$(NULL)

libneon_la_LDFLAGS =			\
	$(NEON_LINK_FLAGS)			\
	$(NULL)

libneon_la_SOURCES = \
	$(NEON_INCLUDES)	\
	$(NEON_BASE_SOURCES) 	\
	$(NEON_SSL_SOURCES)	\
	$(NEON_DAV_SOURCES) 	\
	$(NEON_EXTRA_SOURCES)	\
        $(NULL)
