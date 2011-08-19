 /*
 * Mini-Commander Applet
 * Copyright (C) 1998 Oliver Maruhn <oliver@maruhn.com>,
 *               2002 Sun Microsystems
 *
 * Authors: Oliver Maruhn <oliver@maruhn.com>,
 *          Mark McLoughlin <mark@skynet.ie>
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
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>


#include <gtk/gtk.h>

#include <mate-panel-applet.h>
#include <mateconf/mateconf-client.h>
#include "mini-commander_applet.h"
#include "preferences.h"
#include "command_line.h"
#include "about.h"
#include "help.h"

#include "browser-mini.xpm"
#include "history-mini.xpm"

#define COMMANDLINE_BROWSER_STOCK "commandline-browser"
#define COMMANDLINE_HISTORY_STOCK "commandline-history"

#define COMMANDLINE_DEFAULT_ICON_SIZE 6

static gboolean icons_initialized = FALSE;
static GtkIconSize button_icon_size = 0;

static const GtkActionEntry mini_commander_menu_actions [] = {
	{ "Props", GTK_STOCK_PROPERTIES, N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (mc_show_preferences) },
	{ "Help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (show_help) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_box) }
};

typedef struct {
    char *stock_id;
    const char **icon_data;
} CommandLineStockIcon;

static CommandLineStockIcon items[] = {
    { COMMANDLINE_BROWSER_STOCK, browser_mini_xpm },
    { COMMANDLINE_HISTORY_STOCK, history_mini_xpm }
};

static void
register_command_line_stock_icons (GtkIconFactory *factory)
{
    gint i;

    for (i = 0; i < G_N_ELEMENTS (items); ++i) {
       GtkIconSet *icon_set;
       GdkPixbuf *pixbuf;

       pixbuf = gdk_pixbuf_new_from_xpm_data ((items[i].icon_data));

       icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
       gtk_icon_factory_add (factory, items[i].stock_id, icon_set);

       gtk_icon_set_unref (icon_set);
       g_object_unref (G_OBJECT (pixbuf));
    }

}

static void
command_line_init_stock_icons (void)
{

    GtkIconFactory *factory;

    if (icons_initialized)
	    return;

    factory = gtk_icon_factory_new ();
    gtk_icon_factory_add_default (factory);

    register_command_line_stock_icons (factory);

    button_icon_size = gtk_icon_size_register ("mini-commander-icon",
                                                 COMMANDLINE_DEFAULT_ICON_SIZE,
                                                 COMMANDLINE_DEFAULT_ICON_SIZE);

    icons_initialized = TRUE;
    g_object_unref (factory);

}

void
set_atk_name_description (GtkWidget  *widget,
			  const char *name,
			  const char *description)
{
    AtkObject *aobj;

    aobj = gtk_widget_get_accessible (widget);
    if (GTK_IS_ACCESSIBLE (aobj) == FALSE)
        return;

    atk_object_set_name (aobj, name);
    atk_object_set_description (aobj, description);
}

/* This is a hack around the fact that gtk+ doesn't
 * propogate button presses on button2/3.
 */
static gboolean
button_press_hack (GtkWidget      *widget,
		   GdkEventButton *event,
		   MCData         *mc)
{
    if (event->button == 3 || event->button == 2) {
	gtk_propagate_event (GTK_WIDGET (mc->applet), (GdkEvent *) event);
	return TRUE;
    }

    return FALSE;
}

/* Send button presses on the applet to the entry. This makes Fitts' law work (ie click on the bottom
** most pixel and the key press will be sent to the entry */
static gboolean
send_button_to_entry_event (GtkWidget *widget, GdkEventButton *event, MCData *mc)
{

	if (event->button == 1) {
		gtk_widget_grab_focus (mc->entry);
		return TRUE;
	}
	return FALSE;

}

