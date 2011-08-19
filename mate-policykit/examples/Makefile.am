
bin_PROGRAMS = polkit-mate-example

polkit_mate_example_SOURCES = 				\
	polkit-mate-example.c

polkit_mate_example_CPPFLAGS = 			\
	-I$(top_srcdir)					\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DDATADIR=\""$(pkgdatadir)"\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

polkit_mate_example_CFLAGS = 			\
	$(GTK_CFLAGS)				\
	$(DBUS_GLIB_CFLAGS)			\
	$(POLKIT_DBUS_CFLAGS)			\
	$(WARN_CFLAGS)				\
	$(AM_CFLAGS)

polkit_mate_example_LDFLAGS = 			\
	$(AM_LDFLAGS)

polkit_mate_example_LDADD = 				\
	$(GTK_LIBS)					\
	$(DBUS_GLIB_LIBS)				\
	$(POLKIT_DBUS_LIBS)				\
	$(POLKIT_GRANT_LIBS)				\
	$(INTLLIBS)					\
	$(top_builddir)/polkit-mate/libpolkit-mate.la

# PolicyKit action definitions for this example
#
polkit_policydir = $(datadir)/PolicyKit/policy
dist_polkit_policy_DATA = org.mate.policykit.examples.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.mate.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

check:
	$(POLKIT_POLICY_FILE_VALIDATE) $(top_srcdir)/examples/$(dist_polkit_policy_DATA)

EXTRA_DIST = org.mate.policykit.examples.policy.in

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = org.mate.policykit.examples.policy

clean-local :
	rm -f *~
