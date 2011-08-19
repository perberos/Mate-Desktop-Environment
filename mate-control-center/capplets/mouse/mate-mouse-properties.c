/* -*- mode: c; style: linux -*- */

/* mouse-properties-capplet.c
 * Copyright (C) 2001 Red Hat, Inc.
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by: Jonathon Blandford <jrb@redhat.com>,
 *             Bradford Hovinen <hovinen@ximian.com>,
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

#include <config.h>

#include <glib/gi18n.h>
#include <string.h>
#include <mateconf/mateconf-client.h>
#include <gdk/gdkx.h>
#include <math.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"
#include "activate-settings-daemon.h"
#include "capplet-stock-icons.h"
#include "mate-mouse-accessibility.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef HAVE_XINPUT
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#endif

#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

enum
{
	DOUBLE_CLICK_TEST_OFF,
	DOUBLE_CLICK_TEST_MAYBE,
	DOUBLE_CLICK_TEST_ON
};

/* We use this in at least half a dozen places, so it makes sense just to
 * define the macro */

#define DOUBLE_CLICK_KEY "/desktop/mate/peripherals/mouse/double_click"

/* State in testing the double-click speed. Global for a great deal of
 * convenience
 */
static gint double_click_state = DOUBLE_CLICK_TEST_OFF;

/* normalization routines */
/* All of our scales but double_click are on the range 1->10 as a result, we
 * have a few routines to convert from whatever the mateconf key is to our range.
 */
static MateConfValue *
double_click_from_mateconf (MateConfPropertyEditor *peditor, const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_INT);
	mateconf_value_set_int (new_value, CLAMP ((int) floor ((mateconf_value_get_int (value) + 50) / 100.0) * 100, 100, 1000));
	return new_value;
}

static void
get_default_mouse_info (int *default_numerator, int *default_denominator, int *default_threshold)
{
	int numerator, denominator;
	int threshold;
	int tmp_num, tmp_den, tmp_threshold;

	/* Query X for the default value */
	XGetPointerControl (GDK_DISPLAY (), &numerator, &denominator,
			    &threshold);
	XChangePointerControl (GDK_DISPLAY (), True, True, -1, -1, -1);
	XGetPointerControl (GDK_DISPLAY (), &tmp_num, &tmp_den, &tmp_threshold);
	XChangePointerControl (GDK_DISPLAY (), True, True, numerator, denominator, threshold);

	if (default_numerator)
		*default_numerator = tmp_num;

	if (default_denominator)
		*default_denominator = tmp_den;

	if (default_threshold)
		*default_threshold = tmp_threshold;

}

static MateConfValue *
motion_acceleration_from_mateconf (MateConfPropertyEditor *peditor,
				const MateConfValue *value)
{
	MateConfValue *new_value;
	gfloat motion_acceleration;

	new_value = mateconf_value_new (MATECONF_VALUE_FLOAT);

	if (mateconf_value_get_float (value) == -1.0) {
		int numerator, denominator;

		get_default_mouse_info (&numerator, &denominator, NULL);

		motion_acceleration = CLAMP ((gfloat)(numerator / denominator), 0.2, 6.0);
	}
	else {
		motion_acceleration = CLAMP (mateconf_value_get_float (value), 0.2, 6.0);
	}

	if (motion_acceleration >= 1)
		mateconf_value_set_float (new_value, motion_acceleration + 4);
	else
		mateconf_value_set_float (new_value, motion_acceleration * 5);

	return new_value;
}

static MateConfValue *
motion_acceleration_to_mateconf (MateConfPropertyEditor *peditor,
			      const MateConfValue *value)
{
	MateConfValue *new_value;
	gfloat motion_acceleration;

	new_value = mateconf_value_new (MATECONF_VALUE_FLOAT);
	motion_acceleration = CLAMP (mateconf_value_get_float (value), 1.0, 10.0);

	if (motion_acceleration < 5)
		mateconf_value_set_float (new_value, motion_acceleration / 5.0);
	else
		mateconf_value_set_float (new_value, motion_acceleration - 4);

	return new_value;
}

static MateConfValue *
threshold_from_mateconf (MateConfPropertyEditor *peditor,
		      const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_FLOAT);

	if (mateconf_value_get_int (value) == -1) {
		int threshold;

		get_default_mouse_info (NULL, NULL, &threshold);
		mateconf_value_set_float (new_value, CLAMP (threshold, 1, 10));
	}
	else {
		mateconf_value_set_float (new_value, CLAMP (mateconf_value_get_int (value), 1, 10));
	}

	return new_value;
}

