/* MATE GUI Library
 * Copyright (C) 1995-1998 Jay Painter
 * All rights reserved.
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

#ifndef __MATE_DIALOG_H__
#define __MATE_DIALOG_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <stdarg.h>

G_BEGIN_DECLS

#define MATE_TYPE_DIALOG            (mate_dialog_get_type ())
#define MATE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DIALOG, MateDialog))
#define MATE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DIALOG, MateDialogClass))
#define MATE_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DIALOG))
#define MATE_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DIALOG))
#define MATE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DIALOG, MateDialogClass))

typedef struct _MateDialog        MateDialog;
typedef struct _MateDialogClass   MateDialogClass;

/**
 * MateDialog:
 * @vbox: The middle portion of the dialog box.
 *
 * Client code can pack widgets (for example, text or images) into @vbox.
 */
struct _MateDialog
{
  GtkWindow window;
  /*< public >*/
  GtkWidget * vbox;
  /*< private >*/
  GList *buttons;
  GtkWidget      *action_area; /* A button box, not an hbox */

  GtkAccelGroup  *accelerators;

  unsigned int    click_closes : 1;
  unsigned int    just_hide : 1;
};

struct _MateDialogClass
{
  GtkWindowClass parent_class;

  void (* clicked)  (MateDialog *dialog, gint button_number);
  gboolean (* close) (MateDialog * dialog);
};

/* MateDialog creates an action area with the buttons of your choice.
   You should pass the button names (possibly MATE_STOCK_BUTTON_*) as 
   arguments to mate_dialog_new(). The buttons are numbered in the 
   order you passed them in, starting at 0. These numbers are used
   in other functions, and passed to the "clicked" callback. */

GType      mate_dialog_get_type       (void) G_GNUC_CONST;

/* Arguments: Title and button names, then NULL */
GtkWidget* mate_dialog_new            (const gchar * title,
					...);
/* Arguments: Title and NULL terminated array of button names. */
GtkWidget* mate_dialog_newv           (const gchar * title,
					const gchar **buttons);

/* For now this just means the dialog can be centered over
   its parent. */
void       mate_dialog_set_parent     (MateDialog * dialog,
					GtkWindow   * parent);

/* Note: it's better to use MateDialog::clicked rather than 
   connecting to a button. These are really here in case
   you're lazy. */
/* Connect to the "clicked" signal of a single button */
void       mate_dialog_button_connect (MateDialog *dialog,
					gint button,
					GCallback callback,
					gpointer data);
/* Connect the object to the "clicked" signal of a single button */
void       mate_dialog_button_connect_object (MateDialog *dialog,
					       gint button,
					       GCallback callback,
					       GtkObject * obj);

/* Run the dialog, return the button # that was pressed or -1 if none.
   (this sets the dialog modal while it blocks)
 */
gint       mate_dialog_run	       (MateDialog *dialog);
gint       mate_dialog_run_and_close  (MateDialog *dialog);


/* Set the default button. - it will have a little highlight, 
   and pressing return will activate it. */
void       mate_dialog_set_default    (MateDialog *dialog,
					gint         button);

/* Makes the nth button the focused widget in the dialog */
void       mate_dialog_grab_focus     (MateDialog *dialog,
					gint button);
/* Set sensitivity of a button */
void       mate_dialog_set_sensitive  (MateDialog *dialog,
					gint         button,
					gboolean     setting);

/* Set the accelerator for a button. Note that there are two
   default accelerators: "Return" will be the same as clicking
   the default button, and "Escape" will emit delete_event. 
   (Note: neither of these is in the accelerator table,
   Return is a Gtk default and Escape comes from a key press event
   handler.) */
void       mate_dialog_set_accelerator(MateDialog * dialog,
					gint button,
					const guchar accelerator_key,
					guint8       accelerator_mods);

/* Hide and optionally destroy. Destroys by default, use close_hides() 
   to change this. */
void       mate_dialog_close (MateDialog * dialog);

/* Make _close just hide, not destroy. */
void       mate_dialog_close_hides (MateDialog * dialog, 
				     gboolean just_hide);

/* Whether to close after emitting clicked signal - default is
   FALSE. If clicking *any* button should close the dialog, set it to
   TRUE.  */
void       mate_dialog_set_close      (MateDialog * dialog,
					gboolean click_closes);

/* Normally an editable widget will grab "Return" and keep it from 
   activating the dialog's default button. This connects the activate
   signal of the editable to the default button. */
void       mate_dialog_editable_enters   (MateDialog * dialog,
					   GtkEditable * editable);

/* Use of append_buttons is discouraged, it's really
   meant for subclasses. */
void       mate_dialog_append_buttons  (MateDialog * dialog,
					 const gchar * first,
					 ...);
void       mate_dialog_append_button   (MateDialog * dialog,
				         const gchar * button_name);
void       mate_dialog_append_buttonsv (MateDialog * dialog,
					 const gchar **buttons);

/* Add button with arbitrary text and pixmap. */
void       mate_dialog_append_button_with_pixmap (MateDialog * dialog,
						   const gchar * button_name,
						   const gchar * pixmap_name);
void       mate_dialog_append_buttons_with_pixmaps (MateDialog * dialog,
						     const gchar **names,
						     const gchar **pixmaps);

/* Don't use this either; it's for bindings to languages other 
   than C (which makes the varargs kind of lame... feel free to fix)
   You want _new, see above. */
void       mate_dialog_construct  (MateDialog * dialog,
				    const gchar * title,
				    va_list ap);
void       mate_dialog_constructv (MateDialog * dialog,
				    const gchar * title,
				    const gchar **buttons);

/* Stock defines for compatibility, not to be used
 * in new applications, please see gtk stock icons
 * and you should use those instead. */
#define MATE_STOCK_BUTTON_OK     GTK_STOCK_OK
#define MATE_STOCK_BUTTON_CANCEL GTK_STOCK_CANCEL
#define MATE_STOCK_BUTTON_YES    GTK_STOCK_YES
#define MATE_STOCK_BUTTON_NO     GTK_STOCK_NO
#define MATE_STOCK_BUTTON_CLOSE  GTK_STOCK_CLOSE
#define MATE_STOCK_BUTTON_APPLY  GTK_STOCK_APPLY
#define MATE_STOCK_BUTTON_HELP   GTK_STOCK_HELP
#define MATE_STOCK_BUTTON_NEXT   GTK_STOCK_GO_FORWARD
#define MATE_STOCK_BUTTON_PREV   GTK_STOCK_GO_BACK
#define MATE_STOCK_BUTTON_UP     GTK_STOCK_GO_UP
#define MATE_STOCK_BUTTON_DOWN   GTK_STOCK_GO_DOWN
#define MATE_STOCK_BUTTON_FONT   GTK_STOCK_SELECT_FONT

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_DIALOG_H__ */

