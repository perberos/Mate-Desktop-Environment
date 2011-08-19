/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of time-admin, a ximian-setup-tool frontend
 * for system time configuration.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 *          Jacob Berkman <jacob@ximian.com>
 *          Chema Celorio <chema@ximian.com>
 *          Carlos Garnacho Parro <garparr@teleline.es>
 */

#include <config.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <glib/gi18n.h>
#include "gst.h"
#include "time-tool.h"

#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"
#include "ntp-servers-list.h"

static const gchar *policy_widgets [] = {
	"timezone_button",
	"configuration_options",
	"timeserver_button",
	"hours",
	"minutes",
	"seconds",
	"calendar",
	"update_time",
	NULL
};

ETzMap *tzmap;

static void update_tz (GstTimeTool *time_tool);

void timezone_button_clicked (GtkWidget *w, gpointer data);
void server_button_clicked   (GtkWidget *w, gpointer data);

#define is_leap_year(yyy) ((((yyy % 4) == 0) && ((yyy % 100) != 0)) || ((yyy % 400) == 0));

static void
gst_time_update_date (GstTimeTool *tool, gint add)
{
	static const gint month_length[2][13] =
	{
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	GtkWidget *calendar;
	guint day, month, year;
	gint days_in_month;
	gboolean leap_year;

	calendar = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog,
					  "calendar");
	gtk_calendar_get_date (GTK_CALENDAR (calendar),
			       &year, &month, &day);

	/* Taken from gtk_calendar which was taken from lib_date */
	leap_year = is_leap_year (year);
	days_in_month = month_length [leap_year][month+1];

	if (add != 0) {
		day += add;
		if (day < 1) {
			day = (month_length [leap_year][month]) + day;
			month--;
		} else if (day > days_in_month) {
			day -= days_in_month;
			month++;
		}

		if (month < 0) {
			year--;
			leap_year = is_leap_year (year);
			month = 11;
			day = month_length [leap_year][month+1];
		} else if (month > 11) {
			year++;
			leap_year = is_leap_year (year);
			month = 0;
			day = 1;
		}
	}

	gtk_calendar_select_month (GTK_CALENDAR (calendar),
				   month, year);
	gtk_calendar_select_day (GTK_CALENDAR (calendar),
				 day);
}
#undef is_leap_year

void
timezone_button_clicked (GtkWidget *w, gpointer data)
{
	GstTimeTool *time_tool;
	GstDialog *dialog;

	dialog = GST_DIALOG (data);
	time_tool = GST_TIME_TOOL (gst_dialog_get_tool (dialog));
	gst_time_tool_run_timezone_dialog (time_tool);
}

void
server_button_clicked (GtkWidget *w, gpointer data)
{
	GtkWidget *d;
	GstDialog *dialog;
	GstTool *tool;

	dialog = GST_DIALOG (data);
	tool = gst_dialog_get_tool (dialog);

	d = gst_dialog_get_widget (dialog, "time_server_window");

	gst_dialog_add_edit_dialog (tool->main_dialog, d);

	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (dialog));
	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_hide (d);

	gst_dialog_remove_edit_dialog (tool->main_dialog, d);
}

int main (int argc, char* argv[])
{
	gst_init_tool("mate-time-admin", argc, argv, NULL);

	GstTool* tool = GST_TOOL(gst_time_tool_new());

	gst_dialog_require_authentication_for_widgets(tool->main_dialog, policy_widgets);

	gtk_widget_show(GTK_WIDGET(tool->main_dialog));
	gtk_main();

	return 0;
}
