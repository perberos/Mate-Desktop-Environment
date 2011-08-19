/* gm_audio-profile-choose.c: combo box to choose a specific profile */

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
#include <gio/gio.h>
#include <gst/gst.h>
#include <mateconf/mateconf-client.h>

#include "gmp-util.h"
#include "audio-profile-choose.h"

/* Private structure */
struct _GMAudioProfileChoosePrivate
{
  GtkTreeModel   *model;
  GMAudioProfile *profile;
};

enum
{
  P_0,
  P_ACTIVE_PROFILE
};

enum
{
  S_PROFILE_CHANGED,
  LAST_SIGNAL
};

enum
{
  NAME_COLUMN,
  ID_COLUMN,
  PROFILE_COLUMN,
  N_COLUMNS
};

static guint signals[LAST_SIGNAL] = { 0 };

/* GObject methods */
static void gm_audio_profile_choose_set_property        (GObject      *object,
							 guint         prop_id,
							 const GValue *value,
							 GParamSpec   *pspec);
static void gm_audio_profile_choose_get_property        (GObject      *object,
							 guint         prop_id,
							 GValue       *value,
							 GParamSpec   *pspec);
static void gm_audio_profile_choose_dispose             (GObject      *object);

/* GtkComboBox methods */
static void gm_audio_profile_choose_changed             (GtkComboBox  *combo);

/* Internal routines */
static void audio_profile_forgotten                     (GMAudioProfile       *profile,
							 GMAudioProfileChoose *choose);

G_DEFINE_TYPE (GMAudioProfileChoose, gm_audio_profile_choose, GTK_TYPE_COMBO_BOX)

/* Initialization */
static void
gm_audio_profile_choose_init (GMAudioProfileChoose *choose)
{
  GMAudioProfileChoosePrivate *priv;
  GtkListStore                *store;
  GList                       *orig,
			      *profiles;
  GtkCellRenderer             *cell;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (choose,
				      GM_AUDIO_TYPE_PROFILE_CHOOSE,
				      GMAudioProfileChoosePrivate);

  /* Create model */
  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING,
					 G_TYPE_STRING,
					 GM_AUDIO_TYPE_PROFILE);
  for (orig = profiles = gm_audio_profile_get_active_list ();
       profiles;
       profiles = g_list_next (profiles))
    {
      GMAudioProfile *profile = profiles->data;
      gchar          *profile_name,
		     *temp_file_name,
		     *mime_type_description;
      GtkTreeIter     iter;

      temp_file_name = g_strdup_printf (".%s", gm_audio_profile_get_extension (profile));
      mime_type_description = g_content_type_get_description (temp_file_name);
      g_free (temp_file_name);

      profile_name = g_strdup_printf (gettext ("%s (%s)"),
				      gm_audio_profile_get_name (profile),
				      mime_type_description);
      g_free (mime_type_description);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  NAME_COLUMN, profile_name,
			  ID_COLUMN, gm_audio_profile_get_id (profile),
			  PROFILE_COLUMN, profile,
			  -1);

      g_signal_connect (profile, "forgotten",
			G_CALLBACK (audio_profile_forgotten), choose);

      g_free (profile_name);
    }
  g_list_free (orig);

  /* Display name in the combo */
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (choose), cell, TRUE );
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (choose), cell, "text", NAME_COLUMN);
  gtk_combo_box_set_model (GTK_COMBO_BOX (choose), GTK_TREE_MODEL (store));
  g_object_unref (store);

  /* Populate private struct */
  priv->model   = GTK_TREE_MODEL (store);
  priv->profile = NULL;

  /* cache access to private data */
  choose->priv = priv;
}

