/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-hal-mounts.c - read and monitor volumes using freedesktop HAL

   Copyright (C) 2004-2005 Red Hat, Inc

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

   Author: David Zeuthen <davidz@redhat.com>
*/

#include <config.h>

#ifdef USE_HAL

/* uncomment to get helpful debug messages */
/* #define HAL_SHOW_DEBUG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libmatevfs/mate-vfs-utils.h>

#include <libhal.h>
#include <libhal-storage.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>


#include "mate-vfs-hal-mounts.h"
#include "mate-vfs-volume-monitor-daemon.h"
#include "mate-vfs-volume-monitor-private.h"

#ifndef PATHNAME_MAX
# define PATHNAME_MAX	1024
#endif

typedef struct {
	MateVFSVolumeMonitorDaemon *volume_monitor_daemon;
} MateVFSHalUserData;

typedef enum {
	HAL_ICON_DRIVE_REMOVABLE_DISK           = 0x10000,
	HAL_ICON_DRIVE_REMOVABLE_DISK_IDE       = 0x10001,
	HAL_ICON_DRIVE_REMOVABLE_DISK_SCSI      = 0x10002,
	HAL_ICON_DRIVE_REMOVABLE_DISK_USB       = 0x10003,
	HAL_ICON_DRIVE_REMOVABLE_DISK_IEEE1394  = 0x10004,
	HAL_ICON_DRIVE_REMOVABLE_DISK_CCW       = 0x10005,
	HAL_ICON_DRIVE_DISK                     = 0x10100,
	HAL_ICON_DRIVE_DISK_IDE                 = 0x10101,
	HAL_ICON_DRIVE_DISK_SCSI                = 0x10102,
	HAL_ICON_DRIVE_DISK_USB                 = 0x10103,
	HAL_ICON_DRIVE_DISK_IEEE1394            = 0x10104,
	HAL_ICON_DRIVE_DISK_CCW                 = 0x10105,
	HAL_ICON_DRIVE_CDROM                    = 0x10200,
	HAL_ICON_DRIVE_CDWRITER                 = 0x102ff,
	HAL_ICON_DRIVE_FLOPPY                   = 0x10300,
	HAL_ICON_DRIVE_TAPE                     = 0x10400,
	HAL_ICON_DRIVE_COMPACT_FLASH            = 0x10500,
	HAL_ICON_DRIVE_MEMORY_STICK             = 0x10600,
	HAL_ICON_DRIVE_SMART_MEDIA              = 0x10700,
	HAL_ICON_DRIVE_SD_MMC                   = 0x10800,
	HAL_ICON_DRIVE_CAMERA                   = 0x10900,
	HAL_ICON_DRIVE_PORTABLE_AUDIO_PLAYER    = 0x10a00,
	HAL_ICON_DRIVE_ZIP                      = 0x10b00,
        HAL_ICON_DRIVE_JAZ                      = 0x10c00,
        HAL_ICON_DRIVE_FLASH_KEY                = 0x10d00,

	HAL_ICON_VOLUME_REMOVABLE_DISK          = 0x20000,
	HAL_ICON_VOLUME_REMOVABLE_DISK_IDE      = 0x20001,
	HAL_ICON_VOLUME_REMOVABLE_DISK_SCSI     = 0x20002,
	HAL_ICON_VOLUME_REMOVABLE_DISK_USB      = 0x20003,
	HAL_ICON_VOLUME_REMOVABLE_DISK_IEEE1394 = 0x20004,
	HAL_ICON_VOLUME_REMOVABLE_DISK_CCW      = 0x20005,
	HAL_ICON_VOLUME_DISK                    = 0x20100,
	HAL_ICON_VOLUME_DISK_IDE                = 0x20101,
	HAL_ICON_VOLUME_DISK_SCSI               = 0x20102,
	HAL_ICON_VOLUME_DISK_USB                = 0x20103,
	HAL_ICON_VOLUME_DISK_IEEE1394           = 0x20104,
	HAL_ICON_VOLUME_DISK_CCW                = 0x20105,
	/* specifically left out as we use icons based on media type in the optical drive
	HAL_ICON_VOLUME_CDROM                   = 0x20200 */
	HAL_ICON_VOLUME_FLOPPY                  = 0x20300,
	HAL_ICON_VOLUME_TAPE                    = 0x20400,
	HAL_ICON_VOLUME_COMPACT_FLASH           = 0x20500,
	HAL_ICON_VOLUME_MEMORY_STICK            = 0x20600,
	HAL_ICON_VOLUME_SMART_MEDIA             = 0x20700,
	HAL_ICON_VOLUME_SD_MMC                  = 0x20800,
	HAL_ICON_VOLUME_CAMERA                  = 0x20900,
	HAL_ICON_VOLUME_PORTABLE_AUDIO_PLAYER   = 0x20a00,
	HAL_ICON_VOLUME_ZIP                     = 0x20b00,
        HAL_ICON_VOLUME_JAZ                     = 0x20c00,
        HAL_ICON_VOLUME_FLASH_KEY               = 0x20d00,

	HAL_ICON_DISC_CDROM                     = 0x30000,
	HAL_ICON_DISC_CDR                       = 0x30001,
	HAL_ICON_DISC_CDRW                      = 0x30002,
	HAL_ICON_DISC_DVDROM                    = 0x30003,
	HAL_ICON_DISC_DVDRAM                    = 0x30004,
	HAL_ICON_DISC_DVDR                      = 0x30005,
	HAL_ICON_DISC_DVDRW                     = 0x30006,
	HAL_ICON_DISC_DVDPLUSR                  = 0x30007,
	HAL_ICON_DISC_DVDPLUSRW                 = 0x30008,
	HAL_ICON_DISC_DVDPLUSR_DL               = 0x30009
} HalIcon;

typedef struct {
	HalIcon icon;
	const char *icon_path;
} HalIconPair;

