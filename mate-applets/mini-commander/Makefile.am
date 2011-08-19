## Process this file with automake to produce Makefile.in

DIST_SUBDIRS = src help

server_in_files =                               \
	MATE_MiniCommanderApplet.server.in

if BUILD_MINICOMMANDER_APPLET
SUBDIRS = src help
else
# we need to generate the alternative server file here
serverdir = $(libdir)/matecomponent/servers
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

CLEANFILES = $(server_DATA)

endif

EXTRA_DIST = $(server_in_files)


-include $(top_srcdir)/git.mk
