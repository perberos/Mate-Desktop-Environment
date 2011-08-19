/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-shell.h
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef GDU_SHELL_H
#define GDU_SHELL_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include <gdu/gdu.h>

#define GDU_TYPE_SHELL             (gdu_shell_get_type ())
#define GDU_SHELL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_SHELL, GduShell))
#define GDU_SHELL_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GDU_SHELL,  GduShellClass))
#define GDU_IS_SHELL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_SHELL))
#define GDU_IS_SHELL_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GDU_TYPE_SHELL))
#define GDU_SHELL_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_SHELL, GduShellClass))

typedef struct _GduShellClass       GduShellClass;
typedef struct _GduShell            GduShell;

struct _GduShellPrivate;
typedef struct _GduShellPrivate     GduShellPrivate;

struct _GduShell
{
        GObject parent;

        /* private */
        GduShellPrivate *priv;
};

struct _GduShellClass
{
        GObjectClass parent_class;
};

GType           gdu_shell_get_type                          (void);
GduShell       *gdu_shell_new                               (const char      *ssh_address);
GtkWidget      *gdu_shell_get_toplevel                      (GduShell       *shell);
GduPool        *gdu_shell_get_pool_for_selected_presentable (GduShell       *shell);
void            gdu_shell_update                            (GduShell       *shell);
GduPresentable *gdu_shell_get_selected_presentable          (GduShell       *shell);
void            gdu_shell_select_presentable                (GduShell       *shell,
                                                             GduPresentable *presentable);
void            gdu_shell_raise_error                       (GduShell       *shell,
                                                             GduPresentable *presentable,
                                                             GError         *error,
                                                             const char     *primary_markup_format,
                                                             ...);

#endif /* GDU_SHELL_H */
