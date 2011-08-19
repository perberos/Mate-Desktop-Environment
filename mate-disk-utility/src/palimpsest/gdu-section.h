/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section.h
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

#include <gtk/gtk.h>
#include <gdu/gdu.h>
#include "gdu-shell.h"

#ifndef GDU_SECTION_H
#define GDU_SECTION_H

#define GDU_TYPE_SECTION           (gdu_section_get_type ())
#define GDU_SECTION(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SECTION, GduSection))
#define GDU_SECTION_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SECTION,  GduSectionClass))
#define GDU_IS_SECTION(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SECTION))
#define GDU_IS_SECTION_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SECTION))
#define GDU_SECTION_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SECTION, GduSectionClass))

typedef struct _GduSectionClass       GduSectionClass;
typedef struct _GduSection            GduSection;

struct _GduSectionPrivate;
typedef struct _GduSectionPrivate     GduSectionPrivate;

struct _GduSection
{
        GtkVBox parent;

        /* private */
        GduSectionPrivate *priv;
};

struct _GduSectionClass
{
        GtkVBoxClass parent_class;

        /* virtual table */
        void (*update) (GduSection *section);
};

GType            gdu_section_get_type         (void);
void             gdu_section_update           (GduSection     *section);

GduShell        *gdu_section_get_shell        (GduSection     *section);
GduPresentable  *gdu_section_get_presentable  (GduSection     *section);


#endif /* GDU_SECTION_H */
