/* mate-href.h
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
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

#ifndef MATE_HREF_H
#define MATE_HREF_H

#ifndef MATE_DISABLE_DEPRECATED

#include <glib.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define MATE_TYPE_HREF            (mate_href_get_type ())
#define MATE_HREF(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MATE_TYPE_HREF, MateHRef))
#define MATE_HREF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MATE_TYPE_HREF, MateHRefClass))
#define MATE_IS_HREF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MATE_TYPE_HREF))
#define MATE_IS_HREF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_HREF))
#define MATE_HREF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_HREF, MateHRefClass))

typedef struct _MateHRef        MateHRef;
typedef struct _MateHRefPrivate MateHRefPrivate;
typedef struct _MateHRefClass   MateHRefClass;


struct _MateHRef {
  GtkButton button;

  /*< private >*/
  MateHRefPrivate *_priv;
};

struct _MateHRefClass {
  GtkButtonClass parent_class;

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

/*
 * MATE href class methods
 */

GType mate_href_get_type(void) G_GNUC_CONST;
GtkWidget *mate_href_new(const gchar *url, const gchar *text);

/* for bindings and subclassing, use the mate_href_new from C */

void mate_href_set_url(MateHRef *href, const gchar *url);
const gchar *mate_href_get_url(MateHRef *href);

void mate_href_set_text(MateHRef *href, const gchar *text);
const gchar *mate_href_get_text(MateHRef *href);

void mate_href_set_label(MateHRef *href, const gchar *label);
const gchar *mate_href_get_label(MateHRef *href);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif

