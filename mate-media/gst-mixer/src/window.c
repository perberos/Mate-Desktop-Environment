/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * window.c: main window
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gdk/gdkkeysyms.h>

#include "keys.h"
#include "preferences.h"
#include "window.h"

G_DEFINE_TYPE (MateVolumeControlWindow, mate_volume_control_window, GTK_TYPE_WINDOW)

void mate_volume_control_window_set_page(GtkWidget *widget, const gchar *page)
{
  MateVolumeControlWindow *win = MATE_VOLUME_CONTROL_WINDOW (widget);

  if (g_ascii_strncasecmp(page, "playback",8) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 0);
  else if (g_ascii_strncasecmp(page, "recording",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 1);
  else if (g_ascii_strncasecmp(page, "switches",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 2);
  else if (g_ascii_strncasecmp(page, "options",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 3);
  else /* default is "playback" */
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 0);
}


/*
 * Menu actions.
 */

static void
cb_change (GtkComboBox *widget,
	   MateVolumeControlWindow *win)
{
  gchar *device_name;

  device_name = gtk_combo_box_get_active_text (widget);
  g_return_if_fail (device_name != NULL);

  mateconf_client_set_string (win->client, MATE_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT, device_name, NULL);

  g_free (device_name);
}

static void
cb_exit (GtkAction *action,
	 MateVolumeControlWindow *win)
{
  gtk_widget_destroy (GTK_WIDGET (win));
}

static void
cb_preferences_destroy (GtkWidget *widget,
			MateVolumeControlWindow *win)
{
  win->prefs = NULL;
}

static void
cb_preferences (GtkAction *action,
		MateVolumeControlWindow *win)
{

  if (!win->prefs) {
    win->prefs = mate_volume_control_preferences_new (GST_ELEMENT (win->el->mixer),
						       win->client);
    g_signal_connect (win->prefs, "destroy", G_CALLBACK (cb_preferences_destroy), win);
    gtk_widget_show (win->prefs);
  } else {
    gtk_window_present (GTK_WINDOW (win->prefs));
  }
}

static void
cb_help (GtkAction *action,
	 MateVolumeControlWindow *win)
{
  GdkScreen *screen;
  GtkWidget *dialog;
  GError *error = NULL;

  screen = gtk_window_get_screen (GTK_WINDOW (win));

  if (gtk_show_uri (screen, "ghelp:mate-volume-control", GDK_CURRENT_TIME,
  				&error) == FALSE) {
  	dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", error->message);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (error);
  }
}

static void
cb_show_about (MateVolumeControlWindow *win)
{
  const gchar *authors[] = { "Ronald Bultje <rbultje@ronald.bitfreak.net>",
			     "Leif Johnson <leif@ambient.2y.net>",
			     NULL };
  const gchar *documenters[] = { "Sun Microsystems",
				 NULL};

  gtk_show_about_dialog (GTK_WINDOW (win),
			 "version", VERSION,
			 "copyright", "Copyright \xc2\xa9 2003-2004 Ronald Bultje",
			 "comments", _("A MATE/GStreamer-based volume control application"),
			 "authors", authors,
			 "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "multimedia-volume-control",
			 NULL);
}

static void
window_change_mixer_element (MateVolumeControlWindow *win,
			     const gchar *el)
{
  const char *cur_el_str;
  GList *item;

  g_return_if_fail (win != NULL);
  g_return_if_fail (el != NULL);

  for (item = win->elements; item != NULL; item = item->next) {
    cur_el_str = g_object_get_data (item->data, "mate-volume-control-name");

    if (cur_el_str == NULL)
      continue;

    if (g_str_equal (cur_el_str, el)) {
      GstElement *old_element = GST_ELEMENT (win->el->mixer);
      gchar *title;

      /* change element */
      gst_element_set_state (item->data, GST_STATE_READY);
      mate_volume_control_element_change (win->el, item->data);

      if (win->prefs != NULL)
	mate_volume_control_preferences_change (MATE_VOLUME_CONTROL_PREFERENCES (win->prefs), 
						 item->data);

      if (old_element != NULL)
	gst_element_set_state (old_element, GST_STATE_NULL);

      /* change window title */
      title = g_strdup_printf (_("Volume Control: %s"), cur_el_str);
      gtk_window_set_title (GTK_WINDOW (win), title);
      g_free (title);

      break;
    }
  }
}

static void
cb_mateconf (MateConfClient *client,
	  guint        connection_id,
	  MateConfEntry  *entry,
	  gpointer     data)
{
  g_return_if_fail (mateconf_entry_get_key (entry) != NULL);

  if (g_str_equal (mateconf_entry_get_key (entry),
		   MATE_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT)) {
    window_change_mixer_element (MATE_VOLUME_CONTROL_WINDOW (data),
				 mateconf_value_get_string (mateconf_entry_get_value (entry)));
  }
}

/*
 * Signal handlers.
 */

#if 0
static void
cb_error (GstElement *element,
	  GstElement *source,
	  GError     *error,
	  gchar      *debug,
	  gpointer    data)
{
  MateVolumeControlWindow *win = MATE_VOLUME_CONTROL_WINDOW (data);
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (win),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                   error->message);
  gtk_widget_show (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
#endif

static void
mate_volume_control_window_dispose (GObject *object)
{
  MateVolumeControlWindow *win = MATE_VOLUME_CONTROL_WINDOW (object);

  if (win->prefs) {
    gtk_widget_destroy (win->prefs);
    win->prefs = NULL;
  }

  /* clean up */
  if (win->elements) {

    g_list_foreach (win->elements, (GFunc) g_object_unref, NULL);
    g_list_free (win->elements);
    win->elements = NULL;
  }

  if (win->client) {
    g_object_unref (win->client);
    win->client = NULL;
  }

  G_OBJECT_CLASS (mate_volume_control_window_parent_class)->dispose (object);
}


static void
mate_volume_control_window_class_init (MateVolumeControlWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = mate_volume_control_window_dispose;
}


static void
mate_volume_control_window_init (MateVolumeControlWindow *win)
{
  int width, height;

  win->elements = NULL;
  win->el = NULL;
  win->client = mateconf_client_get_default ();
  win->prefs = NULL;
  win->use_default_mixer = FALSE;

  g_set_application_name (_("Volume Control"));
  gtk_window_set_title (GTK_WINDOW (win), _("Volume Control"));

  /* To set the window according to previous geometry */
  width = mateconf_client_get_int (win->client, PREF_UI_WINDOW_WIDTH, NULL);
  if (width < 250)
    width = 250;
  height = mateconf_client_get_int (win->client, PREF_UI_WINDOW_HEIGHT, NULL);
  if (height < 100)
    height = -1;
  gtk_window_set_default_size (GTK_WINDOW (win), width, height);
}

GtkWidget *
mate_volume_control_window_new (GList *elements)
{
  gchar *active_el_str, *cur_el_str;
  GstElement *active_element;
  GList *item;
  MateVolumeControlWindow *win;
  GtkAccelGroup *accel_group;
  GtkWidget *combo_box;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *buttons;
  GtkWidget *el;
  GtkWidget *prefsbtn;
  GtkWidget *closebtn;
  GtkWidget *helpbtn;
  gint count = 0;
  GtkWidget *vbox;
  GtkCellRenderer *renderer;
  gint active_element_num;

  g_return_val_if_fail (elements != NULL, NULL);
  active_element = NULL;

  /* window */
  win = g_object_new (MATE_VOLUME_CONTROL_TYPE_WINDOW, NULL);
  win->elements = elements;

  accel_group = gtk_accel_group_new ();

  gtk_window_add_accel_group (GTK_WINDOW (win), accel_group);
  gtk_accel_group_connect (accel_group, GDK_A, GDK_CONTROL_MASK, 0,
			   g_cclosure_new_swap (G_CALLBACK (cb_show_about), win, NULL));

  /* get active element, if any (otherwise we use the default) */
  active_el_str = mateconf_client_get_string (win->client,
					   MATE_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT,
					   NULL);
  if (active_el_str != NULL && *active_el_str != '\0') {
    for (count = 0, item = elements; item != NULL; item = item->next, count++) {
      cur_el_str = g_object_get_data (item->data, "mate-volume-control-name");
      if (cur_el_str == NULL)
	continue;

      if (g_str_equal (active_el_str, cur_el_str)) {
        active_element = item->data;
        break;
      }
    }
    g_free (active_el_str);
    if (!item) {
      count = 0;
      active_element = elements->data;
      /* If there's a default but it doesn't match what we have available,
       * reset the default */
      mateconf_client_set_string (win->client,
      			       MATE_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT,
      			       g_object_get_data (G_OBJECT (active_element),
      			       			  "mate-volume-control-name"),
      			       NULL);
    }
    /* default element to first */
    if (!active_element)
      active_element = elements->data;
  } else {
    count = 0;
    active_element = elements->data;
  }
  active_element_num = count;

  combo_box = gtk_combo_box_new_text ();
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box), renderer, "text", 0);
  for (count = 0, item = elements; item != NULL; item = item->next, count++) {
    const gchar *name;

    name = g_object_get_data (item->data, "mate-volume-control-name");
    gtk_combo_box_append_text(GTK_COMBO_BOX (combo_box), name);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), active_element_num);
  g_signal_connect (combo_box, "changed", G_CALLBACK (cb_change), win);


  /* mateconf */
  mateconf_client_add_dir (win->client, MATE_VOLUME_CONTROL_KEY_DIR,
			MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
  mateconf_client_notify_add (win->client, MATE_VOLUME_CONTROL_KEY_DIR,
			   cb_mateconf, win, NULL, NULL);

  win->use_default_mixer = (active_el_str == NULL);

  /* add the combo box to choose the device */
  label = gtk_label_new (NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (label), _("_Device: "));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo_box, TRUE, TRUE, 0);

  /* add content for this element */
  el = mate_volume_control_element_new (win->client);
  win->el = MATE_VOLUME_CONTROL_ELEMENT (el);

  /* create the buttons box */
  helpbtn = gtk_button_new_from_stock (GTK_STOCK_HELP);
  prefsbtn = gtk_button_new_from_stock (GTK_STOCK_PREFERENCES);
  closebtn = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (helpbtn, "clicked", G_CALLBACK (cb_help), win);
  g_signal_connect (prefsbtn, "clicked", G_CALLBACK (cb_preferences), win);
  g_signal_connect (closebtn, "clicked", G_CALLBACK (cb_exit), win);
  gtk_widget_add_accelerator (closebtn, "clicked", accel_group,
			      GDK_Escape, 0, 0);
  gtk_widget_add_accelerator (helpbtn, "clicked", accel_group,
			      GDK_F1, 0, 0);
  buttons = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (buttons), helpbtn, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), prefsbtn, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), closebtn, FALSE, FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (buttons), 6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttons), GTK_BUTTONBOX_END);
  gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttons), helpbtn, TRUE);

  /* Put the the elements in a vbox */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(win), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), el, TRUE, TRUE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  /* set tooltips */
  gtk_widget_set_tooltip_text (combo_box, _("Control volume on a different device"));

  gtk_widget_show_all (GTK_WIDGET (win));

  /* refresh the control and window title with the default mixer */
  window_change_mixer_element (win, g_object_get_data (G_OBJECT (active_element),
						       "mate-volume-control-name"));

  /* FIXME:
   * - set error handler (cb_error) after device activation:
   *     g_signal_connect (element, "error", G_CALLBACK (cb_error), win);.
   * - on device change: reset error handler, change menu (in case this
   *     was done outside the UI).
   */

  return GTK_WIDGET (win);
}


