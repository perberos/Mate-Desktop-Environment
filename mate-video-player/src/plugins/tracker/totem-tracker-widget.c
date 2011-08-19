/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * The _get_result_count method taken from the tracker-client.h file from libtracker
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@mate.org>
 * Copyright (C) 2007, Javier Goday <jgoday@gmail.com>
 * Copyright (C) 2010, Martyn Russell <martyn@lanedo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Jamie McCracken <jamiemcc@mate.org>
 *         Javier Goday <jgoday@gmail.com>
 *         Martyn Russell <martyn@lanedo.com>
 */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <dbus/dbus.h>
#include <libtracker-client/tracker-client.h>

#include "totem-tracker-widget.h"
#include "totem-cell-renderer-video.h"
#include "totem-playlist.h"
#include "totem-video-list.h"
#include "totem-interface.h"

#define TOTEM_TRACKER_MAX_RESULTS_SIZE	20

G_DEFINE_TYPE (TotemTrackerWidget, totem_tracker_widget, GTK_TYPE_VBOX)

struct TotemTrackerWidgetPrivate {
	GtkWidget *search_entry;
	GtkWidget *search_button;
	GtkWidget *status_label;

	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *page_selector;

	guint total_result_count;
	guint current_result_page;

	GtkListStore *result_store;
	TotemVideoList *result_list;
};

typedef struct {
	TotemTrackerWidget *widget;
	TrackerClient *client;
	gchar *search_text;
	guint cookie;
} SearchResultsData;

enum {
	IMAGE_COLUMN,
	FILE_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};

enum {
	PROP_0,
	PROP_TOTEM
};

static void totem_tracker_widget_dispose	(GObject *object);
static void totem_tracker_widget_set_property	(GObject *object,
						 guint property_id,
						 const GValue *value,
						 GParamSpec *pspec);
static void page_selector_value_changed_cb (GtkSpinButton *self, TotemTrackerWidget *widget);

