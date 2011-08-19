/*
 * glade-matecomponent.c: support for matecomponent widgets in libglade.
 *
 * Authors:
 *      Michael Meeks (michael@ximian.com)
 *      Jacob Berkman (jacob@ximian.com>
 *
 * Copyright (C) 2000,2001 Ximian, Inc., 2001,2002 James Henstridge.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <libmatecomponentui.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>

#define INT(s)   (strtol ((s), NULL, 0))
#define UINT(s)  (strtoul ((s), NULL, 0))
#define BOOL(s)  (g_ascii_tolower (*(s)) == 't' || g_ascii_tolower (*(s)) == 'y' || INT (s))
#define FLOAT(s) (g_strtod ((s), NULL))

static void
dock_allow_floating (GladeXML *xml, GtkWidget *widget,
		     const char *name, const char *value)
{
	matecomponent_dock_allow_floating_items (MATECOMPONENT_DOCK (widget), BOOL (value));
}

static void
dock_item_set_shadow_type (GladeXML *xml, GtkWidget *widget,
			   const char *name, const char *value)
{
	matecomponent_dock_item_set_shadow_type (
		MATECOMPONENT_DOCK_ITEM (widget),
		glade_enum_from_string (GTK_TYPE_SHADOW_TYPE, value));
}

static void
dock_item_set_behavior (GladeXML *xml, GtkWidget *widget,
			const char *name, const char *value)
{
	MateComponentDockItem *dock_item = MATECOMPONENT_DOCK_ITEM (widget);
	gchar *old_name;

	old_name = dock_item->name;
	dock_item->name = NULL;
	matecomponent_dock_item_construct (dock_item, old_name,
				    glade_flags_from_string (
					MATECOMPONENT_TYPE_COMPONENT_DOCK_ITEM_BEHAVIOR,
					value));
	g_free (old_name);
}

static GtkWidget *
dock_item_build (GladeXML *xml, GType widget_type,
		 GladeWidgetInfo *info)
{
	GtkWidget *w;

	w = glade_standard_build_widget (xml, widget_type, info);

	g_free(MATECOMPONENT_DOCK_ITEM (w)->name);
	MATECOMPONENT_DOCK_ITEM (w)->name = g_strdup (info->name);

	return w;
}


static GtkWidget *
glade_matecomponent_widget_new (GladeXML        *xml,
			 GType            widget_type,
			 GladeWidgetInfo *info)
{
	const gchar *control_moniker = NULL;
	GtkWidget *widget;
	GObjectClass *oclass;
	MateComponentControlFrame *cf;
	MateComponent_PropertyBag pb;
	gint i;

	for (i = 0; i < info->n_properties; i++) {
		if (!strcmp (info->properties[i].name, "moniker")) {
			control_moniker = info->properties[i].value;
			break;
		}
	}

	if (!control_moniker) {
		g_warning (G_STRLOC " MateComponentWidget doesn't have moniker property");
		return NULL;
	}
	widget = matecomponent_widget_new_control (
		control_moniker, CORBA_OBJECT_NIL);

	if (!widget) {
		g_warning (G_STRLOC " unknown matecomponent control '%s'", control_moniker);
		return NULL;
	}

	oclass = G_OBJECT_GET_CLASS (widget);

	cf = matecomponent_widget_get_control_frame (MATECOMPONENT_WIDGET (widget));

	if (!cf) {
		g_warning ("control '%s' has no frame", control_moniker);
		gtk_widget_unref (widget);
		return NULL;
	}

	pb = matecomponent_control_frame_get_control_property_bag (cf, NULL);
	if (pb == CORBA_OBJECT_NIL)
		return widget;

	for (i = 0; i < info->n_properties; i++) {
		const gchar *name = info->properties[i].name;
		const gchar *value = info->properties[i].value;
		GParamSpec *pspec;

		if (!strcmp (name, "moniker"))
			continue;

		pspec = g_object_class_find_property (oclass, name);
		if (pspec) {
			GValue gvalue = { 0 };

			if (glade_xml_set_value_from_string(xml, pspec, value, &gvalue)) {
				g_object_set_property(G_OBJECT(widget), name, &gvalue);
				g_value_unset(&gvalue);
			}
		} else if (pb != CORBA_OBJECT_NIL) {
			CORBA_TypeCode tc =
				matecomponent_property_bag_client_get_property_type (pb, name, NULL);

			switch (tc->kind) {
			case CORBA_tk_boolean:
				matecomponent_property_bag_client_set_value_gboolean (pb, name,
									       value[0] == 'T' || value[0] == 'y', NULL);
				break;
			case CORBA_tk_string:
				matecomponent_property_bag_client_set_value_string (pb, name, value,
									     NULL);
				break;
			case CORBA_tk_long:
				matecomponent_property_bag_client_set_value_glong (pb, name,
									    strtol (value, NULL,0), NULL);
				break;
			case CORBA_tk_float:
				matecomponent_property_bag_client_set_value_gfloat (pb, name,
									     strtod (value, NULL), NULL);
				break;
			case CORBA_tk_double:
				matecomponent_property_bag_client_set_value_gdouble (pb, name,
									      strtod (value, NULL),
									      NULL);
				break;
			default:
				g_warning ("Unhandled type %d for `%s'", tc->kind, name);
				break;
			}
		} else {
			g_warning ("could not handle property `%s'", name);
		}
	}

	matecomponent_object_release_unref (pb, NULL);

	return widget;
}

static GtkWidget *
matecomponent_window_find_internal_child (GladeXML    *xml,
				   GtkWidget   *parent,
				   const gchar *childname)
{
	if (!strcmp (childname, "vbox")) {
		GtkWidget *ret;

		if ((ret = matecomponent_window_get_contents (
			MATECOMPONENT_WINDOW (parent))))
			return ret;

		else {
			GtkWidget *box;

			box = gtk_vbox_new (FALSE, 0);
			
			matecomponent_window_set_contents (
				MATECOMPONENT_WINDOW (parent), box);

			return box;
		}
	}

    return NULL;
}

static void
add_dock_item (GladeXML *xml, 
	       GtkWidget *parent,
	       GladeWidgetInfo *info,
	       GladeChildInfo *childinfo)
{
	MateComponentDockPlacement placement;
	guint band, offset;
	int position;
	int i;
	GtkWidget *child;
	
	band = offset = position = 0;
	placement = MATECOMPONENT_DOCK_TOP;
	
	for (i = 0; i < childinfo->n_properties; i++) {
		const char *name  = childinfo->properties[i].name;
		const char *value = childinfo->properties[i].value;
		
		if (!strcmp (name, "placement"))
			placement = glade_enum_from_string (
				MATECOMPONENT_TYPE_COMPONENT_DOCK_PLACEMENT,
				value);
		else if (!strcmp (name, "band"))
			band = UINT (value);
		else if (!strcmp (name, "position"))
			position = INT (value);
		else if (!strcmp (name, "offset"))
			offset = UINT (value);
	}

	child = glade_xml_build_widget (xml, childinfo->child);

	matecomponent_dock_add_item (MATECOMPONENT_DOCK (parent),
			      MATECOMPONENT_DOCK_ITEM (child),
			      placement, band, position, offset, 
			      FALSE);
}
				

static void
dock_build_children (GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info)
{
	int i;
	GtkWidget *child;
	GladeChildInfo *childinfo;

	for (i = 0; i < info->n_children; i++) {
		childinfo = &info->children[i];

		if (!strcmp (childinfo->child->classname, "MateComponentDockItem")) {
			add_dock_item (xml, w, info, childinfo);
			continue;
		}
		
		if (matecomponent_dock_get_client_area (MATECOMPONENT_DOCK (w)))
			g_warning ("Multiple client areas for MateComponentDock found.");
		
		child = glade_xml_build_widget (xml, childinfo->child);
		matecomponent_dock_set_client_area (MATECOMPONENT_DOCK (w), child);
	}
}

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
	glade_require ("gtk");

	glade_register_custom_prop (MATECOMPONENT_TYPE_DOCK, "allow_floating", dock_allow_floating);
	glade_register_custom_prop (MATECOMPONENT_TYPE_DOCK_ITEM, "shadow_type", dock_item_set_shadow_type);
	glade_register_custom_prop (MATECOMPONENT_TYPE_DOCK_ITEM, "behavior", dock_item_set_behavior);

	glade_register_widget (MATECOMPONENT_TYPE_WIDGET,
			       glade_matecomponent_widget_new,
			       NULL, NULL);
	glade_register_widget (MATECOMPONENT_TYPE_WINDOW,
			       NULL, glade_standard_build_children,
			       matecomponent_window_find_internal_child);
	glade_register_widget (MATECOMPONENT_TYPE_DOCK,
			       NULL, dock_build_children,
			       NULL);
	glade_register_widget (MATECOMPONENT_TYPE_DOCK_ITEM,
			       dock_item_build, glade_standard_build_children, NULL);
	glade_provide ("matecomponent");
}