static MateConfValue *
drag_threshold_from_mateconf (MateConfPropertyEditor *peditor,
			   const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_FLOAT);

	mateconf_value_set_float (new_value, CLAMP (mateconf_value_get_int (value), 1, 10));

	return new_value;
}

/* Double Click handling */

struct test_data_t
{
	gint *timeout_id;
	GtkWidget *image;
};

/* Timeout for the double click test */

static gboolean
test_maybe_timeout (struct test_data_t *data)
{
	double_click_state = DOUBLE_CLICK_TEST_OFF;

	gtk_image_set_from_stock (GTK_IMAGE (data->image),
				  MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size());

	*data->timeout_id = 0;

	return FALSE;
}

/* Callback issued when the user clicks the double click testing area. */

static gboolean
event_box_button_press_event (GtkWidget   *widget,
			      GdkEventButton *event,
			      MateConfChangeSet *changeset)
{
	gint                       double_click_time;
	MateConfValue                *value;
	static struct test_data_t  data;
	static gint                test_on_timeout_id     = 0;
	static gint                test_maybe_timeout_id  = 0;
	static guint32             double_click_timestamp = 0;
	GtkWidget                 *image;
	MateConfClient               *client;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	image = g_object_get_data (G_OBJECT (widget), "image");

	if (!(changeset && mateconf_change_set_check_value (changeset, DOUBLE_CLICK_KEY, &value))) {
		client = mateconf_client_get_default();
		double_click_time = mateconf_client_get_int (client, DOUBLE_CLICK_KEY, NULL);
		g_object_unref (client);

	} else
		double_click_time = mateconf_value_get_int (value);

	if (test_maybe_timeout_id != 0)
		g_source_remove  (test_maybe_timeout_id);
	if (test_on_timeout_id != 0)
		g_source_remove (test_on_timeout_id);

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_OFF:
		double_click_state = DOUBLE_CLICK_TEST_MAYBE;
		data.image = image;
		data.timeout_id = &test_maybe_timeout_id;
		test_maybe_timeout_id = g_timeout_add (double_click_time, (GtkFunction) test_maybe_timeout, &data);
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		if (event->time - double_click_timestamp < double_click_time) {
			double_click_state = DOUBLE_CLICK_TEST_ON;
			data.image = image;
			data.timeout_id = &test_on_timeout_id;
			test_on_timeout_id = g_timeout_add (2500, (GtkFunction) test_maybe_timeout, &data);
		}
		break;
	case DOUBLE_CLICK_TEST_ON:
		double_click_state = DOUBLE_CLICK_TEST_OFF;
		break;
	}

	double_click_timestamp = event->time;

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_ON:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_ON, mouse_capplet_dblclck_icon_get_size());
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_MAYBE, mouse_capplet_dblclck_icon_get_size());
		break;
	case DOUBLE_CLICK_TEST_OFF:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size());
		break;
	}

	return TRUE;
}

static void
orientation_radio_button_release_event (GtkWidget   *widget,
				        GdkEventButton *event)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
}

static MateConfValue *
left_handed_from_mateconf (MateConfPropertyEditor *peditor,
			const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_INT);

	mateconf_value_set_int (new_value, mateconf_value_get_bool (value));

	return new_value;
}

static MateConfValue *
left_handed_to_mateconf (MateConfPropertyEditor *peditor,
		      const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_BOOL);

	mateconf_value_set_bool (new_value, mateconf_value_get_int (value) == 1);

	return new_value;
}

static void
scrollmethod_changed_event (MateConfPropertyEditor *peditor,
			    const gchar *key,
			    const MateConfValue *value,
			    GtkBuilder *dialog)
{
	GtkToggleButton *disabled = GTK_TOGGLE_BUTTON (WID ("scroll_disabled_radio"));

	gtk_widget_set_sensitive (WID ("horiz_scroll_toggle"),
				  !gtk_toggle_button_get_active (disabled));
}

static void
synaptics_check_capabilities (GtkBuilder *dialog)
{
#ifdef HAVE_XINPUT
	int numdevices, i;
	XDeviceInfo *devicelist;
	Atom realtype, prop;
	int realformat;
	unsigned long nitems, bytes_after;
	unsigned char *data;

	prop = XInternAtom (GDK_DISPLAY (), "Synaptics Capabilities", True);
	if (!prop)
		return;

	devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
	for (i = 0; i < numdevices; i++) {
		if (devicelist[i].use != IsXExtensionPointer)
			continue;

		gdk_error_trap_push ();
		XDevice *device = XOpenDevice (GDK_DISPLAY (),
					       devicelist[i].id);
		if (gdk_error_trap_pop ())
			continue;

		gdk_error_trap_push ();
		if ((XGetDeviceProperty (GDK_DISPLAY (), device, prop, 0, 2, False,
					 XA_INTEGER, &realtype, &realformat, &nitems,
					 &bytes_after, &data) == Success) && (realtype != None)) {
			/* Property data is booleans for has_left, has_middle,
			 * has_right, has_double, has_triple */
			if (!data[0]) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WID ("tap_to_click_toggle")), TRUE);
				gtk_widget_set_sensitive (WID ("tap_to_click_toggle"), FALSE);
			}

			if (!data[3])
				gtk_widget_set_sensitive (WID ("scroll_twofinger_radio"), FALSE);

			XFree (data);
		}
		gdk_error_trap_pop ();

		XCloseDevice (GDK_DISPLAY (), device);
	}
	XFreeDeviceList (devicelist);
