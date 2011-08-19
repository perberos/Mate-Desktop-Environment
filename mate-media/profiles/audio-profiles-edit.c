/* gm_audio-profiles-edit.c: widget for a profiles edit dialog */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
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
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include "audio-profile.h"
#include "audio-profile-edit.h"
#include "audio-profile-private.h"
#include "gmp-util.h"
#include "audio-profiles-edit.h"

#define MANAGE_STOCK_EDIT "manage-edit"

struct _GMAudioProfilesEditPrivate
{
  MateConfClient *conf;
  GtkWidget *new_button;
  GtkWidget *new_profile_dialog;
  GtkWidget *edit_button;
  GtkWidget *delete_button;
  GtkWindow *transient_parent;
  GtkWidget *manage_profiles_list; /* FIXME: rename this one ? */
  GList *deleted_profiles_list; /* pass through to delete confirm dialog */
};

static void gm_audio_profiles_edit_init		(GMAudioProfilesEdit *edit);
static void gm_audio_profiles_edit_class_init	(GMAudioProfilesEditClass *klass);
static void gm_audio_profiles_edit_finalize	(GObject *object);


/* profile list column names */
enum
{
  COLUMN_NAME,
  COLUMN_PROFILE_OBJECT,
  COLUMN_LAST
};

G_DEFINE_TYPE (GMAudioProfilesEdit, gm_audio_profiles_edit, GTK_TYPE_DIALOG);

/* register custom edit stock icon */
static void
gm_audio_profile_manage_register_stock (void)
{
  static gboolean registered = FALSE;

  if (!registered)
  {
    GtkIconFactory *factory;
    GtkIconSet     *icons;

    static const GtkStockItem edit_item [] = {
      { MANAGE_STOCK_EDIT, N_("_Edit"), 0, 0, GETTEXT_PACKAGE },
    };

    icons = gtk_icon_factory_lookup_default (GTK_STOCK_PREFERENCES);
    factory = gtk_icon_factory_new ();
    gtk_icon_factory_add (factory, MANAGE_STOCK_EDIT, icons);
    gtk_icon_factory_add_default (factory);
    gtk_stock_add (edit_item, 1);
    registered = TRUE;
  }
}

/* widget callbacks */

static void
count_selected_profiles_func (GtkTreeModel      *model,
                              GtkTreePath       *path,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
  int *count = data;

  *count += 1;
}

static void
selection_changed_callback (GtkTreeSelection *selection,
                            GMAudioProfilesEditPrivate *priv)
{
  int count;

  count = 0;
  gtk_tree_selection_selected_foreach (selection,
                                       count_selected_profiles_func,
                                       &count);

  gtk_widget_set_sensitive (priv->edit_button,
                            count == 1);
  gtk_widget_set_sensitive (priv->delete_button,
                            count > 0);
}

static void
profile_activated_callback (GtkTreeView       *tree_view,
                            GtkTreePath       *path,
                            GtkTreeViewColumn *column,
                            gpointer           *ptr)
{
  GMAudioProfile *profile;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *edit_dialog;

  model = gtk_tree_view_get_model (tree_view);

  if (!gtk_tree_model_get_iter (model, &iter, path))
    return;

  profile = NULL;
  gtk_tree_model_get (model,
                      &iter,
                      COLUMN_PROFILE_OBJECT,
                      &profile,
                      -1);
  if (profile) {
    /* FIXME: is this the right function name ? */
    edit_dialog = gm_audio_profile_edit_new ((MateConfClient *) profile, gm_audio_profile_get_id (profile));
    g_return_if_fail (edit_dialog != NULL);
    gtk_widget_show_all (GTK_WIDGET (edit_dialog));
  } else {
    g_warning ("Could not retrieve profile");
  }
}

static void
fix_button_align (GtkWidget *button)
{
  GtkWidget *child;

  child = gtk_bin_get_child (GTK_BIN (button));

  if (GTK_IS_ALIGNMENT (child))
    g_object_set (G_OBJECT (child), "xalign", 0.0, NULL);
  else if (GTK_IS_LABEL (child))
    g_object_set (G_OBJECT (child), "xalign", 0.0, NULL);
}

