/* mate-screenshot.c - Take a screenshot of the desktop
 *
 * Copyright (C) 2001 Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) 2006 Emmanuele Bassi <ebassi@mate.org>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/* THERE ARE NO FEATURE REQUESTS ALLOWED */
/* IF YOU WANT YOUR OWN FEATURE -- WRITE THE DAMN THING YOURSELF (-: */
/* MAYBE I LIED... -jrb */

#include <config.h>
#include <mateconf/mateconf-client.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <pwd.h>
#include <X11/Xutil.h>
#include <canberra-gtk.h>

#include "screenshot-shadow.h"
#include "screenshot-utils.h"
#include "screenshot-save.h"
#include "screenshot-dialog.h"
#include "screenshot-xfer.h"

#define SCREENSHOOTER_ICON "applets-screenshooter"

#define MATE_SCREENSHOT_MATECONF  "/apps/mate-screenshot"
#define INCLUDE_BORDER_KEY      MATE_SCREENSHOT_MATECONF "/include_border"
#define INCLUDE_POINTER_KEY     MATE_SCREENSHOT_MATECONF "/include_pointer"
#define LAST_SAVE_DIRECTORY_KEY MATE_SCREENSHOT_MATECONF "/last_save_directory"
#define BORDER_EFFECT_KEY       MATE_SCREENSHOT_MATECONF "/border_effect"
#define DELAY_KEY               MATE_SCREENSHOT_MATECONF "/delay"


enum
{
  COLUMN_NICK,
  COLUMN_LABEL,
  COLUMN_ID,

  N_COLUMNS
};

typedef enum {
  SCREENSHOT_EFFECT_NONE,
  SCREENSHOT_EFFECT_SHADOW,
  SCREENSHOT_EFFECT_BORDER
} ScreenshotEffectType;

typedef enum
{
  TEST_LAST_DIR = 0,
  TEST_DESKTOP = 1,
  TEST_TMP = 2,
} TestType;

typedef struct 
{
  char *base_uris[3];
  char *retval;
  int iteration;
  TestType type;
  GdkWindow *window;
  GdkRectangle *rectangle;
} AsyncExistenceJob;

static GdkPixbuf *screenshot = NULL;

/* Global variables*/
static char *last_save_dir = NULL;
static char *window_title = NULL;
static char *temporary_file = NULL;
static gboolean save_immediately = FALSE;

/* Options */
static gboolean take_window_shot = FALSE;
static gboolean take_area_shot = FALSE;
static gboolean include_border = FALSE;
static gboolean include_pointer = TRUE;
static char *border_effect = NULL;
static guint delay = 0;

/* some local prototypes */
static void  display_help           (GtkWindow *parent);
static void  save_done_notification (gpointer   data);
static char *get_desktop_dir        (void);
static void  save_options           (void);

static GtkWidget *border_check = NULL;
static GtkWidget *effect_combo = NULL;
static GtkWidget *effect_label = NULL;
static GtkWidget *effects_vbox = NULL;
static GtkWidget *delay_hbox = NULL;

static void
display_help (GtkWindow *parent)
{
  GError *error = NULL;

  gtk_show_uri (gtk_window_get_screen (parent),
		"ghelp:user-guide#goseditmainmenu-53",
		gtk_get_current_event_time (), &error);

  if (error)
    {
      screenshot_show_gerror_dialog (parent,
                                     _("Error loading the help page"),
                                     error);
      g_error_free (error);
    }
}

static void
interactive_dialog_response_cb (GtkDialog *dialog,
                                gint       response,
                                gpointer   user_data)
{
  switch (response)
    {
    case GTK_RESPONSE_HELP:
      g_signal_stop_emission_by_name (dialog, "response");
      display_help (GTK_WINDOW (dialog));
      break;
    default:
      gtk_widget_hide (GTK_WIDGET (dialog));
      break;
    }
}

#define TARGET_TOGGLE_DESKTOP 0
#define TARGET_TOGGLE_WINDOW  1
#define TARGET_TOGGLE_AREA    2

static void
target_toggled_cb (GtkToggleButton *button,
                   gpointer         data)
{
  int target_toggle = GPOINTER_TO_INT (data);

  if (gtk_toggle_button_get_active (button))
    {
      take_window_shot = (target_toggle == TARGET_TOGGLE_WINDOW);
      take_area_shot = (target_toggle == TARGET_TOGGLE_AREA);
      
      gtk_widget_set_sensitive (border_check, take_window_shot);
      gtk_widget_set_sensitive (effect_combo, take_window_shot);
      gtk_widget_set_sensitive (effect_label, take_window_shot);

      gtk_widget_set_sensitive (delay_hbox, !take_area_shot);
      gtk_widget_set_sensitive (effects_vbox, !take_area_shot);
    }
}

