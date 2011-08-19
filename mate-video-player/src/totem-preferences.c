/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001,2002,2003 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "totem.h"
#include "totem-private.h"
#include "totem-preferences.h"
#include "totem-interface.h"
#include "video-utils.h"
#include "totem-subtitle-encoding.h"
#include "totem-plugin.h"
#include "totem-plugins-engine.h"

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void checkbutton1_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void checkbutton2_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void checkbutton3_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void audio_screensaver_button_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void no_deinterlace_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void remember_position_checkbutton_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);
G_MODULE_EXPORT void connection_combobox_changed (GtkComboBox *combobox, Totem *totem);
G_MODULE_EXPORT void visual_menu_changed (GtkComboBox *combobox, Totem *totem);
G_MODULE_EXPORT void visual_quality_menu_changed (GtkComboBox *combobox, Totem *totem);
G_MODULE_EXPORT void brightness_changed (GtkRange *range, Totem *totem);
G_MODULE_EXPORT void contrast_changed (GtkRange *range, Totem *totem);
G_MODULE_EXPORT void saturation_changed (GtkRange *range, Totem *totem);
G_MODULE_EXPORT void hue_changed (GtkRange *range, Totem *totem);
G_MODULE_EXPORT void tpw_color_reset_clicked_cb (GtkButton *button, Totem *totem);
G_MODULE_EXPORT void audio_out_menu_changed (GtkComboBox *combobox, Totem *totem);
G_MODULE_EXPORT void font_set_cb (GtkFontButton * fb, Totem * totem);
G_MODULE_EXPORT void encoding_set_cb (GtkComboBox *cb, Totem *totem);
G_MODULE_EXPORT void font_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, Totem *totem);
G_MODULE_EXPORT void encoding_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, Totem *totem);
G_MODULE_EXPORT void auto_chapters_toggled_cb (GtkToggleButton *togglebutton, Totem *totem);

static void
totem_action_info (char *reason, Totem *totem)
{
	GtkWidget *parent, *error_dialog;

	if (totem == NULL)
		parent = NULL;
	else
		parent = totem->prefs;

	error_dialog =
		gtk_message_dialog_new (GTK_WINDOW (parent),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				"%s", reason);
	gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
			GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (error_dialog), "destroy", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	gtk_window_set_modal (GTK_WINDOW (error_dialog), TRUE);

	gtk_widget_show (error_dialog);
}

static gboolean
ask_show_visuals (Totem *totem)
{
	GtkWidget *dialog;
	int answer;

	dialog =
		gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_YES_NO,
				_("Enable visual effects?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("It seems you are running Totem remotely.\n"
						    "Are you sure you want to enable the visual "
						    "effects?"));
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
			GTK_RESPONSE_NO);
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return (answer == GTK_RESPONSE_YES ? TRUE : FALSE);
}

void
checkbutton1_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);
	mateconf_client_set_bool (totem->gc, MATECONF_PREFIX"/auto_resize",
			value, NULL);
	bacon_video_widget_set_auto_resize
		(BACON_VIDEO_WIDGET (totem->bvw), value);
}

static void
totem_prefs_set_show_visuals (Totem *totem, gboolean value)
{
	GtkWidget *item;

	mateconf_client_set_bool (totem->gc,
			MATECONF_PREFIX"/show_vfx", value, NULL);

	item = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tpw_visuals_type_label"));
	gtk_widget_set_sensitive (item, value);
	item = GTK_WIDGET (gtk_builder_get_object (totem->xml,
			"tpw_visuals_type_combobox"));
	gtk_widget_set_sensitive (item, value);
	item = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tpw_visuals_size_label"));
	gtk_widget_set_sensitive (item, value);
	item = GTK_WIDGET (gtk_builder_get_object (totem->xml,
			"tpw_visuals_size_combobox"));
	gtk_widget_set_sensitive (item, value);

	bacon_video_widget_set_show_visuals
		(BACON_VIDEO_WIDGET (totem->bvw), value);
}