static void
list_selected_profiles_func (GtkTreeModel      *model,
                             GtkTreePath       *path,
                             GtkTreeIter       *iter,
                             gpointer           data)
{
  GList **list = data;
  GMAudioProfile *profile = NULL;

  gtk_tree_model_get (model,
                      iter,
                      COLUMN_PROFILE_OBJECT,
                      &profile,
                      -1);

  *list = g_list_prepend (*list, profile);
}

static void
free_profiles_list (gpointer data)
{
  GList *profiles = data;

  g_list_foreach (profiles, (GFunc) g_object_unref, NULL);
  g_list_free (profiles);
}

/* refill profile treeview
 * recreates the profile tree view from scratch */
static void
refill_profile_treeview (GtkWidget *tree_view)
{
  GList *profiles;
  GList *tmp;
  GtkTreeSelection *selection;
  GtkListStore *model;
  GList *selected_profiles;
  GtkTreeIter iter;

  GST_DEBUG ("refill_profile_treeview: start\n");
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view)));

  selected_profiles = NULL;
  gtk_tree_selection_selected_foreach (selection,
                                       list_selected_profiles_func,
                                       &selected_profiles);

  gtk_list_store_clear (model);

  profiles = gm_audio_profile_get_list ();
  tmp = profiles;
  while (tmp != NULL)
  {
    GMAudioProfile *profile = tmp->data;

    GST_DEBUG ("refill: appending profile with name %s\n",
             gm_audio_profile_get_name (profile));
    gtk_list_store_append (model, &iter);

    /* We are assuming the list store will hold a reference to
     * the profile object, otherwise we would be in danger of disappearing
     * profiles.
     */
    gtk_list_store_set (model,
                        &iter,
                        COLUMN_NAME, gm_audio_profile_get_name (profile),
                        COLUMN_PROFILE_OBJECT, profile,
                        -1);

    if (g_list_find (selected_profiles, profile) != NULL)
      gtk_tree_selection_select_iter (selection, &iter);

    tmp = tmp->next;
  }

  if (selected_profiles == NULL)
  {
    /* Select first row */
    GtkTreePath *path;

    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, 0);
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)), path);
    gtk_tree_path_free (path);
  }

  free_profiles_list (selected_profiles);
  GST_DEBUG ("refill_profile_treeview: stop\n");
}


/* create a profile list widget */
static GtkWidget*
create_profile_list (void)
{
  GtkTreeSelection *selection;
  GtkCellRenderer *cell;
  GtkWidget *tree_view;
  GtkTreeViewColumn *column;
  GtkListStore *model;

  model = gtk_list_store_new (COLUMN_LAST,
                              G_TYPE_STRING,
                              G_TYPE_OBJECT);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  //terminal_util_set_atk_name_description (tree_view, _("Profile list"), NULL);

  g_object_unref (G_OBJECT (model));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
                               GTK_SELECTION_MULTIPLE);

  refill_profile_treeview (tree_view);

  cell = gtk_cell_renderer_text_new ();

  g_object_set (G_OBJECT (cell),
                "xpad", 2,
                NULL);

  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     cell,
                                                     "text", COLUMN_NAME,
                                                     NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
                               GTK_TREE_VIEW_COLUMN (column));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  return tree_view;
}

/* profile callbacks */
static void
on_profile_changed (GMAudioProfile *profile,
                    const GMAudioSettingMask *mask,
                    GtkWidget *tree_view)
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  GtkTreeModel *tree_model;
  gboolean valid;

  if (mask->name)
  {
    tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
    list_store = GTK_LIST_STORE (tree_model);

    /* Get the first iter in the list */
    valid = gtk_tree_model_get_iter_first (tree_model, &iter);

    while (valid)
    {
      /* Walk through the list, reading each row */
      GMAudioProfile *model_profile;

      /* Make sure you terminate calls to gtk_tree_model_get()
       * with a '-1' value
       */
      gtk_tree_model_get (tree_model, &iter,
                          COLUMN_PROFILE_OBJECT, &model_profile,
                          -1);

      if (profile == model_profile)
      {
        /* bingo ! */
        gtk_list_store_set (list_store,
                            &iter,
                            COLUMN_NAME, gm_audio_profile_get_name (profile),
                            -1);
      }
      valid = gtk_tree_model_iter_next (tree_model, &iter);
    }
  }
}

