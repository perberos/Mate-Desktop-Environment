/* MATE GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include "mate-dialog.h"
#include <libmate/mate-util.h>
#include <libmate/mate-config.h>
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "mate-uidefs.h"
#include "mate-dialog-util.h"
#include "mate-marshal.h"

enum {
  CLICKED,
  CLOSE,
  LAST_SIGNAL
};

static void mate_dialog_class_init       (MateDialogClass *klass);
static void mate_dialog_init             (MateDialog * dialog);
static void mate_dialog_init_action_area (MateDialog * dialog);


static void mate_dialog_button_clicked (GtkWidget   *button,
					 GtkWidget   *messagebox);
static gint mate_dialog_key_pressed (GtkWidget * d, GdkEventKey * e);
static gint mate_dialog_delete_event (GtkWidget * d, GdkEventAny * e);
static void mate_dialog_destroy (GtkObject *object);
static void mate_dialog_finalize (GObject *object);
static void mate_dialog_show (GtkWidget * d);
static void mate_dialog_close_real(MateDialog * d);

static GtkWindowClass *parent_class;
static gint dialog_signals[LAST_SIGNAL] = { 0, 0 };

GType
mate_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (G_UNLIKELY (dialog_type == 0))
    {
      const GTypeInfo dialog_info =
      {
	sizeof (MateDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
	(GClassInitFunc) mate_dialog_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,			/* class_data */
	sizeof (MateDialog),
	0,			/* n_preallocs */
	(GInstanceInitFunc) mate_dialog_init,
	NULL			/* value_table */
      };

      dialog_type = g_type_register_static (GTK_TYPE_WINDOW,
					    "MateDialog",
					    &dialog_info, 0);
    }

  return dialog_type;
}

static void
mate_dialog_class_init (MateDialogClass *klass)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  gobject_class = (GObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  parent_class = g_type_class_peek_parent (klass);

  dialog_signals[CLOSE] =
    g_signal_new ("close",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (MateDialogClass, close),
		  NULL, NULL,
		  _mate_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  dialog_signals[CLICKED] =
    g_signal_new ("clicked",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (MateDialogClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  klass->clicked = NULL;
  klass->close = NULL;
  object_class->destroy = mate_dialog_destroy;
  gobject_class->finalize = mate_dialog_finalize;
  widget_class->key_press_event = mate_dialog_key_pressed;
  widget_class->delete_event = mate_dialog_delete_event;
  widget_class->show = mate_dialog_show;
}

static void
mate_dialog_init (MateDialog *dialog)
{
  GtkWidget * vbox;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  dialog->just_hide = FALSE;
  dialog->click_closes = FALSE;
  dialog->buttons = NULL;

  /* FIXME:!!!!!!!!!!!!!! */
  GTK_WINDOW(dialog)->type = 1;
  gtk_window_set_position(GTK_WINDOW(dialog), 1);

  /*
  GTK_WINDOW(dialog)->type = mate_preferences_get_dialog_type();
  gtk_window_set_position(GTK_WINDOW(dialog),
		      mate_preferences_get_dialog_position());
		      */

  dialog->accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group (GTK_WINDOW(dialog),
			      dialog->accelerators);

  vbox = gtk_vbox_new(FALSE, MATE_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox),
			      MATE_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);
  gtk_widget_show(vbox);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  dialog->vbox = gtk_vbox_new(FALSE, MATE_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->vbox,
		      TRUE, TRUE,
		      MATE_PAD_SMALL);
  gtk_widget_show(dialog->vbox);
}

static void
mate_dialog_init_action_area (MateDialog * dialog)
{
  GtkWidget * separator;

  if (dialog->action_area)
    return;

  dialog->action_area = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->action_area),
			     GTK_BUTTONBOX_END);

  gtk_box_pack_end (GTK_BOX (dialog->vbox), dialog->action_area,
		    FALSE, TRUE, 0);
  gtk_widget_show (dialog->action_area);

  separator = gtk_hseparator_new ();
  gtk_box_pack_end (GTK_BOX (dialog->vbox), separator,
		      FALSE, TRUE,
		      MATE_PAD_SMALL);
  gtk_widget_show (separator);
}


