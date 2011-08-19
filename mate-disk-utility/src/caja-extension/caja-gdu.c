/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  caja-gdu.c
 *
 *  Copyright (C) 2008-2009 Red Hat, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Tomas Bzatek <tbzatek@redhat.com>
 *
 */

#include "config.h"

#include "caja-gdu.h"
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include "gdu/gdu.h"

static void caja_gdu_instance_init (CajaGdu      *gdu);
static void caja_gdu_class_init    (CajaGduClass *klass);

static GType caja_gdu_type = 0;


static GduDevice *
get_device_for_device_file (const gchar *device_file)
{
        GduPool *pool;
        GduDevice *device;

        pool = NULL;
        device = NULL;

        if (device_file == NULL || strlen (device_file) <= 1)
                goto out;

        pool = gdu_pool_new ();
        device = gdu_pool_get_by_device_file (pool, device_file);

 out:
        if (pool != NULL)
                g_object_unref (pool);
        return device;
}

static GduDevice *
get_device_from_caja_file (CajaFileInfo *caja_file)
{
        GFile *file;
        GFileInfo *info;
        GError *error;
        GFileType file_type;
        GMount *mount;
        GVolume *volume;
        GduDevice *device;
        gchar *device_file;

        g_return_val_if_fail (caja_file != NULL, NULL);

        device = NULL;
        device_file = NULL;

        file = caja_file_info_get_location (caja_file);
        if (file == NULL)
                goto out;

        file_type = caja_file_info_get_file_type (caja_file);

        /* first try to find mount target from a mountable  */
        if (file_type == G_FILE_TYPE_MOUNTABLE || file_type == G_FILE_TYPE_SHORTCUT) {
                /* get a mount if exists and extract device file from it  */
                mount = caja_file_info_get_mount (caja_file);
                if (mount != NULL) {
                        volume = g_mount_get_volume (mount);
                        if (volume != NULL) {
                                device_file = g_volume_get_identifier (volume,
                                                                       G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
                                g_object_unref (volume);
                        }
                        g_object_unref (mount);
                }

                /* not mounted, assuming we've been spawned from computer://  */
                if (device_file == NULL) {
                        /* retrieve DeviceKit device ID for non-mounted devices  */
                        error = NULL;
                        info = g_file_query_info (file, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE, G_FILE_QUERY_INFO_NONE, NULL, &error);
                        if (info != NULL) {
                                device_file = g_file_info_get_attribute_as_string (info, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE);
                                g_object_unref (info);
                        }
                        if (error != NULL) {
                                g_warning ("unable to query info: %s\n", error->message);
                                g_error_free (error);
                        }
                 }
        }

 out:
        if (device_file != NULL) {
                device = get_device_for_device_file (device_file);
                g_free (device_file);
        }

        return device;
}

static void
open_format_utility (CajaMenuItem *item)
{
        const gchar *format_argv[4];
        GduDevice *device;
        GError *error;

        device = GDU_DEVICE (g_object_get_data (G_OBJECT (item), "gdu-device"));

        format_argv[0] = LIBEXECDIR "/gdu-format-tool";
        format_argv[1] = "--device-file";
        format_argv[2] = gdu_device_get_device_file (device);
        format_argv[3] = NULL;

        error = NULL;
        g_spawn_async (NULL,        /* working_directory */
                       (gchar **) format_argv,
                       NULL,        /* envp */
                       0,           /* flags */
                       NULL,        /* child_setup */
                       NULL,        /* user_data */
                       NULL,        /* child_pid */
                       &error);
        if (error != NULL) {
                /* TODO: would be nice with a GtkMessageDialog here */
                g_warning ("Error launching " LIBEXECDIR "/gdu-format-tool: %s", error->message);
                g_error_free (error);
        }
}

static void
format_callback (CajaMenuItem *item,
                 gpointer user_data)
{
        open_format_utility (g_object_ref (item));
}

static GList *
caja_gdu_get_file_items (CajaMenuProvider *provider,
			     GtkWidget            *window,
			     GList                *files)
{
        CajaMenuItem *item;
        CajaFileInfo *caja_file;
        GduDevice *device;
        GList *items;
        GduPresentable *volume;
        GduPool *pool;

        items = NULL;
        pool = NULL;
        device = NULL;
        volume = NULL;

        if (g_list_length (files) != 1)
                goto out;

        caja_file = (CajaFileInfo*) files->data;
        device = get_device_from_caja_file (caja_file);
        if (device == NULL)
                goto out;

        pool = gdu_device_get_pool (device);

        /* If the device is a cleartext LUKS device, don't attempt
         * to format the cleartext device as that's gonna end with
         * strange loops such as
         *
         *      - USB Thumbdrive (/dev/sdb)
         *       - Encrypted Data (/dev/sdb1, LUKS)
         *        - Encrypted Data (/dev/dm-0, LUKS)
         *         - My File System (/dev/dm-1, ext3 fs)
         *
         * and we specifically refuse to handle that in the GVfs gdu
         * volume monitor. So find the cryptotext device instead.
         */
        if (gdu_device_is_luks_cleartext (device)) {
                GduDevice *cleartext_device;
                const gchar *cryptotext_objpath;

                cleartext_device = device;
                device = NULL;
                cryptotext_objpath = gdu_device_luks_cleartext_get_slave (cleartext_device);
                device = gdu_pool_get_by_object_path (pool, cryptotext_objpath);
                g_object_unref (cleartext_device);
        }

        /* only allow to format devices that are actual volumes - e.g. not empty
         * drives
         */
        volume = gdu_pool_get_volume_by_device (pool, device);
        if (volume == NULL)
                goto out;

        /* never allow to format optical discs - brasero / caja-cd-burner handles
         * that kind of media
         */
        if (gdu_device_is_optical_disc (device))
                goto out;

        item = caja_menu_item_new ("CajaGdu::format",
                                       /* Translators: this is a verb */
                                       _("_Format..."),
                                       _("Create new filesystem on the selected device"),
                                       "caja-gdu");
        g_object_set_data_full (G_OBJECT (item),
                                "gdu-device",
                                g_object_ref (device),
                                (GDestroyNotify) g_object_unref);
        g_object_set_data_full (G_OBJECT (item),
                                "caja-file",
                                g_object_ref (caja_file),
                                (GDestroyNotify) g_object_unref);
        g_signal_connect (item, "activate",
                          G_CALLBACK (format_callback),
                          NULL);

        items = g_list_append (NULL, item);

out:
        if (volume != NULL)
                g_object_unref (volume);
        if (device != NULL)
                g_object_unref (device);
        if (pool != NULL)
                g_object_unref (pool);
        return items;
}

static void
caja_gdu_menu_provider_iface_init (CajaMenuProviderIface *iface)
{
        iface->get_file_items = caja_gdu_get_file_items;
}

static void
caja_gdu_instance_init (CajaGdu *gdu)
{
}

static void
caja_gdu_class_init (CajaGduClass *klass)
{
}

GType
caja_gdu_get_type (void)
{
  return caja_gdu_type;
}

void
caja_gdu_register_type (GTypeModule *module)
{
  const GTypeInfo info = {
          sizeof (CajaGduClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) caja_gdu_class_init,
          NULL,
          NULL,
          sizeof (CajaGdu),
          0,
          (GInstanceInitFunc) caja_gdu_instance_init,
  };

  const GInterfaceInfo menu_provider_iface_info = {
          (GInterfaceInitFunc) caja_gdu_menu_provider_iface_init,
          NULL,
          NULL
  };

  caja_gdu_type = g_type_module_register_type (module,
                                                   G_TYPE_OBJECT,
                                                   "CajaGdu",
                                                   &info, 0);

  g_type_module_add_interface (module,
                               caja_gdu_type,
                               CAJA_TYPE_MENU_PROVIDER,
                               &menu_provider_iface_info);
}
