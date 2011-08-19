/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Jacob Berkman   <jacob@ximian.com>
 *          Carlos Garnacho <carlosg@mate.org>
 */

#include <config.h>

#include <gmodule.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include "gst-tool.h"
#include "gst-dialog.h"

#ifdef HAVE_POLKIT
#include <polkitgtk/polkitgtk.h>
#endif

#define GST_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_DIALOG, GstDialogPrivate))

typedef struct _GstDialogPrivate GstDialogPrivate;
typedef struct _GstWidgetPolicy  GstWidgetPolicy;

struct _GstDialogPrivate {
	GstTool *tool;

	gchar   *title;
	gchar   *widget_name;
	gboolean lock_button;

	GtkBuilder *builder;
	GtkWidget  *child;
#ifdef HAVE_POLKIT
	GtkWidget  *polkit_button;
#endif
	GSList     *policy_widgets;

	GSList     *edit_dialogs;

	guint    frozen;
	guint    modified : 1;
};

struct _GstWidgetPolicy {
	gushort need_access   : 1;
	gushort was_sensitive : 1;
};

static GQuark widget_policy_quark;

static void gst_dialog_class_init (GstDialogClass *class);
static void gst_dialog_init       (GstDialog      *dialog);
static void gst_dialog_finalize   (GObject        *object);

static GObject*	gst_dialog_constructor (GType                  type,
					guint                  n_construct_properties,
					GObjectConstructParam *construct_params);

static void gst_dialog_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);

static void gst_dialog_response (GtkDialog *dialog,
				 gint       response);

static gboolean gst_dialog_delete_event (GtkWidget   *widget,
					 GdkEventAny *event);
static void     gst_dialog_realize      (GtkWidget   *widget);

static void     gst_dialog_set_cursor   (GstDialog     *dialog,
					 GdkCursorType  cursor_type);

static void             gst_dialog_lock_changed          (GstDialog *dialog);
static GstWidgetPolicy *gst_dialog_get_policy_for_widget (GtkWidget *widget);

enum {
	PROP_0,
	PROP_TOOL,
	PROP_WIDGET_NAME,
	PROP_TITLE,
	PROP_LOCK_BUTTON
};

enum {
	LOCK_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GstDialog, gst_dialog, GTK_TYPE_DIALOG);

static void
gst_dialog_class_init (GstDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->set_property = gst_dialog_set_property;
	object_class->constructor  = gst_dialog_constructor;
	object_class->finalize     = gst_dialog_finalize;

	dialog_class->response     = gst_dialog_response;
	widget_class->delete_event = gst_dialog_delete_event;
	widget_class->realize      = gst_dialog_realize;

	widget_policy_quark = g_quark_from_static_string ("gst-dialog-widget-policy");

	g_object_class_install_property (object_class,
					 PROP_TOOL,
					 g_param_spec_object ("tool",
							      "tool",
							      "tool",
							      GST_TYPE_TOOL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_WIDGET_NAME,
					 g_param_spec_string ("widget_name",
							      "Widget name",
							      "Widget name",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_TITLE,
					 g_param_spec_string ("title",
							      "title",
							      "title",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_LOCK_BUTTON,
					 g_param_spec_boolean ("lock_button",
					                       "Lock button",
					                       "Show PolkitLockButton",
					                       TRUE,
					                       G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	signals [LOCK_CHANGED] =
		g_signal_new ("lock-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstDialogClass, lock_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class,
				  sizeof (GstDialogPrivate));
}

static void
gst_dialog_init (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	priv->tool  = NULL;
	priv->title = NULL;
	priv->builder = NULL;
	priv->child = NULL;

	priv->modified = FALSE;

	priv->policy_widgets = NULL;
}

static GObject*
gst_dialog_constructor (GType                  type,
			guint                  n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject *object;
	GstDialog *dialog;
	GstDialogPrivate *priv;
	GtkWidget *toplevel;
	GError *error = NULL;

	object = (* G_OBJECT_CLASS (gst_dialog_parent_class)->constructor) (type,
									    n_construct_properties,
									    construct_params);
	dialog = GST_DIALOG (object);
	priv = GST_DIALOG_GET_PRIVATE (dialog);

	if (priv->tool && priv->widget_name) {
		priv->builder = gtk_builder_new ();

		if (!gtk_builder_add_from_file (priv->builder, priv->tool->ui_path, &error)) {
			g_critical ("Error loading UI: %s", error->message);
			g_error_free (error);

			/* no point in continuing */
			exit (-1);
		}
		gtk_builder_connect_signals (priv->builder, dialog);
		priv->child = gst_dialog_get_widget (dialog, priv->widget_name);
		toplevel = gtk_widget_get_toplevel (priv->child);

		if (gtk_widget_is_toplevel (priv->child)) {
			g_critical ("The widget \"%s\" should not be a toplevel widget in the .ui file\n"
				    "You just need to add the widget inside a GtkWindow so that it can be deparented.", priv->widget_name);

			/* no point in continuing */
			exit (-1);
		}

		g_object_ref (priv->child);
		gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (priv->child)), priv->child);
		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), priv->child);

		gtk_widget_destroy (toplevel);
	}

#ifdef HAVE_POLKIT
	if (priv->tool) {
		const gchar *action;
		GtkWidget *action_area;

		/* Some tools don't use the lock button at all */
		if (priv->lock_button) {
			action = oobs_session_get_authentication_action (priv->tool->session);
			priv->polkit_button = polkit_lock_button_new (action);

			action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
			gtk_box_pack_start (GTK_BOX (action_area), priv->polkit_button, TRUE, TRUE, 0);
			gtk_widget_show (priv->polkit_button);

			g_signal_connect_swapped (priv->polkit_button, "changed",
		                          G_CALLBACK (gst_dialog_lock_changed), dialog);
		}
	}
#endif

	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_HELP, GTK_RESPONSE_HELP,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);

	if (priv->title)
		gtk_window_set_title (GTK_WINDOW (dialog), priv->title);

	return object;
}