/* ui callbacks */
static void
new_button_clicked (GtkWidget   *button,
                    GMAudioProfilesEdit *dialog)
{
  gm_audio_profiles_edit_new_profile (dialog, GTK_WINDOW (dialog));
}

static void
edit_button_clicked (GtkWidget   *button,
                     GMAudioProfilesEdit *dialog)
{
  GtkTreeSelection *selection;
  GList *profiles;
  GMAudioProfile *profile;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->manage_profiles_list));

  profiles = NULL;
  gtk_tree_selection_selected_foreach (selection,
                                       list_selected_profiles_func,
                                       &profiles);

  if (profiles == NULL)
    return; /* edit button was supposed to be insensitive... */

  /* only one selection ? */
  if (profiles->next == NULL)
  {
    GtkWidget *edit_dialog;	  
    profile = (GMAudioProfile *) profiles->data;
    /* connect to profile changed signal so we can update the name in the list
     * if it gets changed */
    g_signal_connect_object (G_OBJECT (profile), "changed",
                             G_CALLBACK (on_profile_changed),
                             dialog->priv->manage_profiles_list, 0);

    /* FIXME: is this the right function name ? */
    edit_dialog = gm_audio_profile_edit_new ((MateConfClient *)profile, gm_audio_profile_get_id (profile));
    g_return_if_fail (edit_dialog != NULL);
    gtk_window_set_modal (GTK_WINDOW (edit_dialog), TRUE);
    gtk_widget_show_all (GTK_WIDGET (edit_dialog));
  }
  else
  {
    /* edit button was supposed to be insensitive due to multiple
     * selection
     */
  }

  g_list_foreach (profiles, (GFunc) g_object_unref, NULL);
  g_list_free (profiles);
}

/* ui callback for confirmation from delete_button_clicked */
static void
delete_confirm_response (GtkWidget   *confirm_dialog,
                         int          response_id,
                         GMAudioProfilesEdit *dialog)
{
  GList *deleted_profiles;
  GError *err;

  deleted_profiles = dialog->priv->deleted_profiles_list;

  err = NULL;
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      gm_audio_profile_delete_list (dialog->priv->conf, deleted_profiles,
                                 &err);
    }

  if (err != NULL)
  {
    g_print ("FIXME: GError on deletion: %s\n", err->message);
    g_error_free (err);
  }

  dialog->priv->deleted_profiles_list = NULL;

  /* reget from MateConf and refill tree view */
  gm_audio_profile_sync_list (FALSE, NULL);
  refill_profile_treeview (dialog->priv->manage_profiles_list);

  gtk_widget_destroy (confirm_dialog);
}

static void
delete_button_clicked (GtkWidget   *button,
                       GMAudioProfilesEdit *dialog)
{
  GtkTreeSelection *selection;
  GList *deleted_profiles;
  GtkWidget *confirm_dialog;
  GString *str;
  GList *tmp;
  int count;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->manage_profiles_list));

  deleted_profiles = NULL;
  gtk_tree_selection_selected_foreach (selection,
                                       list_selected_profiles_func,
                                       &deleted_profiles);

  count = g_list_length (deleted_profiles);
  if (count > 1)
  {
    str = g_string_new (NULL);
    /* the first argument will never be used since we only run for count > 1 */
    g_string_printf (str, ngettext ("Delete this profile?\n",
      "Delete these %d profiles?\n", count), count);

    tmp = deleted_profiles;
    while (tmp != NULL)
    {
      g_string_append (str, "    ");
      g_string_append (str,
                       gm_audio_profile_get_name (tmp->data));
      if (tmp->next)
        g_string_append (str, "\n");

      tmp = tmp->next;
    }
  }
  else
  {
    str = g_string_new (NULL);
    g_string_printf (str,
                     _("Delete profile \"%s\"?"),
                     gm_audio_profile_get_name (deleted_profiles->data));
  }

  confirm_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE,
                                   "%s",
                                   str->str);
  g_string_free (str, TRUE);

  gtk_dialog_add_buttons (GTK_DIALOG (confirm_dialog),
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_REJECT,
                          GTK_STOCK_DELETE,
                          GTK_RESPONSE_ACCEPT,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (confirm_dialog),
                                   GTK_RESPONSE_ACCEPT);

  gtk_window_set_title (GTK_WINDOW (confirm_dialog), _("Delete Profile"));
  gtk_window_set_resizable (GTK_WINDOW (confirm_dialog), FALSE);

  /* FIXME: what's this ? */
  dialog->priv->deleted_profiles_list = deleted_profiles;

  g_signal_connect (G_OBJECT (confirm_dialog), "response",
                    G_CALLBACK (delete_confirm_response),
                    dialog);
  g_return_if_fail (confirm_dialog != NULL);
  gtk_widget_show_all (confirm_dialog);
  gtk_dialog_run (GTK_DIALOG (confirm_dialog));
}

