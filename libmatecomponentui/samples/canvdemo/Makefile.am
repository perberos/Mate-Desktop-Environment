noinst_PROGRAMS = main square circle

INCLUDES = -I$(top_srcdir) $(MATECOMPONENTUI_CFLAGS)

main_SOURCES = main.c
square_SOURCE = square.c
circle_SOURCES = circle.c

main_LDADD = $(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)
square_LDADD = $(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)
circle_LDADD = $(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)

serverdir       = $(libdir)/matecomponent/servers
server_in_files = CanvDemo.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

EXTRA_DIST= flow.png  \
	CanvDemo.server.in.in CanvDemo.sgml CanvDemo.html

CLEANFILES = $(server_in_files) $(server_DATA)

@INTLTOOL_SERVER_RULE@