static void
delay_spin_value_changed_cb (GtkSpinButton *button)
{
  delay = gtk_spin_button_get_value_as_int (button);
}

static void
include_border_toggled_cb (GtkToggleButton *button,
                           gpointer         data)
{
  include_border = gtk_toggle_button_get_active (button);
}

static void
include_pointer_toggled_cb (GtkToggleButton *button,
                            gpointer         data)
{
  include_pointer = gtk_toggle_button_get_active (button);
}

static void
effect_combo_changed_cb (GtkComboBox *combo,
                         gpointer     user_data)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      GtkTreeModel *model;
      gchar *effect;

      model = gtk_combo_box_get_model (combo);
      gtk_tree_model_get (model, &iter, COLUMN_NICK, &effect, -1);

      g_assert (effect != NULL);

      g_free (border_effect);
      border_effect = effect; /* gets free'd later */
    }
}

static gint 
key_press_cb (GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  if (event->keyval == GDK_F1)
    {
      display_help (GTK_WINDOW (widget));
      return TRUE;
    }

  return FALSE;
}

typedef struct {
  ScreenshotEffectType id;
  const gchar *label;
  const gchar *nick;
} ScreenshotEffect;

/* Translators:
 * these are the names of the effects available which will be
 * displayed inside a combo box in interactive mode for the user
 * to chooser.
 */
static const ScreenshotEffect effects[] = {
  { SCREENSHOT_EFFECT_NONE, N_("None"), "none" },
  { SCREENSHOT_EFFECT_SHADOW, N_("Drop shadow"), "shadow" },
  { SCREENSHOT_EFFECT_BORDER, N_("Border"), "border" }
};

static guint n_effects = G_N_ELEMENTS (effects);

static GtkWidget *
create_effects_combo (void)
{
  GtkWidget *retval;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  gint i;

  model = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_UINT);
  
  for (i = 0; i < n_effects; i++)
    {
      GtkTreeIter iter;

      gtk_list_store_insert (model, &iter, i);
      gtk_list_store_set (model, &iter,
                          COLUMN_ID, effects[i].id,
                          COLUMN_LABEL, gettext (effects[i].label),
                          COLUMN_NICK, effects[i].nick,
                          -1);
    }

  retval = gtk_combo_box_new ();
  gtk_combo_box_set_model (GTK_COMBO_BOX (retval),
                           GTK_TREE_MODEL (model));
  g_object_unref (model);

  switch (border_effect[0])
    {
    case 's': /* shadow */
      gtk_combo_box_set_active (GTK_COMBO_BOX (retval),
                                SCREENSHOT_EFFECT_SHADOW);
      break;
    case 'b': /* border */
      gtk_combo_box_set_active (GTK_COMBO_BOX (retval),
                                SCREENSHOT_EFFECT_BORDER);
      break;
    case 'n': /* none */
      gtk_combo_box_set_active (GTK_COMBO_BOX (retval),
                                SCREENSHOT_EFFECT_NONE);
      break;
    default:
      break;
    }
  
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (retval), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (retval), renderer,
                                  "text", COLUMN_LABEL,
                                  NULL);

  g_signal_connect (retval, "changed",
                    G_CALLBACK (effect_combo_changed_cb),
                    NULL);

  return retval;
}

static void
create_effects_frame (GtkWidget   *outer_vbox,
                      const gchar *frame_title)
{
  GtkWidget *main_vbox, *vbox, *hbox;
  GtkWidget *align;
  GtkWidget *label;
  GtkWidget *check;
  GtkWidget *combo;
  gchar *title;

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (outer_vbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);
  effects_vbox = main_vbox;

  title = g_strconcat ("<b>", frame_title, "</b>", NULL);
  label = gtk_label_new (title);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  g_free (title);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (align), vbox);
  gtk_widget_show (vbox);

  /** Include pointer **/
  check = gtk_check_button_new_with_mnemonic (_("Include _pointer"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), include_pointer);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (include_pointer_toggled_cb),
                    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  /** Include window border **/
  check = gtk_check_button_new_with_mnemonic (_("Include the window _border"));
  gtk_widget_set_sensitive (check, take_window_shot);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), include_border);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (include_border_toggled_cb),
                    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);
  border_check = check;

  /** Effects **/
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Apply _effect:"));
  gtk_widget_set_sensitive (label, take_window_shot);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  effect_label = label;

  combo = create_effects_combo ();
  gtk_widget_set_sensitive (combo, take_window_shot);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);
  effect_combo = combo;
}

