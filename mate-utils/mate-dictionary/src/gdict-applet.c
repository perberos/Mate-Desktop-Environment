/* gdict-applet.c - MATE Dictionary Applet
 *
 * Copyright (c) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "gdict-applet.h"
#include "gdict-about.h"
#include "gdict-pref-dialog.h"
#include "gdict-print.h"
#include "gdict-common.h"
#include "gdict-aligned-window.h"

#define GDICT_APPLET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_APPLET, GdictAppletClass))
#define GDICT_APPLET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_APPLET, GdictAppletClass))

struct _GdictAppletClass
{
  MatePanelAppletClass parent_class;
};

#define GDICT_APPLET_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_APPLET, GdictAppletPrivate))

struct _GdictAppletPrivate
{
  guint size;
  GtkOrientation orient;
  
  MateConfClient *mateconf_client;
  guint notify_id;
  guint font_notify_id;

  gchar *database;
  gchar *strategy;
  gchar *source_name;  
  gchar *print_font;
  gchar *defbox_font;

  gchar *word;  
  GdictContext *context;
  guint lookup_start_id;
  guint lookup_end_id;
  guint error_id;

  GdictSourceLoader *loader;

  GtkWidget *box;
  GtkWidget *toggle;
  GtkWidget *image;
  GtkWidget *entry;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *defbox;

  guint idle_draw_id;

  GdkPixbuf *icon;

  gint window_width;
  gint window_height;

  guint is_window_showing : 1;
};

#define WINDOW_MIN_WIDTH 	300
#define WINDOW_MIN_HEIGHT 	200
#define WINDOW_NUM_COLUMNS 	47
#define WINDOW_NUM_ROWS  	20

G_DEFINE_TYPE (GdictApplet, gdict_applet, PANEL_TYPE_APPLET);


static const GtkTargetEntry drop_types[] =
{
  { "text/plain",    0, 0 },
  { "TEXT",          0, 0 },
  { "STRING",        0, 0 },
  { "UTF8_STRING",   0, 0 },
};
static const guint n_drop_types = G_N_ELEMENTS (drop_types);


static void
set_atk_name_description (GtkWidget  *widget,
			  const char *name,
			  const char *description)
{	
  AtkObject *aobj;
	
  aobj = gtk_widget_get_accessible (widget);
  if (!GTK_IS_ACCESSIBLE (aobj))
    return;

  atk_object_set_name (aobj, name);
  atk_object_set_description (aobj, description);
}

static void
set_window_default_size (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *widget, *defbox;
  gint width, height;
  gint font_size;
  GdkScreen *screen;
  gint monitor_num;
  GtkRequisition req;
  GdkRectangle monitor;

  if (!priv->window)
    return;
  
  widget = priv->window;
  defbox = priv->defbox;

  /* Size based on the font size */
  font_size = pango_font_description_get_size (gtk_widget_get_style (defbox)->font_desc);
  font_size = PANGO_PIXELS (font_size);

  width = font_size * WINDOW_NUM_COLUMNS;
  height = font_size * WINDOW_NUM_ROWS;

  /* Use at least the requisition size of the window... */
  gtk_widget_size_request (widget, &req);
  width = MAX (width, req.width);
  height = MAX (height, req.height);

  /* ... but make it no larger than half the monitor size */
  screen = gtk_widget_get_screen (widget);
  monitor_num = gdk_screen_get_monitor_at_window (screen,
                                                  gtk_widget_get_window (widget));

  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  width = MIN (width, monitor.width / 2);
  height = MIN (height, monitor.height / 2);

  /* Set size */
  gtk_widget_set_size_request (priv->frame, width, height);
}

static void
clear_cb (GtkWidget   *widget,
	  GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  gtk_entry_set_text (GTK_ENTRY (priv->entry), "");

  if (!priv->defbox)
    return;
  
  gdict_defbox_clear (GDICT_DEFBOX (priv->defbox));
}

static void
print_cb (GtkWidget   *widget,
	  GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->defbox)
    return;
  
  gdict_show_print_dialog (GTK_WINDOW (priv->window),
  			   GDICT_DEFBOX (priv->defbox));
}

