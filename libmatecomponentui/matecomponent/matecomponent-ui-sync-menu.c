/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-sync-menu.h: The MateComponent UI/XML sync engine for menus
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
#include <matecomponent/matecomponent-dock-item.h>
#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-sync.h>
#include <matecomponent/matecomponent-ui-sync-menu.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-preferences.h>

#undef WIDGET_SYNC_DEBUG

static GObjectClass *parent_class = NULL;

#define PARENT_TYPE matecomponent_ui_sync_get_type ()

/* Doesn't the GtkRadioMenuItem API suck badly ! */
#define MAGIC_RADIO_GROUP_KEY "MateComponent::RadioGroupName"
#define UI_SYNC_MENU_KEY "MateComponent::UISyncMenu"

static GQuark menu_id = 0;
static GQuark popups_id = 0;
static GQuark submenu_id = 0;
static GQuark menuitem_id = 0;

typedef struct {
	GtkMenu          *menu;
	char             *path;
} Popup;

static void
popup_remove (MateComponentUISyncMenu *smenu,
	      Popup            *popup)
{
	MateComponentUINode *node;

	g_return_if_fail (smenu != NULL);
	g_return_if_fail (popup != NULL);

	g_signal_handlers_disconnect_matched (
		popup->menu, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, popup);

	node = matecomponent_ui_engine_get_path (
		smenu->parent.engine, popup->path);

	if (node)
		matecomponent_ui_engine_prune_widget_info (
			smenu->parent.engine, node, TRUE);

	smenu->popups = g_slist_remove (
		smenu->popups, popup);
	
	g_free (popup->path);
	g_free (popup);
}

void
matecomponent_ui_sync_menu_remove_popup (MateComponentUISyncMenu *sync,
				  const char       *path)
{
	GSList *l, *next;

	g_return_if_fail (path != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC_MENU (sync));

	for (l = sync->popups; l; l = next) {
		Popup *popup = l->data;

		next = l->next;
		if (!strcmp (popup->path, path))
			popup_remove (sync, popup);
	}
}

static void
popup_destroy (GtkObject *menu, Popup *popup)
{
	MateComponentUISyncMenu *smenu = g_object_get_data (
		G_OBJECT (menu), UI_SYNC_MENU_KEY);

	g_return_if_fail (smenu != NULL);
	popup_remove (smenu, popup);
}

static gboolean
node_is_popup (MateComponentUINode *node)
{
	if (!node)
		return FALSE;

	if (matecomponent_ui_node_has_name (node, "popup"))
		return TRUE;
	
	else if (matecomponent_ui_node_has_name (node, "menu"))
		return FALSE;

	else
		return node_is_popup (matecomponent_ui_node_parent (node));
}

static void
add_tearoff (MateComponentUINode *node, GtkMenu *menu, gboolean popup_init)
{
	GtkWidget *tearoff;
	gboolean   has_tearoff;

	has_tearoff = matecomponent_ui_preferences_get_menus_have_tearoff ();

	if (node) {
		const char *txt;

		txt = matecomponent_ui_node_peek_attr (node, "tearoff");

		if (txt)
			has_tearoff = atoi (txt);

		else if (node_is_popup (node))
			has_tearoff = FALSE;

	} else if (popup_init)
		has_tearoff = FALSE;

	/*
	 * Create the tearoff item at the beginning of the menu shell,
	 * if appropriate.
	 */
	if (has_tearoff) {
		tearoff = gtk_tearoff_menu_item_new ();
		gtk_widget_show (tearoff);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), tearoff);
	}
}

void
matecomponent_ui_sync_menu_add_popup (MateComponentUISyncMenu *smenu,
			       GtkMenu          *menu,
			       const char       *path)
{
	Popup        *popup;
	MateComponentUINode *node;
	GList        *children;

	g_return_if_fail (path != NULL);
	g_return_if_fail (GTK_IS_MENU (menu));
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC_MENU (smenu));

	matecomponent_ui_sync_menu_remove_popup (smenu, path);

	popup       = g_new (Popup, 1);
	popup->menu = menu;
	popup->path = g_strdup (path);

	if ((children = gtk_container_get_children (GTK_CONTAINER (menu)))) {
		g_warning ("Extraneous items in blank popup");
		g_list_free (children);
	}

	add_tearoff (matecomponent_ui_engine_get_path (
		smenu->parent.engine, path), menu, TRUE);

	smenu->popups = g_slist_prepend (smenu->popups, popup);

	g_object_set_data (G_OBJECT (menu), UI_SYNC_MENU_KEY, smenu);

	g_signal_connect (G_OBJECT (menu), "destroy",
			  G_CALLBACK (popup_destroy), popup);

	node = matecomponent_ui_engine_get_path (smenu->parent.engine, path);

	matecomponent_ui_engine_dirty_tree (smenu->parent.engine, node);
}