static void
create_screenshot_frame (GtkWidget   *outer_vbox,
                         const gchar *frame_title)
{
  GtkWidget *main_vbox, *vbox, *hbox;
  GtkWidget *align;
  GtkWidget *radio;
  GtkWidget *image;
  GtkWidget *spin;
  GtkWidget *label;
  GtkAdjustment *adjust;
  GSList *group;
  gchar *title;

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (outer_vbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  title = g_strconcat ("<b>", frame_title, "</b>", NULL);
  label = gtk_label_new (title);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  g_free (title);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_widget_set_size_request (align, 48, -1);
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  image = gtk_image_new_from_stock (SCREENSHOOTER_ICON,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (align), image);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /** Grab whole desktop **/
  group = NULL;
  radio = gtk_radio_button_new_with_mnemonic (group,
                                              _("Grab the whole _desktop"));
  if (take_window_shot || take_area_shot)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), FALSE);
  g_signal_connect (radio, "toggled",
                    G_CALLBACK (target_toggled_cb),
                    GINT_TO_POINTER (TARGET_TOGGLE_DESKTOP));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_widget_show (radio);

  /** Grab current window **/
  radio = gtk_radio_button_new_with_mnemonic (group,
                                              _("Grab the current _window"));
  if (take_window_shot)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
  g_signal_connect (radio, "toggled",
                    G_CALLBACK (target_toggled_cb),
                    GINT_TO_POINTER (TARGET_TOGGLE_WINDOW));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  gtk_widget_show (radio);

  /** Grab area of the desktop **/
  radio = gtk_radio_button_new_with_mnemonic (group,
                                              _("Select _area to grab"));
  if (take_area_shot)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
  g_signal_connect (radio, "toggled",
                    G_CALLBACK (target_toggled_cb),
                    GINT_TO_POINTER (TARGET_TOGGLE_AREA));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
  gtk_widget_show (radio);

  /** Grab after delay **/
  delay_hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), delay_hbox, FALSE, FALSE, 0);
  gtk_widget_show (delay_hbox);

  /* translators: this is the first part of the "grab after a
   * delay of <spin button> seconds".
   */
  label = gtk_label_new_with_mnemonic (_("Grab _after a delay of"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (delay_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adjust = GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble) delay,
                                               0.0, 99.0,
                                               1.0,  1.0,
                                               0.0));
  spin = gtk_spin_button_new (adjust, 1.0, 0);
  g_signal_connect (spin, "value-changed",
                    G_CALLBACK (delay_spin_value_changed_cb),
                    NULL);
  gtk_box_pack_start (GTK_BOX (delay_hbox), spin, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin);
  gtk_widget_show (spin);

  /* translators: this is the last part of the "grab after a
   * delay of <spin button> seconds".
   */
  label = gtk_label_new (_("seconds"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_end (GTK_BOX (delay_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
}

static GtkWidget *
create_interactive_dialog (void)
{
  GtkWidget *retval;
  GtkWidget *main_vbox;
  GtkWidget *content_area;

  retval = gtk_dialog_new ();
  gtk_window_set_resizable (GTK_WINDOW (retval), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (retval), 5);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (retval));
  gtk_box_set_spacing (GTK_BOX (content_area), 2);
  gtk_window_set_title (GTK_WINDOW (retval), _("Take Screenshot"));

  /* main container */
  main_vbox = gtk_vbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 5);
  gtk_box_pack_start (GTK_BOX (content_area), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  create_screenshot_frame (main_vbox, _("Take Screenshot"));
  create_effects_frame (main_vbox, _("Effects"));

  gtk_dialog_set_has_separator (GTK_DIALOG (retval), FALSE);
  gtk_dialog_add_buttons (GTK_DIALOG (retval),
                          GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          _("Take _Screenshot"), GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (retval), GTK_RESPONSE_OK);

  /* we need to block on "response" and keep showing the interactive
   * dialog in case the user did choose "help"
   */
  g_signal_connect (retval, "response",
                    G_CALLBACK (interactive_dialog_response_cb),
                    NULL);

  g_signal_connect (G_OBJECT (retval), "key-press-event",
                    G_CALLBACK(key_press_cb), 
                    NULL);

  return retval;
}

static void
save_folder_to_mateconf (ScreenshotDialog *dialog)
{
  MateConfClient *mateconf_client;
  char *folder;

  mateconf_client = mateconf_client_get_default ();

  folder = screenshot_dialog_get_folder (dialog);
  /* Error is NULL, as there's nothing we can do */
  mateconf_client_set_string (mateconf_client,
			   LAST_SAVE_DIRECTORY_KEY, folder,
                           NULL);

  g_free (folder);
  g_object_unref (mateconf_client);
}

static void
set_recent_entry (ScreenshotDialog *dialog)
{
  char *uri, *app_exec = NULL;
  GtkRecentManager *recent;
  GtkRecentData recent_data;
  GAppInfo *app;
  const char *exec_name = NULL;
  static char * groups[2] = { "Graphics", NULL };

  app = g_app_info_get_default_for_type ("image/png", TRUE);

  if (!app) {
    /* return early, as this would be an useless recent entry anyway. */
    return;
  }

  uri = screenshot_dialog_get_uri (dialog);
  recent = gtk_recent_manager_get_default ();
  
  exec_name = g_app_info_get_executable (app);
  app_exec = g_strjoin (" ", exec_name, "%u", NULL);

  recent_data.display_name = NULL;
  recent_data.description = NULL;
  recent_data.mime_type = "image/png";
  recent_data.app_name = "MATE Screenshot";
  recent_data.app_exec = app_exec;
  recent_data.groups = groups;
  recent_data.is_private = FALSE;

  gtk_recent_manager_add_full (recent, uri, &recent_data);

  g_object_unref (app);
  g_free (app_exec);
  g_free (uri);
}

static void
error_dialog_response_cb (GtkDialog *d,
                          gint response,
                          ScreenshotDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (d));
  
  screenshot_dialog_focus_entry (dialog);
}

static void
save_callback (TransferResult result,
               char *error_message,
               gpointer data)
{
  ScreenshotDialog *dialog = data;
  GtkWidget *toplevel;
  
  toplevel = screenshot_dialog_get_toplevel (dialog);
  screenshot_dialog_set_busy (dialog, FALSE);

  if (result == TRANSFER_OK)
    {
      save_folder_to_mateconf (dialog);
      set_recent_entry (dialog);
      gtk_widget_destroy (toplevel);
      
      /* we're done, stop the mainloop now */
      gtk_main_quit ();
    }
  else if (result == TRANSFER_OVERWRITE ||
           result == TRANSFER_CANCELLED)
    {
      /* user has canceled the overwrite dialog or the transfer itself, let him
       * choose another name.
       */
      screenshot_dialog_focus_entry (dialog);
    }
  else /* result == TRANSFER_ERROR */
    {
      /* we had an error, display a dialog to the user and let him choose
       * another name/location to save the screenshot.
       */
      GtkWidget *error_dialog;
      char *uri;
      
      uri = screenshot_dialog_get_uri (dialog);
      error_dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("Error while saving screenshot"));
      /* translators: first %s is the file path, second %s is the VFS error */
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
                                                _("Impossible to save the screenshot "
                                                  "to %s.\n Error was %s.\n Please choose another "
                                                  "location and retry."), uri, error_message);
      gtk_widget_show (error_dialog);
      g_signal_connect (error_dialog,
                        "response",
                        G_CALLBACK (error_dialog_response_cb),
                        dialog);

      g_free (uri);
    }
      
}

