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

#include <glib.h>
#include <glib/gi18n.h>
#include "services-tool.h"
#include "gst.h"

static void  gst_services_tool_class_init     (GstServicesToolClass *class);
static void  gst_services_tool_init           (GstServicesTool      *tool);
static void  gst_services_tool_finalize       (GObject              *object);

static void  gst_services_tool_update_gui     (GstTool *tool);
static void  gst_services_tool_update_config  (GstTool *tool);

G_DEFINE_TYPE (GstServicesTool, gst_services_tool, GST_TYPE_TOOL);

static void
gst_services_tool_class_init (GstServicesToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GstToolClass *tool_class = GST_TOOL_CLASS (class);
	
	object_class->finalize = gst_services_tool_finalize;
	tool_class->update_gui = gst_services_tool_update_gui;
	tool_class->update_config = gst_services_tool_update_config;
}

static void
gst_services_tool_init (GstServicesTool *tool)
{
	tool->services_config = oobs_services_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->services_config, TRUE);
}

static void
gst_services_tool_finalize (GObject *object)
{
	GstServicesTool *tool = GST_SERVICES_TOOL (object);

	g_object_unref (tool->services_config);

	(* G_OBJECT_CLASS (gst_services_tool_parent_class)->finalize) (object);
}

static void
gst_services_tool_update_gui (GstTool *tool)
{
	OobsServicesConfig *config;
	OobsServicesRunlevel *rl;
	OobsList *list;
	OobsListIter iter;
	GObject *service;
	gboolean valid;
	guint status;

	config = OOBS_SERVICES_CONFIG (GST_SERVICES_TOOL (tool)->services_config);
	list = oobs_services_config_get_services (config);
	valid = oobs_list_get_iter_first (list, &iter);

	rl = (OobsServicesRunlevel *) GST_SERVICES_TOOL (tool)->default_runlevel;

	while (valid) {
		service = oobs_list_get (list, &iter);
		/* Don't add services not listed for this runlevel, as this status is not valid */
		oobs_service_get_runlevel_configuration (OOBS_SERVICE (service), rl, &status, NULL);
		if (status != OOBS_SERVICE_IGNORE);
			table_add_service (service, &iter);
		g_object_unref (service);

		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
gst_services_tool_update_config (GstTool *tool)
{
	GstServicesTool *services_tool = GST_SERVICES_TOOL (tool);

	services_tool->default_runlevel = oobs_services_config_get_default_runlevel (OOBS_SERVICES_CONFIG (services_tool->services_config));
}

GstTool*
gst_services_tool_new (void)
{
	return g_object_new (GST_TYPE_SERVICES_TOOL,
			     "name", "services",
			     "title", _("Services Settings"),
			     "icon", "network-server",
	                     "lock-button", FALSE,
			     NULL);
}
