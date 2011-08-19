/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-volumes.h
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

#ifndef GDU_SECTION_VOLUMES_H
#define GDU_SECTION_VOLUMES_H

#define GDU_TYPE_SECTION_VOLUMES           (gdu_section_volumes_get_type ())
#define GDU_SECTION_VOLUMES(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SECTION_VOLUMES, GduSectionVolumes))
#define GDU_SECTION_VOLUMES_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SECTION_VOLUMES,  GduSectionVolumesClass))
#define GDU_IS_SECTION_VOLUMES(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SECTION_VOLUMES))
#define GDU_IS_SECTION_VOLUMES_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SECTION_VOLUMES))
#define GDU_SECTION_VOLUMES_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SECTION_VOLUMES, GduSectionVolumesClass))

typedef struct _GduSectionVolumesClass       GduSectionVolumesClass;
typedef struct _GduSectionVolumes            GduSectionVolumes;

struct _GduSectionVolumesPrivate;
typedef struct _GduSectionVolumesPrivate     GduSectionVolumesPrivate;

struct _GduSectionVolumes
{
        GduSection parent;

        /* private */
        GduSectionVolumesPrivate *priv;
};

struct _GduSectionVolumesClass
{
        GduSectionClass parent_class;
};

GType            gdu_section_volumes_get_type      (void);
GtkWidget       *gdu_section_volumes_new           (GduShell       *shell,
                                                    GduPresentable *presentable);
gboolean         gdu_section_volumes_select_volume (GduSectionVolumes *section,
                                                    GduPresentable    *volume);

#endif /* GDU_SECTION_VOLUMES_H */
