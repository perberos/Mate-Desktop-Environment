/* gm_audio-profile-edit.c: dialog to edit a specific profile */

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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include "gmp-util.h"
#include "audio-profile-edit.h"
#include "audio-profile.h"
#include "audio-profile-private.h"

struct _GMAudioProfileEditPrivate
{
  MateConfClient *conf;
  GtkBuilder *builder;
  GMAudioProfile *profile;
  GtkWidget *content;
};

static void	gm_audio_profile_edit_init		(GMAudioProfileEdit *edit);
static void	gm_audio_profile_edit_class_init	(GMAudioProfileEditClass *klass);
static void	gm_audio_profile_edit_dispose		(GObject *object);

static void     gm_audio_profile_edit_response          (GtkDialog *dialog,
			                                 int        id);
static GtkWidget*
		gm_audio_profile_edit_get_widget	(GMAudioProfileEdit *dialog,
							 const char *widget_name);
static void	gm_audio_profile_edit_update_name	(GMAudioProfileEdit *dialog,
							 GMAudioProfile *profile);
static void	gm_audio_profile_edit_update_description
							(GMAudioProfileEdit *dialog,
							 GMAudioProfile *profile);
static void	gm_audio_profile_edit_update_pipeline	(GMAudioProfileEdit *dialog,
							 GMAudioProfile *profile);
static void	gm_audio_profile_edit_update_extension	(GMAudioProfileEdit *dialog,
							 GMAudioProfile *profile);
static void	gm_audio_profile_edit_update_active	(GMAudioProfileEdit *dialog,
							 GMAudioProfile *profile);
static void	on_profile_changed			(GMAudioProfile *profile,
							 const GMAudioSettingMask *mask,
							 GMAudioProfileEdit *dialog);

G_DEFINE_TYPE (GMAudioProfileEdit, gm_audio_profile_edit, GTK_TYPE_DIALOG)

/* ui callbacks */

/* initialize a dialog widget from the ui builder file */
static void
gm_audio_profile_edit_init (GMAudioProfileEdit *dialog)
{
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GM_AUDIO_TYPE_PROFILE_EDIT, GMAudioProfileEditPrivate);
}

static void
gm_audio_profile_edit_class_init (GMAudioProfileEditClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->dispose = gm_audio_profile_edit_dispose;

  dialog_class->response = gm_audio_profile_edit_response;

  g_type_class_add_private (object_class, sizeof (GMAudioProfileEditPrivate));
}

#if 0
static void
gm_audio_profile_edit_finalize (GObject *object)
{
  GMAudioProfileEdit *dialog;

  dialog = GM_AUDIO_PROFILE_EDIT (object);

  G_OBJECT_CLASS (gm_audio_profile_edit_parent_class)->finalize (object);
}
#endif

