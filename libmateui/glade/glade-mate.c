/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-mate.c: support for libmateui widgets in libglade.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

/* this file is only built if MATE support is enabled */

#undef GTK_DISABLE_DEPRECATED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>
#include <libmateui/libmateui.h>

#include <libmatecomponentui.h>

/* following function is not public, so we define the prototype here ... */
void	matecomponent_dock_item_set_behavior	(MateComponentDockItem         *dock_item,
					 MateComponentDockItemBehavior  behavior);


#define INT(s)   (strtol ((s), NULL, 0))
#define BOOL(s)  (g_ascii_tolower (*(s)) == 't' || g_ascii_tolower (*(s)) == 'y' || INT (s))
#define FLOAT(s) (g_strtod ((s), NULL))

typedef struct {
    const char *extension;
    MateUIInfo data;
} mateuiinfo_map_t;

static MateUIInfo tmptree[] = {
    MATEUIINFO_END
};

static const mateuiinfo_map_t mate_uiinfo_mapping[] = {
    { "ABOUT_ITEM", MATEUIINFO_MENU_ABOUT_ITEM(NULL, NULL) },
    { "CLEAR_ITEM", MATEUIINFO_MENU_CLEAR_ITEM(NULL, NULL) },
    { "CLOSE_ITEM", MATEUIINFO_MENU_CLOSE_ITEM(NULL, NULL) },
    { "CLOSE_WINDOW_ITEM", MATEUIINFO_MENU_CLOSE_WINDOW_ITEM(NULL,NULL) },
    { "COPY_ITEM", MATEUIINFO_MENU_COPY_ITEM(NULL, NULL) },
    { "CUT_ITEM", MATEUIINFO_MENU_CUT_ITEM(NULL, NULL) },
    { "EDIT_TREE", MATEUIINFO_MENU_EDIT_TREE(tmptree) },
    { "END_GAME_ITEM", MATEUIINFO_MENU_END_GAME_ITEM(NULL, NULL) },
    { "EXIT_ITEM", MATEUIINFO_MENU_EXIT_ITEM(NULL, NULL) },
    { "FILES_TREE", MATEUIINFO_MENU_FILES_TREE(tmptree) },
    { "FILE_TREE", MATEUIINFO_MENU_FILE_TREE(tmptree) },
    { "FIND_AGAIN_ITEM", MATEUIINFO_MENU_FIND_AGAIN_ITEM(NULL, NULL) },
    { "FIND_ITEM", MATEUIINFO_MENU_FIND_ITEM(NULL, NULL) },
    { "GAME_TREE", MATEUIINFO_MENU_GAME_TREE(tmptree) },
    { "HELP_TREE", MATEUIINFO_MENU_HELP_TREE(tmptree) },
    { "HINT_ITEM", MATEUIINFO_MENU_HINT_ITEM(NULL, NULL) },
    { "NEW_GAME_ITEM", MATEUIINFO_MENU_NEW_GAME_ITEM(NULL, NULL) },
    { "NEW_ITEM", MATEUIINFO_MENU_NEW_ITEM(NULL, NULL, NULL, NULL) },
    { "NEW_SUBTREE", MATEUIINFO_MENU_NEW_SUBTREE(tmptree) },
    { "NEW_WINDOW_ITEM", MATEUIINFO_MENU_NEW_WINDOW_ITEM(NULL, NULL) },
    { "OPEN_ITEM", MATEUIINFO_MENU_OPEN_ITEM(NULL, NULL) },
    { "PASTE_ITEM", MATEUIINFO_MENU_PASTE_ITEM(NULL, NULL) },
    { "PAUSE_GAME_ITEM", MATEUIINFO_MENU_PAUSE_GAME_ITEM(NULL, NULL) },
    { "PREFERENCES_ITEM", MATEUIINFO_MENU_PREFERENCES_ITEM(NULL, NULL) },
    { "PRINT_ITEM", MATEUIINFO_MENU_PRINT_ITEM(NULL, NULL) },
    { "PRINT_SETUP_ITEM", MATEUIINFO_MENU_PRINT_SETUP_ITEM(NULL, NULL) },
    { "PROPERTIES_ITEM", MATEUIINFO_MENU_PROPERTIES_ITEM(NULL, NULL) },
    { "REDO_ITEM", MATEUIINFO_MENU_REDO_ITEM(NULL, NULL) },
    { "REDO_MOVE_ITEM", MATEUIINFO_MENU_REDO_MOVE_ITEM(NULL, NULL) },
    { "REPLACE_ITEM", MATEUIINFO_MENU_REPLACE_ITEM(NULL, NULL) },
    { "RESTART_GAME_ITEM", MATEUIINFO_MENU_RESTART_GAME_ITEM(NULL,NULL) },
    { "REVERT_ITEM", MATEUIINFO_MENU_REVERT_ITEM(NULL, NULL) },
    { "SAVE_AS_ITEM", MATEUIINFO_MENU_SAVE_AS_ITEM(NULL, NULL) },
    { "SAVE_ITEM", MATEUIINFO_MENU_SAVE_ITEM(NULL, NULL) },
    { "SCORES_ITEM", MATEUIINFO_MENU_SCORES_ITEM(NULL, NULL) },
    { "SELECT_ALL_ITEM", MATEUIINFO_MENU_SELECT_ALL_ITEM(NULL, NULL) },
    { "SETTINGS_TREE", MATEUIINFO_MENU_SETTINGS_TREE(tmptree) },
    { "UNDO_ITEM", MATEUIINFO_MENU_UNDO_ITEM(NULL, NULL) },
    { "UNDO_MOVE_ITEM", MATEUIINFO_MENU_UNDO_MOVE_ITEM(NULL, NULL) },
    { "VIEW_TREE", MATEUIINFO_MENU_VIEW_TREE(tmptree) },
    { "WINDOWS_TREE", MATEUIINFO_MENU_WINDOWS_TREE(tmptree) }
};