/**
 * mate_dialog_construct: Functionality of mate_dialog_new() for language wrappers.
 * @dialog: Dialog to construct.
 * @title: Title of the dialog.
 * @ap: va_list of buttons, NULL-terminated.
 *
 * See mate_dialog_new().
 **/
void
mate_dialog_construct (MateDialog * dialog,
			const gchar * title,
			va_list ap)
{
  gchar * button_name;

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  while (TRUE) {
    button_name = va_arg (ap, gchar *);

    if (button_name == NULL) {
      break;
    }

    mate_dialog_append_button( dialog,
				  button_name);
  }

  /* argument list may be null if the user wants to do weird things to the
   * dialog, but we need to make sure this is initialized */
  mate_dialog_init_action_area(dialog);
}

/**
 * mate_dialog_constructv: Functionality of mate_dialog_new(), for language wrappers.
 * @dialog: Dialog to construct.
 * @title: Title of the dialog.
 * @buttons: NULL-terminated array of buttons.
 *
 * See mate_dialog_new().
 **/
void mate_dialog_constructv (MateDialog * dialog,
			      const gchar * title,
			      const gchar ** buttons)
{
  const gchar * button_name;

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  if (buttons) {
    while (TRUE) {

      button_name = *buttons++;

      if (button_name == NULL) {
	break;
      }

      mate_dialog_append_button( dialog,
				  button_name);
    };
  }

  /* argument list may be null if the user wants to do weird things to the
   * dialog, but we need to make sure this is initialized */
  mate_dialog_init_action_area(dialog);
}



/**
 * mate_dialog_new: Create a new #MateDialog.
 * @title: The title of the dialog; appears in window titlebar.
 * @...: NULL-terminated varargs list of button names or MATE_STOCK_BUTTON_* defines.
 *
 * Creates a new #MateDialog, with the given title, and any button names
 * in the arg list. Buttons can be simple names, such as _("My Button"),
 * or mate-stock defines such as %MATE_STOCK_BUTTON_OK, etc. The last
 * argument should be NULL to terminate the list.
 *
 * Buttons passed to this function are numbered from left to right,
 * starting with 0. So the first button in the arglist is button 0,
 * then button 1, etc.  These numbers are used throughout the
 * #MateDialog API.
 *
 * Return value: The new #MateDialog.
 **/
GtkWidget* mate_dialog_new            (const gchar * title,
					...)
{
  va_list ap;
  MateDialog *dialog;

  dialog = g_object_new (MATE_TYPE_DIALOG, NULL);

  va_start (ap, title);

  mate_dialog_construct(dialog, title, ap);

  va_end(ap);

  return GTK_WIDGET (dialog);
}

/**
 * mate_dialog_newv: Create a new #MateDialog.
 * @title: Title of the dialog.
 * @buttons: NULL-terminated vector of buttons names.
 *
 * See mate_dialog_new(), this function is identical but does not use
 * varargs.
 *
 * Return value: The new #MateDialog.
 **/
GtkWidget* mate_dialog_newv            (const gchar * title,
					 const gchar ** buttons)
{
  MateDialog *dialog;

  dialog = g_object_new (MATE_TYPE_DIALOG, NULL);

  mate_dialog_constructv(dialog, title, buttons);

  return GTK_WIDGET (dialog);
}

/**
 * mate_dialog_set_parent: Set the logical parent window of a #MateDialog.
 * @dialog: #MateDialog to set the parent of.
 * @parent: Parent #GtkWindow.
 *
 * Dialogs have "parents," usually the main application window which spawned
 * them. This function will let the window manager know about the parent-child
 * relationship. Usually this means the dialog must stay on top of the parent,
 * and will be minimized when the parent is. Mate also allows users to
 * request dialog placement above the parent window (vs. at the mouse position,
 * or at a default window manger location).
 *
 **/
