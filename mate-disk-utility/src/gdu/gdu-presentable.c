/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-presentable.h
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

#include "gdu-presentable.h"
#include "gdu-pool.h"

/**
 * SECTION:gdu-presentable
 * @title: GduPresentable
 * @short_description: Interface for devices presentable to the end user
 *
 * All storage devices in <literal>UNIX</literal> and
 * <literal>UNIX</literal>-like operating systems are mostly
 * represented by so-called <literal>block</literal> devices at the
 * kernel/user-land interface. UNIX block devices, including
 * information and operations are represented by the #GduDevice class.
 *
 * However, from an user-interface point of view, it's useful to make
 * a finer-grained distinction; for example it's useful to make a
 * distinction between drives (e.g. a phyiscal hard disk, optical
 * drives) and volumes (e.g. a mountable file system).
 *
 * As such, classes encapsulating aspects of a UNIX block device (such
 * as it being drive, volume, empty space) that are interesting to
 * present in the user interface all implement the #GduPresentable
 * interface. This interface provides lowest-common denominator
 * functionality assisting in the creation of user interfaces; name
 * and icons are easily available as well as hierarchical grouping
 * in terms of parent/child relationships. Thus, several classes
 * such as #GduVolume, #GduDrive and others implement the
 * #GduPresentable interface
 *
 * For example, if a device (<literal>/dev/sda</literal>) is
 * partitioned into two partitions (<literal>/dev/sda1</literal> and
 * <literal>/dev/sda2</literal>), the parent/child relation look looks
 * like this
 *
 * <programlisting>
 * GduDrive     (/dev/sda)
 *   GduVolume  (/dev/sda1)
 *   GduVolume  (/dev/sda2)
 * </programlisting>
 *
 * Some partitioning schemes (notably Master Boot Record) have a
 * concept of nested partition tables. Supposed
 * <literal>/dev/sda2</literal> is an extended partition and
 * <literal>/dev/sda5</literal> and <literal>/dev/sda6</literal> are
 * logical partitions:
 *
 * <programlisting>
 * GduDrive       (/dev/sda)
 *   GduVolume    (/dev/sda1)
 *   GduVolume    (/dev/sda2)
 *     GduVolume  (/dev/sda5)
 *     GduVolume  (/dev/sda6)
 * </programlisting>
 *
 * The gdu_presentable_get_offset() function can be used to
 * determine the ordering; this function will return the offset
 * of a #GduPresentable relative to the topmost enclosing device.
 *
 * Now, consider the case where there are "holes", e.g. where
 * there exists one or more regions on the partitioned device
 * not occupied by any partitions. In that case, the #GduPool
 * object will create #GduVolumeHole objects to patch the holes:
 *
 * <programlisting>
 * GduDrive           (/dev/sda)
 *   GduVolume        (/dev/sda1)
 *   GduVolume        (/dev/sda2)
 *     GduVolume      (/dev/sda5)
 *     GduVolumeHole  (no device)
 *     GduVolume      (/dev/sda6)
 *   GduVolumeHole    (no device)
 * </programlisting>
 *
 * Also, some devices are not partitioned. For example, the UNIX
 * block device <literal>/dev/sr0</literal> refers to both the
 * optical drive and (if any medium is present) the contents of
 * the optical disc inserted into the disc. In that case, the
 * following structure will be created:
 *
 * <programlisting>
 * GduDrive     (/dev/sr0)
 *   GduVolume  (/dev/sr0)
 * </programlisting>
 *
 * If no media is available, only a single #GduDrive object will
 * exist:
 *
 * <programlisting>
 * GduDrive     (/dev/sr0)
 * </programlisting>
 *
 * Finally, unlocked LUKS Encrypted devices are represented as
 * children of their encrypted counter parts, for example:
 *
 * <programlisting>
 * GduDrive       (/dev/sda)
 *   GduVolume    (/dev/sda1)
 *   GduVolume    (/dev/sda2)
 *     GduVolume  (/dev/dm-0)
 * </programlisting>
 *
 * Some devices, like RAID and LVM devices, needs to be assembled from
 * components (e.g. "activated" or "started". This is encapsulated in
 * the #GduActivatableDrive class; this is not much different from
 * #GduDrive except that there only is a #GduDevice assoicated with
 * the object when the device itself is started. For example:
 *
 * <programlisting>
 * GduActivatableDrive     (no device)
 * </programlisting>
 *
 * will be created (e.g. synthesized) as soon as the first component
 * of the activatable drive is available. When activated, the
 * #GduActivatableDrive will gain a #GduDevice and the hierarchy looks
 * somewhat like this
 *
 * <programlisting>
 * GduActivatableDrive     (/dev/md0)
 *   GduVolume             (/dev/md0)
 * </programlisting>
 *
 * To sum up, the #GduPresentable interface (and classes implementing
 * it such as #GduDrive and #GduVolume) describe how a drive / medium
 * is organized such that it's easy to compose an user interface. To
 * perform operations on devices, use gdu_presentable_get_device() and
 * the functions on #GduDevice.
 **/


static void gdu_presentable_base_init (gpointer g_class);
static void gdu_presentable_class_init (gpointer g_class,
                                        gpointer class_data);

GType
gdu_presentable_get_type (void)
{
  static GType presentable_type = 0;

  if (! presentable_type)
    {
      static const GTypeInfo presentable_info =
      {
        sizeof (GduPresentableIface), /* class_size */
	gdu_presentable_base_init,   /* base_init */
	NULL,		/* base_finalize */
	gdu_presentable_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      presentable_type =
	g_type_register_static (G_TYPE_INTERFACE, "GduPresentable",
				&presentable_info, 0);

      g_type_interface_add_prerequisite (presentable_type, G_TYPE_OBJECT);
    }

  return presentable_type;
}

static void
gdu_presentable_class_init (gpointer g_class,
                     gpointer class_data)
{
}

static void
gdu_presentable_base_init (gpointer g_class)
{
        static gboolean initialized = FALSE;

        if (! initialized)
        {
                /**
                 * GduPresentable::changed
                 * @presentable: A #GduPresentable.
                 *
                 * Emitted when @presentable changes.
                 **/
                g_signal_new ("changed",
                              GDU_TYPE_PRESENTABLE,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPresentableIface, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

                /**
                 * GduPresentable::job-changed
                 * @presentable: A #GduPresentable.
                 *
                 * Emitted when job status on @presentable changes.
                 **/
                g_signal_new ("job-changed",
                              GDU_TYPE_PRESENTABLE,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPresentableIface, job_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

                /**
                 * GduPresentable::removed
                 * @presentable: The #GduPresentable that was removed.
                 *
                 * Emitted when @presentable is removed. Recipients
                 * should release references to @presentable.
                 **/
                g_signal_new ("removed",
                              GDU_TYPE_PRESENTABLE,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPresentableIface, removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

                initialized = TRUE;
        }
}

/**
 * gdu_presentable_get_id:
 * @presentable: A #GduPresentable.
 *
 * Gets a stable identifier for @presentable.
 *
 * Returns: An stable identifier for @presentable. Do not free, the string is
 * owned by @presentable.
 **/
const gchar *
gdu_presentable_get_id (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_id) (presentable);
}

/**
 * gdu_presentable_get_device:
 * @presentable: A #GduPresentable.
 *
 * Gets the underlying device for @presentable if one is available.
 *
 * Returns: A #GduDevice or #NULL if there are no underlying device of
 * @presentable. Caller must unref the object when done with it.
 **/
GduDevice *
gdu_presentable_get_device (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_device) (presentable);
}

/**
 * gdu_presentable_get_enclosing_presentable:
 * @presentable: A #GduPresentable.
 *
 * Gets the #GduPresentable that is the parent of @presentable or
 * #NULL if there is no parent.
 *
 * Returns: The #GduPresentable that is a parent of @presentable or
 * #NULL if @presentable is the top-most presentable already. Caller
 * must unref the object.
 **/
GduPresentable *
gdu_presentable_get_enclosing_presentable (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_enclosing_presentable) (presentable);
}

/**
 * gdu_presentable_get_name:
 * @presentable: A #GduPresentable.
 *
 * Gets a name for @presentable suitable for presentation in an user
 * interface.
 *
 * For drives (e.g. #GduDrive) this could be "80G Solid-State Disk",
 * "500G Hard Disk", "CompactFlash Drive", "CD/DVD Drive"
 *
 * For volumes (e.g. #GduVolume) this string could be "Fedora
 * (Rawhide)" (for filesystems with labels), "48G Free" (for
 * unallocated space), "2GB Swap Space" (for swap), "430GB RAID
 * Component" (for RAID components) and so on.
 *
 * Constrast with gdu_presentable_get_vpd_name() that returns a name
 * based on Vital Product Data including things such as the name of
 * the vendor, the model name and so on.
 *
 * Returns: The name. Caller must free the string with g_free().
 **/
gchar *
gdu_presentable_get_name (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_name) (presentable);
}