static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, MCData *mc)
{
	switch (event->keyval) {
	case GDK_b:
		if (event->state == GDK_CONTROL_MASK) {
			mc_show_file_browser (NULL, mc);
			return TRUE;
		}
		break;
	case GDK_h:
		if (event->state == GDK_CONTROL_MASK) {
			mc_show_history (NULL, mc);
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;

}

void
mc_applet_draw (MCData *mc)
{
    GtkWidget *icon;
    GtkWidget *button;
    GtkWidget *hbox_buttons;
    MCPreferences prefs = mc->preferences;
    int        size_frames = 0;
    gchar     *command_text = NULL;

    if (mc->entry != NULL)
	command_text = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (mc->entry), 0, -1));

    mc->cmd_line_size_y = mc->preferences.normal_size_y - size_frames;

    if (mc->applet_box) {
        gtk_widget_destroy (mc->applet_box);
    }

    if ( ((mc->orient == MATE_PANEL_APPLET_ORIENT_LEFT) || (mc->orient == MATE_PANEL_APPLET_ORIENT_RIGHT)) && (prefs.panel_size_x < 36) )
      mc->applet_box = gtk_vbox_new (FALSE, 0);
    else
      mc->applet_box = gtk_hbox_new (FALSE, 0);

    gtk_container_set_border_width (GTK_CONTAINER (mc->applet_box), 0);

    mc_create_command_entry (mc);

    if (command_text != NULL) {
	gtk_entry_set_text (GTK_ENTRY (mc->entry), command_text);
	g_free (command_text);
    }

    /* hbox for message label and buttons */
    if ((mc->orient == MATE_PANEL_APPLET_ORIENT_LEFT) || (mc->orient == MATE_PANEL_APPLET_ORIENT_RIGHT))
      if (prefs.panel_size_x < 36)
	hbox_buttons = gtk_vbox_new (TRUE, 0);
      else
	hbox_buttons = gtk_hbox_new (TRUE, 0);
    else
      if (prefs.normal_size_y > 36)
	hbox_buttons = gtk_vbox_new (TRUE, 0);
      else
	hbox_buttons = gtk_hbox_new (TRUE, 0);

    /* add file-browser button */
    button = gtk_button_new ();

    g_signal_connect (button, "clicked",
		      G_CALLBACK (mc_show_file_browser), mc);
    g_signal_connect (button, "button_press_event",
		      G_CALLBACK (button_press_hack), mc);

    icon = gtk_image_new_from_stock (COMMANDLINE_BROWSER_STOCK, button_icon_size);
    gtk_container_add (GTK_CONTAINER (button), icon);

    gtk_widget_set_tooltip_text (button, _("Browser"));
    gtk_box_pack_start (GTK_BOX (hbox_buttons), button, TRUE, TRUE, 0);

    set_atk_name_description (button,
			      _("Browser"),
			      _("Click this button to start the browser"));

    /* add history button */
    button = gtk_button_new ();

    g_signal_connect (button, "clicked",
		      G_CALLBACK (mc_show_history), mc);
    g_signal_connect (button, "button_press_event",
		      G_CALLBACK (button_press_hack), mc);

    icon = gtk_image_new_from_stock (COMMANDLINE_HISTORY_STOCK, 								     button_icon_size);
    gtk_container_add (GTK_CONTAINER (button), icon);

    gtk_widget_set_tooltip_text (button, _("History"));
    gtk_box_pack_end (GTK_BOX (hbox_buttons), button, TRUE, TRUE, 0);

    set_atk_name_description (button,
			      _("History"),
			      _("Click this button for the list of previous commands"));

    gtk_box_pack_start (GTK_BOX (mc->applet_box), mc->entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (mc->applet_box), hbox_buttons, TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER (mc->applet), mc->applet_box);

    gtk_widget_show_all (mc->applet_box);
}

static void
mc_destroyed (GtkWidget *widget,
	      MCData    *mc)
{
    MateConfClient *client;
    int          i;

    client = mateconf_client_get_default ();
    for (i = 0; i < MC_NUM_LISTENERS; i++) {
	mateconf_client_notify_remove (client, mc->listeners [i]);
	mc->listeners [i] = 0;
    }
    g_object_unref (client);

    mc_macros_free (mc->preferences.macros);

    if (mc->prefs_dialog.dialog)
        gtk_widget_destroy (mc->prefs_dialog.dialog);

    if (mc->prefs_dialog.dialog)
        g_object_unref (mc->prefs_dialog.macros_store);

    if (mc->file_select)
        gtk_widget_destroy (mc->file_select);

    g_free (mc);
}

static void
mc_orient_changed (MatePanelApplet *applet,
		   MatePanelAppletOrient orient,
		   MCData *mc)
{
  mc->orient = orient;
  mc_applet_draw (mc);
}

