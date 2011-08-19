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
#include "stickynotes_applet_callbacks.h"
#include "stickynotes.h"
#include <gdk/gdkkeysyms.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

static gboolean get_desktop_window (Window *window)
{
	Window *desktop_window;
	GdkWindow *root_window;
	GdkAtom type_returned;
	int format_returned;
	int length_returned;

	root_window = gdk_screen_get_root_window (
				gdk_screen_get_default ());

	if (gdk_property_get (root_window,
			      gdk_atom_intern ("CAJA_DESKTOP_WINDOW_ID", FALSE),
			      gdk_x11_xatom_to_atom (XA_WINDOW),
			      0, 4, FALSE,
			      &type_returned,
			      &format_returned,
			      &length_returned,
			      (guchar**) &desktop_window)) {
		*window = *desktop_window;
		g_free (desktop_window);
		return TRUE;
	}
	else {
		*window = 0;
		return FALSE;
	}
}

static void
popup_add_note (StickyNotesApplet *applet, GtkWidget *item)
{
	stickynotes_add (gtk_widget_get_screen (applet->w_applet));
}

static void
stickynote_show_notes (gboolean visible)
{
	StickyNote *note;
	GList *l;

    if (stickynotes->visible == visible) return;
    stickynotes->visible = visible;

	for (l = stickynotes->notes; l; l = l->next)
	{
		note = l->data;
		stickynote_set_visible (note, visible);
	}
}

static void
stickynote_toggle_notes_visible ()
{
    stickynote_show_notes(!stickynotes->visible);
}

/* Applet Callback : Mouse button press on the applet. */
gboolean
applet_button_cb (GtkWidget         *widget,
		  GdkEventButton    *event,
		  StickyNotesApplet *applet)
{
	if (event->type == GDK_2BUTTON_PRESS)
	{
		popup_add_note (applet, NULL);
		return TRUE;
	}
	else if (event->button == 1)
	{
		stickynote_toggle_notes_visible ();
		return TRUE;
	}

	return FALSE;
}

/* Applet Callback : Keypress on the applet. */
gboolean
applet_key_cb (GtkWidget         *widget,
	       GdkEventKey       *event,
	       StickyNotesApplet *applet)
{
	switch (event->keyval)
	{
		case GDK_KP_Space:
		case GDK_space:
		case GDK_KP_Enter:
		case GDK_Return:
			stickynote_show_notes (TRUE);
			return TRUE;
	}
 	return FALSE;
}

/* Applet Callback : Cross (enter or leave) the applet. */
gboolean applet_cross_cb(GtkWidget *widget, GdkEventCrossing *event, StickyNotesApplet *applet)
{
	applet->prelighted = event->type == GDK_ENTER_NOTIFY || gtk_widget_has_focus(widget);

	stickynotes_applet_update_icon(applet);

	return FALSE;
}

/* Applet Callback : On focus (in or out) of the applet. */
gboolean applet_focus_cb(GtkWidget *widget, GdkEventFocus *event, StickyNotesApplet *applet)
{
	applet->prelighted = event->in;

	stickynotes_applet_update_icon(applet);

	return FALSE;
}