static void
save_cb (GtkWidget   *widget,
         GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *dialog;

  if (!priv->defbox)
    return;
  
  dialog = gtk_file_chooser_dialog_new (_("Save a Copy"),
  					GTK_WINDOW (priv->window),
  					GTK_FILE_CHOOSER_ACTION_SAVE,
  					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
  					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  /* default to user's $HOME */
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("Untitled document"));
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;
      gchar *text;
      gsize len;
      GError *write_error = NULL;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      
      text = gdict_defbox_get_text (GDICT_DEFBOX (priv->defbox), &len);
      
      g_file_set_contents (filename,
      			   text,
      			   len,
      			   &write_error);
      if (write_error)
        {
          gchar *message;
          
          message = g_strdup_printf (_("Error while writing to '%s'"), filename);
          
          gdict_show_error_dialog (GTK_WINDOW (priv->window),
          			   message,
          			   write_error->message);
          
          g_error_free (write_error);
          g_free (message);
        }
      
      g_free (text);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
gdict_applet_set_menu_items_sensitive (GdictApplet *applet,
				       gboolean     is_sensitive)
{
  MateComponentUIComponent *popup_component;
  
  popup_component = mate_panel_applet_get_popup_component (MATE_PANEL_APPLET (applet));
  if (!MATECOMPONENT_IS_UI_COMPONENT (popup_component))
    return;

  matecomponent_ui_component_set_prop (popup_component,
		  		"/commands/Clear",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
  matecomponent_ui_component_set_prop (popup_component,
		  		"/commands/Print",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
  matecomponent_ui_component_set_prop (popup_component,
		  		"/commands/Save",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
}

static gboolean
window_key_press_event_cb (GtkWidget   *widget,
			   GdkEventKey *event,
			   gpointer     user_data)
{
  GdictApplet *applet = GDICT_APPLET (user_data);
  
  if (event->keyval == GDK_Escape)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->toggle), FALSE);
      gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (applet->priv->toggle));

      return TRUE;
    }
  else if ((event->keyval == GDK_l) &&
	   (event->state & GDK_CONTROL_MASK))
    {
      gtk_widget_grab_focus (applet->priv->entry);

      return TRUE;
    }

  return FALSE;
}

static void
window_show_cb (GtkWidget   *window,
		GdictApplet *applet)
{
  set_window_default_size (applet);
}

static void
gdict_applet_build_window (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *button;

  if (!priv->entry)
    {
      g_warning ("No entry widget defined");

      return;
    }
  
  window = gdict_aligned_window_new (priv->toggle);
  g_signal_connect (window, "key-press-event",
		    G_CALLBACK (window_key_press_event_cb),
		    applet);
  g_signal_connect (window, "show",
  		    G_CALLBACK (window_show_cb),
		    applet);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);
  priv->frame = frame;
  
  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);
  
  priv->defbox = gdict_defbox_new ();
  if (priv->context)
    gdict_defbox_set_context (GDICT_DEFBOX (priv->defbox), priv->context);
  
  gtk_box_pack_start (GTK_BOX (vbox), priv->defbox, TRUE, TRUE, 0);
  gtk_widget_show (priv->defbox);
  gtk_widget_set_can_focus (priv->defbox, TRUE);
  gtk_widget_set_can_default (priv->defbox, TRUE);
  
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);
  
  button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
  gtk_widget_set_tooltip_text (button, _("Clear the definitions found"));
  set_atk_name_description (button,
		  	    _("Clear definition"),
			    _("Clear the text of the definition"));
  
  g_signal_connect (button, "clicked", G_CALLBACK (clear_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_widget_set_tooltip_text (button, _("Print the definitions found"));
  set_atk_name_description (button,
		  	    _("Print definition"),
			    _("Print the text of the definition"));
  
  g_signal_connect (button, "clicked", G_CALLBACK (print_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_widget_set_tooltip_text (button, _("Save the definitions found"));
  set_atk_name_description (button,
		  	    _("Save definition"),
			    _("Save the text of the definition to a file"));
  
  g_signal_connect (button, "clicked", G_CALLBACK (save_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  
  gtk_window_set_default (GTK_WINDOW (window), priv->defbox);
  
  priv->window = window;
  priv->is_window_showing = FALSE;
}

static gboolean
gdict_applet_icon_toggled_cb (GtkWidget   *widget,
			      GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->window)
    gdict_applet_build_window (applet);
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gtk_window_set_screen (GTK_WINDOW (priv->window),
	                     gtk_widget_get_screen (GTK_WIDGET (applet)));
      gtk_window_present (GTK_WINDOW (priv->window));
      gtk_widget_grab_focus (priv->defbox);
      
      priv->is_window_showing = TRUE;
    }
  else
    {
      /* force hiding the find pane */
      gdict_defbox_set_show_find (GDICT_DEFBOX (priv->defbox), FALSE);

      gtk_widget_grab_focus (priv->entry);
      gtk_widget_hide (priv->window);
      
      priv->is_window_showing = FALSE;
    }

  return FALSE;
}

static void
gdict_applet_entry_activate_cb (GtkWidget   *widget,
				GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  gchar *text;
  
  text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  if (!text)
    return;
  
  g_free (priv->word);
  priv->word = text;
  
  if (!priv->window)
    gdict_applet_build_window (applet);

  gdict_defbox_lookup (GDICT_DEFBOX (priv->defbox), priv->word);
}

static gboolean
gdict_applet_entry_key_press_cb (GtkWidget   *widget,
				 GdkEventKey *event,
				 gpointer     user_data)
{
  GdictAppletPrivate *priv = GDICT_APPLET (user_data)->priv;
  
  if (event->keyval == GDK_Escape)
    {
      if (priv->is_window_showing)
	{
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle), FALSE);
	  gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (priv->toggle));
	  
	  return TRUE;
	}
    }
  else if (event->keyval == GDK_Tab)
    {
      if (priv->is_window_showing)
	gtk_widget_grab_focus (priv->defbox);
    }

  return FALSE;
}