static void
mc_pixel_size_changed (MatePanelApplet *applet,
		       GtkAllocation *allocation,
		       MCData      *mc)
{
  if ((mc->orient == MATE_PANEL_APPLET_ORIENT_LEFT) || (mc->orient == MATE_PANEL_APPLET_ORIENT_RIGHT)) {
    if (mc->preferences.panel_size_x == allocation->width)
      return;
    mc->preferences.panel_size_x = allocation->width;
  } else {
    if (mc->preferences.normal_size_y == allocation->height)
      return;
    mc->preferences.normal_size_y = allocation->height;
  }

  mc_applet_draw (mc);
}

static gboolean
mini_commander_applet_fill (MatePanelApplet *applet)
{
    MCData *mc;
    MateConfClient *client;
    GtkActionGroup *action_group;
    gchar *ui_path;

    client = mateconf_client_get_default ();
    if (mateconf_client_get_bool (client, "/desktop/mate/lockdown/inhibit_command_line", NULL)) {
	    GtkWidget *error_dialog;

	    error_dialog = gtk_message_dialog_new (NULL,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_OK,
						   _("Command line has been disabled by your system administrator"));

	    gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
	    gtk_dialog_set_has_separator (GTK_DIALOG (error_dialog), FALSE);
	    gtk_window_set_screen (GTK_WINDOW (error_dialog),
				   gtk_widget_get_screen (GTK_WIDGET (applet)));
	    gtk_dialog_run (GTK_DIALOG (error_dialog));
	    gtk_widget_destroy (error_dialog);

	    /* Note that this is only kosher if this is an out of process thing,
	       which we really are.  We really don't need/want this applet when
	       command line is disabled */
	    exit (1);
    }

    g_set_application_name (_("Command Line"));

    gtk_window_set_default_icon_name ("mate-mini-commander");

    mc = g_new0 (MCData, 1);
    mc->applet = applet;

    mate_panel_applet_add_preferences (applet, "/schemas/apps/mini-commander/prefs", NULL);
    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
    mc_load_preferences (mc);
    command_line_init_stock_icons ();

    g_signal_connect (mc->applet, "change_orient",
		      G_CALLBACK (mc_orient_changed), mc);
    g_signal_connect (mc->applet, "size_allocate",
		      G_CALLBACK (mc_pixel_size_changed), mc);

    mc->preferences.normal_size_y = mate_panel_applet_get_size (applet);
    mc->orient = mate_panel_applet_get_orient (applet);
    mc_applet_draw(mc);
    gtk_widget_show (GTK_WIDGET (mc->applet));

    g_signal_connect (mc->applet, "destroy", G_CALLBACK (mc_destroyed), mc);
    g_signal_connect (mc->applet, "button_press_event",
		      G_CALLBACK (send_button_to_entry_event), mc);
    g_signal_connect (mc->applet, "key_press_event",
		      G_CALLBACK (key_press_cb), mc);

    action_group = gtk_action_group_new ("MiniCommander Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
				  mini_commander_menu_actions,
				  G_N_ELEMENTS (mini_commander_menu_actions),
				  mc);
    ui_path = g_build_filename (MC_MENU_UI_DIR, "mini-commander-applet-menu.xml", NULL);
    mate_panel_applet_setup_menu_from_file (mc->applet, ui_path, action_group);

    if (mate_panel_applet_get_locked_down (mc->applet)) {
	    GtkAction *action;

	    action = gtk_action_group_get_action (action_group, "Props");
	    gtk_action_set_visible (action, FALSE);
    }
    g_object_unref (action_group);

    set_atk_name_description (GTK_WIDGET (applet),
			      _("Mini-Commander applet"),
			      _("This applet adds a command line to the panel"));

    return TRUE;
}

static gboolean
mini_commander_applet_factory (MatePanelApplet *applet,
			       const gchar *iid,
			       gpointer     data)
{
        gboolean retval = FALSE;

        if (!strcmp (iid, "MiniCommanderApplet"))
                retval = mini_commander_applet_fill(applet);

        return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("MiniCommanderAppletFactory",
				  PANEL_TYPE_APPLET,
				  "command-line",
				  mini_commander_applet_factory,
				  NULL)
