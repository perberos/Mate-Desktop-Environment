/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-sync-toolbar.h: The MateComponent UI/XML sync engine for toolbars
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-engine-config.h>
#include <matecomponent/matecomponent-ui-sync.h>
#include <matecomponent/matecomponent-ui-sync-toolbar.h>
#include <matecomponent/matecomponent-ui-preferences.h>
#include <matecomponent/matecomponent-ui-private.h>

#include <matecomponent/matecomponent-ui-toolbar.h>
#include <matecomponent/matecomponent-ui-toolbar-button-item.h>
#include <matecomponent/matecomponent-ui-toolbar-toggle-button-item.h>
#include <matecomponent/matecomponent-ui-toolbar-popup-item.h>
#include <matecomponent/matecomponent-ui-toolbar-control-item.h>

static GObjectClass *parent_class = NULL;

static GQuark dockitem_id = 0;
static GQuark toolitem_id = 0;

#define PARENT_TYPE matecomponent_ui_sync_get_type ()

static MateComponentUIToolbarControlDisplay
decode_control_disp (const char *txt)
{
	if (!txt || !strcmp (txt, "control"))
		return MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL;

	else if (!strcmp (txt, "button"))
		return MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_BUTTON;

	else if (!strcmp (txt, "none"))
		return MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_NONE;

	else
		return MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL;
}

static gboolean 
string_array_contains (char **str_array, const char *match)
{
	int i = 0;
	char *string;

	while ((string = str_array [i++]))
		if (strcmp (string, match) == 0)
			return TRUE;

	return FALSE;
}