static void
try_to_save (ScreenshotDialog *dialog,
	     const char       *target)
{
  GFile *source_file, *target_file;

  g_assert (temporary_file);

  screenshot_dialog_set_busy (dialog, TRUE);

  source_file = g_file_new_for_path (temporary_file);
  target_file = g_file_new_for_uri (target);
  
  screenshot_xfer_uri (source_file,
                       target_file,
                       screenshot_dialog_get_toplevel (dialog),
                       save_callback, dialog);
  
  /* screenshot_xfer_uri () holds a ref, so we can unref now */
  g_object_unref (source_file);
  g_object_unref (target_file);
}

static void
save_done_notification (gpointer data)
{
  ScreenshotDialog *dialog = data;

  temporary_file = g_strdup (screenshot_save_get_filename ());
  screenshot_dialog_enable_dnd (dialog);

  if (save_immediately)
    {
      GtkWidget *toplevel;

      toplevel = screenshot_dialog_get_toplevel (dialog);
      gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_OK);
    }
}

static void
screenshot_dialog_response_cb (GtkDialog *d,
                               gint response_id,
                               ScreenshotDialog *dialog)
{
  char *uri;

  if (response_id == GTK_RESPONSE_HELP)
    {
      display_help (GTK_WINDOW (d));
    }
  else if (response_id == GTK_RESPONSE_OK)
    {
      uri = screenshot_dialog_get_uri (dialog);
      if (temporary_file == NULL)
        {
          save_immediately = TRUE;
          screenshot_dialog_set_busy (dialog, TRUE);
        }
      else
        {
          /* we've saved the temporary file, lets try to copy it to the
           * correct location.
           */
          try_to_save (dialog, uri);
        }
      g_free (uri);
    }
  else if (response_id == SCREENSHOT_RESPONSE_COPY)
    {
      GtkClipboard *clipboard;
      GdkPixbuf    *screenshot;

      clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (d)),
                                                 GDK_SELECTION_CLIPBOARD);
      screenshot = screenshot_dialog_get_screenshot (dialog);
      gtk_clipboard_set_image (clipboard, screenshot);
    }
  else /* dialog was canceled */
    {
      gtk_widget_destroy (GTK_WIDGET (d));
      gtk_main_quit ();
    }
}
                               

