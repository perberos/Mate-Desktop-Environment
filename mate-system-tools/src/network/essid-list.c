/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#include <glib.h>
#include <glib-object.h>
#include <iwlib.h>
#include "essid-list.h"

#define RESCAN_TIMEOUT 2000

typedef struct _GstEssidListPrivate GstEssidListPrivate;

struct _GstEssidListPrivate {
  gchar *interface;
  GList *list;
  gint fd;

  guint timeout;

  guint8 *buffer;
  gint buflen;
};

#define GST_ESSID_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_ESSID_LIST, GstEssidListPrivate))

static void gst_essid_list_class_init (GstEssidListClass *class);
static void gst_essid_list_init       (GstEssidList      *tool);
static void gst_essid_list_finalize   (GObject           *object);

static GObject* gst_essid_list_constructor (GType                  type,
					    guint                  n_construct_properties,
					    GObjectConstructParam *construct_params);

static void gst_essid_list_get_property (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec);
static void gst_essid_list_set_property (GObject         *object,
					 guint            prop_id,
					 const GValue    *value,
					 GParamSpec      *pspec);
static void schedule_query_essids       (GstEssidList *list);

enum {
  PROP_0,
  PROP_INTERFACE
};
  

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GstEssidList, gst_essid_list, G_TYPE_OBJECT);

static void
gst_essid_list_class_init (GstEssidListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gst_essid_list_finalize;
  object_class->constructor = gst_essid_list_constructor;
  object_class->get_property = gst_essid_list_get_property;
  object_class->set_property = gst_essid_list_set_property;

  g_object_class_install_property (object_class,
				   PROP_INTERFACE,
				   g_param_spec_string ("interface",
							"interface",
							"Interface used for scanning",
							NULL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GstEssidListClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_type_class_add_private (object_class,
			    sizeof (GstEssidListPrivate));
}

static void
allocate_buffer (GstEssidList *list,
		 gint          size)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (list);

  if (priv->buffer)
    g_free (priv->buffer);

  priv->buflen = size;
  priv->buffer = g_malloc0 (size);
}

static void
gst_essid_list_init (GstEssidList *list)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (list);
  priv->fd = iw_sockets_open ();

  allocate_buffer (list, IW_SCAN_MAX_DATA);
}

static void
free_essid_entry (GstEssidListEntry *entry)
{
  g_free (entry->essid);
  g_free (entry);
}

static void
gst_essid_list_finalize (GObject *object)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (object);

  if (priv->list)
    {
      g_list_foreach (priv->list, (GFunc) free_essid_entry, NULL);
      g_list_free (priv->list);
    }

  if (priv->timeout)
    {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
    }

  if (priv->fd >= 0)
    close (priv->fd);

  g_free (priv->interface);
  g_free (priv->buffer);

  (* G_OBJECT_CLASS (gst_essid_list_parent_class)->finalize) (object);
}

static gdouble
normalize_quality (iwqual          qual,
		   struct iw_range range,
		   gboolean        has_range)
{
  gdouble val;

  if (has_range)
    val = (gdouble) qual.qual / range.max_qual.qual;
  else
    {
      /* just put something, dunno what would
       * be the more intuitive value here...
       */
      val = 0.5;
    }

  return CLAMP (val, 0.0, 1.0);
}

static GList *
get_scan_info (GstEssidList    *list,
	       struct iwreq     req)
{
  GstEssidListPrivate *priv;
  struct iw_range  range;
  struct iw_event event;
  struct stream_descr stream;
  GstEssidListEntry *entry = NULL;
  GList *info = NULL;
  gboolean has_range;
  int ret = 1;

  if (!req.u.data.length)
    return NULL;

  priv = GST_ESSID_LIST_GET_PRIVATE (list);
  has_range = (iw_get_range_info (priv->fd, priv->interface, &range) >= 0);

  iw_init_event_stream (&stream, (char*) priv->buffer, req.u.data.length);

