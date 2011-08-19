NULL =

AM_CPPFLAGS =			\
	-I.			\
	-I..			\
	-I$(top_srcdir)/common	\
	$(COMMON_CFLAGS)	\
	$(NULL)

TESTS = 			\
	m-common		\
	$(NULL)

if HAVE_CHECK
noinst_PROGRAMS =		\
	$(TESTS)		\
	$(NULL)
endif

m_common_SOURCES =		\
	m-common.c		\
	s-common-address.c	\
	s-common-address.h	\
	s-common-utils.c	\
	s-common-utils.h	\
	$(NULL)

m_common_CFLAGS =		\
	@CHECK_CFLAGS@		\
	$(COMMON_CFLAGS)	\
	$(NULL)

m_common_LDADD = 		\
	$(top_builddir)/common/libgdmcommon.la	\
	@CHECK_LIBS@		\
	$(COMMON_LIBS)		\
	$(EXTRA_TEST_LIBS)	\
	$(NULL)