static void
radio_group_remove (GtkRadioMenuItem *menuitem,
		    char             *group_name)
{
	GtkRadioMenuItem *master, *insert;
	MateComponentUISyncMenu *menu_sync;
	GSList           *l;

	menu_sync = g_object_get_data (
		G_OBJECT (menuitem), MAGIC_RADIO_GROUP_KEY);

	master = g_hash_table_lookup (menu_sync->radio_groups, group_name);
	g_return_if_fail (master != NULL);

	l = master->group;
	while (l && l->data == menuitem)
		l = l->next;

	if (l)
		insert = g_object_ref (l->data);
	else
		insert = NULL;
	
	g_hash_table_remove (menu_sync->radio_groups, group_name);

	if (insert)/* Entries left in group */
		g_hash_table_insert (
			menu_sync->radio_groups,
			g_strdup (group_name), insert);

	g_object_unref (menu_sync);
}

static void
radio_group_add (MateComponentUISyncMenu *menu_sync,
		 GtkRadioMenuItem *menuitem,
		 const char       *group_name)
{
	GtkRadioMenuItem *master;

	g_return_if_fail (menuitem != NULL);
	g_return_if_fail (menu_sync != NULL);
	g_return_if_fail (group_name != NULL);

	if (!(master = g_hash_table_lookup (menu_sync->radio_groups, group_name)))
		g_hash_table_insert (menu_sync->radio_groups,
				     g_strdup (group_name),
				     g_object_ref (menuitem));

	else {
		gtk_radio_menu_item_set_group (
			menuitem, gtk_radio_menu_item_get_group (master));
		/* 
		 * Since we created this item without a group, it's
		 * active, but now we are adding it to a group so it
		 * should not be active.
		 */
		GTK_CHECK_MENU_ITEM (menuitem)->active = FALSE;
	}

	g_object_set_data (G_OBJECT (menuitem),
			   MAGIC_RADIO_GROUP_KEY,
			   g_object_ref (menu_sync));

	g_signal_connect_data (G_OBJECT (menuitem),
			       "destroy", 
			       G_CALLBACK (radio_group_remove),
			       g_strdup (group_name),
			       (GClosureNotify) g_free, 0);
}

static GtkWidget *
get_item_widget (GtkWidget *widget)
{
	if (!widget)
		return NULL;

	if (GTK_IS_MENU (widget))
		return gtk_menu_get_attach_widget (
			GTK_MENU (widget));

	return NULL;
}

static gboolean
label_same (GtkBin *menu_widget, const char *txt)
{
	GtkWidget *label;

	return menu_widget &&
		(label = menu_widget->child) &&
		GTK_IS_LABEL (label) &&
		((GtkLabel *)label)->label &&
		!strcmp (((GtkLabel *)label)->label, txt);
}

static gboolean
widget_has_accel (GtkWidget       *widget,
		  GtkAccelGroup   *accel_group,
		  guint           key, 
		  GdkModifierType mods)
{
	int i;
	guint n_entries;
	GList *l_tmp, *l_closures = gtk_widget_list_accel_closures (widget);
	GtkAccelGroupEntry *entry = gtk_accel_group_query (accel_group,
							   key, mods,
							   &n_entries);

	if (n_entries)
		for (l_tmp = l_closures; l_tmp; l_tmp = l_tmp->next)
	  		for (i=0; i<n_entries; i++) 
			  	if (entry[i].closure == l_tmp->data) {
					g_list_free (l_closures);
					return TRUE;
				}
	
	g_list_free (l_closures);

	return FALSE;
}

