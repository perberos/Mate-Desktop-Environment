/* Monitor Settings. A preference panel for configuring monitors
 *
 * Copyright (C) 2007, 2008  Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Soren Sandmann <sandmann@redhat.com>
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <gtk/gtk.h>
#include "scrollarea.h"
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmateui/mate-rr.h>
#include <libmateui/mate-rr-config.h>
#include <libmateui/mate-rr-labeler.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

typedef struct App App;
typedef struct GrabInfo GrabInfo;

struct App
{
    MateRRScreen       *screen;
    MateRRConfig  *current_configuration;
    MateRRLabeler *labeler;
    MateOutputInfo         *current_output;

    GtkWidget	   *dialog;
    GtkWidget      *current_monitor_event_box;
    GtkWidget      *current_monitor_label;
    GtkWidget      *monitor_on_radio;
    GtkWidget      *monitor_off_radio;
    GtkListStore   *resolution_store;
    GtkWidget	   *resolution_combo;
    GtkWidget	   *refresh_combo;
    GtkWidget	   *rotation_combo;
    GtkWidget	   *panel_checkbox;
    GtkWidget	   *clone_checkbox;
    GtkWidget	   *show_icon_checkbox;

    /* We store the event timestamp when the Apply button is clicked */
    GtkWidget      *apply_button;
    guint32         apply_button_clicked_timestamp;

    GtkWidget      *area;
    gboolean	    ignore_gui_changes;
    MateConfClient	   *client;

    /* These are used while we are waiting for the ApplyConfiguration method to be executed over D-bus */
    DBusGConnection *connection;
    DBusGProxy *proxy;
    DBusGProxyCall *proxy_call;

    enum {
	APPLYING_VERSION_1,
	APPLYING_VERSION_2
    } apply_configuration_state;
};

/* Response codes for custom buttons in the main dialog */
enum {
    RESPONSE_MAKE_DEFAULT = 1
};

static void rebuild_gui (App *app);
static void on_clone_changed (GtkWidget *box, gpointer data);
static void on_rate_changed (GtkComboBox *box, gpointer data);
static gboolean output_overlaps (MateOutputInfo *output, MateRRConfig *config);
static void select_current_output_from_dialog_position (App *app);
static void monitor_on_off_toggled_cb (GtkToggleButton *toggle, gpointer data);
static void get_geometry (MateOutputInfo *output, int *w, int *h);
static void apply_configuration_returned_cb (DBusGProxy *proxy, DBusGProxyCall *call_id, void *data);
static gboolean get_clone_size (MateRRScreen *screen, int *width, int *height);
static gboolean output_info_supports_mode (App *app, MateOutputInfo *info, int width, int height);

static void
error_message (App *app, const char *primary_text, const char *secondary_text)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new ((app && app->dialog) ? GTK_WINDOW (app->dialog) : NULL,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_CLOSE,
				     "%s", primary_text);

    if (secondary_text)
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_text);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static gboolean
do_free (gpointer data)
{
    g_free (data);
    return FALSE;
}

static gchar *
idle_free (gchar *s)
{
    g_idle_add (do_free, s);

    return s;
}

static void
on_screen_changed (MateRRScreen *scr,
		   gpointer data)
{
    MateRRConfig *current;
    App *app = data;

    current = mate_rr_config_new_current (app->screen);

    if (app->current_configuration)
	mate_rr_config_free (app->current_configuration);

    app->current_configuration = current;
    app->current_output = NULL;

    if (app->labeler) {
	mate_rr_labeler_hide (app->labeler);
	g_object_unref (app->labeler);
    }

    app->labeler = mate_rr_labeler_new (app->current_configuration);

    select_current_output_from_dialog_position (app);
}

static void
on_viewport_changed (FooScrollArea *scroll_area,
		     GdkRectangle  *old_viewport,
		     GdkRectangle  *new_viewport)
{
    foo_scroll_area_set_size (scroll_area,
			      new_viewport->width,
			      new_viewport->height);

    foo_scroll_area_invalidate (scroll_area);
}

static void
layout_set_font (PangoLayout *layout, const char *font)
{
    PangoFontDescription *desc =
	pango_font_description_from_string (font);

    if (desc)
    {
	pango_layout_set_font_description (layout, desc);

	pango_font_description_free (desc);
    }
}

static void
clear_combo (GtkWidget *widget)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    GtkListStore *store = GTK_LIST_STORE (model);

    gtk_list_store_clear (store);
}

typedef struct
{
    const char *text;
    gboolean found;
    GtkTreeIter iter;
} ForeachInfo;

static gboolean
foreach (GtkTreeModel *model,
	 GtkTreePath *path,
	 GtkTreeIter *iter,
	 gpointer data)
{
    ForeachInfo *info = data;
    char *text = NULL;

    gtk_tree_model_get (model, iter, 0, &text, -1);

    g_assert (text != NULL);

    if (strcmp (info->text, text) == 0)
    {
	info->found = TRUE;
	info->iter = *iter;
	return TRUE;
    }

    return FALSE;
}

static void
add_key (GtkWidget *widget,
	 const char *text,
	 int width, int height, int rate,
	 MateRRRotation rotation)
{
    ForeachInfo info;
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    GtkListStore *store = GTK_LIST_STORE (model);
    gboolean retval;

    info.text = text;
    info.found = FALSE;

    gtk_tree_model_foreach (model, foreach, &info);

    if (!info.found)
    {
	GtkTreeIter iter;
	gtk_list_store_insert_with_values (store, &iter, -1,
                                           0, text,
                                           1, width,
                                           2, height,
                                           3, rate,
                                           4, width * height,
                                           5, rotation,
                                           -1);

	retval = TRUE;
    }
    else
    {
	retval = FALSE;
    }
}

static gboolean
combo_select (GtkWidget *widget, const char *text)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkTreeModel *model = gtk_combo_box_get_model (box);
    ForeachInfo info;

    info.text = text;
    info.found = FALSE;

    gtk_tree_model_foreach (model, foreach, &info);

    if (!info.found)
	return FALSE;

    gtk_combo_box_set_active_iter (box, &info.iter);
    return TRUE;
}

static MateRRMode **
get_current_modes (App *app)
{
    MateRROutput *output;

    if (app->current_configuration->clone)
    {
	return mate_rr_screen_list_clone_modes (app->screen);
    }
    else
    {
	if (!app->current_output)
	    return NULL;

	output = mate_rr_screen_get_output_by_name (
	    app->screen, app->current_output->name);

	if (!output)
	    return NULL;

	return mate_rr_output_list_modes (output);
    }
}

static void
rebuild_rotation_combo (App *app)
{
    typedef struct
    {
	MateRRRotation	rotation;
	const char *	name;
    } RotationInfo;
    static const RotationInfo rotations[] = {
	{ MATE_RR_ROTATION_0, N_("Normal") },
	{ MATE_RR_ROTATION_90, N_("Left") },
	{ MATE_RR_ROTATION_270, N_("Right") },
	{ MATE_RR_ROTATION_180, N_("Upside Down") },
    };
    const char *selection;
    MateRRRotation current;
    int i;

    clear_combo (app->rotation_combo);

    gtk_widget_set_sensitive (
	app->rotation_combo, app->current_output && app->current_output->on);

    if (!app->current_output)
	return;

    current = app->current_output->rotation;

    selection = NULL;
    for (i = 0; i < G_N_ELEMENTS (rotations); ++i)
    {
	const RotationInfo *info = &(rotations[i]);

	app->current_output->rotation = info->rotation;

	/* NULL-GError --- FIXME: we should say why this rotation is not available! */
	if (mate_rr_config_applicable (app->current_configuration, app->screen, NULL))
	{
 	    add_key (app->rotation_combo, _(info->name), 0, 0, 0, info->rotation);

	    if (info->rotation == current)
		selection = _(info->name);
	}
    }

    app->current_output->rotation = current;

    if (!(selection && combo_select (app->rotation_combo, selection)))
	combo_select (app->rotation_combo, _("Normal"));
}

static char *
make_rate_string (int hz)
{
    return g_strdup_printf (_("%d Hz"), hz);
}