static gboolean
gdict_applet_icon_button_press_event_cb (GtkWidget      *widget,
					 GdkEventButton *event,
					 GdictApplet    *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  
  /* we don't want to block the applet's popup menu unless the
   * user is toggling the button
   */
  if (event->button != 1)
    g_signal_stop_emission_by_name (priv->toggle, "button-press-event");

  return FALSE;
}

static gboolean
gdict_applet_entry_button_press_event_cb (GtkWidget      *widget,
					  GdkEventButton *event,
					  GdictApplet    *applet)
{
  mate_panel_applet_request_focus (MATE_PANEL_APPLET (applet), event->time);

  return FALSE;
}

static gboolean
gdict_applet_draw (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *box;
  GtkWidget *hbox;
  gchar *text = NULL;

  if (priv->entry)
    text = gtk_editable_get_chars (GTK_EDITABLE (priv->entry), 0, -1);
  
  if (priv->box)
    gtk_widget_destroy (priv->box);

  switch (priv->orient)
    {
    case GTK_ORIENTATION_VERTICAL:
      box = gtk_vbox_new (FALSE, 0);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
      box = gtk_hbox_new (FALSE, 0);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  gtk_container_add (GTK_CONTAINER (applet), box);
  gtk_widget_show (box);

  /* toggle button */
  priv->toggle = gtk_toggle_button_new ();
  gtk_widget_set_tooltip_text (priv->toggle, _("Click to view the dictionary window"));
  set_atk_name_description (priv->toggle,
			    _("Toggle dictionary window"),
		  	    _("Show or hide the definition window"));
  
  gtk_button_set_relief (GTK_BUTTON (priv->toggle),
		  	 GTK_RELIEF_NONE);
  g_signal_connect (priv->toggle, "toggled",
		    G_CALLBACK (gdict_applet_icon_toggled_cb),
		    applet);
  g_signal_connect (priv->toggle, "button-press-event",
		    G_CALLBACK (gdict_applet_icon_button_press_event_cb),
		    applet);
  gtk_box_pack_start (GTK_BOX (box), priv->toggle, FALSE, FALSE, 0);
  gtk_widget_show (priv->toggle);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (priv->toggle), hbox);
  gtk_widget_show (hbox);

  if (priv->icon)
    {
      GdkPixbuf *scaled;
      
      priv->image = gtk_image_new ();
      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);

      scaled = gdk_pixbuf_scale_simple (priv->icon,
		      			priv->size - 5,
					priv->size - 5,
					GDK_INTERP_BILINEAR);
      
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
      g_object_unref (scaled);
      
      gtk_box_pack_start (GTK_BOX (hbox), priv->image, FALSE, FALSE, 0);
      
      gtk_widget_show (priv->image);
    }
  else
    {
      priv->image = gtk_image_new ();

      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);
      gtk_image_set_from_stock (GTK_IMAGE (priv->image),
				GTK_STOCK_MISSING_IMAGE,
				-1);
      
      gtk_box_pack_start (GTK_BOX (hbox), priv->image, FALSE, FALSE, 0);
      gtk_widget_show (priv->image);
    }

  /* entry */
  priv->entry = gtk_entry_new ();
  gtk_widget_set_tooltip_text (priv->entry, _("Type the word you want to look up"));
  set_atk_name_description (priv->entry,
		  	    _("Dictionary entry"),
			    _("Look up words in dictionaries"));
  
  gtk_editable_set_editable (GTK_EDITABLE (priv->entry), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (priv->entry), 12);
  g_signal_connect (priv->entry, "activate",
  		    G_CALLBACK (gdict_applet_entry_activate_cb),
  		    applet);
  g_signal_connect (priv->entry, "button-press-event",
		    G_CALLBACK (gdict_applet_entry_button_press_event_cb),
		    applet);
  g_signal_connect (priv->entry, "key-press-event",
		    G_CALLBACK (gdict_applet_entry_key_press_cb),
		    applet);
  gtk_box_pack_end (GTK_BOX (box), priv->entry, FALSE, FALSE, 0);
  gtk_widget_show (priv->entry);

  if (text)
    {
      gtk_entry_set_text (GTK_ENTRY (priv->entry), text);

      g_free (text);
    }
  
  priv->box = box;

