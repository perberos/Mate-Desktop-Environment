/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-drive.h
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

#ifndef GDU_SECTION_DRIVE_H
#define GDU_SECTION_DRIVE_H

#define GDU_TYPE_SECTION_DRIVE           (gdu_section_drive_get_type ())
#define GDU_SECTION_DRIVE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SECTION_DRIVE, GduSectionDrive))
#define GDU_SECTION_DRIVE_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SECTION_DRIVE,  GduSectionDriveClass))
#define GDU_IS_SECTION_DRIVE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SECTION_DRIVE))
#define GDU_IS_SECTION_DRIVE_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SECTION_DRIVE))
#define GDU_SECTION_DRIVE_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SECTION_DRIVE, GduSectionDriveClass))

typedef struct _GduSectionDriveClass       GduSectionDriveClass;
typedef struct _GduSectionDrive            GduSectionDrive;

struct _GduSectionDrivePrivate;
typedef struct _GduSectionDrivePrivate     GduSectionDrivePrivate;

struct _GduSectionDrive
{
        GduSection parent;

        /* private */
        GduSectionDrivePrivate *priv;
};

struct _GduSectionDriveClass
{
        GduSectionClass parent_class;
};

GType            gdu_section_drive_get_type (void);
GtkWidget       *gdu_section_drive_new      (GduShell       *shell,
                                             GduPresentable *presentable);

/* these functions are exported for use in GduSectionLinuxMd and other sections - user_data must
 * be a GduSection instance
 */
void
gdu_section_drive_on_format_button_clicked (GduButtonElement *button_element,
                                            gpointer          user_data);
void
gdu_section_drive_on_benchmark_button_clicked (GduButtonElement *button_element,
                                               gpointer          user_data);


#endif /* GDU_SECTION_DRIVE_H */