/* by design, the enums are laid out so we can do easy computations */
static const HalIconPair hal_icon_mapping[] = {
	{HAL_ICON_DRIVE_REMOVABLE_DISK,           "mate-dev-removable"},
	{HAL_ICON_DRIVE_REMOVABLE_DISK_IDE,       "mate-dev-removable"},
	{HAL_ICON_DRIVE_REMOVABLE_DISK_SCSI,      "mate-dev-removable"},
	{HAL_ICON_DRIVE_REMOVABLE_DISK_USB,       "mate-dev-removable-usb"},
	{HAL_ICON_DRIVE_REMOVABLE_DISK_IEEE1394,  "mate-dev-removable-1394"},
	{HAL_ICON_DRIVE_REMOVABLE_DISK_CCW,       "mate-dev-removable"},
	{HAL_ICON_DRIVE_DISK,                     "mate-dev-removable"},
	{HAL_ICON_DRIVE_DISK_IDE,                 "mate-dev-removable"},
	{HAL_ICON_DRIVE_DISK_SCSI,                "mate-dev-removable"},       /* TODO: mate-dev-removable-scsi */
	{HAL_ICON_DRIVE_DISK_USB,                 "mate-dev-removable-usb"},
	{HAL_ICON_DRIVE_DISK_IEEE1394,            "mate-dev-removable-1394"},
	{HAL_ICON_DRIVE_DISK_CCW,                 "mate-dev-removable"},
	{HAL_ICON_DRIVE_CDROM,                    "mate-dev-removable"},       /* TODO: mate-dev-removable-cdrom */
	{HAL_ICON_DRIVE_CDWRITER,                 "mate-dev-removable"},       /* TODO: mate-dev-removable-cdwriter */
	{HAL_ICON_DRIVE_FLOPPY,                   "mate-dev-removable"},       /* TODO: mate-dev-removable-floppy */
	{HAL_ICON_DRIVE_TAPE,                     "mate-dev-removable"},       /* TODO: mate-dev-removable-tape */
	{HAL_ICON_DRIVE_COMPACT_FLASH,            "mate-dev-removable"},       /* TODO: mate-dev-removable-cf */
	{HAL_ICON_DRIVE_MEMORY_STICK,             "mate-dev-removable"},       /* TODO: mate-dev-removable-ms */
	{HAL_ICON_DRIVE_SMART_MEDIA,              "mate-dev-removable"},       /* TODO: mate-dev-removable-sm */
	{HAL_ICON_DRIVE_SD_MMC,                   "mate-dev-removable"},       /* TODO: mate-dev-removable-sdmmc */
	{HAL_ICON_DRIVE_CAMERA,                   "mate-dev-removable"},       /* TODO: mate-dev-removable-camera */
	{HAL_ICON_DRIVE_PORTABLE_AUDIO_PLAYER,    "mate-dev-removable"},       /* TODO: mate-dev-removable-ipod */
	{HAL_ICON_DRIVE_ZIP,                      "mate-dev-removable"},       /* TODO: mate-dev-removable-zip */
	{HAL_ICON_DRIVE_JAZ,                      "mate-dev-removable"},       /* TODO: mate-dev-removable-jaz */
	{HAL_ICON_DRIVE_FLASH_KEY,                "mate-dev-removable"},       /* TODO: mate-dev-removable-pendrive */

	{HAL_ICON_VOLUME_REMOVABLE_DISK,          "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_REMOVABLE_DISK_IDE,      "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_REMOVABLE_DISK_SCSI,     "mate-dev-harddisk"},        /* TODO: mate-dev-harddisk-scsi */
	{HAL_ICON_VOLUME_REMOVABLE_DISK_USB,      "mate-dev-harddisk-usb"},
	{HAL_ICON_VOLUME_REMOVABLE_DISK_IEEE1394, "mate-dev-harddisk-1394"},
	{HAL_ICON_VOLUME_REMOVABLE_DISK_CCW,      "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_DISK,                    "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_DISK_IDE,                "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_DISK_SCSI,               "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_DISK_USB,                "mate-dev-harddisk-usb"},
	{HAL_ICON_VOLUME_DISK_IEEE1394,           "mate-dev-harddisk-1394"},
	{HAL_ICON_VOLUME_DISK_CCW,                "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_FLOPPY,                  "mate-dev-floppy"},
	{HAL_ICON_VOLUME_TAPE,                    "mate-dev-harddisk"},
	{HAL_ICON_VOLUME_COMPACT_FLASH,           "mate-dev-media-cf"},
	{HAL_ICON_VOLUME_MEMORY_STICK,            "mate-dev-media-ms"},
	{HAL_ICON_VOLUME_SMART_MEDIA,             "mate-dev-media-sm"},
	{HAL_ICON_VOLUME_SD_MMC,                  "mate-dev-media-sdmmc"},
	{HAL_ICON_VOLUME_CAMERA,                  "camera"},
	{HAL_ICON_VOLUME_PORTABLE_AUDIO_PLAYER,   "mate-dev-ipod"},
	{HAL_ICON_VOLUME_ZIP,                     "mate-dev-zipdisk"},
	{HAL_ICON_VOLUME_JAZ,                     "mate-dev-jazdisk"},
	{HAL_ICON_VOLUME_FLASH_KEY,               "mate-dev-harddisk"},        /* TODO: mate-dev-pendrive */

	{HAL_ICON_DISC_CDROM,                     "mate-dev-cdrom"},
	{HAL_ICON_DISC_CDR,                       "mate-dev-disc-cdr"},
	{HAL_ICON_DISC_CDRW,                      "mate-dev-disc-cdrw"},
	{HAL_ICON_DISC_DVDROM,                    "mate-dev-disc-dvdrom"},
	{HAL_ICON_DISC_DVDRAM,                    "mate-dev-disc-dvdram"},
	{HAL_ICON_DISC_DVDR,                      "mate-dev-disc-dvdr"},
	{HAL_ICON_DISC_DVDRW,                     "mate-dev-disc-dvdrw"},
	{HAL_ICON_DISC_DVDPLUSR,                  "mate-dev-disc-dvdr-plus"},
	{HAL_ICON_DISC_DVDPLUSRW,                 "mate-dev-disc-dvdrw"},      /* TODO: mate-dev-disc-dvdrw-plus */
	{HAL_ICON_DISC_DVDPLUSR_DL,               "mate-dev-disc-dvdr-plus"},  /* TODO: mate-dev-disc-dvdr-plus-dl */

	{0x00, NULL}
};

/*------------------------------------------------------------------------*/

static const char *
_hal_lookup_icon (HalIcon icon)
{
	int i;
	const char *result;

	result = NULL;

	/* TODO: could make lookup better than O(n) */
	for (i = 0; hal_icon_mapping[i].icon_path != NULL; i++) {
		if (hal_icon_mapping[i].icon == icon) {
			result = hal_icon_mapping[i].icon_path;
			break;
		}
	}

	return result;
}

/* hal_volume may be NULL */
static char *
_hal_drive_policy_get_icon (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
			    LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	const char *name;
	LibHalDriveBus bus;
	LibHalDriveType drive_type;

	name = libhal_drive_get_dedicated_icon_drive (hal_drive);
	if (name != NULL)
		goto out;

	bus        = libhal_drive_get_bus (hal_drive);
	drive_type = libhal_drive_get_type (hal_drive);

	/* by design, the enums are laid out so we can do easy computations */

	switch (drive_type) {
	case LIBHAL_DRIVE_TYPE_REMOVABLE_DISK:
	case LIBHAL_DRIVE_TYPE_DISK:
		name = _hal_lookup_icon (0x10000 + drive_type*0x100 + bus);
		break;

	case LIBHAL_DRIVE_TYPE_CDROM:
	{
		LibHalDriveCdromCaps cdrom_caps;
		gboolean cdrom_can_burn;

		/* can burn if other flags than cdrom and dvdrom */
		cdrom_caps = libhal_drive_get_cdrom_caps (hal_drive);
		cdrom_can_burn = ((cdrom_caps & (LIBHAL_DRIVE_CDROM_CAPS_CDROM|
						 LIBHAL_DRIVE_CDROM_CAPS_DVDROM)) == cdrom_caps);

		name = _hal_lookup_icon (0x10000 + drive_type*0x100 + (cdrom_can_burn ? 0xff : 0x00));
		break;
	}

	default:
		name = _hal_lookup_icon (0x10000 + drive_type*0x100);
	}

out:
	if (name != NULL)
		return g_strdup (name);
	else {
		g_warning ("_hal_drive_policy_get_icon : error looking up icon; defaulting to mate-dev-removable");
		return g_strdup ("mate-dev-removable");
	}
}

static char *
_hal_volume_policy_get_icon (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
			     LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	const char *name;
	LibHalDriveBus bus;
	LibHalDriveType drive_type;
	LibHalVolumeDiscType disc_type;

	name = libhal_drive_get_dedicated_icon_volume (hal_drive);
	if (name != NULL)
		goto out;

	/* by design, the enums are laid out so we can do easy computations */

	if (libhal_volume_is_disc (hal_volume)) {
		disc_type = libhal_volume_get_disc_type (hal_volume);
		name = _hal_lookup_icon (0x30000 + disc_type);
		goto out;
	}

	if (hal_drive == NULL) {
		name = _hal_lookup_icon (HAL_ICON_VOLUME_REMOVABLE_DISK);
		goto out;
	}

	bus        = libhal_drive_get_bus (hal_drive);
	drive_type = libhal_drive_get_type (hal_drive);

	switch (drive_type) {
	case LIBHAL_DRIVE_TYPE_REMOVABLE_DISK:
	case LIBHAL_DRIVE_TYPE_DISK:
		name = _hal_lookup_icon (0x20000 + drive_type*0x100 + bus);
		break;

	default:
		name = _hal_lookup_icon (0x20000 + drive_type*0x100);
	}
out:
	if (name != NULL)
		return g_strdup (name);
	else {
		g_warning ("_hal_volume_policy_get_icon : error looking up icon; defaulting to mate-dev-harddisk");
		return g_strdup ("mate-dev-harddisk");
	}
}

/*------------------------------------------------------------------------*/
/* hal_volume may be NULL */
static char *
_hal_drive_policy_get_display_name (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				    LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	const char *model;
	const char *vendor;
	LibHalDriveType drive_type;
	char *name;
	char *vm_name;
	gboolean may_prepend_external;


	name = NULL;
	may_prepend_external = FALSE;

	drive_type = libhal_drive_get_type (hal_drive);

	/* Handle disks without removable media */
	if ((drive_type == LIBHAL_DRIVE_TYPE_DISK) &&
	    !libhal_drive_uses_removable_media (hal_drive) &&
	    hal_volume != NULL) {
		const char *label;
		const char *size_str;

		/* use label if available */
		label = libhal_volume_get_label (hal_volume);
		if (label != NULL && strlen (label) > 0) {
			name = g_strdup (label);
			goto out;
		}

		/* Otherwise, just use volume size */

		size_str = mate_vfs_format_file_size_for_display (libhal_volume_get_size (hal_volume));
		if (size_str != NULL) {
			name = g_strdup_printf (_("%s Volume"), size_str);
		}

		goto out;
	}

	/* removable media and special drives */

	/* drives we know the type of */
	if (drive_type == LIBHAL_DRIVE_TYPE_CDROM) {
		const char *first;
		const char *second;
		LibHalDriveCdromCaps drive_cdrom_caps;

		drive_cdrom_caps = libhal_drive_get_cdrom_caps (hal_drive);

		first = _("CD-ROM");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_CDR)
			first = _("CD-R");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_CDRW)
			first = _("CD-RW");

		second = NULL;
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDROM)
			second = _("DVD-ROM");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDPLUSR)
			second = _("DVD+R");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDPLUSRW)
			second = _("DVD+RW");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDR)
			second = _("DVD-R");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDRW)
			second = _("DVD-RW");
		if (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDRAM)
			second = _("DVD-RAM");
		if ((drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDR) &&
		    (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDPLUSR))
			second = _("DVD±R");
		if ((drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDRW) &&
		    (drive_cdrom_caps & LIBHAL_DRIVE_CDROM_CAPS_DVDPLUSRW))
			second = _("DVD±RW");

		if (second != NULL) {
			name = g_strdup_printf (_("%s/%s Drive"), first, second);
		} else {
			name = g_strdup_printf (_("%s Drive"), first);
		}

		may_prepend_external = TRUE;
	} else if (drive_type == LIBHAL_DRIVE_TYPE_FLOPPY) {
		name = g_strdup (_("Floppy Drive"));
		may_prepend_external = TRUE;
	} else if (drive_type == LIBHAL_DRIVE_TYPE_COMPACT_FLASH) {
		name = g_strdup (_("Compact Flash Drive"));
	} else if (drive_type == LIBHAL_DRIVE_TYPE_MEMORY_STICK) {
		name = g_strdup (_("Memory Stick Drive"));
	} else if (drive_type == LIBHAL_DRIVE_TYPE_SMART_MEDIA) {
		name = g_strdup (_("Smart Media Drive"));
	} else if (drive_type == LIBHAL_DRIVE_TYPE_SD_MMC) {
		name = g_strdup (_("SD/MMC Drive"));
	} else if (drive_type == LIBHAL_DRIVE_TYPE_ZIP) {
		name = g_strdup (_("Zip Drive"));
		may_prepend_external = TRUE;
	} else if (drive_type == LIBHAL_DRIVE_TYPE_JAZ) {
		name = g_strdup (_("Jaz Drive"));
		may_prepend_external = TRUE;
	} else if (drive_type == LIBHAL_DRIVE_TYPE_FLASHKEY) {
		name = g_strdup (_("Pen Drive"));
	} else if (drive_type == LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER) {
		const char *vendor;
		const char *model;
		vendor = libhal_drive_get_vendor (hal_drive);
		model = libhal_drive_get_model (hal_drive);
		name = g_strdup_printf (_("%s %s Music Player"),
					vendor != NULL ? vendor : "",
					model != NULL ? model : "");
	} else if (drive_type == LIBHAL_DRIVE_TYPE_CAMERA) {
		const char *vendor;
		const char *model;
		vendor = libhal_drive_get_vendor (hal_drive);
		model = libhal_drive_get_model (hal_drive);
		name = g_strdup_printf (_("%s %s Digital Camera"),
					vendor != NULL ? vendor : "",
					model != NULL ? model : "");
	}

	if (name != NULL)
		goto out;

	/* model and vendor at last resort */
	model = libhal_drive_get_model (hal_drive);
	vendor = libhal_drive_get_vendor (hal_drive);
	vm_name = NULL;
	if (vendor == NULL || strlen (vendor) == 0) {
		if (model != NULL && strlen (model) > 0)
			vm_name = g_strdup (model);
	} else {
		if (model == NULL || strlen (model) == 0)
			vm_name = g_strdup (vendor);
		else {
			vm_name = g_strdup_printf ("%s %s", vendor, model);
		}
	}

	if (vm_name != NULL) {
		name = vm_name;
	}