static void
run_dialog (ScreenshotDialog *dialog)
{
  GtkWidget *toplevel;

  toplevel = screenshot_dialog_get_toplevel (dialog);
  
  gtk_widget_show (toplevel);
  
  g_signal_connect (toplevel,
                    "response",
                    G_CALLBACK (screenshot_dialog_response_cb),
                    dialog);
}

static void
play_sound_effect (GdkWindow *window)
{
  ca_context *c;
  ca_proplist *p = NULL;
  int res;

  c = ca_gtk_context_get ();

  res = ca_proplist_create (&p);
  if (res < 0)
    goto done;

  res = ca_proplist_sets (p, CA_PROP_EVENT_ID, "screen-capture");
  if (res < 0)
    goto done;

  res = ca_proplist_sets (p, CA_PROP_EVENT_DESCRIPTION, _("Screenshot taken"));
  if (res < 0)
    goto done;

  if (window != NULL)
    {
      res = ca_proplist_setf (p,
                              CA_PROP_WINDOW_X11_XID,
                              "%lu",
                              (unsigned long) GDK_WINDOW_XID (window));
      if (res < 0)
        goto done;
    }

  ca_context_play_full (c, 0, p, NULL, NULL);

 done:
  if (p != NULL)
    ca_proplist_destroy (p);

}

static void
finish_prepare_screenshot (char *initial_uri, GdkWindow *window, GdkRectangle *rectangle)
{  
  ScreenshotDialog *dialog;

  /* always disable window border for full-desktop or selected-area screenshots */
  if (!take_window_shot)
    screenshot = screenshot_get_pixbuf (window, rectangle, include_pointer, FALSE);
  else
    {
      screenshot = screenshot_get_pixbuf (window, rectangle, include_pointer, include_border);

      switch (border_effect[0])
        {
        case 's': /* shadow */
          screenshot_add_shadow (&screenshot);
          break;
        case 'b': /* border */
          screenshot_add_border (&screenshot);
          break;
        case 'n': /* none */
        default:
          break;
        }
    }

  /* release now the lock, it was acquired when we were finding the window */
  screenshot_release_lock ();

  if (screenshot == NULL)
    {
      screenshot_show_error_dialog (NULL,
                                    _("Unable to take a screenshot of the current window"),
                                    NULL);
      exit (1);
    }

  play_sound_effect (window);

  dialog = screenshot_dialog_new (screenshot, initial_uri, take_window_shot);
  g_free (initial_uri);

  screenshot_save_start (screenshot, save_done_notification, dialog);

  run_dialog (dialog);
}

static void
async_existence_job_free (AsyncExistenceJob *job)
{
  if (!job)
    return;

  g_free (job->base_uris[1]);
  g_free (job->base_uris[2]);
  g_free (job->rectangle);
  g_slice_free (AsyncExistenceJob, job);
}

