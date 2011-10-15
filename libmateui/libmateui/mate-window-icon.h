/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * mate-window-icon.h: convenience functions for window mini-icons
 *
 * Copyright 2000, 2001 Ximian, Inc.
 *
 * Author:  Jacob Berkman  <jacob@ximian.com>
 */

/*
 * These functions are a convenience wrapper for the
 * gtk_window_set_icon_list() function.  They allow setting a default
 * icon, which is used by many top level windows in libmateui, such
 * as MateApp and MateDialog windows.
 *
 * They also handle drawing the icon on the iconified window's icon in
 * window managers such as TWM and Window Maker.
 *
 */

#ifndef MATE_WINDOW_ICON_H
#define MATE_WINDOW_ICON_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/* set an icon on a window */
void mate_window_icon_set_from_default   (GtkWindow *w);
void mate_window_icon_set_from_file      (GtkWindow *w, const char  *filename);
void mate_window_icon_set_from_file_list (GtkWindow *w, const char **filenames);

/* set the default icon used */
void mate_window_icon_set_default_from_file      (const char  *filename);
void mate_window_icon_set_default_from_file_list (const char **filenames);

/* check for the MATE_DESKTOP_ICON environment variable */
void mate_window_icon_init (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_WINDOW_ICON_H */