static void
rebuild_rate_combo (App *app)
{
    GHashTable *rates;
    MateRRMode **modes;
    int best;
    int i;

    clear_combo (app->refresh_combo);

    gtk_widget_set_sensitive (
	app->refresh_combo, app->current_output && app->current_output->on);

    if (!app->current_output
        || !(modes = get_current_modes (app)))
	return;

    rates = g_hash_table_new_full (
	g_str_hash, g_str_equal, (GFreeFunc) g_free, NULL);

    best = -1;
    for (i = 0; modes[i] != NULL; ++i)
    {
	MateRRMode *mode = modes[i];
	int width, height, rate;

	width = mate_rr_mode_get_width (mode);
	height = mate_rr_mode_get_height (mode);
	rate = mate_rr_mode_get_freq (mode);

	if (width == app->current_output->width		&&
	    height == app->current_output->height)
	{
	    add_key (app->refresh_combo,
		     idle_free (make_rate_string (rate)),
		     0, 0, rate, -1);

	    if (rate > best)
		best = rate;
	}
    }

    if (!combo_select (app->refresh_combo, idle_free (make_rate_string (app->current_output->rate))))
	combo_select (app->refresh_combo, idle_free (make_rate_string (best)));
}

static int
count_active_outputs (App *app)
{
    int i, count = 0;

    for (i = 0; app->current_configuration->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output = app->current_configuration->outputs[i];
	if (output->on)
	    count++;
    }

    return count;
}

#if 0
static int
count_all_outputs (MateRRConfig *config)
{
    int i;

    for (i = 0; config->outputs[i] != NULL; i++)
	;

    return i;
}
#endif

/* Computes whether "Mirror Screens" (clone mode) is supported based on these criteria:
 *
 * 1. There is an available size for cloning.
 *
 * 2. There are 2 or more connected outputs that support that size.
 */
static gboolean
mirror_screens_is_supported (App *app)
{
    int clone_width, clone_height;
    gboolean have_clone_size;
    gboolean mirror_is_supported;

    mirror_is_supported = FALSE;

    have_clone_size = get_clone_size (app->screen, &clone_width, &clone_height);

    if (have_clone_size) {
	int i;
	int num_outputs_with_clone_size;

	num_outputs_with_clone_size = 0;

	for (i = 0; app->current_configuration->outputs[i] != NULL; i++)
	{
	    MateOutputInfo *output = app->current_configuration->outputs[i];

	    /* We count the connected outputs that support the clone size.  It
	     * doesn't matter if those outputs aren't actually On currently; we
	     * will turn them on in on_clone_changed().
	     */
	    if (output->connected && output_info_supports_mode (app, output, clone_width, clone_height))
		num_outputs_with_clone_size++;
	}

	if (num_outputs_with_clone_size >= 2)
	    mirror_is_supported = TRUE;
    }

    return mirror_is_supported;
}

static void
rebuild_mirror_screens (App *app)
{
    gboolean mirror_is_active;
    gboolean mirror_is_supported;

    g_signal_handlers_block_by_func (app->clone_checkbox, G_CALLBACK (on_clone_changed), app);

    mirror_is_active = app->current_configuration && app->current_configuration->clone;

    /* If mirror_is_active, then it *must* be possible to turn mirroring off */
    mirror_is_supported = mirror_is_active || mirror_screens_is_supported (app);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->clone_checkbox), mirror_is_active);
    gtk_widget_set_sensitive (app->clone_checkbox, mirror_is_supported);

    g_signal_handlers_unblock_by_func (app->clone_checkbox, G_CALLBACK (on_clone_changed), app);
}

static void
rebuild_current_monitor_label (App *app)
{
	char *str, *tmp;
	GdkColor color;
	gboolean use_color;

	if (app->current_output)
	{
	    if (app->current_configuration->clone)
		tmp = g_strdup (_("Mirror Screens"));
	    else
		tmp = g_strdup_printf (_("Monitor: %s"), app->current_output->display_name);

	    str = g_strdup_printf ("<b>%s</b>", tmp);
	    mate_rr_labeler_get_color_for_output (app->labeler, app->current_output, &color);
	    use_color = TRUE;
	    g_free (tmp);
	}
	else
	{
	    str = g_strdup_printf ("<b>%s</b>", _("Monitor"));
	    use_color = FALSE;
	}

	gtk_label_set_markup (GTK_LABEL (app->current_monitor_label), str);
	g_free (str);

	if (use_color)
	{
	    GdkColor black = { 0, 0, 0, 0 };

	    gtk_widget_modify_bg (app->current_monitor_event_box, gtk_widget_get_state (app->current_monitor_event_box), &color);

	    /* Make the label explicitly black.  We don't want it to follow the
	     * theme's colors, since the label is always shown against a light
	     * pastel background.  See bgo#556050
	     */
	    gtk_widget_modify_fg (app->current_monitor_label, gtk_widget_get_state (app->current_monitor_label), &black);
	}
	else
	{
	    /* Remove any modifications we did on the label's color */
	    GtkRcStyle *reset_rc_style;

	    reset_rc_style = gtk_rc_style_new ();
	    gtk_widget_modify_style (app->current_monitor_label, reset_rc_style); /* takes ownership of, and destroys, the rc style */
	}

	gtk_event_box_set_visible_window (GTK_EVENT_BOX (app->current_monitor_event_box), use_color);
}

static void
rebuild_on_off_radios (App *app)
{
    gboolean sensitive;
    gboolean on_active;
    gboolean off_active;

    g_signal_handlers_block_by_func (app->monitor_on_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_handlers_block_by_func (app->monitor_off_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);

    sensitive = FALSE;
    on_active = FALSE;
    off_active = FALSE;

    if (!app->current_configuration->clone && app->current_output)
    {
	if (count_active_outputs (app) > 1 || !app->current_output->on)
	    sensitive = TRUE;
	else
	    sensitive = FALSE;

	on_active = app->current_output->on;
	off_active = !on_active;
    }

    gtk_widget_set_sensitive (app->monitor_on_radio, sensitive);
    gtk_widget_set_sensitive (app->monitor_off_radio, sensitive);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->monitor_on_radio), on_active);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->monitor_off_radio), off_active);

    g_signal_handlers_unblock_by_func (app->monitor_on_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_handlers_unblock_by_func (app->monitor_off_radio, G_CALLBACK (monitor_on_off_toggled_cb), app);
}

static char *
make_resolution_string (int width, int height)
{
    return g_strdup_printf (_("%d x %d"), width, height);
}

static void
find_best_mode (MateRRMode **modes, int *out_width, int *out_height)
{
    int i;

    *out_width = 0;
    *out_height = 0;

    for (i = 0; modes[i] != NULL; i++)
    {
	int w, h;

	w = mate_rr_mode_get_width (modes[i]);
	h = mate_rr_mode_get_height (modes[i]);

	if (w * h > *out_width * *out_height)
	{
	    *out_width = w;
	    *out_height = h;
	}
    }
}

static void
rebuild_resolution_combo (App *app)
{
    int i;
    MateRRMode **modes;
    const char *current;

    clear_combo (app->resolution_combo);

    if (!(modes = get_current_modes (app))
	|| !app->current_output
	|| !app->current_output->on)
    {
	gtk_widget_set_sensitive (app->resolution_combo, FALSE);
	return;
    }

    g_assert (app->current_output != NULL);
    g_assert (app->current_output->width != 0 && app->current_output->height != 0);

    gtk_widget_set_sensitive (app->resolution_combo, TRUE);

    for (i = 0; modes[i] != NULL; ++i)
    {
	int width, height;

	width = mate_rr_mode_get_width (modes[i]);
	height = mate_rr_mode_get_height (modes[i]);

	add_key (app->resolution_combo,
		 idle_free (make_resolution_string (width, height)),
		 width, height, 0, -1);
    }

    current = idle_free (make_resolution_string (app->current_output->width, app->current_output->height));

    if (!combo_select (app->resolution_combo, current))
    {
	int best_w, best_h;

	find_best_mode (modes, &best_w, &best_h);
	combo_select (app->resolution_combo, idle_free (make_resolution_string (best_w, best_h)));
    }
}

static void
rebuild_gui (App *app)
{
    gboolean sensitive;

    /* We would break spectacularly if we recursed, so
     * just assert if that happens
     */
    g_assert (app->ignore_gui_changes == FALSE);

    app->ignore_gui_changes = TRUE;

    sensitive = app->current_output ? TRUE : FALSE;

#if 0
    g_debug ("rebuild gui, is on: %d", app->current_output->on);
#endif

    rebuild_mirror_screens (app);
    rebuild_current_monitor_label (app);
    rebuild_on_off_radios (app);
    rebuild_resolution_combo (app);
    rebuild_rate_combo (app);
    rebuild_rotation_combo (app);

#if 0
    g_debug ("sensitive: %d, on: %d", sensitive, app->current_output->on);
#endif
    gtk_widget_set_sensitive (app->panel_checkbox, sensitive);

    app->ignore_gui_changes = FALSE;
}