static void
impl_matecomponent_ui_sync_menu_state (MateComponentUISync *sync,
				MateComponentUINode *node,
				MateComponentUINode *cmd_node,
				GtkWidget    *widget,
				GtkWidget    *parent)
{
	GtkWidget        *menu_widget;
	MateComponentUISyncMenu *sync_menu = MATECOMPONENT_UI_SYNC_MENU (sync);
	MateComponentUIEngine   *engine = sync->engine;
	char             *type, *txt, *label_attr;

	g_return_if_fail (parent != NULL);

	if (matecomponent_ui_node_has_name (node, "placeholder") ||
	    matecomponent_ui_node_has_name (node, "separator")) {

		matecomponent_ui_engine_queue_update (
			engine, widget, node, cmd_node);
		return;
	}

	if (matecomponent_ui_node_has_name (node, "submenu")) {
		menu_widget = get_item_widget (widget);
		if (!menu_widget)
			menu_widget = widget;

		/* Recurse here just once, don't duplicate in the build. */
		matecomponent_ui_engine_update_node (engine, sync, node);

	} else if (matecomponent_ui_node_has_name (node, "menuitem"))
		menu_widget = widget;
	else
		return;

	if ((type = matecomponent_ui_engine_get_attr (node, cmd_node, "type")))
		matecomponent_ui_node_free_string (type);
	else {
		GtkImageMenuItem *image_menu_item;
		
		image_menu_item = (GtkImageMenuItem *) menu_widget;
		
		if (!matecomponent_ui_preferences_get_menus_have_icons ())
			gtk_image_menu_item_set_image (image_menu_item, NULL);

		else if (matecomponent_ui_node_peek_attr (node, "pixtype") ||
			 matecomponent_ui_node_peek_attr (cmd_node, "pixtype")) {
			GtkWidget *image;

			image = gtk_image_menu_item_get_image (image_menu_item);
		
			if (!image) {
				image = gtk_image_new ();
				g_object_set (G_OBJECT (image_menu_item),
					      "image", image, NULL);
			}

			matecomponent_ui_util_xml_set_image (
				GTK_IMAGE (image), node, cmd_node,
				GTK_ICON_SIZE_MENU);

			gtk_widget_show (image);
		}
	}
	
	if ((label_attr = matecomponent_ui_engine_get_attr (node, cmd_node, "label"))) {
		GtkWidget *label;
		if (!label_same (GTK_BIN (menu_widget), label_attr)) {
			if (!GTK_BIN (menu_widget)->child) {
				label = gtk_accel_label_new (label_attr);

				g_object_freeze_notify (G_OBJECT (label));

				/* this widget has a mnemonic */
				gtk_label_set_use_underline (GTK_LABEL (label), TRUE);

				/* Setup the widget. */
				gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
				gtk_widget_show (label);
				gtk_container_add (GTK_CONTAINER (menu_widget), label);

			} else {
				label = GTK_BIN (menu_widget)->child;

				g_object_freeze_notify (G_OBJECT (label));

				gtk_label_set_text_with_mnemonic (GTK_LABEL (label), label_attr);
			}

			gtk_accel_label_set_accel_widget (
				GTK_ACCEL_LABEL (label), menu_widget);

			g_object_thaw_notify (G_OBJECT (label));
		} /* else
			g_warning ("No change in label '%s'", label_attr); */

		matecomponent_ui_node_free_string (label_attr);
	}
	
	if ((txt = matecomponent_ui_engine_get_attr (node, cmd_node, "accel"))) {
		guint           key;
		GdkModifierType mods;

/*		fprintf (stderr, "Accel name is afterwards '%s'\n", text); */
		matecomponent_ui_util_accel_parse (txt, &key, &mods);
		matecomponent_ui_node_free_string (txt);

		if (!key) /* FIXME: this looks strange */
			return;

	  	if (! widget_has_accel (menu_widget, sync_menu->accel_group, key, mods))
			gtk_widget_add_accelerator (menu_widget,
						    "activate",
						    sync_menu->accel_group,
						    key, mods,
						    GTK_ACCEL_VISIBLE);
	}

	matecomponent_ui_engine_queue_update (
		engine, menu_widget, node, cmd_node);
}