/* ui callbacks */
static void
on_gm_audio_profiles_edit_response (GtkWidget *dialog,
                               int        id,
                               void      *data)
{
  if (id == GTK_RESPONSE_HELP)
    {
      GError *err = NULL;

      gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
		    "ghelp:mate-audio-profiles?mate-audio-profiles-profile-edit",
		    gtk_get_current_event_time (),
		    &err);

      if (err)
        {
          gmp_util_show_error_dialog  (GTK_WINDOW (dialog), NULL,
                                       _("There was an error displaying help: %s"), err->message);
          g_error_free (err);
        }
      return;
    }
      
    gtk_widget_destroy (dialog);
}

static void
on_gm_audio_profiles_edit_destroy (GtkWidget *dialog, gpointer *user_data)
{
      GST_DEBUG ("on_destroy: destroying dialog widget\n");
  /* FIXME: set stuff to NULL here */
}

#if 0
/* MateConf notification callback for profile_list */
static void
gm_audio_profiles_list_notify (MateConfClient *client,
                          guint        cnxn_id,
                          MateConfEntry  *entry,
                          gpointer    user_data)
{
  GMAudioProfilesEdit *dialog;

  dialog = (GMAudioProfilesEdit *) user_data;
  GST_DEBUG ("profile_list changed, notified from mateconf, redrawing\n");
  /* refill the profile tree view */

  refill_profile_treeview (dialog->priv->manage_profiles_list);
}
#endif

/* create a dialog widget from scratch */
static void
gm_audio_profiles_edit_init (GMAudioProfilesEdit *dialog)
{
  GtkDialog *gdialog = GTK_DIALOG (dialog);
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *sw;
  GtkWidget *hbox;
  GtkWidget *bbox;
  GtkWidget *button;
  GtkTreeSelection *selection;

  /*
  dialog =
    gtk_dialog_new_with_buttons (_("Edit Profiles"),
                                 NULL,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_STOCK_HELP,
                                 GTK_RESPONSE_HELP,
                                 GTK_STOCK_CLOSE,
                                 GTK_RESPONSE_ACCEPT,
                                 NULL);
  */
                                 // FIXME: GTK_DIALOG_DESTROY_WITH_PARENT,
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GM_AUDIO_TYPE_PROFILES_EDIT, GMAudioProfilesEditPrivate);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Edit MATE Audio Profiles"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 320, 240);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                 GTK_STOCK_HELP,
                                 GTK_RESPONSE_HELP,
                                 GTK_STOCK_CLOSE,
                                 GTK_RESPONSE_ACCEPT,
                                 NULL);

#if !GTK_CHECK_VERSION (2, 21, 8)
  gtk_dialog_set_has_separator (gdialog, FALSE);