static void
impl_matecomponent_ui_sync_toolbar_state (MateComponentUISync     *sync,
				   MateComponentUINode     *node,
				   MateComponentUINode     *cmd_node,
				   GtkWidget        *widget,
				   GtkWidget        *parent)
{
	char *type, *label, *txt;
	char *behavior;
	char **behavior_array;
	gboolean priority;

	/* FIXME: to debug control problem */
	gtk_widget_show (widget);

	if ((behavior = matecomponent_ui_engine_get_attr (node, cmd_node, "behavior"))) {
		
		behavior_array = g_strsplit (behavior, ",", -1);
		matecomponent_ui_node_free_string (behavior);

		if (GTK_IS_TOOL_ITEM (widget)) {
			gtk_tool_item_set_expand
				(GTK_TOOL_ITEM (widget),
				 string_array_contains (behavior_array, "expandable"));
			
			/* The pack-end behavior is deprecated. You can use
			   <separator type="space" behavior="expanded"/> to
			   achieve the same result */
		} else {
			matecomponent_ui_toolbar_item_set_expandable
				(MATECOMPONENT_UI_TOOLBAR_ITEM (widget),
				 string_array_contains (behavior_array, "expandable"));

			matecomponent_ui_toolbar_item_set_pack_end
				(MATECOMPONENT_UI_TOOLBAR_ITEM (widget),
				 string_array_contains (behavior_array, "pack-end"));
		}

		g_strfreev (behavior_array);
	}

	if ((txt = matecomponent_ui_engine_get_attr (node, cmd_node, "priority"))) {
		priority = atoi (txt);
		matecomponent_ui_node_free_string (txt);
	} else
		priority = FALSE;

	if (GTK_IS_TOOL_ITEM (widget))
		gtk_tool_item_set_is_important (GTK_TOOL_ITEM (widget), priority);

	type  = matecomponent_ui_engine_get_attr (node, cmd_node, "type");
	label = matecomponent_ui_engine_get_attr (node, cmd_node, "label");
	
	if (GTK_IS_TOOL_BUTTON (widget)) {
		GtkToolButton *button_item = (GtkToolButton *) widget;

		if ((matecomponent_ui_node_peek_attr (node, "pixtype") ||
		     matecomponent_ui_node_peek_attr (cmd_node, "pixtype"))) {
			GtkWidget *image;

			if (!(image = gtk_tool_button_get_icon_widget (button_item))) {
				image = gtk_image_new ();
				gtk_tool_button_set_icon_widget (button_item, image);
			}

			matecomponent_ui_util_xml_set_image (
				GTK_IMAGE (image), node, cmd_node,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
			gtk_widget_show (image);
		} else
			gtk_tool_button_set_icon_widget (button_item, NULL);

		if (label) {
			GtkLabel *label_widget;

			if (!(label_widget = GTK_LABEL (gtk_tool_button_get_label_widget (button_item)))) {
				label_widget = GTK_LABEL (gtk_label_new (NULL));
				gtk_tool_button_set_label_widget (button_item, GTK_WIDGET (label_widget));
			}

			if (!label_widget->label || /* optimise, we hit here a lot */
			    strcmp (label, label_widget->label))
				gtk_label_set_text (label_widget, label);
			gtk_widget_show (GTK_WIDGET (label_widget));
		} else
			gtk_tool_button_set_label_widget (button_item, NULL);
	}

	matecomponent_ui_node_free_string (type);
	matecomponent_ui_node_free_string (label);

	if (matecomponent_ui_node_has_name (node, "control") &&
	    MATECOMPONENT_IS_UI_TOOLBAR_CONTROL_ITEM (widget)) {
		const char *text;
		MateComponentUIToolbarControlDisplay hdisp, vdisp;
		
		text = matecomponent_ui_node_peek_attr (node, "hdisplay");
		hdisp = decode_control_disp (text);

		text = matecomponent_ui_node_peek_attr (node, "vdisplay");
		vdisp = decode_control_disp (text);

		matecomponent_ui_toolbar_control_item_set_display (
			MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM (widget), hdisp, vdisp);
	}
	
	if ((txt = matecomponent_ui_engine_get_attr (node, cmd_node, "tip"))) {
		if (GTK_IS_TOOL_ITEM (widget))
			gtk_tool_item_set_tooltip
				(GTK_TOOL_ITEM (widget),
				 GTK_TOOLBAR (parent)->tooltips,
				 txt, NULL);
		else
			matecomponent_ui_toolbar_item_set_tooltip
				(MATECOMPONENT_UI_TOOLBAR_ITEM (widget),
				 (GTK_TOOLBAR (parent)->tooltips), txt);

		matecomponent_ui_node_free_string (txt);
	}

	matecomponent_ui_engine_queue_update (
		sync->engine, widget, node, cmd_node);
}

static gint
exec_verb_cb (GtkWidget *item, MateComponentUIEngine *engine)
{
	matecomponent_ui_engine_emit_verb_on_w (engine, item);

	return FALSE;
}

static gint
win_item_emit_ui_event (GtkToggleToolButton *toggle,
			MateComponentUIEngine      *engine)
{
	MateComponentUINode *node;
	const char   *state;

	node = matecomponent_ui_engine_widget_get_node (GTK_WIDGET (toggle));

	g_return_val_if_fail (node != NULL, FALSE);

	state = gtk_toggle_tool_button_get_active (toggle) ? "1": "0";
	matecomponent_ui_engine_emit_event_on (engine, node, state);

	return FALSE;
}

static GtkWidget *
toolbar_build_control (MateComponentUISync     *sync,
		       MateComponentUINode     *node,
		       MateComponentUINode     *cmd_node,
		       int              *pos,
		       GtkWidget        *parent)
{
	GtkWidget  *matecomponent_item = NULL;

	g_return_val_if_fail (sync != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);
	
	if ((matecomponent_item = matecomponent_ui_engine_node_get_widget (
		sync->engine, node))) {
		
		g_assert (matecomponent_item->parent == NULL);
		if (!GTK_IS_TOOL_ITEM (matecomponent_item))
			g_warning ("Serious oddness not a toolbar item: '%s'",
				   g_type_name_from_instance ((GTypeInstance *)matecomponent_item));
	} else {
		MateComponent_Control control;

		control = matecomponent_ui_engine_node_get_object (
			sync->engine, node);

		if (control != CORBA_OBJECT_NIL) {
			matecomponent_item = matecomponent_ui_toolbar_control_item_new (control);

			if (!matecomponent_item)
				return NULL;

			matecomponent_ui_engine_stamp_custom (
				sync->engine, node);
		} else
			return NULL;
	}

	gtk_toolbar_insert (GTK_TOOLBAR (parent),
			    GTK_TOOL_ITEM (matecomponent_item),
			    (*pos)++);

	gtk_widget_show (matecomponent_item);

	return matecomponent_item;
}

static GtkWidget *
toolbar_build_widget (MateComponentUISync *sync,
		      MateComponentUINode *node,
		      MateComponentUINode *cmd_node,
		      int          *pos,
		      GtkWidget    *parent)
{
	char         *type, *stock_id;
	GtkWidget    *tool_item;

	g_return_val_if_fail (sync != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	type = matecomponent_ui_engine_get_attr (node, cmd_node, "type");

	if ((stock_id = matecomponent_ui_engine_get_attr (node, cmd_node, "stockid"))) {
		GtkStockItem  stock_item;
		GtkIconSet   *icon_set;

		if (!gtk_stock_lookup (stock_id, &stock_item))
			g_warning ("Unknown stock id '%s' on %s", stock_id,
				   matecomponent_ui_xml_make_path (node));
		else {
			gchar *label;
			int i, len;

			label = g_strdup (dgettext (stock_item.translation_domain, stock_item.label));

			len = strlen (label);
			for (i = 0; i < len; i++) {
				if (label [i] == '_') {
					memmove (label+i, label+i+1, strlen (label+i+1) + 1);
					len--;
				}
			}

			matecomponent_ui_node_set_attr (node, "label", label);

			g_free (label);
		}

		icon_set = gtk_icon_factory_lookup_default (stock_id);

		if (icon_set) {
			matecomponent_ui_node_set_attr (node, "pixtype", "stock");
			matecomponent_ui_node_set_attr (node, "pixname", stock_id);
		}
	}

	if (matecomponent_ui_node_has_name (node, "separator")) {
		tool_item = GTK_WIDGET (gtk_separator_tool_item_new ());

		if (type && !strcmp (type, "space")) {
			gtk_separator_tool_item_set_draw
				(GTK_SEPARATOR_TOOL_ITEM (tool_item), FALSE);
		}
	}

	else if (!type)
		tool_item = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));

	else if (!strcmp (type, "toggle"))
		tool_item = GTK_WIDGET (gtk_toggle_tool_button_new ());

	else {
		g_warning ("Invalid type '%s'", type);
		return NULL;
	}

	matecomponent_ui_node_free_string (type);
	
	gtk_toolbar_insert (GTK_TOOLBAR (parent),
			    GTK_TOOL_ITEM (tool_item),
			    (*pos)++);

	gtk_widget_show (tool_item);

	return tool_item;
}