static gboolean
get_mode (GtkWidget *widget, int *width, int *height, int *freq, MateRRRotation *rot)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    int dummy;

    if (!gtk_combo_box_get_active_iter (box, &iter))
	return FALSE;

    if (!width)
	width = &dummy;

    if (!height)
	height = &dummy;

    if (!freq)
	freq = &dummy;

    if (!rot)
	rot = (MateRRRotation *)&dummy;

    model = gtk_combo_box_get_model (box);
    gtk_tree_model_get (model, &iter,
			1, width,
			2, height,
			3, freq,
			5, rot,
			-1);

    return TRUE;

}

static void
on_rotation_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    MateRRRotation rotation;

    if (!app->current_output)
	return;

    if (get_mode (app->rotation_combo, NULL, NULL, NULL, &rotation))
	app->current_output->rotation = rotation;

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
on_rate_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    int rate;

    if (!app->current_output)
	return;

    if (get_mode (app->refresh_combo, NULL, NULL, &rate, NULL))
	app->current_output->rate = rate;

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
select_resolution_for_current_output (App *app)
{
    MateRRMode **modes;
    int width, height;

    if (app->current_output->pref_width != 0 && app->current_output->pref_height != 0)
    {
	app->current_output->width = app->current_output->pref_width;
	app->current_output->height = app->current_output->pref_height;
	return;
    }

    modes = get_current_modes (app);
    if (!modes)
	return;

    find_best_mode (modes, &width, &height);

    app->current_output->width = width;
    app->current_output->height = height;
}

static void
monitor_on_off_toggled_cb (GtkToggleButton *toggle, gpointer data)
{
    App *app = data;
    gboolean is_on;

    if (!app->current_output)
	return;

    if (!gtk_toggle_button_get_active (toggle))
	return;

    if (GTK_WIDGET (toggle) == app->monitor_on_radio)
	is_on = TRUE;
    else if (GTK_WIDGET (toggle) == app->monitor_off_radio)
	is_on = FALSE;
    else
    {
	g_assert_not_reached ();
	return;
    }

    app->current_output->on = is_on;

    if (is_on)
	select_resolution_for_current_output (app); /* The refresh rate will be picked in rebuild_rate_combo() */

    rebuild_gui (app);
    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
realign_outputs_after_resolution_change (App *app, MateOutputInfo *output_that_changed, int old_width, int old_height)
{
    /* We find the outputs that were below or to the right of the output that
     * changed, and realign them; we also do that for outputs that shared the
     * right/bottom edges with the output that changed.  The outputs that are
     * above or to the left of that output don't need to change.
     */

    int i;
    int old_right_edge, old_bottom_edge;
    int dx, dy;

    g_assert (app->current_configuration != NULL);

    if (output_that_changed->width == old_width && output_that_changed->height == old_height)
	return;

    old_right_edge = output_that_changed->x + old_width;
    old_bottom_edge = output_that_changed->y + old_height;

    dx = output_that_changed->width - old_width;
    dy = output_that_changed->height - old_height;

    for (i = 0; app->current_configuration->outputs[i] != NULL; i++) {
	MateOutputInfo *output;
	int output_width, output_height;

	output = app->current_configuration->outputs[i];

	if (output == output_that_changed || !output->connected)
	    continue;

	get_geometry (output, &output_width, &output_height);

	if (output->x >= old_right_edge)
	    output->x += dx;
	else if (output->x + output_width == old_right_edge)
	    output->x = output_that_changed->x + output_that_changed->width - output_width;

	if (output->y >= old_bottom_edge)
	    output->y += dy;
	else if (output->y + output_height == old_bottom_edge)
	    output->y = output_that_changed->y + output_that_changed->height - output_height;
    }
}

static void
on_resolution_changed (GtkComboBox *box, gpointer data)
{
    App *app = data;
    int old_width, old_height;
    int width;
    int height;

    if (!app->current_output)
	return;

    old_width = app->current_output->width;
    old_height = app->current_output->height;

    if (get_mode (app->resolution_combo, &width, &height, NULL, NULL))
    {
	app->current_output->width = width;
	app->current_output->height = height;

	if (width == 0 || height == 0)
	    app->current_output->on = FALSE;
	else
	    app->current_output->on = TRUE;
    }

    realign_outputs_after_resolution_change (app, app->current_output, old_width, old_height);

    rebuild_rate_combo (app);
    rebuild_rotation_combo (app);

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));
}

static void
lay_out_outputs_horizontally (App *app)
{
    int i;
    int x;

    /* Lay out all the monitors horizontally when "mirror screens" is turned
     * off, to avoid having all of them overlapped initially.  We put the
     * outputs turned off on the right-hand side.
     */

    x = 0;

    /* First pass, all "on" outputs */

    for (i = 0; app->current_configuration->outputs[i]; ++i)
    {
	MateOutputInfo *output;

	output = app->current_configuration->outputs[i];
	if (output->connected && output->on) {
	    output->x = x;
	    output->y = 0;
	    x += output->width;
	}
    }

    /* Second pass, all the black screens */

    for (i = 0; app->current_configuration->outputs[i]; ++i)
    {
	MateOutputInfo *output;

	output = app->current_configuration->outputs[i];
	if (!(output->connected && output->on)) {
	    output->x = x;
	    output->y = 0;
	    x += output->width;
	}
    }

}

/* FIXME: this function is copied from mate-settings-daemon/plugins/xrandr/gsd-xrandr-manager.c.
 * Do we need to put this function in mate-desktop for public use?
 */
static gboolean
get_clone_size (MateRRScreen *screen, int *width, int *height)
{
        MateRRMode **modes = mate_rr_screen_list_clone_modes (screen);
        int best_w, best_h;
        int i;

        best_w = 0;
        best_h = 0;

        for (i = 0; modes[i] != NULL; ++i) {
                MateRRMode *mode = modes[i];
                int w, h;

                w = mate_rr_mode_get_width (mode);
                h = mate_rr_mode_get_height (mode);

                if (w * h > best_w * best_h) {
                        best_w = w;
                        best_h = h;
                }
        }

        if (best_w > 0 && best_h > 0) {
                if (width)
                        *width = best_w;
                if (height)
                        *height = best_h;

                return TRUE;
        }

        return FALSE;
}

static gboolean
output_info_supports_mode (App *app, MateOutputInfo *info, int width, int height)
{
    MateRROutput *output;
    MateRRMode **modes;
    int i;

    if (!info->connected)
	return FALSE;

    output = mate_rr_screen_get_output_by_name (app->screen, info->name);
    if (!output)
	return FALSE;

    modes = mate_rr_output_list_modes (output);

    for (i = 0; modes[i]; i++) {
	if (mate_rr_mode_get_width (modes[i]) == width
	    && mate_rr_mode_get_height (modes[i]) == height)
	    return TRUE;
    }

    return FALSE;
}

static void
on_clone_changed (GtkWidget *box, gpointer data)
{
    App *app = data;

    app->current_configuration->clone =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (app->clone_checkbox));

    if (app->current_configuration->clone)
    {
	int i;
	int width, height;

	for (i = 0; app->current_configuration->outputs[i]; ++i)
	{
	    if (app->current_configuration->outputs[i]->connected)
	    {
		app->current_output = app->current_configuration->outputs[i];
		break;
	    }
	}

	/* Turn on all the connected screens that support the best clone mode.
	 * The user may hit "Mirror Screens", but he shouldn't have to turn on
	 * all the required outputs as well.
	 */

	get_clone_size (app->screen, &width, &height);

	for (i = 0; app->current_configuration->outputs[i]; i++) {
	    if (output_info_supports_mode (app, app->current_configuration->outputs[i], width, height)) {
		app->current_configuration->outputs[i]->on = TRUE;
		app->current_configuration->outputs[i]->width = width;
		app->current_configuration->outputs[i]->height = height;
	    }
	}
    }
    else
    {
	if (output_overlaps (app->current_output, app->current_configuration))
	    lay_out_outputs_horizontally (app);
    }

    rebuild_gui (app);
}