void
checkbutton2_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	if (value != FALSE && totem_display_is_local () == FALSE)
	{
		if (ask_show_visuals (totem) == FALSE)
		{
			mateconf_client_set_bool (totem->gc,
					MATECONF_PREFIX"/show_vfx", FALSE, NULL);
			gtk_toggle_button_set_active (togglebutton, FALSE);
			return;
		}
	}

	totem_prefs_set_show_visuals (totem, value);
}

void
checkbutton3_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	mateconf_client_set_bool (totem->gc,
			       MATECONF_PREFIX"/autoload_subtitles", value, NULL);
	totem->autoload_subs = value;
}

void
audio_screensaver_button_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	mateconf_client_set_bool (totem->gc,
			       MATECONF_PREFIX"/lock_screensaver_on_audio",
			       value, NULL);
}

void
no_deinterlace_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	bacon_video_widget_set_deinterlacing (totem->bvw, !value);
	mateconf_client_set_bool (totem->gc,
			       MATECONF_PREFIX"/disable_deinterlacing",
			       value, NULL);
}

static void
no_deinterlace_changed_cb (MateConfClient *client,
			   guint cnxn_id,
			   MateConfEntry *entry,
			   Totem *totem)
{
	GObject *button;
	gboolean value;

	button = gtk_builder_get_object (totem->xml, "tpw_no_deinterlace_checkbutton");

	g_signal_handlers_block_matched (button,
					 G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, totem);

	value = mateconf_client_get_bool (totem->gc,
				       MATECONF_PREFIX"/disable_deinterlacing", NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), value);
	bacon_video_widget_set_deinterlacing (totem->bvw, !value);

	g_signal_handlers_unblock_matched (button,
					   G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, totem);
}

void
remember_position_checkbutton_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	mateconf_client_set_bool (totem->gc,
			       MATECONF_PREFIX"/remember_position",
			       value, NULL);
	totem->remember_position = value;
}

static void
remember_position_changed_cb (MateConfClient *client, guint cnxn_id,
                              MateConfEntry *entry, Totem *totem)
{
	GObject *item;

	item = gtk_builder_get_object (totem->xml, "tpw_remember_position_checkbutton");
	g_signal_handlers_block_by_func (item, remember_position_checkbutton_toggled_cb,
					 totem);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item),
				      mateconf_client_get_bool (totem->gc,
							     MATECONF_PREFIX"/remember_position", NULL));

	g_signal_handlers_unblock_by_func (item, remember_position_checkbutton_toggled_cb,
					   totem);
}

static void
auto_resize_changed_cb (MateConfClient *client, guint cnxn_id,
		MateConfEntry *entry, Totem *totem)
{
	GObject *item;

	item = gtk_builder_get_object (totem->xml, "tpw_display_checkbutton");
	g_signal_handlers_disconnect_by_func (item,
			checkbutton1_toggled_cb, totem);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item),
			mateconf_client_get_bool (totem->gc,
				MATECONF_PREFIX"/auto_resize", NULL));

	g_signal_connect (item, "toggled",
			G_CALLBACK (checkbutton1_toggled_cb), totem);
}

static void
show_vfx_changed_cb (MateConfClient *client, guint cnxn_id,
		     MateConfEntry *entry, Totem *totem)
{
	GObject *item;

	item = gtk_builder_get_object (totem->xml, "tpw_visuals_checkbutton");
	g_signal_handlers_disconnect_by_func (item,
			checkbutton2_toggled_cb, totem);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item),
			mateconf_client_get_bool (totem->gc,
				MATECONF_PREFIX"/show_vfx", NULL));

	g_signal_connect (item, "toggled",
			G_CALLBACK (checkbutton2_toggled_cb), totem);
}