static int
stock_compare (const void *a, const void *b)
{
    const mateuiinfo_map_t *ga = a;
    const mateuiinfo_map_t *gb = b;

    return strcmp (ga->extension, gb->extension);
}

static gboolean
get_stock_uiinfo (const char *stock_name, MateUIInfo *info)
{
    const int len = strlen ("MATEUIINFO_MENU_");
    mateuiinfo_map_t *v;
    mateuiinfo_map_t base;

    /* If an error happens, return this */
    if (!strncmp (stock_name, "MATEUIINFO_MENU_", len)) {
	base.extension = stock_name + len;
	v = bsearch (
	    &base,
	    mate_uiinfo_mapping,
	    sizeof(mate_uiinfo_mapping) /
	    sizeof(mateuiinfo_map_t),
	    sizeof (mate_uiinfo_mapping [0]),
	    stock_compare);
	if (v) {
	    *info = v->data;
	    return TRUE;
	} else
	    return FALSE;
    }
    return FALSE;
}

static void
menushell_build_children (GladeXML *xml, GtkWidget *w, 
			  GladeWidgetInfo *info)
{
    int i, j;
    MateUIInfo infos[2] = {
	{ MATE_APP_UI_ITEM },
	MATEUIINFO_END
    };

    for (i = 0; i < info->n_children; i++) {
	GladeChildInfo *cinfo = &info->children[i];
	GladeWidgetInfo *cwinfo = cinfo->child;
	GtkWidget *child;
	gchar *stock_name = NULL;
	
	for (j = 0; j < cwinfo->n_properties; j++) {
	    if (!strcmp (cwinfo->properties[j].name, "stock_item")) {
		stock_name = cwinfo->properties[j].value;
		break;
	    }
	}
	if (!stock_name) {
	    /* this is a normal menu item */
	    child = glade_xml_build_widget (xml, cwinfo);
	    gtk_menu_shell_append (GTK_MENU_SHELL (w), child);
	    continue;
	}
	/* load the template MateUIInfo for this item */
	if (!get_stock_uiinfo (stock_name, &infos[0])) {
	    /* failure ... */
	    if (!strncmp (stock_name, "MATEUIINFO_", 12))
		stock_name += 12;
	    child = gtk_menu_item_new_with_label (stock_name);
	    glade_xml_set_common_params (xml, child, cwinfo);
	    gtk_menu_shell_append (GTK_MENU_SHELL(w), child);
	    continue;
	}
	/* we now have the template for this item.  Now fill it in */
	for (j = 0; j < cwinfo->n_properties; j++) {
	    const char *name  = cwinfo->properties[j].name;
	    const char *value = cwinfo->properties[j].value;
	    if (!strcmp (name, "label"))
		infos[0].label = _(value);
	    else if (!strcmp (name, "tooltip"))
		infos[0].hint = _(value);
	}
	mate_app_fill_menu (GTK_MENU_SHELL(w), infos,
			     glade_xml_ensure_accel(xml), TRUE,
			     i);
	child = infos[0].widget;
	gtk_menu_item_remove_submenu(GTK_MENU_ITEM(child));
	glade_xml_set_common_params(xml, child, cwinfo);
    }

#if 0
    if (uline)
	glade_xml_pop_uline_accel(xml);
#endif
#if 0
    if (strcmp(info->classname, "GtkMenuBar") != 0 &&
	mate_preferences_get_menus_have_tearoff()) {
	GtkWidget *tearoff = gtk_tearoff_menu_item_new();
	
	gtk_menu_prepend(GTK_MENU(w), tearoff);
	gtk_widget_show(tearoff);
    }
#endif
}