out:
	/* lame fallback */
	if (name == NULL)
		name = g_strdup (_("Drive"));

	if (may_prepend_external) {
		if (libhal_drive_is_hotpluggable (hal_drive)) {
			char *tmp = name;
			name = g_strdup_printf (_("External %s"), name);
			g_free (tmp);
		}
	}

	return name;
}

static char *
_hal_volume_policy_get_display_name (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				     LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	LibHalDriveType drive_type;
	const char *volume_label;
	char *name;
	char *size_str;

	name = NULL;

	drive_type = libhal_drive_get_type (hal_drive);
	volume_label = libhal_volume_get_label (hal_volume);

	/* Use volume label if available */
	if (volume_label != NULL) {
		name = g_strdup (volume_label);
		goto out;
	}

	/* Handle media in optical drives */
	if (drive_type == LIBHAL_DRIVE_TYPE_CDROM) {
		switch (libhal_volume_get_disc_type (hal_volume)) {

		default:
			/* explict fallthrough */

		case LIBHAL_VOLUME_DISC_TYPE_CDROM:
			name = g_strdup (_("CD-ROM Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_CDR:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank CD-R Disc"));
			else
				name = g_strdup (_("CD-R Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_CDRW:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank CD-RW Disc"));
			else
				name = g_strdup (_("CD-RW Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDROM:
			name = g_strdup (_("DVD-ROM Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDRAM:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank DVD-RAM Disc"));
			else
				name = g_strdup (_("DVD-RAM Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDR:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank DVD-R Disc"));
			else
				name = g_strdup (_("DVD-R Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDRW:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank DVD-RW Disc"));
			else
				name = g_strdup (_("DVD-RW Disc"));

			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDPLUSR:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank DVD+R Disc"));
			else
				name = g_strdup (_("DVD+R Disc"));
			break;

		case LIBHAL_VOLUME_DISC_TYPE_DVDPLUSRW:
			if (libhal_volume_disc_is_blank (hal_volume))
				name = g_strdup (_("Blank DVD+RW Disc"));
			else
				name = g_strdup (_("DVD+RW Disc"));
			break;
		}

		/* Special case for pure audio disc */
		if (libhal_volume_disc_has_audio (hal_volume) && !libhal_volume_disc_has_data (hal_volume)) {
			free (name);
			name = g_strdup (_("Audio Disc"));
		}

		goto out;
	}

	/* Fallback: size of media */

	size_str = mate_vfs_format_file_size_for_display (libhal_volume_get_size (hal_volume));
	if (size_str != NULL) {
		if (libhal_drive_uses_removable_media (hal_drive)) {
			name = g_strdup_printf (_("%s Removable Volume"), size_str);
		} else {
			name = g_strdup_printf (_("%s Volume"), size_str);
		}
		g_free (size_str);
	}


out:
	/* lame fallback */
	if (name == NULL)
		name = g_strdup (_("Volume"));

	return name;
}

/*------------------------------------------------------------------------*/

/* hal_volume may be NULL */
static gboolean
_hal_drive_policy_check (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
			 LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	gboolean ret;
	MateVFSHalUserData *hal_userdata;

	ret = FALSE;

	g_return_val_if_fail (volume_monitor_daemon != NULL, ret);

	hal_userdata = (MateVFSHalUserData *) libhal_ctx_get_user_data (volume_monitor_daemon->hal_ctx);

	g_return_val_if_fail (hal_userdata != NULL, ret);

	ret = TRUE;

	return ret;
}

static gboolean
_hal_volume_policy_check (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
			  LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
#if 0
	const char *label;
	const char *fstype;
#endif
	gboolean ret;
	const char *fhs23_toplevel_mount_points[] = {
		"/",
		"/bin",
		"/boot",
		"/dev",
		"/etc",
		"/home",
		"/lib",
		"/lib64",
		"/media",
		"/mnt",
		"/opt",
		"/root",
		"/sbin",
		"/srv",
		"/tmp",
		"/usr",
		"/var",
		"/proc",
		"/sbin",
		NULL
	};

	ret = FALSE;

	/* need to pass drive checks before considering volumes */
	if (!_hal_drive_policy_check (volume_monitor_daemon, hal_drive, hal_volume))
		goto out;

	/* needs to be a mountable filesystem OR contain crypto bits OR audio disc OR blank disc */
	if (! ((libhal_volume_get_fsusage (hal_volume) == LIBHAL_VOLUME_USAGE_MOUNTABLE_FILESYSTEM) ||
	       (libhal_volume_get_fsusage (hal_volume) == LIBHAL_VOLUME_USAGE_CRYPTO) ||
	       libhal_volume_disc_has_audio (hal_volume) ||
	       libhal_volume_disc_is_blank (hal_volume)))
		goto out;

	/* if we contain crypto bits, only show if our cleartext volume is not yet setup */
	if (libhal_volume_get_fsusage (hal_volume) == LIBHAL_VOLUME_USAGE_CRYPTO) {
		char *clear_udi;

		clear_udi = libhal_volume_crypto_get_clear_volume_udi (volume_monitor_daemon->hal_ctx, hal_volume);
		if (clear_udi != NULL) {
			free (clear_udi);
			goto out;
		}
	}


	/* for volumes the vendor and/or sysadmin wants to be ignore (e.g. bootstrap HFS
	 * partitions on the Mac, HP_RECOVERY partitions on HP systems etc.)
	 */
	if (libhal_volume_should_ignore (hal_volume))
		goto out;

	/* if mounted; discard if it got a FHS-2.3 name (to get /, /boot, /usr etc. out of the way)
	 *
	 * (yes, this breaks if the user mounts it later but that is not normally the case for such volumes)
	 */
	if (libhal_volume_is_mounted (hal_volume)) {
		int i;
		const char *mount_point;

		mount_point = libhal_volume_get_mount_point (hal_volume);
		/* blacklist fhs2.3 top level mount points */
		if (mount_point != NULL) {
			for (i = 0; fhs23_toplevel_mount_points[i] != NULL; i++) {
				if (strcmp (mount_point, fhs23_toplevel_mount_points[i]) == 0)
					goto out;
			}
		}
	}
#if 0
	label = libhal_volume_get_label (hal_volume);
	fstype = libhal_volume_get_fstype (hal_volume);

	/* blacklist partitions with name 'bootstrap' of type HFS (Apple uses that) */
	if (label != NULL && fstype != NULL && strcmp (label, "bootstrap") == 0 && strcmp (fstype, "hfs") == 0)
		goto out;
#endif
	ret = TRUE;
out:
	return ret;
}

static gboolean
_hal_volume_temp_udi (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
                          LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
        const char *volume_udi;
        gboolean ret;

        ret = FALSE;

        volume_udi = libhal_volume_get_udi (hal_volume);

        if (strncmp (volume_udi, "/org/freedesktop/Hal/devices/temp",
                     strlen ("/org/freedesktop/Hal/devices/temp")) == 0)
                ret = TRUE;

        return ret;
}

static gboolean
_hal_volume_policy_show_on_desktop (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				    LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	gboolean ret;

	ret = TRUE;

	/* Right now we show everything on the desktop as there is no setting
	 * for this.. potentially we could hide fixed drives..
	 */

	return ret;
}

/*------------------------------------------------------------------------*/

static int
_hal_get_mate_vfs_device_type (LibHalDrive *hal_drive)
{
	int result;

	/* fallthroughs are explicit */
	switch (libhal_drive_get_type (hal_drive)) {
        case LIBHAL_DRIVE_TYPE_CDROM:
		result = MATE_VFS_DEVICE_TYPE_CDROM;
		break;
        case LIBHAL_DRIVE_TYPE_FLOPPY:
		result = MATE_VFS_DEVICE_TYPE_FLOPPY;
		break;
        case LIBHAL_DRIVE_TYPE_ZIP:
		result = MATE_VFS_DEVICE_TYPE_ZIP;
		break;
        case LIBHAL_DRIVE_TYPE_JAZ:
		result = MATE_VFS_DEVICE_TYPE_JAZ;
		break;
        case LIBHAL_DRIVE_TYPE_CAMERA:
		result = MATE_VFS_DEVICE_TYPE_CAMERA;
		break;
        case LIBHAL_DRIVE_TYPE_COMPACT_FLASH:
        case LIBHAL_DRIVE_TYPE_SMART_MEDIA:
        case LIBHAL_DRIVE_TYPE_SD_MMC:
        case LIBHAL_DRIVE_TYPE_FLASHKEY:
        case LIBHAL_DRIVE_TYPE_MEMORY_STICK:
		result = MATE_VFS_DEVICE_TYPE_MEMORY_STICK;
		break;
        case LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER:
		result = MATE_VFS_DEVICE_TYPE_MUSIC_PLAYER;
		break;
	case LIBHAL_DRIVE_TYPE_TAPE:
        case LIBHAL_DRIVE_TYPE_REMOVABLE_DISK:
        case LIBHAL_DRIVE_TYPE_DISK:
	default:
		result = MATE_VFS_DEVICE_TYPE_HARDDRIVE;
		break;
	}

	return result;
}

static void
_hal_add_drive_without_volumes (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				LibHalDrive *hal_drive)
{
	MateVFSDrive *drive;
	MateVFSVolumeMonitor *volume_monitor;
	char *name;
	DBusError error;
	gboolean media_check_enabled;

	g_return_if_fail (hal_drive != NULL);

#ifdef HAL_SHOW_DEBUG
	g_debug ("entering _hal_add_drive_without_volumes for\n  drive udi '%s'\n",
		 libhal_drive_get_udi (hal_drive));
#endif

	volume_monitor = MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon);

	if (!_hal_drive_policy_check (volume_monitor_daemon, hal_drive, NULL)) {
		/* make sure to delete the drive/volume for policy changes */
		drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (
			volume_monitor, libhal_drive_get_udi (hal_drive));
		if (drive != NULL) {
#ifdef HAL_SHOW_DEBUG
		g_debug ("Removing MateVFSDrive for device path %s", drive->priv->device_path);
#endif
			_mate_vfs_volume_monitor_disconnected (volume_monitor, drive);
		}
		goto out;
	}

	/* don't add if it's already there */
	drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (volume_monitor, libhal_drive_get_udi (hal_drive));
	if (drive != NULL) {
		goto out;
	}

	/* doesn't make sense for devices without removable storage */
	if (!libhal_drive_uses_removable_media (hal_drive)) {
		goto out;
	}

 	dbus_error_init (&error);
 	media_check_enabled = libhal_device_get_property_bool (volume_monitor_daemon->hal_ctx,
 							       libhal_drive_get_udi (hal_drive),
 							       "storage.media_check_enabled",
 							       &error);
 	if (dbus_error_is_set (&error)) {
 		g_warning ("Error retrieving storage.media_check_enabled on '%s': Error: '%s' Message: '%s'",
 			   libhal_drive_get_udi (hal_drive), error.name, error.message);
 		dbus_error_free (&error);
 		media_check_enabled = FALSE;
 	}

	drive = g_object_new (MATE_VFS_TYPE_DRIVE, NULL);
	drive->priv->activation_uri = g_strdup ("");
	drive->priv->is_connected = 1;
	drive->priv->device_path = g_strdup (libhal_drive_get_device_file (hal_drive));
	drive->priv->device_type = _hal_get_mate_vfs_device_type (hal_drive);
	drive->priv->icon = _hal_drive_policy_get_icon (volume_monitor_daemon, hal_drive, NULL);
	name = _hal_drive_policy_get_display_name (volume_monitor_daemon, hal_drive, NULL);
	drive->priv->display_name = _mate_vfs_volume_monitor_uniquify_drive_name (volume_monitor, name);
	g_free (name);
	drive->priv->is_user_visible = !media_check_enabled; /* See http://bugzilla.gnome.org/show_bug.cgi?id=321320 */
	name = g_utf8_casefold (drive->priv->display_name, -1);
	drive->priv->display_name_key = g_utf8_collate_key (name, -1);
	g_free (name);
	drive->priv->volumes = NULL;
	drive->priv->hal_udi = g_strdup (libhal_drive_get_udi (hal_drive));
        drive->priv->must_eject_at_unmount = libhal_drive_requires_eject (hal_drive);

#ifdef HAL_SHOW_DEBUG
		g_debug ("Adding MateVFSDrive for device path %s", libhal_drive_get_device_file (hal_drive));
#endif

	_mate_vfs_volume_monitor_connected (volume_monitor, drive);
	mate_vfs_drive_unref (drive);

out:
	;
}

static gboolean
_hal_add_volume (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
		 LibHalDrive *hal_drive, LibHalVolume *hal_volume)
{
	gboolean ret;
	MateVFSVolume *vol;
	MateVFSDrive *drive;
	MateVFSVolumeMonitor *volume_monitor;
	char *name;
	gboolean allowed_by_policy;
	const char *backing_udi;

	g_return_val_if_fail (hal_drive != NULL, FALSE);
	g_return_val_if_fail (hal_volume != NULL, FALSE);

	ret = FALSE;
	backing_udi = NULL;

	volume_monitor = MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon);

	allowed_by_policy = _hal_volume_policy_check (volume_monitor_daemon, hal_drive, hal_volume);

#ifdef HAL_SHOW_DEBUG
	g_debug ("entering _hal_add_volume for\n"
		 "  drive udi '%s'\n"
		 "  volume udi '%s'\n"
		 "  allowd_by_policy %s",
		 libhal_drive_get_udi (hal_drive), libhal_volume_get_udi (hal_volume),
		 allowed_by_policy ? "yes" : "no");
#endif

	if (!allowed_by_policy) {
		/* make sure to completey delete any existing drive/volume for policy changes if the
		 * user_visible flag differs... */

		vol = _mate_vfs_volume_monitor_find_volume_by_hal_udi (
			volume_monitor, libhal_volume_get_udi (hal_volume));
		if (vol != NULL && vol->priv->is_user_visible) {
#ifdef HAL_SHOW_DEBUG
			g_debug ("Removing MateVFSVolume for device path %s", vol->priv->device_path);
#endif
			_mate_vfs_volume_monitor_unmounted (volume_monitor, vol);
			vol = NULL;
		}

		drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (
			volume_monitor, libhal_volume_get_udi (hal_volume));
		if (drive != NULL && drive->priv->is_user_visible) {

			if (vol != NULL) {
#ifdef HAL_SHOW_DEBUG
				g_debug ("Removing MateVFSVolume for device path %s", vol->priv->device_path);
#endif
				_mate_vfs_volume_monitor_unmounted (volume_monitor, vol);
				vol = NULL;
			}
#ifdef HAL_SHOW_DEBUG
			g_debug ("Removing MateVFSDrive for device path %s", drive->priv->device_path);
#endif
			_mate_vfs_volume_monitor_disconnected (volume_monitor, drive);
		}
	}

	if ( _hal_volume_temp_udi (volume_monitor_daemon, hal_drive, hal_volume))
		goto out;


	/* OK, check if we got a drive_without_volumes drive and delete that since we're going to add a
	 * drive for added partitions */
	drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (volume_monitor, libhal_drive_get_udi (hal_drive));
	if (drive != NULL) {
#ifdef HAL_SHOW_DEBUG
		g_debug ("Removing MateVFSDrive for device path %s", drive->priv->device_path);
#endif
		_mate_vfs_volume_monitor_disconnected (volume_monitor, drive);
	}

	if (!allowed_by_policy &&
	    libhal_volume_get_fsusage (hal_volume) != LIBHAL_VOLUME_USAGE_MOUNTABLE_FILESYSTEM)
		goto out;

	/* If we're stemming from a crypto volume... then remove the
	 * MateVFSDrive we added so users had a way to invoke
	 * mate-mount for asking for the pass-phrase...
	 */
	backing_udi = libhal_volume_crypto_get_backing_volume_udi (hal_volume);
	if (backing_udi != NULL) {
		MateVFSDrive *backing_drive;

		backing_drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (volume_monitor, backing_udi);
		if (backing_drive != NULL) {
#ifdef HAL_SHOW_DEBUG
			g_debug ("Removing MateVFSDrive for crypto device with path %s "
				 "(got cleartext device at path %s)",
				 backing_drive->priv->device_path,
				 libhal_volume_get_device_file (hal_volume));
#endif
			_mate_vfs_volume_monitor_disconnected (volume_monitor, backing_drive);
		}

	}

	/* if we had a drive from here but where we weren't mounted, just use that drive since nothing actually
	 * changed
	 */
	drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (volume_monitor, libhal_volume_get_udi (hal_volume));
	if (drive == NULL && allowed_by_policy) {
		drive = g_object_new (MATE_VFS_TYPE_DRIVE, NULL);
		if (libhal_volume_disc_has_audio (hal_volume)) {
			drive->priv->activation_uri = g_strdup_printf ("cdda://%s",
								       libhal_volume_get_device_file (hal_volume));
		} else if (libhal_volume_disc_is_blank (hal_volume)) {
			drive->priv->activation_uri = g_strdup ("burn:///");
		} else if (libhal_volume_is_mounted (hal_volume)) {
			drive->priv->activation_uri = mate_vfs_get_uri_from_local_path (
				libhal_volume_get_mount_point (hal_volume));
		} else {
			/* This sucks but it doesn't make sense to talk about the activation_uri if we're not mounted!
			 * So just set it to the empty string
			 */
			drive->priv->activation_uri = g_strdup ("");
		}
		drive->priv->is_connected = TRUE;
		drive->priv->device_path = g_strdup (libhal_volume_get_device_file (hal_volume));
		drive->priv->device_type = _hal_get_mate_vfs_device_type (hal_drive);

		/* TODO: could add an icon of a drive with media in it since this codepath only
		 * handles drives with media in them
		 */
		drive->priv->icon = _hal_drive_policy_get_icon (volume_monitor_daemon, hal_drive, NULL);
		name = _hal_drive_policy_get_display_name (volume_monitor_daemon, hal_drive, hal_volume);
		drive->priv->display_name = _mate_vfs_volume_monitor_uniquify_drive_name (volume_monitor, name);
		g_free (name);
		name = g_utf8_casefold (drive->priv->display_name, -1);
		drive->priv->display_name_key = g_utf8_collate_key (name, -1);
		g_free (name);
		drive->priv->is_user_visible = allowed_by_policy;
		drive->priv->volumes = NULL;
		drive->priv->hal_udi = g_strdup (libhal_volume_get_udi (hal_volume));
		drive->priv->hal_drive_udi = g_strdup (libhal_drive_get_udi (hal_drive));
		drive->priv->hal_backing_crypto_volume_udi = g_strdup (backing_udi);
                drive->priv->must_eject_at_unmount = libhal_drive_requires_eject (hal_drive);

#ifdef HAL_SHOW_DEBUG
		g_debug ("Adding MateVFSDrive for device path %s", libhal_volume_get_device_file (hal_volume));
#endif

		_mate_vfs_volume_monitor_connected (volume_monitor, drive);
		mate_vfs_drive_unref (drive);
	}

	vol = _mate_vfs_volume_monitor_find_volume_by_hal_udi (volume_monitor, libhal_volume_get_udi (hal_volume));
	if (vol == NULL &&
	    (libhal_volume_is_mounted (hal_volume) ||
	     libhal_volume_disc_has_audio (hal_volume) ||
	     libhal_volume_disc_is_blank (hal_volume))) {

		vol = g_object_new (MATE_VFS_TYPE_VOLUME, NULL);

		vol->priv->volume_type = MATE_VFS_VOLUME_TYPE_MOUNTPOINT;
		vol->priv->device_path = g_strdup (libhal_volume_get_device_file (hal_volume));
		vol->priv->unix_device = makedev (libhal_volume_get_device_major (hal_volume),
						  libhal_volume_get_device_minor (hal_volume));

		if (libhal_volume_disc_has_audio (hal_volume)) {
			vol->priv->volume_type = MATE_VFS_VOLUME_TYPE_VFS_MOUNT;
			vol->priv->activation_uri = g_strdup_printf ("cdda://%s",
								     libhal_volume_get_device_file (hal_volume));
		} else if (libhal_volume_disc_is_blank (hal_volume)) {
			vol->priv->volume_type = MATE_VFS_VOLUME_TYPE_VFS_MOUNT;
			vol->priv->activation_uri = g_strdup ("burn:///");
		} else {
			vol->priv->activation_uri = mate_vfs_get_uri_from_local_path (
				libhal_volume_get_mount_point (hal_volume));
		}
		vol->priv->filesystem_type = g_strdup (libhal_volume_get_fstype (hal_volume));
		vol->priv->is_read_only = libhal_device_get_property_bool (volume_monitor_daemon->hal_ctx,
									   libhal_volume_get_udi (hal_volume),
									   "volume.is_mounted_read_only", NULL);
		vol->priv->is_mounted = TRUE;

		vol->priv->device_type = _hal_get_mate_vfs_device_type (hal_drive);

		name = _hal_volume_policy_get_display_name (volume_monitor_daemon, hal_drive, hal_volume);
		vol->priv->display_name = _mate_vfs_volume_monitor_uniquify_volume_name (volume_monitor, name);
		g_free (name);
		name = g_utf8_casefold (vol->priv->display_name, -1);
		vol->priv->display_name_key = g_utf8_collate_key (name, -1);
		g_free (name);
		vol->priv->icon = _hal_volume_policy_get_icon (volume_monitor_daemon, hal_drive, hal_volume);
		vol->priv->is_user_visible = allowed_by_policy &&
			_hal_volume_policy_show_on_desktop (volume_monitor_daemon, hal_drive, hal_volume);
		vol->priv->hal_udi = g_strdup (libhal_volume_get_udi (hal_volume));
		vol->priv->hal_drive_udi = g_strdup (libhal_drive_get_udi (hal_drive));

		if (drive) {
			vol->priv->drive = drive;
			mate_vfs_drive_add_mounted_volume_private (drive, vol);
		}

#ifdef HAL_SHOW_DEBUG
		g_debug ("Adding MateVFSVolume for device path %s", libhal_volume_get_device_file (hal_volume));
#endif

		_mate_vfs_volume_monitor_mounted (volume_monitor, vol);
		mate_vfs_volume_unref (vol);
	}

	ret = TRUE;
out:

	return ret;
}