static GtkWidget *
impl_matecomponent_ui_sync_toolbar_build (MateComponentUISync     *sync,
				   MateComponentUINode     *node,
				   MateComponentUINode     *cmd_node,
				   int              *pos,
				   GtkWidget        *parent)
{
	GtkWidget *widget;
	char      *verb;
	
	if (matecomponent_ui_node_has_name (node, "control"))
		widget = toolbar_build_control (
			sync, node, cmd_node, pos, parent);
	else
		widget = toolbar_build_widget (
			sync, node, cmd_node, pos, parent);

	if (widget) {
		if ((verb = matecomponent_ui_engine_get_attr (node, NULL, "verb"))) {
			g_signal_connect (widget, "clicked",
					  G_CALLBACK (exec_verb_cb),
					  sync->engine);
			matecomponent_ui_node_free_string (verb);
		}

		if (GTK_IS_TOGGLE_TOOL_BUTTON (widget))
			g_signal_connect (widget, "toggled",
					  G_CALLBACK (win_item_emit_ui_event),
					  sync->engine);
	}

	return widget;
}

static GtkWidget *
impl_matecomponent_ui_sync_toolbar_build_placeholder (MateComponentUISync     *sync,
					       MateComponentUINode     *node,
					       MateComponentUINode     *cmd_node,
					       int              *pos,
					       GtkWidget        *parent)
{
	GtkToolItem *item;

	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert (GTK_TOOLBAR (parent), item, (*pos)++);

	return GTK_WIDGET (item);
}