static void
mate_add_dock_item (GladeXML *xml, 
		     GtkWidget *parent,
		     GladeWidgetInfo *info,
		     GladeChildInfo *childinfo)
{
	MateComponentDockPlacement placement;
	guint band, offset;
	int position;
	MateComponentDockItemBehavior behavior;
	int i;
	GtkWidget *child;
	GtkWidget *toplevel;

	band = offset = position = 0;
	placement = MATECOMPONENT_DOCK_TOP;
	behavior  = MATECOMPONENT_DOCK_ITEM_BEH_NORMAL;

	for (i = 0; i < childinfo->n_properties; i++) {
		const char *name  = childinfo->properties[i].name;
		const char *value = childinfo->properties[i].value;

		if (!strcmp (name, "placement"))
			placement = glade_enum_from_string (
				MATECOMPONENT_TYPE_DOCK_PLACEMENT,
				value);
		else if (!strcmp (name, "band"))
			band = strtoul (value, NULL, 10);
		else if (!strcmp (name, "position"))
			position = strtol (value, NULL, 10);
		else if (!strcmp (name, "offset"))
			offset = strtoul (value, NULL, 10);
		else if (!strcmp (name, "behavior"))
			behavior = glade_flags_from_string (
				MATECOMPONENT_TYPE_DOCK_ITEM_BEHAVIOR,
				value);
	}

	child = glade_xml_build_widget (xml, childinfo->child);

	toplevel = gtk_widget_get_ancestor (parent, MATE_TYPE_APP);

	matecomponent_dock_item_set_behavior (MATECOMPONENT_DOCK_ITEM (child), behavior);

	if (toplevel != NULL) {
	    mate_app_add_dock_item (
		MATE_APP (toplevel),
		MATECOMPONENT_DOCK_ITEM (child),
		placement, 
		band, 
		position,
		offset);
	} else {
	    matecomponent_dock_add_item (MATECOMPONENT_DOCK (parent),
				  MATECOMPONENT_DOCK_ITEM (child),
				  placement, band, position, offset, 
				  FALSE);
	}
}
				

static void
mate_dock_build_children (GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info)
{
	int i;
	GtkWidget *child;
	GtkWidget *toplevel;
	GladeChildInfo *childinfo;

	toplevel = gtk_widget_get_ancestor (w, MATE_TYPE_APP);

	for (i = 0; i < info->n_children; i++) {
		childinfo = &info->children[i];

		if (!strcmp (childinfo->child->classname, "MateComponentDockItem")) {
			mate_add_dock_item (xml, w, info, childinfo);
			continue;
		}
		
		if (matecomponent_dock_get_client_area (MATECOMPONENT_DOCK (w)))
			g_warning ("Multiple client areas for MateComponentDock found.");
		
		child = glade_xml_build_widget (xml, childinfo->child);
		
		if (toplevel != NULL)
		    mate_app_set_contents (MATE_APP (toplevel), child);
		else
		    matecomponent_dock_set_client_area (MATECOMPONENT_DOCK (w), child);
	}
}

static void
dialog_build_children(GladeXML *self, GtkWidget *w,
		      GladeWidgetInfo *info)

