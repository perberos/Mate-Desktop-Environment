/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*-

   Copyright (C) 1999, 2000 Red Hat, Inc.
   All rights reserved.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   MatePixmap Developers: Havoc Pennington, Jonathan Blandford

   Though this is hardly MatePixmap anymore :)
   If you use this API in new applications, you will be strangled to death,
   please use GtkImage, it's much nicer and cooler and this just uses it anyway

     -George
*/
/*
  @NOTATION@
*/

#ifndef MATE_PIXMAP_H
#define MATE_PIXMAP_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_PIXMAP            (mate_pixmap_get_type ())
#define MATE_PIXMAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_PIXMAP, MatePixmap))
#define MATE_PIXMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_PIXMAP, MatePixmapClass))
#define MATE_IS_PIXMAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_PIXMAP))
#define MATE_IS_PIXMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_PIXMAP))
#define MATE_PIXMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_PIXMAP, MatePixmapClass))

/* Note:
 * You should use GtkImage if you can, this is just a compatibility wrapper to get
 * old code to compile */

typedef struct _MatePixmap        MatePixmap;
typedef struct _MatePixmapClass   MatePixmapClass;
typedef struct _MatePixmapPrivate MatePixmapPrivate;

struct _MatePixmap {
	GtkImage parent;

	MatePixmapPrivate *_priv;
};


struct _MatePixmapClass {
	GtkImageClass parent_class;
};

GType           mate_pixmap_get_type                (void) G_GNUC_CONST;

GtkWidget      *mate_pixmap_new_from_file           (const gchar      *filename);
GtkWidget      *mate_pixmap_new_from_file_at_size   (const gchar      *filename,
						      gint              width,
						      gint              height);
GtkWidget      *mate_pixmap_new_from_xpm_d          (const gchar     **xpm_data);
GtkWidget      *mate_pixmap_new_from_xpm_d_at_size  (const gchar     **xpm_data,
						      gint              width,
						      gint              height);
GtkWidget      *mate_pixmap_new_from_mate_pixmap   (MatePixmap      *gpixmap);

void            mate_pixmap_load_file               (MatePixmap      *gpixmap,
						      const char       *filename);
void            mate_pixmap_load_file_at_size       (MatePixmap      *gpixmap,
						      const char       *filename,
						      int               width,
						      int               height);
void            mate_pixmap_load_xpm_d              (MatePixmap      *gpixmap,
						      const char      **xpm_data);
void            mate_pixmap_load_xpm_d_at_size      (MatePixmap      *gpixmap,
						      const char      **xpm_data,
						      int               width,
						      int               height);


#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_PIXMAP_H__ */
