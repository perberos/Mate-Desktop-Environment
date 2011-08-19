/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Based on BlingSpinner: Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */


#ifndef _GDU_SPINNER_H_
#define _GDU_SPINNER_H_

#include <gdu-gtk/gdu-gtk-types.h>

G_BEGIN_DECLS

#define GDU_TYPE_SPINNER         (gdu_spinner_get_type ())
#define GDU_SPINNER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SPINNER, GduSpinner))
#define GDU_SPINNER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_SPINNER,  GduSpinnerClass))
#define GDU_IS_SPINNER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SPINNER))
#define GDU_IS_SPINNER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SPINNER))
#define GDU_SPINNER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SPINNER, GduSpinnerClass))

typedef struct GduSpinnerClass    GduSpinnerClass;
typedef struct GduSpinnerPrivate  GduSpinnerPrivate;

struct GduSpinner
{
	GtkDrawingArea parent;
};

struct GduSpinnerClass
{
	GtkDrawingAreaClass parent_class;
	GduSpinnerPrivate *priv;
};

GType gdu_spinner_get_type (void) G_GNUC_CONST;

GtkWidget *gdu_spinner_new (void);

void gdu_spinner_start (GduSpinner *spinner);
void gdu_spinner_stop  (GduSpinner *spinner);

G_END_DECLS

#endif