#endif
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (gdialog)), 2); /* 2 * 5 + 2 = 12 */
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_action_area (gdialog)), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_action_area (gdialog)), 6);

  g_signal_connect (GTK_DIALOG (dialog),
                    "response",
                    G_CALLBACK (on_gm_audio_profiles_edit_response),
                    NULL);

  g_signal_connect (G_OBJECT (dialog),
                    "destroy",
                    G_CALLBACK (on_gm_audio_profiles_edit_destroy),
                    NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (gdialog)),
                      vbox, TRUE, TRUE, 0);

  /* FIXME
  hbox = gtk_hbox_new (FALSE, PADDING);
  gtk_box_pack_end (GTK_BOX (vbox),
                    hbox, FALSE, FALSE, 0);

  app->manage_profiles_default_menu = profile_optionmenu_new ();
  g_signal_connect (G_OBJECT (app->manage_profiles_default_menu),
                    "changed", G_CALLBACK (default_menu_changed),
                    app);

  gtk_box_pack_start (GTK_BOX (hbox),
                      label, TRUE, TRUE, 0);

  gtk_box_pack_end (GTK_BOX (hbox),
                    app->manage_profiles_default_menu, FALSE, FALSE, 0);
  */

  hbox = gtk_hbox_new (FALSE, 6);

  label = gtk_label_new_with_mnemonic (_("_Profiles:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox, TRUE, TRUE, 0);

  dialog->priv->manage_profiles_list = create_profile_list ();

  g_signal_connect (G_OBJECT (dialog->priv->manage_profiles_list),
                    "row_activated",
                    G_CALLBACK (profile_activated_callback),
                    NULL);

  sw = gtk_scrolled_window_new (NULL, NULL);
  /* FIXME
  terminal_util_set_labelled_by (GTK_WIDGET (dialog->priv->manage_profiles_list),
                                 GTK_LABEL (label));
  */

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);

  gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (sw), dialog->priv->manage_profiles_list);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 dialog->priv->manage_profiles_list);

  bbox = gtk_vbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  fix_button_align (button);
  gtk_box_pack_start (GTK_BOX (bbox),
                      button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (new_button_clicked), dialog);
  dialog->priv->new_button = button;
/*
  terminal_util_set_atk_name_description (dialog->priv->new_button, NULL,
                                          _("Click to open new profile dialog"));
*/

  gm_audio_profile_manage_register_stock ();

  button = gtk_button_new_from_stock (MANAGE_STOCK_EDIT);
  fix_button_align (button);
  gtk_box_pack_start (GTK_BOX (bbox),
                     button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_button_clicked), dialog);
  gtk_widget_set_sensitive (button, FALSE);
  dialog->priv->edit_button = button;
/*
  terminal_util_set_atk_name_description (app->manage_profiles_edit_button, NULL,
                                          _("Click to open edit profile dialog"));
*/

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  fix_button_align (button);
  gtk_box_pack_start (GTK_BOX (bbox),
                      button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (delete_button_clicked), dialog);
  gtk_widget_set_sensitive (button, FALSE);
  dialog->priv->delete_button = button;
/*
  terminal_util_set_atk_name_description (app->manage_profiles_delete_button, NULL,
                                          _("Click to delete selected profile"));
  */

  gtk_widget_grab_focus (dialog->priv->manage_profiles_list);

  /* Monitor selection for sensitivity */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->manage_profiles_list));

  selection_changed_callback (selection, dialog->priv);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (selection_changed_callback),
                    dialog->priv);
}

static void
gm_audio_profiles_edit_class_init (GMAudioProfilesEditClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gm_audio_profiles_edit_finalize;

  g_type_class_add_private (object_class, sizeof (GMAudioProfilesEditPrivate));
}

static void
gm_audio_profiles_edit_finalize (GObject *object)
{
  GMAudioProfilesEdit *dialog;

  dialog = GM_AUDIO_PROFILES_EDIT (object);
  G_OBJECT_CLASS (gm_audio_profiles_edit_parent_class)->finalize (object);
}

GtkWidget*
gm_audio_profiles_edit_new (MateConfClient *conf, GtkWindow *transient_parent)
{
  GMAudioProfilesEdit *dialog;
  /*GError *err;*/

  dialog = g_object_new (GM_AUDIO_TYPE_PROFILES_EDIT, NULL);

  g_object_ref (G_OBJECT (conf));
  dialog->priv->conf = conf;
  /* set transient parent to itself if NULL */
  if (transient_parent)
    dialog->priv->transient_parent = transient_parent;
  else
    dialog->priv->transient_parent = GTK_WINDOW (dialog);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  /* subscribe to changes to profile list */
/*
  err = NULL;
  GST_DEBUG ("gap_p_e_new: subscribing to profile_list changes\n");
  mateconf_client_notify_add (dialog->priv->conf,
                           CONF_GLOBAL_PREFIX"/profile_list",
                           gm_audio_profiles_list_notify,
                           dialog,
                           NULL, &err);

  if (err)
    {
      g_printerr (_("There was an error subscribing to notification of terminal profile list changes. (%s)\n"),
                  err->message);
      g_error_free (err);
    }
*/
  return GTK_WIDGET (dialog);
}

