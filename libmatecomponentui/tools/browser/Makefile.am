INCLUDES =	\
	-DMATECOMPONENT_BROWSER_DATADIR=\""$(datadir)"\"	\
        -DMATECOMPONENT_SUPPORT_LOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_srcdir)/matecomponent-browser			\
	$(MATECOMPONENTUI_CFLAGS)				\
	$(WARN_CFLAGS)

bin_PROGRAMS = matecomponent-browser

matecomponent_browser_SOURCES = 		\
	matecomponent-browser.c		\
	matecomponent-browser.h		\
	component-info.c		\
	component-list.c		\
	component-list.h		\
	component-details.c		\
	component-details.h		\
	oaf-helper.c			\
	oaf-helper.h			\
	window.c

matecomponent_browser_LDADD = \
	$(MATECOMPONENTUI_LIBS)				\
	$(top_builddir)/matecomponent/libmatecomponentui-2.la

uidir = $(datadir)/mate-2.0/ui
ui_DATA = matecomponent-browser.xml

Utilitiesdir = $(datadir)/applications

Utilities_in_files = matecomponent-browser.desktop.in
Utilities_DATA = $(Utilities_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
        $(Utilities_in_files) 	\
        $(Utilities_DATA)	\
	matecomponent-browser.xml

CLEANFILES = matecomponent-browser.desktop