{
    MateDialog *dialog = MATE_DIALOG (w);
    GtkWidget *aa;
    GList *children, *list;
    char *label;

    glade_standard_build_children (self, w, info);

    aa = dialog->action_area;
    if (!aa)
	return;

    children = gtk_container_get_children (GTK_CONTAINER (aa));
    for (list = children; list; list = list->next) {
	gtk_widget_ref (GTK_WIDGET (list->data));
	gtk_container_remove (GTK_CONTAINER (aa), GTK_WIDGET (list->data));
    }

    for (list = children; list; list = list->next) {
	g_object_get (G_OBJECT (list->data), "label", &label, NULL);
	if (label) {
	    mate_dialog_append_button (dialog, label);
	    g_free (label);
	}
    }

    g_list_foreach (children, (GFunc)gtk_widget_unref, NULL);
    g_list_free (children);
}

static void
app_build_children (GladeXML *xml, GtkWidget *parent,
		    GladeWidgetInfo *info)
{
    int i;

    for (i = 0; i < info->n_children; i++) {
	GladeChildInfo *cinfo;
	GtkWidget *child;

	cinfo = &info->children[i];

	if (cinfo->internal_child) {
	    /* not quite proper handling of appbar, but ... */
	    if (!strcmp (cinfo->internal_child, "appbar")) {
		child = glade_xml_build_widget (xml, cinfo->child);
		mate_app_set_statusbar (MATE_APP (parent), child);
	    } else {
		glade_xml_handle_internal_child (xml, parent, cinfo);
	    }
	} else {
	    child = glade_xml_build_widget (xml, cinfo->child);

#if 0
	    g_object_ref (G_OBJECT (child));
	    gtk_widget_freeze_child_notify (child);
	    for (j = 0; j < info->children[i].n_properties; j++)
		glade_xml_set_packing_property (
			xml, MATE_APP (parent)->vbox, child,
			cinfo->properties[j].name,
			cinfo->properties[j].value);
	    gtk_widget_thaw_child_notify(child);
	    g_object_unref(G_OBJECT(child));
#endif
	}
    }
}

static GtkWidget *
app_build (GladeXML *xml, GType widget_type,
	   GladeWidgetInfo *info)
{
    GtkWidget *app;
    char *s;

    app = glade_standard_build_widget (xml, widget_type, info);

    g_object_get (G_OBJECT (mate_program_get ()),
		  MATE_PARAM_APP_ID, &s,
		  NULL);

    g_object_set (G_OBJECT (app), "app_id", s, NULL);

    g_free (s);

    return app;
}

static GtkWidget *
dialog_new (GladeXML *xml, GType widget_type,
	    GladeWidgetInfo *info)
{
    GtkWidget *dialog;
    const char *buttons[] = { NULL };

    dialog = glade_standard_build_widget (xml, widget_type, info);

    mate_dialog_constructv (MATE_DIALOG (dialog), NULL, buttons);

    return dialog;
}


#define DRUID_SET_STRING(pos, POS, var)					    \
static void								    \
druid_page_##pos##_set_##var (GladeXML *xml, GtkWidget *w,		    \
			    const char *name, const char *value)	    \
{									    \
    mate_druid_page_##pos##_set_##var (MATE_DRUID_PAGE_##POS (w), value); \
}

#define DRUID_SET_COLOR(pos, POS, var)						\
static void									\
    druid_page_##pos##_set_##var##_color (GladeXML *xml, GtkWidget *w,		\
			    const char *name, const char *value)		\
{										\
    GdkColor color = { 0 };							\
    if (gdk_color_parse(value, &color) &&					\
	gdk_colormap_alloc_color (gtk_widget_get_default_colormap(),		\
				  &color, FALSE, TRUE)) {			\
    } else {									\
	g_warning ("could not parse color name `%s'", value);			\
	return;									\
    }										\
    mate_druid_page_##pos##_set_##var##_color (MATE_DRUID_PAGE_##POS (w),	\
						&color);			\
}


#define DRUID_SET_IMAGE(pos, POS, image)				\
static void								\
druid_page_##pos##_set_##image(GladeXML *xml, GtkWidget *w,		\
			       const char *name, const char *value)	\
{									\
    GdkPixbuf *image;							\
    char *filename;							\
									\
    filename = glade_xml_relative_file (xml, value);			\
    image = gdk_pixbuf_new_from_file (filename, NULL);			\
    g_free (filename);							\
									\
    mate_druid_page_##pos##_set_##image(MATE_DRUID_PAGE_##POS(w),	\
                                         image);			\
									\
    g_object_unref (G_OBJECT (image));					\
}