static void
disable_kbd_shortcuts_changed_cb (MateConfClient *client,
				  guint cnxn_id,
				  MateConfEntry *entry,
				  Totem *totem)
{
	totem->disable_kbd_shortcuts = mateconf_client_get_bool (totem->gc,
							      MATECONF_PREFIX"/disable_keyboard_shortcuts", NULL);
}

static void
lock_screensaver_on_audio_changed_cb (MateConfClient *client, guint cnxn_id,
				      MateConfEntry *entry, Totem *totem)
{
	GObject *item, *radio;
	gboolean value;

	item = gtk_builder_get_object (totem->xml, "tpw_audio_toggle_button");
	g_signal_handlers_disconnect_by_func (item,
					      audio_screensaver_button_toggled_cb, totem);

	value = mateconf_client_get_bool (totem->gc,
				       MATECONF_PREFIX"/lock_screensaver_on_audio", NULL);
	if (value != FALSE) {
		radio = gtk_builder_get_object (totem->xml, "tpw_audio_toggle_button");
	} else {
		radio = gtk_builder_get_object (totem->xml, "tpw_video_toggle_button");
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

	g_signal_connect (item, "toggled",
			  G_CALLBACK (audio_screensaver_button_toggled_cb), totem);
}

static void
autoload_subtitles_changed_cb (MateConfClient *client, guint cnxn_id,
			       MateConfEntry *entry, Totem *totem)
{
	GObject *item;

	item = gtk_builder_get_object (totem->xml, "tpw_auto_subtitles_checkbutton");
	g_signal_handlers_disconnect_by_func (item,
			checkbutton3_toggled_cb, totem);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item),
			mateconf_client_get_bool (totem->gc,
				MATECONF_PREFIX"/autoload_subtitles", NULL));

	g_signal_connect (item, "toggled",
			G_CALLBACK (checkbutton3_toggled_cb), totem);
}

static void
autoload_chapters_changed_cb (MateConfClient *client, guint cnxn_id,
			      MateConfEntry *entry, Totem *totem)
{
	GObject *item;

	item = gtk_builder_get_object (totem->xml, "tpw_auto_chapters_checkbutton");
	g_signal_handlers_disconnect_by_func (item,
					      auto_chapters_toggled_cb, totem);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item),
				      mateconf_client_get_bool (totem->gc, MATECONF_PREFIX"/autoload_chapters", NULL));

	g_signal_connect (item, "toggled",
			  G_CALLBACK (auto_chapters_toggled_cb), totem);
}

void
connection_combobox_changed (GtkComboBox *combobox, Totem *totem)
{
	int i;

	i = gtk_combo_box_get_active (combobox);
	bacon_video_widget_set_connection_speed
		(BACON_VIDEO_WIDGET (totem->bvw), i);
}

void
visual_menu_changed (GtkComboBox *combobox, Totem *totem)
{
	GList *list;
	char *old_name, *name;
	int i;

	i = gtk_combo_box_get_active (combobox);
	list = bacon_video_widget_get_visuals_list (totem->bvw);
	name = g_list_nth_data (list, i);

	old_name = mateconf_client_get_string (totem->gc,
			MATECONF_PREFIX"/visual", NULL);

	if (old_name == NULL || strcmp (old_name, name) != 0)
	{
		mateconf_client_set_string (totem->gc, MATECONF_PREFIX"/visual",
				name, NULL);

		if (bacon_video_widget_set_visuals (totem->bvw, name) != FALSE)
			totem_action_info (_("Changing the visuals effect type will require a restart to take effect."), totem);
	}

	g_free (old_name);
}

void
visual_quality_menu_changed (GtkComboBox *combobox, Totem *totem)
{
	int i;

	i = gtk_combo_box_get_active (combobox);
	mateconf_client_set_int (totem->gc,
			MATECONF_PREFIX"/visual_quality", i, NULL);
	bacon_video_widget_set_visuals_quality (totem->bvw, i);
}

void
brightness_changed (GtkRange *range, Totem *totem)
{
	gdouble i;

	i = gtk_range_get_value (range);
	bacon_video_widget_set_video_property (totem->bvw,
			BVW_VIDEO_BRIGHTNESS, (int) i);
}