/**
 * gdu_presentable_get_vpd_name:
 * @presentable: A #GduPresentable.
 *
 * Gets a name for @presentable suitable for presentation in an user
 * interface that includes Vital Product Data details such as the name
 * of the vendor, the model name and so on.
 *
 * For drives (e.g. #GduDrive) this typically includes the
 * vendor/model strings obtained from the hardware, for example
 * "MATSHITA DVD/CDRW UJDA775" or "ATA INTEL SSDSA2MH080G1GC".
 *
 * For volumes (e.g. #GduVolume) this includes information about e.g.
 * partition infomation for example "Whole-disk volume on ATA INTEL
 * SSDSA2MH080G1GC", "Partition 2 of ATA INTEL SSDSA2MH080G1GC".
 *
 * Contrast with gdu_presentable_get_name() that may not include this
 * information.
 *
 * Returns: The name. Caller must free the string with g_free().
 **/
gchar *
gdu_presentable_get_vpd_name (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_vpd_name) (presentable);
}

/**
 * gdu_presentable_get_description:
 * @presentable: A #GduPresentable.
 *
 * Gets a description for @presentable suitable for presentation in an user
 * interface.
 *
 * Returns: The description. Caller must free the string with g_free().
 */
gchar *
gdu_presentable_get_description (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_description) (presentable);
}

/**
 * gdu_presentable_get_icon:
 * @presentable: A #GduPresentable.
 *
 * Gets a #GIcon suitable for display in an user interface.
 *
 * Returns: The icon. Caller must free this with g_object_unref().
 **/
GIcon *
gdu_presentable_get_icon (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_icon) (presentable);
}

/**
 * gdu_presentable_get_offset:
 * @presentable: A #GduPresentable.
 *
 * Gets where the data represented by the presentable starts on the
 * underlying main block device
 *
 * Returns: Offset of @presentable or 0 if @presentable has no underlying device.
 **/
guint64
gdu_presentable_get_offset (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), 0);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_offset) (presentable);
}

/**
 * gdu_presentable_get_size:
 * @presentable: A #GduPresentable.
 *
 * Gets the size of @presentable.
 *
 * Returns: The size of @presentable or 0 if @presentable has no underlying device.
 **/
guint64
gdu_presentable_get_size (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), 0);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_size) (presentable);
}

/**
 * gdu_presentable_get_pool:
 * @presentable: A #GduPresentable.
 *
 * Gets the #GduPool that @presentable stems from.
 *
 * Returns: A #GduPool. Caller must unref object when done with it.
 **/
GduPool *
gdu_presentable_get_pool (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->get_pool) (presentable);
}

/**
 * gdu_presentable_is_allocated:
 * @presentable: A #GduPresentable.
 *
 * Determines if @presentable represents an underlying block device with data.
 *
 * Returns: Whether @presentable is allocated.
 **/
gboolean
gdu_presentable_is_allocated (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), FALSE);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->is_allocated) (presentable);
}

/**
 * gdu_presentable_is_recognized:
 * @presentable: A #GduPresentable.
 *
 * Gets whether the contents of @presentable are recognized; e.g. if
 * it's a file system, encrypted data or swap space.
 *
 * Returns: Whether @presentable is recognized.
 **/
