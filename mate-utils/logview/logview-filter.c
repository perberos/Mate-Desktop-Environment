/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * mate-utils
 * Copyright (C) Johannes Schmid 2009 <jhs@mate.org>
 * 
 * mate-utils is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * mate-utils is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "logview-filter.h"

enum {
  PROP_0,
  PROP_REGEX,
  PROP_NAME,
  PROP_TEXTTAG
};

G_DEFINE_TYPE (LogviewFilter, logview_filter, G_TYPE_OBJECT);

struct _LogviewFilterPrivate {
  GRegex* regex;
  gchar* name;
  GtkTextTag* tag;
};

#define LOGVIEW_FILTER_GET_PRIVATE(o)  \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_FILTER, LogviewFilterPrivate))

static void
logview_filter_init (LogviewFilter *object) 
{
  object->priv = LOGVIEW_FILTER_GET_PRIVATE(object);
  object->priv->tag = NULL;
}

static void
logview_filter_finalize (GObject *object)
{
  LogviewFilterPrivate *priv = LOGVIEW_FILTER (object)->priv;

  if (priv->tag) {
    g_object_unref (priv->tag);
  }

  g_regex_unref (priv->regex);
  g_free (priv->name);

  G_OBJECT_CLASS (logview_filter_parent_class)->finalize (object);
}

static void
logview_filter_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  LogviewFilterPrivate* priv = LOGVIEW_FILTER (object)->priv;

  switch (prop_id) {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;
    case PROP_REGEX: {
      GError* err;
      const gchar* regex;

      err = NULL;
      
      regex = g_value_get_string (value);
      priv->regex = g_regex_new (regex, 0, 0, &err);

      if (err) {
        g_regex_unref (priv->regex);
        priv->regex = NULL;
        g_warning ("Couldn't create GRegex object: %s", err->message);
        g_error_free (err);
      }

      break;
    }
    case PROP_TEXTTAG: {
      if (priv->tag) {
        g_object_unref (priv->tag);
      }

      priv->tag = g_value_dup_object (value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
logview_filter_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  LogviewFilterPrivate* priv = LOGVIEW_FILTER (object)->priv;

  switch (prop_id) {
    case PROP_REGEX:
      g_value_set_string (value, g_regex_get_pattern (priv->regex));
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_TEXTTAG:
      g_value_set_object (value, priv->tag);
      break;
  }
}

static void
logview_filter_class_init (LogviewFilterClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = logview_filter_finalize;
  object_class->set_property = logview_filter_set_property;
  object_class->get_property = logview_filter_get_property;

  g_object_class_install_property (object_class,
                                   PROP_REGEX,
                                   g_param_spec_string ("regex",
                                                        "regular expression",
                                                        "regular expression",
                                                        "NULL",
                                                        G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        "NULL",
                                                        G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_TEXTTAG,
                                   g_param_spec_object ("texttag",
                                                        "texttag",
                                                        "The text tag to be set on matching lines",
                                                        GTK_TYPE_TEXT_TAG,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));


  g_type_class_add_private (klass, sizeof (LogviewFilterPrivate));
}

/* public methods */

LogviewFilter*
logview_filter_new (const gchar *name, const gchar *regex)
{
  return g_object_new (LOGVIEW_TYPE_FILTER,
                       "name", name,
                       "regex", regex,
                       NULL);
}

gboolean
logview_filter_filter (LogviewFilter *filter, const gchar *line)
{
  GMatchInfo* match_info;
  LogviewFilterPrivate* priv;
  gboolean retval;

  g_return_val_if_fail (LOGVIEW_IS_FILTER (filter), FALSE);
  g_return_val_if_fail (line != NULL, FALSE);

  priv = filter->priv;

  g_regex_match (priv->regex, line, 0, &match_info);

  retval = g_match_info_matches (match_info);

  g_match_info_free (match_info);

  return retval;
}

GtkTextTag *
logview_filter_get_tag (LogviewFilter *filter)
{
  g_return_val_if_fail (LOGVIEW_IS_FILTER (filter), NULL);

  return filter->priv->tag;
}