/* ui callbacks */
static void
gm_audio_profile_edit_response (GtkDialog *dialog,
                                int        id)
{
  if (id == GTK_RESPONSE_HELP)
    {
      GError *err = NULL;

      gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
		    "ghelp:mate-audio-profiles?mate-audio-profiles-edit",
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

  /* FIXME: hide or destroy ? */
  gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
gm_audio_profile_edit_dispose (GObject *object)
{
  GMAudioProfileEdit *dialog = GM_AUDIO_PROFILE_EDIT (object);
  GMAudioProfileEditPrivate *priv = dialog->priv;

  g_signal_handlers_disconnect_by_func (priv->profile,
                                        G_CALLBACK (on_profile_changed),
                                        dialog);
}
/* profile callbacks */

static void
on_profile_changed (GMAudioProfile           *profile,
                    const GMAudioSettingMask *mask,
                    GMAudioProfileEdit         *dialog)
{
  if (mask->name)
    gm_audio_profile_edit_update_name (dialog, profile);
  if (mask->description)
    gm_audio_profile_edit_update_description (dialog, profile);
  if (mask->pipeline)
    gm_audio_profile_edit_update_pipeline (dialog, profile);
  if (mask->extension)
    gm_audio_profile_edit_update_extension (dialog, profile);
  if (mask->active)
    gm_audio_profile_edit_update_active (dialog, profile);
}

/* ui callbacks */
static void
on_profile_name_changed (GtkWidget       *entry,
                               GMAudioProfile *profile)
{
  char *text;

  text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gm_audio_profile_set_name (profile, text);

  g_free (text);
}

static void
on_profile_description_changed (GtkTextBuffer  *tb,
                                GMAudioProfile *profile)
{
  char *text;

  g_object_get (G_OBJECT (tb), "text", &text, NULL);
  gm_audio_profile_set_description (profile, text);
  g_free (text);
}

static void
on_profile_pipeline_changed (GtkWidget       *entry,
                             GMAudioProfile *profile)
{
  char *text;

  text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gm_audio_profile_set_pipeline (profile, text);

  g_free (text);
}

static void
on_profile_extension_changed (GtkWidget       *entry,
                                GMAudioProfile *profile)
{
  char *text;

  text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gm_audio_profile_set_extension (profile, text);

  g_free (text);
}

static void
on_profile_active_toggled (GtkWidget *button, GMAudioProfile *profile)
{
  gm_audio_profile_set_active (profile, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}

/* create and return a new Profile Edit Dialog
 * given the MateConf connection and the id of the profile
 */
GtkWidget*
gm_audio_profile_edit_new (MateConfClient *conf, const char *id)
{
  GMAudioProfileEdit *dialog;
  GtkBuilder *builder;
  GtkWidget *w;
  GtkTextBuffer *tb;
  GError *error = NULL;

  /* get the dialog */
  builder = gmp_util_load_builder_file ("mate-audio-profile-edit.ui", NULL, &error);
  if (error != NULL) {
    g_warning (error->message);
    g_error_free (error);
    return NULL;
  }

  dialog = GM_AUDIO_PROFILE_EDIT (gtk_builder_get_object (builder, "profile-edit-dialog"));
  g_return_val_if_fail (dialog != NULL, NULL);

  /* make sure we have priv */
  if (dialog->priv == NULL)
  {
    /* we didn't go through _init; this chould happen for example when
     * we specified profile-edit-dialog as a GtkDialog instead of a
     * GMAudioProfileEdit for easy glade editing */
    /* FIXME: to be honest, this smells like I'm casting a created GtkDialog
     * widget to a GapEditProfile and then doing stuff to it.  That doesn't
     * smell to good to me */
    dialog->priv = g_new0 (GMAudioProfileEditPrivate, 1);
  }
  dialog->priv->builder = builder;

  /* save the MateConf stuff and get the profile belonging to this id */
  dialog->priv->conf = g_object_ref (conf);

  dialog->priv->profile = gm_audio_profile_lookup (id);
  g_assert (dialog->priv->profile);

  /* autoconnect doesn't handle data pointers, sadly, so do by hand */
  w = GTK_WIDGET (gtk_builder_get_object (builder, "profile-name-entry"));
  gm_audio_profile_edit_update_name (dialog, dialog->priv->profile);
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (on_profile_name_changed), dialog->priv->profile);
  w = GTK_WIDGET (gtk_builder_get_object (builder, "profile-description-textview"));
  gm_audio_profile_edit_update_description (dialog, dialog->priv->profile);
  tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
  g_signal_connect (G_OBJECT (tb), "changed",
                    G_CALLBACK (on_profile_description_changed), dialog->priv->profile);
  w = GTK_WIDGET (gtk_builder_get_object (builder, "profile-pipeline-entry"));
  gm_audio_profile_edit_update_pipeline (dialog, dialog->priv->profile);
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (on_profile_pipeline_changed), dialog->priv->profile);
  w = GTK_WIDGET (gtk_builder_get_object (builder, "profile-extension-entry"));
  gm_audio_profile_edit_update_extension (dialog, dialog->priv->profile);
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (on_profile_extension_changed), dialog->priv->profile);
  w = GTK_WIDGET (gtk_builder_get_object (builder, "profile-active-button"));
  gm_audio_profile_edit_update_active (dialog, dialog->priv->profile);
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (on_profile_active_toggled), dialog->priv->profile);


  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  /* connect to profile changes */

  g_signal_connect (G_OBJECT (dialog->priv->profile),
                    "changed",
                    G_CALLBACK (on_profile_changed),
                    dialog);

  gtk_window_present (GTK_WINDOW (dialog));

  return GTK_WIDGET (dialog);
}

