/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho Parro.
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

#ifndef __DNS_SEARCH_H_
#define __DNS_SEARCH_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
{
  GST_ADDRESS_TYPE_IP,
  GST_ADDRESS_TYPE_DOMAIN
} GstAddressType;

#define GST_TYPE_ADDRESS_TYPE            (gst_address_type_get_type ())
#define GST_TYPE_ADDRESS_LIST            (gst_address_list_get_type ())
#define GST_ADDRESS_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_ADDRESS_LIST, GstAddressList))
#define GST_ADDRESS_LIST_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_ADDRESS_LIST, GstAddressListClass))
#define GST_IS_ADDRESS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_ADDRESS_LIST))
#define GST_IS_ADDRESS_LIST_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_ADDRESS_LIST))
#define GST_ADDRESS_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_ADDRESS_LIST, GstAddressListClass))

typedef struct _GstAddressList        GstAddressList;
typedef struct _GstAddressListClass   GstAddressListClass;
typedef struct _GstAddressListPrivate GstAddressListPrivate;

struct _GstAddressList
{
  GObject parent_instance;

  GstAddressListPrivate *_priv;
};

struct _GstAddressListClass
{
  GObjectClass parent_class;
};

typedef void (*GstAddressListSaveFunc) (GList *list, gpointer data);

GType    gst_address_type_get_type (void);
GType    gst_address_list_get_type (void);

GstAddressList *gst_address_list_new         (GtkTreeView*, GtkButton*, GtkButton*, GstAddressType);
void            gst_address_list_add_address (GstAddressList*, const gchar*);
GSList*         gst_address_list_get_list    (GstAddressList*);
void            gst_address_list_clear       (GstAddressList*);

void            gst_address_list_set_save_func (GstAddressList         *address_list,
						GstAddressListSaveFunc  save_func,
						gpointer                save_func_data);

G_END_DECLS

#endif /* __DNS_SEARCH_H_ */