static void
put_hint_in_statusbar (GtkWidget      *menuitem,
		       MateComponentUIEngine *engine)
{
	MateComponentUINode *node;
	MateComponentUINode *cmd_node;
	char *hint;

	g_return_if_fail (engine != NULL);

	node = matecomponent_ui_engine_widget_get_node (menuitem);
	g_return_if_fail (node != NULL);

	cmd_node = matecomponent_ui_engine_get_cmd_node (engine, node);

	hint = matecomponent_ui_engine_get_attr (node, cmd_node, "tip");

/*	g_warning ("Getting tooltip on '%s', '%s' : '%s'",
		   matecomponent_ui_xml_make_path (node),
		   cmd_node ? matecomponent_ui_xml_make_path (cmd_node) : "no cmd",
		   hint);*/
	if (!hint)
		return;

	matecomponent_ui_engine_add_hint (engine, hint);

	matecomponent_ui_node_free_string (hint);
}

static void
remove_hint_from_statusbar (GtkWidget      *menuitem,
			    MateComponentUIEngine *engine)
{
	matecomponent_ui_engine_remove_hint (engine);
}

static gint
exec_verb_cb (GtkWidget *item, MateComponentUIEngine *engine)
{
	matecomponent_ui_engine_emit_verb_on_w (engine, GTK_WIDGET (item));

	return FALSE;
}

static gint
menu_toggle_emit_ui_event (GtkCheckMenuItem *item,
			   MateComponentUIEngine   *engine)
{
	const char *state;

	if (item->active)
		state = "1";
	else
		state = "0";

	matecomponent_ui_engine_emit_event_on_w (
		engine, GTK_WIDGET (item), state);

	return FALSE;
}

static gint
sucking_gtk_keybindings_cb (GtkWidget   *widget,
			    GdkEventKey *event,
			    gpointer     dummy)
{
	static GtkWidgetClass *klass = NULL;
	static guint           id = 0;
	gboolean               ret;

	if (!klass)
		klass = gtk_type_class (GTK_TYPE_MENU_SHELL);
	if (!id)
		id = g_signal_lookup ("key_press_event", GTK_TYPE_WIDGET);

	if (klass->key_press_event (widget, event))
		ret = TRUE;
	else
		ret = FALSE;

	g_signal_stop_emission (widget, id, 0);
	
	return ret;
}