DRUID_SET_STRING (edge, EDGE, title)
DRUID_SET_STRING (edge, EDGE, text)
DRUID_SET_COLOR  (edge, EDGE, title)
DRUID_SET_COLOR  (edge, EDGE, text)
DRUID_SET_COLOR  (edge, EDGE, bg)
DRUID_SET_COLOR  (edge, EDGE, logo_bg)
DRUID_SET_COLOR  (edge, EDGE, textbox)
DRUID_SET_IMAGE  (edge, EDGE, logo)
DRUID_SET_IMAGE  (edge, EDGE, watermark)
DRUID_SET_IMAGE  (edge, EDGE, top_watermark)


static GtkWidget *
druid_page_edge_new (GladeXML *xml, GType widget_type,
		     GladeWidgetInfo *info)
{
    GtkWidget *druid;
    MateEdgePosition position = MATE_EDGE_OTHER;
    int i;

    const char *title, *text;
    char *filename;
    GdkPixbuf *logo, *watermark, *top_watermark;

    title = text = NULL;
    logo = watermark = top_watermark = NULL;

    for (i = 0; i < info->n_properties; i++) {
	const char *name  = info->properties[i].name;
	const char *value = info->properties[i].value;	

	if (!strcmp (name, "position"))
	    position = glade_enum_from_string (
		MATE_TYPE_EDGE_POSITION, value);
	else if (!strcmp (name, "text"))
	    text = value;
	else if (!strcmp (name, "title"))
	    title = value;
	else if (!strcmp (name, "logo")) {
	    if (logo)
		g_object_unref (G_OBJECT (logo));
	    filename = glade_xml_relative_file (xml, value);
	    logo = gdk_pixbuf_new_from_file (filename, NULL);
	    g_free (filename);
	} else if (!strcmp (name, "watermark")) {
	    if (watermark)
		g_object_unref (G_OBJECT (watermark));
	    filename = glade_xml_relative_file (xml, value);
	    watermark = gdk_pixbuf_new_from_file (filename, NULL);
	    g_free (filename);
	} else if (!strcmp (name, "top_watermark")) {
	    if (top_watermark)
		g_object_unref (G_OBJECT (top_watermark));
	    filename = glade_xml_relative_file (xml, value);
	    top_watermark = gdk_pixbuf_new_from_file (filename, NULL);
	    g_free (filename);
	}

    }

    druid = glade_standard_build_widget (xml, widget_type, info);

    mate_druid_page_edge_construct (
	MATE_DRUID_PAGE_EDGE (druid),
	position, TRUE, title, text,
	logo, watermark, top_watermark);

    if (logo)
	g_object_unref (G_OBJECT (logo));
    if (watermark)
	g_object_unref (G_OBJECT (watermark));
    if (top_watermark)
	g_object_unref (G_OBJECT (top_watermark));

    return druid;
}

static GtkWidget *
message_box_new (GladeXML *xml, GType widget_type,
		 GladeWidgetInfo *info)
{
    GtkWidget *dialog;
    const char *buttons[] = { NULL };
    const char *type = "generic", *message = NULL;
    int i;

    for (i = 0; i < info->n_properties; i++) {
	const char *name  = info->properties[i].name;
	const char *value = info->properties[i].value;	

	if (!strcmp (name, "message"))
	    message = value;
	if (!strcmp (name, "message_box_type"))
	    type = value;
    }

    dialog = glade_standard_build_widget (xml, widget_type, info);

    mate_message_box_construct (MATE_MESSAGE_BOX (dialog), message, type, buttons);

    return dialog;
}

static GtkWidget *
date_edit_new (GladeXML *xml, GType widget_type,
	       GladeWidgetInfo *info)
{
    GtkWidget *de;

    de = glade_standard_build_widget (xml, widget_type, info);

    g_object_set (G_OBJECT (de), "time", time (NULL), NULL);

    return de;
}

