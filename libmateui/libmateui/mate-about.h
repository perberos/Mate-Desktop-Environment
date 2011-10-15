/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-about.h - An about box widget for mate.

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>

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

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#ifndef __MATE_ABOUT_H__
#define __MATE_ABOUT_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_ABOUT            (mate_about_get_type ())
#define MATE_ABOUT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MATE_TYPE_ABOUT, MateAbout))
#define MATE_ABOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ABOUT, MateAboutClass))
#define MATE_IS_ABOUT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MATE_TYPE_ABOUT))
#define MATE_IS_ABOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ABOUT))
#define MATE_ABOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ABOUT, MateAboutClass))

typedef struct _MateAbout        MateAbout;
typedef struct _MateAboutClass   MateAboutClass;
typedef struct _MateAboutPrivate MateAboutPrivate;

struct _MateAbout {
	GtkDialog parent_instance;

	/*< private >*/
	MateAboutPrivate *_priv;
};

struct _MateAboutClass {
	GtkDialogClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

GType mate_about_get_type (void) G_GNUC_CONST;

GtkWidget *mate_about_new (const gchar  *name,
			    const gchar  *version,
			    const gchar  *copyright,
			    const gchar  *comments,
			    const gchar **authors,
			    const gchar **documenters,
			    const gchar  *translator_credits,
			    GdkPixbuf    *logo_pixbuf);

/* Only for use by bindings to languages other than C; don't use
   in applications. */
void mate_about_construct (MateAbout *about,
			    const gchar  *name,
			    const gchar  *version,
			    const gchar  *copyright,
			    const gchar  *comments,
			    const gchar **authors,
			    const gchar **documenters,
			    const gchar  *translator_credits,
			    GdkPixbuf    *logo_pixbuf);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_ABOUT_H__ */

