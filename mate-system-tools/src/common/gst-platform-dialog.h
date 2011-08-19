/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 */

#ifndef __GST_PLATFORM_H__
#define __GST_PLATFORM_H__

G_BEGIN_DECLS

#include "gst-tool.h"

#define GST_TYPE_PLATFORM_DIALOG         (gst_platform_dialog_get_type ())
#define GST_PLATFORM_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_PLATFORM_DIALOG, GstPlatformDialog))
#define GST_PLATFORM_DIALOG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),     GST_TYPE_PLATFORM_DIALOG, GstPlatformDialogClass))
#define GST_IS_PLATFORM_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o),  GST_TYPE_PLATFORM_DIALOG))
#define GST_IS_PLATFORM_DIALOG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),     GST_TYPE_PLATFORM_DIALOG))
#define GST_PLATFORM_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),   GST_TYPE_PLATFORM_DIALOG, GstPlatformDialogClass))

typedef struct _GstPlatformDialog      GstPlatformDialog;
typedef struct _GstPlatformDialogClass GstPlatformDialogClass;

struct _GstPlatformDialog
{
	GtkDialog parent;
	OobsSession *session;

	/* private */
	gpointer _priv;
};

struct _GstPlatformDialogClass
{
	GtkDialogClass parent_class;
};

GType        gst_platform_dialog_get_type    (void);
GtkWidget   *gst_platform_dialog_new         (OobsSession *session);


G_END_DECLS

#endif /* __GST_PLATFORM_H__ */