void
contrast_changed (GtkRange *range, Totem *totem)
{
	gdouble i;

	i = gtk_range_get_value (range);
	bacon_video_widget_set_video_property (totem->bvw,
			BVW_VIDEO_CONTRAST, (int) i);
}

void
saturation_changed (GtkRange *range, Totem *totem)
{
	gdouble i;

	i = gtk_range_get_value (range);
	bacon_video_widget_set_video_property (totem->bvw,
			BVW_VIDEO_SATURATION, (int) i);
}

void
hue_changed (GtkRange *range, Totem *totem)
{
	gdouble i;

	i = gtk_range_get_value (range);
	bacon_video_widget_set_video_property (totem->bvw,
			BVW_VIDEO_HUE, (int) i);
}

void
tpw_color_reset_clicked_cb (GtkButton *button, Totem *totem)
{
	guint i;
	const char *scales[] = {
		"tpw_bright_scale",
		"tpw_contrast_scale",
		"tpw_saturation_scale",
		"tpw_hue_scale"
	};

	for (i = 0; i < G_N_ELEMENTS (scales); i++) {
		GtkRange *item;
		item = GTK_RANGE (gtk_builder_get_object (totem->xml, scales[i]));
		gtk_range_set_value (item, 65535/2);
	}
}

void
audio_out_menu_changed (GtkComboBox *combobox, Totem *totem)
{
	BvwAudioOutType audio_out;
	gboolean need_restart;

	audio_out = gtk_combo_box_get_active (combobox);
	need_restart = bacon_video_widget_set_audio_out_type (totem->bvw, audio_out);
	if (need_restart != FALSE) {
		totem_action_info (_("The change of audio output type will "
					"only take effect when Totem is "
					"restarted."),
				totem);
	}
}

void
font_set_cb (GtkFontButton * fb, Totem * totem)
{
	const gchar *font;

	font = gtk_font_button_get_font_name (fb);
	mateconf_client_set_string (totem->gc, MATECONF_PREFIX"/subtitle_font",
				 font, NULL);
}

void
encoding_set_cb (GtkComboBox *cb, Totem *totem)
{
	const gchar *encoding;

	encoding = totem_subtitle_encoding_get_selected (cb);
	if (encoding)
		mateconf_client_set_string (totem->gc,
				MATECONF_PREFIX"/subtitle_encoding",
				encoding, NULL);
}

void
font_changed_cb (MateConfClient *client, guint cnxn_id,
		 MateConfEntry *entry, Totem *totem)
{
	const gchar *font;
	GtkFontButton *item;

	item = GTK_FONT_BUTTON (gtk_builder_get_object (totem->xml, "font_sel_button"));
	font = mateconf_value_get_string (entry->value);
	gtk_font_button_set_font_name (item, font);
	bacon_video_widget_set_subtitle_font (totem->bvw, font);
}

void
encoding_changed_cb (MateConfClient *client, guint cnxn_id,
		 MateConfEntry *entry, Totem *totem)
{
	const gchar *encoding;
	GtkComboBox *item;

	item = GTK_COMBO_BOX (gtk_builder_get_object (totem->xml, "subtitle_encoding_combo"));
	encoding = mateconf_value_get_string (entry->value);
	totem_subtitle_encoding_set (item, encoding);
	bacon_video_widget_set_subtitle_encoding (totem->bvw, encoding);
}

void
auto_chapters_toggled_cb (GtkToggleButton *togglebutton, Totem *totem)
{
	gboolean value;

	value = gtk_toggle_button_get_active (togglebutton);

	mateconf_client_set_bool (totem->gc, MATECONF_PREFIX"/autoload_chapters", value, NULL);
}