/* UI consistency update functions */
static void
entry_set_text_if_changed (GtkEntry   *entry,
                           const char *text)
{
  char *s;

  GST_DEBUG ("entry_set_text_if_changed on entry %p with text %s\n", entry, text);
  s = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  GST_DEBUG ("got editable text %s\n", s);
  if (text && strcmp (s, text) != 0)
    gtk_entry_set_text (GTK_ENTRY (entry), text);
  GST_DEBUG ("entry_set_text_if_changed: got %s\n", s);

  g_free (s);
}

static void
textview_set_text_if_changed (GtkTextView *view, const char *text)
{
  char *s;
  GtkTextBuffer *tb;

  GST_DEBUG ("textview_set_text_if_changed on textview %p with text %s\n",
	    view, text);
  tb = gtk_text_view_get_buffer (view);
  g_object_get (G_OBJECT (tb), "text", &s, NULL);
  GST_DEBUG ("got textview text %s\n", s);
  if (s && strcmp (s, text) != 0)
    g_object_set (G_OBJECT (tb), "text", text, NULL);
  GST_DEBUG ("textview_set_text_if_changed: got %s\n", s);

  g_free (s);
}

static void
gm_audio_profile_edit_update_name (GMAudioProfileEdit *dialog,
                              GMAudioProfile *profile)
{
  char *s;
  GtkWidget *w;

  s = g_strdup_printf (_("Editing profile \"%s\""),
                       gm_audio_profile_get_name (profile));
  GST_DEBUG ("g_p_e_u_n: title %s\n", s);

  gtk_window_set_title (GTK_WINDOW (dialog), s);

  g_free (s);

  w = gm_audio_profile_edit_get_widget (dialog, "profile-name-entry");
  g_assert (GTK_IS_WIDGET (w));

  entry_set_text_if_changed (GTK_ENTRY (w),
                             gm_audio_profile_get_name (profile));
}

static void
gm_audio_profile_edit_update_description (GMAudioProfileEdit *dialog,
                              GMAudioProfile *profile)
{
  GtkWidget *w;

  w = gm_audio_profile_edit_get_widget (dialog, "profile-description-textview");
  g_assert (GTK_IS_WIDGET (w));

  textview_set_text_if_changed (GTK_TEXT_VIEW (w),
                             gm_audio_profile_get_description (profile));
}

static void
gm_audio_profile_edit_update_pipeline (GMAudioProfileEdit *dialog,
                              GMAudioProfile *profile)
{
  GtkWidget *w;

  w = gm_audio_profile_edit_get_widget (dialog, "profile-pipeline-entry");
  g_assert (GTK_IS_WIDGET (w));

  entry_set_text_if_changed (GTK_ENTRY (w),
                             gm_audio_profile_get_pipeline (profile));
}

static void
gm_audio_profile_edit_update_extension (GMAudioProfileEdit *dialog,
                              GMAudioProfile *profile)
{
  GtkWidget *w;

  w = gm_audio_profile_edit_get_widget (dialog, "profile-extension-entry");
  g_assert (GTK_IS_WIDGET (w));

  entry_set_text_if_changed (GTK_ENTRY (w),
                             gm_audio_profile_get_extension (profile));
}

static void
gm_audio_profile_edit_update_active (GMAudioProfileEdit *dialog,
                                GMAudioProfile *profile)
{
  GtkWidget *w;

  w = gm_audio_profile_edit_get_widget (dialog, "profile-active-button");
  g_assert (GTK_IS_WIDGET (w));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                gm_audio_profile_get_active (profile));
}

static GtkWidget*
gm_audio_profile_edit_get_widget (GMAudioProfileEdit *dialog,
                             const char *widget_name)
{
  GtkBuilder *builder;
  GtkWidget *w;

  builder = dialog->priv->builder;

  g_return_val_if_fail (builder, NULL);

  w = GTK_WIDGET (gtk_builder_get_object (builder, widget_name));

  if (w == NULL)
    g_error ("No such widget %s", widget_name);

  return w;
}

