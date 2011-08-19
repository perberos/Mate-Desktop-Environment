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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 *          Carlos Garnacho Parro <carlosg@mate.org>
 */

#include <glib/gi18n.h>
#include <stdlib.h>
#include "gst.h"
#include "gst-platform-dialog.h"

#define GST_PLATFORM_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_PLATFORM_DIALOG, GstPlatformDialogPrivate))

enum {
	PLATFORM_LIST_COL_NAME,
	PLATFORM_LIST_COL_ID,
	PLATFORM_LIST_COL_LAST
};

typedef struct _GstPlatformDialogPrivate GstPlatformDialogPrivate;

struct _GstPlatformDialogPrivate
{
	GtkWidget *list;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
};

enum {
	PROP_0,
	PROP_SESSION
};

static void gst_platform_dialog_class_init (GstPlatformDialogClass *class);
static void gst_platform_dialog_init       (GstPlatformDialog      *dialog);
static void gst_platform_dialog_finalize   (GObject                *object);

static void gst_platform_dialog_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec);

static GObject* gst_platform_dialog_constructor (GType                  type,
						 guint                  n_construct_properties,
						 GObjectConstructParam *construct_params);

static void gst_platform_dialog_response   (GtkDialog *dialog,
					    gint       response);

G_DEFINE_TYPE (GstPlatformDialog, gst_platform_dialog, GTK_TYPE_DIALOG);

static void
gst_platform_dialog_class_init (GstPlatformDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	object_class->set_property = gst_platform_dialog_set_property;
	object_class->constructor  = gst_platform_dialog_constructor;
	object_class->finalize     = gst_platform_dialog_finalize;

	dialog_class->response     = gst_platform_dialog_response;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "session",
							      "session",
							      OOBS_TYPE_SESSION,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_type_class_add_private (object_class,
				  sizeof (GstPlatformDialogPrivate));
}

static void
on_selection_changed (GtkTreeSelection *selection,
		      gpointer          data)
{
	GstPlatformDialog *dialog;
	GstPlatformDialogPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean selected = FALSE;
	gchar *id;

	dialog = GST_PLATFORM_DIALOG (data);
	priv = dialog->_priv;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    PLATFORM_LIST_COL_ID, &id,
				    -1);

		selected = (id != NULL);
	}

	gtk_widget_set_sensitive (priv->ok_button, selected);
}

static GtkWidget*
gst_platform_dialog_create_treeview (GstPlatformDialog *dialog)
{
	GtkWidget *list;
	GtkTreeStore *store;
	GtkTreeModel *sort_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	list = gtk_tree_view_new ();
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);

	store = gtk_tree_store_new (PLATFORM_LIST_COL_LAST,
				    G_TYPE_STRING,
				    G_TYPE_POINTER);

	sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
					      PLATFORM_LIST_COL_NAME, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), sort_model);
	g_object_unref (sort_model);

	column = gtk_tree_view_column_new ();
	
	/* Insert the text cell */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "markup", PLATFORM_LIST_COL_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (on_selection_changed), dialog);
	return list;
}

static void
gst_platform_dialog_init (GstPlatformDialog *dialog)
{
	GstPlatformDialogPrivate *priv;
	GtkWidget *box, *title, *label, *scrolled_window;
	gchar *str;

	priv = GST_PLATFORM_DIALOG_GET_PRIVATE (dialog);
	dialog->_priv = priv;

	box = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);

	/* title label */
	title = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (title), TRUE);
	gtk_misc_set_alignment (GTK_MISC (title), 0., 0.);
	str = g_strdup_printf ("<span weight='bold' size='larger'>%s</span>",
			       _("The platform you are running is not supported by this tool"));
	gtk_label_set_markup (GTK_LABEL (title), str);
	gtk_box_pack_start (GTK_BOX (box), title, FALSE, FALSE, 0);
	g_free (str);

	/* label */
	label = gtk_label_new (_("If you know for sure that it works like one of "
				 "the platforms listed below, you can select that "
				 "and continue. Note, however, that this might "
				 "damage the system configuration or downright "
				 "cripple your computer."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	/* platforms list */
	priv->list = gst_platform_dialog_create_treeview (dialog);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->list);
	gtk_box_pack_start (GTK_BOX (box), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all (box);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box);

	priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Unsupported platform"));
	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
}

static void
gst_platform_dialog_finalize (GObject *object)
{
	GstPlatformDialog *dialog = GST_PLATFORM_DIALOG (object);

	if (dialog->session)
		g_object_unref (dialog->session);

	(* G_OBJECT_CLASS (gst_platform_dialog_parent_class)->finalize) (object);
}