static void
gst_dialog_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_TOOL:
		priv->tool = GST_TOOL (g_value_get_object (value));
		break;
	case PROP_WIDGET_NAME:
		priv->widget_name = g_value_dup_string (value);
		break;
	case PROP_TITLE:
		priv->title = g_value_dup_string (value);
		break;
	case PROP_LOCK_BUTTON:
		priv->lock_button = g_value_get_boolean (value);
		break;
	}
}

static void
gst_dialog_finalize (GObject *object)
{
	GstDialog *dialog = GST_DIALOG (object);
	GstDialogPrivate *priv = GST_DIALOG_GET_PRIVATE (dialog);

	if (priv->builder)
		g_object_unref (priv->builder);

	if (priv->child)
		gtk_widget_destroy (priv->child);

	g_free (priv->title);
	g_free (priv->widget_name);

	g_slist_free (priv->policy_widgets);

	(* G_OBJECT_CLASS (gst_dialog_parent_class)->finalize) (object);
}

static void
gst_dialog_response (GtkDialog *dialog,
		     gint       response)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	switch (response) {
	case GTK_RESPONSE_CLOSE:
	case GTK_RESPONSE_DELETE_EVENT:
		if (gtk_widget_get_sensitive (GTK_WIDGET (dialog))) {
			gtk_widget_hide (GTK_WIDGET (dialog));
			gst_tool_close (priv->tool);
		}

		break;
	case GTK_RESPONSE_HELP:
		gst_tool_show_help (priv->tool, NULL);
		break;
	}
}

static void
gst_dialog_realize (GtkWidget *widget)
{
	GstDialogPrivate *priv;

	(* GTK_WIDGET_CLASS (gst_dialog_parent_class)->realize) (widget);

	priv = GST_DIALOG_GET_PRIVATE (widget);

	if (priv->frozen > 0)
		gst_dialog_set_cursor (GST_DIALOG (widget), GDK_WATCH);
}

static gboolean
gst_dialog_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
	/* we don't want the dialog to be destroyed at this point */
	return TRUE;
}

GstDialog*
gst_dialog_new (GstTool *tool, const char *widget, const char *title, gboolean lock_button)
{
	return g_object_new (GST_TYPE_DIALOG,
			     "has-separator", FALSE,
			     "tool", tool,
			     "widget-name", widget,
			     "title", title,
	                     "lock-button", lock_button,
			     NULL);
}

