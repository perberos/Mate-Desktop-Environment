/*
 * Copyright © 2004 Noah Levitt
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
#include <glib.h>

#include "gucharmap.h"
#include "gucharmap-private.h"

struct _GucharmapBlockCodepointListPrivate
{
  gunichar start;
  gunichar end;
};

enum
{
  PROP_0,
  PROP_FIRST_CODEPOINT,
  PROP_LAST_CODEPOINT
};

G_DEFINE_TYPE (GucharmapBlockCodepointList, gucharmap_block_codepoint_list, GUCHARMAP_TYPE_CODEPOINT_LIST)

static gunichar
get_char (GucharmapCodepointList *list,
          gint                    index)
{
  GucharmapBlockCodepointList *block_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (list);
  GucharmapBlockCodepointListPrivate *priv = block_list->priv;

  if (index > (gint)priv->end - priv->start)
    return (gunichar)(-1);
  else
    return (gunichar) priv->start + index;
}

static gint 
get_index (GucharmapCodepointList *list,
           gunichar                wc)
{
  GucharmapBlockCodepointList *block_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (list);
  GucharmapBlockCodepointListPrivate *priv = block_list->priv;

  if (wc < priv->start || wc > priv->end)
    return -1;
  else
    return wc - priv->start;
}

static gint
get_last_index (GucharmapCodepointList *list)
{
  GucharmapBlockCodepointList *block_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (list);
  GucharmapBlockCodepointListPrivate *priv = block_list->priv;

  return priv->end - priv->start;
}

static void
gucharmap_block_codepoint_list_init (GucharmapBlockCodepointList *list)
{
  list->priv = G_TYPE_INSTANCE_GET_PRIVATE (list, GUCHARMAP_TYPE_BLOCK_CODEPOINT_LIST, GucharmapBlockCodepointListPrivate);
}

static GObject *
gucharmap_block_codepoint_list_constructor (GType type,
                                            guint n_construct_properties,
                                            GObjectConstructParam *construct_params)
{
  GObject *object;
  GucharmapBlockCodepointList *block_codepoint_list;
  GucharmapBlockCodepointListPrivate *priv;

  object = G_OBJECT_CLASS (gucharmap_block_codepoint_list_parent_class)->constructor
             (type, n_construct_properties, construct_params);

  block_codepoint_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (object);
  priv = block_codepoint_list->priv;

  g_assert (priv->start <= priv->end);

  return object;
}

static void
gucharmap_block_codepoint_list_set_property (GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
  GucharmapBlockCodepointList *block_codepoint_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (object);
  GucharmapBlockCodepointListPrivate *priv = block_codepoint_list->priv;

  switch (prop_id) {
    case PROP_FIRST_CODEPOINT:
      priv->start = g_value_get_uint (value);
      break;
    case PROP_LAST_CODEPOINT:
      priv->end = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gucharmap_block_codepoint_list_get_property (GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
  GucharmapBlockCodepointList *block_codepoint_list = GUCHARMAP_BLOCK_CODEPOINT_LIST (object);
  GucharmapBlockCodepointListPrivate *priv = block_codepoint_list->priv;

  switch (prop_id) {
    case PROP_FIRST_CODEPOINT:
      g_value_set_uint (value, priv->start);
      break;
    case PROP_LAST_CODEPOINT:
      g_value_set_uint (value, priv->end);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gucharmap_block_codepoint_list_class_init (GucharmapBlockCodepointListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GucharmapCodepointListClass *codepoint_list_class = GUCHARMAP_CODEPOINT_LIST_CLASS (klass);

  object_class->get_property = gucharmap_block_codepoint_list_get_property;
  object_class->set_property = gucharmap_block_codepoint_list_set_property;
  object_class->constructor = gucharmap_block_codepoint_list_constructor;

  g_type_class_add_private (klass, sizeof (GucharmapBlockCodepointListPrivate));

  codepoint_list_class->get_char = get_char;
  codepoint_list_class->get_index = get_index;
  codepoint_list_class->get_last_index = get_last_index;

  /* Not using g_param_spec_unichar on purpose, since it disallows certain values
   * we want (it's performing a g_unichar_validate).
   */
  g_object_class_install_property
    (object_class,
     PROP_FIRST_CODEPOINT,
     g_param_spec_uint ("first-codepoint", NULL, NULL,
                        0,
                        UNICHAR_MAX,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_NICK |
                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_LAST_CODEPOINT,
     g_param_spec_uint ("last-codepoint", NULL, NULL,
                        0,
                        UNICHAR_MAX,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_NICK |
                        G_PARAM_STATIC_BLURB));
}

/**
 * gucharmap_block_codepoint_list_new:
 * @start: the first codepoint
 * @end: the last codepoint
 *
 * Creates a new codepoint list for the range @start..@end.
 *
 * Return value: the newly-created #GucharmapBlockCodepointList. Use
 * g_object_unref() to free the result.
 **/
GucharmapCodepointList *
gucharmap_block_codepoint_list_new (gunichar start,
                                    gunichar end)
{
  g_return_val_if_fail (start <= end, NULL);

  return g_object_new (GUCHARMAP_TYPE_BLOCK_CODEPOINT_LIST,
                       "first-codepoint", (guint) start,
                       "last-codepoint", (guint) end,
                       NULL);
}