static gboolean
check_file_done (gpointer user_data)
{
  AsyncExistenceJob *job = user_data;

  finish_prepare_screenshot (job->retval, job->window, job->rectangle);

  async_existence_job_free (job);
  
  return FALSE;
}

static char *
build_uri (AsyncExistenceJob *job)
{
  char *retval, *file_name;

  if (window_title)
    {
      /* translators: this is the name of the file that gets made up
       * with the screenshot if a specific window is taken */
      if (job->iteration == 0)
        {
          file_name = g_strdup_printf (_("Screenshot-%s.png"), window_title);
        }
      else
        {
          /* translators: this is the name of the file that gets
           * made up with the screenshot if a specific window is
           * taken */
          file_name = g_strdup_printf (_("Screenshot-%s-%d.png"),
                                       window_title, job->iteration);
        }
    }
  else
    {
      if (job->iteration == 0)
        {
          /* translators: this is the name of the file that gets made up
           * with the screenshot if the entire screen is taken */
          file_name = g_strdup (_("Screenshot.png"));
        }
      else
        {
          /* translators: this is the name of the file that gets
           * made up with the screenshot if the entire screen is
           * taken */
          file_name = g_strdup_printf (_("Screenshot-%d.png"), job->iteration);
        }
    }

  retval = g_build_filename (job->base_uris[job->type], file_name, NULL);
  g_free (file_name);

  return retval;
}

static gboolean
try_check_file (GIOSchedulerJob *io_job,
                GCancellable *cancellable,
                gpointer data)
{
  AsyncExistenceJob *job = data;
  GFile *file;
  GFileInfo *info;
  GError *error;
  char *uri;

retry:
  error = NULL;
  uri = build_uri (job);
  file = g_file_new_for_uri (uri);

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE,
			    G_FILE_QUERY_INFO_NONE, cancellable, &error);
  if (info != NULL)
    {
      /* file already exists, iterate again */
      g_object_unref (info);
      g_object_unref (file);
      g_free (uri);

      (job->iteration)++;

      goto retry;
    }
  else
    {
      /* see the error to check whether the location is not accessible
       * or the file does not exist.
       */
      if (error->code == G_IO_ERROR_NOT_FOUND)
        {
          GFile *parent;

          /* if the parent directory doesn't exist as well, forget the saved
           * directory and treat this as a generic error.
           */

          parent = g_file_get_parent (file);

          if (!g_file_query_exists (parent, NULL))
            {
              (job->type)++;
              job->iteration = 0;

              g_object_unref (file);
              g_object_unref (parent);
              goto retry;
            }
          else
            {
              job->retval = uri;

              g_object_unref (parent);
              goto out;
            }
        }
      else
        {
          /* another kind of error, assume this location is not
           * accessible.
           */
          g_free (uri);
          if (job->type == TEST_TMP)
            {
              job->retval = NULL;
              goto out;
            }
          else
            {
              (job->type)++;
              job->iteration = 0;

              g_error_free (error);
              g_object_unref (file);
              goto retry;
            }
        }
    }

out:
  g_error_free (error);
  g_object_unref (file);

  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             check_file_done,
                                             job,
                                             NULL);
  return FALSE;
}

static GdkWindow *
find_current_window (char **window_title)
{
  GdkWindow *window;

  if (!screenshot_grab_lock ())
    exit (0);

  if (take_window_shot)
    {
      window = screenshot_find_current_window ();
      if (!window)
	{
	  take_window_shot = FALSE;
	  window = gdk_get_default_root_window ();
	}
      else
	{
	  gchar *tmp, *sanitized;

	  tmp = screenshot_get_window_title (window);
	  sanitized = screenshot_sanitize_filename (tmp);
	  g_free (tmp);
	  *window_title = sanitized;
	}
    }
  else
    {
      window = gdk_get_default_root_window ();
    }

  return window;
}

static GdkRectangle *
find_rectangle (void)
{
  GdkRectangle *rectangle;

  if (!take_area_shot)
    return NULL;

  rectangle = g_new0 (GdkRectangle, 1);
  if (screenshot_select_area (&rectangle->x, &rectangle->y,
                              &rectangle->width, &rectangle->height))
    {
      g_assert (rectangle->width >= 0);
      g_assert (rectangle->height >= 0);

      return rectangle;
    }
  else
    {
      g_free (rectangle);
      return NULL;
    }
}