GtkWidget *
gst_dialog_get_widget (GstDialog *dialog, const char *widget)
{
	GstDialogPrivate *priv;
	GtkWidget *w;

	g_return_val_if_fail (dialog != NULL, NULL);
	g_return_val_if_fail (GST_IS_DIALOG (dialog), NULL);

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	g_return_val_if_fail (priv->builder != NULL, NULL);

	w = GTK_WIDGET (gtk_builder_get_object (priv->builder, widget));

	if (!w)
		g_error ("Could not find widget: %s", widget);

	return w;
}

static void
gst_dialog_lock_changed (GstDialog *dialog)
{
	GstDialogPrivate *priv;
	GSList *list;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	for (list = priv->policy_widgets; list; list = g_slist_next (list)) {
		GtkWidget *widget;
		GstWidgetPolicy *policy;

		widget = list->data;
		if (gst_dialog_is_authenticated (dialog)) {
			policy = gst_dialog_get_policy_for_widget (widget);
			gtk_widget_set_sensitive (widget, policy->was_sensitive);
		}
		else
			gtk_widget_set_sensitive (widget, FALSE);
	}

	g_signal_emit (dialog, signals [LOCK_CHANGED], 0);
}

static void
gst_dialog_set_cursor (GstDialog     *dialog,
		       GdkCursorType  cursor_type)
{
	GdkCursor *cursor;

	g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (dialog)));

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (dialog)), cursor);
	gdk_cursor_unref (cursor);
}

void
gst_dialog_freeze (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	g_return_if_fail (GST_IS_DIALOG (dialog));

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	if (priv->frozen == 0) {
		if (gtk_widget_get_window (GTK_WIDGET (dialog)))
			gst_dialog_set_cursor (dialog, GDK_WATCH);

		gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);
	}

	priv->frozen++;
}

void
gst_dialog_thaw (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	g_return_if_fail (GST_IS_DIALOG (dialog));

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	g_return_if_fail (priv->frozen != 0);

	priv->frozen--;

	if (priv->frozen == 0) {
		if (gtk_widget_get_window (GTK_WIDGET (dialog)))
			gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (dialog)), NULL);

		gtk_widget_set_sensitive (GTK_WIDGET (dialog), TRUE);
	}
}

guint
gst_dialog_get_freeze_level (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	g_return_val_if_fail (GST_IS_DIALOG (dialog), 0);

	priv = GST_DIALOG_GET_PRIVATE (dialog);
	return priv->frozen;
}

gboolean
gst_dialog_get_modified (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	g_return_val_if_fail (dialog != NULL, FALSE);
	g_return_val_if_fail (GST_IS_DIALOG (dialog), FALSE);

	priv = GST_DIALOG_GET_PRIVATE (dialog);
	return priv->modified;
}

void
gst_dialog_set_modified (GstDialog *dialog, gboolean state)
{
	GstDialogPrivate *priv;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));

	priv = GST_DIALOG_GET_PRIVATE (dialog);
	priv->modified = state;
}

void
gst_dialog_modify (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	if (priv->frozen || !gst_dialog_is_authenticated (dialog))
		return;

	gst_dialog_set_modified (dialog, TRUE);
}

void
gst_dialog_modify_cb (GtkWidget *w, gpointer data)
{
	gst_dialog_modify (data);
}

GstTool*
gst_dialog_get_tool (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);
	return priv->tool;
}

static GstWidgetPolicy *
gst_dialog_get_policy_for_widget (GtkWidget *widget)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	return g_object_get_qdata (G_OBJECT (widget), widget_policy_quark);
}

void
gst_dialog_require_authentication_for_widget (GstDialog *xd, GtkWidget *w)
{
	GstDialogPrivate *priv;
	GstWidgetPolicy *policy;

	g_return_if_fail (GST_IS_DIALOG (xd));
	g_return_if_fail (GTK_IS_WIDGET (w));

	priv = GST_DIALOG_GET_PRIVATE (xd);
	policy = g_new0 (GstWidgetPolicy, 1);

	policy->was_sensitive = gtk_widget_get_sensitive (w);

	g_object_set_qdata_full (G_OBJECT (w), widget_policy_quark,
				 policy, (GDestroyNotify) g_free);

	priv->policy_widgets = g_slist_prepend (priv->policy_widgets, w);

	gtk_widget_set_sensitive (w, gst_dialog_is_authenticated (xd));
}

