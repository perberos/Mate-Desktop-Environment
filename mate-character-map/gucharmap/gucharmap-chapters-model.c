/*
 * Copyright © 2004 Noah Levitt
 * Copyright © 2007 Christian Persch
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#include <config.h>

#include <string.h>

#include "gucharmap.h"
#include "gucharmap-private.h"
#include "gucharmap-marshal.h"

G_DEFINE_TYPE (GucharmapChaptersModel, gucharmap_chapters_model, GTK_TYPE_LIST_STORE)

static GucharmapCodepointList * 
default_get_codepoint_list (GucharmapChaptersModel *chapters,
                            GtkTreeIter *iter)
{
  return gucharmap_block_codepoint_list_new (0, UNICHAR_MAX);
}

static void
gucharmap_chapters_model_init (GucharmapChaptersModel *model)
{
  model->priv = G_TYPE_INSTANCE_GET_PRIVATE (model, GUCHARMAP_TYPE_CHAPTERS_MODEL, GucharmapChaptersModelPrivate);
}

static void
gucharmap_chapters_model_finalize (GObject *object)
{
  GucharmapChaptersModel *model = GUCHARMAP_CHAPTERS_MODEL (object);
  GucharmapChaptersModelPrivate *priv = model->priv;

  if (priv->book_list)
    g_object_unref (priv->book_list);

  G_OBJECT_CLASS (gucharmap_chapters_model_parent_class)->finalize (object);
}

static void
gucharmap_chapters_model_class_init (GucharmapChaptersModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (object_class, sizeof (GucharmapChaptersModelPrivate));

  object_class->finalize = gucharmap_chapters_model_finalize;

  klass->get_codepoint_list = default_get_codepoint_list;
}

/**
 * gucharmap_chapters_model_get_codepoint_list:
 * @chapters: a #GucharmapChaptersModel
 * @iter: a #GtkTreeIter
 *
 * Creates a new #GucharmapCodepointList representing the characters in the
 * current chapter.
 *
 * Return value: the newly-created #GucharmapCodepointList, or NULL if
 * there is no chapter selected. The caller should release the result with
 * g_object_unref() when finished.
 **/
GucharmapCodepointList * 
gucharmap_chapters_model_get_codepoint_list (GucharmapChaptersModel *chapters,
                                       GtkTreeIter       *iter)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHAPTERS_MODEL (chapters), NULL);

  return GUCHARMAP_CHAPTERS_MODEL_GET_CLASS (chapters)->get_codepoint_list (chapters, iter);
}

/**
 * gucharmap_chapters_model_get_codepoint_list:
 * @chapters: a #GucharmapChaptersModel
 *
 * Return value: a reference to a #GucharmapCodepointList representing all the characters
 * in all the chapters. It should not be modified, but must be g_object_unref()'d after use.
 **/
GucharmapCodepointList *
gucharmap_chapters_model_get_book_codepoint_list (GucharmapChaptersModel *chapters)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHAPTERS_MODEL (chapters), NULL);

  return GUCHARMAP_CHAPTERS_MODEL_GET_CLASS (chapters)->get_book_codepoint_list (chapters);
}

/**
 * gucharmap_chapters_model_character_to_iter:
 * @chapters: a #GucharmapChaptersModel
 * @wc: a character
 * @iter: a #GtkTreeIter
 *
 * Return value: %TRUE on success, %FALSE on failure.
 **/
gboolean
gucharmap_chapters_model_character_to_iter (GucharmapChaptersModel *chapters,
                                            gunichar           wc,
                                            GtkTreeIter       *iter)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHAPTERS_MODEL (chapters), FALSE);

  return GUCHARMAP_CHAPTERS_MODEL_GET_CLASS (chapters)->character_to_iter (chapters, wc, iter);
}

const char *
gucharmap_chapters_model_get_title (GucharmapChaptersModel *chapters)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHAPTERS_MODEL (chapters), NULL);

  return GUCHARMAP_CHAPTERS_MODEL_GET_CLASS (chapters)->title;
}

gboolean
gucharmap_chapters_model_id_to_iter (GucharmapChaptersModel *chapters_model,
                                     const char *id,
                                     GtkTreeIter *_iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (chapters_model);
  GtkTreeIter iter;
  char *str;
  int match;

  g_return_val_if_fail (GUCHARMAP_IS_CHAPTERS_MODEL (model), FALSE);

  if (!id)
    return FALSE;

  if (!gtk_tree_model_get_iter_first (model, &iter))
    return FALSE;

  do {
    gtk_tree_model_get(model, &iter, GUCHARMAP_CHAPTERS_MODEL_COLUMN_ID, &str, -1);
    match = strcmp (id, str);
    g_free(str);
    if (0 == match) {
      *_iter = iter;
      break;
    }
  } while (gtk_tree_model_iter_next (model, &iter));

  return 0 == match;
}
