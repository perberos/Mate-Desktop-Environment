/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * preferences.c: preferences screen
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
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <glib.h>

#include <gtk/gtk.h>

#include <gst/interfaces/mixer.h>

#include "applet.h"
#include "preferences.h"
#include "keys.h"

enum {
  COL_LABEL,
  COL_TRACK,
  NUM_COLS
};

static void	mate_volume_applet_preferences_class_init	(MateVolumeAppletPreferencesClass *klass);
static void	mate_volume_applet_preferences_init	(MateVolumeAppletPreferences *prefs);
static void	mate_volume_applet_preferences_dispose (GObject *object);
static void	mate_volume_applet_preferences_response (GtkDialog *dialog,
							  gint       response_id);

static void	cb_dev_selected				(GtkComboBox *box,
							 gpointer    data);

static gboolean cb_track_select				(GtkTreeSelection *selection,
							 GtkTreeModel     *model,
							 GtkTreePath      *path,
							 gboolean          path_selected,
							 gpointer          data);

static GtkDialogClass *parent_class = NULL;

G_DEFINE_TYPE (MateVolumeAppletPreferences, mate_volume_applet_preferences, GTK_TYPE_DIALOG)

static void
mate_volume_applet_preferences_class_init (MateVolumeAppletPreferencesClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *gtkdialog_class = (GtkDialogClass *) klass;

  parent_class = g_type_class_ref (GTK_TYPE_DIALOG);

  gobject_class->dispose = mate_volume_applet_preferences_dispose;
  gtkdialog_class->response = mate_volume_applet_preferences_response;
}

static void
mate_volume_applet_preferences_init (MateVolumeAppletPreferences *prefs)
{
  GtkWidget *box, *label, *view;
  GtkListStore *store;
  GtkTreeSelection *sel;
  GtkTreeViewColumn *col;
  GtkCellRenderer *render;
  GList *cells;

  prefs->applet = NULL;
  prefs->mixer = NULL;

  /* make window look cute */
  gtk_window_set_title (GTK_WINDOW (prefs), _("Volume Control Preferences"));
  gtk_dialog_set_has_separator (GTK_DIALOG (prefs), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (prefs), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(prefs))), 2);
  gtk_dialog_add_buttons (GTK_DIALOG (prefs),
			  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			  /* help goes here (future) */
			  NULL);

  /* add a treeview for all the properties */
  box = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  label = gtk_label_new (_("Select the device and track to control."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* optionmenu */
  prefs->optionmenu = gtk_combo_box_new_text ();
  cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (prefs->optionmenu));
  g_object_set (G_OBJECT (cells->data), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_list_free (cells);

  gtk_box_pack_start (GTK_BOX (box), prefs->optionmenu, FALSE, FALSE, 0);
  gtk_widget_show (prefs->optionmenu);
  g_signal_connect (prefs->optionmenu, "changed",
		    G_CALLBACK (cb_dev_selected), prefs);

  store = gtk_list_store_new (NUM_COLS,
			      G_TYPE_STRING, G_TYPE_POINTER);
  prefs->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prefs->treeview), FALSE);

  /* viewport for lots of tracks */
  view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view),
				       GTK_SHADOW_IN);
  gtk_widget_set_size_request (view, -1, 100);

  gtk_container_add (GTK_CONTAINER (view), prefs->treeview);
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);

  gtk_widget_show (prefs->treeview);
  gtk_widget_show (view);

  /* treeview internals */
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
  gtk_tree_selection_set_select_function (sel, cb_track_select, prefs, NULL);

  render = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes ("Track name", render,
						  "text", COL_LABEL,
						  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prefs->treeview), col);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (prefs->treeview), COL_LABEL);

  /* and show */
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (prefs))), box,
		      TRUE, TRUE, 0);
  gtk_widget_show (box);
}

GtkWidget *
mate_volume_applet_preferences_new (MatePanelApplet *applet,
				     GList       *elements,
				     GstMixer    *mixer,
				     GList       *tracks)
{
  MateVolumeAppletPreferences *prefs;

  /* element */
  prefs = g_object_new (MATE_VOLUME_APPLET_TYPE_PREFERENCES, NULL);
  prefs->applet = g_object_ref (G_OBJECT (applet));

  /* show devices */
  for ( ; elements != NULL; elements = elements->next) {
    gchar *name = g_object_get_data (G_OBJECT (elements->data),
				     "mate-volume-applet-name");
    gtk_combo_box_append_text (GTK_COMBO_BOX (prefs->optionmenu), name);
  }

  mate_volume_applet_preferences_change (prefs, mixer, tracks);
  return GTK_WIDGET (prefs);
}

