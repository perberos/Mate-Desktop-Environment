/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
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
#include <string.h>
#include "stickynotes_applet.h"
#include "stickynotes_applet_callbacks.h"
#include "stickynotes.h"

#include <gtk/gtk.h>

StickyNotes *stickynotes = NULL;

/* Popup menu on the applet */
static const GtkActionEntry stickynotes_applet_menu_actions[] =
{
	{ "new_note", GTK_STOCK_NEW, N_("_New Note"),
	  NULL, NULL,
	  G_CALLBACK (menu_new_note_cb) },
	{ "hide_notes", NULL, N_("Hi_de Notes"),
	  NULL, NULL,
	  G_CALLBACK (menu_hide_notes_cb) },
	{ "destroy_all", GTK_STOCK_DELETE, N_("_Delete Notes"),
	  NULL, NULL,
	  G_CALLBACK (menu_destroy_all_cb) },
	{ "preferences", GTK_STOCK_PROPERTIES, N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (menu_preferences_cb) },
	{ "help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (menu_help_cb) },
	{ "about", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (menu_about_cb) }
};

static const GtkToggleActionEntry stickynotes_applet_menu_toggle_actions[] =
{
	{ "lock", NULL, N_("_Lock Notes"),
	  NULL, NULL,
	  G_CALLBACK (menu_toggle_lock_cb), FALSE }
};

/* Sticky Notes Icons */
static const StickyNotesStockIcon stickynotes_icons[] =
{
	{ STICKYNOTES_STOCK_LOCKED, STICKYNOTES_ICONDIR "/locked.png" },
	{ STICKYNOTES_STOCK_UNLOCKED, STICKYNOTES_ICONDIR "/unlocked.png" },
	{ STICKYNOTES_STOCK_CLOSE, STICKYNOTES_ICONDIR "/close.png" },
	{ STICKYNOTES_STOCK_RESIZE_SE, STICKYNOTES_ICONDIR "/resize_se.png" },
	{ STICKYNOTES_STOCK_RESIZE_SW, STICKYNOTES_ICONDIR "/resize_sw.png" }
};

/* Sticky Notes applet factory */
static gboolean stickynotes_applet_factory(MatePanelApplet *mate_panel_applet, const gchar *iid, gpointer data)
{
	if (!strcmp(iid, "StickyNotesApplet")) {
		if (!stickynotes)
			stickynotes_applet_init (mate_panel_applet);

		mate_panel_applet_set_flags (mate_panel_applet, MATE_PANEL_APPLET_EXPAND_MINOR);

		/* Add applet to linked list of all applets */
		stickynotes->applets = g_list_append (stickynotes->applets, stickynotes_applet_new(mate_panel_applet));

		stickynotes_applet_update_menus ();
		stickynotes_applet_update_tooltips ();

		return TRUE;
	}

	return FALSE;
}

/* Sticky Notes applet factory */
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("StickyNotesAppletFactory", PANEL_TYPE_APPLET, "stickynotes_applet",
				 stickynotes_applet_factory, NULL)

/* colorshift a pixbuf */
static void
stickynotes_make_prelight_icon (GdkPixbuf *dest, GdkPixbuf *src, int shift)
{
	gint i, j;
	gint width, height, has_alpha, srcrowstride, destrowstride;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *pixsrc;
	guchar *pixdest;
	int val;
	guchar r,g,b;

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	srcrowstride = gdk_pixbuf_get_rowstride (src);
	destrowstride = gdk_pixbuf_get_rowstride (dest);
	target_pixels = gdk_pixbuf_get_pixels (dest);
	original_pixels = gdk_pixbuf_get_pixels (src);

	for (i = 0; i < height; i++) {
		pixdest = target_pixels + i*destrowstride;
		pixsrc = original_pixels + i*srcrowstride;
		for (j = 0; j < width; j++) {
			r = *(pixsrc++);
			g = *(pixsrc++);
			b = *(pixsrc++);
			val = r + shift;
			*(pixdest++) = CLAMP(val, 0, 255);
			val = g + shift;
			*(pixdest++) = CLAMP(val, 0, 255);
			val = b + shift;
			*(pixdest++) = CLAMP(val, 0, 255);
			if (has_alpha)
				*(pixdest++) = *(pixsrc++);
		}
	}
}


/* Create and initalize global sticky notes instance */
void
stickynotes_applet_init (MatePanelApplet *mate_panel_applet)
{
	int timeout;

	stickynotes = g_new(StickyNotes, 1);

	stickynotes->notes = NULL;
	stickynotes->applets = NULL;
	stickynotes->last_timeout_data = 0;

	g_set_application_name (_("Sticky Notes"));

	/* Register size for icons */
	gtk_icon_size_register ("stickynotes_icon_size", 8,8);

	gtk_window_set_default_icon_name ("mate-sticky-notes-applet");

	stickynotes->icon_normal = gtk_icon_theme_load_icon (
			gtk_icon_theme_get_default (),
			"mate-sticky-notes-applet",
			48, 0, NULL);

	stickynotes->icon_prelight = gdk_pixbuf_new (
			gdk_pixbuf_get_colorspace (stickynotes->icon_normal),
			gdk_pixbuf_get_has_alpha (stickynotes->icon_normal),
			gdk_pixbuf_get_bits_per_sample (
				stickynotes->icon_normal),
			gdk_pixbuf_get_width (stickynotes->icon_normal),
			gdk_pixbuf_get_height (stickynotes->icon_normal));
	stickynotes_make_prelight_icon (stickynotes->icon_prelight,
			stickynotes->icon_normal, 30);
	stickynotes->mateconf = mateconf_client_get_default();
	stickynotes->visible = TRUE;

	stickynotes_applet_init_icons();
	stickynotes_applet_init_prefs();

	/* Watch MateConf values */
	mateconf_client_add_dir (stickynotes->mateconf, MATECONF_PATH,
			MATECONF_CLIENT_PRELOAD_NONE, NULL);
	mateconf_client_notify_add (stickynotes->mateconf, MATECONF_PATH "/defaults",
			(MateConfClientNotifyFunc) preferences_apply_cb,
			NULL, NULL, NULL);
	mateconf_client_notify_add (stickynotes->mateconf, MATECONF_PATH "/settings",
			(MateConfClientNotifyFunc) preferences_apply_cb,
			NULL, NULL, NULL);

	/* Max height for large notes*/
	stickynotes->max_height = 0.8*gdk_screen_get_height( gdk_screen_get_default() );

	/* Load sticky notes */
	stickynotes_load (gtk_widget_get_screen (GTK_WIDGET (mate_panel_applet)));

	install_check_click_on_desktop ();
}

/* Initialize Sticky Notes Icons */
void stickynotes_applet_init_icons(void)
{
	GtkIconFactory *icon_factory = gtk_icon_factory_new();

	gint i;
	for (i = 0; i < G_N_ELEMENTS(stickynotes_icons); i++) {
		StickyNotesStockIcon icon = stickynotes_icons[i];
		GtkIconSource *icon_source = gtk_icon_source_new();
		GtkIconSet *icon_set = gtk_icon_set_new();

		gtk_icon_source_set_filename(icon_source, icon.filename);
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_factory_add(icon_factory, icon.stock_id, icon_set);

		gtk_icon_source_free(icon_source);
		gtk_icon_set_unref(icon_set);
	}

	gtk_icon_factory_add_default(icon_factory);

	g_object_unref(G_OBJECT(icon_factory));
}

void stickynotes_applet_init_prefs(void)
{

	stickynotes->builder = gtk_builder_new ();

        gtk_builder_add_from_file (stickynotes->builder, BUILDER_PATH, NULL);

	stickynotes->w_prefs = GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
			"preferences_dialog"));

	stickynotes->w_prefs_width = gtk_spin_button_get_adjustment (
			GTK_SPIN_BUTTON (gtk_builder_get_object (
                                         stickynotes->builder, "width_spin")));
	stickynotes->w_prefs_height = gtk_spin_button_get_adjustment (
			GTK_SPIN_BUTTON (gtk_builder_get_object (
                                         stickynotes->builder, "height_spin")));
	stickynotes->w_prefs_color = GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
			"default_color"));
	stickynotes->w_prefs_font_color = GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
			"prefs_font_color"));
	stickynotes->w_prefs_sys_color = GTK_WIDGET (&GTK_CHECK_BUTTON (
				        gtk_builder_get_object (stickynotes->builder,
					"sys_color_check"))->toggle_button);
	stickynotes->w_prefs_font = GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
			"default_font"));
	stickynotes->w_prefs_sys_font = GTK_WIDGET (&GTK_CHECK_BUTTON (
				        gtk_builder_get_object (stickynotes->builder,
					"sys_font_check"))->toggle_button);
	stickynotes->w_prefs_sticky = GTK_WIDGET (&GTK_CHECK_BUTTON (
				        gtk_builder_get_object (stickynotes->builder,
					"sticky_check"))->toggle_button);
	stickynotes->w_prefs_force = GTK_WIDGET (&GTK_CHECK_BUTTON (
				        gtk_builder_get_object (stickynotes->builder,
					"force_default_check"))->toggle_button);
	stickynotes->w_prefs_desktop = GTK_WIDGET (&GTK_CHECK_BUTTON (
				        gtk_builder_get_object (stickynotes->builder,
					"desktop_hide_check"))->toggle_button);

	g_signal_connect (G_OBJECT (stickynotes->w_prefs), "response",
			G_CALLBACK (preferences_response_cb), NULL);
	g_signal_connect (G_OBJECT (stickynotes->w_prefs), "delete-event",
			G_CALLBACK (preferences_delete_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_width),
			"value-changed",
			G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_height),
			"value-changed",
			G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_sys_color),
			"toggled",
			G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect_swapped (G_OBJECT(stickynotes->w_prefs_sys_font),
			"toggled", G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect (G_OBJECT (stickynotes->w_prefs_color),
			"color-set", G_CALLBACK (preferences_color_cb), NULL);
	g_signal_connect (G_OBJECT (stickynotes->w_prefs_font_color),
			"color-set", G_CALLBACK (preferences_color_cb), NULL);
	g_signal_connect (G_OBJECT (stickynotes->w_prefs_font),
			"font-set", G_CALLBACK (preferences_font_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_sticky),
			"toggled", G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_force),
			"toggled", G_CALLBACK (preferences_save_cb), NULL);
	g_signal_connect_swapped (G_OBJECT (stickynotes->w_prefs_desktop),
			"toggled", G_CALLBACK (preferences_save_cb), NULL);

	{
		GtkSizeGroup *group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

		gtk_size_group_add_widget(group, GTK_WIDGET (gtk_builder_get_object (stickynotes->builder, "width_label")));
		gtk_size_group_add_widget(group, GTK_WIDGET (gtk_builder_get_object (stickynotes->builder, "height_label")));
		gtk_size_group_add_widget(group, GTK_WIDGET (gtk_builder_get_object (stickynotes->builder, "prefs_color_label")));

		g_object_unref(group);
	}

	if (!mateconf_client_key_is_writable(stickynotes->mateconf,
				MATECONF_PATH "/defaults/width", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "width_label")),
				FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "width_spin")),
				FALSE);
	}
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/height", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "height_label")),
				FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "height_spin")),
				FALSE);
	}
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/color", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "prefs_color_label")),
				FALSE);
		gtk_widget_set_sensitive (stickynotes->w_prefs_color, FALSE);
	}
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/font_color", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "prefs_font_color_label")),
				FALSE);
		gtk_widget_set_sensitive (stickynotes->w_prefs_font_color,
				FALSE);
	}
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_color", NULL))
		gtk_widget_set_sensitive (stickynotes->w_prefs_sys_color,
				FALSE);
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/font", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
					stickynotes->builder, "prefs_font_label")),
				FALSE);
		gtk_widget_set_sensitive (stickynotes->w_prefs_font, FALSE);
	}
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_font", NULL))
		gtk_widget_set_sensitive (stickynotes->w_prefs_sys_font,
				FALSE);
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/sticky", NULL))
		gtk_widget_set_sensitive (stickynotes->w_prefs_sticky, FALSE);
	if (!mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/force_default", NULL))
		gtk_widget_set_sensitive (stickynotes->w_prefs_force, FALSE);

	stickynotes_applet_update_prefs();
}