void       mate_dialog_set_parent     (MateDialog * dialog,
					GtkWindow   * parent)
{
  /* This code is duplicated in mate-file-entry.c:browse-clicked.  If
   * a change is made here, update it there too. */
  /* Also, It might be good at some point to make the first argument
   * GtkWidget, instead of MateDialog */
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));
  g_return_if_fail(parent != NULL);
  g_return_if_fail(GTK_IS_WINDOW(parent));
  g_return_if_fail(parent != GTK_WINDOW(dialog));

  gtk_window_set_transient_for (GTK_WINDOW(dialog), parent);
}


/**
 * mate_dialog_append_buttons: Add buttons to a dialog after its initial construction.
 * @dialog: #MateDialog to add buttons to.
 * @first: First button to add.
 * @...: varargs list of additional buttons, NULL-terminated.
 *
 * This function is mostly for internal library use. You should use
 * mate_dialog_new() instead. See that function for a description of
 * the button arguments.
 *
 **/
void       mate_dialog_append_buttons (MateDialog * dialog,
					const gchar * first,
					...)
{
  va_list ap;
  const gchar * button_name = first;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  va_start(ap, first);

  while(button_name != NULL) {
    mate_dialog_append_button (dialog, button_name);
    button_name = va_arg (ap, gchar *);
  }
  va_end(ap);
}

/**
 * mate_dialog_append_button:
 * @dialog: #MateDialog to add button to.
 * @button_name: Button to add.
 *
 * Add a button to a dialog after its initial construction.
 * This function is mostly for internal library use. You should use
 * mate_dialog_new() instead. See that function for a description of
 * the button argument.
 *
 **/
void       mate_dialog_append_button (MateDialog * dialog,
				       const gchar * button_name)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  if (button_name != NULL) {
    GtkWidget *button;

    mate_dialog_init_action_area (dialog);

    button = gtk_button_new_from_stock (button_name);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE, 0);

    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    g_signal_connect_after (button, "clicked",
			    G_CALLBACK (mate_dialog_button_clicked), dialog);

    dialog->buttons = g_list_append (dialog->buttons, button);
  }
}

#define MATE_STOCK_BUTTON_PADDING 2
static GtkWidget *
mate_pixmap_button(GtkWidget *pixmap, const char *text)
{
	GtkWidget *button, *label, *hbox, *w;
	gboolean use_icon, use_label;

	g_return_val_if_fail(text != NULL, NULL);

	button = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(button), hbox);
	w = hbox;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(w), hbox, TRUE, FALSE,
                           MATE_STOCK_BUTTON_PADDING);

	use_icon = mate_config_get_bool("/Mate/Icons/ButtonUseIcons=true");
	use_label = mate_config_get_bool("/Mate/Icons/ButtonUseLabels=true");

	if ((use_label) || (!use_icon) || (!pixmap)) {
		label = gtk_label_new(_(text));
		gtk_widget_show(label);
		gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE,
                                 MATE_STOCK_BUTTON_PADDING);
	}

	if ((use_icon) && (pixmap)) {

		gtk_widget_show(pixmap);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap,
				   FALSE, FALSE, 0);
	} else {
                g_object_unref(pixmap);
        }

	return button;
}



/**
 * mate_dialog_append_button_with_pixmap:
 * @dialog: #MateDialog to add the button to.
 * @button_name: Name of the button, or stock button #define.
 * @pixmap_name: Stock pixmap name.
 *
 * Add a pixmap button to a dialog.
 * mate_dialog_new() does not permit custom buttons with pixmaps, so if you
 * want one of those you need to use this function.
 *
 **/
void       mate_dialog_append_button_with_pixmap (MateDialog * dialog,
						   const gchar * button_name,
						   const gchar * pixmap_name)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  if (button_name != NULL) {
    GtkWidget *button;

    if (pixmap_name != NULL) {
      GtkWidget *pixmap;

      pixmap = gtk_image_new_from_stock (pixmap_name, GTK_ICON_SIZE_BUTTON);
      button = mate_pixmap_button (pixmap, button_name);
    } else {
      button = gtk_button_new_from_stock (button_name);
    }

    mate_dialog_init_action_area (dialog);

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE, 0);

    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    g_signal_connect_after (button, "clicked",
			    G_CALLBACK (mate_dialog_button_clicked), dialog);

    dialog->buttons = g_list_append (dialog->buttons, button);
  }
}

