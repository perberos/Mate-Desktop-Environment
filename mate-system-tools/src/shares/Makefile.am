##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = shares
bin_PROGRAMS = shares-admin

if BUILD_CAJA
SUBDIRS = caja
endif

INCLUDES += $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

shares_admin_LDADD = $(GST_TOOL_LIBS) 
shares_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES)
shares_admin_SOURCES = \
	table.c table.h \
	users-table.c users-table.h \
	nfs-acl-table.c nfs-acl-table.h \
	callbacks.c callbacks.h \
	share-settings.c share-settings.h \
	share-nfs-add-hosts.c share-nfs-add-hosts.h \
	shares-tool.c shares-tool.h \
	main.c 

toolpixmaps =

-include $(top_srcdir)/git.mk