static void
gm_audio_profile_choose_class_init (GMAudioProfileChooseClass *klass)
{
  GObjectClass     *g_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *c_class = GTK_COMBO_BOX_CLASS (klass);
  GParamSpec       *pspec;

  g_class->get_property = gm_audio_profile_choose_get_property;
  g_class->set_property = gm_audio_profile_choose_set_property;
  g_class->dispose      = gm_audio_profile_choose_dispose;

  c_class->changed = gm_audio_profile_choose_changed;

  /* Signals */
  /**
   * GMAudioProfileChoose::profile-changed:
   * @choose: the object which received the signal
   *
   * The profile-changed signal is emitted when the active profile is changed.
   *
   * Since: 2.32/3.0
   */
  signals[S_PROFILE_CHANGED] =
    g_signal_new (g_intern_static_string ("profile-changed"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMAudioProfileChooseClass, profile_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE,
		  1, GM_AUDIO_TYPE_PROFILE);

  /* Properties */
  /**
   * GMAudioProfileChoose:active-profile:
   *
   * Currently shown #GMAudioProfile.
   *
   * Since: 2.32/3.0
   */
  pspec = g_param_spec_object ("active-profile",
			       "Active profile",
			       "Currently selected GMAudioProfile",
			       GM_AUDIO_TYPE_PROFILE,
			       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (g_class, P_ACTIVE_PROFILE, pspec);

  /* Add private data */
  g_type_class_add_private (g_class, sizeof (GMAudioProfileChoose));
}

static void
gm_audio_profile_choose_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gm_audio_profile_choose_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  GMAudioProfileChoosePrivate *priv = GM_AUDIO_PROFILE_CHOOSE (object)->priv;

  switch (prop_id)
    {
    case P_ACTIVE_PROFILE:
      g_value_set_object (value, priv->profile);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gm_audio_profile_choose_dispose (GObject *object)
{
  GMAudioProfileChoosePrivate *priv = GM_AUDIO_PROFILE_CHOOSE (object)->priv;

  if (priv->profile)
    {
      g_object_unref (priv->profile);
      priv->profile = NULL;
    }

  G_OBJECT_CLASS (gm_audio_profile_choose_parent_class)->dispose (object);
}

static void
gm_audio_profile_choose_changed (GtkComboBox *combo)
{
  GMAudioProfileChoosePrivate *priv;
  GtkTreeIter                  iter;
  GMAudioProfile              *new_profile = NULL;

  priv = GM_AUDIO_PROFILE_CHOOSE (combo)->priv;
  if (gtk_combo_box_get_active_iter (combo, &iter))
    gtk_tree_model_get (priv->model, &iter, PROFILE_COLUMN, &new_profile, -1);

  if (priv->profile != new_profile)
    {
      if (priv->profile)
	g_object_unref (priv->profile);
      priv->profile = new_profile;
      g_signal_emit (combo, signals[S_PROFILE_CHANGED], 0, new_profile);
      g_object_notify (G_OBJECT (combo), "active-profile");
    }
  else
    if (new_profile)
      g_object_unref (new_profile);
}

static void
audio_profile_forgotten (GMAudioProfile       *profile,
			 GMAudioProfileChoose *choose)
{
  GMAudioProfileChoosePrivate *priv;
  GtkTreeIter                  iter;
  GMAudioProfile              *tmp;

  priv = choose->priv;

  if (!gtk_tree_model_get_iter_first (priv->model, &iter))
    return;

  do
    {
      gtk_tree_model_get (priv->model, &iter, PROFILE_COLUMN, &tmp, -1);
      if (tmp == profile)
	{
	  gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter);
	  g_object_unref (tmp);
	  return;
	}
      g_object_unref (tmp);
    }
  while (gtk_tree_model_iter_next (priv->model, &iter));
}

/* Public API */
/**
 * gm_audio_profile_choose_new:
 *
 * Creates new #GMAudioProfileChoose, populated with currently active profiles.
 *
 * Return value: A new #GMAudioProfileChoose.
 */
GtkWidget*
gm_audio_profile_choose_new (void)
{
  return g_object_new (GM_AUDIO_TYPE_PROFILE_CHOOSE, NULL);
}

/**
 * gm_audio_profile_choose_get_active_profile:
 * @choose: #GMAudioProfileChoose widget
 *
 * This function retrieves currently selected #GMAudioProfile. Returned object is
 * owned by #GMAudioProfileChoose and should not be unreferenced.
 *
 * Return value: Currently selected #GMAudioProfile or %NULL if none is
 * selected.
 *
 * Since: 2.32/3.0
 */
GMAudioProfile *
gm_audio_profile_choose_get_active_profile (GMAudioProfileChoose *choose)
{
  g_return_val_if_fail (GM_AUDIO_IS_PROFILE_CHOOSE (choose), NULL);

  return choose->priv->profile;
}

/**
 * gm_audio_profile_choose_set_active_profile:
 * @choose: #GMAudioProfileChoose widget
 * @id:     A id of #GMAudioProfile that should be selected
 *
 * This function sets currently selected #GMAudioProfile. If no profile matches
 * the @id, first available profile is set as selected and function returns
 * %FALSE.
 *
 * Return value: %TRUE if profile has been successfully set, %FALSE otherwise.
 *
 * Since: 2.32/3.0
 */
gboolean
gm_audio_profile_choose_set_active_profile (GMAudioProfileChoose *choose,
					    const gchar          *id)
{
  GMAudioProfileChoosePrivate *priv;
  GtkTreeIter                  iter;
  gchar                       *tmp;

  g_return_val_if_fail (GM_AUDIO_IS_PROFILE_CHOOSE (choose), FALSE);

  priv = choose->priv;

  if (!gtk_tree_model_get_iter_first (priv->model, &iter))
    return FALSE;

  do
    {
      gtk_tree_model_get (priv->model, &iter, ID_COLUMN, &tmp, -1);
      if (!g_strcmp0 (tmp, id))
	{
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (choose), &iter);
	  g_free (tmp);
	  return TRUE;
	}
      g_free (tmp);
    }
  while (gtk_tree_model_iter_next (priv->model, &iter));

  /* Fallback to first entry */
  gtk_combo_box_set_active (GTK_COMBO_BOX (choose), 0);

  return FALSE;
}

/* Deprecated functions */
/**
 * gm_audio_profile_choose_get_active:
 * @choose: A #GMAudioProfileChoose widget
 *
 * This function retrieves currently selected #GMAudioProfile. Returned object is
 * owned by #GMAudioProfileChoose and should not be unreferenced.
 *
 * Return value: Currently selected #GMAudioProfile or %NULL if none is
 * selected.
 *
 * Deprecated: 2.32/3.0: Use gm_audio_profile_choose_get_active_profile()
 *  instead.
 */
GMAudioProfile*
gm_audio_profile_choose_get_active (GtkWidget *choose)
{
  /* We cannot simply wrap gm_audio_profile_choose_get_active_profile() here,
   * since this function can be (and in case of sound-juicer is) called from
   * GtkComboBox::changed signal handler. In this situation, code would return
   * invalid profile, since we haven't updated the current selection yet. */
  GMAudioProfileChoosePrivate *priv;
  GtkTreeIter                  iter;
  GMAudioProfile              *new_profile = NULL;

  g_return_val_if_fail (GM_AUDIO_IS_PROFILE_CHOOSE (choose), NULL);

  priv = GM_AUDIO_PROFILE_CHOOSE (choose)->priv;
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (choose), &iter))
    {
      gtk_tree_model_get (priv->model, &iter, PROFILE_COLUMN, &new_profile, -1);
      g_object_unref (new_profile);
    }

  return new_profile;
}

/**
 * gm_audio_profile_choose_set_active:
 * @choose: #GMAudioProfileChoose widget
 * @id:     A id of #GMAudioProfile that should be selected
 *
 * This function sets currently selected #GMAudioProfile. If no profile matches
 * the @id, first available profile is set as selected and function returns
 * %FALSE.
 *
 * Return value: %TRUE if profile has been successfully set, %FALSE otherwise.
 *
 * Deprecated: 2.32/3.0: Use gm_audio_profile_choose_set_active_profile()
 *  instead.
 */
gboolean
gm_audio_profile_choose_set_active (GtkWidget  *choose,
				    const char *id)
{
  return gm_audio_profile_choose_set_active_profile (GM_AUDIO_PROFILE_CHOOSE (choose), id);
}