static void
mate_volume_applet_preferences_dispose (GObject *object)
{
  MateVolumeAppletPreferences *prefs = MATE_VOLUME_APPLET_PREFERENCES (object);

  if (prefs->applet) {
    g_object_unref (G_OBJECT (prefs->applet));
    prefs->applet = NULL;
  }

  if (prefs->mixer) {
    gst_object_unref (GST_OBJECT (prefs->mixer));
    prefs->mixer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
mate_volume_applet_preferences_response (GtkDialog *dialog,
					  gint       response_id)
{
  switch (response_id) {
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    default:
      break;
  }

  if (((GtkDialogClass *) parent_class)->response)
    ((GtkDialogClass *) parent_class)->response (dialog, response_id);
}

/*
 * Change the element. Basically recreates this object internally.
 */

void
mate_volume_applet_preferences_change (MateVolumeAppletPreferences *prefs,
					GstMixer *mixer,
					GList *tracks)
{
  GtkTreeIter iter;
  GtkTreeSelection *sel;
  GtkListStore *store;
  GtkTreeModel *model;
  const GList *item;
  gchar *label;
  gboolean change = (mixer != prefs->mixer), res;
  GList *tree_iter;

  /* because the old list of tracks is cleaned out when the application removes
   * all the tracks, we need to keep a backup of the list before clearing it */
  GList *old_selected_tracks = g_list_copy (tracks);

  prefs->track_lock = TRUE;
  if (change) {
    /* remove old */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->treeview));
    store = GTK_LIST_STORE (model);

    while (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
      gtk_list_store_remove (store, &iter);
    }

    /* take/put reference */
    gst_object_replace ((GstObject **) &prefs->mixer, GST_OBJECT (mixer));

    /* select active element */
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (prefs->optionmenu));
    for (res = gtk_tree_model_get_iter_first (model, &iter); res; res = gtk_tree_model_iter_next (model, &iter)) {
      gtk_tree_model_get (model, &iter, COL_LABEL, &label, -1);
      if (!strcmp (label, g_object_get_data (G_OBJECT (mixer), "mate-volume-applet-name"))) {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (prefs->optionmenu), &iter);
      }
      
      g_free (label);
    }

    /* now over to the tracks */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->treeview));
    store = GTK_LIST_STORE (model);
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));

    /* add all tracks */
    for (item = gst_mixer_list_tracks (mixer); item; item = item->next) {
      GstMixerTrack *track = item->data;

      if (track->num_channels <= 0)
        continue;
      
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  COL_LABEL, track->label,
			  COL_TRACK, track,
			  -1);
      
      /* select active tracks */
      for (tree_iter = g_list_first (old_selected_tracks); tree_iter; tree_iter = tree_iter->next) {
	GstMixerTrack *test_against = tree_iter->data;
        if (!strcmp (test_against->label, track->label))
	  gtk_tree_selection_select_iter (sel, &iter);
      }
    }
  } else {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->treeview));
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));
    gtk_tree_selection_unselect_all (sel);

    for (res = gtk_tree_model_get_iter_first (model, &iter); res == TRUE; res = gtk_tree_model_iter_next (model, &iter)) {
      gtk_tree_model_get (model, &iter, COL_LABEL, &label, -1);
 
      /* select active tracks */
      for (tree_iter = g_list_first (old_selected_tracks); tree_iter; tree_iter = tree_iter->next) {
	GstMixerTrack *track = tree_iter->data;
	if (!strcmp (track->label, label))
	  gtk_tree_selection_select_iter (sel, &iter);
      }

      g_free (label);
    }
  }
  prefs->track_lock = FALSE;
  g_list_free (old_selected_tracks);
}

/*
 * Select callback (menu/tree).
 */

