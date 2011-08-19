bin_PROGRAMS = matecorba-idl-2

matecorba_backends_dir = $(libdir)/matecorba-2.0/idl-backends

INCLUDES =							\
	-I$(top_builddir)					\
	-I$(top_builddir)/include				\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/include					\
	-DMATECORBA_BACKENDS_DIR="\"$(matecorba_backends_dir)\""	\
	-DVERSION=\"$(MATECORBA_VERSION)\"				\
	-DMATECORBA2_INTERNAL_API					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(MATECORBA_IDL_CFLAGS)

matecorba_idl_2_LDADD =                     \
	$(MATECORBA_IDL_LIBS)		\
	$(LIBM)

matecorba_idl_2_SOURCES =			\
	matecorba-idl-main.c		\
	matecorba-idl-driver.c		\
	matecorba-idl-backend.c		\
	matecorba-idl-backend.h		\
	matecorba-idl-utils.c		\
	matecorba-idl2.h			\
	matecorba-idl3-types.h		\
        matecorba-idl-c-backend.h		\
        matecorba-idl-c-backend.c		\
        matecorba-idl-c-stubs.c		\
        matecorba-idl-c-skels.c		\
        matecorba-idl-c-headers.c		\
        matecorba-idl-c-common.c		\
	matecorba-idl-c-imodule.c		\
        matecorba-idl-c-skelimpl.c		\
        matecorba-idl-c-utils.c		\
        matecorba-idl-c-typecode.c		\
        matecorba-idl-c-deps.c		\
	matecorba-idl2.h

matecorbaidlincludedir = $(includedir)/matecorba-2.0/matecorba-idl
matecorbaidlinclude_HEADERS = matecorba-idl-backend.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = MateCORBA-idl-2.0.pc