static GHashTable*
get_hash_table (GList *platforms)
{
	GHashTable *distros;
	GList *list = platforms;
	GList *versions;
	OobsPlatform *platform;

	/* values contain a list of OobsPlatform*, the list must be
	 * freed, the OobsPlatform structs are managed by liboobs
	 */
	distros = g_hash_table_new_full (g_str_hash, g_str_equal,
					 (GDestroyNotify) g_free,
					 (GDestroyNotify) g_list_free);

	while (list) {
		platform = list->data;
		versions = g_hash_table_lookup (distros, platform->name);
		g_hash_table_steal (distros, platform->name);

		/* add the version to the distro list */
		versions = g_list_append (versions, platform);
		g_hash_table_insert (distros, (gpointer) platform->name, versions);

		list = list->next;
	}

	return distros;
}

static gchar *
get_distro_string (const gchar  *prefix,
		   OobsPlatform *platform)
{
	GString *str;
	gchar *s;

	str = g_string_new (prefix);

	if (platform->version)
		g_string_append_printf (str, " %s", platform->version);

	if (platform->codename)
		g_string_append_printf (str, " (%s)", platform->codename);
	
	s = str->str;
	g_string_free (str, FALSE);
	return s;
}

static void
populate_distro (gchar        *distro,
		 GList        *versions,
		 GtkTreeStore *store)
{
	GtkTreeIter parent_iter, iter;
	GList *elem = versions;
	OobsPlatform *platform;
	gchar *str;

	if (g_list_length (elem) == 1) {
		platform = elem->data;
		str = get_distro_string (distro, platform);

		gtk_tree_store_append (store, &parent_iter, NULL);
		gtk_tree_store_set (store, &parent_iter,
				    PLATFORM_LIST_COL_NAME, str,
				    PLATFORM_LIST_COL_ID, platform->id,
				    -1);
		g_free (str);
	} else {
		gtk_tree_store_append (store, &parent_iter, NULL);
		gtk_tree_store_set (store, &parent_iter,
				    PLATFORM_LIST_COL_NAME, distro,
				    -1);
		while (elem) {
			platform = elem->data;
			str = get_distro_string (NULL, platform);

			gtk_tree_store_append (store, &iter, &parent_iter);
			gtk_tree_store_set (store, &iter,
					    PLATFORM_LIST_COL_NAME, str,
					    PLATFORM_LIST_COL_ID, platform->id,
					    -1);
			elem = elem->next;
			g_free (str);
		}
	}
}

static void
gst_platform_dialog_populate_list (GstPlatformDialog *dialog)
{
	GstPlatformDialogPrivate *priv;
	GtkTreeModel *sort_model;
	GtkTreeStore *store;
	GList *platforms;
	GHashTable *distros;

	g_return_if_fail (OOBS_IS_SESSION (dialog->session));

	if (oobs_session_get_supported_platforms (dialog->session, &platforms) != OOBS_RESULT_OK)
		return;

	priv = dialog->_priv;
	distros = get_hash_table (platforms);

	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->list));
	store = GTK_TREE_STORE (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model)));

	g_hash_table_foreach (distros, (GHFunc) populate_distro, store);
	g_hash_table_destroy (distros);
	g_list_free (platforms);
}

static GObject*
gst_platform_dialog_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_params)
{
	GObject *object;

	object = (* G_OBJECT_CLASS (gst_platform_dialog_parent_class)->constructor) (type,
										     n_construct_properties,
										     construct_params);
	gst_platform_dialog_populate_list (GST_PLATFORM_DIALOG (object));

	return object;
}

static void
gst_platform_dialog_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GstPlatformDialog *dialog = GST_PLATFORM_DIALOG (object);

	switch (prop_id) {
	case PROP_SESSION:
		dialog->session = g_value_dup_object (value);
		break;
	}
}

static void
gst_platform_dialog_response (GtkDialog *dialog,
			      gint       response)
{
	if (response == GTK_RESPONSE_OK) {
		GstPlatformDialog *platform_dialog;
		GstPlatformDialogPrivate *priv;
		GtkTreeSelection *selection;
		GtkTreeModel *sort_model, *model;
		GtkTreeIter iter, child_iter;
		gchar *platform;

		platform_dialog = GST_PLATFORM_DIALOG (dialog);
		priv = platform_dialog->_priv;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->list));

		if (gtk_tree_selection_get_selected (selection, &sort_model, &iter)) {
			model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
			gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (sort_model),
									&child_iter, &iter);
			gtk_tree_model_get (model, &child_iter,
					    PLATFORM_LIST_COL_ID, &platform,
					    -1);

			oobs_session_set_platform (platform_dialog->session, platform);
			g_free (platform);
		}
	} else {
		exit (0);
	}
}

GtkWidget*
gst_platform_dialog_new (OobsSession *session)
{
	g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

	return g_object_new (GST_TYPE_PLATFORM_DIALOG,
			     "has-separator", FALSE,
			     "session", session,
			     NULL);
}
