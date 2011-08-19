/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * gdm-display-access-file.h - Abstraction around xauth cookies
 *
 * Copyright (C) 2007 Ray Strode <rstrode@redhat.com>
 *
 * Written by Ray Strode <rstrode@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef __GDM_DISPLAY_ACCESS_FILE_H__
#define __GDM_DISPLAY_ACCESS_FILE_H__

#include <glib.h>
#include <glib-object.h>

#include "gdm-display.h"

G_BEGIN_DECLS
#define GDM_TYPE_DISPLAY_ACCESS_FILE            (gdm_display_access_file_get_type ())
#define GDM_DISPLAY_ACCESS_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_DISPLAY_ACCESS_FILE, GdmDisplayAccessFile))
#define GDM_DISPLAY_ACCESS_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_DISPLAY_ACCESS_FILE, GdmDisplayAccessFileClass))
#define GDM_IS_DISPLAY_ACCESS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_DISPLAY_ACCESS_FILE))
#define GDM_IS_DISPLAY_ACCESS_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_DISPLAY_ACCESS_FILE))
#define GDM_DISPLAY_ACCESS_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GDM_TYPE_DISPLAY_ACCESS_FILE, GdmDisplayAccessFileClass))
#define GDM_DISPLAY_ACCESS_FILE_ERROR           (gdm_display_access_file_error_quark ())

typedef struct _GdmDisplayAccessFile GdmDisplayAccessFile;
typedef struct _GdmDisplayAccessFileClass GdmDisplayAccessFileClass;
typedef struct _GdmDisplayAccessFilePrivate GdmDisplayAccessFilePrivate;
typedef enum _GdmDisplayAccessFileError GdmDisplayAccessFileError;

struct _GdmDisplayAccessFile
{
        GObject parent;

        GdmDisplayAccessFilePrivate *priv;
};

struct _GdmDisplayAccessFileClass
{
        GObjectClass parent_class;
};

enum _GdmDisplayAccessFileError
{
        GDM_DISPLAY_ACCESS_FILE_ERROR_GENERAL = 0,
        GDM_DISPLAY_ACCESS_FILE_ERROR_FINDING_AUTH_ENTRY
};

GQuark                gdm_display_access_file_error_quark             (void);
GType                 gdm_display_access_file_get_type                (void);

GdmDisplayAccessFile *gdm_display_access_file_new                     (const char            *username);
gboolean              gdm_display_access_file_open                    (GdmDisplayAccessFile  *file,
                                                                       GError               **error);
gboolean              gdm_display_access_file_add_display             (GdmDisplayAccessFile  *file,
                                                                       GdmDisplay            *display,
                                                                       char                 **cookie,
                                                                       gsize                 *cookie_size,
                                                                       GError               **error);
gboolean              gdm_display_access_file_add_display_with_cookie (GdmDisplayAccessFile  *file,
                                                                       GdmDisplay            *display,
                                                                       const char            *cookie,
                                                                       gsize                  cookie_size,
                                                                       GError               **error);
gboolean              gdm_display_access_file_remove_display          (GdmDisplayAccessFile  *file,
                                                                       GdmDisplay            *display,
                                                                       GError               **error);

void                  gdm_display_access_file_close                   (GdmDisplayAccessFile  *file);
char                 *gdm_display_access_file_get_path                (GdmDisplayAccessFile  *file);

G_END_DECLS
#endif /* __GDM_DISPLAY_ACCESS_FILE_H__ */
