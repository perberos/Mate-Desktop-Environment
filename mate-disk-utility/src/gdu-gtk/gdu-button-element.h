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

#ifndef __GDU_BUTTON_ELEMENT_H
#define __GDU_BUTTON_ELEMENT_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_BUTTON_ELEMENT         (gdu_button_element_get_type())
#define GDU_BUTTON_ELEMENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_BUTTON_ELEMENT, GduButtonElement))
#define GDU_BUTTON_ELEMENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_BUTTON_ELEMENT, GduButtonElementClass))
#define GDU_IS_BUTTON_ELEMENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_BUTTON_ELEMENT))
#define GDU_IS_BUTTON_ELEMENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_BUTTON_ELEMENT))
#define GDU_BUTTON_ELEMENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_BUTTON_ELEMENT, GduButtonElementClass))

typedef struct GduButtonElementClass   GduButtonElementClass;
typedef struct GduButtonElementPrivate GduButtonElementPrivate;

struct GduButtonElement
{
        GObject parent;

        /*< private >*/
        GduButtonElementPrivate *priv;
};

struct GduButtonElementClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed) (GduButtonElement *element);

        /* signals */
        void (*clicked) (GduButtonElement *element);
};

GType              gdu_button_element_get_type             (void) G_GNUC_CONST;
GduButtonElement*  gdu_button_element_new                  (const gchar       *icon_name,
                                                            const gchar       *primary_text,
                                                            const gchar       *secondary_text);
const gchar       *gdu_button_element_get_icon_name        (GduButtonElement *element);
const gchar       *gdu_button_element_get_primary_text     (GduButtonElement *element);
const gchar       *gdu_button_element_get_secondary_text   (GduButtonElement *element);
gboolean           gdu_button_element_get_visible          (GduButtonElement *element);
void               gdu_button_element_set_icon_name        (GduButtonElement *element,
                                                            const gchar      *icon_name);
void               gdu_button_element_set_primary_text     (GduButtonElement  *element,
                                                            const gchar       *primary_text);
void               gdu_button_element_set_secondary_text   (GduButtonElement  *element,
                                                            const gchar       *primary_text);
void               gdu_button_element_set_visible          (GduButtonElement  *element,
                                                            gboolean           visible);

G_END_DECLS

#endif  /* __GDU_BUTTON_ELEMENT_H */