static void
get_geometry (MateOutputInfo *output, int *w, int *h)
{
    if (output->on)
    {
	*h = output->height;
	*w = output->width;
    }
    else
    {
	*h = output->pref_height;
	*w = output->pref_width;
    }
   if ((output->rotation & MATE_RR_ROTATION_90) || (output->rotation & MATE_RR_ROTATION_270))
   {
        int tmp;
        tmp = *h;
        *h = *w;
        *w = tmp;
   }
}

#define SPACE 15
#define MARGIN  15

static GList *
list_connected_outputs (App *app, int *total_w, int *total_h)
{
    int i, dummy;
    GList *result = NULL;

    if (!total_w)
	total_w = &dummy;
    if (!total_h)
	total_h = &dummy;

    *total_w = 0;
    *total_h = 0;
    for (i = 0; app->current_configuration->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output = app->current_configuration->outputs[i];

	if (output->connected)
	{
	    int w, h;

	    result = g_list_prepend (result, output);

	    get_geometry (output, &w, &h);

	    *total_w += w;
	    *total_h += h;
	}
    }

    return g_list_reverse (result);
}

static int
get_n_connected (App *app)
{
    GList *connected_outputs = list_connected_outputs (app, NULL, NULL);
    int n = g_list_length (connected_outputs);

    g_list_free (connected_outputs);

    return n;
}

static double
compute_scale (App *app)
{
    int available_w, available_h;
    int total_w, total_h;
    int n_monitors;
    GdkRectangle viewport;
    GList *connected_outputs;

    foo_scroll_area_get_viewport (FOO_SCROLL_AREA (app->area), &viewport);

    connected_outputs = list_connected_outputs (app, &total_w, &total_h);

    n_monitors = g_list_length (connected_outputs);

    g_list_free (connected_outputs);

    available_w = viewport.width - 2 * MARGIN - (n_monitors - 1) * SPACE;
    available_h = viewport.height - 2 * MARGIN - (n_monitors - 1) * SPACE;

    return MIN ((double)available_w / total_w, (double)available_h / total_h);
}

typedef struct Edge
{
    MateOutputInfo *output;
    int x1, y1;
    int x2, y2;
} Edge;

typedef struct Snap
{
    Edge *snapper;		/* Edge that should be snapped */
    Edge *snappee;
    int dy, dx;
} Snap;

static void
add_edge (MateOutputInfo *output, int x1, int y1, int x2, int y2, GArray *edges)
{
    Edge e;

    e.x1 = x1;
    e.x2 = x2;
    e.y1 = y1;
    e.y2 = y2;
    e.output = output;

    g_array_append_val (edges, e);
}

static void
list_edges_for_output (MateOutputInfo *output, GArray *edges)
{
    int x, y, w, h;

    x = output->x;
    y = output->y;
    get_geometry (output, &w, &h);

    /* Top, Bottom, Left, Right */
    add_edge (output, x, y, x + w, y, edges);
    add_edge (output, x, y + h, x + w, y + h, edges);
    add_edge (output, x, y, x, y + h, edges);
    add_edge (output, x + w, y, x + w, y + h, edges);
}

static void
list_edges (MateRRConfig *config, GArray *edges)
{
    int i;

    for (i = 0; config->outputs[i]; ++i)
    {
	MateOutputInfo *output = config->outputs[i];

	if (output->connected)
	    list_edges_for_output (output, edges);
    }
}

static gboolean
overlap (int s1, int e1, int s2, int e2)
{
    return (!(e1 < s2 || s1 >= e2));
}

static gboolean
horizontal_overlap (Edge *snapper, Edge *snappee)
{
    if (snapper->y1 != snapper->y2 || snappee->y1 != snappee->y2)
	return FALSE;

    return overlap (snapper->x1, snapper->x2, snappee->x1, snappee->x2);
}

static gboolean
vertical_overlap (Edge *snapper, Edge *snappee)
{
    if (snapper->x1 != snapper->x2 || snappee->x1 != snappee->x2)
	return FALSE;

    return overlap (snapper->y1, snapper->y2, snappee->y1, snappee->y2);
}

static void
add_snap (GArray *snaps, Snap snap)
{
    if (ABS (snap.dx) <= 200 || ABS (snap.dy) <= 200)
	g_array_append_val (snaps, snap);
}

static void
add_edge_snaps (Edge *snapper, Edge *snappee, GArray *snaps)
{
    Snap snap;

    snap.snapper = snapper;
    snap.snappee = snappee;

    if (horizontal_overlap (snapper, snappee))
    {
	snap.dx = 0;
	snap.dy = snappee->y1 - snapper->y1;

	add_snap (snaps, snap);
    }
    else if (vertical_overlap (snapper, snappee))
    {
	snap.dy = 0;
	snap.dx = snappee->x1 - snapper->x1;

	add_snap (snaps, snap);
    }

    /* Corner snaps */
    /* 1->1 */
    snap.dx = snappee->x1 - snapper->x1;
    snap.dy = snappee->y1 - snapper->y1;

    add_snap (snaps, snap);

    /* 1->2 */
    snap.dx = snappee->x2 - snapper->x1;
    snap.dy = snappee->y2 - snapper->y1;

    add_snap (snaps, snap);

    /* 2->2 */
    snap.dx = snappee->x2 - snapper->x2;
    snap.dy = snappee->y2 - snapper->y2;

    add_snap (snaps, snap);

    /* 2->1 */
    snap.dx = snappee->x1 - snapper->x2;
    snap.dy = snappee->y1 - snapper->y2;

    add_snap (snaps, snap);
}

static void
list_snaps (MateOutputInfo *output, GArray *edges, GArray *snaps)
{
    int i;

    for (i = 0; i < edges->len; ++i)
    {
	Edge *output_edge = &(g_array_index (edges, Edge, i));

	if (output_edge->output == output)
	{
	    int j;

	    for (j = 0; j < edges->len; ++j)
	    {
		Edge *edge = &(g_array_index (edges, Edge, j));

		if (edge->output != output)
		    add_edge_snaps (output_edge, edge, snaps);
	    }
	}
    }
}

#if 0
static void
print_edge (Edge *edge)
{
    g_debug ("(%d %d %d %d)", edge->x1, edge->y1, edge->x2, edge->y2);
}
#endif

static gboolean
corner_on_edge (int x, int y, Edge *e)
{
    if (x == e->x1 && x == e->x2 && y >= e->y1 && y <= e->y2)
	return TRUE;

    if (y == e->y1 && y == e->y2 && x >= e->x1 && x <= e->x2)
	return TRUE;

    return FALSE;
}

static gboolean
edges_align (Edge *e1, Edge *e2)
{
    if (corner_on_edge (e1->x1, e1->y1, e2))
	return TRUE;

    if (corner_on_edge (e2->x1, e2->y1, e1))
	return TRUE;

    return FALSE;
}

static gboolean
output_is_aligned (MateOutputInfo *output, GArray *edges)
{
    gboolean result = FALSE;
    int i;

    for (i = 0; i < edges->len; ++i)
    {
	Edge *output_edge = &(g_array_index (edges, Edge, i));

	if (output_edge->output == output)
	{
	    int j;

	    for (j = 0; j < edges->len; ++j)
	    {
		Edge *edge = &(g_array_index (edges, Edge, j));

		/* We are aligned if an output edge matches
		 * an edge of another output
		 */
		if (edge->output != output_edge->output)
		{
		    if (edges_align (output_edge, edge))
		    {
			result = TRUE;
			goto done;
		    }
		}
	    }
	}
    }
done:

    return result;
}

static void
get_output_rect (MateOutputInfo *output, GdkRectangle *rect)
{
    int w, h;

    get_geometry (output, &w, &h);

    rect->width = w;
    rect->height = h;
    rect->x = output->x;
    rect->y = output->y;
}

static gboolean
output_overlaps (MateOutputInfo *output, MateRRConfig *config)
{
    int i;
    GdkRectangle output_rect;

    get_output_rect (output, &output_rect);

    for (i = 0; config->outputs[i]; ++i)
    {
	MateOutputInfo *other = config->outputs[i];

	if (other != output && other->connected)
	{
	    GdkRectangle other_rect;

	    get_output_rect (other, &other_rect);
	    if (gdk_rectangle_intersect (&output_rect, &other_rect, NULL))
		return TRUE;
	}
    }

    return FALSE;
}

