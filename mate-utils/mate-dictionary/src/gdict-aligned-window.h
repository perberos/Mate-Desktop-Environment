/* gdict-aligned-widget.h - Popup window aligned to a widget
 *
 * Copyright (c) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License  
 * along with this program; if not, write to the Free Software  
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Ported from Seth Nickell's Python class:
 *   Copyright (c) 2003 Seth Nickell
 */

#ifndef __GDICT_ALIGNED_WINDOW_H__
#define __GDICT_ALIGNED_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDICT_TYPE_ALIGNED_WINDOW		(gdict_aligned_window_get_type ())
#define GDICT_ALIGNED_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_ALIGNED_WINDOW, GdictAlignedWindow))
#define GDICT_IS_ALIGNED_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_ALIGNED_WINDOW))
#define GDICT_ALIGNED_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_ALIGNED_WINDOW, GdictAlignedWindowClass))
#define GDICT_IS_ALIGNED_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_ALIGNED_WINDOW))
#define GDICT_ALIGNED_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_ALIGNED_WINDOW, GdictAlignedWindowClass))

typedef struct _GdictAlignedWindow        GdictAlignedWindow;
typedef struct _GdictAlignedWindowClass	  GdictAlignedWindowClass;
typedef struct _GdictAlignedWindowPrivate GdictAlignedWindowPrivate;

struct _GdictAlignedWindow
{
  /*< private >*/
  GtkWindow parent_instance;
  
  GdictAlignedWindowPrivate *priv;
};

struct _GdictAlignedWindowClass
{
  /*< private >*/
  GtkWindowClass parent_class;
  
  void (*_gdict_reserved1) (void);
  void (*_gdict_reserved2) (void);
  void (*_gdict_reserved3) (void);
  void (*_gdict_reserved4) (void);
};

GType      gdict_aligned_window_get_type   (void) G_GNUC_CONST;

GtkWidget *gdict_aligned_window_new        (GtkWidget          *align_widget);
void       gdict_aligned_window_set_widget (GdictAlignedWindow *aligned_window,
					    GtkWidget          *align_widget);
GtkWidget *gdict_aligned_window_get_widget (GdictAlignedWindow *aligned_window);

G_END_DECLS

#endif