static void
totem_tracker_widget_class_init (TotemTrackerWidgetClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (TotemTrackerWidgetPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = totem_tracker_widget_dispose;
	object_class->set_property = totem_tracker_widget_set_property;

	g_object_class_install_property (object_class, PROP_TOTEM,
					 g_param_spec_object ("totem", NULL, NULL,
							      TOTEM_TYPE_OBJECT, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
totem_tracker_widget_dispose (GObject *object)
{
	TotemTrackerWidget *self = TOTEM_TRACKER_WIDGET (object);

	if (self->priv->result_store != NULL) {
		g_object_unref (self->priv->result_store);
		self->priv->result_store = NULL;
	}

	G_OBJECT_CLASS (totem_tracker_widget_parent_class)->dispose (object);
}

static void
totem_tracker_widget_set_property (GObject *object,
				   guint property_id,
				   const GValue *value,
				   GParamSpec *pspec)
{
	TotemTrackerWidget *widget;

	widget = TOTEM_TRACKER_WIDGET (object);

	switch (property_id)
	{
	case PROP_TOTEM:
		widget->totem = g_value_dup_object (value);
		g_object_set (G_OBJECT (widget->priv->result_list), "totem", widget->totem, NULL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
search_results_populate (TotemTrackerWidget *widget, 
			 const gchar        *uri)
{	
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;

	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file, "standard::display-name,thumbnail::path", G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error == NULL) {
		GtkTreeIter iter;
		GdkPixbuf *thumbnail = NULL;
		const gchar *thumbnail_path;

		gtk_list_store_append (GTK_LIST_STORE (widget->priv->result_store), &iter);  /* Acquire an iterator */
		thumbnail_path = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);

		if (thumbnail_path != NULL)
			thumbnail = gdk_pixbuf_new_from_file (thumbnail_path, NULL);

		gtk_list_store_set (GTK_LIST_STORE (widget->priv->result_store), &iter,
				    IMAGE_COLUMN, thumbnail,
				    FILE_COLUMN, uri,
				    NAME_COLUMN, g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME),
				    -1);

		if (thumbnail != NULL)
			g_object_unref (thumbnail);
	} else {
		/* Display an error */
		gchar *message = g_strdup_printf (_("Could not get name and thumbnail for %s: %s"), uri, error->message);
		totem_interface_error_blocking	(_("File Error"), message, NULL);
		g_free (message);
		g_error_free (error);
	}

	g_object_unref (info);
	g_object_unref (file);
}

static SearchResultsData *
search_results_new (TotemTrackerWidget *widget,
		    const gchar        *search_text)
{
	SearchResultsData *srd;
	TrackerClient *client;

	if (!widget) {
		return NULL;
	}

	client = tracker_client_new (TRACKER_CLIENT_ENABLE_WARNINGS, G_MAXINT);
	if (!client) {
		return NULL;
	}

	srd = g_slice_new0 (SearchResultsData);

	srd->widget = g_object_ref (widget);
	srd->client = client;
	srd->search_text = g_strdup (search_text);

	return srd;
}

static void
search_results_free (SearchResultsData *srd)
{
	if (!srd) {
		return;
	}

	if (srd->cookie != 0) {
		tracker_cancel_call (srd->client, srd->cookie);
	}

	if (srd->widget) {
		g_object_unref (srd->widget);
	}

	if (srd->client) {
		g_object_unref (srd->client);
	}

	g_free (srd->search_text);
	g_slice_free (SearchResultsData, srd);
}

static void
search_results_cb (GPtrArray *results, 
		   GError    *error, 
		   gpointer   userdata)
{
	TotemTrackerWidgetPrivate *priv;
	SearchResultsData *srd;
	gchar *str;
	guint i, next_page, total_pages;

	srd = userdata;
	priv = srd->widget->priv;

	gtk_widget_set_sensitive (priv->search_entry, TRUE);

	if (error) {
		g_warning ("Error getting the search results for '%s': %s", 
			   srd->search_text, 
			   error->message ? error->message : "No reason");

		gtk_label_set_text (GTK_LABEL (priv->status_label), _("Could not connect to Tracker"));
		search_results_free (srd);

		return;
	}

	if (!results || results->len < 1) {
		gtk_label_set_text (GTK_LABEL (priv->status_label), _("No results"));
		search_results_free (srd);

		return;
	}

	for (i = 0; i < results->len; i++) {
		GStrv details;

		details = g_ptr_array_index (results, i);
		search_results_populate (srd->widget, details[0]);
	}
	
	next_page = (priv->current_result_page + 1) * TOTEM_TRACKER_MAX_RESULTS_SIZE;
	total_pages = priv->total_result_count / TOTEM_TRACKER_MAX_RESULTS_SIZE + 1;
	
	/* Set the new range on the page selector's adjustment and ensure the current page is correct */
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->page_selector), 1, total_pages);
	priv->current_result_page = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->page_selector)) - 1;
	
	/* Translators:
	 * This is used to show which items are listed in the list view, for example:
	 * Showing 10-20 of 128 matches
	 * This is similar to what web searches use, eg. Google on the top-right of their search results page show:
	 * Personalized Results 1 - 10 of about 4,130,000 for foobar */
	str = g_strdup_printf (ngettext ("Showing %i - %i of %i match", "Showing %i - %i of %i matches", priv->total_result_count),
			       priv->current_result_page * TOTEM_TRACKER_MAX_RESULTS_SIZE, 
			       next_page > priv->total_result_count ? priv->total_result_count : next_page,
			       priv->total_result_count);
	gtk_label_set_text (GTK_LABEL (priv->status_label), str);
	g_free (str);
	
	/* Enable or disable the pager buttons */
	if (priv->current_result_page < priv->total_result_count / TOTEM_TRACKER_MAX_RESULTS_SIZE) {
		gtk_widget_set_sensitive (GTK_WIDGET (priv->page_selector), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (priv->next_button), TRUE);
	}
	
	if (priv->current_result_page > 0) {
		gtk_widget_set_sensitive (GTK_WIDGET (priv->page_selector), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (priv->previous_button), TRUE);
	}

	if (priv->current_result_page >= total_pages - 1) {
		gtk_widget_set_sensitive (GTK_WIDGET (priv->next_button), FALSE);
	}
	
	g_signal_handlers_unblock_by_func (priv->page_selector, page_selector_value_changed_cb, srd->widget);
	search_results_free (srd);
}

static gchar *
get_fts_string (GStrv    search_words,
		gboolean use_or_operator)
{
	GString *fts;
	gint i, len;

	if (!search_words) {
		return NULL;
	}

	fts = g_string_new ("");
	len = g_strv_length (search_words);

	for (i = 0; i < len; i++) {
		g_string_append (fts, search_words[i]);
		g_string_append_c (fts, '*');

		if (i < len - 1) { 
			if (use_or_operator) {
				g_string_append (fts, " OR ");
			} else {
				g_string_append (fts, " ");
			}
		}
	}

	return g_string_free (fts, FALSE);
}

