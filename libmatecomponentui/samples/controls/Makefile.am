MateComponent_Sample_Controls.server.in: MateComponent_Sample_Controls.server.in.in
	sed -e "s|\@prefix\@|$(prefix)|" $< > $@

samplesdir = $(libdir)/matecomponent-2.0/samples
samples_PROGRAMS = matecomponent-sample-controls-2

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(MATECOMPONENTUI_CFLAGS)

matecomponent_sample_controls_2_SOURCES =			\
	matecomponent-sample-controls.c

matecomponent_sample_controls_2_LDADD =			\
	$(top_builddir)/matecomponent/libmatecomponentui-2.la		\
	$(MATECOMPONENTUI_LIBS)				\
	$(INTLLIBS)

serverdir = $(libdir)/matecomponent/servers
server_in_files = MateComponent_Sample_Controls.server.in
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = MateComponent_Sample_Controls.server.in.in

CLEANFILES = $(server_in_files) $(server_DATA)
