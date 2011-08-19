/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006 Carlos Garnacho.
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

#ifndef __SERVICES_TOOL_H__
#define __SERVICES_TOOL_H__

G_BEGIN_DECLS

#include "gst-tool.h"

#define GST_TYPE_SERVICES_TOOL            (gst_services_tool_get_type ())
#define GST_SERVICES_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SERVICES_TOOL, GstServicesTool))
#define GST_SERVICES_TOOL_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SERVICES_TOOL, GstServicesToolClass))
#define GST_IS_SERVICES_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SERVICES_TOOL))
#define GST_IS_SERVICES_TOOL_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SERVICES_TOOL))

typedef struct _GstServicesTool      GstServicesTool;
typedef struct _GstServicesToolClass GstServicesToolClass;

struct _GstServicesTool {
	GstTool tool;

	OobsObject *services_config;
	const OobsServicesRunlevel *default_runlevel;
};

struct _GstServicesToolClass {
	GstToolClass parent_class;
};

GType    gst_services_tool_get_type           (void);

GstTool *gst_services_tool_new                (void);


G_END_DECLS

#endif /* __SERVICES_TOOL_H__ */
