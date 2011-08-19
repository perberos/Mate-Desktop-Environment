/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-filesystem-type.c - Handling of filesystems for the MATE Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "mate-vfs-filesystem-type.h"

struct FSInfo {
	char *fs_type;
	char *fs_name;
	gboolean use_trash;
};

/* NB: Keep in sync with mate_vfs_volume_get_filesystem_type()
 * documentation! */
static struct FSInfo fs_data[] = {
	{ "affs"     , N_("AFFS Volume"), 0},
	{ "afs"      , N_("AFS Network Volume"), 0 },
	{ "auto"     , N_("Auto-detected Volume"), 0 },
	{ "btrfs"    , N_("Btrfs Linux Volume"), 1 },
	{ "cd9660"   , N_("CD-ROM Drive"), 0 },
	{ "cdda"     , N_("CD Digital Audio"), 0 },
	{ "cdrom"    , N_("CD-ROM Drive"), 0 },
	{ "devfs"    , N_("Hardware Device Volume"), 0 },
	{ "encfs"    , N_("EncFS Volume"), 1 },
	{ "ecryptfs" , N_("eCryptfs Volume"), 1},
	{ "ext2"     , N_("Ext2 Linux Volume"), 1 },
	{ "ext2fs"   , N_("Ext2 Linux Volume"), 1 },
	{ "ext3"     , N_("Ext3 Linux Volume"), 1 },
	{ "ext4"     , N_("Ext4 Linux Volume"), 1 },
	{ "ext4dev"  , N_("Ext4 Linux Volume"), 1 },
	{ "fat"      , N_("MSDOS Volume"), 1 },
	{ "ffs"      , N_("BSD Volume"), 1 },
	{ "fuse"     , N_("FUSE Volume"), 1 },
	{ "hfs"	     , N_("MacOS Volume"), 1 },
	{ "hfsplus"  , N_("MacOS Volume"), 0 },
	{ "iso9660"  , N_("CDROM Volume"), 0 },
	{ "hsfs"     , N_("Hsfs CDROM Volume"), 0 },
	{ "jfs"      , N_("JFS Volume"), 1 },
	{ "hpfs"     , N_("Windows NT Volume"), 0 },
	{ "kernfs"   , N_("System Volume"), 0 },
	{ "lfs"      , N_("BSD Volume"), 1 },
	{ "linprocfs", N_("System Volume"), 0 },
	{ "mfs"      , N_("Memory Volume"), 1 },
	{ "minix"    , N_("Minix Volume"), 0 },
	{ "msdos"    , N_("MSDOS Volume"), 0 },
	{ "msdosfs"  , N_("MSDOS Volume"), 0 },
	{ "nfs"      , N_("NFS Network Volume"), 1 },
	{ "ntfs"     , N_("Windows NT Volume"), 0 },
	{ "ntfs-3g"  , N_("Windows NT Volume"), 1 },
	{ "nilfs2"   , N_("NILFS Linux Volume"), 1 },
	{ "nwfs"     , N_("Netware Volume"), 0 },
	{ "proc"     , N_("System Volume"), 0 },
	{ "procfs"   , N_("System Volume"), 0 },
	{ "ptyfs"    , N_("System Volume"), 0 },
	{ "reiser4"  , N_("Reiser4 Linux Volume"), 1 },
	{ "reiserfs" , N_("ReiserFS Linux Volume"), 1 },
	{ "smbfs"    , N_("Windows Shared Volume"), 1 },
	{ "supermount",N_("SuperMount Volume"), 0 },
	{ "udf"      , N_("DVD Volume"), 0 },
	{ "ufs"      , N_("Solaris/BSD Volume"), 1 },
	{ "udfs"     , N_("Udfs Solaris Volume"), 1 },
	{ "pcfs"     , N_("Pcfs Solaris Volume"), 1 },
	{ "samfs"    , N_("Sun SAM-QFS Volume"), 1 },
	{ "tmpfs"    , N_("Temporary Volume"), 1 },
	{ "umsdos"   , N_("Enhanced DOS Volume"), 0 },
	{ "vfat"     , N_("Windows VFAT Volume"), 1 },
	{ "xenix"    , N_("Xenix Volume"), 0 },
	{ "xfs"      , N_("XFS Linux Volume"), 1 },
	{ "xiafs"    , N_("XIAFS Volume"), 0 },
	{ "cifs"     , N_("CIFS Volume"), 1 },
};

static struct FSInfo *
find_fs_info (const char *fs_type)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (fs_data); i++) {
		if (strcmp (fs_data[i].fs_type, fs_type) == 0) {
			return &fs_data[i];
		}
	}
	
	return NULL;
}

char *
_mate_vfs_filesystem_volume_name (const char *fs_type)
{
	struct FSInfo *info;

	info = find_fs_info (fs_type);

	if (info != NULL) {
		return g_strdup (_(info->fs_name));
	}
	
	return g_strdup (_("Unknown"));
}
     
gboolean
_mate_vfs_filesystem_use_trash (const char *fs_type)
{
	struct FSInfo *info;

	info = find_fs_info (fs_type);

	if (info != NULL) {
		return info->use_trash;
	}
	
	return FALSE;
}
