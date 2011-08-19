##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = users
bin_PROGRAMS = users-admin

SUBDIRS = 
INCLUDES += $(GST_TOOL_CFLAGS)

users_admin_LDADD = $(GST_TOOL_LIBS) 
users_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES) 
users_admin_SOURCES = \
	main.c 			\
	passwd.c 		passwd.h \
	users-tool.c 		users-tool.h \
	callbacks.c 		callbacks.h \
	user-settings.c 	user-settings.h \
	group-settings.c 	group-settings.h \
	users-table.c 		users-table.h \
	groups-table.c 		groups-table.h \
	table.c 		table.h	\
	privileges-table.c 	privileges-table.h \
	group-members-table.c 	group-members-table.h \
	user-profiles.c		user-profiles.h \
	test-battery.c		test-battery.h	\
	run-passwd.c		run-passwd.h	\
	user-password.c		user-password.h

toolpixmaps =

-include $(top_srcdir)/git.mk

profilesdir = $(confdir)
profiles_in_files = user-profiles.conf.in
profiles_DATA = $(profiles_in_files:.conf.in=.conf)
user-profiles.conf: user-profiles.conf.in @INTLTOOL_MERGE@ $(wildcard $(top_srcdir)/po/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< user-profiles.conf

CLEANFILES = $(GST_TOOL_CLEANFILES) user-profiles.conf
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST) user-profiles.conf.in
