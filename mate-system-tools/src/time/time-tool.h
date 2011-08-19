/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#ifndef __TIME_TOOL_H__
#define __TIME_TOOL_H__

G_BEGIN_DECLS

#include "gst-tool.h"
#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"

#define GST_TYPE_TIME_TOOL            (gst_time_tool_get_type ())
#define GST_TIME_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_TIME_TOOL, GstTimeTool))
#define GST_TIME_TOOL_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_TIME_TOOL, GstTimeToolClass))
#define GST_IS_TIME_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_TIME_TOOL))
#define GST_IS_TIME_TOOL_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_TIME_TOOL))

typedef struct _GstTimeTool      GstTimeTool;
typedef struct _GstTimeToolClass GstTimeToolClass;

struct _GstTimeTool {
	GstTool tool;

	/* config */
	OobsObject *time_config;
	OobsObject *ntp_config;
	OobsObject *services_config;

	/* gui */
	GtkWidget *calendar;
	GtkWidget *seconds;
	GtkWidget *minutes;
	GtkWidget *hours;

	GtkWidget *ntp_use;
	GtkWidget *ntp_list;
	GtkWidget *synchronize_now;

	ETzMap    *tzmap;
	GtkWidget *timezone_dialog;
	GtkWidget *map_hover_label;

	OobsService *ntp_service;
};

struct _GstTimeToolClass {
	GstToolClass parent_class;
};

GType    gst_time_tool_get_type            (void);

GstTool *gst_time_tool_new                 (void);

void     gst_time_tool_start_clock         (GstTimeTool *tool);
void     gst_time_tool_stop_clock          (GstTimeTool *tool);

void     gst_time_tool_run_timezone_dialog (GstTimeTool *time_tool);


G_END_DECLS

#endif /* __TIME_TOOL_H__ */