static GdkFilterReturn desktop_window_event_filter (GdkXEvent *xevent,
						    GdkEvent  *event,
						    gpointer   data)
{
	gboolean desktop_hide = mateconf_client_get_bool (stickynotes->mateconf,
			MATECONF_PATH "/settings/desktop_hide", NULL);
	if (desktop_hide  &&
	    (((XEvent*)xevent)->xany.type == PropertyNotify) &&
	    (((XEvent*)xevent)->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME"))) {
		stickynote_show_notes (FALSE);
	}
	return GDK_FILTER_CONTINUE;
}

void install_check_click_on_desktop (void)
{
	Window desktop_window;
	GdkWindow *window;
	Atom user_time_window;
	Atom user_time;

	if (!get_desktop_window (&desktop_window)) {
		return;
	}

	/* Access the desktop window */
	window = gdk_window_foreign_new (desktop_window);

	/* It may contain an atom to tell us which other window to monitor */
	user_time_window = gdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME_WINDOW");
	user_time = gdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME");
	if (user_time != None  &&  user_time_window != None)
	{
		/* Looks like the atoms are there */
		Atom actual_type;
		int  actual_format;
		long nitems;
		long bytes;
		Window *data;

		/* We only use this extra property if the actual user-time property's missing */
		int  status = XGetWindowProperty( GDK_DISPLAY(), desktop_window, user_time,
					0, 4, False, AnyPropertyType, &actual_type, &actual_format,
					&nitems, &bytes, (unsigned char **)&data );
		if (actual_type == None)
		{
			/* No user-time property, so look for the user-time-window */
			status = XGetWindowProperty( GDK_DISPLAY(), desktop_window, user_time_window,
					0, 4, False, AnyPropertyType, &actual_type, &actual_format,
					&nitems, &bytes, (unsigned char **)&data );
			if (actual_type != None)
			{
				/* We have another window to monitor */
				desktop_window = *data;
				window = gdk_window_foreign_new (desktop_window);
			}
		}
	}

	gdk_window_set_events (window, GDK_PROPERTY_CHANGE_MASK);
	gdk_window_add_filter (window, desktop_window_event_filter, NULL);
}

/* Applet Callback : Change the panel orientation. */
void applet_change_orient_cb(MatePanelApplet *mate_panel_applet, MatePanelAppletOrient orient, StickyNotesApplet *applet)
{
	applet->panel_orient = orient;

	return;
}

/* Applet Callback : Resize the applet. */
void applet_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, StickyNotesApplet *applet)
{
	if ((applet->panel_orient == MATE_PANEL_APPLET_ORIENT_UP) || (applet->panel_orient == MATE_PANEL_APPLET_ORIENT_DOWN)) {
	  if (applet->panel_size == allocation->height)
	    return;
	  applet->panel_size = allocation->height;
	} else {
	  if (applet->panel_size == allocation->width)
	    return;
	  applet->panel_size = allocation->width;
	}

	stickynotes_applet_update_icon(applet);

	return;
}

/* Applet Callback : Change the applet background. */
void
applet_change_bg_cb (MatePanelApplet *mate_panel_applet,
		     MatePanelAppletBackgroundType type,
		     GdkColor *color,
		     GdkPixmap *pixmap,
		     StickyNotesApplet *applet)
{
	/* Taken from TrashApplet */
	GtkRcStyle *rc_style;
	GtkStyle *style;

	if (!applet) g_print ("arrg, no applet!\n");

	/* reset style */
	gtk_widget_set_style (GTK_WIDGET (applet->w_applet), NULL);
	rc_style = gtk_rc_style_new ();
	gtk_widget_modify_style (GTK_WIDGET (applet->w_applet), rc_style);
	g_object_unref (rc_style);

	switch (type)
	{
		case PANEL_NO_BACKGROUND:
			break;
		case PANEL_COLOR_BACKGROUND:
			gtk_widget_modify_bg (GTK_WIDGET (applet->w_applet),
					GTK_STATE_NORMAL, color);
			break;
		case PANEL_PIXMAP_BACKGROUND:
			style = gtk_style_copy (
					gtk_widget_get_style (GTK_WIDGET (applet->w_applet)));
			if (style->bg_pixmap[GTK_STATE_NORMAL])
				g_object_unref (
					style->bg_pixmap[GTK_STATE_NORMAL]);
			style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (
					pixmap);
			gtk_widget_set_style (
					GTK_WIDGET (applet->w_applet), style);
			g_object_unref (style);
			break;
	}
}

/* Applet Callback : Deletes the applet. */
void applet_destroy_cb (MatePanelApplet *mate_panel_applet, StickyNotesApplet *applet)
{
	GList *notes;

	stickynotes_save_now ();

	if (applet->destroy_all_dialog != NULL)
		gtk_widget_destroy (applet->destroy_all_dialog);

	if (applet->action_group)
		g_object_unref (applet->action_group);

	if (stickynotes->applets != NULL)
		stickynotes->applets = g_list_remove (stickynotes->applets, applet);

	if (stickynotes->applets == NULL) {
               notes = stickynotes->notes;
               while (notes) {
                       StickyNote *note = notes->data;
                       stickynote_free (note);
                       notes = g_list_next (notes);
               }
	}


}

/* Destroy all response Callback: Callback for the destroy all dialog */
static void
destroy_all_response_cb (GtkDialog *dialog, gint id, StickyNotesApplet *applet)
{
	if (id == GTK_RESPONSE_OK) {
		while (g_list_length(stickynotes->notes) > 0) {
			StickyNote *note = g_list_nth_data(stickynotes->notes, 0);
			stickynote_free(note);
			stickynotes->notes = g_list_remove(stickynotes->notes, note);
		}
	}

	stickynotes_applet_update_tooltips();
	stickynotes_save();

	gtk_widget_destroy (GTK_WIDGET (dialog));
	applet->destroy_all_dialog = NULL;
}

/* Menu Callback : New Note */
void menu_new_note_cb(GtkAction *action, StickyNotesApplet *applet)
{
	popup_add_note (applet, NULL);
}