#if 0
  gtk_widget_grab_focus (priv->entry);
#endif
  
  gtk_widget_show_all (GTK_WIDGET (applet));

  return FALSE;
}

static void
gdict_applet_queue_draw (GdictApplet *applet)
{
  if (!applet->priv->idle_draw_id)
    applet->priv->idle_draw_id = g_idle_add ((GSourceFunc) gdict_applet_draw,
					     applet);
}

static void
gdict_applet_lookup_start_cb (GdictContext *context,
			      GdictApplet  *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->window)
    gdict_applet_build_window (applet);

  if (!priv->is_window_showing)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle), TRUE);
      
      gtk_window_present (GTK_WINDOW (priv->window));
      gtk_widget_grab_focus (priv->defbox);

      priv->is_window_showing = TRUE;
    }

  gdict_applet_set_menu_items_sensitive (applet, FALSE);
}

static void
gdict_applet_lookup_end_cb (GdictContext *context,
			    GdictApplet  *applet)
{
  gdict_applet_set_menu_items_sensitive (applet, TRUE);

  gtk_window_present (GTK_WINDOW (applet->priv->window));
}

static void
gdict_applet_error_cb (GdictContext *context,
		       const GError *error,
		       GdictApplet  *applet)
{
  /* disable menu items */
  gdict_applet_set_menu_items_sensitive (applet, FALSE);
}

static void
gdict_applet_cmd_lookup (MateComponentUIComponent *component,
			 GdictApplet       *applet,
			 const gchar       *cname)
{
  GdictAppletPrivate *priv = applet->priv;
  gchar *text = NULL;;
  
  text = gtk_editable_get_chars (GTK_EDITABLE (priv->entry), 0, -1);
  if (!text)
    return;
  
  g_free (priv->word);
  priv->word = text;
  
  if (!priv->window)
    gdict_applet_build_window (applet);
  
  gdict_defbox_lookup (GDICT_DEFBOX (priv->defbox), priv->word);
}

static void
gdict_applet_cmd_clear (MateComponentUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  clear_cb (NULL, applet);
}

static void
gdict_applet_cmd_print (MateComponentUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  print_cb (NULL, applet);
}

static void
gdict_applet_cmd_preferences (MateComponentUIComponent *component,
			      GdictApplet       *applet,
			      const gchar       *cname)
{
  gdict_show_pref_dialog (GTK_WIDGET (applet),
  			  _("Dictionary Preferences"),
  			  applet->priv->loader);
}

static void
gdict_applet_cmd_about (MateComponentUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  gdict_show_about_dialog (GTK_WIDGET (applet));
}

static void
gdict_applet_cmd_help (MateComponentUIComponent *component,
		       GdictApplet       *applet,
		       const gchar       *cname)
{
  GError *err = NULL;

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (applet)),
		"ghelp:mate-dictionary#mate-dictionary-applet",
		gtk_get_current_event_time (), &err);
  
  if (err)
    {
      gdict_show_error_dialog (NULL,
      			       _("There was an error while displaying help"),
      			       err->message);
      g_error_free (err);
    }
}

