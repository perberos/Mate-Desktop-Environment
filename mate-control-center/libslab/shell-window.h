/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __SHELL_WINDOW_H__
#define __SHELL_WINDOW_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libmate/mate-desktop-item.h>

#include <libslab/app-shell.h>

G_BEGIN_DECLS

#define SHELL_WINDOW_TYPE            (shell_window_get_type ())
#define SHELL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHELL_WINDOW_TYPE, ShellWindow))
#define SHELL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SHELL_WINDOW_TYPE, ShellWindowClass))
#define IS_SHELL_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHELL_WINDOW_TYPE))
#define IS_SHELL_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SHELL_WINDOW_TYPE))
#define SHELL_WINDOW_GET_CLASS(obj)  (G_TYPE_CHECK_GET_CLASS ((obj), SHELL_WINDOW_TYPE, ShellWindowClass))

typedef struct _ShellWindow ShellWindow;
typedef struct _ShellWindowClass ShellWindowClass;

struct _ShellWindow
{
	GtkFrame frame;

	GtkBox *_hbox;
	GtkWidget *_left_pane;
	GtkWidget *_right_pane;

	gulong resize_handler_id;
};

struct _ShellWindowClass
{
	GtkFrameClass parent_class;
};

GType shell_window_get_type (void);
GtkWidget *shell_window_new (AppShellData * app_data);
void shell_window_set_contents (ShellWindow * window, GtkWidget * left_pane,
	GtkWidget * right_pane);
void shell_window_clear_resize_handler (ShellWindow * win);

G_END_DECLS
#endif /* __SHELL_WINDOW_H__ */
