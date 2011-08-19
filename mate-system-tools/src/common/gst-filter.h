/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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

#ifndef __GST_FILTER_
#define __GST_FILTER_

#include <gtk/gtk.h>

enum {
  GST_FILTER_IP,
  GST_FILTER_IPV4,
  GST_FILTER_IPV6,
  GST_FILTER_PHONE
};

typedef enum {
  GST_ADDRESS_IPV4,
  GST_ADDRESS_IPV6,
  GST_ADDRESS_INCOMPLETE,
  GST_ADDRESS_IPV4_INCOMPLETE,
  GST_ADDRESS_IPV6_INCOMPLETE,
  GST_ADDRESS_ERROR
} GstAddressRet;

GstAddressRet gst_filter_check_ip_address (const gchar*);
void          gst_filter_init             (GtkEntry*, gint);

#endif /* __GST_FILTER_ */