gboolean
gdu_presentable_is_recognized (GduPresentable *presentable)
{
  GduPresentableIface *iface;

  g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), FALSE);

  iface = GDU_PRESENTABLE_GET_IFACE (presentable);

  return (* iface->is_recognized) (presentable);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * gdu_presentable_get_toplevel:
 * @presentable: A #GduPresentable.
 *
 * Gets the top-level presentable for a given presentable.
 *
 * Returns: A #GduPresentable or #NULL if @presentable is the top-most presentable. Caller must
 * unref the object when done with it
 **/
GduPresentable *
gdu_presentable_get_toplevel (GduPresentable *presentable)
{
        GduPresentable *parent;
        GduPresentable *maybe_parent;

        parent = presentable;
        do {
                maybe_parent = gdu_presentable_get_enclosing_presentable (parent);
                if (maybe_parent != NULL) {
                        g_object_unref (maybe_parent);
                        parent = maybe_parent;
                }
        } while (maybe_parent != NULL);

        return g_object_ref (parent);
}

/* ---------------------------------------------------------------------------------------------------- */
guint
gdu_presentable_hash (GduPresentable *presentable)
{
        return g_str_hash (gdu_presentable_get_id (presentable));
}

gboolean
gdu_presentable_equals (GduPresentable *a,
                        GduPresentable *b)
{
        return g_strcmp0 (gdu_presentable_get_id (a), gdu_presentable_get_id (b)) == 0;
}

static void
compute_sort_path (GduPresentable *presentable,
                   GString        *s)
{
        GduPresentable *enclosing_presentable;

        enclosing_presentable = gdu_presentable_get_enclosing_presentable (presentable);
        if (enclosing_presentable != NULL) {
                compute_sort_path (enclosing_presentable, s);
                g_object_unref (enclosing_presentable);
        }

        g_string_append_printf (s, "_%s", gdu_presentable_get_id (presentable));
}

gint
gdu_presentable_compare (GduPresentable *a,
                         GduPresentable *b)
{
        GString *sort_a;
        GString *sort_b;
        gint ret;

        sort_a = g_string_new (NULL);
        sort_b = g_string_new (NULL);
        compute_sort_path (a, sort_a);
        compute_sort_path (b, sort_b);
        ret = strcmp (sort_a->str, sort_b->str);
        g_string_free (sort_a, TRUE);
        g_string_free (sort_b, TRUE);

        return ret;
}

GList *
gdu_presentable_get_enclosed (GduPresentable *presentable)
{
        GList *l;
        GList *presentables;
        GList *ret;
        GduPool *pool;

        pool = gdu_presentable_get_pool (presentable);
        presentables = gdu_pool_get_presentables (pool);

        ret = NULL;
        for (l = presentables; l != NULL; l = l->next) {
                GduPresentable *p = l->data;
                GduPresentable *e;

                e = gdu_presentable_get_enclosing_presentable (p);
                if (e != NULL) {
                        if (gdu_presentable_equals (e, presentable)) {
                                GList *enclosed_by_p;

                                ret = g_list_prepend (ret, g_object_ref (p));

                                enclosed_by_p = gdu_presentable_get_enclosed (p);
                                ret = g_list_concat (ret, enclosed_by_p);
                        }
                        g_object_unref (e);
                }
        }

        g_list_foreach (presentables, (GFunc) g_object_unref, NULL);
        g_list_free (presentables);
        g_object_unref (pool);
        return ret;
}

gboolean
gdu_presentable_encloses (GduPresentable *a,
                          GduPresentable *b)
{
        GList *enclosed_by_a;
        GList *l;
        gboolean ret;

        ret = FALSE;
        enclosed_by_a = gdu_presentable_get_enclosed (a);
        for (l = enclosed_by_a; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                if (gdu_presentable_equals (b, p)) {
                        ret = TRUE;
                        break;
                }
        }
        g_list_foreach (enclosed_by_a, (GFunc) g_object_unref, NULL);
        g_list_free (enclosed_by_a);

        return ret;
}