/* Create a Sticky Notes applet */
StickyNotesApplet * stickynotes_applet_new(MatePanelApplet *mate_panel_applet)
{
	AtkObject *atk_obj;
	gchar *ui_path;

	/* Create Sticky Notes Applet */
	StickyNotesApplet *applet = g_new(StickyNotesApplet, 1);

	/* Initialize Sticky Notes Applet */
	applet->w_applet = GTK_WIDGET(mate_panel_applet);
	applet->w_image = gtk_image_new();
	applet->destroy_all_dialog = NULL;
	applet->prelighted = FALSE;
	applet->pressed = FALSE;

	applet->menu_tip = NULL;

	/* Expand the applet for Fitts' law complience. */
	mate_panel_applet_set_flags(mate_panel_applet, MATE_PANEL_APPLET_EXPAND_MINOR);

	/* Add the applet icon */
	gtk_container_add(GTK_CONTAINER(mate_panel_applet), applet->w_image);
	applet->panel_size = mate_panel_applet_get_size (mate_panel_applet);
	applet->panel_orient = mate_panel_applet_get_orient (mate_panel_applet);
	stickynotes_applet_update_icon(applet);

	/* Add the popup menu */
	applet->action_group = gtk_action_group_new ("StickyNotes Applet Actions");
	gtk_action_group_set_translation_domain (applet->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (applet->action_group,
				      stickynotes_applet_menu_actions,
				      G_N_ELEMENTS (stickynotes_applet_menu_actions),
				      applet);
	gtk_action_group_add_toggle_actions (applet->action_group,
					     stickynotes_applet_menu_toggle_actions,
					     G_N_ELEMENTS (stickynotes_applet_menu_toggle_actions),
					     applet);
	ui_path = g_build_filename (STICKYNOTES_MENU_UI_DIR, "stickynotes-applet-menu.xml", NULL);
	mate_panel_applet_setup_menu_from_file(mate_panel_applet, ui_path, applet->action_group);
	g_free (ui_path);

	if (mate_panel_applet_get_locked_down (mate_panel_applet)) {
		GtkAction *action;

		action = gtk_action_group_get_action (applet->action_group, "preferences");
		gtk_action_set_visible (action, FALSE);
	}

	/* Connect all signals for applet management */
	g_signal_connect(G_OBJECT(applet->w_applet), "button-press-event",
			G_CALLBACK(applet_button_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "key-press-event",
			G_CALLBACK(applet_key_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "focus-in-event",
			G_CALLBACK(applet_focus_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "focus-out-event",
			G_CALLBACK(applet_focus_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "enter-notify-event",
			G_CALLBACK(applet_cross_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "leave-notify-event",
			G_CALLBACK(applet_cross_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "size-allocate",
			G_CALLBACK(applet_size_allocate_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "change-orient",
			G_CALLBACK(applet_change_orient_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "change_background",
			G_CALLBACK(applet_change_bg_cb), applet);
	g_signal_connect(G_OBJECT(applet->w_applet), "destroy",
			G_CALLBACK(applet_destroy_cb), applet);

	atk_obj = gtk_widget_get_accessible (applet->w_applet);
	atk_object_set_name (atk_obj, _("Sticky Notes"));

	/* Show the applet */
	gtk_widget_show_all(GTK_WIDGET(applet->w_applet));

	return applet;
}

void stickynotes_applet_update_icon(StickyNotesApplet *applet)
{
	GdkPixbuf *pixbuf1, *pixbuf2;

	gint size = applet->panel_size;

        if (size > 3)
           size = size -3;

	/* Choose appropriate icon and size it */
	if (applet->prelighted)
	    	pixbuf1 = gdk_pixbuf_scale_simple(stickynotes->icon_prelight, size, size, GDK_INTERP_BILINEAR);
	else
	    	pixbuf1 = gdk_pixbuf_scale_simple(stickynotes->icon_normal, size, size, GDK_INTERP_BILINEAR);

	/* Shift the icon if pressed */
	pixbuf2 = gdk_pixbuf_copy(pixbuf1);
	if (applet->pressed)
		gdk_pixbuf_scale(pixbuf1, pixbuf2, 0, 0, size, size, 1, 1, 1, 1, GDK_INTERP_BILINEAR);

	/* Apply the finished pixbuf to the applet image */
	gtk_image_set_from_pixbuf(GTK_IMAGE(applet->w_image), pixbuf2);

	g_object_unref(pixbuf1);
	g_object_unref(pixbuf2);
}

void
stickynotes_applet_update_prefs (void)
{
	int height;
	gboolean sys_color, sys_font, sticky, force_default, desktop_hide;
	char *font_str;
	char *color_str, *font_color_str;
	GdkColor color, font_color;

	gint width = mateconf_client_get_int(stickynotes->mateconf,
			MATECONF_PATH "/defaults/width", NULL);

	width = MAX (width, 1);
	height = mateconf_client_get_int (stickynotes->mateconf,
			MATECONF_PATH "/defaults/height", NULL);
	height = MAX (height, 1);

	sys_color = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/use_system_color", NULL);
	sys_font = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/use_system_font", NULL);
	sticky = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/sticky", NULL);
	force_default = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/force_default", NULL);
	font_str = mateconf_client_get_string (stickynotes->mateconf,
			MATECONF_PATH "/defaults/font", NULL);
	desktop_hide = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/desktop_hide", NULL);

	if (!font_str)
	{
		font_str = g_strdup ("Sans 10");
	}

	color_str = mateconf_client_get_string (stickynotes->mateconf,
			MATECONF_PATH "/defaults/color", NULL);
	if (!color_str)
	{
		color_str = g_strdup ("#ECF833");
	}
	font_color_str = mateconf_client_get_string (stickynotes->mateconf,
			MATECONF_PATH "/defaults/font_color", NULL);
	if (!font_color_str)
	{
		font_color_str = g_strdup ("#000000");
	}

	gdk_color_parse (color_str, &color);
	g_free (color_str);

	gdk_color_parse (font_color_str, &font_color);
	g_free (font_color_str);

	gtk_adjustment_set_value (stickynotes->w_prefs_width, width);
	gtk_adjustment_set_value (stickynotes->w_prefs_height, height);
	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_color),
			sys_color);
	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(stickynotes->w_prefs_sys_font),
			sys_font);
	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sticky),
			sticky);
	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_force),
			force_default);
	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_desktop),
			desktop_hide);

	gtk_color_button_set_color (
			GTK_COLOR_BUTTON (stickynotes->w_prefs_color), &color);
	gtk_color_button_set_color (
			GTK_COLOR_BUTTON (stickynotes->w_prefs_font_color),
			&font_color);
	gtk_font_button_set_font_name (
			GTK_FONT_BUTTON (stickynotes->w_prefs_font), font_str);
	g_free (font_str);

	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/color", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
				stickynotes->builder, "prefs_color_label")),
				!sys_color);
		gtk_widget_set_sensitive (stickynotes->w_prefs_color,
				!sys_color);
	}
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/prefs_font_color", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
				stickynotes->builder, "prefs_font_color_label")),
				!sys_color);
		gtk_widget_set_sensitive (stickynotes->w_prefs_font_color,
				!sys_color);
	}
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/font", NULL))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (
				stickynotes->builder, "prefs_font_label")),
				!sys_font);
		gtk_widget_set_sensitive (stickynotes->w_prefs_font,
				!sys_font);
	}
}