/**
 * mate_dialog_append_buttonsv: Like mate_dialog_append_buttons(), but with a vector arg instead of a varargs list.
 * @dialog: #MateDialog to append to.
 * @buttons: NULL-terminated vector of buttons to append.
 *
 * For internal use, language bindings, etc. Use mate_dialog_new() instead.
 *
 **/
void       mate_dialog_append_buttonsv (MateDialog * dialog,
					 const gchar ** buttons)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  while(*buttons != NULL) {
    mate_dialog_append_button (dialog, *buttons);
    buttons++;
  }
}

/**
 * mate_dialog_append_buttons_with_pixmaps: Like mate_dialog_append_button_with_pixmap(), but allows multiple buttons.
 * @dialog: #MateDialog to append to.
 * @names: NULL-terminated vector of button names.
 * @pixmaps: NULL-terminated vector of pixmap names.
 *
 * Simply calls mate_dialog_append_button_with_pixmap() repeatedly.
 *
 **/
void       mate_dialog_append_buttons_with_pixmaps (MateDialog * dialog,
						     const gchar **names,
						     const gchar **pixmaps)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  while(*names != NULL) {
    mate_dialog_append_button_with_pixmap (dialog, *names, *pixmaps);
    names++; pixmaps++;
  }
}

struct MateDialogRunInfo {
  gint button_number;
  gint close_id, clicked_id, destroy_id;
  gboolean destroyed;
  GMainLoop *mainloop;
};

static void
mate_dialog_shutdown_run(MateDialog* dialog,
                          struct MateDialogRunInfo* runinfo)
{
  if (!runinfo->destroyed)
    {
      g_signal_handler_disconnect(dialog, runinfo->close_id);
      g_signal_handler_disconnect(dialog, runinfo->clicked_id);

      runinfo->close_id = runinfo->clicked_id = -1;
    }

  if (runinfo->mainloop)
    {
      g_main_loop_quit (runinfo->mainloop);
      g_main_loop_unref (runinfo->mainloop);
      runinfo->mainloop = NULL;
    }
}

static void
mate_dialog_setbutton_callback(MateDialog *dialog,
				gint button_number,
				struct MateDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return;

  runinfo->button_number = button_number;

  mate_dialog_shutdown_run(dialog, runinfo);
}

static gboolean
mate_dialog_quit_run(MateDialog *dialog,
		      struct MateDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return FALSE;

  mate_dialog_shutdown_run(dialog, runinfo);

  return FALSE;
}

static void
mate_dialog_mark_destroy(MateDialog* dialog,
                          struct MateDialogRunInfo* runinfo)
{
  runinfo->destroyed = TRUE;

  if(runinfo->close_id < 0)
    return;
  else mate_dialog_shutdown_run(dialog, runinfo);
}