static void
_hal_update_all (MateVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	char **drives;
	int num_drives;
	DBusError error;

#ifdef HAL_SHOW_DEBUG
	g_debug ("entering _hal_update_all");
#endif

	dbus_error_init (&error);
	drives = libhal_find_device_by_capability (volume_monitor_daemon->hal_ctx,
						   "storage", &num_drives, &error);
	if (drives != NULL) {
		int i;

		for (i = 0; i < num_drives; i++) {
			LibHalDrive *drive;

#ifdef HAL_SHOW_DEBUG
			g_debug ("drive = '%s'", drives[i]);
#endif
			drive = libhal_drive_from_udi (volume_monitor_daemon->hal_ctx, drives[i]);
			if (drive != NULL) {
				char **volumes;
				int num_volumes;
				int num_volumes_added;

				num_volumes_added = 0;

				volumes = libhal_drive_find_all_volumes (volume_monitor_daemon->hal_ctx,
									 drive, &num_volumes);
				if (num_volumes > 0) {
					int j;

					for (j = 0; j < num_volumes; j++) {
						LibHalVolume *volume;

#ifdef HAL_SHOW_DEBUG
						g_debug ("  volume = '%s'", volumes[j]);
#endif
						volume = libhal_volume_from_udi (volume_monitor_daemon->hal_ctx,
										 volumes[j]);

						if (_hal_add_volume (volume_monitor_daemon, drive, volume))
							num_volumes_added++;

						libhal_volume_free (volume);

					}

					/* TODO: figure out why this crashes: libhal_free_string_array (volumes); */

				}

#ifdef HAL_SHOW_DEBUG
				g_debug ("  added %d volumes", num_volumes_added);
#endif

				if (num_volumes_added == 0) {
					/* if we didn't add any volumes show the drive_without_volumes drive */
					_hal_add_drive_without_volumes (volume_monitor_daemon, drive);
				}

				libhal_drive_free (drive);
			}

		}

		libhal_free_string_array (drives);
	}

#ifdef HAL_SHOW_DEBUG
	g_debug ("leaving _hal_update_all");
#endif

}