static void
gdict_applet_change_background (MatePanelApplet               *applet,
				MatePanelAppletBackgroundType  type,
				GdkColor                  *color,
				GdkPixmap                 *pixmap)
{
  if (MATE_PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_background)
    MATE_PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_background (applet,
    								       type,
    								       color,
    								       pixmap);
}

static void
gdict_applet_change_orient (MatePanelApplet       *applet,
			    MatePanelAppletOrient  orient)
{
  GdictAppletPrivate *priv = GDICT_APPLET (applet)->priv;
  guint new_size;
  GtkAllocation allocation;
  
  gtk_widget_get_allocation (GTK_WIDGET (applet), &allocation);
  switch (orient)
    {
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
      priv->orient = GTK_ORIENTATION_VERTICAL;
      new_size = allocation.width;
      break;
    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
      priv->orient = GTK_ORIENTATION_HORIZONTAL;
      new_size = allocation.height;
      break;
    }
  
  if (new_size != priv->size)
    priv->size = new_size;
  
  gdict_applet_queue_draw (GDICT_APPLET (applet));
  
  if (MATE_PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_orient)
    MATE_PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_orient (applet,
    								   orient);
}

static void
gdict_applet_size_allocate (GtkWidget    *widget,
			    GdkRectangle *allocation)
{
  GdictApplet *applet = GDICT_APPLET (widget);
  GdictAppletPrivate *priv = applet->priv;
  guint new_size;
 
  if (priv->orient == GTK_ORIENTATION_HORIZONTAL)
    new_size = allocation->height;
  else
    new_size = allocation->width;
  
  if (priv->size != new_size)
    {
      priv->size = new_size;

      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);

      /* re-scale the icon, if it was found */
      if (priv->icon)
        {
          GdkPixbuf *scaled;

	  scaled = gdk_pixbuf_scale_simple (priv->icon,
			  		    priv->size - 5,
					    priv->size - 5,
					    GDK_INTERP_BILINEAR);
	  
	  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
	  g_object_unref (scaled);
	}
     }

  if (GTK_WIDGET_CLASS (gdict_applet_parent_class)->size_allocate)
    GTK_WIDGET_CLASS (gdict_applet_parent_class)->size_allocate (widget,
		    						 allocation);
}

static void
gdict_applet_style_set (GtkWidget *widget,
			GtkStyle  *old_style)
{
  if (GTK_WIDGET_CLASS (gdict_applet_parent_class)->style_set)
    GTK_WIDGET_CLASS (gdict_applet_parent_class)->style_set (widget,
		    					     old_style);
#if 0
  set_window_default_size (GDICT_APPLET (widget));
#endif
}

static void
gdict_applet_set_database (GdictApplet *applet,
			   const gchar *database)
{
  GdictAppletPrivate *priv = applet->priv;
  
  g_free (priv->database);

  if (database)
    priv->database = g_strdup (database);
  else
    priv->database = gdict_mateconf_get_string_with_default (priv->mateconf_client,
							  GDICT_MATECONF_DATABASE_KEY,
							  GDICT_DEFAULT_DATABASE);
  if (priv->defbox)
    gdict_defbox_set_database (GDICT_DEFBOX (priv->defbox),
			       priv->database);
}

static void
gdict_applet_set_strategy (GdictApplet *applet,
			   const gchar *strategy)
{
  GdictAppletPrivate *priv = applet->priv;
  
  g_free (priv->strategy);

  if (strategy)
    priv->strategy = g_strdup (strategy);
  else
    priv->strategy = gdict_mateconf_get_string_with_default (priv->mateconf_client,
							  GDICT_MATECONF_STRATEGY_KEY,
							  GDICT_DEFAULT_STRATEGY);
}

static GdictContext *
get_context_from_loader (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GdictSource *source;
  GdictContext *retval;

  if (!priv->source_name)
    priv->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

  source = gdict_source_loader_get_source (priv->loader,
		  			   priv->source_name);
  if (!source)
    {
      gchar *detail;
      
      detail = g_strdup_printf (_("No dictionary source available with name '%s'"),
      				priv->source_name);

      gdict_show_error_dialog (NULL,
                               _("Unable to find dictionary source"),
                               NULL);
      
      g_free (detail);

      return NULL;
    }
  
  gdict_applet_set_database (applet, gdict_source_get_database (source));
  gdict_applet_set_strategy (applet, gdict_source_get_strategy (source));
  
  retval = gdict_source_get_context (source);
  if (!retval)
    {
      gchar *detail;
      
      detail = g_strdup_printf (_("No context available for source '%s'"),
      				gdict_source_get_description (source));
      				
      gdict_show_error_dialog (NULL,
                               _("Unable to create a context"),
                               detail);
      
      g_free (detail);
      g_object_unref (source);
      
      return NULL;
    }
  
  g_object_unref (source);
  
  return retval;
}