#endif
}

static gboolean
find_synaptics (void)
{
	gboolean ret = FALSE;
#ifdef HAVE_XINPUT
	int numdevices, i;
	XDeviceInfo *devicelist;
	Atom realtype, prop;
	int realformat;
	unsigned long nitems, bytes_after;
	unsigned char *data;
	XExtensionVersion *version;

	/* Input device properties require version 1.5 or higher */
	version = XGetExtensionVersion (GDK_DISPLAY (), "XInputExtension");
	if (!version->present ||
		(version->major_version * 1000 + version->minor_version) < 1005) {
		XFree (version);
		return False;
	}

	prop = XInternAtom (GDK_DISPLAY (), "Synaptics Off", True);
	if (!prop)
		return False;

	devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
	for (i = 0; i < numdevices; i++) {
		if (devicelist[i].use != IsXExtensionPointer)
			continue;

		gdk_error_trap_push();
		XDevice *device = XOpenDevice (GDK_DISPLAY (),
					       devicelist[i].id);
		if (gdk_error_trap_pop ())
			continue;

		gdk_error_trap_push ();
		if ((XGetDeviceProperty (GDK_DISPLAY (), device, prop, 0, 1, False,
					 XA_INTEGER, &realtype, &realformat, &nitems,
					 &bytes_after, &data) == Success) && (realtype != None)) {
			XFree (data);
			ret = TRUE;
		}
		gdk_error_trap_pop ();

		XCloseDevice (GDK_DISPLAY (), device);

		if (ret)
			break;
	}

	XFree (version);
	XFreeDeviceList (devicelist);
#endif
	return ret;
}