static void
do_search (TotemTrackerWidget *widget)
{
	SearchResultsData *srd;
	GPtrArray *results;
	GError *error = NULL;
	const gchar *search_text;
	gchar *fts, *query;
	guint offset;

	/* Cancel previous searches */
	/* tracker_cancel_call (widget->priv->cookie_id); */

	/* Clear the list store */
	gtk_list_store_clear (GTK_LIST_STORE (widget->priv->result_store));

	/* Stop pagination temporarily */
	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->previous_button), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->page_selector), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->next_button), FALSE);

	/* Stop after clearing the list store if they're just emptying the search entry box */
	search_text = gtk_entry_get_text (GTK_ENTRY (widget->priv->search_entry));

	g_signal_handlers_block_by_func (widget->priv->page_selector, page_selector_value_changed_cb, widget);

	/* Get the tracker client */
	srd = search_results_new (widget, search_text);
	if (!srd) {
		gtk_label_set_text (GTK_LABEL (widget->priv->status_label), _("Could not connect to Tracker"));
		return;
	}

	offset = widget->priv->current_result_page * TOTEM_TRACKER_MAX_RESULTS_SIZE;

	if (search_text && search_text[0] != '\0') {
		GStrv strv;

		strv = g_strsplit (search_text, " ", -1);
		fts = get_fts_string (strv, FALSE);
		g_strfreev (strv);
	} else {
		fts = NULL;
	}

	/* NOTE: We use FILTERS here for the type because we may want
	 * to show more items than just videos in the future - like
	 * music or some other specialised content.
	 */
	if (fts) {
		query = g_strdup_printf ("SELECT COUNT(?urn) "
					 "WHERE {"
					 "  ?urn a nmm:Video ;"
					 "  fts:match \"%s\" ;"
					 "  tracker:available true . "
					 "}",
					 fts);
	} else {
		query = g_strdup_printf ("SELECT COUNT(?urn) "
					 "WHERE {"
					 "  ?urn a nmm:Video ;"
					 "  tracker:available true . "
					 "}");
	}

	results = tracker_resources_sparql_query (srd->client, query, &error);
	g_free (query);

	if (results && results->pdata && results->pdata[0]) {
		GStrv count;

		count = g_ptr_array_index (results, 0);
		widget->priv->total_result_count = atoi (count[0]);
	} else {
		widget->priv->total_result_count = 0;

		g_warning ("Could not get count for videos available, %s",
			   error ? error->message : "no error given"); 
		g_clear_error (&error);
	}

	if (fts) {
		query = g_strdup_printf ("SELECT nie:url(?urn) "
					 "WHERE {"
					 "  ?urn a nmm:Video ;"
					 "  fts:match \"%s\" ;"
					 "  tracker:available true . "
					 "} "
					 "ORDER BY DESC(fts:rank(?urn)) ASC(nie:url(?urn)) "
					 "OFFSET %d "
					 "LIMIT %d",
					 fts,
					 offset,
					 TOTEM_TRACKER_MAX_RESULTS_SIZE);
	} else {
		query = g_strdup_printf ("SELECT nie:url(?urn) "
					 "WHERE {"
					 "  ?urn a nmm:Video ; "
					 "  tracker:available true . "
					 "} "
					 "ORDER BY DESC(fts:rank(?urn)) ASC(nie:url(?urn)) "
					 "OFFSET %d "
					 "LIMIT %d",
					 offset,
					 TOTEM_TRACKER_MAX_RESULTS_SIZE);
	}

	g_free (fts);

	gtk_widget_set_sensitive (widget->priv->search_entry, FALSE);

	/* Cookie is used for cancelling */
	srd->cookie = tracker_resources_sparql_query_async (srd->client, 
							    query, 
							    search_results_cb, 
							    srd);
	g_free (query);
}

static void
go_next (GtkWidget *button, TotemTrackerWidget *widget)
{
	if (widget->priv->current_result_page < widget->priv->total_result_count / TOTEM_TRACKER_MAX_RESULTS_SIZE)
		gtk_spin_button_spin (GTK_SPIN_BUTTON (widget->priv->page_selector), GTK_SPIN_STEP_FORWARD, 1);

	/* do_search is called via page_selector_value_changed_cb */
}

static void
go_previous (GtkWidget *button, TotemTrackerWidget *widget)
{
	if (widget->priv->current_result_page > 0)
		gtk_spin_button_spin (GTK_SPIN_BUTTON (widget->priv->page_selector), GTK_SPIN_STEP_BACKWARD, 1);

	/* do_search is called via page_selector_value_changed_cb */
}

static void
page_selector_value_changed_cb (GtkSpinButton *self, TotemTrackerWidget *widget)
{
	widget->priv->current_result_page = gtk_spin_button_get_value (self) - 1;
	do_search (widget);
}

static void
new_search (GtkButton *button, TotemTrackerWidget *widget)
{
	/* Reset from the last search */
	g_signal_handlers_block_by_func (widget->priv->page_selector, page_selector_value_changed_cb, widget);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget->priv->page_selector), 1);
	g_signal_handlers_unblock_by_func (widget->priv->page_selector, page_selector_value_changed_cb, widget);

	do_search (widget);
}