static gint
mate_dialog_run_real(MateDialog* dialog, gboolean close_after)
{
  gboolean was_modal;
  struct MateDialogRunInfo ri = {-1,-1,-1,-1,FALSE,NULL};

  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(MATE_IS_DIALOG(dialog), -1);

  was_modal = GTK_WINDOW(dialog)->modal;
  if (!was_modal)
    gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

  /* There are several things that could happen to the dialog, and we
     need to handle them all: click, delete_event, close, destroy */

  ri.clicked_id =
    g_signal_connect(dialog, "clicked",
		     G_CALLBACK (mate_dialog_setbutton_callback), &ri);

  ri.close_id =
    g_signal_connect(dialog, "close",
		     G_CALLBACK (mate_dialog_quit_run), &ri);

  ri.destroy_id =
    g_signal_connect(dialog, "destroy",
		     G_CALLBACK(mate_dialog_mark_destroy), &ri);

  if ( ! GTK_WIDGET_VISIBLE(GTK_WIDGET(dialog)) )
    gtk_widget_show(GTK_WIDGET(dialog));

  ri.mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (ri.mainloop);

  g_assert(ri.mainloop == NULL);

  if(!ri.destroyed) {

    g_signal_handler_disconnect(dialog, ri.destroy_id);

    if(!was_modal)
      {
	gtk_window_set_modal(GTK_WINDOW(dialog),FALSE);
      }

    if(ri.close_id >= 0) /* We didn't shut down the run? */
      {
	g_signal_handler_disconnect(dialog, ri.close_id);
	g_signal_handler_disconnect(dialog, ri.clicked_id);
      }

    if (close_after)
      {
        mate_dialog_close(dialog);
      }
  }

  return ri.button_number;
}

/**
 * mate_dialog_run: Make the dialog modal and block waiting for user response.
 * @dialog: #MateDialog to use.
 *
 * Blocks until the user clicks a button, or closes the dialog with the
 * window manager's close decoration (or by pressing Escape).
 *
 * You need to set up the dialog to do the right thing when a button
 * is clicked or delete_event is received; you must consider both of
 * those possibilities so that you know the status of the dialog when
 * mate_dialog_run() returns. A common mistake is to forget about
 * Escape and the window manager close decoration; by default, these
 * call mate_dialog_close(), which by default destroys the dialog. If
 * your button clicks do not destroy the dialog, you don't know
 * whether the dialog is destroyed when mate_dialog_run()
 * returns. This is bad.
 *
 * So you should either close the dialog on button clicks as well, or
 * change the mate_dialog_close() behavior to hide instead of
 * destroy. You can do this with mate_dialog_close_hides().
 *
 * Return value:  If a button was pressed, the button number is returned. If not, -1 is returned.
 **/
gint
mate_dialog_run(MateDialog *dialog)
{
  return mate_dialog_run_real(dialog,FALSE);
}

/**
 * mate_dialog_run_and_close: Like mate_dialog_run(), but force-closes the dialog after the run, iff the dialog was not closed already.
 * @dialog: #MateDialog to use.
 *
 * See mate_dialog_run(). The only difference is that this function calls
 * mate_dialog_close() before returning, if the dialog was not already closed.
 *
 * Return value: If a button was pressed, the button number. Otherwise -1.
 **/
gint
mate_dialog_run_and_close(MateDialog* dialog)
{
  return mate_dialog_run_real(dialog,TRUE);
}

/**
 * mate_dialog_set_default: Set the default button for the dialog. The Enter key activates the default button.
 * @dialog: #MateDialog to affect.
 * @button: Number of the default button.
 *
 * The default button will be activated if the user just presses return.
 * Usually you should make the least-destructive button the default.
 * Otherwise, the most commonly-used button.
 *
 **/
void
mate_dialog_set_default (MateDialog *dialog,
			  gint button)
{
  GList *list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_widget_grab_default (GTK_WIDGET (list->data));
    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * mate_dialog_grab_focus: Makes a button grab the focus. T
 * @dialog: #MateDialog to affect.
 * @button: Number of the default button.
 *
 * The button @button will grab the focus.  Use this for dialogs
 * Where only buttons are displayed and you want to change the
 * default button.
 **/
void
mate_dialog_grab_focus (MateDialog *dialog, gint button)
{
  GList *list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data){
    gtk_widget_grab_focus (GTK_WIDGET (list->data));
    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * mate_dialog_set_close: Whether to call mate_dialog_close() when a button is clicked.
 * @dialog: #MateDialog to affect.
 * @click_closes: TRUE if clicking any button should call mate_dialog_close().
 *
 * This is a convenience function so you don't have to connect callbacks
 * to each button just to close the dialog. By default, #MateDialog
 * has this parameter set the FALSE and it will not close on any click.
 * (This was a design error.) However, almost all the #MateDialog subclasses,
 * such as #MateMessageBox and #MatePropertyBox, have this parameter set to
 * TRUE by default.
 *
 **/
void       mate_dialog_set_close    (MateDialog * dialog,
				      gboolean click_closes)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  dialog->click_closes = click_closes;
}

/**
 * mate_dialog_close_hides: mate_dialog_close() can destroy or hide the dialog; toggle this behavior.
 * @dialog: #MateDialog to affect.
 * @just_hide: If TRUE, mate_dialog_close() calls gtk_widget_hide() instead of gtk_widget_destroy().
 *
 * Some dialogs are expensive to create, so you want to keep them around and just
 * gtk_widget_show() them when they are opened, and gtk_widget_hide() them when
 * they're closed. Other dialogs are expensive to keep around, so you want to
 * gtk_widget_destroy() them when they're closed. It's a judgment call you
 * will need to make for each dialog.
 *
 **/
void       mate_dialog_close_hides (MateDialog * dialog,
				     gboolean just_hide)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  dialog->just_hide = just_hide;
}

