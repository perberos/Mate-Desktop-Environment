noinst_PROGRAMS = matekbd-indicator-test \
                  matekbd-keyboard-drawing-test \
                  matekbd-status-test

common_CFLAGS = $(WARN_CFLAGS) -I$(top_srcdir) -Wall \
	$(GTK_CFLAGS) \
	$(LIBXKLAVIER_CFLAGS) \
	-DMATELOCALEDIR="\"$(datadir)/locale\""
	
common_LDFLAGS = $(GTK_LIBS) \
	$(LIBXKLAVIER_LIBS) \
	$(top_builddir)/libmatekbd/libmatekbd.la	\
	$(top_builddir)/libmatekbd/libmatekbdui.la

matekbd_indicator_test_CFLAGS=$(common_CFLAGS)

matekbd_indicator_test_LDFLAGS=$(common_LDFLAGS)

matekbd_keyboard_drawing_test_CFLAGS=$(common_CFLAGS)

matekbd_keyboard_drawing_test_LDFLAGS=$(common_LDFLAGS)

matekbd_status_test_CFLAGS=$(common_CFLAGS)

matekbd_status_test_LDFLAGS=$(common_LDFLAGS)