static gboolean
mate_rr_config_is_aligned (MateRRConfig *config, GArray *edges)
{
    int i;
    gboolean result = TRUE;

    for (i = 0; config->outputs[i]; ++i)
    {
	MateOutputInfo *output = config->outputs[i];

	if (output->connected)
	{
	    if (!output_is_aligned (output, edges))
		return FALSE;

	    if (output_overlaps (output, config))
		return FALSE;
	}
    }

    return result;
}

struct GrabInfo
{
    int grab_x;
    int grab_y;
    int output_x;
    int output_y;
};

static gboolean
is_corner_snap (const Snap *s)
{
    return s->dx != 0 && s->dy != 0;
}

static int
compare_snaps (gconstpointer v1, gconstpointer v2)
{
    const Snap *s1 = v1;
    const Snap *s2 = v2;
    int sv1 = MAX (ABS (s1->dx), ABS (s1->dy));
    int sv2 = MAX (ABS (s2->dx), ABS (s2->dy));
    int d;

    d = sv1 - sv2;

    /* This snapping algorithm is good enough for rock'n'roll, but
     * this is probably a better:
     *
     *    First do a horizontal/vertical snap, then
     *    with the new coordinates from that snap,
     *    do a corner snap.
     *
     * Right now, it's confusing that corner snapping
     * depends on the distance in an axis that you can't actually see.
     *
     */
    if (d == 0)
    {
	if (is_corner_snap (s1) && !is_corner_snap (s2))
	    return -1;
	else if (is_corner_snap (s2) && !is_corner_snap (s1))
	    return 1;
	else
	    return 0;
    }
    else
    {
	return d;
    }
}

/* Sets a mouse cursor for a widget's window.  As a hack, you can pass
 * GDK_BLANK_CURSOR to mean "set the cursor to NULL" (i.e. reset the widget's
 * window's cursor to its default).
 */
static void
set_cursor (GtkWidget *widget, GdkCursorType type)
{
	GdkCursor *cursor;
	GdkWindow *window;

	if (type == GDK_BLANK_CURSOR)
	    cursor = NULL;
	else
	    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget), type);

	window = gtk_widget_get_window (widget);

	if (window)
	    gdk_window_set_cursor (window, cursor);

	if (cursor)
	    gdk_cursor_unref (cursor);
}

static void
set_monitors_tooltip (App *app, gboolean is_dragging)
{
    const char *text;

    if (is_dragging)
	text = NULL;
    else
	text = _("Select a monitor to change its properties; drag it to rearrange its placement.");

    gtk_widget_set_tooltip_text (app->area, text);
}

static void
on_output_event (FooScrollArea *area,
		 FooScrollAreaEvent *event,
		 gpointer data)
{
    MateOutputInfo *output = data;
    App *app = g_object_get_data (G_OBJECT (area), "app");

    /* If the mouse is inside the outputs, set the cursor to "you can move me".  See
     * on_canvas_event() for where we reset the cursor to the default if it
     * exits the outputs' area.
     */
    if (!app->current_configuration->clone && get_n_connected (app) > 1)
	set_cursor (GTK_WIDGET (area), GDK_FLEUR);

    if (event->type == FOO_BUTTON_PRESS)
    {
	GrabInfo *info;

	app->current_output = output;

	rebuild_gui (app);
	set_monitors_tooltip (app, TRUE);

	if (!app->current_configuration->clone && get_n_connected (app) > 1)
	{
	    foo_scroll_area_begin_grab (area, on_output_event, data);

	    info = g_new0 (GrabInfo, 1);
	    info->grab_x = event->x;
	    info->grab_y = event->y;
	    info->output_x = output->x;
	    info->output_y = output->y;

	    output->user_data = info;
	}

	foo_scroll_area_invalidate (area);
    }
    else
    {
	if (foo_scroll_area_is_grabbed (area))
	{
	    GrabInfo *info = output->user_data;
	    double scale = compute_scale (app);
	    int old_x, old_y;
	    int new_x, new_y;
	    int i;
	    GArray *edges, *snaps, *new_edges;

	    old_x = output->x;
	    old_y = output->y;
	    new_x = info->output_x + (event->x - info->grab_x) / scale;
	    new_y = info->output_y + (event->y - info->grab_y) / scale;

	    output->x = new_x;
	    output->y = new_y;

	    edges = g_array_new (TRUE, TRUE, sizeof (Edge));
	    snaps = g_array_new (TRUE, TRUE, sizeof (Snap));
	    new_edges = g_array_new (TRUE, TRUE, sizeof (Edge));

	    list_edges (app->current_configuration, edges);
	    list_snaps (output, edges, snaps);

	    g_array_sort (snaps, compare_snaps);

	    output->x = info->output_x;
	    output->y = info->output_y;

	    for (i = 0; i < snaps->len; ++i)
	    {
		Snap *snap = &(g_array_index (snaps, Snap, i));
		GArray *new_edges = g_array_new (TRUE, TRUE, sizeof (Edge));

		output->x = new_x + snap->dx;
		output->y = new_y + snap->dy;

		g_array_set_size (new_edges, 0);
		list_edges (app->current_configuration, new_edges);

		if (mate_rr_config_is_aligned (app->current_configuration, new_edges))
		{
		    g_array_free (new_edges, TRUE);
		    break;
		}
		else
		{
		    output->x = info->output_x;
		    output->y = info->output_y;
		}
	    }

	    g_array_free (new_edges, TRUE);
	    g_array_free (snaps, TRUE);
	    g_array_free (edges, TRUE);

	    if (event->type == FOO_BUTTON_RELEASE)
	    {
		foo_scroll_area_end_grab (area);
		set_monitors_tooltip (app, FALSE);

		g_free (output->user_data);
		output->user_data = NULL;

#if 0
		g_debug ("new position: %d %d %d %d", output->x, output->y, output->width, output->height);
#endif
	    }

	    foo_scroll_area_invalidate (area);
	}
    }
}

static void
on_canvas_event (FooScrollArea *area,
		 FooScrollAreaEvent *event,
		 gpointer data)
{
    /* If the mouse exits the outputs, reset the cursor to the default.  See
     * on_output_event() for where we set the cursor to the movement cursor if
     * it is over one of the outputs.
     */
    set_cursor (GTK_WIDGET (area), GDK_BLANK_CURSOR);
}

static PangoLayout *
get_display_name (App *app,
		  MateOutputInfo *output)
{
    const char *text;

    if (app->current_configuration->clone) {
	/* Translators:  this is the feature where what you see on your laptop's
	 * screen is the same as your external monitor.  Here, "Mirror" is being
	 * used as an adjective, not as a verb.  For example, the Spanish
	 * translation could be "Pantallas en Espejo", *not* "Espejar Pantallas".
	 */
	text = _("Mirror Screens");
    } else
	text = output->display_name;

    return gtk_widget_create_pango_layout (
	GTK_WIDGET (app->area), text);
}

static void
paint_background (FooScrollArea *area,
		  cairo_t       *cr)
{
    GdkRectangle viewport;
    GtkWidget *widget;
    GtkStyle *widget_style;

    widget = GTK_WIDGET (area);

    foo_scroll_area_get_viewport (area, &viewport);
    widget_style = gtk_widget_get_style (widget);

    cairo_set_source_rgb (cr,
                          widget_style->base[GTK_STATE_SELECTED].red / 65535.0,
                          widget_style->base[GTK_STATE_SELECTED].green / 65535.0,
                          widget_style->base[GTK_STATE_SELECTED].blue / 65535.0);

    cairo_rectangle (cr,
		     viewport.x, viewport.y,
		     viewport.width, viewport.height);

    cairo_fill_preserve (cr);

    foo_scroll_area_add_input_from_fill (area, cr, on_canvas_event, NULL);

    cairo_set_source_rgb (cr,
                          widget_style->dark[GTK_STATE_SELECTED].red / 65535.0,
                          widget_style->dark[GTK_STATE_SELECTED].green / 65535.0,
                          widget_style->dark[GTK_STATE_SELECTED].blue / 65535.0);

    cairo_stroke (cr);
}