static void
prepare_screenshot (void)
{
  AsyncExistenceJob *job;
  
  job = g_slice_new0 (AsyncExistenceJob);
  job->base_uris[0] = last_save_dir;
  /* we'll have to free these two */
  job->base_uris[1] = get_desktop_dir ();
  job->base_uris[2] = g_strconcat ("file://", g_get_tmp_dir (), NULL);
  job->iteration = 0;
  job->type = TEST_LAST_DIR;
  job->window = find_current_window (&window_title);
  job->rectangle = find_rectangle ();

  /* Check if the area selection was cancelled */
  if (job->rectangle &&
      (job->rectangle->width == 0 || job->rectangle->height == 0))
    {
      async_existence_job_free (job);
      gtk_main_quit ();
      return;
    }

  g_io_scheduler_push_job (try_check_file,
                           job,
                           NULL,
                           0, NULL);
                           
}

static gboolean
prepare_screenshot_timeout (gpointer data)
{
  prepare_screenshot ();
  save_options ();

  return FALSE;
}


static gchar *
get_desktop_dir (void)
{
  MateConfClient *mateconf_client;
  gboolean desktop_is_home_dir = FALSE;
  gchar *desktop_dir;

  mateconf_client = mateconf_client_get_default ();
  desktop_is_home_dir = mateconf_client_get_bool (mateconf_client,
                                               "/apps/caja/preferences/desktop_is_home_dir",
                                               NULL);
  if (desktop_is_home_dir)
    desktop_dir = g_strconcat ("file://", g_get_home_dir (), NULL);
  else
    desktop_dir = g_strconcat ("file://", g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP), NULL);

  g_object_unref (mateconf_client);

  return desktop_dir;
}

/* Taken from mate-vfs-utils.c */
static char *
expand_initial_tilde (const char *path)
{
  char *slash_after_user_name, *user_name;
  struct passwd *passwd_file_entry;

  if (path[1] == '/' || path[1] == '\0') {
    return g_strconcat (g_get_home_dir (), &path[1], NULL);
  }
  
  slash_after_user_name = strchr (&path[1], '/');
  if (slash_after_user_name == NULL) {
    user_name = g_strdup (&path[1]);
  } else {
    user_name = g_strndup (&path[1],
                           slash_after_user_name - &path[1]);
  }
  passwd_file_entry = getpwnam (user_name);
  g_free (user_name);
  
  if (passwd_file_entry == NULL || passwd_file_entry->pw_dir == NULL) {
    return g_strdup (path);
  }
  
  return g_strconcat (passwd_file_entry->pw_dir,
                      slash_after_user_name,
                      NULL);
}

/* Load options */
static void
load_options (void)
{
  MateConfClient *mateconf_client;

  mateconf_client = mateconf_client_get_default ();

  /* Find various dirs */
  last_save_dir = mateconf_client_get_string (mateconf_client,
                                           LAST_SAVE_DIRECTORY_KEY,
                                           NULL);
  if (!last_save_dir || !last_save_dir[0])
    {
      last_save_dir = get_desktop_dir ();
    }
  else if (last_save_dir[0] == '~')
    {
      char *tmp = expand_initial_tilde (last_save_dir);
      g_free (last_save_dir);
      last_save_dir = tmp;
    }

  include_border = mateconf_client_get_bool (mateconf_client,
                                          INCLUDE_BORDER_KEY,
                                          NULL);

  include_pointer = mateconf_client_get_bool (mateconf_client,
                                          INCLUDE_POINTER_KEY,
                                          NULL);

  border_effect = mateconf_client_get_string (mateconf_client,
                                           BORDER_EFFECT_KEY,
                                           NULL);
  if (!border_effect)
    border_effect = g_strdup ("none");

  delay = mateconf_client_get_int (mateconf_client, DELAY_KEY, NULL);

  g_object_unref (mateconf_client);
}

static void
save_options (void)
{
  MateConfClient *mateconf_client;

  mateconf_client = mateconf_client_get_default ();

  /* Error is NULL, as there's nothing we can do */

  mateconf_client_set_bool (mateconf_client,
                         INCLUDE_BORDER_KEY, include_border,
                         NULL);
  mateconf_client_set_bool (mateconf_client,
                         INCLUDE_POINTER_KEY, include_pointer,
                         NULL);
  mateconf_client_set_int (mateconf_client, DELAY_KEY, delay, NULL);
  mateconf_client_set_string (mateconf_client,
                           BORDER_EFFECT_KEY, border_effect,
                           NULL);
 
  g_object_unref (mateconf_client);
}