static MateComponentDockItem *
get_dock_item (MateComponentUISyncToolbar *sync,
	       const char          *dockname)
{
	guint dummy;
	MateComponentDockPlacement *dummy_placement = NULL;
	
	g_return_val_if_fail (dockname != NULL, NULL);

	return matecomponent_dock_get_item_by_name (sync->dock,
					    dockname,
					    dummy_placement, &dummy,
					    &dummy, &dummy);
}

static GList *
impl_matecomponent_ui_sync_toolbar_get_widgets (MateComponentUISync *sync,
					 MateComponentUINode *node)
{
	const char     *dockname;
	MateComponentDockItem *item;

	dockname = matecomponent_ui_node_peek_attr (node, "name");
	item = get_dock_item (MATECOMPONENT_UI_SYNC_TOOLBAR (sync), dockname);

	if (!item) {
		g_warning ("Serious internal error building toolbar");
		return NULL;
	}

	return matecomponent_ui_internal_toolbar_get_children (matecomponent_dock_item_get_child (item));
}

static void
impl_matecomponent_ui_sync_toolbar_state_update (MateComponentUISync *sync,
					  GtkWidget    *widget,
					  const char   *new_state)
{
	g_return_if_fail (widget != NULL);

	if (new_state) {
		if (GTK_IS_TOGGLE_TOOL_BUTTON (widget)) {
			GtkToggleToolButton *button = (GtkToggleToolButton *) widget;
			
			g_signal_handlers_block_by_func
				(widget, G_CALLBACK (win_item_emit_ui_event), sync->engine);

			gtk_toggle_tool_button_set_active (button, atoi (new_state));
			
			g_signal_handlers_unblock_by_func
				(widget, G_CALLBACK (win_item_emit_ui_event), sync->engine);
		} 
		else
			g_warning ("Toolbar: strange, setting "
				   "state '%s' on weird object '%s'",
				   new_state,
				   g_type_name_from_instance (
					   (GTypeInstance *) widget));
	}
}

static void
impl_dispose (GObject *object)
{
	MateComponentUISyncToolbar *sync = (MateComponentUISyncToolbar *) object;

	if (sync->dock) {
		g_object_unref (sync->dock);
		sync->dock = NULL;
	}

	parent_class->dispose (object);
}

static gboolean
impl_matecomponent_ui_sync_toolbar_ignore_widget (MateComponentUISync *sync,
					   GtkWidget    *widget)
{
	return MATECOMPONENT_IS_UI_TOOLBAR_POPUP_ITEM (widget);
}

static GtkToolbarStyle
parse_look (const char *look)
{
	if (look) {
		if (!strcmp (look, "both"))
			return GTK_TOOLBAR_BOTH;
		if (!strcmp (look, "icon"))
			return GTK_TOOLBAR_ICONS;
		if (!strcmp (look, "text"))
			return GTK_TOOLBAR_TEXT;
		if (!strcmp (look, "both_horiz"))
			return GTK_TOOLBAR_BOTH_HORIZ;
	}

	return matecomponent_ui_preferences_get_toolbar_style ();
}

GtkToolbarStyle
matecomponent_ui_sync_toolbar_get_look (MateComponentUIEngine *engine,
				 MateComponentUINode   *node)
{
	const char *txt;
	MateComponentUIToolbarStyle look;

	if ((txt = matecomponent_ui_node_peek_attr (node, "look")))
		look = parse_look (txt);

	else {
		GtkWidget *widget;

		widget = matecomponent_ui_engine_node_get_widget (engine, node);

		if (!widget || !MATECOMPONENT_IS_UI_TOOLBAR (widget) ||
		    gtk_toolbar_get_orientation (GTK_TOOLBAR (widget)) ==
		    GTK_ORIENTATION_HORIZONTAL) {
			txt = matecomponent_ui_node_peek_attr (node, "hlook");
			look = parse_look (txt);
		} else {
			txt = matecomponent_ui_node_peek_attr (node, "vlook");
			look = parse_look (txt);
		}
	}		
	
	return look;
}

