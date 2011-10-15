/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DRW_BREAK_WINDOW_H__
#define __DRW_BREAK_WINDOW_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRW_TYPE_BREAK_WINDOW         (drw_break_window_get_type ())
#define DRW_BREAK_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DRW_TYPE_BREAK_WINDOW, DrwBreakWindow))
#define DRW_BREAK_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), DRW_TYPE_BREAK_WINDOW, DrwBreakWindowClass))
#define DRW_IS_BREAK_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DRW_TYPE_BREAK_WINDOW))
#define DRW_IS_BREAK_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DRW_TYPE_BREAK_WINDOW))
#define DRW_BREAK_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DRW_TYPE_BREAK_WINDOW, DrwBreakWindowClass))

typedef struct _DrwBreakWindow         DrwBreakWindow;
typedef struct _DrwBreakWindowClass    DrwBreakWindowClass;
typedef struct _DrwBreakWindowPrivate  DrwBreakWindowPrivate;

struct _DrwBreakWindow {
        GtkWindow              parent;

        DrwBreakWindowPrivate *priv;
};

struct _DrwBreakWindowClass {
        GtkWindowClass parent_class;
};

GType       drw_break_window_get_type (void) G_GNUC_CONST;
GtkWidget * drw_break_window_new      (void);

#ifdef __cplusplus
}
#endif

#endif /* __DRW_BREAK_WINDOW_H__ */