static void
cb_dev_selected (GtkComboBox *box,
		 gpointer    data)
{
  MateVolumeAppletPreferences *prefs = data;
  /* MateVolumeApplet *applet = (MateVolumeApplet *) prefs->applet; */
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (box, &iter)) {
    gchar *label;
    MateConfValue *value;

    gtk_tree_model_get (gtk_combo_box_get_model (box),
			&iter, COL_LABEL, &label, -1);

    /* write to mateconf */
    value = mateconf_value_new (MATECONF_VALUE_STRING);
    mateconf_value_set_string (value, label);
    mate_panel_applet_mateconf_set_value (MATE_PANEL_APPLET (prefs->applet),
		      MATE_VOLUME_APPLET_KEY_ACTIVE_ELEMENT,
		      value, NULL);
    g_free (label);
    mateconf_value_free (value);
  }
}

/* get the percent volume from a track */

static int 
mate_volume_applet_get_volume (GstMixer *mixer, GstMixerTrack *track)
{
  int *volumes, main_volume, range;

  if (track->num_channels == 0)
    return 0;

  volumes = g_new (gint, track->num_channels);
  gst_mixer_get_volume (mixer, track, volumes);
  main_volume = volumes[0];
  g_free (volumes);

  range = track->max_volume - track->min_volume;
  if (range == 0)
    return 0;

  return 100 * (main_volume - track->min_volume) / range;
}

static gboolean
cb_track_select (GtkTreeSelection *selection,
		 GtkTreeModel     *model,
		 GtkTreePath      *path,
		 gboolean          path_selected,
		 gpointer          data)
{
  MateVolumeAppletPreferences *prefs = data;
  GtkTreeIter iter;
  gchar *label;
  MateConfValue *value;
  GtkTreeSelection *sel;
  GString *mateconf_string;
  GstMixerTrack *selected_track; /* the track just selected */
  MateVolumeApplet *applet = (MateVolumeApplet*) prefs->applet; /* required to update the track settings */
  int volume_percent;

  if (prefs->track_lock)
    return TRUE;

  mateconf_string = g_string_new ("");

  /* get value */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COL_LABEL, &label, COL_TRACK, &selected_track, -1);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));

  /* clear the list of selected tracks */
  if (applet->tracks) {
    g_list_free (applet->tracks);
    applet->tracks = NULL;
  }

  if (gtk_tree_selection_count_selected_rows (sel) > 0) {
    GList *lst;

    /* add the ones already selected */
    for (lst = gtk_tree_selection_get_selected_rows (sel, &model); lst != NULL; lst = lst->next) {
      GstMixerTrack *curr = NULL;
      gchar *it_label;

      gtk_tree_model_get_iter (model, &iter, lst->data);
      gtk_tree_model_get (model, &iter, COL_LABEL, &it_label, COL_TRACK, &curr, -1);

      /* grab the main volume (they will all be the same, so it doesn't matter 
       * which one we get) */
      volume_percent = mate_volume_applet_get_volume (prefs->mixer, curr);

      if (strcmp (it_label, label)) {
	applet->tracks = g_list_append (applet->tracks, curr);

	if (!path_selected) {
	  g_string_append_printf (mateconf_string, "%s:", curr->label);
	} else {
	  mateconf_string = g_string_append (mateconf_string, curr->label);
	}
      }
    }
    g_list_foreach (lst, (GFunc)gtk_tree_path_free, NULL);
    g_list_free (lst);
  }

  /* add the one just selected and adjust its volume if it's not the only one
   * selected */
  if (!path_selected) {
    GstMixerTrack *curr;

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COL_TRACK, &curr, -1);
    mateconf_string = g_string_append (mateconf_string, curr->label);

    applet->tracks = g_list_append (applet->tracks, curr);

    /* unify the volume of this track with the others already added */
    if (g_list_length (applet->tracks) > 1) {
      mate_volume_applet_adjust_volume (prefs->mixer, curr, volume_percent);
    }
  }

  /* write to mateconf */
  value = mateconf_value_new (MATECONF_VALUE_STRING);
  mateconf_value_set_string (value, mateconf_string->str);
  mate_panel_applet_mateconf_set_value (MATE_PANEL_APPLET (prefs->applet),
				MATE_VOLUME_APPLET_KEY_ACTIVE_TRACK,
				value, NULL);
  g_free (label);
  g_string_free (mateconf_string, TRUE);
  mateconf_value_free (value);
  
  return TRUE;
}
