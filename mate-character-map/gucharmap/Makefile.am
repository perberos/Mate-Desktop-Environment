## Copyright © 2003  Noah Levitt
## Copyright © 2008  Christian Persch
##
## This program is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation; either version 2 of the License, or (at your
## option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
##

NULL =

lib_LTLIBRARIES = libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@.la

BUILT_SOURCES = \
	gucharmap-marshal.c \
	gucharmap-marshal.h \
	gucharmap-type-builtins.c \
	gucharmap-type-builtins.h \
	$(NULL)

libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_SOURCES = \
	gucharmap-block-chapters-model.c \
	gucharmap-block-chapters-model.h \
	gucharmap-block-codepoint-list.c \
	gucharmap-block-codepoint-list.h \
	gucharmap-chapters-model.c \
	gucharmap-chapters-model.h \
	gucharmap-chapters-view.c \
	gucharmap-chapters-view.h \
	gucharmap-charmap.c \
	gucharmap-charmap.h \
	gucharmap-chartable-accessible.c \
	gucharmap-chartable-accessible.h \
	gucharmap-chartable-cell-accessible.c \
	gucharmap-chartable-cell-accessible.h \
	gucharmap-chartable.c \
	gucharmap-chartable.h \
	gucharmap-codepoint-list.c \
	gucharmap-codepoint-list.h \
	gucharmap-init.c \
	gucharmap-private.h \
	gucharmap-script-chapters-model.c \
	gucharmap-script-chapters-model.h \
	gucharmap-script-codepoint-list.c \
	gucharmap-script-codepoint-list.h \
	gucharmap-unicode-info.c \
	gucharmap-unicode-info.h \
	unicode-blocks.h \
	unicode-categories.h \
	unicode-names.h \
	unicode-nameslist.h \
	unicode-scripts.h \
	unicode-unihan.h \
	unicode-versions.h \
	$(NULL)

nodist_libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_SOURCES = $(BUILT_SOURCES)

libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGUCHARMAP_COMPILATION \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DHELPDIR=\"$(HELP_DIR)\" \
	-DG_DISABLE_SINGLE_INCLUDES \
	-DPANGO_DISABLE_SINGLE_INCLUDES \
	-DATK_DISABLE_SINGLE_INCLUDES \
	-DGDK_DISABLE_SINGLE_INCLUDES \
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES \
	-DGTK_DISABLE_SINGLE_INCLUDES \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS)

libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)

libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_LDFLAGS = \
	-version-info $(LIBGUCHARMAP_LT_VERSION)	\
	-export-symbols-regex "^gucharmap_*"		\
	$(AM_LDFLAGS)

libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@_la_LIBADD = \
	$(GTK_LIBS)	\
	$(INTLLIBS)

libgucharmapincludedir = $(includedir)/gucharmap-$(GUCHARMAP_API_VERSION)/gucharmap
libgucharmapinclude_HEADERS = \
	gucharmap-block-chapters-model.h \
	gucharmap-block-codepoint-list.h \
	gucharmap-chapters-model.h \
	gucharmap-chapters-view.h \
	gucharmap-charmap.h \
	gucharmap-chartable.h \
	gucharmap-codepoint-list.h \
	gucharmap.h \
	gucharmap-script-chapters-model.h \
	gucharmap-script-codepoint-list.h \
	gucharmap-unicode-info.h \
	$(NULL)

nodist_libgucharmapinclude_HEADERS = \
	gucharmap-type-builtins.h \
	$(NULL)

if ENABLE_CHARMAP
bin_PROGRAMS = gucharmap
else
noinst_PROGRAMS = gucharmap
endif

gucharmap_SOURCES = \
	gucharmap-mini-fontsel.c \
	gucharmap-mini-fontsel.h \
	gucharmap-print-operation.c \
	gucharmap-print-operation.h \
	gucharmap-search-dialog.c \
	gucharmap-search-dialog.h \
	gucharmap-settings.c \
	gucharmap-settings.h \
	gucharmap-window.c \
 	gucharmap-window.h \
	main.c \
	$(NULL)

gucharmap_CPPFLAGS = \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DHELPDIR=\"$(HELP_DIR)\" \
	-DG_DISABLE_SINGLE_INCLUDES \
	-DPANGO_DISABLE_SINGLE_INCLUDES \
	-DATK_DISABLE_SINGLE_INCLUDES \
	-DGDK_DISABLE_SINGLE_INCLUDES \
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES \
	-DGTK_DISABLE_SINGLE_INCLUDES \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS)

gucharmap_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(MATECONF_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)

gucharmap_LDFLAGS = \
	$(AM_LDFLAGS)

gucharmap_LDADD = \
	libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@.la	\
	$(GTK_LIBS)	\
	$(MATECONF_LIBS)	\
	$(INTL_LIBS)

if OS_WIN32
gucharmap_LDADD += gucharmaprc.o
gucharmaprc.o: gucharmaprc.rc
	$(AM_V_GEN) windres -i $< -o $@