/* Menu Callback : Hide Notes */
void menu_hide_notes_cb(GtkAction *action, StickyNotesApplet *applet)
{
	stickynote_show_notes (FALSE);
}

/* Menu Callback : Destroy all sticky notes */
void menu_destroy_all_cb(GtkAction *action, StickyNotesApplet *applet)
{
	GtkBuilder *builder;

	builder = gtk_builder_new ();
  	gtk_builder_add_from_file (builder, BUILDER_PATH, NULL);

	if (applet->destroy_all_dialog != NULL) {
		gtk_window_set_screen (GTK_WINDOW (applet->destroy_all_dialog),
				       gtk_widget_get_screen (GTK_WIDGET (applet->w_applet)));

		gtk_window_present (GTK_WINDOW (applet->destroy_all_dialog));
		return;
	}

	applet->destroy_all_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "delete_all_dialog"));

	g_object_unref (builder);

	g_signal_connect (applet->destroy_all_dialog, "response",
			  G_CALLBACK (destroy_all_response_cb),
			  applet);

	gtk_window_set_screen (GTK_WINDOW (applet->destroy_all_dialog),
			gtk_widget_get_screen (applet->w_applet));

	gtk_widget_show_all (applet->destroy_all_dialog);
}

/* Menu Callback: Lock/Unlock sticky notes */
void menu_toggle_lock_cb(GtkAction *action, StickyNotesApplet *applet)
{
	gboolean locked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (mateconf_client_key_is_writable(stickynotes->mateconf, MATECONF_PATH "/settings/locked", NULL))
		mateconf_client_set_bool(stickynotes->mateconf, MATECONF_PATH "/settings/locked", locked, NULL);
}

/* Menu Callback : Configure preferences */
void menu_preferences_cb(GtkAction *action, StickyNotesApplet *applet)
{
	stickynotes_applet_update_prefs();
	gtk_window_set_screen(GTK_WINDOW(stickynotes->w_prefs), gtk_widget_get_screen(applet->w_applet));
	gtk_window_present(GTK_WINDOW(stickynotes->w_prefs));
}

/* Menu Callback : Show help */
void menu_help_cb(GtkAction *action, StickyNotesApplet *applet)
{
	GError *error = NULL;
	gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (applet->w_applet)),
			"ghelp:stickynotes_applet",
			gtk_get_current_event_time (),
			&error);
	if (error) {
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							   _("There was an error displaying help: %s"), error->message);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
		gtk_window_set_screen (GTK_WINDOW(dialog), gtk_widget_get_screen(applet->w_applet));
		gtk_widget_show(dialog);
		g_error_free(error);
	}
}

/* Menu Callback : Display About window */
void
menu_about_cb (GtkAction *action,
	       StickyNotesApplet *applet)
{
	static const gchar *authors[] = {
		"Loban A Rahman <loban@earthling.net>",
		"Davyd Madeley <davyd@madeley.id.au>",
		NULL
	};

	static const gchar *documenters[] = {
		"Loban A Rahman <loban@earthling.net>",
		"Sun MATE Documentation Team <gdocteam@sun.com>",
		NULL
	};

	gtk_show_about_dialog (NULL,
		"version",	VERSION,
		"copyright",	"\xC2\xA9 2002-2003 Loban A Rahman, "
				"\xC2\xA9 2005 Davyd Madeley",
		"comments",	_("Sticky Notes for the "
				  "MATE Desktop Environment"),
		"authors",	authors,
		"documenters",	documenters,
		"translator-credits",	_("translator-credits"),
		"logo-icon-name",	"mate-sticky-notes-applet",
		NULL);
}