static GtkWidget *
icon_list_new (GladeXML *xml, GType widget_type,
	       GladeWidgetInfo *info)
{
    GtkWidget *gil;
    int flags = 0;
    int icon_width = 0;
    int i;

    for (i = 0; i < info->n_properties; i++) {
	const char *name  = info->properties[i].name;
	const char *value = info->properties[i].value;

	if (!strcmp (name, "text_editable")) {
	    if (BOOL (value))
		flags |= MATE_ICON_LIST_IS_EDITABLE;
	} else if (!strcmp (name, "text_static")) {
	    if (BOOL (value))
		flags |= MATE_ICON_LIST_STATIC_TEXT;
	} else if (!strcmp (name, "icon_width")) {
	    icon_width = INT (value);
	}
    }

    gil = glade_standard_build_widget (xml, widget_type, info);

    mate_icon_list_construct (MATE_ICON_LIST (gil),
			       icon_width, NULL, flags);

    return gil;
}

static GtkWidget *
druidpagestandard_find_internal_child(GladeXML *xml, GtkWidget *parent,
				      const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return MATE_DRUID_PAGE_STANDARD(parent)->vbox;
    return NULL;
}

static GtkWidget *
dialog_find_internal_child(GladeXML *xml, GtkWidget *parent,
			   const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return MATE_DIALOG(parent)->vbox;
    else if (!strcmp (childname, "action_area"))
	return MATE_DIALOG (parent)->action_area;

    return NULL;
}

static GtkWidget *
entry_find_internal_child(GladeXML *xml, GtkWidget *parent,
			  const gchar *childname)
{
    if (!strcmp (childname, "entry"))
	return mate_entry_gtk_entry (MATE_ENTRY (parent));

    return NULL;
}

static GtkWidget *
file_entry_find_internal_child(GladeXML *xml, GtkWidget *parent,
			      const gchar *childname)
{
    if (!strcmp (childname, "entry"))
	return mate_file_entry_gtk_entry (MATE_FILE_ENTRY (parent));

    return NULL;
}

static GtkWidget *
propertybox_find_internal_child(GladeXML *xml, GtkWidget *parent,
				const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return MATE_DIALOG(parent)->vbox;
    else if (!strcmp(childname, "action_area"))
	return MATE_DIALOG(parent)->action_area;
    else if (!strcmp(childname, "notebook"))
	return MATE_PROPERTY_BOX(parent)->notebook;
    else if (!strcmp(childname, "ok_button"))
	return MATE_PROPERTY_BOX(parent)->ok_button;
    else if (!strcmp(childname, "apply_button"))
	return MATE_PROPERTY_BOX(parent)->apply_button;
    else if (!strcmp(childname, "cancel_button"))
	return MATE_PROPERTY_BOX(parent)->cancel_button;
    else if (!strcmp(childname, "help_button"))
	return MATE_PROPERTY_BOX(parent)->help_button;
    return NULL;
}

static GtkWidget *
app_find_internal_child (GladeXML *xml, GtkWidget *parent,
			 const char *childname)
{
    if (!strcmp (childname, "dock"))
	return MATE_APP (parent)->dock;
#if 0
    else if (!strcmp (childname, "appbar"))
	return MATE_APP (parent)->statusbar;
#endif

    return NULL;
}

static void
app_enable_layout_config (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    mate_app_enable_layout_config (MATE_APP (w), BOOL (value));
}

static void
pixmap_entry_set_preview (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    mate_pixmap_entry_set_preview (MATE_PIXMAP_ENTRY (w), BOOL (value));
}

static void
icon_list_set_selection_mode (GladeXML *xml, GtkWidget *w,
			      const char *name, const char *value)
{
    mate_icon_list_set_selection_mode (
	MATE_ICON_LIST (w),
	glade_enum_from_string (GTK_TYPE_SELECTION_MODE,
				value));
}

static void
entry_set_max_saved (GladeXML *xml, GtkWidget *w,
		     const char *name, const char *value)
{
    mate_entry_set_max_saved (MATE_ENTRY (w), INT (value));
}

static void
file_entry_set_max_saved (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    entry_set_max_saved (xml, 
			 mate_file_entry_mate_entry (MATE_FILE_ENTRY (w)),
			 name, value);
}

static void
file_entry_set_use_filechooser (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    g_object_set (G_OBJECT (w), "use_filechooser", BOOL (value), NULL);
}

