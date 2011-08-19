/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-sync-status.h: The MateComponent UI/XML sync engine for status bits.
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

#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-sync.h>
#include <matecomponent/matecomponent-ui-sync-status.h>
#include <matecomponent/matecomponent-ui-preferences.h>
#include <matecomponent/matecomponent-ui-private.h>

static GObjectClass *parent_class = NULL;

#define PARENT_TYPE matecomponent_ui_sync_get_type ()

#define HINT_KEY "MateComponentWindow:hint"

static void
set_hint_cb (MateComponentUIEngine   *engine,
	     const char       *str,
	     MateComponentUISyncStatus *msync)
{
	guint id;

	if (msync->main_status) {
		id = gtk_statusbar_get_context_id (
			msync->main_status, HINT_KEY);

		gtk_statusbar_push (msync->main_status, id, str);
	}
}

static void
remove_hint_cb (MateComponentUIEngine   *engine,
		MateComponentUISyncStatus *msync)
{
	if (msync->main_status) {
		guint id;

		id = gtk_statusbar_get_context_id (
			msync->main_status, HINT_KEY);

		gtk_statusbar_pop (msync->main_status, id);
	}
}

/* Only works for non-end packed @widget */
static gboolean
has_item_to_the_right (GtkBox *box, GtkWidget *widget)
{
	GList *l;
	gboolean passed_us = FALSE;
	gboolean has_to_right = FALSE;

	g_return_val_if_fail (GTK_IS_BOX (box), FALSE);

	for (l = box->children; l; l=l->next) {
		GtkBoxChild *child = l->data;

		if (child->widget == widget) {
			passed_us = TRUE;
			
		} else if (GTK_WIDGET_VISIBLE (child->widget)) {
			if (child->pack == GTK_PACK_END || passed_us) {
				has_to_right = TRUE;
				break;
			}
		}
	}
	return has_to_right;
}

static void
impl_matecomponent_ui_sync_status_state (MateComponentUISync     *sync,
				  MateComponentUINode     *node,
				  MateComponentUINode     *cmd_node,
				  GtkWidget        *widget,
				  GtkWidget        *parent)
{
	const char         *name;
	MateComponentUISyncStatus *msync = MATECOMPONENT_UI_SYNC_STATUS (sync);
		
	name = matecomponent_ui_node_peek_attr (node, "name");
	if (!name)
		return;

	if (!strcmp (name, "main")) {
		MateComponentUINode *next;
		const char   *id_str;
		const char   *resize_grip;
		gboolean      has_grip;

		resize_grip = matecomponent_ui_node_peek_attr (
			matecomponent_ui_node_parent (node), "resize_grip");

		has_grip = TRUE;
		if (resize_grip && atoi (resize_grip) == 0)
        		has_grip = FALSE;

		next = node;
		while ((next = matecomponent_ui_node_next (next))) {
			const char *hidden;

			/* The grip is useless if we have items to the right */
			if (!(hidden = matecomponent_ui_node_peek_attr (next, "hidden")) ||
			    !atoi (hidden))
				has_grip = FALSE;
		}
		if (has_item_to_the_right (GTK_BOX (parent), widget))
			has_grip = FALSE;

		gtk_statusbar_set_has_resize_grip (msync->main_status, has_grip);
		
		id_str = matecomponent_ui_engine_node_get_id (sync->engine, node);

		msync->main_status = GTK_STATUSBAR (widget);
			
		if (id_str) {
			guint id;
			char *txt;

			id = gtk_statusbar_get_context_id (
				msync->main_status, id_str);

			if ((txt = matecomponent_ui_node_get_content (node)))
				gtk_statusbar_push (msync->main_status, id, txt);

			else
				gtk_statusbar_pop (msync->main_status, id);

			matecomponent_ui_node_free_string (txt);
		}
	}
}

static void
main_status_null (MateComponentUISyncStatus *msync)
{
	msync->main_status = NULL;
}

/*
 * This function is to ensure that the status bar
 * does not ask for any space, but fills the free
 * horizontal space in the hbox.
 */