static GtkWidget *
impl_matecomponent_ui_sync_menu_build (MateComponentUISync     *sync,
				MateComponentUINode     *node,
				MateComponentUINode     *cmd_node,
				int              *pos,
				GtkWidget        *parent)
{
	MateComponentUIEngine *engine = sync->engine;
	MateComponentUISyncMenu *menu_sync = MATECOMPONENT_UI_SYNC_MENU (sync);
	GtkWidget      *menu_widget = NULL;
	GtkWidget      *ret_widget;
	char           *type, *stock_id;

	if (!parent) /* A popup without a GtkMenu inserted as yet. */
		return NULL;

	if (matecomponent_ui_node_has_name (node, "separator")) {

		menu_widget = gtk_separator_menu_item_new ();

	} else if (matecomponent_ui_node_has_name (node, "control")) {

		GtkWidget *control = matecomponent_ui_engine_build_control (
			engine, node);

		if (!control)
			return NULL;

		else if (!GTK_IS_MENU_ITEM (control)) {
			menu_widget = gtk_menu_item_new ();
			gtk_container_add (GTK_CONTAINER (menu_widget),
					   control);
		} else
			menu_widget = control;

	} else if (matecomponent_ui_node_has_name (node, "menuitem") ||
		   matecomponent_ui_node_has_name (node, "submenu")) {

		if ((stock_id = matecomponent_ui_engine_get_attr (node, cmd_node, "stockid"))) {
			GtkStockItem  stock_item;
			GtkIconSet   *icon_set;

			if (!gtk_stock_lookup (stock_id, &stock_item))
				g_warning ("Unknown stock id '%s' on %s", stock_id,
					   matecomponent_ui_xml_make_path (node));
			else {
				gchar *label, *accel;

				if (!matecomponent_ui_node_has_attr (node, "label") &&
				    !matecomponent_ui_node_has_attr (cmd_node, "label")) {
					label = dgettext (stock_item.translation_domain, stock_item.label);
					matecomponent_ui_node_set_attr (node, "label", label);
				}

				if (!matecomponent_ui_node_has_attr (node, "accel") &&
				    !matecomponent_ui_node_has_attr (cmd_node, "accel")) {
					accel = matecomponent_ui_util_accel_name (stock_item.keyval, stock_item.modifier);
					matecomponent_ui_node_set_attr (node, "accel", accel);
					g_free (accel);
				}
			}

			icon_set = gtk_icon_factory_lookup_default (stock_id);

			if (icon_set) {
				if (!matecomponent_ui_node_has_attr (node, "pixtype") &&
				    !matecomponent_ui_node_has_attr (cmd_node, "pixtype"))
					matecomponent_ui_node_set_attr (node, "pixtype", "stock");
				if (!matecomponent_ui_node_has_attr (node, "pixname") &&
				    !matecomponent_ui_node_has_attr (cmd_node, "pixname"))
					matecomponent_ui_node_set_attr (node, "pixname", stock_id);
			}

			g_free (stock_id);
		}

		/* Create menu item */
		if ((type = matecomponent_ui_engine_get_attr (node, cmd_node, "type"))) {
			if (!strcmp (type, "radio")) {
				char *group = matecomponent_ui_engine_get_attr (node, cmd_node, "group");

				menu_widget = gtk_radio_menu_item_new (NULL);

				if (group)
					radio_group_add (
						MATECOMPONENT_UI_SYNC_MENU (sync),
						GTK_RADIO_MENU_ITEM (menu_widget), group);

				matecomponent_ui_node_free_string (group);
			} else if (!strcmp (type, "toggle"))
				menu_widget = gtk_check_menu_item_new ();
			
			else
				menu_widget = NULL;

			g_signal_connect (menu_widget, "toggled",
					  G_CALLBACK (menu_toggle_emit_ui_event),
					  engine);

			matecomponent_ui_node_free_string (type);
		} else {
			/* We always use an image menu item, it doesn't hurt. */
			menu_widget = gtk_image_menu_item_new ();
		}

		if (!menu_widget)
			return NULL;
			
		g_signal_connect (G_OBJECT (menu_widget),
				    "select",
				    G_CALLBACK (put_hint_in_statusbar),
				    engine);

		g_signal_connect (G_OBJECT (menu_widget),
				    "deselect",
				    G_CALLBACK (remove_hint_from_statusbar),
				    engine);
	}

	if (!menu_widget)
		return NULL;	

	if (matecomponent_ui_node_has_name (node, "submenu")) {
		GtkMenuShell *shell;
		GtkMenu      *menu;
		
		shell = GTK_MENU_SHELL (parent);
		g_signal_connect (G_OBJECT (shell), "key_press_event",
				  G_CALLBACK (sucking_gtk_keybindings_cb), NULL);


		/* Create the menu shell. */
		menu = GTK_MENU (gtk_menu_new ());
		g_signal_connect (G_OBJECT (menu), "key_press_event",
				  G_CALLBACK (sucking_gtk_keybindings_cb), NULL);

		gtk_menu_set_accel_group (menu, menu_sync->accel_group);

		add_tearoff (node, GTK_MENU (menu), FALSE);

		/*
		 * Associate this menu shell with the menu item for
		 * this submenu.
		 */
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget),
					   GTK_WIDGET (menu));

		/* We don't recurse here, it is done once in the state set */

		gtk_widget_show (GTK_WIDGET (menu));
		gtk_widget_show (GTK_WIDGET (shell));

		ret_widget = GTK_WIDGET (menu);
	} else
		ret_widget = menu_widget;

	if (!GTK_IS_CHECK_MENU_ITEM (menu_widget))
		g_signal_connect (G_OBJECT (menu_widget), "activate",
				  G_CALLBACK (exec_verb_cb), engine);

	g_signal_connect (G_OBJECT (menu_widget), "key_press_event",
			  G_CALLBACK (sucking_gtk_keybindings_cb), NULL);

	gtk_widget_show (menu_widget);
			    
	gtk_menu_shell_insert (GTK_MENU_SHELL (parent),
			       menu_widget, (*pos)++);

	return ret_widget;
}

static GtkWidget *
impl_matecomponent_ui_sync_menu_build_placeholder (MateComponentUISync     *sync,
					    MateComponentUINode     *node,
					    MateComponentUINode     *cmd_node,
					    int              *pos,
					    GtkWidget        *parent)
{
	GtkWidget *widget;

	if (!parent) /* A popup without a GtkMenu inserted as yet. */
		return NULL;

	widget = gtk_separator_menu_item_new ();

	gtk_menu_shell_insert (GTK_MENU_SHELL (parent),
			       GTK_WIDGET (widget), (*pos)++);

	return widget;
}

