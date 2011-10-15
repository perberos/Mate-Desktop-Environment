INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)					\
	$(MATECOMPONENTUI_CFLAGS)				\
	-DPREFIX=\""$(prefix)"\"			\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-DMATECOMPONENT_UI_INTERNAL				\
	-DMATECOMPONENT_UIDIR=\""$(datadir)/mate-2.0/ui/"\"	\
        -DMATECOMPONENT_LIBDIR=\""$(libdir)"\" 		\
        -DMATECOMPONENT_DATADIR=\""$(datadir)"\" 		\
        -DMATECOMPONENT_PIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DMATECOMPONENT_BINDIR=\""$(bindir)"\" 		\
        -DMATECOMPONENT_LOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DMATECOMPONENT_LOCALEDIR="\"$(datadir)/locale\""	\
	-DMATECOMPONENT_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE" \
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(DISABLE_DEPRECATED)				\
	$(MATECOMPONENTUI_SINGLE_INCLUDES)			\
	-UMATECOMPONENT_DISABLE_DEPRECATED			\
	-UMATECOMPONENT_UI_DISABLE_DEPRECATED

lib_LTLIBRARIES = libmatecomponentui-2.la

noinst_HEADERS = \
	matecomponent-control-internal.h		\
	matecomponent-ui-engine-config.h		\
	matecomponent-ui-engine-private.h		\
	matecomponent-ui-marshal.h			\
	matecomponent-ui-node-private.h		\
	matecomponent-ui-preferences.h			\
	matecomponent-ui-private.h			\
	matecomponent-ui-sync-menu.h			\
	matecomponent-ui-sync-keys.h			\
	matecomponent-ui-sync-status.h			\
	matecomponent-ui-sync-toolbar.h		\
	matecomponent-ui-toolbar.h			\
	matecomponent-ui-toolbar-control-item.h  	\
	matecomponent-ui-toolbar-popup-item.h		\
	matecomponent-ui-xml.h

libmatecomponentincludedir = $(includedir)/libmatecomponentui-2.0/matecomponent

libmatecomponentui_h_sources = \
	matecomponent-canvas-component.h		\
	matecomponent-canvas-item.h			\
	matecomponent-control-frame.h			\
	matecomponent-control.h			\
	matecomponent-dock-band.h			\
	matecomponent-dock-item.h			\
	matecomponent-dock-layout.h			\
	matecomponent-dock.h				\
	matecomponent-file-selector-util.h		\
	matecomponent-plug.h				\
	matecomponent-property-control.h		\
	matecomponent-selector-widget.h		\
	matecomponent-selector.h			\
	matecomponent-socket.h				\
	matecomponent-ui-component.h			\
	matecomponent-ui-config-widget.h		\
	matecomponent-ui-container.h			\
	matecomponent-ui-engine.h			\
	matecomponent-ui-main.h			\
	matecomponent-ui-node.h			\
	matecomponent-ui-sync.h			\
	matecomponent-ui-toolbar-button-item.h		\
	matecomponent-ui-toolbar-item.h		\
	matecomponent-ui-toolbar-toggle-button-item.h	\
	matecomponent-ui-toolbar.h			\
	matecomponent-ui-util.h			\
	matecomponent-widget.h				\
	matecomponent-window.h				\
	matecomponent-zoomable-frame.h			\
	matecomponent-zoomable.h

libmatecomponentui_2_la_LDFLAGS = \
	-no-undefined

libmatecomponentui_2_la_LIBADD = \
	$(MATECOMPONENTUI_LIBS) ${X_LIBS} $(INTLLIBS) $(POPT_LIBS)

