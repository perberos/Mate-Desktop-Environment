/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "egg-debug.h"
#include "gpm-common.h"

/**
 * gpm_get_timestring:
 * @time_secs: The time value to convert in seconds
 * @cookie: The cookie we are looking for
 *
 * Returns a localised timestring
 *
 * Return value: The time string, e.g. "2 hours 3 minutes"
 **/
gchar *
gpm_get_timestring (guint time_secs)
{
	char* timestring = NULL;
	gint  hours;
	gint  minutes;

	/* Add 0.5 to do rounding */
	minutes = (int) ( ( time_secs / 60.0 ) + 0.5 );

	if (minutes == 0) {
		timestring = g_strdup (_("Unknown time"));
		return timestring;
	}

	if (minutes < 60) {
		timestring = g_strdup_printf (ngettext ("%i minute",
							"%i minutes",
							minutes), minutes);
		return timestring;
	}

	hours = minutes / 60;
	minutes = minutes % 60;

	if (minutes == 0)
		timestring = g_strdup_printf (ngettext (
				"%i hour",
				"%i hours",
				hours), hours);
	else
		/* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
		 * Swap order with "%2$s %2$i %1$s %1$i if needed */
		timestring = g_strdup_printf (_("%i %s %i %s"),
				hours, ngettext ("hour", "hours", hours),
				minutes, ngettext ("minute", "minutes", minutes));
	return timestring;
}

/**
 * gpm_icon_policy_from_string:
 **/
GpmIconPolicy
gpm_icon_policy_from_string (const gchar *policy)
{
	if (policy == NULL)
		return GPM_ICON_POLICY_NEVER;
	if (g_strcmp0 (policy, "always") == 0)
		return GPM_ICON_POLICY_ALWAYS;
	if (g_strcmp0 (policy, "present") == 0)
		return GPM_ICON_POLICY_PRESENT;
	if (g_strcmp0 (policy, "charge") == 0)
		return GPM_ICON_POLICY_CHARGE;
	if (g_strcmp0 (policy, "low") == 0)
		return GPM_ICON_POLICY_LOW;
	if (g_strcmp0 (policy, "critical") == 0)
		return GPM_ICON_POLICY_CRITICAL;
	if (g_strcmp0 (policy, "never") == 0)
		return GPM_ICON_POLICY_NEVER;
	return GPM_ICON_POLICY_NEVER;
}

/**
 * gpm_icon_policy_to_string:
 **/
const gchar *
gpm_icon_policy_to_string (GpmIconPolicy policy)
{
	if (policy == GPM_ICON_POLICY_ALWAYS)
		return "always";
	if (policy == GPM_ICON_POLICY_PRESENT)
		return "present";
	if (policy == GPM_ICON_POLICY_CHARGE)
		return "charge";
	if (policy == GPM_ICON_POLICY_LOW)
		return "low";
	if (policy == GPM_ICON_POLICY_CRITICAL)
		return "critical";
	if (policy == GPM_ICON_POLICY_NEVER)
		return "never";
	return "never";
}

/**
 * gpm_action_policy_from_string:
 **/
GpmActionPolicy
gpm_action_policy_from_string (const gchar *policy)
{
	if (policy == NULL)
		return GPM_ACTION_POLICY_NOTHING;
	if (g_strcmp0 (policy, "blank") == 0)
		return GPM_ACTION_POLICY_BLANK;
	if (g_strcmp0 (policy, "shutdown") == 0)
		return GPM_ACTION_POLICY_SHUTDOWN;
	if (g_strcmp0 (policy, "suspend") == 0)
		return GPM_ACTION_POLICY_SUSPEND;
	if (g_strcmp0 (policy, "hibernate") == 0)
		return GPM_ACTION_POLICY_HIBERNATE;
	if (g_strcmp0 (policy, "interactive") == 0)
		return GPM_ACTION_POLICY_INTERACTIVE;
	return GPM_ACTION_POLICY_NOTHING;
}

/**
 * gpm_action_policy_to_string:
 **/
const gchar *
gpm_action_policy_to_string (GpmActionPolicy policy)
{
	if (policy == GPM_ACTION_POLICY_BLANK)
		return "blank";
	if (policy == GPM_ACTION_POLICY_SHUTDOWN)
		return "shutdown";
	if (policy == GPM_ACTION_POLICY_SUSPEND)
		return "suspend";
	if (policy == GPM_ACTION_POLICY_HIBERNATE)
		return "hibernate";
	if (policy == GPM_ACTION_POLICY_INTERACTIVE)
		return "interactive";
	return "nothing";
}

/**
 * gpm_help_display:
 * @link_id: Subsection of mate-power-manager help section
 **/
void
gpm_help_display (const gchar *link_id)
{
	GError *error = NULL;
	gchar *uri;

	if (link_id != NULL)
		uri = g_strconcat ("ghelp:mate-power-manager?", link_id, NULL);
	else
		uri = g_strdup ("ghelp:mate-power-manager");

	gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &error);

	if (error != NULL) {
		GtkWidget *d;
		d = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", error->message);
		gtk_dialog_run (GTK_DIALOG(d));
		gtk_widget_destroy (d);
		g_error_free (error);
	}
	g_free (uri);
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

void
gpm_common_test (gpointer data)
{
	EggTest *test = (EggTest *) data;
	if (egg_test_start (test, "GpmCommon") == FALSE)
		return;

	egg_test_end (test);
}

#endif