static void
_hal_device_added (LibHalContext *hal_ctx,
		   const char *udi)
{
	MateVFSHalUserData *hal_userdata;
	MateVFSVolumeMonitorDaemon *volume_monitor_daemon;

#ifdef HAL_SHOW_DEBUG
	g_debug ("Entering %s for udi %s", G_STRFUNC, udi);
#endif

	hal_userdata = (MateVFSHalUserData *) libhal_ctx_get_user_data (hal_ctx);
	volume_monitor_daemon = hal_userdata->volume_monitor_daemon;

	if (libhal_device_query_capability (hal_ctx, udi, "volume", NULL)) {
		char *drive_udi;
		LibHalDrive *drive;
		LibHalVolume *volume;
		DBusError error;

		drive = NULL;
		volume = NULL;

		dbus_error_init (&error);
		drive_udi = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error);
		if (dbus_error_is_set (&error)) {
			g_warning ("Error retrieving block.storage_device on '%s': Error: '%s' Message: '%s'",
				   udi, error.name, error.message);
			dbus_error_free (&error);
			goto vol_add_out;
		}

		drive = libhal_drive_from_udi (volume_monitor_daemon->hal_ctx, drive_udi);
		volume = libhal_volume_from_udi (volume_monitor_daemon->hal_ctx, udi);
		if (drive == NULL || volume == NULL)
			goto vol_add_out;

		_hal_add_volume (volume_monitor_daemon, drive, volume);

	vol_add_out:
		if (drive_udi != NULL)
			libhal_free_string (drive_udi);

		if (drive != NULL)
			libhal_drive_free (drive);

		if (volume != NULL)
			libhal_volume_free (volume);

	} else if (libhal_device_query_capability (hal_ctx, udi, "storage", NULL)) {
		char **devs;
		int num_dev;
		DBusError error;

		dbus_error_init (&error);

		/* don't want to add as drive if we got volumes */
		devs = libhal_manager_find_device_string_match (volume_monitor_daemon->hal_ctx,
								"block.storage_device", udi,
								&num_dev, &error);
		if (dbus_error_is_set (&error)) {
			g_warning ("Error retrieving finding devs on '%s': Error: '%s' Message: '%s'",
				   udi, error.name, error.message);
			dbus_error_free (&error);
			goto drive_add_out;
		}

		/* our own device object also got block.storage_device==udi so num_dev must be one */
		if (num_dev == 1) {
			LibHalDrive *drive;

			drive = libhal_drive_from_udi (volume_monitor_daemon->hal_ctx, udi);
			if (drive != NULL) {
				_hal_add_drive_without_volumes (volume_monitor_daemon, drive);
				libhal_drive_free (drive);
			}
		}

		libhal_free_string_array (devs);
	drive_add_out:
		;
	}

}

