/* -*- mode: c; style: linux -*- */

/* keyboard-properties.c
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2001 Jonathan Blandford
 *
 * Written by: Bradford Hovinen <hovinen@ximian.com>
 *             Rachel Hestilow <hestilow@ximian.com>
 *	       Jonathan Blandford <jrb@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <mateconf/mateconf-client.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"
#include "activate-settings-daemon.h"
#include "capplet-stock-icons.h"

#include "mate-keyboard-properties-a11y.h"
#include "mate-keyboard-properties-xkb.h"

enum {
	RESPONSE_APPLY = 1,
	RESPONSE_CLOSE
};

static GtkBuilder *
create_dialog (void)
{
	GtkBuilder *dialog;
	GtkSizeGroup *size_group;
	GtkWidget *image;

	dialog = gtk_builder_new ();
    gtk_builder_add_from_file (dialog, MATECC_UI_DIR
                               "/mate-keyboard-properties-dialog.ui",
                               NULL);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("repeat_slow_label"));
	gtk_size_group_add_widget (size_group, WID ("delay_short_label"));
	gtk_size_group_add_widget (size_group, WID ("blink_slow_label"));
	g_object_unref (G_OBJECT (size_group));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("repeat_fast_label"));
	gtk_size_group_add_widget (size_group, WID ("delay_long_label"));
	gtk_size_group_add_widget (size_group, WID ("blink_fast_label"));
	g_object_unref (G_OBJECT (size_group));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("repeat_delay_scale"));
	gtk_size_group_add_widget (size_group, WID ("repeat_speed_scale"));
	gtk_size_group_add_widget (size_group, WID ("cursor_blink_time_scale"));
	g_object_unref (G_OBJECT (size_group));

	image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (WID ("xkb_layouts_add")), image);

	image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (WID ("xkb_reset_to_defaults")), image);

	return dialog;
}

static MateConfValue *
blink_from_widget (MateConfPropertyEditor * peditor, const MateConfValue * value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_INT);
	mateconf_value_set_int (new_value,
			     2600 - mateconf_value_get_int (value));

	return new_value;
}

static MateConfValue *
blink_to_widget (MateConfPropertyEditor * peditor, const MateConfValue * value)
{
	MateConfValue *new_value;
	gint current_rate;

	current_rate = mateconf_value_get_int (value);
	new_value = mateconf_value_new (MATECONF_VALUE_INT);
	mateconf_value_set_int (new_value,
			     CLAMP (2600 - current_rate, 100, 2500));

	return new_value;
}

static void
dialog_response (GtkWidget * widget,
		 gint response_id, MateConfChangeSet * changeset)
{
	if (response_id == GTK_RESPONSE_HELP)
		capplet_help (GTK_WINDOW (widget), "goscustperiph-2");
	else
		gtk_main_quit ();
}

static void
setup_dialog (GtkBuilder * dialog, MateConfChangeSet * changeset)
{
	GObject *peditor;
	gchar *monitor;

	peditor = mateconf_peditor_new_boolean
	    (changeset, "/desktop/mate/peripherals/keyboard/repeat",
	     WID ("repeat_toggle"), NULL);
	mateconf_peditor_widget_set_guard (MATECONF_PROPERTY_EDITOR (peditor),
					WID ("repeat_table"));

	mateconf_peditor_new_numeric_range
	    (changeset, "/desktop/mate/peripherals/keyboard/delay",
	     WID ("repeat_delay_scale"), NULL);

	mateconf_peditor_new_numeric_range
	    (changeset, "/desktop/mate/peripherals/keyboard/rate",
	     WID ("repeat_speed_scale"), NULL);

	peditor = mateconf_peditor_new_boolean
	    (changeset, "/desktop/mate/interface/cursor_blink",
	     WID ("cursor_toggle"), NULL);
	mateconf_peditor_widget_set_guard (MATECONF_PROPERTY_EDITOR (peditor),
					WID ("cursor_hbox"));
	mateconf_peditor_new_numeric_range (changeset,
					 "/desktop/mate/interface/cursor_blink_time",
					 WID ("cursor_blink_time_scale"),
					 "conv-to-widget-cb",
					 blink_to_widget,
					 "conv-from-widget-cb",
					 blink_from_widget, NULL);

	/* Ergonomics */
	monitor = g_find_program_in_path ("mate-typing-monitor");
	if (monitor != NULL) {
		g_free (monitor);

		peditor = mateconf_peditor_new_boolean
			(changeset, "/desktop/mate/typing_break/enabled",
	     		WID ("break_enabled_toggle"), NULL);
		mateconf_peditor_widget_set_guard (MATECONF_PROPERTY_EDITOR (peditor),
						WID ("break_details_table"));
		mateconf_peditor_new_numeric_range (changeset,
						 "/desktop/mate/typing_break/type_time",
						 WID ("break_enabled_spin"), NULL);
		mateconf_peditor_new_numeric_range (changeset,
						 "/desktop/mate/typing_break/break_time",
						 WID ("break_interval_spin"),
						 NULL);
		mateconf_peditor_new_boolean (changeset,
					   "/desktop/mate/typing_break/allow_postpone",
					   WID ("break_postponement_toggle"),
					   NULL);

	} else {
		/* don't show the typing break tab if the daemon is not available */
		GtkNotebook *nb = GTK_NOTEBOOK (WID ("keyboard_notebook"));
		gint tb_page = gtk_notebook_page_num (nb, WID ("break_enabled_toggle"));
		gtk_notebook_remove_page (nb, tb_page);
	}

	g_signal_connect (WID ("keyboard_dialog"), "response",
			  (GCallback) dialog_response, changeset);

	setup_xkb_tabs (dialog, changeset);
	setup_a11y_tabs (dialog, changeset);
}