static void
init_result_list (TotemTrackerWidget *widget)
{
	/* Initialize the store result list */
	widget->priv->result_store = gtk_list_store_new (N_COLUMNS, 
							 GDK_TYPE_PIXBUF,
							 G_TYPE_STRING,
							 G_TYPE_STRING);

	/* Create the gtktreewidget to show the results */
	widget->priv->result_list = g_object_new (TOTEM_TYPE_VIDEO_LIST,
						  "mrl-column", FILE_COLUMN,
						  "tooltip-column", NAME_COLUMN,
						  NULL);

	gtk_tree_view_set_model (GTK_TREE_VIEW (widget->priv->result_list), 
				 GTK_TREE_MODEL (widget->priv->result_store));
}

static void
initialize_list_store (TotemTrackerWidget *widget) 
{
	TotemCellRendererVideo *renderer;
	GtkTreeViewColumn *column;

	/* Initialise the columns of the result list */
	renderer = totem_cell_renderer_video_new (TRUE);
	column = gtk_tree_view_column_new_with_attributes (_("Search Results"), GTK_CELL_RENDERER (renderer),
							   "thumbnail", IMAGE_COLUMN, 
							   "title", NAME_COLUMN, 
							   NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->result_list), column);
}

static void
search_entry_changed_cb (GtkWidget *entry,
			 gpointer   user_data)
{
	TotemTrackerWidget *widget = user_data;

	gtk_label_set_text (GTK_LABEL (widget->priv->status_label), "");
}

static void
totem_tracker_widget_init (TotemTrackerWidget *widget)
{
	GtkWidget *v_box;		/* the main vertical box of the widget */
	GtkWidget *h_box;
	GtkWidget *pager_box;		/* box that holds the next and previous buttons */
	GtkScrolledWindow *scroll;	/* make the result list scrollable */
	GtkAdjustment *adjust;		/* adjustment for the page selector spin button */

	widget->priv = G_TYPE_INSTANCE_GET_PRIVATE (widget, TOTEM_TYPE_TRACKER_WIDGET, TotemTrackerWidgetPrivate);

	init_result_list (widget);

	v_box = gtk_vbox_new (FALSE, 6);
	h_box = gtk_hbox_new (FALSE, 6);

	/* Search entry */
	widget->priv->search_entry = gtk_entry_new ();

	/* Search button */
	widget->priv->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	gtk_box_pack_start (GTK_BOX (h_box), widget->priv->search_entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (h_box), widget->priv->search_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (v_box), h_box, FALSE, TRUE, 0);

	/* Insert the result list and initialize the viewport */
	scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
	gtk_scrolled_window_set_policy (scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (widget->priv->result_list));
	gtk_container_add (GTK_CONTAINER (v_box), GTK_WIDGET (scroll));

	/* Status label */
	widget->priv->status_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (widget->priv->status_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (v_box), widget->priv->status_label, FALSE, TRUE, 2);

	/* Initialise the pager box */
	pager_box = gtk_hbox_new (FALSE, 6);
	widget->priv->next_button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
	widget->priv->previous_button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
	adjust = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 1, 1, 5, 0));
	widget->priv->page_selector = gtk_spin_button_new (adjust, 1, 0);
	gtk_box_pack_start (GTK_BOX (pager_box), widget->priv->previous_button, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (pager_box), gtk_label_new (_("Page")), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (pager_box), widget->priv->page_selector, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (pager_box), widget->priv->next_button, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (v_box), pager_box, FALSE, TRUE, 0);

	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->previous_button), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->page_selector), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (widget->priv->next_button), FALSE);

	/* Add the main container to the widget */
	gtk_container_add (GTK_CONTAINER (widget), v_box);

	gtk_widget_show_all (GTK_WIDGET (widget));

	/* Connect the search button clicked signal and the search entry  */
	g_signal_connect (widget->priv->search_entry, "changed", 
			  G_CALLBACK (search_entry_changed_cb), widget);
	g_signal_connect (widget->priv->search_button, "clicked",
			  G_CALLBACK (new_search), widget);
	g_signal_connect (widget->priv->search_entry, "activate",
			  G_CALLBACK (new_search), widget);
	/* Connect the pager buttons */
	g_signal_connect (widget->priv->next_button, "clicked",
			  G_CALLBACK (go_next), widget);
	g_signal_connect (widget->priv->previous_button, "clicked",
			  G_CALLBACK (go_previous), widget);
	g_signal_connect (widget->priv->page_selector, "value-changed",
			  G_CALLBACK (page_selector_value_changed_cb), widget);
}

GtkWidget *
totem_tracker_widget_new (TotemObject *totem)
{
	GtkWidget *widget;

	widget = g_object_new (TOTEM_TYPE_TRACKER_WIDGET,
			       "totem", totem, NULL);

	/* Reset the info about the search */
	TOTEM_TRACKER_WIDGET (widget)->priv->total_result_count = 0;
	TOTEM_TRACKER_WIDGET (widget)->priv->current_result_page = 0;

	initialize_list_store (TOTEM_TRACKER_WIDGET (widget));

	return widget;
}