static void
clobber_request_cb (GtkWidget      *widget,
		    GtkRequisition *requisition,
		    gpointer        user_data)
{
	requisition->width = 1;
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

static GtkWidget *
impl_matecomponent_ui_sync_status_build (MateComponentUISync     *sync,
				  MateComponentUINode     *node,
				  MateComponentUINode     *cmd_node,
				  int              *pos,
				  GtkWidget        *parent)
{
	const char *name;
	GtkWidget *widget = NULL;
	MateComponentUISyncStatus *msync = MATECOMPONENT_UI_SYNC_STATUS (sync);
		
	name = matecomponent_ui_node_peek_attr (node, "name");
	if (!name)
		return NULL;

	if (!strcmp (name, "main")) {
		widget = gtk_statusbar_new ();
		
		g_signal_connect (GTK_OBJECT (widget),
				    "size_request",
				    G_CALLBACK (clobber_request_cb), NULL);

		msync->main_status = GTK_STATUSBAR (widget);

		g_signal_connect_object (widget, "destroy",
					 G_CALLBACK (main_status_null),
					 msync, G_CONNECT_SWAPPED);

		/* insert a little padding so text isn't jammed against frame */
		gtk_misc_set_padding (
			GTK_MISC (GTK_STATUSBAR (widget)->label),
			MATECOMPONENT_UI_PAD, 0);
		gtk_widget_show (GTK_WIDGET (widget));

		gtk_box_pack_start (GTK_BOX (parent), widget, TRUE, TRUE, 0);
			
	} else if (matecomponent_ui_node_has_name (node, "control")) {
		char *behavior;
		char **behavior_array;

		gboolean pack_start = FALSE;
		gboolean fill = FALSE;
		gboolean expand = FALSE;

		widget = matecomponent_ui_engine_build_control (sync->engine, node);
		if (widget) {
			behavior_array = NULL;
			if ((behavior = matecomponent_ui_engine_get_attr (node, cmd_node, "behavior"))) {
				behavior_array = g_strsplit (behavior, ",", -1);
				matecomponent_ui_node_free_string (behavior);
			}

			if (behavior_array != NULL) {
				pack_start = string_array_contains (behavior_array, "pack-start");
				fill = string_array_contains (behavior_array, "fill");
				expand = string_array_contains (behavior_array, "expand");
			}
			
			if (pack_start)
				gtk_box_pack_start (GTK_BOX (parent), widget,
						    expand, fill, 0);
			else
				gtk_box_pack_end (GTK_BOX (parent), widget,
						  expand, fill, 0);

			g_strfreev (behavior_array);
		}
	}

	if (widget)
		gtk_box_reorder_child (msync->status, widget, (*pos)++);

	return widget;
}

static GtkWidget *
impl_matecomponent_ui_sync_status_build_placeholder (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      int              *pos,
					      GtkWidget        *parent)
{
	GtkWidget *widget;
	MateComponentUISyncStatus *msync = MATECOMPONENT_UI_SYNC_STATUS (sync);

	widget = gtk_vseparator_new ();
	gtk_widget_set_sensitive (widget, FALSE);

	gtk_box_pack_end (GTK_BOX (parent), widget,
			  FALSE, FALSE, 0);

	if (widget)
		gtk_box_reorder_child (msync->status, widget, (*pos)++);

	return widget;
}

static GList *
box_get_children_in_order (GtkBox *box)
{
	GList       *ret = NULL;
	GList       *l;

	g_return_val_if_fail (GTK_IS_BOX (box), NULL);

	for (l = box->children; l; l = l->next) {
		GtkBoxChild *child = l->data;

		ret = g_list_prepend (ret, child->widget);
	}

	return g_list_reverse (ret);
}

static void
impl_matecomponent_ui_sync_status_stamp_root (MateComponentUISync *sync)
{
	MateComponentUISyncStatus *sstatus = MATECOMPONENT_UI_SYNC_STATUS (sync);
	MateComponentUINode       *node;
	GtkWidget          *widget;

	node = matecomponent_ui_engine_get_path (sync->engine, "/status");

	if (node) {
		widget = GTK_WIDGET (sstatus->status);

		matecomponent_ui_engine_stamp_root (sync->engine, node, widget);

		matecomponent_ui_sync_do_show_hide (sync, node, NULL, widget);
	}
}

static GList *
impl_matecomponent_ui_sync_status_get_widgets (MateComponentUISync *sync,
					MateComponentUINode *node)
{
	if (matecomponent_ui_node_has_name (node, "status"))
		return box_get_children_in_order (
			GTK_BOX (MATECOMPONENT_UI_SYNC_STATUS (sync)->status));
	else
		return NULL;
}

static void
impl_dispose (GObject *object)
{
	MateComponentUISyncStatus *sync = (MateComponentUISyncStatus *) object;

	if (sync->status) {
		gtk_widget_destroy (GTK_WIDGET (sync->status));
		g_object_unref (sync->status);
		sync->status = NULL;
	}

	parent_class->dispose (object);
}

static void
impl_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static gboolean
impl_matecomponent_ui_sync_status_can_handle (MateComponentUISync *sync,
				       MateComponentUINode *node)
{
	return matecomponent_ui_node_has_name (node, "status");
}

/* We need to map the shell to the item */

static void
class_init (MateComponentUISyncClass *sync_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (sync_class);

	object_class = G_OBJECT_CLASS (sync_class);
	object_class->dispose  = impl_dispose;
	object_class->finalize = impl_finalize;

	sync_class->sync_state = impl_matecomponent_ui_sync_status_state;
	sync_class->build      = impl_matecomponent_ui_sync_status_build;
	sync_class->build_placeholder = impl_matecomponent_ui_sync_status_build_placeholder;

	sync_class->get_widgets   = impl_matecomponent_ui_sync_status_get_widgets;
	sync_class->stamp_root    = impl_matecomponent_ui_sync_status_stamp_root;

	sync_class->can_handle    = impl_matecomponent_ui_sync_status_can_handle;
}

static void
init (MateComponentUISyncStatus *msync)
{
}

GType
matecomponent_ui_sync_status_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUISyncStatusClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUISyncStatus),
			0, /* n_preallocs */
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUISyncStatus",
					       &info, 0);
	}

	return type;
}

MateComponentUISync *
matecomponent_ui_sync_status_new (MateComponentUIEngine *engine,
			   GtkBox         *status)
{
	MateComponentUISyncStatus *sync;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	sync = g_object_new (MATECOMPONENT_TYPE_UI_SYNC_STATUS, NULL);

	sync->status = g_object_ref (status);

	g_signal_connect (engine, "add_hint",
			  G_CALLBACK (set_hint_cb), sync);

	g_signal_connect (engine, "remove_hint",
			  G_CALLBACK (remove_hint_cb), sync);

	return matecomponent_ui_sync_construct (
		MATECOMPONENT_UI_SYNC (sync), engine, FALSE, TRUE);
}
