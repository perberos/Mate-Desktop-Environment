noinst_LTLIBRARIES=liborb-util.la

INCLUDES =						\
	-I$(top_srcdir)/include				\
	-I$(top_builddir)/include			\
	-DMATECORBA_SYSTEM_RCFILE=\"$(sysconfdir)/matecorbarc\"	\
	-DMATECORBA2_INTERNAL_API				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(LOCAL_LINC_CFLAGS)				\
	$(MATECORBA_CFLAGS)

liborb_util_la_SOURCES= \
	matecorba-purify.h  \
	genrand.c       \
	matecorba-options.c \
	matecorba-options.h \
	matecorba-util.c