/*
 * creating a new profile from the Edit Profiles dialog
 */

static void
new_profile_response_callback (GtkWidget *new_profile_dialog,
                               int        response_id,
                               GMAudioProfilesEdit *dialog)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
  {
    GtkWidget *name_entry;
    char *name;
    char *id;
    /*GtkWidget *base_option_menu;*/
    GMAudioProfile *new_profile;
    GList *profiles;
    GList *tmp;
    GtkWindow *transient_parent;
    GError *error;

    name_entry = g_object_get_data (G_OBJECT (new_profile_dialog), "name_entry");
    name = gtk_editable_get_chars (GTK_EDITABLE (name_entry), 0, -1);
    g_strstrip (name); /* name will be non empty after stripping */

    profiles = gm_audio_profile_get_list ();
    for (tmp = profiles; tmp != NULL; tmp = tmp->next)
    {
      GMAudioProfile *profile = tmp->data;

      if (strcmp (name, gm_audio_profile_get_name (profile)) == 0)
        break;
    }
    if (tmp)
    {
      gmp_util_run_error_dialog (GTK_WINDOW (new_profile_dialog),
                                   _("You already have a profile called \"%s\""), name);
      goto cleanup;
    }
    g_list_free (profiles);

/*
      base_option_menu = g_object_get_data (G_OBJECT (new_profile_dialog), "base_option_menu");
      base_profile = profile_optionmenu_get_selected (base_option_menu);

      if (base_profile == NULL)
        {
          terminal_util_show_error_dialog (GTK_WINDOW (new_profile_dialog), NULL,
                                          _("The profile you selected as a base for your new profile no longer exists"));
          goto cleanup;
        }
*/
    transient_parent = gtk_window_get_transient_for (GTK_WINDOW (new_profile_dialog));


    error = NULL;
    id = gm_audio_profile_create (name, dialog->priv->conf, &error);
    if (error)
    {
      g_print ("ERROR: %s\n", error->message);
      gmp_util_run_error_dialog (GTK_WINDOW (transient_parent),
                                  _("MateConf Error (FIXME): %s\n"),
                                  error->message);
      g_error_free (error);
      goto cleanup;
    }
    gtk_widget_destroy (new_profile_dialog);

    /* FIXME: hm, not very proud of having to unstatic this function */
    GST_DEBUG ("new profile callback: syncing list\n");
    //FIXME: in mate-terminal, it's TRUE, &n, which then gets overruled
    // by some other sync call with the new list
    //audio_profile_sync_list (TRUE, &n);
    gm_audio_profile_sync_list (FALSE, NULL);
    refill_profile_treeview (dialog->priv->manage_profiles_list);

    new_profile = gm_audio_profile_lookup (id);
    g_assert (new_profile != NULL);

    //audio_profile_edit (new_profile, transient_parent);

  cleanup:
    g_free (name);
  }
  else
  {
    gtk_widget_destroy (new_profile_dialog);
  }
  GST_DEBUG ("done creating new profile\n");
}

static void
new_profile_name_entry_changed_callback (GtkEditable *editable, gpointer data)
{
  char *name, *saved_name;
  GtkWidget *create_button;

  create_button = (GtkWidget*) data;

  saved_name = name = gtk_editable_get_chars (editable, 0, -1);

  /* make the create button sensitive only if something other than space has been set */
  while (*name != '\0' && g_ascii_isspace (*name))
    name++;

  gtk_widget_set_sensitive (create_button, *name != '\0' ? TRUE : FALSE);

  g_free (saved_name);
}

