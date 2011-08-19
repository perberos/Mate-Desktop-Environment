noinst_PROGRAMS = \
	test-replace \
	test-replace-widget \
	test-server-info \
	test-default-action \
	test-multi-actions \
	test-image \
	test-basic \
	test-error \
	test-markup \
	test-rtl \
	test-size-changes \
	test-urgency \
	test-xy \
	test-xy-actions \
	test-xy-stress

common_ldflags = \
	$(top_builddir)/libmatenotify/libmatenotify.la \
	$(PACKAGE_LIBS) \
	$(GDK_LIBS)

test_replace_SOURCES = test-replace.c
test_replace_LDADD  = $(common_ldflags)

test_replace_widget_SOURCES = test-replace-widget.c
test_replace_widget_LDADD  = $(common_ldflags)

test_server_info_SOURCES = test-server-info.c
test_server_info_LDADD   = $(common_ldflags)

test_size_changes_SOURCES = test-size-changes.c
test_size_changes_LDADD   = $(common_ldflags)

test_default_action_SOURCES = test-default-action.c
test_default_action_LDADD  = $(common_ldflags)

test_multi_actions_SOURCES = test-multi-actions.c
test_multi_actions_LDADD  = $(common_ldflags)

test_image_SOURCES = test-image.c
test_image_LDADD  = $(common_ldflags)

test_basic_SOURCES = test-basic.c
test_basic_LDADD = $(common_ldflags)

test_error_SOURCES = test-error.c
test_error_LDADD = $(common_ldflags)

test_markup_SOURCES = test-markup.c
test_markup_LDADD = $(common_ldflags)

test_urgency_SOURCES = test-urgency.c
test_urgency_LDADD = $(common_ldflags)

test_xy_SOURCES = test-xy.c
test_xy_LDADD = $(common_ldflags)

test_xy_actions_SOURCES = test-xy-actions.c
test_xy_actions_LDADD = $(common_ldflags)

test_xy_stress_SOURCES = test-xy-stress.c
test_xy_stress_LDADD = $(common_ldflags)

test_rtl_SOURCES = test-rtl.c
test_rtl_LDADD = $(common_ldflags)

EXTRA_DIST = applet-critical.png

INCLUDES = $(PACKAGE_CFLAGS)	\
	$(GDK_CFLAGS)		\
	-I$(top_srcdir)

-include $(top_srcdir)/git.mk