static void
register_screenshooter_icon (GtkIconFactory * factory)
{
  GtkIconSource *source;
  GtkIconSet *icon_set;

  source = gtk_icon_source_new ();
  gtk_icon_source_set_icon_name (source, SCREENSHOOTER_ICON);

  icon_set = gtk_icon_set_new ();
  gtk_icon_set_add_source (icon_set, source);

  gtk_icon_factory_add (factory, SCREENSHOOTER_ICON, icon_set);
  gtk_icon_set_unref (icon_set);
  gtk_icon_source_free (source);
}

static void
screenshooter_init_stock_icons (void)
{
  GtkIconFactory *factory;

  factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (factory);

  register_screenshooter_icon (factory);
  g_object_unref (factory);
}

/* main */
int
main (int argc, char *argv[])
{
  GOptionContext *context;
  gboolean window_arg = FALSE;
  gboolean area_arg = FALSE;
  gboolean include_border_arg = FALSE;
  gboolean disable_border_arg = FALSE;
  gboolean interactive_arg = FALSE;
  gchar *border_effect_arg = NULL;
  guint delay_arg = 0;
  GError *error = NULL;

  const GOptionEntry entries[] = {
    { "window", 'w', 0, G_OPTION_ARG_NONE, &window_arg, N_("Grab a window instead of the entire screen"), NULL },
    { "area", 'a', 0, G_OPTION_ARG_NONE, &area_arg, N_("Grab an area of the screen instead of the entire screen"), NULL },
    { "include-border", 'b', 0, G_OPTION_ARG_NONE, &include_border_arg, N_("Include the window border with the screenshot"), NULL },
    { "remove-border", 'B', 0, G_OPTION_ARG_NONE, &disable_border_arg, N_("Remove the window border from the screenshot"), NULL },
    { "delay", 'd', 0, G_OPTION_ARG_INT, &delay_arg, N_("Take screenshot after specified delay [in seconds]"), N_("seconds") },
    { "border-effect", 'e', 0, G_OPTION_ARG_STRING, &border_effect_arg, N_("Effect to add to the border (shadow, border or none)"), N_("effect") },
    { "interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive_arg, N_("Interactively set options"), NULL },
    { NULL },
  };

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_thread_init (NULL);

  context = g_option_context_new (_("Take a picture of the screen"));
  g_option_context_set_ignore_unknown_options (context, FALSE);
  g_option_context_set_help_enabled (context, TRUE);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  g_option_context_parse (context, &argc, &argv, &error);

  if (error) {
    g_critical ("Unable to parse arguments: %s", error->message);
    g_error_free (error);
    g_option_context_free (context);
    exit (1);
  }

  g_option_context_free (context);

  if (window_arg && area_arg) {
    g_printerr (_("Conflicting options: --window and --area should not be "
                  "used at the same time.\n"));
    exit (1);
  }

  gtk_window_set_default_icon_name (SCREENSHOOTER_ICON);
  screenshooter_init_stock_icons ();

  load_options ();
  /* allow the command line to override options */
  if (window_arg)
    take_window_shot = TRUE;

  if (area_arg)
    take_area_shot = TRUE;

  if (include_border_arg)
    include_border = TRUE;

  if (disable_border_arg)
    include_border = FALSE;

  if (border_effect_arg)
    {
      g_free (border_effect);
      border_effect = border_effect_arg;
    }

  if (delay_arg > 0)
    delay = delay_arg;

  /* interactive mode overrides everything */
  if (interactive_arg)
    {
      GtkWidget *dialog;
      gint response;

      dialog = create_interactive_dialog ();
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      switch (response)
        {
        case GTK_RESPONSE_DELETE_EVENT:
        case GTK_RESPONSE_CANCEL:
          return EXIT_SUCCESS;
        case GTK_RESPONSE_OK:
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }

  if (((delay > 0 && interactive_arg) || delay_arg > 0) &&
      !take_area_shot)
    {      
      g_timeout_add (delay * 1000,
		     prepare_screenshot_timeout,
		     NULL);
    }
  else
    {
      /* start this in an idle anyway and fire up the mainloop */
      g_idle_add (prepare_screenshot_timeout, NULL);
    }

  gtk_main ();

  return EXIT_SUCCESS;
}
