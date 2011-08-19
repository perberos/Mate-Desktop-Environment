noinst_LTLIBRARIES = libwizard.la

libwizard_la_SOURCES = pin.c pin.h

libwizard_la_LIBADD = $(WIZARD_LIBS)
libwizard_la_LDFLAGS = -no-undefined $(AM_LDFLAGS)

noinst_PROGRAMS = test-input

bin_PROGRAMS = bluetooth-wizard

bluetooth_wizard_SOURCES = main.c

bluetooth_wizard_LDADD = $(top_builddir)/lib/libmate-bluetooth.la $(top_builddir)/lib/libcommon.la libwizard.la $(WIZARD_LIBS)

test_input_SOURCES = test-input.c bluetooth-input.c bluetooth-input.h
test_input_LDADD = $(INPUT_LIBS)

AM_CFLAGS = $(WIZARD_CFLAGS) $(INPUT_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED) -DPKGDATADIR="\"$(pkgdatadir)\""

pin_DATA = pin-code-database.xml
pindir = $(pkgdatadir)

ui_DATA = wizard.ui
uidir = $(pkgdatadir)

INCLUDES = -I$(top_srcdir)/lib

man_MANS = bluetooth-wizard.1

all: check

check:
	@if test -n $(XMLLINT) ; then \
		xmllint --noout --valid $(srcdir)/pin-code-database.xml ;	\
	fi

EXTRA_DIST = $(man_MANS) $(pin_DATA) $(ui_DATA)

MAINTAINERCLEANFILES = Makefile.in