static void
gdict_applet_set_print_font (GdictApplet *applet,
			     const gchar *print_font)
{
  GdictAppletPrivate *priv = applet->priv;

  g_free (priv->print_font);

  if (print_font)
    priv->print_font = g_strdup (print_font);
  else
    priv->print_font = gdict_mateconf_get_string_with_default (priv->mateconf_client,
							    GDICT_MATECONF_PRINT_FONT_KEY,
							    GDICT_DEFAULT_PRINT_FONT);
}

static void
gdict_applet_set_defbox_font (GdictApplet *applet,
			      const gchar *defbox_font)
{
  GdictAppletPrivate *priv = applet->priv;

  g_free (priv->defbox_font);

  if (defbox_font)
    priv->defbox_font = g_strdup (defbox_font);
  else
    priv->defbox_font = gdict_mateconf_get_string_with_default (priv->mateconf_client,
							     DOCUMENT_FONT_KEY,
							     GDICT_DEFAULT_DEFBOX_FONT);

  if (priv->defbox)
    gdict_defbox_set_font_name (GDICT_DEFBOX (priv->defbox),
				priv->defbox_font);
}

static void
gdict_applet_set_context (GdictApplet  *applet,
			  GdictContext *context)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->context)
    {
      g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
      g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
      g_signal_handler_disconnect (priv->context, priv->error_id);
      
      priv->lookup_start_id = 0;
      priv->lookup_end_id = 0;
      priv->error_id = 0;
      
      g_object_unref (priv->context);
      priv->context = NULL;
    }

  if (priv->defbox)
    gdict_defbox_set_context (GDICT_DEFBOX (priv->defbox), context);

  if (!context)
    return;
  
  /* attach our callbacks */
  priv->lookup_start_id = g_signal_connect (context, "lookup-start",
					    G_CALLBACK (gdict_applet_lookup_start_cb),
					    applet);
  priv->lookup_end_id   = g_signal_connect (context, "lookup-end",
					    G_CALLBACK (gdict_applet_lookup_end_cb),
					    applet);
  priv->error_id        = g_signal_connect (context, "error",
		  			    G_CALLBACK (gdict_applet_error_cb),
					    applet);
  
  priv->context = context;
}

static void
gdict_applet_set_source_name (GdictApplet *applet,
			      const gchar *source_name)
{
  GdictAppletPrivate *priv = applet->priv;
  GdictContext *context;

  g_free (priv->source_name);

  if (source_name)
    priv->source_name = g_strdup (source_name);
  else
    priv->source_name = gdict_mateconf_get_string_with_default (priv->mateconf_client,
							     GDICT_MATECONF_SOURCE_KEY,
							     GDICT_DEFAULT_SOURCE_NAME);
  
  context = get_context_from_loader (applet);
  gdict_applet_set_context (applet, context);
}

static void
gdict_applet_mateconf_notify_cb (MateConfClient *client,
			      guint        cnxn_id,
			      MateConfEntry  *entry,
			      gpointer     user_data)
{
  GdictApplet *applet = GDICT_APPLET (user_data);

  if (strcmp (entry->key, GDICT_MATECONF_PRINT_FONT_KEY) == 0)
    {
      if (entry->value && (entry->value->type == MATECONF_VALUE_STRING))
        gdict_applet_set_print_font (applet, mateconf_value_get_string (entry->value));
      else
        gdict_applet_set_print_font (applet, GDICT_DEFAULT_PRINT_FONT);
    }
  else if (strcmp (entry->key, GDICT_MATECONF_SOURCE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == MATECONF_VALUE_STRING))
        gdict_applet_set_source_name (applet, mateconf_value_get_string (entry->value));
      else
        gdict_applet_set_source_name (applet, GDICT_DEFAULT_SOURCE_NAME);
    }
  else if (strcmp (entry->key, GDICT_MATECONF_DATABASE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == MATECONF_VALUE_STRING))
        gdict_applet_set_database (applet, mateconf_value_get_string (entry->value));
      else
        gdict_applet_set_database (applet, GDICT_DEFAULT_DATABASE);
    }
  else if (strcmp (entry->key, GDICT_MATECONF_STRATEGY_KEY) == 0)
    {
      if (entry->value && (entry->value->type == MATECONF_VALUE_STRING))
        gdict_applet_set_strategy (applet, mateconf_value_get_string (entry->value));
      else
        gdict_applet_set_strategy (applet, GDICT_DEFAULT_STRATEGY);
    }
  else if (strcmp (entry->key, DOCUMENT_FONT_KEY) == 0)
    {
      if (entry->value && (entry->value->type == MATECONF_VALUE_STRING))
        gdict_applet_set_defbox_font (applet, mateconf_value_get_string (entry->value));
      else
        gdict_applet_set_defbox_font (applet, GDICT_DEFAULT_DEFBOX_FONT);
    }
}