void
gst_dialog_require_authentication_for_widgets (GstDialog *xd, const gchar **names)
{
	GstDialogPrivate *priv;

	g_return_if_fail (GST_IS_DIALOG (xd));

	priv = GST_DIALOG_GET_PRIVATE (xd);

	for (; *names != NULL; names++) {
		GtkWidget *widget;

		widget = gst_dialog_get_widget (xd, *names);

		if (!widget) {
			g_warning ("widget named '%s' doesn't exist", *names);
			continue;
		}

		gst_dialog_require_authentication_for_widget (xd, widget);
	}
}

void
gst_dialog_try_set_sensitive (GstDialog *dialog, GtkWidget *w, gboolean sensitive)
{
	g_return_if_fail (GST_IS_DIALOG (dialog));
	g_return_if_fail (GTK_IS_WIDGET (w));

	if (sensitive) {
		GstWidgetPolicy *policy;

		policy = gst_dialog_get_policy_for_widget (w);

		if (policy) {
			policy->was_sensitive = TRUE;
			sensitive = gst_dialog_is_authenticated (dialog);
		}
	}

	gtk_widget_set_sensitive (w, sensitive);
}

static void
dialog_connect_signals (GstDialog *dialog, GstDialogSignal *signals, gboolean connect_after)
{       
	GtkWidget *w;
	guint sig;
	int i;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));

	for (i=0; signals[i].widget; i++) {
		w = gst_dialog_get_widget (dialog, signals[i].widget);

		if (connect_after) {
			sig = g_signal_connect_after (G_OBJECT (w),
						      signals[i].signal_name,
						      G_CALLBACK (signals[i].func),
						      dialog);
		} else {
			sig = g_signal_connect (G_OBJECT (w),
						signals[i].signal_name,
						G_CALLBACK (signals[i].func),
						dialog);
		}
		
		if (G_UNLIKELY (!sig))
			g_error ("Error connecting signal `%s' in widget `%s'",
				 signals[i].signal_name, signals[i].widget);
	}
}

void
gst_dialog_connect_signals (GstDialog *dialog, GstDialogSignal *signals)
{
	dialog_connect_signals (dialog, signals, FALSE);
}

void
gst_dialog_connect_signals_after (GstDialog *dialog, GstDialogSignal *signals)
{
	dialog_connect_signals (dialog, signals, TRUE);
}

gboolean
gst_dialog_is_authenticated (GstDialog *dialog)
{
#ifdef HAVE_POLKIT
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	return (polkit_lock_button_get_is_authorized (POLKIT_LOCK_BUTTON (priv->polkit_button)) ||
		(polkit_lock_button_get_can_obtain (POLKIT_LOCK_BUTTON (priv->polkit_button)) &&
		!polkit_lock_button_get_is_visible (POLKIT_LOCK_BUTTON (priv->polkit_button))));
#else
	return TRUE;
#endif
}

gboolean
gst_dialog_get_editing (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	return (priv->edit_dialogs != NULL);
}

GtkWidget *
gst_dialog_get_topmost_edit_dialog (GstDialog *dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	if (!priv->edit_dialogs)
		return NULL;

	return GTK_WIDGET (priv->edit_dialogs->data);
}

void
gst_dialog_add_edit_dialog (GstDialog *dialog,
			    GtkWidget *edit_dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	priv->edit_dialogs = g_slist_prepend (priv->edit_dialogs, edit_dialog);
}

void
gst_dialog_remove_edit_dialog (GstDialog *dialog,
			       GtkWidget *edit_dialog)
{
	GstDialogPrivate *priv;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	priv->edit_dialogs = g_slist_remove (priv->edit_dialogs, edit_dialog);
}

void
gst_dialog_stop_editing (GstDialog *dialog)
{
	GstDialogPrivate *priv;
	GSList *dialogs;

	priv = GST_DIALOG_GET_PRIVATE (dialog);

	for (dialogs= priv->edit_dialogs; dialogs; dialogs = dialogs->next) {
		/* hide dialogs, if they were running inside
		 * gtk_dialog_run (), the loop will shutdown
		 */
		gtk_widget_hide (GTK_WIDGET (dialogs->data));
	}

	g_slist_free (priv->edit_dialogs);
	priv->edit_dialogs = NULL;
}