  while (ret > 0)
    {
      ret = iw_extract_event_stream (&stream, &event, range.we_version_compiled);

      switch (event.cmd)
	{
	case SIOCGIWAP:
	  /* found a new cell, store the last one if any */
	  if (entry)
	    info = g_list_prepend (info, entry);

	  entry = g_new0 (GstEssidListEntry, 1);
	  break;
	case SIOCGIWESSID:
	  entry->essid =
	    (event.u.essid.pointer) ?
	    g_strndup (event.u.essid.pointer, event.u.essid.length) :
	    g_strdup ("any");
	  break;
	case SIOCGIWENCODE:
	  entry->encrypted = ! (event.u.data.flags & IW_ENCODE_DISABLED);
	  break;
	case IWEVQUAL:
	  entry->quality = normalize_quality (event.u.qual, range, has_range);
	  break;
	default:
	  break;
	}
    }

  /* store the last entry */
  if (entry)
    info = g_list_prepend (info, entry);

  return info;
}

static gboolean
query_essids (GstEssidList *list)
{
  GstEssidListPrivate *priv;
  struct iwreq req;

  priv = GST_ESSID_LIST_GET_PRIVATE (list);

  req.u.data.pointer = priv->buffer;
  req.u.data.flags = 0;
  req.u.data.length = priv->buflen;

  if (iw_get_ext (priv->fd, priv->interface, SIOCGIWSCAN, &req) < 0)
    {
      if (errno == E2BIG)
	{
	  /* double buffer size */
	  allocate_buffer (list, priv->buflen * 2);
	}

      if (errno == E2BIG ||
	  errno == EAGAIN)
	{
	  /* reschedule the timeout, so we don't have
	   * to wait too long to get some data
	   */
	  schedule_query_essids (list);
	}

      return FALSE;
    }

  /* get info */
  g_list_foreach (priv->list, (GFunc) free_essid_entry, NULL);
  g_list_free (priv->list);
  priv->list = get_scan_info (list, req);

  g_signal_emit (list, signals[CHANGED], 0);

  return TRUE;
}

static void
schedule_query_essids (GstEssidList *list)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (list);

  if (priv->timeout)
    {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
    }

  priv->timeout = g_timeout_add (RESCAN_TIMEOUT, (GSourceFunc) query_essids, list);
}

static GObject*
gst_essid_list_constructor (GType                  type,
			    guint                  n_construct_properties,
			    GObjectConstructParam *construct_params)
{
  GObject *object;

  object = (* G_OBJECT_CLASS (gst_essid_list_parent_class)->constructor) (type,
									  n_construct_properties,
									  construct_params);
  query_essids (GST_ESSID_LIST (object));
  schedule_query_essids (GST_ESSID_LIST (object));

  return object;
}

static void
gst_essid_list_get_property (GObject         *object,
			     guint            prop_id,
			     GValue          *value,
			     GParamSpec      *pspec)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_INTERFACE:
      g_value_set_string (value, priv->interface);
      break;
    }
}

static void
gst_essid_list_set_property (GObject         *object,
			     guint            prop_id,
			     const GValue    *value,
			     GParamSpec      *pspec)
{
  GstEssidListPrivate *priv;

  priv = GST_ESSID_LIST_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_INTERFACE:
      if (priv->interface)
	g_free (priv->interface);

      priv->interface = g_value_dup_string (value);
      break;
    }
}

GstEssidList*
gst_essid_list_new (const gchar *interface)
{
  return g_object_new (GST_TYPE_ESSID_LIST,
		       "interface", interface,
		       NULL);
}

GList*
gst_essid_list_get_list (GstEssidList *list)
{
  GstEssidListPrivate *priv;

  g_return_val_if_fail (GST_IS_ESSID_LIST (list), NULL);

  priv = GST_ESSID_LIST_GET_PRIVATE (list);

  return priv->list;
}
