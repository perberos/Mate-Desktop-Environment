/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * mate-window.h: wrappers for setting window properties
 *
 * Author:  Chema Celorio <chema@celorio.com>
 */

/*
 * These functions are a convenience wrapper for gtk_window_set_title
 * This allows all the mate-apps to have a consitent way of setting
 * the window & dialogs titles. We could also add a configurable way
 * of setting the windows titles in the future..
 */

#ifndef MATE_WINDOW_H
#define MATE_WINDOW_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/* set the window title */
void mate_window_toplevel_set_title (GtkWindow *window,
				      const gchar *doc_name,
				      const gchar *app_name,
				      const gchar *extension);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_WINDOW_H */

