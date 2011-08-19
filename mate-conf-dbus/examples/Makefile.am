INCLUDES=-I$(top_srcdir) -I$(top_builddir) \
	$(DEPENDENT_WITH_GTK_CFLAGS) \
        -DMATECONF_SRCDIR=\""$(absolute_top_srcdir)"\" -DMATECONF_SYSCONFDIR=\""$(sysmateconfdir)"\"

EFENCE=

if GTK
GTK_EXAMPLES=basic-mateconf-app simple-view simple-controller complex-mateconf-app
else
GTK_EXAMPLES=
endif

noinst_PROGRAMS=$(GTK_EXAMPLES)

GTK_EXAMPLES_LINK= $(EFENCE) $(INTLLIBS) $(DEPENDENT_WITH_GTK_LIBS) $(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la $(EFENCE)

basic_mateconf_app_SOURCES = basic-mateconf-app.c

basic_mateconf_app_LDADD = $(GTK_EXAMPLES_LINK)

simple_view_SOURCES = simple-view.c

simple_view_LDADD = $(GTK_EXAMPLES_LINK)

simple_controller_SOURCES = simple-controller.c

simple_controller_LDADD = $(GTK_EXAMPLES_LINK)

complex_mateconf_app_SOURCES = basic-mateconf-app.c

complex_mateconf_app_LDADD = $(GTK_EXAMPLES_LINK)