int
main (int argc, char **argv)
{
	MateConfClient *client;
	MateConfChangeSet *changeset;
	GtkBuilder *dialog;
	GOptionContext *context;

	static gboolean apply_only = FALSE;
	static gboolean switch_to_typing_break_page = FALSE;
	static gboolean switch_to_a11y_page = FALSE;

	static GOptionEntry cap_options[] = {
		{"apply", 0, 0, G_OPTION_ARG_NONE, &apply_only,
		 N_
		 ("Just apply settings and quit (compatibility only; now handled by daemon)"),
		 NULL},
		{"init-session-settings", 0, 0, G_OPTION_ARG_NONE,
		 &apply_only,
		 N_
		 ("Just apply settings and quit (compatibility only; now handled by daemon)"),
		 NULL},
		{"typing-break", 0, 0, G_OPTION_ARG_NONE,
		 &switch_to_typing_break_page,
		 N_
		 ("Start the page with the typing break settings showing"),
		 NULL},
		{"a11y", 0, 0, G_OPTION_ARG_NONE,
		 &switch_to_a11y_page,
		 N_
		 ("Start the page with the accessibility settings showing"),
		 NULL},
		{NULL}
	};


	context = g_option_context_new (_("- MATE Keyboard Preferences"));
	g_option_context_add_main_entries (context, cap_options,
					   GETTEXT_PACKAGE);

	capplet_init (context, &argc, &argv);

	activate_settings_daemon ();

	client = mateconf_client_get_default ();
	mateconf_client_add_dir (client,
			      "/desktop/mate/peripherals/keyboard",
			      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_add_dir (client, "/desktop/mate/interface",
			      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	g_object_unref (client);

	changeset = NULL;
	dialog = create_dialog ();
	setup_dialog (dialog, changeset);
	if (switch_to_typing_break_page) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK
					       (WID
						("keyboard_notebook")),
					       4);
	}
	else if (switch_to_a11y_page) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK
					       (WID
						("keyboard_notebook")),
					       2);

	}

	capplet_set_icon (WID ("keyboard_dialog"),
			  "preferences-desktop-keyboard");
	gtk_widget_show (WID ("keyboard_dialog"));
	gtk_main ();

	return 0;
}