static char *
do_config_popup (MateComponentUIEngineConfig *config,
		 MateComponentUINode         *config_node,
		 MateComponentUIEngine       *popup_engine)
{
	char *ret;
	gboolean tip;
	const char *txt;
	MateComponentUIToolbarStyle style;
	
	tip = TRUE;
	if ((txt = matecomponent_ui_node_peek_attr (config_node, "tips")))
		tip = atoi (txt);

	style = matecomponent_ui_sync_toolbar_get_look (matecomponent_ui_engine_config_get_engine (config),
						 config_node);

	ret = g_strdup_printf (
		"<Root>"
		"<commands>"
		"<cmd name=\"LookBoth\" state=\"%d\"/>"
		"<cmd name=\"LookIcon\" state=\"%d\"/>"
		"<cmd name=\"LookText\" state=\"%d\"/>"
		"</commands>"
		"<popups>"
		"<popup>"
		"<submenu label=\"%s\">"
		"<menuitem verb=\"LookBoth\" label=\"%s\" set=\"both\" "
		"type=\"radio\" group=\"look\"/>"
		"<menuitem verb=\"LookIcon\" label=\"%s\" set=\"icon\" "
		"type=\"radio\" group=\"look\"/>"
		"<menuitem verb=\"LookText\" label=\"%s\" set=\"text\" "
		"type=\"radio\" group=\"look\"/>"
		"</submenu>"
		"<separator/>"
		"<menuitem verb=\"Tip\" label=\"%s\" set=\"%d\"/>"
		"<menuitem verb=\"Hide\" label=\"%s\"/>"
		"<menuitem verb=\"Customize\" label=\"%s\" tip=\"%s\""
		" pixtype=\"stock\" pixname=\"Preferences\"/>"
		"</popup>"
		"</popups>"
		"</Root>",
		style == MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_AND_TEXT,
		style == MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_ONLY,
		style == MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT,
		_("Look"), _("B_oth"), _("_Icon"), _("T_ext"),
		tip ? _("Hide t_ips") :  _("Show t_ips"), !tip,
		_("_Hide toolbar"), _("Customi_ze"),
		_("Customize the toolbar"));

	return ret;
}

static void
config_verb_fn (MateComponentUIEngineConfig *config,
		const char           *path,
		const char           *opt_state,
		MateComponentUIEngine       *popup_engine,
		MateComponentUINode         *popup_node)
{
	const char *verb;
	gboolean changed = TRUE;

	if ((verb = matecomponent_ui_node_peek_attr (popup_node, "verb"))) {
		const char *set;

		set = matecomponent_ui_node_peek_attr (popup_node, "set");

		if (!strcmp (verb, "Hide"))
			matecomponent_ui_engine_config_add (
				config, path, "hidden", "1");

		else if (!strcmp (verb, "Show"))
			matecomponent_ui_engine_config_remove (
				config, path, "hidden");

		else if (!strcmp (verb, "Tip"))
			matecomponent_ui_engine_config_add (
				config, path, "tips", set);

		else if (!strncmp (verb, "Look", 4)) {
			if (opt_state && atoi (opt_state))
				matecomponent_ui_engine_config_add (
					config, path, "look", set);
			else
				changed = FALSE;
			
		} else if (!strcmp (verb, "Customize")) {
			matecomponent_ui_engine_config_configure (config);
			changed = FALSE;

		} else
			g_warning ("Unknown verb '%s'", verb);
	}


	if (changed)
		matecomponent_ui_engine_config_serialize (config);
}