static void
paint_output (App *app, cairo_t *cr, int i)
{
    int w, h;
    double scale = compute_scale (app);
    double x, y;
    int total_w, total_h;
    GList *connected_outputs = list_connected_outputs (app, &total_w, &total_h);
    MateOutputInfo *output = g_list_nth (connected_outputs, i)->data;
    PangoLayout *layout = get_display_name (app, output);
    PangoRectangle ink_extent, log_extent;
    GdkRectangle viewport;
    GdkColor output_color;
    double r, g, b;
    double available_w;
    double factor;

    cairo_save (cr);

    foo_scroll_area_get_viewport (FOO_SCROLL_AREA (app->area), &viewport);

    get_geometry (output, &w, &h);

#if 0
    g_debug ("%s (%p) geometry %d %d %d", output->name, output,
	     w, h, output->rate);
#endif

    viewport.height -= 2 * MARGIN;
    viewport.width -= 2 * MARGIN;

    x = output->x * scale + MARGIN + (viewport.width - total_w * scale) / 2.0;
    y = output->y * scale + MARGIN + (viewport.height - total_h * scale) / 2.0;

#if 0
    g_debug ("scaled: %f %f", x, y);

    g_debug ("scale: %f", scale);

    g_debug ("%f %f %f %f", x, y, w * scale + 0.5, h * scale + 0.5);
#endif

    cairo_save (cr);

    cairo_translate (cr,
		     x + (w * scale + 0.5) / 2,
		     y + (h * scale + 0.5) / 2);

    /* rotation is already applied in get_geometry */

    if (output->rotation & MATE_RR_REFLECT_X)
	cairo_scale (cr, -1, 1);

    if (output->rotation & MATE_RR_REFLECT_Y)
	cairo_scale (cr, 1, -1);

    cairo_translate (cr,
		     - x - (w * scale + 0.5) / 2,
		     - y - (h * scale + 0.5) / 2);


    cairo_rectangle (cr, x, y, w * scale + 0.5, h * scale + 0.5);
    cairo_clip_preserve (cr);

    mate_rr_labeler_get_color_for_output (app->labeler, output, &output_color);
    r = output_color.red / 65535.0;
    g = output_color.green / 65535.0;
    b = output_color.blue / 65535.0;

    if (!output->on)
    {
	/* If the output is turned off, just darken the selected color */
	r *= 0.2;
	g *= 0.2;
	b *= 0.2;
    }

    cairo_set_source_rgba (cr, r, g, b, 1.0);

    foo_scroll_area_add_input_from_fill (FOO_SCROLL_AREA (app->area),
					 cr, on_output_event, output);
    cairo_fill (cr);

    if (output == app->current_output)
    {
	cairo_rectangle (cr, x + 2, y + 2, w * scale + 0.5 - 4, h * scale + 0.5 - 4);

	cairo_set_line_width (cr, 4);
	cairo_set_source_rgba (cr, 0.33, 0.43, 0.57, 1.0);
	cairo_stroke (cr);
    }

    cairo_rectangle (cr, x + 0.5, y + 0.5, w * scale + 0.5 - 1, h * scale + 0.5 - 1);

    cairo_set_line_width (cr, 1);
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

    cairo_stroke (cr);
    cairo_set_line_width (cr, 2);

    layout_set_font (layout, "Sans Bold 12");
    pango_layout_get_pixel_extents (layout, &ink_extent, &log_extent);

    available_w = w * scale + 0.5 - 6; /* Same as the inner rectangle's width, minus 1 pixel of padding on each side */
    if (available_w < ink_extent.width)
	factor = available_w / ink_extent.width;
    else
	factor = 1.0;

    cairo_move_to (cr,
		   x + ((w * scale + 0.5) - factor * log_extent.width) / 2,
		   y + ((h * scale + 0.5) - factor * log_extent.height) / 2);

    cairo_scale (cr, factor, factor);

    if (output->on)
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    else
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    pango_cairo_show_layout (cr, layout);

    cairo_restore (cr);

    g_object_unref (layout);
}

static void
on_area_paint (FooScrollArea *area,
	       cairo_t	     *cr,
	       GdkRectangle  *extent,
	       GdkRegion     *region,
	       gpointer	      data)
{
    App *app = data;
    double scale;
    GList *connected_outputs = NULL;
    GList *list;

    paint_background (area, cr);

    if (!app->current_configuration)
	return;

    scale = compute_scale (app);
    connected_outputs = list_connected_outputs (app, NULL, NULL);

#if 0
    g_debug ("scale: %f", scale);
#endif

    for (list = connected_outputs; list != NULL; list = list->next)
    {
	paint_output (app, cr, g_list_position (connected_outputs, list));

	if (app->current_configuration->clone)
	    break;
    }
}

static void
make_text_combo (GtkWidget *widget, int sort_column)
{
    GtkComboBox *box = GTK_COMBO_BOX (widget);
    GtkListStore *store = gtk_list_store_new (
	6,
	G_TYPE_STRING,		/* Text */
	G_TYPE_INT,		/* Width */
	G_TYPE_INT,		/* Height */
	G_TYPE_INT,		/* Frequency */
	G_TYPE_INT,		/* Width * Height */
	G_TYPE_INT);		/* Rotation */

    GtkCellRenderer *cell;

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));

    gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
				    "text", 0,
				    NULL);

    if (sort_column != -1)
    {
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      sort_column,
					      GTK_SORT_DESCENDING);
    }
}

static void
compute_virtual_size_for_configuration (MateRRConfig *config, int *ret_width, int *ret_height)
{
    int i;
    int width, height;

    width = height = 0;

    for (i = 0; config->outputs[i] != NULL; i++)
    {
	MateOutputInfo *output;

	output = config->outputs[i];

	if (output->on)
	{
	    width = MAX (width, output->x + output->width);
	    height = MAX (height, output->y + output->height);
	}
    }

    *ret_width = width;
    *ret_height = height;
}

static void
check_required_virtual_size (App *app)
{
    int req_width, req_height;
    int min_width, max_width;
    int min_height, max_height;

    compute_virtual_size_for_configuration (app->current_configuration, &req_width, &req_height);

    mate_rr_screen_get_ranges (app->screen, &min_width, &max_width, &min_height, &max_height);

#if 0
    g_debug ("X Server supports:");
    g_debug ("min_width = %d, max_width = %d", min_width, max_width);
    g_debug ("min_height = %d, max_height = %d", min_height, max_height);

    g_debug ("Requesting size of %dx%d", req_width, req_height);
#endif

    if (!(min_width <= req_width && req_width <= max_width
	  && min_height <= req_height && req_height <= max_height))
    {
	/* FIXME: present a useful dialog, maybe even before the user tries to Apply */
#if 0
	g_debug ("Your X server needs a larger Virtual size!");
#endif
    }
}

static void
begin_version2_apply_configuration (App *app, GdkWindow *parent_window, guint32 timestamp)
{
    XID parent_window_xid;

    parent_window_xid = GDK_WINDOW_XID (parent_window);

    app->proxy = dbus_g_proxy_new_for_name (app->connection,
					    "org.mate.SettingsDaemon",
					    "/org/mate/SettingsDaemon/XRANDR",
					    "org.mate.SettingsDaemon.XRANDR_2");
    g_assert (app->proxy != NULL); /* that call does not fail unless we pass bogus names */

    app->apply_configuration_state = APPLYING_VERSION_2;
    app->proxy_call = dbus_g_proxy_begin_call (app->proxy, "ApplyConfiguration",
					       apply_configuration_returned_cb, app,
					       NULL,
					       G_TYPE_INT64, (gint64) parent_window_xid,
					       G_TYPE_INT64, (gint64) timestamp,
					       G_TYPE_INVALID,
					       G_TYPE_INVALID);
    /* FIXME: we don't check for app->proxy_call == NULL, which could happen if
     * the connection was disconnected.  This is left as an exercise for the
     * reader.
     */
}

static void
begin_version1_apply_configuration (App *app)
{
    app->proxy = dbus_g_proxy_new_for_name (app->connection,
					    "org.mate.SettingsDaemon",
					    "/org/mate/SettingsDaemon/XRANDR",
					    "org.mate.SettingsDaemon.XRANDR");
    g_assert (app->proxy != NULL); /* that call does not fail unless we pass bogus names */

    app->apply_configuration_state = APPLYING_VERSION_1;
    app->proxy_call = dbus_g_proxy_begin_call (app->proxy, "ApplyConfiguration",
					       apply_configuration_returned_cb, app,
					       NULL,
					       G_TYPE_INVALID,
					       G_TYPE_INVALID);
    /* FIXME: we don't check for app->proxy_call == NULL, which could happen if
     * the connection was disconnected.  This is left as an exercise for the
     * reader.
     */
}