static GList *
impl_matecomponent_ui_sync_menu_get_widgets (MateComponentUISync *sync,
				      MateComponentUINode *node)
{
	GtkWidget *widget;

	widget = matecomponent_ui_engine_node_get_widget (sync->engine, node);

	if (widget)
		return gtk_container_get_children (GTK_CONTAINER (widget));
	else
		return NULL; /* A popup child with no GtkMenu yet */
}

static void
impl_matecomponent_ui_sync_menu_state_update (MateComponentUISync *sync,
				       GtkWidget    *widget,
				       const char   *new_state)
{
	if (GTK_IS_CHECK_MENU_ITEM (widget)) {
#ifdef STATE_SYNC_DEBUG
		g_warning ("Setting check menu item '%p' to '%s'",
			   widget, state);
#endif
		gtk_check_menu_item_set_active (
			GTK_CHECK_MENU_ITEM (widget), 
			atoi (new_state));
	} else
		g_warning ("TESTME: strange, setting "
			   "state '%s' on weird object '%s'",
			   new_state,
			   g_type_name_from_instance (
				   (GTypeInstance *) widget));
}

static void
impl_dispose (GObject *object)
{
	MateComponentUISyncMenu *sync = (MateComponentUISyncMenu *) object;

	if (sync->menu) {
		gtk_widget_destroy (GTK_WIDGET (sync->menu));
		g_object_unref (sync->menu);
		sync->menu = NULL;
	}

	if (sync->menu_dock_item) {
		g_object_unref (sync->menu_dock_item);
		sync->menu_dock_item = NULL;
	}

	if (sync->accel_group) {
		g_object_unref (sync->accel_group);
		sync->accel_group = NULL;
	}

	if (sync->radio_groups) {
		GHashTable *dest = sync->radio_groups;

		sync->radio_groups = NULL;
		g_hash_table_destroy (dest);
	}

	parent_class->dispose (object);
}

static gboolean
impl_matecomponent_ui_sync_menu_ignore_widget (MateComponentUISync *sync,
					GtkWidget    *widget)
{
	return GTK_IS_TEAROFF_MENU_ITEM (widget);
}

static void
impl_matecomponent_ui_sync_menu_update_root (MateComponentUISync *sync,
				      MateComponentUINode *root)
{
	MateComponentUISyncMenu *smenu = MATECOMPONENT_UI_SYNC_MENU (sync);
	const char *txt;
	gboolean    detachable;

	if (matecomponent_ui_node_has_name (root, "menu") &&
	    smenu->menu_dock_item)
		matecomponent_ui_sync_do_show_hide (
			sync, root, NULL,
			smenu->menu_dock_item);

	if ((txt = matecomponent_ui_node_peek_attr (root, "behavior")) &&
	    strstr (txt, "detachable"))
		detachable = TRUE;
	else
	    detachable = matecomponent_ui_preferences_get_menubar_detachable ();
	matecomponent_dock_item_set_locked (MATECOMPONENT_DOCK_ITEM (smenu->menu_dock_item), !detachable);
}

static void
impl_matecomponent_ui_sync_menu_stamp_root (MateComponentUISync *sync)
{
	MateComponentUISyncMenu *smenu = MATECOMPONENT_UI_SYNC_MENU (sync);
	MateComponentUINode     *node;
	GSList           *l;

#ifdef WIDGET_SYNC_DEBUG
	fprintf (stderr, "Stamping menu sync's roots\n");
#endif

	node = matecomponent_ui_engine_get_path (sync->engine, "/menu");
	if (smenu->menu) {
		GtkWidget *widget = GTK_WIDGET (smenu->menu);

		matecomponent_ui_engine_stamp_root (sync->engine, node, widget);
		matecomponent_ui_sync_do_show_hide (sync, node, NULL, widget);
	}

	for (l = smenu->popups; l; l = l->next) {
		Popup *popup = l->data;

		if ((node = matecomponent_ui_engine_get_path (sync->engine,
						       popup->path))) {
#ifdef WIDGET_SYNC_DEBUG
			fprintf (stderr, "Stamping popup root '%s(%p)' with '%p'\n",
				 popup->path, node, popup->menu);
#endif
			matecomponent_ui_engine_stamp_root (sync->engine, node,
						     GTK_WIDGET (popup->menu));
		} else
			g_warning ("Can't find path '%s' for popup widget",
				   popup->path);
	}

	if ((node = matecomponent_ui_engine_get_path (sync->engine, "/popups")))
		matecomponent_ui_engine_node_set_dirty (sync->engine, node, FALSE);
}

