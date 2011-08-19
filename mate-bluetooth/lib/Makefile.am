SUBDIRS = plugins

EXTRA_DIST = 
BUILT_GIRSOURCES =

noinst_LTLIBRARIES = libcommon.la
lib_LTLIBRARIES = libmate-bluetooth.la

libcommon_la_SOURCES =						\
		bluetooth-client.h bluetooth-client.c		\
		bluetooth-client-private.h			\
		bluetooth-agent.h bluetooth-agent.c		\
		bluetooth-killswitch.h bluetooth-killswitch.c	\
		obex-agent.h obex-agent.c			\
		bluetooth-plugin-manager.c			\
		bluetooth-plugin-manager.h			\
		bluetooth-enums.h				\
		rfkill.h

libcommon_la_LIBADD = $(COMMON_LIBS)
libcommon_la_LDFLAGS = -no-undefined $(AM_LDFLAGS)

libmate_bluetooth_la_SOURCES =				\
	bluetooth-enums.h				\
	bluetooth-client.h bluetooth-client.c		\
	bluetooth-client-private.h			\
	marshal.h marshal.c				\
	mate-bluetooth-enum-types.h			\
	mate-bluetooth-enum-types.c			\
	bluetooth-chooser.c bluetooth-chooser.h		\
	bluetooth-chooser-private.h			\
	bluetooth-filter-widget.c			\
	bluetooth-filter-widget.h			\
	bluetooth-chooser-button.c			\
	bluetooth-chooser-button.h			\
	bluetooth-chooser-combo.c			\
	bluetooth-chooser-combo.h

libmate_bluetooth_la_LIBADD = $(LIBMATEBT_LIBS)

libmate_bluetooth_la_LDFLAGS =				\
	-version-info $(MATEBT_LT_VERSION)		\
	-no-undefined					\
	$(AM_LDFLAGS)					\
	-export-symbols $(srcdir)/mate-bluetooth.symbols

matebluetoothdir = $(pkgincludedir)
matebluetooth_HEADERS =				\
	bluetooth-client.h				\
	bluetooth-chooser.h				\
	bluetooth-chooser-button.h			\
	bluetooth-filter-widget.h			\
	bluetooth-enums.h				\
	bluetooth-plugin.h

AM_CFLAGS = -I$(srcdir) $(LIBMATEBT_CFLAGS) $(COMMON_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED) -DPLUGINDIR=\"$(libdir)/mate-bluetooth/plugins\"

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = $(matebluetooth_HEADERS)

MateBluetooth-1.0.gir: libmate-bluetooth.la
MateBluetooth_1_0_gir_INCLUDES = GObject-2.0 Gtk-2.0 DBusGLib-1.0 GModule-2.0
MateBluetooth_1_0_gir_CFLAGS = -I$(srcdir) $(LIBMATEBT_CFLAGS)
MateBluetooth_1_0_gir_LIBS = libmate-bluetooth.la
MateBluetooth_1_0_gir_SCANNERFLAGS = --strip-prefix=Bluetooth --pkg-export=mate-bluetooth-1.0
MateBluetooth_1_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += MateBluetooth-1.0.gir

girdir = $(INTROSPECTION_GIRDIR)

EXTRA_DIST += $(BUILT_GIRSOURCES)
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)


endif # HAVE_INTROSPECTION

BUILT_SOURCES = marshal.h marshal.c \
		bluetooth-client-glue.h \
		bluetooth-agent-glue.h \
		obex-agent-glue.h \
		mate-bluetooth-enum-types.h \
		mate-bluetooth-enum-types.c

nodist_libcommon_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES) $(gir_DATA) $(typelib_DATA)

noinst_PROGRAMS = test-client test-agent test-deviceselection test-plugins test-class

test_client_LDADD = libcommon.la $(GTK_LIBS) $(DBUS_LIBS)

test_agent_LDADD = libcommon.la $(DBUS_LIBS)

test_deviceselection_LDADD = libcommon.la libmate-bluetooth.la

test_plugins_LDADD = libcommon.la

test_class_LDADD = libcommon.la

EXTRA_DIST += marshal.list \
		bluetooth-client.xml \
		bluetooth-agent.xml \
		obex-agent.xml \
		mate-bluetooth.symbols

MAINTAINERCLEANFILES = Makefile.in

marshal.h: marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=marshal $< --header > $@

marshal.c: marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=marshal $< --header --body > $@

bluetooth-client-glue.h: bluetooth-client.xml bluetooth-agent.h bluetooth-agent.c
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=bluetooth_client --mode=glib-client --output=$@ $<

bluetooth-agent-glue.h: bluetooth-agent.xml bluetooth-agent.h bluetooth-agent.c
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=bluetooth_agent --mode=glib-server --output=$@ $<

obex-agent-glue.h: obex-agent.xml
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --prefix=obex_agent --mode=glib-server --output=$@ $<

mate_bluetooth_headers_to_scan_for_enums = bluetooth-enums.h
# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
mate_bluetooth_built_headers = mate-bluetooth-enum-types.h
mate_bluetooth_built_cfiles = mate-bluetooth-enum-types.c

mate-bluetooth-enum-types.h: $(mate_bluetooth_headers_to_scan_for_enums) Makefile
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __MATE_BLUETOOTH_ENUM_TYPES_H__\n#define __MATE_BLUETOOTH_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define BLUETOOTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __MATE_BLUETOOTH_ENUM_TYPES_H__ */" \
		$(mate_bluetooth_headers_to_scan_for_enums)) > $@

mate-bluetooth-enum-types.c: $(mate_bluetooth_headers_to_scan_for_enums) Makefile mate-bluetooth-enum-types.h
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#include <bluetooth-enums.h>\n" \
			--fhead "#include \"mate-bluetooth-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
		      	--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(mate_bluetooth_headers_to_scan_for_enums)) > $@
