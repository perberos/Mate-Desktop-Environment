/* mate-appbar.h: statusbar/progress/minibuffer widget for Mate apps
 *
 * This version by Havoc Pennington
 * Based on MozStatusbar widget, by Chris Toshok
 * In turn based on GtkStatusbar widget, from Gtk+,
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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

#ifndef __MATE_APPBAR_H__
#define __MATE_APPBAR_H__

#include <gtk/gtk.h>

#include "mate-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_APPBAR            (mate_appbar_get_type ())
#define MATE_APPBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_APPBAR, MateAppBar))
#define MATE_APPBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_APPBAR, MateAppBarClass))
#define MATE_IS_APPBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_APPBAR))
#define MATE_IS_APPBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_APPBAR))
#define MATE_APPBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_APPBAR, MateAppBarClass))

/* Used in mate-app-util to determine the capabilities of the appbar */
#define MATE_APPBAR_HAS_STATUS(appbar) (mate_appbar_get_status(MATE_APPBAR(appbar)) != NULL)
#define MATE_APPBAR_HAS_PROGRESS(appbar) (mate_appbar_get_progress(MATE_APPBAR(appbar)) != NULL)

typedef struct _MateAppBar        MateAppBar;
typedef struct _MateAppBarPrivate MateAppBarPrivate;
typedef struct _MateAppBarClass   MateAppBarClass;
typedef struct _MateAppBarMsg     MateAppBarMsg;

struct _MateAppBar
{
  GtkHBox parent_widget;

  /*< private >*/
  MateAppBarPrivate *_priv;
};

struct _MateAppBarClass
{
  GtkHBoxClass parent_class;

  /* Emitted when the user hits enter after a prompt. */
  void (* user_response) (MateAppBar * ab);
  /* Emitted when the prompt is cleared. */
  void (* clear_prompt)  (MateAppBar * ab);

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

#define MATE_APPBAR_INTERACTIVE(ab) ((ab) ? (ab)->interactive : FALSE)

GType      mate_appbar_get_type     	(void) G_GNUC_CONST;

GtkWidget* mate_appbar_new          	(gboolean has_progress,
					 gboolean has_status,
					 MatePreferencesType interactivity);

/* Sets the status label without changing widget state; next set or push
   will destroy this permanently. */
void       mate_appbar_set_status       (MateAppBar * appbar,
					  const gchar * status);

/* get the statusbar */
GtkWidget* mate_appbar_get_status       (MateAppBar * appbar);

/* What to show when showing nothing else; defaults to nothing */
void	   mate_appbar_set_default      (MateAppBar * appbar,
					  const gchar * default_status);

void       mate_appbar_push             (MateAppBar * appbar,
					  const gchar * status);
/* OK to call on empty stack */
void       mate_appbar_pop              (MateAppBar * appbar);

/* Nuke the stack. */
void       mate_appbar_clear_stack      (MateAppBar * appbar);

/* Sugar function to set the percentage of the progressbar. */
void	     mate_appbar_set_progress_percentage	  (MateAppBar *appbar,
							   gfloat percentage);
/* use GtkProgress functions on returned value */
GtkProgressBar* mate_appbar_get_progress    (MateAppBar * appbar);

/* Reflect the current state of stack/default. Useful to force a set_status
   to disappear. */
void       mate_appbar_refresh         (MateAppBar * appbar);

/* Put a prompt in the appbar and wait for a response. When the
   user responds or cancels, a user_response signal is emitted. */
void       mate_appbar_set_prompt          (MateAppBar * appbar,
					     const gchar * prompt,
					     gboolean modal);
/* Remove any prompt */
void       mate_appbar_clear_prompt    (MateAppBar * appbar);

/* Get the response to the prompt, if any. Result must be g_free'd. */
gchar *    mate_appbar_get_response    (MateAppBar * appbar);


#ifdef __cplusplus
}
#endif

#endif /* __MATE_APPBAR_H__ */