/**
 * mate_dialog_set_sensitive: Set the sensitivity of a button.
 * @dialog: #MateDialog to affect.
 * @button: Which button to affect.
 * @setting: TRUE means it's sensitive.
 *
 * Calls gtk_widget_set_sensitive() on the specified button number.
 *
 **/
void       mate_dialog_set_sensitive  (MateDialog *dialog,
					gint         button,
					gboolean     setting)
{
  GList *list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_widget_set_sensitive(GTK_WIDGET(list->data), setting);
    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * mate_dialog_button_connect: Connect a callback to one of the button's "clicked" signals.
 * @dialog: #MateDialog to affect.
 * @button: Button number.
 * @callback: A standard Gtk callback.
 * @data: Callback data.
 *
 * Simply g_signal_connect() to the "clicked" signal of the specified button.
 *
 **/
void       mate_dialog_button_connect (MateDialog *dialog,
					gint button,
					GCallback callback,
					gpointer data)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    g_signal_connect(list->data, "clicked",
		     callback, data);
    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * mate_dialog_button_connect_object: g_signal_connect_swapped() to a button.
 * @dialog: #MateDialog to affect.
 * @button: Button to connect to.
 * @callback: Callback.
 * @obj: As for g_signal_connect_swapped().
 *
 * g_signal_connect_swapped() to the "clicked" signal of the given button.
 *
 **/
void       mate_dialog_button_connect_object (MateDialog *dialog,
					       gint button,
					       GCallback callback,
					       GtkObject * obj)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    g_signal_connect_swapped (list->data, "clicked",
			      callback, obj);
    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}


/**
 * mate_dialog_set_accelerator:
 * @dialog: #MateDialog to affect.
 * @button: Button number.
 * @accelerator_key: Key for the accelerator.
 * @accelerator_mods: Modifier.
 *
 * Set an accelerator key for a button.
 **/
void       mate_dialog_set_accelerator(MateDialog * dialog,
					gint button,
					const guchar accelerator_key,
					guint8       accelerator_mods)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    /*FIXME*/
    gtk_widget_add_accelerator(GTK_WIDGET(list->data),
			       "clicked",
			       dialog->accelerators,
			       accelerator_key,
			       accelerator_mods,
			       GTK_ACCEL_VISIBLE);

    return;
  }
#ifdef MATE_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * mate_dialog_editable_enters: Make the "activate" signal of an editable click the default dialog button.
 * @dialog: #MateDialog to affect.
 * @editable: Editable to affect.
 *
 * Normally if there's an editable widget (such as #GtkEntry) in your
 * dialog, pressing Enter will activate the editable rather than the
 * default dialog button. However, in most cases, the user expects to
 * type something in and then press enter to close the dialog. This
 * function enables that behavior.
 *
 **/
void       mate_dialog_editable_enters   (MateDialog * dialog,
					   GtkEditable * editable)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(editable != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));
  g_return_if_fail(GTK_IS_EDITABLE(editable));

  g_signal_connect_swapped(editable, "activate",
			   G_CALLBACK(gtk_window_activate_default),
			   dialog);
}


