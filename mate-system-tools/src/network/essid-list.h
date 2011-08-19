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

#ifndef __ESSID_LIST_H
#define __ESSID_LIST_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GST_TYPE_ESSID_LIST           (gst_essid_list_get_type ())
#define GST_ESSID_LIST(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_ESSID_LIST, GstEssidList))
#define GST_ESSID_LIST_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_ESSID_LIST, GstEssidListClass))
#define GST_IS_ESSID_LIST(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_ESSID_LIST))
#define GST_IS_ESSID_LIST_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_ESSID_LIST))
#define GST_ESSID_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_ESSID_LIST, GstEssidListClass))

typedef struct _GstEssidList      GstEssidList;
typedef struct _GstEssidListClass GstEssidListClass;
typedef struct _GstEssidListEntry GstEssidListEntry;

struct _GstEssidList
{
  GObject parent_instance;

  GList *essids;

  /*<private>*/
  gpointer _priv;
};

struct _GstEssidListClass
{
  GObjectClass parent_class;

  void (*changed) (GstEssidList *list);
};

struct _GstEssidListEntry
{
  gboolean encrypted;
  gdouble quality;
  gchar *essid;
};

GType         gst_essid_list_get_type  (void);
GstEssidList *gst_essid_list_new       (const gchar *interface);
GList        *gst_essid_list_get_list  (GstEssidList *list);

G_END_DECLS

#endif /* __ESSID_LIST_H */
