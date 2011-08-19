/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-cdrom.c - cdrom utilities

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
           Gene Z. Ragan <gzr@eazel.com>
           Seth Nickell  <snickell@stanford.edu>
*/

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#ifdef __linux__
#include <linux/cdrom.h>
#endif

#ifdef HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#endif

#include "mate-vfs-iso9660.h"
#include "mate-vfs-cdrom.h"


int
_mate_vfs_get_cdrom_type (const char *vol_dev_path, int* fd)
{
#if defined (HAVE_SYS_MNTTAB_H)   /* Solaris */
	GString *new_dev_path;
	struct cdrom_tocentry entry;
	struct cdrom_tochdr header;
	int type;

	/* For ioctl call to work dev_path has to be /vol/dev/rdsk.
	 * There has to be a nicer way to do this.
	 */
	new_dev_path = g_string_new (vol_dev_path);
	new_dev_path = g_string_insert_c (new_dev_path, 9, 'r');
	*fd = open (new_dev_path->str, O_RDONLY | O_NONBLOCK);
	g_string_free (new_dev_path, TRUE);

	if (*fd < 0) {
		return -1;
	}

	if (ioctl (*fd, CDROMREADTOCHDR, &header) == 0) {
		return -1;
	}

	type = CDS_DATA_1;
	
	for (entry.cdte_track = 1;
	     entry.cdte_track <= header.cdth_trk1;
	     entry.cdte_track++) {
		entry.cdte_format = CDROM_LBA;
		if (ioctl (*fd, CDROMREADTOCENTRY, &entry) == 0) {
			if (entry.cdte_ctrl & CDROM_DATA_TRACK) {
				type = CDS_AUDIO;
				break;
			}
		}
	}

	return type;
#elif defined(HAVE_SYS_MNTCTL_H)
	return CDS_NO_INFO;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
	struct ioc_toc_header header;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
	struct ioc_read_toc_single_entry entry;
#else
	struct ioc_read_toc_entry entries;
	struct cd_toc_entry entry;
	int i;
#endif
	int type;
#ifndef CDROM_DATA_TRACK
#define CDROM_DATA_TRACK 4
#endif

	*fd = open (vol_dev_path, O_RDONLY|O_NONBLOCK);
	if (*fd < 0) {
	    	return -1;
	}

	if (ioctl (*fd, CDIOREADTOCHEADER, &header) == 0) {
	    	return -1;
	}

	type = CDS_DATA_1;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
	for (entry.track = header.starting_track;
		entry.track <= header.ending_track;
		entry.track++) {
	    	entry.address_format = CD_LBA_FORMAT;
		if (ioctl (*fd, CDIOREADTOCENTRY, &entry) == 0) {
		    	if (entry.entry.control & CDROM_DATA_TRACK) {
			    	type = CDS_AUDIO;
				break;
			}
		}
	}

#else /* defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)*/
	entries.data_len = sizeof(entry);
	entries.data = &entry;
	for (i = header.starting_track; i <= header.ending_track; i++) {
		entries.starting_track = i;
	    	entries.address_format = CD_LBA_FORMAT;
		if (ioctl (*fd, CDIOREADTOCENTRYS, &entries) == 0) {
		    	if (entry.control & CDROM_DATA_TRACK) {
			    	type = CDS_AUDIO;
				break;
			}
		}
	}

#endif /* defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)*/
	return type;
#elif defined(__linux__)
	*fd = open (vol_dev_path, O_RDONLY|O_NONBLOCK);
	if (*fd  < 0) {
		return -1;
	}
	if (ioctl (*fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) != CDS_DISC_OK) {
		close (*fd);
		*fd = -1;
		return -1;
	}
	return ioctl (*fd, CDROM_DISC_STATUS, CDSL_CURRENT);
#else /* defined(__linux__) */
	return CDS_NO_INFO;
#endif /* defined(__linux__) */
}

#ifdef __linux__
static int
get_iso9660_volume_name_data_track_offset (int fd)
{
	struct cdrom_tocentry toc;
	char toc_header[2];
	int i, offset;

	if (ioctl (fd, CDROMREADTOCHDR, &toc_header)) {
		return 0;
	}

	for (i = toc_header[0]; i <= toc_header[1]; i++) {
		memset (&toc, 0, sizeof (struct cdrom_tocentry));
		toc.cdte_track = i;
		toc.cdte_format = CDROM_MSF;
		if (ioctl (fd, CDROMREADTOCENTRY, &toc)) {
			return 0;
		}

		if (toc.cdte_ctrl & CDROM_DATA_TRACK) {
			offset = ((i == 1) ? 0 :
				(int)toc.cdte_addr.msf.frame +
				(int)toc.cdte_addr.msf.second*75 +
				(int)toc.cdte_addr.msf.minute*75*60 - 150);
			return offset;
		}
	}

	return 0;
}
#endif

char *
_mate_vfs_get_iso9660_volume_name (int fd)
{
	struct iso_primary_descriptor iso_buffer;
	int offset;
	int i;
	int vd_alt_offset;
	gchar *joliet_label;

	memset (&iso_buffer, 0, sizeof (struct iso_primary_descriptor));
	
#ifdef __linux__
	offset = get_iso9660_volume_name_data_track_offset (fd);
#else
	offset = 0;
#endif

#define ISO_SECTOR_SIZE   2048
#define ISO_ROOT_START   (ISO_SECTOR_SIZE * (offset + 16))
#define ISO_VD_MAX        84

	joliet_label = NULL;
	for (i = 0, vd_alt_offset = ISO_ROOT_START + ISO_SECTOR_SIZE;
	     i < ISO_VD_MAX;
	     i++, vd_alt_offset += ISO_SECTOR_SIZE)
	{
		lseek (fd, (off_t) vd_alt_offset, SEEK_SET);
		read (fd, &iso_buffer, ISO_SECTOR_SIZE);
		if ((unsigned char)iso_buffer.type[0] == ISO_VD_END)
			break;
		if (iso_buffer.type[0] != ISO_VD_SUPPLEMENTARY)
			continue;
		joliet_label = g_convert (iso_buffer.volume_id, 32, "UTF-8",
		                          "UTF-16BE", NULL, NULL, NULL);
		if (!joliet_label)
			continue;
		break;
	}

	lseek (fd, (off_t) ISO_ROOT_START, SEEK_SET);
	read (fd, &iso_buffer, ISO_SECTOR_SIZE);

	if (iso_buffer.volume_id[0] == 0 && !joliet_label) {
		return g_strdup (_("ISO 9660 Volume"));
	}

	if (joliet_label) {
		if (strncmp (joliet_label, iso_buffer.volume_id, 16) != 0)
			return joliet_label;
		g_free (joliet_label);
	}
	return g_strndup (iso_buffer.volume_id, 32);
}