static MateComponentDockItem *
create_dockitem (MateComponentUISyncToolbar *sync,
		 MateComponentUINode        *node,
		 const char          *dockname)
{
	MateComponentDockItem *item;
	MateComponentDockItemBehavior beh = 0;
	const char *prop;
	char      **behavior_array;
	gboolean force_detachable = FALSE;
	MateComponentDockPlacement placement = MATECOMPONENT_DOCK_TOP;
	gint band_num = 1;
	gint position = 0;
	guint offset = 0;
	gboolean in_new_band = TRUE;
	gboolean can_config = TRUE;
	GtkWidget *toolbar;

	if ((prop = matecomponent_ui_node_peek_attr (node, "behavior"))) {
		behavior_array = g_strsplit (prop, ",", -1);
	
		if (string_array_contains (behavior_array, "detachable"))
			force_detachable = TRUE;

		if (string_array_contains (behavior_array, "exclusive"))
			beh |= MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE;

		if (string_array_contains (behavior_array, "never vertical"))
			beh |= MATECOMPONENT_DOCK_ITEM_BEH_NEVER_VERTICAL;

		if (string_array_contains (behavior_array, "never floating"))
			beh |= MATECOMPONENT_DOCK_ITEM_BEH_NEVER_FLOATING;

		if (string_array_contains (behavior_array, "never horizontal"))
			beh |= MATECOMPONENT_DOCK_ITEM_BEH_NEVER_HORIZONTAL;

		g_strfreev (behavior_array);
	}

	if (!force_detachable && !matecomponent_ui_preferences_get_toolbar_detachable ())
		beh |= MATECOMPONENT_DOCK_ITEM_BEH_LOCKED;

	item = MATECOMPONENT_DOCK_ITEM (matecomponent_dock_item_new (
		dockname, beh));

	matecomponent_dock_item_set_shadow_type (item, GTK_SHADOW_OUT);

	if ((prop = matecomponent_ui_node_peek_attr (node, "placement"))) {
		if (!strcmp (prop, "top"))
			placement = MATECOMPONENT_DOCK_TOP;
		else if (!strcmp (prop, "right"))
			placement = MATECOMPONENT_DOCK_RIGHT;
		else if (!strcmp (prop, "bottom"))
			placement = MATECOMPONENT_DOCK_BOTTOM;
		else if (!strcmp (prop, "left"))
			placement = MATECOMPONENT_DOCK_LEFT;
		else if (!strcmp (prop, "floating"))
			placement = MATECOMPONENT_DOCK_FLOATING;
	}

	if ((prop = matecomponent_ui_node_peek_attr (node, "band_num")))
		band_num = atoi (prop);

	if ((prop = matecomponent_ui_node_peek_attr (node, "position")))
		position = atoi (prop);

	if ((prop = matecomponent_ui_node_peek_attr (node, "offset")))
		offset = atoi (prop);

	if ((prop = matecomponent_ui_node_peek_attr (node, "in_new_band")))
		in_new_band = atoi (prop);

	matecomponent_dock_add_item (sync->dock, item,
			      placement, band_num,
			      position, offset, in_new_band);

	toolbar = matecomponent_ui_internal_toolbar_new();
	gtk_container_add (GTK_CONTAINER (item), toolbar);
	gtk_widget_show (toolbar);

	if ((prop = matecomponent_ui_node_peek_attr (node, "config")))
		can_config = atoi (prop);
	else
		can_config = TRUE;

	if (can_config) {
		char *path;
		path = matecomponent_ui_xml_make_path (node);

		matecomponent_ui_engine_config_connect (
			GTK_WIDGET (item), sync->parent.engine,
			path, do_config_popup, config_verb_fn);

		matecomponent_ui_engine_config_connect (
			GTK_WIDGET (toolbar), sync->parent.engine,
			path, do_config_popup, config_verb_fn);

		g_free (path);
	}

	return item;
}

static void
impl_matecomponent_ui_sync_toolbar_remove_root (MateComponentUISync *sync,
					 MateComponentUINode *node)
{
	const char *name;

	if ((name = matecomponent_ui_node_peek_attr (node, "name"))) {
		MateComponentDockItem *item;

		item = get_dock_item (MATECOMPONENT_UI_SYNC_TOOLBAR (sync), name);

		if (item)
			gtk_widget_destroy (GTK_WIDGET (item));
	}
}