static void
gdict_applet_finalize (GObject *object)
{
  GdictApplet *applet = GDICT_APPLET (object);
  GdictAppletPrivate *priv = applet->priv;

  if (priv->idle_draw_id)
    g_source_remove (priv->idle_draw_id);

  if (priv->notify_id)
    mateconf_client_notify_remove (priv->mateconf_client, priv->notify_id);

  if (priv->font_notify_id)
    mateconf_client_notify_remove (priv->mateconf_client, priv->font_notify_id);
  
  if (priv->mateconf_client)
    g_object_unref (priv->mateconf_client);
  
  if (priv->context)
    {
      if (priv->lookup_start_id)
        {
          g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
	  g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
	  g_signal_handler_disconnect (priv->context, priv->error_id);
	}

      g_object_unref (priv->context);
    }
  
  if (priv->loader)
    g_object_unref (priv->loader);
  
  if (priv->icon)
    g_object_unref (priv->icon);
  
  g_free (priv->source_name);
  g_free (priv->print_font);
  g_free (priv->defbox_font);
  g_free (priv->word);
  
  G_OBJECT_CLASS (gdict_applet_parent_class)->finalize (object);
}

static void
gdict_applet_class_init (GdictAppletClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  MatePanelAppletClass *applet_class = MATE_PANEL_APPLET_CLASS (klass);
  
  gobject_class->finalize = gdict_applet_finalize;
  
  widget_class->size_allocate = gdict_applet_size_allocate;
  widget_class->style_set = gdict_applet_style_set;
  
  applet_class->change_background = gdict_applet_change_background;
  applet_class->change_orient = gdict_applet_change_orient;
  
  g_type_class_add_private (gobject_class, sizeof (GdictAppletPrivate));
}

static void
gdict_applet_init (GdictApplet *applet)
{
  GdictAppletPrivate *priv;
  gchar *data_dir;
  GError *mateconf_error;

  priv = GDICT_APPLET_GET_PRIVATE (applet);
  applet->priv = priv;
      
  if (!priv->loader)
    priv->loader = gdict_source_loader_new ();

  /* add our data dir inside $HOME to the loader's search paths */
  data_dir = gdict_get_data_dir ();
  gdict_source_loader_add_search_path (priv->loader, data_dir);
  g_free (data_dir);
  
  gtk_window_set_default_icon_name ("accessories-dictionary");
  
  mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet),
			  MATE_PANEL_APPLET_EXPAND_MINOR);
  
  /* get the default mateconf client */
  if (!priv->mateconf_client)
    priv->mateconf_client = mateconf_client_get_default ();
  
  mateconf_error = NULL;
  mateconf_client_add_dir (priv->mateconf_client,
  			GDICT_MATECONF_DIR,
  			MATECONF_CLIENT_PRELOAD_ONELEVEL,
  			&mateconf_error);
  if (mateconf_error)
    {
      gdict_show_gerror_dialog (NULL,
		                _("Unable to connect to MateConf"),
		                mateconf_error);
      mateconf_error = NULL;
    }
  
  priv->notify_id = mateconf_client_notify_add (priv->mateconf_client,
		  			     GDICT_MATECONF_DIR,
					     gdict_applet_mateconf_notify_cb,
					     applet, NULL,
					     &mateconf_error);
  if (mateconf_error)
    {
      gdict_show_gerror_dialog (NULL,
		                _("Unable to get notification for preferences"),
		                mateconf_error);

      mateconf_error = NULL;
    }
  
  priv->font_notify_id =  mateconf_client_notify_add (priv->mateconf_client,
		  				   DOCUMENT_FONT_KEY,
						   gdict_applet_mateconf_notify_cb,
						   applet, NULL,
						   &mateconf_error);
  if (mateconf_error)
    {
      gdict_show_gerror_dialog (NULL,
                               _("Unable to get notification for the document font"),
                               mateconf_error);

      mateconf_error = NULL;
    }
  