endif

gucharmap-marshal.h: $(srcdir)/gucharmap-marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=_gucharmap_marshal $(srcdir)/gucharmap-marshal.list \
                            --header \
                            --internal > gucharmap-marshal.h.tmp \
	  && mv gucharmap-marshal.h.tmp gucharmap-marshal.h ) \
        || ( rm -f gucharmap-marshal.h.tmp && exit 1 )

gucharmap-marshal.c: $(srcdir)/gucharmap-marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=_gucharmap_marshal $(srcdir)/gucharmap-marshal.list \
			    --header \
                            --body \
                            --internal > gucharmap-marshal.c.tmp \
          && mv gucharmap-marshal.c.tmp gucharmap-marshal.c ) \
        || ( rm -f gucharmap-marshal.c.tmp && exit 1 )


gucharmap-type-builtins.h: stamp-gucharmap-type-builtins.h
	@true

stamp-gucharmap-type-builtins.h: gucharmap-type-builtins.h.template $(libgucharmapinclude_HEADERS)
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-gtbh \
	&& (cmp -s xgen-gtbh gucharmap-type-builtins.h || cp xgen-gtbh gucharmap-type-builtins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)

gucharmap-type-builtins.c: gucharmap-type-builtins.c.template $(libgucharmapinclude_HEADERS)
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-gtbc \
	&& (cmp -s xgen-gtbc gucharmap-type-builtins.c || cp xgen-gtbc gucharmap-type-builtins.c ) \
	&& rm -f xgen-gtbc

# printoptions.h: print.glade
# 	$(AM_V_GEN) $(GTK_BUILDER_CONVERT) --root custom_tab_container $< - | \
# 		$(XMLLINT) --noblanks - | \
# 		$(HEXDUMP) -v -e '"" 1/1 "0x%02x,""\n"' > xgen-$(@F) \
# 	&& (cmp -s xgen-$(@F) $@ || cp -f xgen-$(@F) $@) \
# 	&& rm -f xgen-$(@F)

if ENABLE_INTROSPECTION

girdir = $(GIRDIR)
gir_DATA = Gucharmap-$(GUCHARMAP_API_VERSION).gir

typelibdir = $(GIRTYPELIBDIR)
typelib_DATA = Gucharmap-$(GUCHARMAP_API_VERSION).typelib

Gucharmap-$(GUCHARMAP_API_VERSION).gir: $(G_IR_SCANNER) libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@.la $(libgucharmapinclude_HEADERS) Makefile
	$(AM_V_GEN) PKG_CONFIG_PATH=$(top_builddir):$$PKG_CONFIG_PATH \
	$(G_IR_SCANNER) \
		--verbose \
		--namespace=Gucharmap \
		--nsversion=$(GUCHARMAP_API_VERSION) \
		--include=GLib-2.0 \
		--include=GObject-2.0 \
		--include=Gtk-$(GTK_API_VERSION) \
		--library=gucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@ \
		--libtool="$(LIBTOOL)" \
		--output $@ \
		--pkg gucharmap-$(GUCHARMAP_API_PC_VERSION) \
		-I$(top_srcdir) \
		-I$(top_builddir) \
		-DGUCHARMAP_COMPILATION \
		$(filter %.h,$^)

%.typelib: %.gir $(G_IR_COMPILER)
	$(AM_V_GEN) $(G_IR_COMPILER) --includedir=$(srcdir) --includedir=. $(G_IR_COMPILER_OPTS) $< -o $@

endif # ENABLE_INTROSPECTION

EXTRA_DIST = \
	gucharmap-marshal.list \
	gucharmap-type-builtins.c.template \
	gucharmap-type-builtins.h.template \
	gucharmaprc.rc \
	$(NULL)

CLEANFILES = \
	stamp-gucharmap-type-builtins.h \
	$(BUILT_SOURCES) \
	$(gir_DATA) \
	$(typelib_DATA) \
	$(NULL)

if ENABLE_CHARMAP

# don't do the symlink stuff if this is windows
if !OS_WIN32
# install a symlink to the binary called "charmap"
install-exec-hook:
	test -e "$(DESTDIR)$(bindir)/charmap" \
        || (cd "$(DESTDIR)$(bindir)" && $(LN_S) gucharmap charmap) ; \
	test -e "$(DESTDIR)$(bindir)/mate-character-map" \
        || (cd "$(DESTDIR)$(bindir)" && $(LN_S) gucharmap mate-character-map)

# remove bin/charmap if it's a symlink (it's a pain to test what it's
# a symlink to)
uninstall-local:
	test -h "$(DESTDIR)$(bindir)/charmap" \
	&& rm -f "$(DESTDIR)$(bindir)/charmap" ; \
	test -h "$(DESTDIR)$(bindir)/mate-character-map" \
	&& rm -f "$(DESTDIR)$(bindir)/mate-character-map"

endif # !OS_WIN32

endif # ENABLE_CHARMAP

-include $(top_srcdir)/git.mk