static void
_hal_device_removed (LibHalContext *hal_ctx, const char *udi)
{
	MateVFSDrive *drive;
	MateVFSVolume *volume;
	MateVFSHalUserData *hal_userdata;
	MateVFSVolumeMonitorDaemon *volume_monitor_daemon;
	char *hal_drive_udi;

#ifdef HAL_SHOW_DEBUG
	g_debug ("Entering %s for udi %s", G_STRFUNC, udi);
#endif

	hal_userdata = (MateVFSHalUserData *) libhal_ctx_get_user_data (hal_ctx);
	volume_monitor_daemon = hal_userdata->volume_monitor_daemon;

	volume = _mate_vfs_volume_monitor_find_volume_by_hal_udi (
		MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon), udi);

	drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (
		MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon), udi);

	hal_drive_udi = NULL;

	if (volume != NULL) {
#ifdef HAL_SHOW_DEBUG
		g_debug ("Removing MateVFSVolume for device path %s", volume->priv->device_path);
#endif
		_mate_vfs_volume_monitor_unmounted (MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon), volume);
	}

	if (drive != NULL) {
		char *backing_udi;

		if (hal_drive_udi == NULL)
			hal_drive_udi = g_strdup (drive->priv->hal_drive_udi);
#ifdef HAL_SHOW_DEBUG
		g_debug ("Removing MateVFSDrive for device path %s", drive->priv->device_path);
#endif

		if (drive->priv->hal_backing_crypto_volume_udi != NULL)
			backing_udi = g_strdup (drive->priv->hal_backing_crypto_volume_udi);
		else
			backing_udi = NULL;


		_mate_vfs_volume_monitor_disconnected (MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon), drive);

		if (backing_udi != NULL) {
			LibHalVolume *crypto_volume;

#ifdef HAL_SHOW_DEBUG
			g_debug ("Adding back MateVFSDrive for crypto volume");
#endif
			crypto_volume = libhal_volume_from_udi (volume_monitor_daemon->hal_ctx, backing_udi);
			if (crypto_volume != NULL) {
				LibHalDrive *crypto_drive;

				crypto_drive = libhal_drive_from_udi (
					volume_monitor_daemon->hal_ctx,
					libhal_volume_get_storage_device_udi (crypto_volume));
				if (crypto_drive != NULL) {
					_hal_add_volume (volume_monitor_daemon, crypto_drive, crypto_volume);
					libhal_drive_free (crypto_drive);
				}
				libhal_volume_free (crypto_volume);
			}

			g_free (backing_udi);
		}

	}