libmatecomponentui_2_la_SOURCES = \
	matecomponent-a11y.c				\
	matecomponent-a11y.h				\
	matecomponent-dock.c				\
	matecomponent-dock.h				\
	matecomponent-dock-item.c			\
	matecomponent-dock-item.h			\
	matecomponent-dock-item-grip.c			\
	matecomponent-dock-item-grip.h			\
	matecomponent-dock-band.c			\
	matecomponent-dock-band.h			\
	matecomponent-dock-layout.c			\
	matecomponent-dock-layout.h			\
	matecomponent-ui-init-gtk.c			\
	matecomponent-ui-main.h			\
	matecomponent-ui-main.c			\
	matecomponent-canvas-component.c		\
	matecomponent-canvas-item.c			\
	matecomponent-control-frame.c			\
	matecomponent-control.c			\
	matecomponent-file-selector-util.c		\
	matecomponent-plug.c				\
	matecomponent-property-control.c		\
	matecomponent-selector.c			\
	matecomponent-selector-widget.c		\
	matecomponent-socket.c				\
	matecomponent-ui-component.c            	\
	matecomponent-ui-config-widget.c		\
	matecomponent-ui-container.c            	\
	matecomponent-ui-engine.c			\
	matecomponent-ui-engine-config.c		\
	matecomponent-ui-node.c                 	\
	matecomponent-ui-preferences.c			\
	matecomponent-ui-sync.c			\
	matecomponent-ui-sync-menu.c			\
	matecomponent-ui-sync-keys.c			\
	matecomponent-ui-sync-status.c			\
	matecomponent-ui-sync-toolbar.c		\
	matecomponent-ui-toolbar-button-item.c  	\
	matecomponent-ui-toolbar-control-item.c  	\
	matecomponent-ui-toolbar-item.c		\
	matecomponent-ui-toolbar-popup-item.c		\
	matecomponent-ui-toolbar-toggle-button-item.c	\
	matecomponent-ui-toolbar.c			\
	matecomponent-ui-internal-toolbar.c		\
	matecomponent-ui-type-builtins.c		\
	matecomponent-ui-util.c			\
	matecomponent-ui-win32.c			\
	matecomponent-ui-xml.c				\
	matecomponent-widget.c				\
	matecomponent-window.c				\
	matecomponent-zoomable.c			\
	matecomponent-zoomable-frame.c			\
	matecomponent-ui-marshal-main.c


$(libmatecomponentui_2_la_OBJECTS): $(libmatecomponentui_built_sources)

libmatecomponentui_built_public_sources = \
	matecomponent-ui-type-builtins.h

libmatecomponentui_built_private_sources = \
	matecomponent-ui-type-builtins.c	\
	matecomponent-ui-marshal.c 		\
	matecomponent-ui-marshal.h		\
	libmatecomponentui.defs

libmatecomponentui_built_sources = \
	$(libmatecomponentui_built_public_sources)	\
	$(libmatecomponentui_built_private_sources)

libmatecomponentinclude_HEADERS = \
	$(libmatecomponentui_h_sources) \
	$(libmatecomponentui_built_public_sources)

matecomponent-ui-marshal.h: matecomponent-ui-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=matecomponent_ui_marshal > $@

matecomponent-ui-marshal.c: matecomponent-ui-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=matecomponent_ui_marshal > $@

matecomponent-ui-type-builtins.c: Makefile $(libmatecomponentui_h_sources)
	$(GLIB_MKENUMS) --fhead "#undef GTK_DISABLE_DEPRECATED\n\n" \
			--fhead "#include \"config.h\"\n\n" \
			--fhead "#include \"libmatecomponentui.h\"\n\n" \
			--fhead "#include \"matecomponent-ui-type-builtins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(filter-out $<,$^) > xgen-gtbec \
	&& cp xgen-gtbec $(@F) \
	&& rm -f xgen-gtbec

matecomponent-ui-type-builtins.h: Makefile $(libmatecomponentui_h_sources)
	$(GLIB_MKENUMS) --fhead "#ifndef __MATECOMPONENT_UI_TYPE_BUILTINS_H__\n" \
			--fhead "#define __MATECOMPONENT_UI_TYPE_BUILTINS_H__ 1\n\n" \
			--fhead "#include <glib-object.h>\n\n" \
			--fhead "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n" \
			--ftail "#ifdef __cplusplus\n}\n#endif\n\n" \
			--ftail "#endif /* __MATECOMPONENT_UI_TYPE_BUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define MATECOMPONENT_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(filter-out $<,$^) > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth

libmatecomponentui.defs: Makefile $(libmatecomponentui_h_sources)
	$(GLIB_MKENUMS) --comments ";; @comment@" \
			--fprod "; enumerations from \"@filename@\"\n " \
	                --vhead "(define-@type@ @EnumName@" \
	                --vprod "   (@valuenick@ @VALUENAME@)" \
	                --vtail ")" \
			$(filter-out $<,$^) > xgen-gdef \
	&& (cmp -s xgen-gdef libmatecomponentui.defs || cp xgen-gdef libmatecomponentui.defs) \
	&& rm -f xgen-gdef

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmatecomponentui-2.0.pc

EXTRA_DIST = matecomponent-insert-component.xpm libmatecomponentui-2.0.pc.in \
	libmatecomponentui-2.0-uninstalled.pc.in matecomponent-ui-marshal.list
CLEANFILES = $(libmatecomponentui_built_sources)

BUILT_SOURCE = $(libmatecomponentui_built_sources)
DONT_DIST_SOURCE = $(libmatecomponentui_built_sources)

dist-hook:
	for file in $(DONT_DIST_SOURCE) ; do \
	    rm -f $(distdir)/$$file ; \
	done