static void
mate_dialog_button_clicked (GtkWidget   *button,
			     GtkWidget   *dialog)
{
  GList *list;
  int which = 0;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  list = MATE_DIALOG (dialog)->buttons;

  while (list){
    if (list->data == button) {
	      gboolean click_closes;

	      click_closes = MATE_DIALOG(dialog)->click_closes;

	      g_signal_emit (dialog, dialog_signals[CLICKED], 0,
			     which);

	      /* The dialog may have been destroyed by the clicked signal, which
		 is why we had to save the click_closes flag.  Users should be
		 careful not to set click_closes and then destroy the dialog
		 themselves too. */

	      if (click_closes) {
		      mate_dialog_close(MATE_DIALOG(dialog));
	      }

	      /* Dialog may now be destroyed... */
	      break;
    }
    list = list->next;
    ++which;
  }
}

static gint mate_dialog_key_pressed (GtkWidget * d, GdkEventKey * e)
{
  g_return_val_if_fail(MATE_IS_DIALOG(d), TRUE);

  if(e->keyval == GDK_Escape)
    {
      mate_dialog_close(MATE_DIALOG(d));

      return TRUE; /* Stop the event? is this TRUE or FALSE? */
    }

  /* Have to call parent's handler, or the widget wouldn't get any
     key press events. Note that this is NOT done if the dialog
     may have been destroyed. */
  if (GTK_WIDGET_CLASS(parent_class)->key_press_event)
    return (* (GTK_WIDGET_CLASS(parent_class)->key_press_event))(d, e);
  else return FALSE; /* Not handled. */
}

static gint mate_dialog_delete_event (GtkWidget * d, GdkEventAny * e)
{
  mate_dialog_close(MATE_DIALOG(d));
  return TRUE; /* We handled it. */
}

static void mate_dialog_destroy (GtkObject *object)
{
  MateDialog *dialog;

  /* remember, destroy can be run multiple times! */

  g_return_if_fail(object != NULL);
  g_return_if_fail(MATE_IS_DIALOG(object));

  dialog = MATE_DIALOG(object);

  if(dialog->buttons)
	  g_list_free(dialog->buttons);
  dialog->buttons = NULL;

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(object);
}

static void mate_dialog_finalize (GObject *object)
{
  g_return_if_fail(object != NULL);
  g_return_if_fail(MATE_IS_DIALOG(object));

  if (G_OBJECT_CLASS(parent_class)->finalize)
    (* (G_OBJECT_CLASS(parent_class)->finalize))(object);
}

static
void mate_dialog_close_real(MateDialog * dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  gtk_widget_hide(GTK_WIDGET(dialog));

  if ( ! dialog->just_hide ) {
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }
}

/**
 * mate_dialog_close: Close (hide or destroy) the dialog.
 * @dialog: #MateDialog to close.
 *
 * See also mate_dialog_close_hides(). This function emits the
 * "close" signal, which either hides or destroys the dialog (destroy
 * by default). If you connect to the "close" signal, and your
 * callback returns TRUE, the hide or destroy will be blocked. You can
 * do this to avoid closing the dialog if the user gives invalid
 * input, for example.
 *
 * Using mate_dialog_close() in place of gtk_widget_hide() or
 * gtk_widget_destroy() allows you to easily catch all sources of
 * dialog closure, including delete_event and button clicks, and
 * handle them in a central location.
 **/
void mate_dialog_close(MateDialog * dialog)
{
  gint close_handled = FALSE;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(MATE_IS_DIALOG(dialog));

  g_signal_emit (dialog, dialog_signals[CLOSE], 0,
		 &close_handled);

  if ( ! close_handled ) {
    mate_dialog_close_real(dialog);
  }
}

static void mate_dialog_show (GtkWidget * d)
{
  if (GTK_WIDGET_CLASS(parent_class)->show)
    (* (GTK_WIDGET_CLASS(parent_class)->show))(d);
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