static gboolean
impl_matecomponent_ui_sync_menu_can_handle (MateComponentUISync *sync,
				     MateComponentUINode *node)
{
	if (!menu_id) {
		menu_id = g_quark_from_static_string ("menu");
		popups_id = g_quark_from_static_string ("popups");
		submenu_id = g_quark_from_static_string ("submenu");
		menuitem_id = g_quark_from_static_string ("menuitem");
	}
	
	return (node->name_id == menu_id ||
		node->name_id == popups_id ||
		node->name_id == submenu_id ||
		node->name_id == menuitem_id);
}

/* We need to map the shell to the item */

static GtkWidget *
impl_matecomponent_ui_sync_menu_get_attached (MateComponentUISync *sync,
				       GtkWidget    *widget,
				       MateComponentUINode *node)
{
	return get_item_widget (widget);
}

static GtkWidget *
impl_matecomponent_ui_sync_menu_wrap_widget (MateComponentUISync *sync,
				      GtkWidget    *custom_widget)
{
	if (!GTK_IS_MENU_ITEM (custom_widget)) {
		GtkWidget *widget;

		widget = gtk_menu_item_new ();
		gtk_container_add (GTK_CONTAINER (widget),
				   custom_widget);
		return widget;
	} else 
		return custom_widget;
}

static void
class_init (MateComponentUISyncClass *sync_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (sync_class);

	object_class = G_OBJECT_CLASS (sync_class);
	object_class->dispose  = impl_dispose;

	sync_class->sync_state = impl_matecomponent_ui_sync_menu_state;
	sync_class->build      = impl_matecomponent_ui_sync_menu_build;
	sync_class->build_placeholder = impl_matecomponent_ui_sync_menu_build_placeholder;

	sync_class->get_widgets   = impl_matecomponent_ui_sync_menu_get_widgets;
	sync_class->ignore_widget = impl_matecomponent_ui_sync_menu_ignore_widget;
	sync_class->update_root   = impl_matecomponent_ui_sync_menu_update_root;

	sync_class->state_update  = impl_matecomponent_ui_sync_menu_state_update;
	sync_class->stamp_root    = impl_matecomponent_ui_sync_menu_stamp_root;
	sync_class->can_handle    = impl_matecomponent_ui_sync_menu_can_handle;

	sync_class->get_attached  = impl_matecomponent_ui_sync_menu_get_attached;
	sync_class->wrap_widget   = impl_matecomponent_ui_sync_menu_wrap_widget;
}

static void
init (MateComponentUISyncMenu *sync)
{
	sync->radio_groups = g_hash_table_new_full (
		g_str_hash, g_str_equal,
		(GDestroyNotify) g_free,
		(GDestroyNotify) g_object_unref);
}

GType
matecomponent_ui_sync_menu_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUISyncMenuClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUISyncMenu),
			0, /* n_preallocs */
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUISyncMenu",
					       &info, 0);
	}

	return type;
}

MateComponentUISync *
matecomponent_ui_sync_menu_new (MateComponentUIEngine *engine,
			 GtkMenuBar     *menu,
			 GtkWidget      *menu_dock_item,
			 GtkAccelGroup  *group)
{
	MateComponentUISyncMenu *sync;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	sync = g_object_new (MATECOMPONENT_TYPE_UI_SYNC_MENU, NULL);

	sync->menu = menu ? g_object_ref (menu) : NULL;

	sync->menu_dock_item = menu_dock_item ?
		g_object_ref (menu_dock_item) :
		menu_dock_item;

	sync->accel_group = group ? g_object_ref (group) : gtk_accel_group_new ();

	return matecomponent_ui_sync_construct (
		MATECOMPONENT_UI_SYNC (sync), engine, TRUE, TRUE);
}
