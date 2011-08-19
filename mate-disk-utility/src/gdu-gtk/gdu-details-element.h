/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
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
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __GDU_DETAILS_ELEMENT_H
#define __GDU_DETAILS_ELEMENT_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_DETAILS_ELEMENT         (gdu_details_element_get_type())
#define GDU_DETAILS_ELEMENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_DETAILS_ELEMENT, GduDetailsElement))
#define GDU_DETAILS_ELEMENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_DETAILS_ELEMENT, GduDetailsElementClass))
#define GDU_IS_DETAILS_ELEMENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_DETAILS_ELEMENT))
#define GDU_IS_DETAILS_ELEMENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_DETAILS_ELEMENT))
#define GDU_DETAILS_ELEMENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_DETAILS_ELEMENT, GduDetailsElementClass))

typedef struct GduDetailsElementClass   GduDetailsElementClass;
typedef struct GduDetailsElementPrivate GduDetailsElementPrivate;

struct GduDetailsElement
{
        GObject parent;

        /*< private >*/
        GduDetailsElementPrivate *priv;
};

struct GduDetailsElementClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed) (GduDetailsElement *element);

        /* signals */
        void (*activated) (GduDetailsElement *element,
                           const gchar       *uri);
};

GType               gdu_details_element_get_type            (void) G_GNUC_CONST;
GduDetailsElement*  gdu_details_element_new                 (const gchar       *heading,
                                                             const gchar       *text,
                                                             const gchar       *tooltip);

const gchar        *gdu_details_element_get_heading        (GduDetailsElement *element);
GIcon              *gdu_details_element_get_icon           (GduDetailsElement *element);
const gchar        *gdu_details_element_get_text           (GduDetailsElement *element);
guint64             gdu_details_element_get_time           (GduDetailsElement *element);
gdouble             gdu_details_element_get_progress       (GduDetailsElement *element);
const gchar        *gdu_details_element_get_tooltip        (GduDetailsElement *element);
const gchar        *gdu_details_element_get_action_text    (GduDetailsElement *element);
const gchar        *gdu_details_element_get_action_uri     (GduDetailsElement *element);
const gchar        *gdu_details_element_get_action_tooltip (GduDetailsElement *element);
gboolean            gdu_details_element_get_is_spinning    (GduDetailsElement *element);
GtkWidget          *gdu_details_element_get_widget         (GduDetailsElement *element);

void                gdu_details_element_set_heading        (GduDetailsElement *element,
                                                            const gchar       *heading);
void                gdu_details_element_set_icon           (GduDetailsElement *element,
                                                            GIcon             *icon);
void                gdu_details_element_set_text           (GduDetailsElement *element,
                                                            const gchar       *text);
void                gdu_details_element_set_time           (GduDetailsElement *element,
                                                            guint64            time);
void                gdu_details_element_set_progress       (GduDetailsElement *element,
                                                            gdouble            progress);
void                gdu_details_element_set_tooltip        (GduDetailsElement *element,
                                                            const gchar       *tooltip);
void                gdu_details_element_set_action_text    (GduDetailsElement *element,
                                                            const gchar       *action_text);
void                gdu_details_element_set_action_uri     (GduDetailsElement *element,
                                                            const gchar       *action_uri);
void                gdu_details_element_set_action_tooltip (GduDetailsElement *element,
                                                            const gchar       *action_tooltip);
void                gdu_details_element_set_is_spinning    (GduDetailsElement *element,
                                                            gboolean           is_spinning);
void                gdu_details_element_set_widget         (GduDetailsElement *element,
                                                            GtkWidget         *widget);

G_END_DECLS

#endif  /* __GDU_DETAILS_ELEMENT_H */