static void
icon_entry_set_max_saved (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    mate_icon_entry_set_max_saved (MATE_ICON_ENTRY (w), INT (value));
}

static void
icon_list_set_row_spacing (GladeXML *xml, GtkWidget *w,
			   const char *name, const char *value)
{
    mate_icon_list_set_row_spacing (MATE_ICON_LIST (w), INT (value));
}

static void
icon_list_set_col_spacing (GladeXML *xml, GtkWidget *w,
			   const char *name, const char *value)
{
    mate_icon_list_set_col_spacing (MATE_ICON_LIST (w), INT (value));
}

static void
icon_list_set_text_spacing (GladeXML *xml, GtkWidget *w,
			    const char *name, const char *value)
{
    mate_icon_list_set_text_spacing (MATE_ICON_LIST (w), INT (value));
}

static void
custom_noop (GladeXML *xml, GtkWidget *w,
	     const char *name, const char *value)
{
    ;
}

static void
dialog_set_auto_close (GladeXML *xml, GtkWidget *w,
		       const char *name, const char *value)
{
    mate_dialog_set_close (MATE_DIALOG (w), BOOL (value));
}

static void
dialog_set_hide_on_close (GladeXML *xml, GtkWidget *w,
			  const char *name, const char *value)
{
    mate_dialog_close_hides (MATE_DIALOG (w), BOOL (value));
}

static void
about_set_authors (GladeXML *xml, GtkWidget *w,
		   const char *name, const char *value)
{
    char **authors;
    GValueArray *authors_array;
    int i;

    authors = g_strsplit (value, "\n", 0);
    authors_array = g_value_array_new (0);
	
    for (i = 0; authors[i] != NULL; i++) {
	GValue value = { 0 };
		g_value_init (&value, G_TYPE_STRING);
	g_value_set_static_string (&value, authors[i]);
	authors_array = g_value_array_append (authors_array, &value);
    }

    g_object_set (G_OBJECT (w), "authors", authors_array, NULL);

    g_value_array_free (authors_array);

    g_strfreev (authors);
}

static void
about_set_translator_credits (GladeXML *xml, GtkWidget *w,
			      const char *name, const char *value)
{
    /* only set this if the translator actually translated the string. */
    if (strcmp(value, "translator_credits") != 0) {
	g_object_set (G_OBJECT (w), "translator_credits", value, NULL);
    }
}

static void
about_set_documentors (GladeXML *xml, GtkWidget *w,
		       const char *name, const char *value)
{
    char **documentors;
    GValueArray *documentors_array;
    int i;

    documentors = g_strsplit (value, "\n", 0);
    documentors_array = g_value_array_new (0);
	
    for (i = 0; documentors[i] != NULL; i++) {
	GValue value = { 0 };
		g_value_init (&value, G_TYPE_STRING);
	g_value_set_static_string (&value, documentors[i]);
	documentors_array = g_value_array_append (documentors_array, &value);
    }

    g_object_set (G_OBJECT (w), "documenters", documentors_array, NULL);

    g_value_array_free (documentors_array);

    g_strfreev (documentors);
}