/* Preferences Callback : Save. */
void
preferences_save_cb (gpointer data)
{
	gint width = gtk_adjustment_get_value (stickynotes->w_prefs_width);
	gint height = gtk_adjustment_get_value (stickynotes->w_prefs_height);
	gboolean sys_color = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_color));
	gboolean sys_font = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_font));
	gboolean sticky = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sticky));
	gboolean force_default = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_force));
	gboolean desktop_hide = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (stickynotes->w_prefs_desktop));

	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/width", NULL))
		mateconf_client_set_int (stickynotes->mateconf,
				MATECONF_PATH "/defaults/width", width, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/defaults/height", NULL))
		mateconf_client_set_int (stickynotes->mateconf,
				MATECONF_PATH "/defaults/height", height, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_color", NULL))
		mateconf_client_set_bool (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_color",
				sys_color, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_font", NULL))
		mateconf_client_set_bool (stickynotes->mateconf,
				MATECONF_PATH "/settings/use_system_font",
				sys_font, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/sticky", NULL))
		mateconf_client_set_bool (stickynotes->mateconf,
				MATECONF_PATH "/settings/sticky", sticky, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/force_default", NULL))
		mateconf_client_set_bool (stickynotes->mateconf,
				MATECONF_PATH "/settings/force_default",
				force_default, NULL);
	if (mateconf_client_key_is_writable (stickynotes->mateconf,
				MATECONF_PATH "/settings/desktop_hide", NULL))
		mateconf_client_set_bool (stickynotes->mateconf,
				MATECONF_PATH "/settings/desktop_hide",
				desktop_hide, NULL);
}

/* Preferences Callback : Change color. */
void
preferences_color_cb (GtkWidget *button, gpointer data)
{
	GdkColor color, font_color;
	char *color_str, *font_color_str;

	gtk_color_button_get_color (
			GTK_COLOR_BUTTON (stickynotes->w_prefs_color), &color);
	gtk_color_button_get_color (
			GTK_COLOR_BUTTON (stickynotes->w_prefs_font_color),
			&font_color);

	color_str = g_strdup_printf ("#%.2x%.2x%.2x",
			color.red / 256,
			color.green / 256,
			color.blue / 256);
	font_color_str = g_strdup_printf ("#%.2x%.2x%.2x",
			font_color.red / 256,
			font_color.green / 256,
			font_color.blue / 256);

	mateconf_client_set_string (stickynotes->mateconf,
			MATECONF_PATH "/defaults/color", color_str, NULL);
	mateconf_client_set_string (stickynotes->mateconf,
			MATECONF_PATH "/defaults/font_color", font_color_str,
			NULL);

	g_free (color_str);
	g_free (font_color_str);
}

/* Preferences Callback : Change font. */
void preferences_font_cb (GtkWidget *button, gpointer data)
{
	const char *font_str;

	font_str = gtk_font_button_get_font_name (GTK_FONT_BUTTON (button));
	mateconf_client_set_string(stickynotes->mateconf,
			MATECONF_PATH "/defaults/font", font_str, NULL);
}

/* Preferences Callback : Apply to existing notes. */
void preferences_apply_cb(MateConfClient *client, guint cnxn_id, MateConfEntry *entry, gpointer data)
{
	GList *l;
	StickyNote *note;

	if (!strcmp (entry->key, MATECONF_PATH "/settings/sticky"))
	{
		if (mateconf_value_get_bool(entry->value))
			for (l = stickynotes->notes; l; l = l->next)
			{
				note = l->data;
				gtk_window_stick (GTK_WINDOW (note->w_window));
			}
		else
			for (l= stickynotes->notes; l; l = l->next)
			{
				note = l->data;
				gtk_window_unstick (GTK_WINDOW (
							note->w_window));
			}
	}

	else if (!strcmp (entry->key, MATECONF_PATH "/settings/locked"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_locked (note,
					mateconf_value_get_bool (entry->value));
		}
		stickynotes_save();
	}

	else if (!strcmp (entry->key,
				MATECONF_PATH "/settings/use_system_color") ||
		 !strcmp (entry->key, MATECONF_PATH "/defaults/color"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_color (note,
					note->color, note->font_color,
					FALSE);
		}
	}

	else if (!strcmp (entry->key, MATECONF_PATH "/settings/use_system_font") ||
		 !strcmp (entry->key, MATECONF_PATH "/defaults/font"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_font (note, note->font, FALSE);
		}
	}

	else if (!strcmp (entry->key, MATECONF_PATH "/settings/force_default"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_color(note,
					note->color, note->font_color,
					FALSE);
			stickynote_set_font(note, note->font, FALSE);
		}
	}

	stickynotes_applet_update_prefs();
	stickynotes_applet_update_menus();
}

/* Preferences Callback : Response. */
void preferences_response_cb(GtkWidget *dialog, gint response, gpointer data)
{
	if (response == GTK_RESPONSE_HELP) {
		GError *error = NULL;
		gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
				"ghelp:stickynotes_applet?stickynotes-advanced-settings",
				gtk_get_current_event_time (),
				&error);
		if (error) {
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
								   _("There was an error displaying help: %s"), error->message);
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
			gtk_window_set_screen (GTK_WINDOW(dialog), gtk_widget_get_screen(GTK_WIDGET(dialog)));
			gtk_widget_show(dialog);
			g_error_free(error);
		}
	}

	else if (response == GTK_RESPONSE_CLOSE)
	        gtk_widget_hide(GTK_WIDGET(dialog));
}

/* Preferences Callback : Delete */
gboolean preferences_delete_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_hide(widget);

	return TRUE;
}