#ifdef HAL_SHOW_DEBUG
	g_debug ("hal_drive_udi = %s", hal_drive_udi);
#endif

	/* if there are no other drives with the same hal_drive_udi as us, add a drive_without_volumes object  */
	if (hal_drive_udi != NULL &&
	    _mate_vfs_volume_monitor_find_drive_by_hal_drive_udi (
		    MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon), hal_drive_udi) == NULL) {
		LibHalDrive *drive;

#ifdef HAL_SHOW_DEBUG
		g_debug ("going to add drive_without_volumes for %s", hal_drive_udi);
#endif

		drive = libhal_drive_from_udi (volume_monitor_daemon->hal_ctx, hal_drive_udi);
		if (drive != NULL) {
			_hal_add_drive_without_volumes (volume_monitor_daemon, drive);
			libhal_drive_free (drive);
		}
	}

	g_free (hal_drive_udi);
}

static void
_hal_device_property_modified (LibHalContext *hal_ctx,
			       const char *udi,
			       const char *key,
			       dbus_bool_t is_removed,
			       dbus_bool_t is_added)
{
	DBusError error;
	MateVFSHalUserData *hal_userdata;
	MateVFSVolumeMonitorDaemon *volume_monitor_daemon;
	MateVFSVolumeMonitor *volume_monitor;
	char *drive_udi;
	LibHalDrive *drive;
	LibHalVolume *volume;

	drive_udi = NULL;
	drive = NULL;
	volume = NULL;

	hal_userdata = (MateVFSHalUserData *) libhal_ctx_get_user_data (hal_ctx);
	volume_monitor_daemon = hal_userdata->volume_monitor_daemon;
	volume_monitor = MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon);


	if (!is_removed && g_ascii_strcasecmp (key, "volume.is_mounted") == 0) {
		gboolean is_mounted;
		gboolean is_audio_disc;
		gboolean is_blank_disc;

		dbus_error_init (&error);
		is_mounted = libhal_device_get_property_bool (hal_ctx, udi, "volume.is_mounted", &error);
		if (dbus_error_is_set (&error)) {
			g_warning ("Error retrieving volume.is_mounted on '%s': Error: '%s' Message: '%s'",
				   udi, error.name, error.message);
			dbus_error_free (&error);
			goto out;
		}

		is_blank_disc = libhal_device_get_property_bool (hal_ctx, udi, "volume.disc.is_blank", NULL);
		is_audio_disc = libhal_device_get_property_bool (hal_ctx, udi, "volume.disc.has_audio", NULL);

		if (is_mounted || is_blank_disc || is_audio_disc) {
			/* add new volume since it's now mounted */

			drive_udi = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error);
			if (dbus_error_is_set (&error)) {
				g_warning ("Error retrieving block.storage_device on '%s': Error: '%s' Message: '%s'",
					   udi, error.name, error.message);
				dbus_error_free (&error);
				goto out;
			}

			drive = libhal_drive_from_udi (volume_monitor_daemon->hal_ctx, drive_udi);
			volume = libhal_volume_from_udi (volume_monitor_daemon->hal_ctx, udi);
			if (drive == NULL || volume == NULL)
				goto out;

			_hal_add_volume (volume_monitor_daemon, drive, volume);
		} else {
			MateVFSVolume *vol;

			vol = _mate_vfs_volume_monitor_find_volume_by_hal_udi (volume_monitor, udi);
			if (vol != NULL) {
				/* remove volume since it's unmounted */
#ifdef HAL_SHOW_DEBUG
		g_debug ("Removing MateVFSVolume for device path %s", vol->priv->device_path);
#endif
				_mate_vfs_volume_monitor_unmounted (volume_monitor, vol);
			}
		}

	}