static void
pixmap_set_filename (GladeXML *xml, GtkWidget *w,
		     const char *name, const char *value)
{
    char *filename;

    filename = glade_xml_relative_file (xml, value);
    mate_pixmap_load_file (MATE_PIXMAP (w), filename);
    g_free (filename);
}

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
    glade_require ("matecomponent");

    glade_register_custom_prop (MATE_TYPE_APP, "enable_layout_config", app_enable_layout_config);
    glade_register_custom_prop (MATE_TYPE_PIXMAP_ENTRY, "preview", pixmap_entry_set_preview);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "selection_mode", icon_list_set_selection_mode);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "icon_width", custom_noop);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "row_spacing", icon_list_set_row_spacing);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "column_spacing", icon_list_set_col_spacing);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "text_spacing", icon_list_set_text_spacing);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "text_editable", custom_noop);
    glade_register_custom_prop (MATE_TYPE_ICON_LIST, "text_static", custom_noop);
    glade_register_custom_prop (MATE_TYPE_DIALOG, "auto_close", dialog_set_auto_close);
    glade_register_custom_prop (MATE_TYPE_DIALOG, "hide_on_close", dialog_set_hide_on_close);
    glade_register_custom_prop (MATE_TYPE_MESSAGE_BOX, "message", custom_noop);
    glade_register_custom_prop (MATE_TYPE_MESSAGE_BOX, "message_box_type", custom_noop);
    glade_register_custom_prop (MATE_TYPE_ABOUT, "authors", about_set_authors);
    glade_register_custom_prop (MATE_TYPE_ABOUT, "translator_credits", about_set_translator_credits);
    glade_register_custom_prop (MATE_TYPE_ABOUT, "documenters", about_set_documentors);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "title", druid_page_edge_set_title);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "text", druid_page_edge_set_text);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "title_color", druid_page_edge_set_title_color);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "text_color", druid_page_edge_set_text_color);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "background_color", druid_page_edge_set_bg_color);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "logo_background_color", druid_page_edge_set_logo_bg_color);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "textbox_color", druid_page_edge_set_textbox_color);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "logo", druid_page_edge_set_logo);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "watermark", druid_page_edge_set_watermark);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "top_watermark", druid_page_edge_set_top_watermark);
    glade_register_custom_prop (MATE_TYPE_DRUID_PAGE_EDGE, "position", custom_noop);
    glade_register_custom_prop (GTK_TYPE_IMAGE_MENU_ITEM, "stock_item", custom_noop);
    glade_register_custom_prop (GTK_TYPE_MENU_ITEM, "stock_item", custom_noop);
    glade_register_custom_prop (MATE_TYPE_PIXMAP, "filename", pixmap_set_filename);
    glade_register_custom_prop (MATE_TYPE_ENTRY, "max_saved", entry_set_max_saved);
    glade_register_custom_prop (MATE_TYPE_FILE_ENTRY, "max_saved", file_entry_set_max_saved);
    glade_register_custom_prop (MATE_TYPE_FILE_ENTRY, "use_filechooser", file_entry_set_use_filechooser);
    glade_register_custom_prop (MATE_TYPE_ICON_ENTRY, "max_saved", icon_entry_set_max_saved);

    glade_register_widget (MATE_TYPE_ABOUT, NULL, NULL, NULL);
    glade_register_widget (MATE_TYPE_APP, app_build, app_build_children,
			   app_find_internal_child);
    glade_register_widget (MATE_TYPE_APPBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_COLOR_PICKER,glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_DATE_EDIT, date_edit_new,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_DIALOG, dialog_new,
			   dialog_build_children, dialog_find_internal_child);
    glade_register_widget (MATE_TYPE_DRUID, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (MATE_TYPE_DRUID_PAGE, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (MATE_TYPE_DRUID_PAGE_EDGE, druid_page_edge_new,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_DRUID_PAGE_STANDARD, glade_standard_build_widget,
			   glade_standard_build_children, druidpagestandard_find_internal_child);
    glade_register_widget (MATE_TYPE_ENTRY, glade_standard_build_widget,
			   glade_standard_build_children, entry_find_internal_child);
    glade_register_widget (MATE_TYPE_FILE_ENTRY, glade_standard_build_widget,
			   glade_standard_build_children, file_entry_find_internal_child);
    glade_register_widget (MATE_TYPE_HREF, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_ICON_ENTRY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_ICON_LIST, icon_list_new,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_ICON_SELECTION, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_MESSAGE_BOX, message_box_new,
			   glade_standard_build_children, dialog_find_internal_child);
    glade_register_widget (MATE_TYPE_PIXMAP_ENTRY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_PROPERTY_BOX, glade_standard_build_widget,
			   glade_standard_build_children, propertybox_find_internal_child);
    glade_register_widget (MATE_TYPE_SCORES, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (MATE_TYPE_PIXMAP, glade_standard_build_widget, NULL, NULL);
    glade_register_widget (MATE_TYPE_FONT_PICKER, NULL, NULL, NULL);

    /* things we need to override */
    glade_register_widget (MATECOMPONENT_TYPE_DOCK, NULL, mate_dock_build_children, NULL);
    glade_register_widget (GTK_TYPE_MENU_BAR, NULL, menushell_build_children, NULL);
    glade_register_widget (GTK_TYPE_MENU, NULL, menushell_build_children, NULL);
    
    glade_provide ("mate");
}
