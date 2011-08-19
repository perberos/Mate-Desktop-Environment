/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-known-filesystem.c
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

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>
#include <dbus/dbus-glib.h>
#include <stdlib.h>

#include "gdu-known-filesystem.h"
#include "gdu-private.h"

struct _GduKnownFilesystemPrivate
{
        char *id;
        char *name;
        gboolean supports_unix_owners;
        gboolean can_mount;
        gboolean can_create;
        guint max_label_len;
        gboolean supports_label_rename;
        gboolean supports_online_label_rename;
        gboolean supports_fsck;
        gboolean supports_online_fsck;
        gboolean supports_resize_enlarge;
        gboolean supports_online_resize_enlarge;
        gboolean supports_resize_shrink;
        gboolean supports_online_resize_shrink;
};

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (GduKnownFilesystem, gdu_known_filesystem, G_TYPE_OBJECT)

static void
gdu_known_filesystem_finalize (GduKnownFilesystem *known_filesystem)
{
        g_free (known_filesystem->priv->id);
        g_free (known_filesystem->priv->name);
        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (known_filesystem));
}

static void
gdu_known_filesystem_class_init (GduKnownFilesystemClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_known_filesystem_finalize;

        g_type_class_add_private (klass, sizeof (GduKnownFilesystemPrivate));
}

static void
gdu_known_filesystem_init (GduKnownFilesystem *known_filesystem)
{
        known_filesystem->priv = G_TYPE_INSTANCE_GET_PRIVATE (known_filesystem, GDU_TYPE_KNOWN_FILESYSTEM, GduKnownFilesystemPrivate);
}

GduKnownFilesystem *
_gdu_known_filesystem_new (gpointer data)
{
        GduKnownFilesystem *known_filesystem;
        GValue elem = {0};

        known_filesystem = GDU_KNOWN_FILESYSTEM (g_object_new (GDU_TYPE_KNOWN_FILESYSTEM, NULL));

        g_value_init (&elem, KNOWN_FILESYSTEMS_STRUCT_TYPE);
        g_value_set_static_boxed (&elem, data);
        dbus_g_type_struct_get (&elem,
                                0, &(known_filesystem->priv->id),
                                1, &(known_filesystem->priv->name),
                                2, &(known_filesystem->priv->supports_unix_owners),
                                3, &(known_filesystem->priv->can_mount),
                                4, &(known_filesystem->priv->can_create),
                                5, &(known_filesystem->priv->max_label_len),
                                6, &(known_filesystem->priv->supports_label_rename),
                                7, &(known_filesystem->priv->supports_online_label_rename),
                                8, &(known_filesystem->priv->supports_fsck),
                                9, &(known_filesystem->priv->supports_online_fsck),
                                10, &(known_filesystem->priv->supports_resize_enlarge),
                                11, &(known_filesystem->priv->supports_online_resize_enlarge),
                                12, &(known_filesystem->priv->supports_resize_shrink),
                                13, &(known_filesystem->priv->supports_online_resize_shrink),
                                G_MAXUINT);

        return known_filesystem;
}

const char *
gdu_known_filesystem_get_id (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->id;
}

const char *
gdu_known_filesystem_get_name (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->name;
}

gboolean
gdu_known_filesystem_get_supports_unix_owners (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_unix_owners;
}

gboolean
gdu_known_filesystem_get_can_mount (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->can_mount;
}

gboolean
gdu_known_filesystem_get_can_create (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->can_create;
}

guint
gdu_known_filesystem_get_max_label_len (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->max_label_len;
}

gboolean
gdu_known_filesystem_get_supports_label_rename (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_label_rename;
}

gboolean
gdu_known_filesystem_get_supports_online_label_rename (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_online_label_rename;
}

gboolean
gdu_known_filesystem_get_supports_fsck (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_fsck;
}

gboolean
gdu_known_filesystem_get_supports_online_fsck (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_online_fsck;
}

gboolean
gdu_known_filesystem_get_supports_resize_enlarge (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_resize_enlarge;
}

gboolean
gdu_known_filesystem_get_supports_online_resize_enlarge (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_online_resize_enlarge;
}

gboolean
gdu_known_filesystem_get_supports_resize_shrink (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_resize_shrink;
}

gboolean
gdu_known_filesystem_get_supports_online_resize_shrink (GduKnownFilesystem *known_filesystem)
{
        return known_filesystem->priv->supports_online_resize_shrink;
}

