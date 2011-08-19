/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* battstat        A MATE battery meter for laptops. 
 * Copyright (C) 2000 by Jörgen Pehrson <jp@spektr.eu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include "battstat.h"

#ifndef gettext_noop
#define gettext_noop(String) (String)
#endif

#define NEVER_SENSITIVE		"never_sensitive"

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	gtk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


#if 0
/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		gtk_widget_set_sensitive (w, FALSE);
	else
		gtk_widget_set_sensitive (w, sensitivity);
}
#endif /* 0 */

static void
combo_ptr_cb (GtkWidget *combo_ptr, gpointer data)
{
	ProgressData *battstat = data;
	
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_ptr)))
		battstat->red_value_is_time = TRUE;
	else
		battstat->red_value_is_time = FALSE;
	
	mate_panel_applet_mateconf_set_bool (MATE_PANEL_APPLET (battstat->applet),
			"red_value_is_time",
			battstat->red_value_is_time,
			NULL);
}

static void
spin_ptr_cb (GtkWidget *spin_ptr, gpointer data)
{
	ProgressData *battstat = data;

	battstat->red_val = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (spin_ptr));
	/* automatically calculate orangle and yellow values from the
	 * red value
	 */
	battstat->orange_val = battstat->red_val * ORANGE_MULTIPLIER;
	battstat->orange_val = CLAMP (battstat->orange_val, 0, 100);

	battstat->yellow_val = battstat->red_val * YELLOW_MULTIPLIER;
	battstat->yellow_val = CLAMP (battstat->yellow_val, 0, 100);
	
	mate_panel_applet_mateconf_set_int (MATE_PANEL_APPLET (battstat->applet),
			"red_value",
			battstat->red_val,
			NULL);
}

static gboolean
key_writable (MatePanelApplet *applet, const char *key)
{
	gboolean writable;
	char *fullkey;
	static MateConfClient *client = NULL;
	if (client == NULL)
		client = mateconf_client_get_default ();

	fullkey = mate_panel_applet_mateconf_get_full_key (applet, key);

	writable = mateconf_client_key_is_writable (client, fullkey, NULL);

	g_free (fullkey);

	return writable;
}

static void
radio_traditional_toggled (GtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  MatePanelApplet *applet = MATE_PANEL_APPLET (battstat->applet);
  gboolean toggled;
  
  toggled = gtk_toggle_button_get_active (button);
 
  /* if (!( toggled || battstat->showtext || battstat->showstatus)) {
    gtk_toggle_button_set_active (button, !toggled);
    return;
  } */

  battstat->showbattery = toggled;
  reconfigure_layout( battstat );

  mate_panel_applet_mateconf_set_bool   (applet, "show_battery", 
  				 battstat->showbattery, NULL);
  				 
}

static void
radio_ubuntu_toggled (GtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  MatePanelApplet *applet = MATE_PANEL_APPLET (battstat->applet);
  gboolean toggled;
  
  toggled = gtk_toggle_button_get_active (button);
  
  /* if (!( toggled || battstat->showtext || battstat->showbattery)) {
    gtk_toggle_button_set_active (button, !toggled);
    return;
  } */
  
  battstat->showstatus = toggled;
  reconfigure_layout( battstat );
  
  mate_panel_applet_mateconf_set_bool   (applet, "show_status", 
  				 battstat->showstatus, NULL);
  				 
}

static void
show_text_toggled (GtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  MatePanelApplet *applet = MATE_PANEL_APPLET (battstat->applet);
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (battstat->radio_text_2))
   && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (battstat->check_text)))
	  battstat->showtext = APPLET_SHOW_PERCENT;
  else if (gtk_toggle_button_get_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_1)) &&
	   gtk_toggle_button_get_active (
		   	  GTK_TOGGLE_BUTTON (battstat->check_text)))
	  battstat->showtext = APPLET_SHOW_TIME;
  else
	  battstat->showtext = APPLET_SHOW_NONE;

  battstat->refresh_label = TRUE;
 
  reconfigure_layout( battstat ); 

  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
		  battstat->showtext);
  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
		  battstat->showtext);
	
  mate_panel_applet_mateconf_set_int   (applet, "show_text", 
  				 battstat->showtext, NULL);
}