void
gm_audio_profiles_edit_new_profile (GMAudioProfilesEdit *dialog,
                               GtkWindow *transient_parent)
{
  GtkWindow *old_transient_parent;
  GtkWidget *create_button;
  gint response;
  GError *error = NULL;

  if (dialog->priv->new_profile_dialog == NULL)
  {
    GtkBuilder *builder;
    GtkWidget *w, *wl;
    GtkWidget *create_button;
    GtkSizeGroup *size_group, *size_group_labels;

    builder = gmp_util_load_builder_file ("mate-audio-profile-new.ui", transient_parent, &error);

    if (error != NULL) {
      g_warning (error->message);
      g_error_free (error);
      return;
    }

    dialog->priv->new_profile_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-dialog"));
    g_signal_connect (G_OBJECT (dialog->priv->new_profile_dialog), "response", G_CALLBACK (new_profile_response_callback), dialog);

    g_object_add_weak_pointer (G_OBJECT (dialog->priv->new_profile_dialog), (void**) &dialog->priv->new_profile_dialog);

    create_button = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-create-button"));
    g_object_set_data (G_OBJECT (dialog->priv->new_profile_dialog), "create_button", create_button);
    gtk_widget_set_sensitive (create_button, FALSE);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* the name entry */
    w = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-name-entry"));
    g_object_set_data (G_OBJECT (dialog->priv->new_profile_dialog), "name_entry", w);
    g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (new_profile_name_entry_changed_callback), create_button);
    gtk_entry_set_activates_default (GTK_ENTRY (w), TRUE);
    gtk_widget_grab_focus (w);
    // FIXME terminal_util_set_atk_name_description (w, _("Enter profile name"), NULL);
    gtk_size_group_add_widget (size_group, w);

    wl = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-name-label"));
    gtk_label_set_mnemonic_widget (GTK_LABEL (wl), w);
    // FIXME terminal_util_set_labelled_by (w, GTK_LABEL (wl));
    gtk_size_group_add_widget (size_group_labels, wl);

#ifdef BASE
    /* the base profile option menu */
    w = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-base-option-menu"));
    g_object_set_data (G_OBJECT (dialog->priv->new_profile_dialog), "base_option_menu", w);
    // FIXME terminal_util_set_atk_name_description (w, _("Choose base profile"), NULL);
    //FIXME profile_optionmenu_refill (w);
    gtk_size_group_add_widget (size_group, w);

    wl = GTK_WIDGET (gtk_builder_get_object (builder, "new-profile-base-label"));
    gtk_label_set_mnemonic_widget (GTK_LABEL (wl), w);
    // FIXME terminal_util_set_labelled_by (w, GTK_LABEL (wl));
    gtk_size_group_add_widget (size_group_labels, wl);
#endif


    /* gtk_dialog_set_default_response (GTK_DIALOG (dialog->priv->new_profile_dialog), GTK_RESPONSE_CREATE); */

    g_object_unref (G_OBJECT (size_group));
    g_object_unref (G_OBJECT (size_group_labels));

    g_object_unref (G_OBJECT (builder));
  }

  old_transient_parent = gtk_window_get_transient_for (GTK_WINDOW (dialog->priv->new_profile_dialog));
  if (old_transient_parent != transient_parent)
  {
    gtk_window_set_transient_for (GTK_WINDOW (dialog->priv->new_profile_dialog),
                                  transient_parent);
    gtk_widget_hide (dialog->priv->new_profile_dialog); /* re-show the window on its new parent */
  }

  create_button = g_object_get_data (G_OBJECT (dialog->priv->new_profile_dialog), "create_button");
  gtk_widget_set_sensitive (create_button, FALSE);

  gtk_window_set_modal (GTK_WINDOW (dialog->priv->new_profile_dialog), TRUE);
  gtk_widget_show_all (dialog->priv->new_profile_dialog);
  gtk_window_present (GTK_WINDOW (dialog->priv->new_profile_dialog));
  
  //keep running the dialog until the response is GTK_RESPONSE_NONE
  do {
    response = gtk_dialog_run (GTK_DIALOG (dialog->priv->new_profile_dialog));
  } while (response != GTK_RESPONSE_NONE);
}