#ifndef GDICT_APPLET_STAND_ALONE
  mate_panel_applet_set_background_widget (MATE_PANEL_APPLET (applet),
		  		      GTK_WIDGET (applet));

  priv->size = mate_panel_applet_get_size (MATE_PANEL_APPLET (applet));

  switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet)))
    {
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
      priv->orient = GTK_ORIENTATION_VERTICAL;
      break;
    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
      priv->orient = GTK_ORIENTATION_HORIZONTAL;
      break;
    }
#else
  priv->size = 24;
  priv->orient = GTK_ORIENTATION_HORIZONTAL;
  g_message ("(in %s) applet { size = %d, orient = %s }",
  	     G_STRFUNC,
  	     priv->size,
  	     (priv->orient == GTK_ORIENTATION_HORIZONTAL ? "H" : "V"));
#endif /* !GDICT_APPLET_STAND_ALONE */

  priv->icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
		  			 "accessories-dictionary",
					 48,
					 0,
					 NULL);
  
  /* force first draw */
  gdict_applet_draw (applet);

  /* force retrieval of the configuration from MateConf */
  gdict_applet_set_source_name (applet, NULL);
  gdict_applet_set_defbox_font (applet, NULL);
  gdict_applet_set_print_font (applet, NULL);
}

#ifndef GDICT_APPLET_STAND_ALONE
static const MateComponentUIVerb gdict_applet_menu_verbs[] =
{
  MATECOMPONENT_UI_UNSAFE_VERB ("LookUp", gdict_applet_cmd_lookup),
  
  MATECOMPONENT_UI_UNSAFE_VERB ("Clear", gdict_applet_cmd_clear),
  MATECOMPONENT_UI_UNSAFE_VERB ("Print", gdict_applet_cmd_print),
  
  MATECOMPONENT_UI_UNSAFE_VERB ("Preferences", gdict_applet_cmd_preferences),
  MATECOMPONENT_UI_UNSAFE_VERB ("Help", gdict_applet_cmd_help),
  MATECOMPONENT_UI_UNSAFE_VERB ("About", gdict_applet_cmd_about),

  MATECOMPONENT_UI_VERB_END
};

static gboolean
gdict_applet_factory (MatePanelApplet *applet,
                      const gchar *iid,
		      gpointer     data)
{
  gboolean retval = FALSE;

  if (((!strcmp (iid, "OAFIID:MATE_DictionaryApplet")) ||
       (!strcmp (iid, "OAFIID:MATE_GDictApplet"))) &&
      gdict_create_data_dir ())
    {
      /* Set up the menu */
      mate_panel_applet_setup_menu_from_file (applet, UIDATADIR,
					 "MATE_DictionaryApplet.xml",
					 NULL,
					 gdict_applet_menu_verbs,
					 applet);

      gtk_widget_show (GTK_WIDGET (applet));

      /* set the menu items insensitive */
      gdict_applet_set_menu_items_sensitive (GDICT_APPLET (applet), FALSE);
      
      retval = TRUE;
    }

  return retval;
}

/* this defines the main () for the applet */
MATE_PANEL_APPLET_MATECOMPONENT_FACTORY ("OAFIID:MATE_DictionaryApplet_Factory",
			     GDICT_TYPE_APPLET,
			     "mate-dictionary-applet",
			     VERSION,
			     gdict_applet_factory,
			     NULL);

#else /* GDICT_APPLET_STAND_ALONE */

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *applet;

  /* gettext stuff */
  bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  applet = GTK_WIDGET (g_object_new (GDICT_TYPE_APPLET, NULL));
  g_message ("(in %s) typeof(applet) = '%s'",
  	     G_STRFUNC,
  	     g_type_name (G_OBJECT_TYPE (applet)));

  gdict_applet_queue_draw (GDICT_APPLET (applet));
  
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  gtk_container_add (GTK_CONTAINER (window), applet);
  
  gtk_widget_show_all (window);
  
  gtk_main ();
  
  return 0;
}

#endif /* !GDICT_APPLET_STAND_ALONE */
