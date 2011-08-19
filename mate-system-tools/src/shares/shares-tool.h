/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#ifndef __GST_SHARES_TOOL__
#define __GST_SHARES_TOOL__

G_BEGIN_DECLS

#include <glib.h>
#include "gst-tool.h"
#include <oobs/oobs.h>

#define GST_TYPE_SHARES_TOOL         (gst_shares_tool_get_type())
#define GST_SHARES_TOOL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_SHARES_TOOL, GstSharesTool))
#define GST_SHARES_TOOL_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    GST_TYPE_SHARES_TOOL, GstSharesToolClass))
#define GST_IS_SHARES_TOOL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_SHARES_TOOL))
#define GST_IS_SHARES_TOOL_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    GST_TYPE_SHARES_TOOL))
#define GST_SHARES_TOOL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  GST_TYPE_SHARES_TOOL, GstSharesToolClass))

typedef struct _GstSharesTool      GstSharesTool;
typedef struct _GstSharesToolClass GstSharesToolClass;
	
struct _GstSharesTool {
	GstTool parent;

	OobsObject *nfs_config;
	OobsObject *smb_config;

	/* read only */
	OobsObject *services_config;
	OobsObject *hosts_config;
	OobsObject *users_config;

	gchar *path;

	guint smb_available : 1;
	guint nfs_available : 1;
};

struct _GstSharesToolClass {
	GstToolClass parent_class;
};

GstSharesTool *gst_shares_tool_new (void);


G_END_DECLS

#endif /* __GST_SHARES_TOOL__ */