void stickynotes_applet_update_menus(void)
{
	GList *l;
	gboolean inconsistent = FALSE;

	gboolean locked = mateconf_client_get_bool(stickynotes->mateconf, MATECONF_PATH "/settings/locked", NULL);
	gboolean locked_writable = mateconf_client_key_is_writable(stickynotes->mateconf, MATECONF_PATH "/settings/locked", NULL);

	for (l = stickynotes->notes; l != NULL; l = l->next) {
		StickyNote *note = l->data;

		if (note->locked != locked) {
			inconsistent = TRUE;
			break;
		}
	}

	for (l = stickynotes->applets; l != NULL; l = l->next) {
		StickyNotesApplet *applet = l->data;
		GSList *proxies, *p;

		GtkAction *action = gtk_action_group_get_action (applet->action_group, "lock");

		g_object_set (action,
			      "active", locked,
			      "sensitive", locked_writable,
			      NULL);

		proxies = gtk_action_get_proxies (action);
		for (p = proxies; p; p = g_slist_next (p)) {
			if (GTK_IS_CHECK_MENU_ITEM (p->data)) {
				gtk_check_menu_item_set_inconsistent (GTK_CHECK_MENU_ITEM (p->data),
								      inconsistent);
			}
		}
	}
}

void
stickynotes_applet_update_tooltips (void)
{
	int num;
	char *tooltip, *no_notes;
	StickyNotesApplet *applet;
	GList *l;

	num = g_list_length (stickynotes->notes);

	no_notes = g_strdup_printf (ngettext ("%d note", "%d notes", num), num);
	tooltip = g_strdup_printf ("%s\n%s", _("Show sticky notes"), no_notes);

	for (l = stickynotes->applets; l; l = l->next)
	{
		applet = l->data;
		gtk_widget_set_tooltip_text (applet->w_applet, tooltip);

		if (applet->menu_tip)
			gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (
						GTK_BIN (applet->menu_tip))),
				no_notes);
	}

	g_free (tooltip);
	g_free (no_notes);
}

void
stickynotes_applet_panel_icon_get_geometry (int *x, int *y, int *width, int *height)
{
	GtkWidget *widget;
        GtkAllocation allocation;
	GtkRequisition requisition;
	StickyNotesApplet *applet;

	applet = stickynotes->applets->data;

	widget = GTK_WIDGET (applet->w_image);

	gtk_widget_size_request (widget, &requisition);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

	gtk_widget_get_allocation (widget, &allocation);
	*width = allocation.x;
	*height = allocation.y;
}
