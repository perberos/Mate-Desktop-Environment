/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * mdm-display-access-file.h - Abstraction around xauth cookies
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
#ifndef __MDM_DISPLAY_ACCESS_FILE_H__
#define __MDM_DISPLAY_ACCESS_FILE_H__

#include <glib.h>
#include <glib-object.h>

#include "mdm-display.h"

G_BEGIN_DECLS
#define MDM_TYPE_DISPLAY_ACCESS_FILE            (mdm_display_access_file_get_type ())
#define MDM_DISPLAY_ACCESS_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_DISPLAY_ACCESS_FILE, MdmDisplayAccessFile))
#define MDM_DISPLAY_ACCESS_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_DISPLAY_ACCESS_FILE, MdmDisplayAccessFileClass))
#define MDM_IS_DISPLAY_ACCESS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_DISPLAY_ACCESS_FILE))
#define MDM_IS_DISPLAY_ACCESS_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_DISPLAY_ACCESS_FILE))
#define MDM_DISPLAY_ACCESS_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MDM_TYPE_DISPLAY_ACCESS_FILE, MdmDisplayAccessFileClass))
#define MDM_DISPLAY_ACCESS_FILE_ERROR           (mdm_display_access_file_error_quark ())

typedef struct _MdmDisplayAccessFile MdmDisplayAccessFile;
typedef struct _MdmDisplayAccessFileClass MdmDisplayAccessFileClass;
typedef struct _MdmDisplayAccessFilePrivate MdmDisplayAccessFilePrivate;
typedef enum _MdmDisplayAccessFileError MdmDisplayAccessFileError;

struct _MdmDisplayAccessFile
{
        GObject parent;

        MdmDisplayAccessFilePrivate *priv;
};

struct _MdmDisplayAccessFileClass
{
        GObjectClass parent_class;
};

enum _MdmDisplayAccessFileError
{
        MDM_DISPLAY_ACCESS_FILE_ERROR_GENERAL = 0,
        MDM_DISPLAY_ACCESS_FILE_ERROR_FINDING_AUTH_ENTRY
};

GQuark                mdm_display_access_file_error_quark             (void);
GType                 mdm_display_access_file_get_type                (void);

MdmDisplayAccessFile *mdm_display_access_file_new                     (const char            *username);
gboolean              mdm_display_access_file_open                    (MdmDisplayAccessFile  *file,
                                                                       GError               **error);
gboolean              mdm_display_access_file_add_display             (MdmDisplayAccessFile  *file,
                                                                       MdmDisplay            *display,
                                                                       char                 **cookie,
                                                                       gsize                 *cookie_size,
                                                                       GError               **error);
gboolean              mdm_display_access_file_add_display_with_cookie (MdmDisplayAccessFile  *file,
                                                                       MdmDisplay            *display,
                                                                       const char            *cookie,
                                                                       gsize                  cookie_size,
                                                                       GError               **error);
gboolean              mdm_display_access_file_remove_display          (MdmDisplayAccessFile  *file,
                                                                       MdmDisplay            *display,
                                                                       GError               **error);

void                  mdm_display_access_file_close                   (MdmDisplayAccessFile  *file);
char                 *mdm_display_access_file_get_path                (MdmDisplayAccessFile  *file);

G_END_DECLS
#endif /* __MDM_DISPLAY_ACCESS_FILE_H__ */
