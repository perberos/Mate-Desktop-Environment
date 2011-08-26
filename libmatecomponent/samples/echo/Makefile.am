#
# Makefile for the sample Echo server/client MateComponent components.
#

server_dot_in=MateComponent_Sample_Echo.server.in

$(server_dot_in): $(server_dot_in).in
	$(AM_V_GEN) $(SED) -e "s|\@prefix\@|$(prefix)|" $< > $@

samplesdir = $(libdir)/matecomponent-2.0/samples
samples_PROGRAMS = matecomponent-echo-2

bin_PROGRAMS     = matecomponent-echo-client-2
noinst_LIBRARIES = libEcho.a

matecomponent_echo_2_SOURCES =		\
	main.c			\
	echo.c			\
	echo.h

matecomponent_echo_2_LDADD =				\
	libEcho.a				\
	$(top_builddir)/matecomponent/libmatecomponent-2.la	\
	$(LIBMATECOMPONENT_LIBS)			\
	$(INTLLIBS)

matecomponent_echo_client_2_SOURCES = echo-client.c
matecomponent_echo_client_2_LDADD =				\
	libEcho.a				\
	$(top_builddir)/matecomponent/libmatecomponent-2.la	\
	$(LIBMATECOMPONENT_LIBS)			\
	$(INTLLIBS)

ECHO_CORBA_GENERATED =				\
	MateComponent_Sample_Echo-common.c		\
	MateComponent_Sample_Echo-skels.c		\
	MateComponent_Sample_Echo-stubs.c		\
	MateComponent_Sample_Echo.h

libEcho_a_SOURCES = $(ECHO_CORBA_GENERATED)

$(ECHO_CORBA_GENERATED): MateComponent_Sample_Echo.idl $(MATECORBA_IDL)
	$(AM_V_GEN) $(MATECORBA_IDL) -I$(top_srcdir)/idl $(srcdir)/MateComponent_Sample_Echo.idl

serverdir = $(libdir)/matecomponent/servers
server_in_files = $(server_dot_in)
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

idldir    = $(datadir)/idl/matecomponent-2.0
idl_DATA  = MateComponent_Sample_Echo.idl

EXTRA_DIST = $(server_DATA) $(idl_DATA) $(server_dot_in).in

INCLUDES = 						\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-DG_LOG_DOMAIN=\"Echo\"				\
	-I$(top_srcdir) 				\
	$(LIBMATECOMPONENT_CFLAGS)				\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(WARN_CFLAGS)

CLEANFILES =  $(server_dot_in) $(server_DATA)