out:
	if (drive_udi != NULL)
		libhal_free_string (drive_udi);

	if (drive != NULL)
		libhal_drive_free (drive);

	if (volume != NULL)
		libhal_volume_free (volume);
}


gboolean
_mate_vfs_hal_mounts_init (MateVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	DBusError error;
	DBusConnection *dbus_connection;
	MateVFSHalUserData *hal_userdata;

#ifdef HAL_SHOW_DEBUG
	g_debug ("Entering %s", G_STRFUNC);
#endif

	/* Initialise the connection to the hal daemon */
	if ((volume_monitor_daemon->hal_ctx =
	     libhal_ctx_new ()) == NULL) {
		g_warning ("libhal_ctx_new failed\n");
		return FALSE;
	}

	dbus_error_init (&error);
	dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	if (dbus_error_is_set (&error)) {
		g_warning ("Error connecting to D-BUS system bus: %s",
			   error.message);
		dbus_error_free (&error);
		return FALSE;
	}
        dbus_connection_setup_with_g_main (dbus_connection, NULL);

	libhal_ctx_set_dbus_connection (volume_monitor_daemon->hal_ctx,
					dbus_connection);

	libhal_ctx_set_device_added (volume_monitor_daemon->hal_ctx,
				     _hal_device_added);
	libhal_ctx_set_device_removed (volume_monitor_daemon->hal_ctx,
		  		       _hal_device_removed);
	libhal_ctx_set_device_property_modified (volume_monitor_daemon->hal_ctx,
						 _hal_device_property_modified);

	if (!libhal_ctx_init (volume_monitor_daemon->hal_ctx, &error)) {
		g_warning ("libhal_ctx_init failed: %s\n", error.message);
		dbus_error_free (&error);
		return FALSE;
	}

	/* Tie some data with the libhal context */
	hal_userdata = g_new0 (MateVFSHalUserData, 1);
	hal_userdata->volume_monitor_daemon = volume_monitor_daemon;
	libhal_ctx_set_user_data (volume_monitor_daemon->hal_ctx,
				  hal_userdata);

	/* Simply watch all property changes instead of dynamically
	 * adding/removing match rules bus-side to only match certain
	 * objects...
	 */
	libhal_device_property_watch_all (volume_monitor_daemon->hal_ctx, &error);

	/* add drives/volumes from HAL */
	_hal_update_all (volume_monitor_daemon);

	return TRUE;
}

void
_mate_vfs_hal_mounts_shutdown (MateVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	DBusError error;

	dbus_error_init (&error);
	if (!libhal_ctx_shutdown (volume_monitor_daemon->hal_ctx, &error)) {
		g_warning ("hal_shutdown failed: %s\n", error.message);
		dbus_error_free (&error);
		return;
	}

	if (!libhal_ctx_free (volume_monitor_daemon->hal_ctx)) {
		g_warning ("hal_shutdown failed - unable to free hal context\n");
	}

}

void
_mate_vfs_hal_mounts_force_reprobe (MateVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
#ifdef HAL_SHOW_DEBUG
	g_debug ("entering _mate_vfs_hal_mounts_force_reprobe");
#endif
	_hal_update_all (volume_monitor_daemon);
}


/**************************************************************************/

MateVFSDrive *
_mate_vfs_hal_mounts_modify_drive (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				    MateVFSDrive *drive)
{
	MateVFSDrive *result;
	LibHalContext *hal_ctx;
	LibHalDrive *hal_drive;
	char path[PATH_MAX + 5] = "/dev/";
	char *target = path + 5;
	int ret;

	hal_drive = NULL;

	result = drive;

	if ((hal_ctx = volume_monitor_daemon->hal_ctx) == NULL)
		goto out;

	if (drive == NULL || drive->priv == NULL || drive->priv->device_path == NULL)
		goto out;

	/* Note, the device_path may point to what hal calls a volume, e.g.
	 * /dev/sda1 etc, however we get the Drive object for the parent if
	 * that is the case. This is a feature of libhal-storage.
	 */
	hal_drive = libhal_drive_from_device_file (hal_ctx, drive->priv->device_path);
	if (hal_drive != NULL) {
		/* ok, this device file is in HAL and thus managed by this backend */
		mate_vfs_drive_unref (drive);
		result = NULL;
		goto out;
	}

	/* No luck? Let's see if device_path is a symlink and check its target, too */
	ret = readlink (drive->priv->device_path, target, PATH_MAX - 1);
	if (ret < 0)
		goto out;
	target[ret] = '\0';
	/* Save some prepending and store the "/dev/" needed for relative links */
	if (target[0] != '/')
		target = path;
	hal_drive = libhal_drive_from_device_file (hal_ctx, target);
	if (hal_drive != NULL) {
		/* ok, this device file is in HAL and thus managed by this backend */
		mate_vfs_drive_unref (drive);
		result = NULL;
	}

out:
	libhal_drive_free (hal_drive);

	return result;
}

MateVFSVolume *
_mate_vfs_hal_mounts_modify_volume (MateVFSVolumeMonitorDaemon *volume_monitor_daemon,
				     MateVFSVolume *volume)
{
	MateVFSVolume *result;
	LibHalContext *hal_ctx;
	LibHalDrive *hal_drive;
	char path[PATH_MAX + 5] = "/dev/";
	char *target = path + 5;
	int ret;

	hal_drive = NULL;

	result = volume;

	if ((hal_ctx = volume_monitor_daemon->hal_ctx) == NULL)
		goto out;

	if (volume == NULL || volume->priv == NULL || volume->priv->device_path == NULL)
		goto out;

	/* Note, the device_path may point to what hal calls a volume, e.g.
	 * /dev/sda1 etc, however we get the Drive object for the parent if
	 * that is the case. This is a feature of libhal-storage.
	 */
	hal_drive = libhal_drive_from_device_file (hal_ctx, volume->priv->device_path);
	if (hal_drive != NULL) {

		/* handle drives that HAL can't poll and the user can still mount */
		if (libhal_device_get_property_bool (hal_ctx,
						     libhal_drive_get_udi (hal_drive),
						     "storage.media_check_enabled",
						     NULL) == FALSE) {
			MateVFSDrive *drive;

			if ((drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (
				     MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon),
				     libhal_drive_get_udi (hal_drive))) != NULL) {
				volume->priv->drive = drive;
				mate_vfs_drive_add_mounted_volume_private (drive, volume);

				goto out;
			}
		}

		/* ok, this device file is in HAL and thus managed by this backend */
		mate_vfs_volume_unref (volume);
		result = NULL;
		goto out;
	}

	/* No luck? Let's see if device_path is a symlink and check its target, too */
	ret = readlink (volume->priv->device_path, target, PATH_MAX - 1);
	if (ret < 0)
		goto out;
	target[ret] = '\0';
	/* Save some prepending and store the "/dev/" needed for relative links */
	if (target[0] != '/')
		target = path;
	hal_drive = libhal_drive_from_device_file (hal_ctx, target);
	if (hal_drive != NULL) {
		/* handle drives that HAL can't poll and the user can still mount */
		if (libhal_device_get_property_bool (hal_ctx,
						     libhal_drive_get_udi (hal_drive),
						     "storage.media_check_enabled",
						     NULL) == FALSE) {
			MateVFSDrive *drive;

			if ((drive = _mate_vfs_volume_monitor_find_drive_by_hal_udi (
				     MATE_VFS_VOLUME_MONITOR (volume_monitor_daemon),
				     libhal_drive_get_udi (hal_drive))) != NULL) {
				volume->priv->drive = drive;
				mate_vfs_drive_add_mounted_volume_private (drive, volume);

				goto out;
			}
		}

		/* ok, this device file is in HAL and thus managed by this backend */
		mate_vfs_volume_unref (volume);
		result = NULL;
	}

out:
	libhal_drive_free (hal_drive);

	return result;
}

#endif /* USE_HAL */
