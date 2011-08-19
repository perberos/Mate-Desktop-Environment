/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-linux-md-drive.h
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

#ifndef GDU_SECTION_LINUX_MD_DRIVE_H
#define GDU_SECTION_LINUX_MD_DRIVE_H

#define GDU_TYPE_SECTION_LINUX_MD_DRIVE             (gdu_section_linux_md_drive_get_type ())
#define GDU_SECTION_LINUX_MD_DRIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_SECTION_LINUX_MD_DRIVE, GduSectionLinuxMdDrive))
#define GDU_SECTION_LINUX_MD_DRIVE_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GDU_SECTION_LINUX_MD_DRIVE,  GduSectionLinuxMdDriveClass))
#define GDU_IS_SECTION_LINUX_MD_DRIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_SECTION_LINUX_MD_DRIVE))
#define GDU_IS_SECTION_LINUX_MD_DRIVE_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GDU_TYPE_SECTION_LINUX_MD_DRIVE))
#define GDU_SECTION_LINUX_MD_DRIVE_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_SECTION_LINUX_MD_DRIVE, GduSectionLinuxMdDriveClass))

typedef struct _GduSectionLinuxMdDriveClass       GduSectionLinuxMdDriveClass;
typedef struct _GduSectionLinuxMdDrive            GduSectionLinuxMdDrive;

struct _GduSectionLinuxMdDrivePrivate;
typedef struct _GduSectionLinuxMdDrivePrivate     GduSectionLinuxMdDrivePrivate;

struct _GduSectionLinuxMdDrive
{
        GduSection parent;

        /* private */
        GduSectionLinuxMdDrivePrivate *priv;
};

struct _GduSectionLinuxMdDriveClass
{
        GduSectionClass parent_class;
};

GType            gdu_section_linux_md_drive_get_type (void);
GtkWidget       *gdu_section_linux_md_drive_new      (GduShell       *shell,
                                                         GduPresentable *presentable);

#endif /* GDU_SECTION_LINUX_MD_DRIVE_H */
