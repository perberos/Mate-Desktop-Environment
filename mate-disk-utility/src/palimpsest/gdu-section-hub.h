/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-hub.h
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
#include "gdu-section.h"

#ifndef GDU_SECTION_HUB_H
#define GDU_SECTION_HUB_H

#define GDU_TYPE_SECTION_HUB           (gdu_section_hub_get_type ())
#define GDU_SECTION_HUB(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SECTION_HUB, GduSectionHub))
#define GDU_SECTION_HUB_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SECTION_HUB,  GduSectionHubClass))
#define GDU_IS_SECTION_HUB(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SECTION_HUB))
#define GDU_IS_SECTION_HUB_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SECTION_HUB))
#define GDU_SECTION_HUB_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SECTION_HUB, GduSectionHubClass))

typedef struct _GduSectionHubClass       GduSectionHubClass;
typedef struct _GduSectionHub            GduSectionHub;

struct _GduSectionHubPrivate;
typedef struct _GduSectionHubPrivate     GduSectionHubPrivate;

struct _GduSectionHub
{
        GduSection parent;

        /* private */
        GduSectionHubPrivate *priv;
};

struct _GduSectionHubClass
{
        GduSectionClass parent_class;
};

GType            gdu_section_hub_get_type (void);
GtkWidget       *gdu_section_hub_new      (GduShell       *shell,
                                           GduPresentable *presentable);

#endif /* GDU_SECTION_HUB_H */