static void
lowbatt_toggled (GtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  MatePanelApplet *applet = MATE_PANEL_APPLET (battstat->applet);
  
  battstat->lowbattnotification = gtk_toggle_button_get_active (button);
  mate_panel_applet_mateconf_set_bool   (applet,"low_battery_notification", 
  				 battstat->lowbattnotification, NULL);  

  hard_set_sensitive (battstat->hbox_ptr, battstat->lowbattnotification);
}

static void
full_toggled (GtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  MatePanelApplet *applet = MATE_PANEL_APPLET (battstat->applet);
  
  battstat->fullbattnot = gtk_toggle_button_get_active (button);
  mate_panel_applet_mateconf_set_bool   (applet,"full_battery_notification", 
  				 battstat->fullbattnot, NULL);  
}

static void
response_cb (GtkDialog *dialog, gint id, gpointer data)
{
  ProgressData *battstat = data;
  
  if (id == GTK_RESPONSE_HELP)
    battstat_show_help (battstat, "battstat-appearance");
  else
    gtk_widget_hide (GTK_WIDGET (battstat->prop_win));
}

void
prop_cb (GtkAction    *action,
				 ProgressData *battstat)
{
  GtkBuilder *builder;
  GtkWidget *combo_ptr, *spin_ptr;
  MateConfClient *client;
  GtkListStore *liststore;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  /* Shouldn't this be used for something later on? */
  gboolean   inhibit_command_line;

  client = mateconf_client_get_default ();
  inhibit_command_line = mateconf_client_get_bool (client, "/desktop/mate/lockdown/inhibit_command_line", NULL);

  if (DEBUG) g_print("prop_cb()\n");

   if (battstat->prop_win) { 
     gtk_window_set_screen (GTK_WINDOW (battstat->prop_win),
			    gtk_widget_get_screen (battstat->applet));
     gtk_window_present (GTK_WINDOW (battstat->prop_win));
     return;
   } 

  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTK_BUILDERDIR"/battstat_applet.ui", NULL);

  battstat->prop_win = GTK_DIALOG (gtk_builder_get_object (builder, 
  				   "battstat_properties"));
  gtk_window_set_screen (GTK_WINDOW (battstat->prop_win),
			 gtk_widget_get_screen (battstat->applet));

  g_signal_connect (G_OBJECT (battstat->prop_win), "delete_event",
		  G_CALLBACK (gtk_true), NULL);
  
  battstat->lowbatt_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "lowbatt_toggle"));
  g_signal_connect (G_OBJECT (battstat->lowbatt_toggle), "toggled",
  		    G_CALLBACK (lowbatt_toggled), battstat);

  if (!key_writable (MATE_PANEL_APPLET (battstat->applet),
			  "low_battery_notification"))
  {
	  hard_set_sensitive (battstat->lowbatt_toggle, FALSE);
  }

  battstat->hbox_ptr = GTK_WIDGET (gtk_builder_get_object (builder, "hbox_ptr"));
  hard_set_sensitive (battstat->hbox_ptr, battstat->lowbattnotification);

  combo_ptr = GTK_WIDGET (gtk_builder_get_object (builder, "combo_ptr"));
  g_signal_connect (G_OBJECT (combo_ptr), "changed",
		  G_CALLBACK (combo_ptr_cb), battstat);

  liststore = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_ptr),
		  GTK_TREE_MODEL (liststore));
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_ptr));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_ptr),
		  renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_ptr),
		  renderer,
		  "text", 0,
		  NULL);
  gtk_list_store_append (liststore, &iter);
  /* TRANSLATOR: this is a selectable item in a drop-down menu to end
   * this sentence:
   *   "Warn when battery charge drops to: [XX] percent".
   */
  gtk_list_store_set (liststore, &iter, 0, _("Percent"), -1);
  gtk_list_store_append (liststore, &iter);
  /* TRANSLATOR: this is a selectable item in a drop-down menu to end
   * this sentence:
   *   "Warn when battery charge drops to: [XX] minutes remaining"
   */
  gtk_list_store_set (liststore, &iter, 0, _("Minutes Remaining"), -1);

  spin_ptr = GTK_WIDGET (gtk_builder_get_object (builder, "spin_ptr"));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ptr),
		  battstat->red_val);
  g_signal_connect (G_OBJECT (spin_ptr), "value-changed",
		  G_CALLBACK (spin_ptr_cb), battstat);

  if (battstat->red_value_is_time)
	  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_ptr), 1);
  else
	  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_ptr), 0);

  battstat->full_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "full_toggle"));
  g_signal_connect (G_OBJECT (battstat->full_toggle), "toggled",
  		    G_CALLBACK (full_toggled), battstat);

  if (!key_writable (MATE_PANEL_APPLET (battstat->applet),
			  "full_battery_notification"))
  {
	  hard_set_sensitive (battstat->full_toggle, FALSE);
  }
  if (battstat->fullbattnot)
  {
     gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (battstat->full_toggle),
		     TRUE);
  }
  if (battstat->lowbattnotification)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (battstat->lowbatt_toggle),
		    TRUE);
  }

  battstat->radio_traditional_battery = GTK_WIDGET (gtk_builder_get_object (builder,
		  "battery_view_2"));
  g_signal_connect (G_OBJECT (battstat->radio_traditional_battery), "toggled",
  		    G_CALLBACK (radio_traditional_toggled), battstat);

  if (!key_writable (MATE_PANEL_APPLET (battstat->applet), "show_battery"))
	  hard_set_sensitive (battstat->radio_traditional_battery, FALSE);
  
  if (battstat->showbattery)
  {
    gtk_toggle_button_set_active (
		    GTK_TOGGLE_BUTTON (battstat->radio_traditional_battery),
		    TRUE);
  }
  
  battstat->radio_ubuntu_battery = GTK_WIDGET (gtk_builder_get_object (builder,
		  "battery_view"));
  g_signal_connect (G_OBJECT (battstat->radio_ubuntu_battery), "toggled",
  		    G_CALLBACK (radio_ubuntu_toggled), battstat);

  if (!key_writable (MATE_PANEL_APPLET (battstat->applet), "show_status"))
	  hard_set_sensitive (battstat->radio_ubuntu_battery, FALSE);
	
  if (battstat->showstatus)
  {
    gtk_toggle_button_set_active (
		    GTK_TOGGLE_BUTTON (battstat->radio_ubuntu_battery), TRUE);
  }

  battstat->radio_text_1 = GTK_WIDGET (gtk_builder_get_object (builder, "show_text_radio"));
  battstat->radio_text_2 = GTK_WIDGET (gtk_builder_get_object (builder,
		  "show_text_radio_2"));
  battstat->check_text = GTK_WIDGET (gtk_builder_get_object (builder,
		  "show_text_remaining"));

  g_object_unref (builder);
  
  g_signal_connect (G_OBJECT (battstat->radio_text_1), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  g_signal_connect (G_OBJECT (battstat->radio_text_2), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  g_signal_connect (G_OBJECT (battstat->check_text), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  
  if (!key_writable (MATE_PANEL_APPLET (battstat->applet), "show_text"))
  {
	  hard_set_sensitive (battstat->check_text, FALSE);
	  hard_set_sensitive (battstat->radio_text_1, FALSE);
	  hard_set_sensitive (battstat->radio_text_2, FALSE);
  }

  if (battstat->showtext == APPLET_SHOW_PERCENT)
  {
	  gtk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text), TRUE);
	  gtk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_2), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  TRUE);
  }
  else if (battstat->showtext == APPLET_SHOW_TIME)
  {
	  gtk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text),
			  TRUE);
	  gtk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_1),
			  TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  TRUE);
  }
  else /* APPLET_SHOW_NONE */
  {
	  gtk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text), FALSE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  FALSE);
	  gtk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  FALSE);
  }

   gtk_dialog_set_default_response (GTK_DIALOG (battstat->prop_win),
		   GTK_RESPONSE_CLOSE);
   gtk_window_set_resizable (GTK_WINDOW (battstat->prop_win), FALSE);
   gtk_dialog_set_has_separator (GTK_DIALOG (battstat->prop_win), FALSE);
   
   g_signal_connect (G_OBJECT (battstat->prop_win), "response",
   		     G_CALLBACK (response_cb), battstat);
   gtk_widget_show_all (GTK_WIDGET (battstat->prop_win));
}