static void
ensure_current_configuration_is_saved (void)
{
        MateRRScreen *rr_screen;
        MateRRConfig *rr_config;

        /* Normally, mate_rr_config_save() creates a backup file based on the
         * old monitors.xml.  However, if *that* file didn't exist, there is
         * nothing from which to create a backup.  So, here we'll save the
         * current/unchanged configuration and then let our caller call
         * mate_rr_config_save() again with the new/changed configuration, so
         * that there *will* be a backup file in the end.
         */

        rr_screen = mate_rr_screen_new (gdk_screen_get_default (), NULL, NULL, NULL); /* NULL-GError */
        if (!rr_screen)
                return;

        rr_config = mate_rr_config_new_current (rr_screen);
        mate_rr_config_save (rr_config, NULL); /* NULL-GError */

        mate_rr_config_free (rr_config);
        mate_rr_screen_destroy (rr_screen);
}

/* Callback for dbus_g_proxy_begin_call() */
static void
apply_configuration_returned_cb (DBusGProxy       *proxy,
				 DBusGProxyCall   *call_id,
				 void             *data)
{
    App *app = data;
    gboolean success;
    GError *error;

    g_assert (call_id == app->proxy_call);

    error = NULL;
    success = dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_INVALID);

    if (!success) {
	if (app->apply_configuration_state == APPLYING_VERSION_2
	    && g_error_matches (error, DBUS_GERROR, DBUS_GERROR_UNKNOWN_METHOD)) {
	    g_error_free (error);

	    g_object_unref (app->proxy);
	    app->proxy = NULL;

	    begin_version1_apply_configuration (app);
	    return;
	} else {
	    /* We don't pop up an error message; mate-settings-daemon already does that
	     * in case the selected RANDR configuration could not be applied.
	     */
	    g_error_free (error);
	}
    }

    g_object_unref (app->proxy);
    app->proxy = NULL;

    dbus_g_connection_unref (app->connection);
    app->connection = NULL;
    app->proxy_call = NULL;

    gtk_widget_set_sensitive (app->dialog, TRUE);
}

static gboolean
sanitize_and_save_configuration (App *app)
{
    GError *error;

    mate_rr_config_sanitize (app->current_configuration);

    check_required_virtual_size (app);

    foo_scroll_area_invalidate (FOO_SCROLL_AREA (app->area));

    ensure_current_configuration_is_saved ();

    error = NULL;
    if (!mate_rr_config_save (app->current_configuration, &error))
    {
	error_message (app, _("Could not save the monitor configuration"), error->message);
	g_error_free (error);
	return FALSE;
    }

    return TRUE;
}

static void
apply (App *app)
{
    GError *error = NULL;

    if (!sanitize_and_save_configuration (app))
	return;

    g_assert (app->connection == NULL);
    g_assert (app->proxy == NULL);
    g_assert (app->proxy_call == NULL);

    app->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (app->connection == NULL) {
	error_message (app, _("Could not get session bus while applying display configuration"), error->message);
	g_error_free (error);
	return;
    }

    gtk_widget_set_sensitive (app->dialog, FALSE);

    begin_version2_apply_configuration (app, gtk_widget_get_window (app->dialog), app->apply_button_clicked_timestamp);
}

#if 0
/* Returns whether the graphics driver doesn't advertise RANDR 1.2 features, and just 1.0 */
static gboolean
driver_is_randr_10 (MateRRConfig *config)
{
    /* In the Xorg code, see xserver/randr/rrinfo.c:RRScanOldConfig().  It gets
     * called when the graphics driver doesn't support RANDR 1.2 yet, just 1.0.
     * In that case, the X server's base code (which supports RANDR 1.2) will
     * simulate having a single output called "default".  For drivers that *do*
     * support RANDR 1.2, the separate outputs will be named differently, we
     * hope.
     *
     * This heuristic is courtesy of Dirk Mueller <dmueller@suse.de>
     *
     * FIXME: however, we don't even check for XRRQueryVersion() returning 1.2, neither
     * here nor in mate-desktop/libmatedesktop*.c.  Do we need to check for that,
     * or is mate_rr_screen_new()'s return value sufficient?
     */

    return (count_all_outputs (config) == 1 && strcmp (config->outputs[0]->name, "default") == 0);
}
#endif

static void
on_detect_displays (GtkWidget *widget, gpointer data)
{
    App *app = data;
    GError *error;

    error = NULL;
    if (!mate_rr_screen_refresh (app->screen, &error)) {
	if (error) {
	    error_message (app, _("Could not detect displays"), error->message);
	    g_error_free (error);
	}
    }
}

#define SHOW_ICON_KEY "/apps/mate_settings_daemon/xrandr/show_notification_icon"


static void
on_show_icon_toggled (GtkWidget *widget, gpointer data)
{
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON (widget);
    App *app = data;

    mateconf_client_set_bool (app->client, SHOW_ICON_KEY,
			   gtk_toggle_button_get_active (tb), NULL);
}

static MateOutputInfo *
get_nearest_output (MateRRConfig *configuration, int x, int y)
{
    int i;
    int nearest_index;
    int nearest_dist;

    nearest_index = -1;
    nearest_dist = G_MAXINT;

    for (i = 0; configuration->outputs[i] != NULL; i++)
    {
	MateOutputInfo *output;
	int dist_x, dist_y;

	output = configuration->outputs[i];

	if (!(output->connected && output->on))
	    continue;

	if (x < output->x)
	    dist_x = output->x - x;
	else if (x >= output->x + output->width)
	    dist_x = x - (output->x + output->width) + 1;
	else
	    dist_x = 0;

	if (y < output->y)
	    dist_y = output->y - y;
	else if (y >= output->y + output->height)
	    dist_y = y - (output->y + output->height) + 1;
	else
	    dist_y = 0;

	if (MIN (dist_x, dist_y) < nearest_dist)
	{
	    nearest_dist = MIN (dist_x, dist_y);
	    nearest_index = i;
	}
    }

    if (nearest_index != -1)
	return configuration->outputs[nearest_index];
    else
	return NULL;

}

/* Gets the output that contains the largest intersection with the window.
 * Logic stolen from gdk_screen_get_monitor_at_window().
 */
static MateOutputInfo *
get_output_for_window (MateRRConfig *configuration, GdkWindow *window)
{
    GdkRectangle win_rect;
    int i;
    int largest_area;
    int largest_index;

    gdk_window_get_geometry (window, &win_rect.x, &win_rect.y, &win_rect.width, &win_rect.height, NULL);
    gdk_window_get_origin (window, &win_rect.x, &win_rect.y);

    largest_area = 0;
    largest_index = -1;

    for (i = 0; configuration->outputs[i] != NULL; i++)
    {
	MateOutputInfo *output;
	GdkRectangle output_rect, intersection;

	output = configuration->outputs[i];

	output_rect.x	   = output->x;
	output_rect.y	   = output->y;
	output_rect.width  = output->width;
	output_rect.height = output->height;

	if (output->connected && gdk_rectangle_intersect (&win_rect, &output_rect, &intersection))
	{
	    int area;

	    area = intersection.width * intersection.height;
	    if (area > largest_area)
	    {
		largest_area = area;
		largest_index = i;
	    }
	}
    }

    if (largest_index != -1)
	return configuration->outputs[largest_index];
    else
	return get_nearest_output (configuration,
				   win_rect.x + win_rect.width / 2,
				   win_rect.y + win_rect.height / 2);
}

/* We select the current output, i.e. select the one being edited, based on
 * which output is showing the configuration dialog.
 */
static void
select_current_output_from_dialog_position (App *app)
{
    if (gtk_widget_get_realized (app->dialog))
	app->current_output = get_output_for_window (app->current_configuration, gtk_widget_get_window (app->dialog));
    else
	app->current_output = NULL;

    rebuild_gui (app);
}

/* This is a GtkWidget::map-event handler.  We wait for the display-properties
 * dialog to be mapped, and then we select the output which corresponds to the
 * monitor on which the dialog is being shown.
 */
static gboolean
dialog_map_event_cb (GtkWidget *widget, GdkEventAny *event, gpointer data)
{
    App *app = data;

    select_current_output_from_dialog_position (app);
    return FALSE;
}