/* Set up the property editors in the dialog. */
static void
setup_dialog (GtkBuilder *dialog, MateConfChangeSet *changeset)
{
	GtkRadioButton    *radio;
	GObject           *peditor;

	/* Orientation radio buttons */
	radio = GTK_RADIO_BUTTON (WID ("left_handed_radio"));
	peditor = mateconf_peditor_new_select_radio
		(changeset, "/desktop/mate/peripherals/mouse/left_handed", gtk_radio_button_get_group (radio),
		 "conv-to-widget-cb", left_handed_from_mateconf,
		 "conv-from-widget-cb", left_handed_to_mateconf,
		 NULL);
	/* explicitly connect to button-release so that you can change orientation with either button */
	g_signal_connect (WID ("right_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);
	g_signal_connect (WID ("left_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);

	/* Locate pointer toggle */
	peditor = mateconf_peditor_new_boolean
		(changeset, "/desktop/mate/peripherals/mouse/locate_pointer", WID ("locate_pointer_toggle"), NULL);

	/* Double-click time */
	peditor = mateconf_peditor_new_numeric_range
		(changeset, DOUBLE_CLICK_KEY, WID ("delay_scale"),
		 "conv-to-widget-cb", double_click_from_mateconf,
		 NULL);
	gtk_image_set_from_stock (GTK_IMAGE (WID ("double_click_image")), MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size ());
	g_object_set_data (G_OBJECT (WID ("double_click_eventbox")), "image", WID ("double_click_image"));
	g_signal_connect (WID ("double_click_eventbox"), "button_press_event",
			  G_CALLBACK (event_box_button_press_event), changeset);

	/* speed */
      	mateconf_peditor_new_numeric_range
		(changeset, "/desktop/mate/peripherals/mouse/motion_acceleration", WID ("accel_scale"),
		 "conv-to-widget-cb", motion_acceleration_from_mateconf,
		 "conv-from-widget-cb", motion_acceleration_to_mateconf,
		 NULL);

	mateconf_peditor_new_numeric_range
		(changeset, "/desktop/mate/peripherals/mouse/motion_threshold", WID ("sensitivity_scale"),
		 "conv-to-widget-cb", threshold_from_mateconf,
		 NULL);

	/* DnD threshold */
	mateconf_peditor_new_numeric_range
		(changeset, "/desktop/mate/peripherals/mouse/drag_threshold", WID ("drag_threshold_scale"),
		 "conv-to-widget-cb", drag_threshold_from_mateconf,
		 NULL);

	/* Trackpad page */
	if (find_synaptics () == FALSE)
		gtk_notebook_remove_page (GTK_NOTEBOOK (WID ("prefs_widget")), -1);
	else {
		mateconf_peditor_new_boolean
			(changeset, "/desktop/mate/peripherals/touchpad/disable_while_typing", WID ("disable_w_typing_toggle"), NULL);
		mateconf_peditor_new_boolean
			(changeset, "/desktop/mate/peripherals/touchpad/tap_to_click", WID ("tap_to_click_toggle"), NULL);
		mateconf_peditor_new_boolean
			(changeset, "/desktop/mate/peripherals/touchpad/horiz_scroll_enabled", WID ("horiz_scroll_toggle"), NULL);
		radio = GTK_RADIO_BUTTON (WID ("scroll_disabled_radio"));
		peditor = mateconf_peditor_new_select_radio
			(changeset, "/desktop/mate/peripherals/touchpad/scroll_method", gtk_radio_button_get_group (radio),
			 NULL);

		synaptics_check_capabilities (dialog);
		scrollmethod_changed_event (MATECONF_PROPERTY_EDITOR (peditor), NULL, NULL, dialog);
		g_signal_connect (peditor, "value-changed",
				  G_CALLBACK (scrollmethod_changed_event), dialog);
	}

}

/* Construct the dialog */

static GtkBuilder *
create_dialog (void)
{
	GtkBuilder   *dialog;
	GtkSizeGroup *size_group;
	GError       *error = NULL;

	dialog = gtk_builder_new ();
	gtk_builder_add_from_file (dialog, MATECC_UI_DIR "/mate-mouse-properties.ui", &error);
	if (error != NULL) {
		g_warning ("Error loading UI file: %s", error->message);
		return NULL;
	}

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_fast_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_high_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_large_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_long_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_slow_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_low_label"));
	gtk_size_group_add_widget (size_group, WID ("threshold_small_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_short_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_short_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_short_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_small_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_long_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_long_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_large_label"));

	return dialog;
}

/* Callback issued when a button is clicked on the dialog */

static void
dialog_response_cb (GtkDialog *dialog, gint response_id, MateConfChangeSet *changeset)
{
	if (response_id == GTK_RESPONSE_HELP)
		capplet_help (GTK_WINDOW (dialog),
			      "goscustperiph-5");
	else
		gtk_main_quit ();
}

int
main (int argc, char **argv)
{
	MateConfClient    *client;
	GtkBuilder     *dialog;
	GtkWidget      *dialog_win, *w;
	gchar *start_page = NULL;

	GOptionContext *context;
	GOptionEntry cap_options[] = {
		{"show-page", 'p', G_OPTION_FLAG_IN_MAIN,
		 G_OPTION_ARG_STRING,
		 &start_page,
		 /* TRANSLATORS: don't translate the terms in brackets */
		 N_("Specify the name of the page to show (general|accessibility)"),
		 N_("page") },
		{NULL}
	};

	context = g_option_context_new (_("- MATE Mouse Preferences"));
	g_option_context_add_main_entries (context, cap_options, GETTEXT_PACKAGE);
	capplet_init (context, &argc, &argv);

	capplet_init_stock_icons ();

	activate_settings_daemon ();

	client = mateconf_client_get_default ();
	mateconf_client_add_dir (client, "/desktop/mate/peripherals/mouse", MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_add_dir (client, "/desktop/mate/peripherals/touchpad", MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	dialog = create_dialog ();

	if (dialog) {
		setup_dialog (dialog, NULL);
		setup_accessibility (dialog, client);

		dialog_win = WID ("mouse_properties_dialog");
		g_signal_connect (dialog_win, "response",
				  G_CALLBACK (dialog_response_cb), NULL);

		if (start_page != NULL) {
			gchar *page_name;

			page_name = g_strconcat (start_page, "_vbox", NULL);
			g_free (start_page);

			w = WID (page_name);
			if (w != NULL) {
				GtkNotebook *nb;
				gint pindex;

				nb = GTK_NOTEBOOK (WID ("prefs_widget"));
				pindex = gtk_notebook_page_num (nb, w);
				if (pindex != -1)
					gtk_notebook_set_current_page (nb, pindex);
			}
			g_free (page_name);
		}

		capplet_set_icon (dialog_win, "input-mouse");
		gtk_widget_show (dialog_win);

		gtk_main ();

		g_object_unref (dialog);
	}

	g_object_unref (client);

	return 0;
}