static void
impl_matecomponent_ui_sync_toolbar_update_root (MateComponentUISync *sync,
					 MateComponentUINode *node)
{
	const char *txt;
	const char *dockname;
	gboolean    tooltips;
	gboolean    detachable;
	MateComponentDockItem *item;
	GtkToolbar  *toolbar;
	MateComponentUIToolbarStyle look;

	dockname = matecomponent_ui_node_peek_attr (node, "name");

	g_return_if_fail (dockname != NULL);

	item = get_dock_item (MATECOMPONENT_UI_SYNC_TOOLBAR (sync), dockname);
	
	if (!item)
		item = create_dockitem (MATECOMPONENT_UI_SYNC_TOOLBAR (sync),
					node, dockname);

	if ((txt = matecomponent_ui_node_peek_attr (node, "behavior")) &&
	    strstr (txt, "detachable"))
		detachable = TRUE;
	else
		detachable = matecomponent_ui_preferences_get_toolbar_detachable ();
	matecomponent_dock_item_set_locked (item, !detachable);

	toolbar = GTK_TOOLBAR (matecomponent_dock_item_get_child (item));

	matecomponent_ui_engine_stamp_root (sync->engine, node, GTK_WIDGET (toolbar));

	look = matecomponent_ui_sync_toolbar_get_look (sync->engine, node);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), look);

	if ((txt = matecomponent_ui_node_peek_attr (node, "tips")))
		tooltips = atoi (txt);
	else
		tooltips = TRUE;

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (toolbar), tooltips);

       /*
	* FIXME: It shouldn't be necessary to explicitly resize the
	* dock, since resizing a widget is supposed to resize it's parent,
	* but the dock is not resized correctly on dockitem show / hides.
	*/
	if (matecomponent_ui_sync_do_show_hide (sync, node, NULL, GTK_WIDGET (item)))
		gtk_widget_queue_resize (GTK_WIDGET (
			MATECOMPONENT_UI_SYNC_TOOLBAR (sync)->dock));
}

static gboolean
impl_matecomponent_ui_sync_toolbar_can_handle (MateComponentUISync *sync,
					MateComponentUINode *node)
{
	if (!dockitem_id) {
		dockitem_id = g_quark_from_static_string ("dockitem");
		toolitem_id = g_quark_from_static_string ("toolitem");
	}

	return (node->name_id == dockitem_id ||
		node->name_id == toolitem_id);
}

static GtkWidget *
impl_matecomponent_ui_sync_toolbar_wrap_widget (MateComponentUISync *sync,
					 GtkWidget    *custom_widget)
{
	if (!GTK_IS_TOOL_ITEM (custom_widget))
		return matecomponent_ui_toolbar_control_item_new_widget (custom_widget);
	else
		return custom_widget;
}

static void
matecomponent_ui_sync_toolbar_class_init (MateComponentUISyncClass *sync_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (sync_class);

	object_class = G_OBJECT_CLASS (sync_class);
	object_class->dispose = impl_dispose;

	sync_class->sync_state = impl_matecomponent_ui_sync_toolbar_state;
	sync_class->build      = impl_matecomponent_ui_sync_toolbar_build;
	sync_class->build_placeholder = impl_matecomponent_ui_sync_toolbar_build_placeholder;

	sync_class->get_widgets   = impl_matecomponent_ui_sync_toolbar_get_widgets;
	sync_class->ignore_widget = impl_matecomponent_ui_sync_toolbar_ignore_widget;
	sync_class->remove_root   = impl_matecomponent_ui_sync_toolbar_remove_root;
	sync_class->update_root   = impl_matecomponent_ui_sync_toolbar_update_root;

	sync_class->state_update  = impl_matecomponent_ui_sync_toolbar_state_update;
	sync_class->can_handle    = impl_matecomponent_ui_sync_toolbar_can_handle;

	sync_class->wrap_widget   = impl_matecomponent_ui_sync_toolbar_wrap_widget;
}

static void
matecomponent_ui_sync_toolbar_init (MateComponentUISyncToolbar *sync)
{
}

GType
matecomponent_ui_sync_toolbar_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUISyncToolbarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) matecomponent_ui_sync_toolbar_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUISyncToolbar),
			0, /* n_preallocs */
			(GInstanceInitFunc) matecomponent_ui_sync_toolbar_init
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUISyncToolbar",
					       &info, 0);
	}

	return type;
}

MateComponentUISync *
matecomponent_ui_sync_toolbar_new (MateComponentUIEngine  *engine,
			    MateComponentDock       *dock)
{
	MateComponentUISyncToolbar *sync;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	sync = g_object_new (MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR, NULL);

	sync->dock = g_object_ref (dock);

	return matecomponent_ui_sync_construct (
		MATECOMPONENT_UI_SYNC (sync), engine, FALSE, TRUE);
}