static void
hide_help_button (App *app)
{
    GtkWidget *action_area;
    GList *children;
    GList *l;

    action_area = gtk_dialog_get_action_area (GTK_DIALOG (app->dialog));
    children = gtk_container_get_children (GTK_CONTAINER (action_area));

    for (l = children; l; l = l->next)
    {
	GtkWidget *child;
	int response;

	child = GTK_WIDGET (l->data);

	response = gtk_dialog_get_response_for_widget (GTK_DIALOG (app->dialog), child);
	if (response == GTK_RESPONSE_HELP)
	{
	    gtk_widget_hide (child);
	    return;
	}
    }
}

static void
apply_button_clicked_cb (GtkButton *button, gpointer data)
{
    App *app = data;

    /* We simply store the timestamp at which the Apply button was clicked.
     * We'll just wait for the dialog to return from gtk_dialog_run(), and
     * *then* use the timestamp when applying the RANDR configuration.
     */

    app->apply_button_clicked_timestamp = gtk_get_current_event_time ();
}

static void
success_dialog_for_make_default (App *app)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (GTK_WINDOW (app->dialog),
				     GTK_DIALOG_MODAL,
				     GTK_MESSAGE_INFO,
				     GTK_BUTTONS_OK,
				     _("The monitor configuration has been saved"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					      _("This configuration will be used the next time someone logs in."));

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
error_dialog_for_make_default (App *app, const char *error_text)
{
    error_message (app, _("Could not set the default configuration for monitors"), error_text);
}

static void
make_default (App *app)
{
    char *command_line;
    char *source_filename;
    char *dest_filename;
    char *dest_basename;
    char *std_error;
    gint exit_status;
    GError *error;

    if (!sanitize_and_save_configuration (app))
	return;

    dest_filename = mateconf_client_get_string (app->client, "/apps/mate_settings_daemon/xrandr/default_configuration_file", NULL);
    if (!dest_filename)
	return; /* FIXME: present an error? */

    dest_basename = g_path_get_basename (dest_filename);

    source_filename = mate_rr_config_get_intended_filename ();

    command_line = g_strdup_printf ("pkexec %s/mate-display-properties-install-systemwide %s %s",
				    SBINDIR,
				    source_filename,
				    dest_basename);

    error = NULL;
    if (!g_spawn_command_line_sync (command_line, NULL, &std_error, &exit_status, &error))
    {
	error_dialog_for_make_default (app, error->message);
	g_error_free (error);
    }
    else if (!WIFEXITED (exit_status) || WEXITSTATUS (exit_status) != 0)
    {
	error_dialog_for_make_default (app, std_error);
    }
    else
    {
	success_dialog_for_make_default (app);
    }

    g_free (std_error);
    g_free (dest_filename);
    g_free (dest_basename);
    g_free (source_filename);
    g_free (command_line);
}

static GtkWidget*
_gtk_builder_get_widget (GtkBuilder *builder, const gchar *name)
{
    return GTK_WIDGET (gtk_builder_get_object (builder, name));
}

static void
run_application (App *app)
{
#ifndef UIDIR
#define UIDIR "."
#endif
#define UI_FILE UIDIR "/display-capplet.ui"
    GtkBuilder *builder;
    GtkWidget *align;
    GError *error;

    error = NULL;
    builder = gtk_builder_new ();

    if (gtk_builder_add_from_file (builder, UI_FILE, &error) == 0)
    {
	g_warning ("Could not parse UI definition: %s", error->message);
	g_error_free (error);
	g_object_unref (builder);
	return;
    }

    app->screen = mate_rr_screen_new (gdk_screen_get_default (),
				       on_screen_changed, app, &error);
    if (!app->screen)
    {
	error_message (NULL, _("Could not get screen information"), error->message);
	g_error_free (error);
	g_object_unref (builder);
	return;
    }

    app->client = mateconf_client_get_default ();

    app->dialog = _gtk_builder_get_widget (builder, "dialog");
    g_signal_connect_after (app->dialog, "map-event",
			    G_CALLBACK (dialog_map_event_cb), app);

    gtk_window_set_default_icon_name ("mate-preferences-desktop-display");
    gtk_window_set_icon_name (GTK_WINDOW (app->dialog),
			      "mate-preferences-desktop-display");

    app->current_monitor_event_box = _gtk_builder_get_widget (builder,
    						   "current_monitor_event_box");
    app->current_monitor_label = _gtk_builder_get_widget (builder,
    						       "current_monitor_label");

    app->monitor_on_radio = _gtk_builder_get_widget (builder,
    						     "monitor_on_radio");
    app->monitor_off_radio = _gtk_builder_get_widget (builder,
    						      "monitor_off_radio");
    g_signal_connect (app->monitor_on_radio, "toggled",
		      G_CALLBACK (monitor_on_off_toggled_cb), app);
    g_signal_connect (app->monitor_off_radio, "toggled",
		      G_CALLBACK (monitor_on_off_toggled_cb), app);

    app->resolution_combo = _gtk_builder_get_widget (builder,
    						     "resolution_combo");
    g_signal_connect (app->resolution_combo, "changed",
		      G_CALLBACK (on_resolution_changed), app);

    app->refresh_combo = _gtk_builder_get_widget (builder, "refresh_combo");
    g_signal_connect (app->refresh_combo, "changed",
		      G_CALLBACK (on_rate_changed), app);

    app->rotation_combo = _gtk_builder_get_widget (builder, "rotation_combo");
    g_signal_connect (app->rotation_combo, "changed",
		      G_CALLBACK (on_rotation_changed), app);

    app->clone_checkbox = _gtk_builder_get_widget (builder, "clone_checkbox");
    g_signal_connect (app->clone_checkbox, "toggled",
		      G_CALLBACK (on_clone_changed), app);

    g_signal_connect (_gtk_builder_get_widget (builder, "detect_displays_button"),
		      "clicked", G_CALLBACK (on_detect_displays), app);

    app->show_icon_checkbox = _gtk_builder_get_widget (builder,
						      "show_notification_icon");

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app->show_icon_checkbox),
				  mateconf_client_get_bool (app->client, SHOW_ICON_KEY, NULL));

    g_signal_connect (app->show_icon_checkbox, "toggled", G_CALLBACK (on_show_icon_toggled), app);

    app->panel_checkbox = _gtk_builder_get_widget (builder, "panel_checkbox");

    make_text_combo (app->resolution_combo, 4);
    make_text_combo (app->refresh_combo, 3);
    make_text_combo (app->rotation_combo, -1);

    g_assert (app->panel_checkbox);

    /* Scroll Area */
    app->area = (GtkWidget *)foo_scroll_area_new ();

    g_object_set_data (G_OBJECT (app->area), "app", app);

    set_monitors_tooltip (app, FALSE);

    /* FIXME: this should be computed dynamically */
    foo_scroll_area_set_min_size (FOO_SCROLL_AREA (app->area), -1, 200);
    gtk_widget_show (app->area);
    g_signal_connect (app->area, "paint",
		      G_CALLBACK (on_area_paint), app);
    g_signal_connect (app->area, "viewport_changed",
		      G_CALLBACK (on_viewport_changed), app);

    align = _gtk_builder_get_widget (builder, "align");

    gtk_container_add (GTK_CONTAINER (align), app->area);

    /* Until we have help to show, we'll just hide the Help button */
    hide_help_button (app);

    app->apply_button = _gtk_builder_get_widget (builder, "apply_button");
    g_signal_connect (app->apply_button, "clicked",
		      G_CALLBACK (apply_button_clicked_cb), app);

    on_screen_changed (app->screen, app);

    g_object_unref (builder);

restart:
    switch (gtk_dialog_run (GTK_DIALOG (app->dialog)))
    {
    default:
	/* Fall Through */
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CLOSE:
#if 0
	g_debug ("Close");
#endif
	break;

    case GTK_RESPONSE_HELP:
#if 0
	g_debug ("Help");
#endif
	goto restart;
	break;

    case GTK_RESPONSE_APPLY:
	apply (app);
	goto restart;
	break;

    case RESPONSE_MAKE_DEFAULT:
	make_default (app);
	goto restart;
	break;
    }

    gtk_widget_destroy (app->dialog);
    mate_rr_screen_destroy (app->screen);
    g_object_unref (app->client);
}

int
main (int argc, char **argv)
{
    App *app;

    bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    gtk_init (&argc, &argv);

    app = g_new0 (App, 1);

    run_application (app);

    g_free (app);

    return 0;
}