void
totem_setup_preferences (Totem *totem)
{
	GtkWidget *menu, *content_area;
	gboolean show_visuals, auto_resize, is_local, no_deinterlace, lock_screensaver_on_audio, auto_chapters;
	int connection_speed;
	guint i, hidden;
	char *visual, *font, *encoding;
	GList *list, *l;
	BvwAudioOutType audio_out;
	MateConfValue *value;
	GObject *item;

	static struct {
		const char *name;
		BvwVideoProperty prop;
		const char *label;
	} props[4] = {
		{ "tpw_contrast_scale", BVW_VIDEO_CONTRAST, "tpw_contrast_label" },
		{ "tpw_saturation_scale", BVW_VIDEO_SATURATION, "tpw_saturation_label" },
		{ "tpw_bright_scale", BVW_VIDEO_BRIGHTNESS, "tpw_brightness_label" },
		{ "tpw_hue_scale", BVW_VIDEO_HUE, "tpw_hue_label" }
	};

	g_return_if_fail (totem->gc != NULL);

	is_local = totem_display_is_local ();

	mateconf_client_add_dir (totem->gc, MATECONF_PREFIX,
			MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/auto_resize",
			(MateConfClientNotifyFunc) auto_resize_changed_cb,
			totem, NULL, NULL);
	mateconf_client_add_dir (totem->gc, "/desktop/mate/lockdown",
			MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	/* Work-around builder dialogue not parenting properly for
	 * On top windows */
	item = gtk_builder_get_object (totem->xml, "tpw_notebook");
	totem->prefs = gtk_dialog_new_with_buttons (_("Preferences"),
			GTK_WINDOW (totem->win),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_container_set_border_width (GTK_CONTAINER (totem->prefs), 5);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (totem->prefs));
	gtk_box_set_spacing (GTK_BOX (content_area), 2);
	gtk_widget_reparent (GTK_WIDGET (item), content_area);
	gtk_widget_show_all (content_area);
	item = gtk_builder_get_object (totem->xml, "totem_preferences_window");
	gtk_widget_destroy (GTK_WIDGET (item));

	g_signal_connect (G_OBJECT (totem->prefs), "response",
			G_CALLBACK (gtk_widget_hide), NULL);
	g_signal_connect (G_OBJECT (totem->prefs), "delete-event",
			G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (totem->prefs, "destroy",
                          G_CALLBACK (gtk_widget_destroyed), &totem->prefs);

	/* Remember position */
	totem->remember_position = mateconf_client_get_bool (totem->gc,
			MATECONF_PREFIX"/remember_position", NULL);
	item = gtk_builder_get_object (totem->xml, "tpw_remember_position_checkbutton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), totem->remember_position);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/remember_position",
	                         (MateConfClientNotifyFunc) remember_position_changed_cb,
	                         totem, NULL, NULL);

	/* Auto-resize */
	auto_resize = mateconf_client_get_bool (totem->gc,
			MATECONF_PREFIX"/auto_resize", NULL);
	item = gtk_builder_get_object (totem->xml, "tpw_display_checkbutton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), auto_resize);
	bacon_video_widget_set_auto_resize
		(BACON_VIDEO_WIDGET (totem->bvw), auto_resize);

	/* Screensaver audio locking */
	lock_screensaver_on_audio = mateconf_client_get_bool (totem->gc,
							   MATECONF_PREFIX"/lock_screensaver_on_audio", NULL);
	if (lock_screensaver_on_audio != FALSE)
		item = gtk_builder_get_object (totem->xml, "tpw_audio_toggle_button");
	else
		item = gtk_builder_get_object (totem->xml, "tpw_video_toggle_button");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), TRUE);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/lock_screensaver_on_audio",
				 (MateConfClientNotifyFunc) lock_screensaver_on_audio_changed_cb,
				 totem, NULL, NULL);

	/* Disable deinterlacing */
	item = gtk_builder_get_object (totem->xml, "tpw_no_deinterlace_checkbutton");
	no_deinterlace = mateconf_client_get_bool (totem->gc,
						MATECONF_PREFIX"/disable_deinterlacing", NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), no_deinterlace);
	bacon_video_widget_set_deinterlacing (totem->bvw, !no_deinterlace);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/disable_deinterlacing",
				 (MateConfClientNotifyFunc) no_deinterlace_changed_cb,
				 totem, NULL, NULL);

	/* Connection Speed */
	connection_speed = bacon_video_widget_get_connection_speed (totem->bvw);
	item = gtk_builder_get_object (totem->xml, "tpw_speed_combobox");
	gtk_combo_box_set_active (GTK_COMBO_BOX (item), connection_speed);

	/* Enable visuals */
	item = gtk_builder_get_object (totem->xml, "tpw_visuals_checkbutton");
	show_visuals = mateconf_client_get_bool (totem->gc,
			MATECONF_PREFIX"/show_vfx", NULL);
	if (is_local == FALSE && show_visuals != FALSE)
		show_visuals = ask_show_visuals (totem);

	g_signal_handlers_disconnect_by_func (item, checkbutton2_toggled_cb, totem);
	gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (item), show_visuals);
	totem_prefs_set_show_visuals (totem, show_visuals);
	g_signal_connect (item, "toggled", G_CALLBACK (checkbutton2_toggled_cb), totem);

	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/show_vfx",
			(MateConfClientNotifyFunc) show_vfx_changed_cb,
			totem, NULL, NULL);

	/* Auto-load subtitles */
	item = gtk_builder_get_object (totem->xml, "tpw_auto_subtitles_checkbutton");
	totem->autoload_subs = mateconf_client_get_bool (totem->gc,
					      MATECONF_PREFIX"/autoload_subtitles", NULL);

	g_signal_handlers_disconnect_by_func (item, checkbutton3_toggled_cb, totem);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), totem->autoload_subs);
	g_signal_connect (item, "toggled", G_CALLBACK (checkbutton3_toggled_cb), totem);

	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/autoload_subtitles",
				 (MateConfClientNotifyFunc) autoload_subtitles_changed_cb,
				 totem, NULL, NULL);

	/* Auto-load external chapters */
	item = gtk_builder_get_object (totem->xml, "tpw_auto_chapters_checkbutton");
	auto_chapters = mateconf_client_get_bool (totem->gc,
					       MATECONF_PREFIX"/autoload_chapters", NULL);

	g_signal_handlers_disconnect_by_func (item, auto_chapters_toggled_cb, totem);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), auto_chapters);
	g_signal_connect (item, "toggled", G_CALLBACK (auto_chapters_toggled_cb), totem);

	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/autoload_chapters",
				 (MateConfClientNotifyFunc) autoload_chapters_changed_cb,
				 totem, NULL, NULL);

	/* Visuals list */
	list = bacon_video_widget_get_visuals_list (totem->bvw);
	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	visual = mateconf_client_get_string (totem->gc,
			MATECONF_PREFIX"/visual", NULL);
	if (visual == NULL || strcmp (visual, "") == 0) {
		g_free (visual);
		visual = g_strdup ("goom");
	}

	item = gtk_builder_get_object (totem->xml, "tpw_visuals_type_combobox");

	i = 0;
	for (l = list; l != NULL; l = l->next) {
		const char *name = l->data;

		gtk_combo_box_append_text (GTK_COMBO_BOX (item), name);

		if (strcmp (name, visual) == 0)
			gtk_combo_box_set_active (GTK_COMBO_BOX (item), i);

		i++;
	}
	g_free (visual);

	/* Visualisation quality */
	i = mateconf_client_get_int (totem->gc,
			MATECONF_PREFIX"/visual_quality", NULL);
	bacon_video_widget_set_visuals_quality (totem->bvw, i);
	item = gtk_builder_get_object (totem->xml, "tpw_visuals_size_combobox");
	gtk_combo_box_set_active (GTK_COMBO_BOX (item), i);

	/* Brightness and all */
	hidden = 0;
	for (i = 0; i < G_N_ELEMENTS (props); i++) {
		int prop_value;
		item = gtk_builder_get_object (totem->xml, props[i].name);
		prop_value = bacon_video_widget_get_video_property (totem->bvw,
							       props[i].prop);
		if (prop_value >= 0)
			gtk_range_set_value (GTK_RANGE (item), (gdouble) prop_value);
		else {
			gtk_range_set_value (GTK_RANGE (item), (gdouble) 65535/2);
			gtk_widget_hide (GTK_WIDGET (item));
			item = gtk_builder_get_object (totem->xml, props[i].label);
			gtk_widget_hide (GTK_WIDGET (item));
			hidden++;
		}
	}

	if (hidden == G_N_ELEMENTS (props)) {
		item = gtk_builder_get_object (totem->xml, "tpw_bright_contr_vbox");
		gtk_widget_hide (GTK_WIDGET (item));
	}

	/* Sound output type */
	item = gtk_builder_get_object (totem->xml, "tpw_sound_output_combobox");
	audio_out = bacon_video_widget_get_audio_out_type (totem->bvw);
	gtk_combo_box_set_active (GTK_COMBO_BOX (item), audio_out);

	/* Subtitle font selection */
	item = gtk_builder_get_object (totem->xml, "font_sel_button");
	gtk_font_button_set_title (GTK_FONT_BUTTON (item),
				   _("Select Subtitle Font"));
	font = mateconf_client_get_string (totem->gc,
		MATECONF_PREFIX"/subtitle_font", NULL);
	if (font && strcmp (font, "") != 0) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (item), font);
		bacon_video_widget_set_subtitle_font (totem->bvw, font);
	}
	g_free (font);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/subtitle_font",
			(MateConfClientNotifyFunc) font_changed_cb,
			totem, NULL, NULL);

	/* Subtitle encoding selection */
	item = gtk_builder_get_object (totem->xml, "subtitle_encoding_combo");
	totem_subtitle_encoding_init (GTK_COMBO_BOX (item));
	value = mateconf_client_get_without_default (totem->gc,
			MATECONF_PREFIX"/subtitle_encoding", NULL);
	/* Make sure the default is UTF-8 */
	if (value != NULL) {
		if (mateconf_value_get_string (value) == NULL) {
			encoding = g_strdup ("UTF-8");
		} else {
			encoding = g_strdup (mateconf_value_get_string (value));
			if (encoding[0] == '\0') {
				g_free (encoding);
				encoding = g_strdup ("UTF-8");
			}
		}
		mateconf_value_free (value);
	} else {
		encoding = g_strdup ("UTF-8");
	}
	totem_subtitle_encoding_set (GTK_COMBO_BOX(item), encoding);
	if (encoding && strcasecmp (encoding, "") != 0) {
		bacon_video_widget_set_subtitle_encoding (totem->bvw, encoding);
	}
	g_free (encoding);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/subtitle_encoding",
			(MateConfClientNotifyFunc) encoding_changed_cb,
			totem, NULL, NULL);

	/* Disable keyboard shortcuts */
	totem->disable_kbd_shortcuts = mateconf_client_get_bool (totem->gc,
							      MATECONF_PREFIX"/disable_keyboard_shortcuts", NULL);
	mateconf_client_notify_add (totem->gc, MATECONF_PREFIX"/disable_keyboard_shortcuts",
				 (MateConfClientNotifyFunc) disable_kbd_shortcuts_changed_cb,
				 totem, NULL, NULL);
}

void
totem_preferences_visuals_setup (Totem *totem)
{
	char *visual;

	visual = mateconf_client_get_string (totem->gc,
			MATECONF_PREFIX"/visual", NULL);
	if (visual == NULL || strcmp (visual, "") == 0) {
		g_free (visual);
		visual = g_strdup ("goom");
	}

	bacon_video_widget_set_visuals (totem->bvw, visual);
	g_free (visual);
}
