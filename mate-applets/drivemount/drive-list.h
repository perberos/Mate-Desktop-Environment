/* -*- mode: C; c-basic-offset: 4 -*-
 * Drive Mount Applet
 * Copyright (c) 2004 Canonical Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 *   James Henstridge <jamesh@canonical.com>
 */

#ifndef DRIVE_LIST_H
#define DRIVE_LIST_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DRIVE_TYPE_LIST         (drive_list_get_type ())
#define DRIVE_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DRIVE_TYPE_LIST, DriveList))
#define DRIVE_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DRIVE_TYPE_LIST, DriveListClass))
#define DRIVE_IS_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DRIVE_TYPE_LIST))
#define DRIVE_IS_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DRIVE_TYPE_LIST))
#define DRIVE_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DRIVE_TYPE_LIST, DriveListClass))

typedef struct _DriveList      DriveList;
typedef struct _DriveListClass DriveListClass;

struct _DriveList
{
    GtkTable parent;

    GHashTable *volumes;
    GHashTable *mounts;
    GtkOrientation orientation;
    guint layout_tag;
    GtkReliefStyle relief;

    int icon_size;
};

struct _DriveListClass
{
    GtkTableClass parent_class;
};

GType      drive_list_get_type (void);
GtkWidget *drive_list_new (void);
void       drive_list_set_orientation (DriveList *list,
				       GtkOrientation orientation);
void       drive_list_set_panel_size  (DriveList *list,
				       int panel_size);
void       drive_list_set_transparent (DriveList *self,
				       gboolean transparent);

#endif /* DRIVE_LIST_H */
