/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-unix-mounts.h - read and monitor fstab/mtab

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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifndef HAVE_SYSCTLBYNAME
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <libmatevfs/mate-vfs.h>

#include "mate-vfs-unix-mounts.h"

#define MOUNT_POLL_INTERVAL 3000

#ifdef HAVE_SYS_MNTTAB_H
#define MNTOPT_RO	"ro"
#endif

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#elif defined (HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_VFSTAB_H
#include <sys/vfstab.h>
#endif

#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
#include <sys/mntctl.h>
#include <sys/vfs.h>
#include <sys/vmount.h>
#include <fshelp.h>
#endif

#if defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <fstab.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

#define STAT_TIMEOUT_SECONDS 3


/* Ideally this should not nonblocking stat, since that can block on
 * downed NFS mounts forever, however there seems to be no good way
 * to avoid this...
 * Various ugly workarounds can be tried later.
 */
GList *
_mate_vfs_unix_mount_get_unix_device (GList *mounts)
{
	GList *result = NULL;

	while (mounts) {
		dev_t unix_device = 0;
		pid_t pid;
		int pipes[2];
		int status;

		if (pipe (pipes) == -1)
			goto error;

		pid = fork ();
		if (pid == -1) {
			close (pipes[0]);
			close (pipes[1]);
			goto error;
		}

		if (pid == 0) {
			/* Child */
			close (pipes[0]);

			/* Fork an intermediate child that immediately exits
			 * so we can waitpid it. This means the final process
			 * will get owned by init and not go zombie.
			 */
			pid = fork ();

			if (pid == 0) {
				/* Grandchild */
				struct stat statbuf;
				while (mounts) {
					MateVFSUnixMount *mount = mounts->data;
					unix_device =
						stat (mount->mount_path, &statbuf) == 0
						? statbuf.st_dev
						: 0;
					write (pipes[1], (char *)&unix_device, sizeof (dev_t));
					mounts = mounts->next;
				}
			}
			close (pipes[1]);
			_exit (0);
			g_assert_not_reached ();
		}

		/* Parent */
		close (pipes[1]);

	retry_waitpid:
		if (waitpid (pid, &status, 0) < 0) {
			if (errno == EINTR)
				goto retry_waitpid;
			else if (errno == ECHILD)
				; /* do nothing, child already reaped */
			else
				g_warning ("waitpid() should not fail in mate_vfs_unix_mount_get_unix_device");
		}

		while (mounts) {
			int res;

			do {
#ifdef HAVE_POLL
				struct pollfd poll_fd;
				poll_fd.fd = pipes[0];
				poll_fd.events = POLLIN;
				res = poll (&poll_fd, 1, STAT_TIMEOUT_SECONDS * 1000);
#else
				struct timeval tv;
				fd_set read_fds;

				tv.tv_sec = STAT_TIMEOUT_SECONDS;
				tv.tv_usec = 0;
			
				FD_ZERO(&read_fds);
				FD_SET(pipes[0], &read_fds);
			
				res = select (pipes[0] + 1,
					      &read_fds, NULL, NULL, &tv);
#endif
			} while (res == -1 && errno == EINTR);

			if (res <= 0 ||
			    read (pipes[0], (char *)&unix_device, sizeof (dev_t)) != sizeof (dev_t))
				break;

			result = g_list_prepend (result, GUINT_TO_POINTER ((gulong)unix_device));
			mounts = mounts->next;
		}

		close (pipes[0]);

	error:
		if (mounts) {
			unix_device = 0;
			result = g_list_prepend (result, GUINT_TO_POINTER ((gulong)unix_device));
			mounts = mounts->next;
		}
	}

	return g_list_reverse (result);
}

#ifndef HAVE_SETMNTENT
#define setmntent(f,m) fopen(f,m)
#endif
#ifndef HAVE_ENDMNTENT
#define endmntent(f) fclose(f)
#endif

#ifdef HAVE_MNTENT_H

static char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
# ifdef __linux__
	return "/proc/mounts";
# else
	return _PATH_MOUNTED;
# endif
#else	
	return "/etc/mtab";
#endif
}

static char *
get_mtab_monitor_file (void)
{
#ifdef _PATH_MOUNTED
	return _PATH_MOUNTED;
#else	
	return "/etc/mtab";
#endif
}

gboolean
_mate_vfs_get_current_unix_mounts (GList **return_list)
{
	static time_t last_mtime = 0;
	static off_t last_size = 0;
	struct mntent *mntent;
	FILE *file;
	char *read_file;
	char *stat_file;
	struct stat sb;
	MateVFSUnixMount *mount_entry;
	GHashTable *mounts_hash;
	
	read_file = get_mtab_read_file ();
	stat_file = get_mtab_monitor_file ();

	*return_list = NULL;

	if (stat (stat_file, &sb) < 0) {
		g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}

	if (sb.st_mtime == last_mtime && sb.st_size == last_size) {
		return FALSE;
	}

	last_mtime = sb.st_mtime;
	last_size = sb.st_size;

	file = setmntent (read_file, "r");
	if (file == NULL) {
		return TRUE;
	}

	mounts_hash = g_hash_table_new (g_str_hash, g_str_equal);
	
	while ((mntent = getmntent (file)) != NULL) {
		/* ignore any mnt_fsname that is repeated and begins with a '/'
		 *
		 * We do this to avoid being fooled by --bind mounts, since
		 * these have the same device as the location they bind to.
		 * Its not an ideal solution to the problem, but its likely that
		 * the most important mountpoint is first and the --bind ones after
		 * that aren't as important. So it should work.
		 *
		 * The '/' is to handle procfs, tmpfs and other no device mounts.
		 */
		if (mntent->mnt_fsname != NULL &&
		    mntent->mnt_fsname[0] == '/' &&
		    g_hash_table_lookup (mounts_hash, mntent->mnt_fsname)) {
			continue;
		}

		mount_entry = g_new0 (MateVFSUnixMount, 1);

		mount_entry->mount_path = g_strdup (mntent->mnt_dir);
		mount_entry->device_path = g_strdup (mntent->mnt_fsname);
		mount_entry->filesystem_type = g_strdup (mntent->mnt_type);
		
		g_hash_table_insert (mounts_hash,
				     mount_entry->device_path,
				     mount_entry->device_path);

#if defined (HAVE_HASMNTOPT)
		if (hasmntopt (mntent, MNTOPT_RO) != NULL) {
			mount_entry->is_read_only = TRUE;
		}
#endif
		*return_list = g_list_prepend (*return_list, mount_entry);
	}
	g_hash_table_destroy (mounts_hash);
	
	endmntent (file);
	
	*return_list = g_list_reverse (*return_list);
	
	return TRUE;
}

#elif defined (HAVE_SYS_MNTTAB_H)

static char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
	return _PATH_MOUNTED;
#else	
	return "/etc/mnttab";
#endif
}

static char *
get_mtab_monitor_file (void)
{
	return get_mtab_read_file ();
}

gboolean
_mate_vfs_get_current_unix_mounts (GList **return_list)
{
	static time_t last_mtime = 0;
	static off_t last_size = 0;
	struct mnttab mntent;
	FILE *file;
	char *read_file;
	char *stat_file;
	struct stat sb;
	MateVFSUnixMount *mount_entry;
	
	read_file = get_mtab_read_file ();
	stat_file = get_mtab_monitor_file ();

	*return_list = NULL;

	if (stat (stat_file, &sb) < 0) {
		g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}

	if (sb.st_mtime == last_mtime && sb.st_size == last_size) {
		return FALSE;
	}

	last_mtime = sb.st_mtime;
	last_size = sb.st_size;

	file = setmntent (read_file, "r");
	if (file == NULL) {
		return TRUE;
	}

	while (! getmntent (file, &mntent)) {
		mount_entry = g_new0 (MateVFSUnixMount, 1);

		mount_entry->mount_path = g_strdup (mntent.mnt_mountp);
		mount_entry->device_path = g_strdup (mntent.mnt_special);
		mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);

#if defined (HAVE_HASMNTOPT)
		if (hasmntopt (&mntent, MNTOPT_RO) != NULL) {
			mount_entry->is_read_only = TRUE;
		}
#endif
		
		*return_list = g_list_prepend (*return_list, mount_entry);
	}
	
	endmntent (file);

	*return_list = g_list_reverse (*return_list);

	return TRUE;
}

#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

static char *
get_mtab_monitor_file (void)
{
	return NULL;
}

gboolean
_mate_vfs_get_current_unix_mounts (GList **return_list)
{
	struct vfs_ent *fs_info;
	struct vmount *vmount_info;
	int vmount_number;
	unsigned int vmount_size;
	int current;

	*return_list = NULL;

	if (mntctl (MCTL_QUERY, sizeof (vmount_size), &vmount_size) != 0) {
		g_warning ("Unable to know the number of mounted volumes\n");

		return TRUE;
	}

	vmount_info = (struct vmount*)g_malloc (vmount_size);
	if (vmount_info == NULL) {
		g_warning ("Unable to allocate memory\n");
		return TRUE;
	}

	vmount_number = mntctl (MCTL_QUERY, vmount_size, vmount_info);

	if (vmount_info->vmt_revision != VMT_REVISION) {
		g_warning ("Bad vmount structure revision number, want %d, got %d\n", VMT_REVISION, vmount_info->vmt_revision);
	}

	if (vmount_number < 0) {
		g_warning ("Unable to recover mounted volumes information\n");

		g_free (vmount_info);
		return TRUE;
	}

	while (vmount_number > 0) {
		mount_entry = g_new0 (MateVFSUnixMount, 1);
		
		mount_entry->device_path = g_strdup (vmt2dataptr (vmount_info, VMT_OBJECT));
		mount_entry->mount_path = g_strdup (vmt2dataptr (vmount_info, VMT_STUB));
		/* is_removable = (vmount_info->vmt_flags & MNT_REMOVABLE) ? 1 : 0; */
		mount_entry->is_read_only = (vmount_info->vmt_flags & MNT_READONLY) ? 1 : 0;

		fs_info = getvfsbytype (vmount_info->vmt_gfstype);

		if (fs_info == NULL) {
			mount_entry->filesystem_type = g_strdup ("unknown");
		} else {
			mount_entry->filesystem_type = g_strdup (fs_info->vfsent_name);
		}

		*return_list = g_list_prepend (*return_list, mount_entry);
		
		vmount_info = (struct vmount *)( (char*)vmount_info 
						+ vmount_info->vmt_length);
		vmount_number--;
	}
	

	g_free (vmount_info);

	*return_list = g_list_reverse (*return_list);

        return TRUE;

}

#elif defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

static char *
get_mtab_monitor_file (void)
{
    	return NULL;
}

gboolean
_mate_vfs_get_current_unix_mounts (GList **return_list)
{
    	struct statfs *mntent = NULL;
	int num_mounts, i;
	MateVFSUnixMount *mount_entry;

	*return_list = NULL;

	/* Pass MNT_NOWAIT to avoid blocking trying to update NFS mounts. */
	if ((num_mounts = getmntinfo (&mntent, MNT_NOWAIT)) == 0) {
	    	return TRUE;
	}

	for (i = 0; i < num_mounts; i++) {
	    	mount_entry = g_new0 (MateVFSUnixMount, 1);

		mount_entry->mount_path = g_strdup (mntent[i].f_mntonname);
		mount_entry->device_path = g_strdup (mntent[i].f_mntfromname);
		mount_entry->filesystem_type = g_strdup (mntent[i].f_fstypename);
		if (mntent[i].f_flags & MNT_RDONLY) {
		    	mount_entry->is_read_only = TRUE;
		}

		*return_list = g_list_prepend (*return_list, mount_entry);
	}

	*return_list = g_list_reverse (*return_list);

	return TRUE;
}
#else
#error No _mate_vfs_get_current_unix_mounts() implementation for system
#endif


/* _mate_vfs_get_unix_mount_table():
 * read the fstab.
 * don't return swap and ignore mounts.
 */

static char *
get_fstab_file (void)
{
#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
	/* AIX */
	return "/etc/filesystems";
#elif defined(_PATH_MNTTAB)
	return _PATH_MNTTAB;
#elif defined(VFSTAB)
	return VFSTAB;
#else
	return "/etc/fstab";
#endif
}

#ifdef HAVE_MNTENT_H
gboolean
_mate_vfs_get_unix_mount_table (GList **return_list)
{
	static time_t last_mtime = 0;
	static off_t last_size = 0;
	struct mntent *mntent;
	FILE *file;
	char *read_file;
	char *stat_file;
	char *opt, *opt_end;
	struct stat sb;
	MateVFSUnixMountPoint *mount_entry;
	
	stat_file = read_file = get_fstab_file ();

	*return_list = NULL;

	if (stat (stat_file, &sb) < 0) {
		g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}

	if (sb.st_mtime == last_mtime && sb.st_size == last_size) {
		return FALSE;
	}

	last_mtime = sb.st_mtime;
	last_size = sb.st_size;

	file = setmntent (read_file, "r");
	if (file == NULL) {
		return TRUE;
	}

	while ((mntent = getmntent (file)) != NULL) {
		if ((strcmp (mntent->mnt_dir, "ignore") == 0) ||
		    (strcmp (mntent->mnt_dir, "swap") == 0)) {
			continue;
		}
		
		mount_entry = g_new0 (MateVFSUnixMountPoint, 1);

		mount_entry->mount_path = g_strdup (mntent->mnt_dir);
		mount_entry->device_path = g_strdup (mntent->mnt_fsname);
		mount_entry->filesystem_type = g_strdup (mntent->mnt_type);

#ifdef HAVE_HASMNTOPT
		if (hasmntopt (mntent, MNTOPT_RO) != NULL) {
			mount_entry->is_read_only = TRUE;
		}
		if (hasmntopt (mntent, "loop") != NULL) {
			mount_entry->is_loopback = TRUE;
		}
		if ((opt = hasmntopt (mntent, "dev=")) != NULL) {
			opt = opt + strlen("dev=");
			opt_end = strchr (opt, ',');
			if (opt_end) {
				mount_entry->dev_opt = g_strndup (opt, opt_end - opt);
			} else {
				mount_entry->dev_opt = g_strdup (opt);
			}
		}
#endif

		if ((mntent->mnt_type != NULL && strcmp ("supermount", mntent->mnt_type) == 0)
#ifdef HAVE_HASMNTOPT
		    || (hasmntopt (mntent, "user") != NULL
			&& hasmntopt (mntent, "user") != hasmntopt (mntent, "user_xattr"))
		    || hasmntopt (mntent, "pamconsole") != NULL
		    || hasmntopt (mntent, "users") != NULL
		    || hasmntopt (mntent, "owner") != NULL
#endif
		    ) {
			mount_entry->is_user_mountable = TRUE;
		}

		
		*return_list = g_list_prepend (*return_list, mount_entry);
	}
	
	endmntent (file);
	
	*return_list = g_list_reverse (*return_list);
	
	return TRUE;
}

#elif defined (HAVE_SYS_MNTTAB_H)

gboolean
_mate_vfs_get_unix_mount_table (GList **return_list)
{
	static time_t last_mtime = 0;
	static off_t last_size = 0;
	struct mnttab mntent;
	FILE *file;
	char *read_file;
	char *stat_file;
	struct stat sb;
	MateVFSUnixMountPoint *mount_entry;
	
	stat_file = read_file = get_fstab_file ();

	*return_list = NULL;

	if (stat (stat_file, &sb) < 0) {
		g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}

	if (sb.st_mtime == last_mtime && sb.st_size == last_size) {
		return FALSE;
	}

	last_mtime = sb.st_mtime;
	last_size = sb.st_size;

	file = setmntent (read_file, "r");
	if (file == NULL) {
		return TRUE;
	}

	while (! getmntent (file, &mntent)) {
		if ((strcmp (mntent.mnt_mountp, "ignore") == 0) ||
		    (strcmp (mntent.mnt_mountp, "swap") == 0)) {
			continue;
		}
		
		mount_entry = g_new0 (MateVFSUnixMountPoint, 1);

		mount_entry->mount_path = g_strdup (mntent.mnt_mountp);
		mount_entry->device_path = g_strdup (mntent.mnt_special);
		mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);


#ifdef HAVE_HASMNTOPT
		if (hasmntopt (&mntent, MNTOPT_RO) != NULL) {
			mount_entry->is_read_only = TRUE;
		}
		if (hasmntopt (&mntent, "lofs") != NULL) {
			mount_entry->is_loopback = TRUE;
		}
#endif

		if ((mntent.mnt_fstype != NULL)
#ifdef HAVE_HASMNTOPT
		    || (hasmntopt (&mntent, "user") != NULL
			&& hasmntopt (&mntent, "user") != hasmntopt (&mntent, "user_xattr"))
		    || hasmntopt (&mntent, "pamconsole") != NULL
		    || hasmntopt (&mntent, "users") != NULL
		    || hasmntopt (&mntent, "owner") != NULL
#endif
		    ) {
			mount_entry->is_user_mountable = TRUE;
		}

		
		*return_list = g_list_prepend (*return_list, mount_entry);
	}
	
	endmntent (file);
	
	*return_list = g_list_reverse (*return_list);
	
	return TRUE;
}
#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

/* functions to parse /etc/filesystems on aix */

/* read character, ignoring comments (begin with '*', end with '\n' */
static int
aix_fs_getc (FILE *fd)
{
	int c;

	while ((c = getc (fd)) == '*') {
		while (((c = getc (fd)) != '\n') && (c != EOF)) {
		}
	}
}

/* eat all continuous spaces in a file */
static int
aix_fs_ignorespace (FILE *fd)
{
	int c;
	
	while ((c = aix_fs_getc (fd)) != EOF) {
		if (!g_ascii_isspace (c)) {
			ungetc (c,fd);
			return c;
		}
	}

        return EOF;
}

/* read one word from file */
static int
aix_fs_getword (FILE *fd, char *word)
{
	int c;

	aix_fs_ignorespace (fd);

	while (((c = aix_fs_getc (fd)) != EOF) && !g_ascii_isspace (c))	{
		if (c == '"') {
			while (((c = aix_fs_getc (fd)) != EOF) && (c != '"')) {
				*word++ = c;
			}
		} else {
			*word++ = c;
		}
	}
	*word = 0;

	return c;
}

typedef struct {
        char mnt_mount[PATH_MAX];
        char mnt_special[PATH_MAX];
        char mnt_fstype[16];
        char mnt_options[128];
} AixMountTableEntry;

/* read mount points properties */
static int
aix_fs_get (FILE *fd, AixMountTableEntry *prop)
{
	static char word[PATH_MAX] = { 0 };
	char value[PATH_MAX];

	/* read stanza */

	if (word[0] == 0) {
		if (aix_fs_getword (fd, word) == EOF)
			return EOF;
	}

	word[strlen(word) - 1] = 0;
	strcpy (prop->mnt_mount, word);

	/* read attributes and value */

	while (aix_fs_getword (fd, word) != EOF) {
		/* test if is attribute or new stanza */

		if (word[strlen(word) - 1] == ':') {
			return 0;
		}

		/* read "=" */
		aix_fs_getword (fd, value);

		/* read value */
		aix_fs_getword (fd, value);

		if (strcmp (word, "dev") == 0) {
			strcpy (prop->mnt_special, value);
		} else if (strcmp (word, "vfs") == 0) {
			strcpy (prop->mnt_fstype, value);
		} else if (strcmp (word, "options") == 0) {
			strcpy(prop->mnt_options, value);
		}
	}

	return 0;
}

gboolean
_mate_vfs_get_unix_mount_table (GList **return_list)
{
	static time_t last_mtime = 0;
	static off_t last_size = 0;
	struct mntent *mntent;
	FILE *file;
	char *read_file;
	char *stat_file;
	struct stat sb;
	MateVFSUnixMountPoint *mount_entry;
	AixMountTableEntry mntent;
	
	stat_file = read_file = get_fstab_file ();

	*return_list = NULL;
	
	if (stat (stat_file, &sb) < 0) {
		g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}
		
	if (last_mtime != 0 && fsb.st_mtime == last_mtime && fsb.st_size == last_size) {
		return FALSE;
	}
	last_mtime = fsb.st_mtime;
	last_size = fsb.st_size;

	file = setmntent (read_file, "r");
	if (file == NULL) {
		return TRUE;
	}

	while (!aix_fs_get (file, &mntent)) {
		if (strcmp ("cdrfs", mntent.mnt_fstype) == 0) {
			mount_entry = g_new0 (MateVFSUnixMountPoint, 1);
			

			mount_entry->mount_path = g_strdup (mntent.mnt_mount);
			mount_entry->device_path = g_strdup (mntent.mnt_special);
			mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);
			mount_entry->is_read_only = TRUE;
			mount_entry->is_user_mountable = TRUE;
			
			*return_list = g_list_prepend (*return_list, mount_entry);
		}
	}
	
	endmntent (file);
	
	*return_list = g_list_reverse (*return_list);
	
	return TRUE;
}

#elif defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

gboolean
_mate_vfs_get_unix_mount_table (GList **return_list)
{
    	static time_t last_mtime = 0;
	static off_t last_size = 0;
    	struct fstab *fstab = NULL;
	char *stat_file;
	struct stat fsb;
	MateVFSUnixMountPoint *mount_entry;
#ifdef HAVE_SYS_SYSCTL_H
	int usermnt = 0;
	size_t len = sizeof(usermnt);
	struct stat sb;
#endif

	stat_file = get_fstab_file ();

	*return_list = NULL;
 
    	if (stat (stat_file, &fsb) < 0) {
	    	g_warning ("Unable to stat %s: %s", stat_file,
			   g_strerror (errno));
		return TRUE;
	}

	if (last_mtime != 0 && fsb.st_mtime == last_mtime && fsb.st_size == last_size) {
	    	return FALSE;
	}
	last_mtime = fsb.st_mtime;
	last_size = fsb.st_size;
 
	*return_list = NULL;

	if (!setfsent ()) {
	    	return TRUE;
	}

#ifdef HAVE_SYS_SYSCTL_H
#if defined(HAVE_SYSCTLBYNAME)
	sysctlbyname ("vfs.usermount", &usermnt, &len, NULL, 0);
#elif defined(CTL_VFS) && defined(VFS_USERMOUNT)
	{
		int mib[2];

		mib[0] = CTL_VFS;
		mib[1] = VFS_USERMOUNT;
		sysctl (mib, 2, &usermnt, &len, NULL, 0);
	}
#elif defined(CTL_KERN) && defined(KERN_USERMOUNT)
	{
		int mib[2];

		mib[0] = CTL_KERN;
		mib[1] = KERN_USERMOUNT;
		sysctl (mib, 2, &usermnt, &len, NULL, 0);
	}
#endif
#endif

	while ((fstab = getfsent ()) != NULL) {
	    	if (strcmp (fstab->fs_vfstype, "swap") == 0) {
		    	continue;
		}

		mount_entry = g_new0 (MateVFSUnixMountPoint, 1);

		mount_entry->mount_path = g_strdup (fstab->fs_file);
		mount_entry->device_path = g_strdup (fstab->fs_spec);
		mount_entry->filesystem_type = g_strdup (fstab->fs_vfstype);

		if (strcmp (fstab->fs_type, "ro") == 0) {
		    	mount_entry->is_read_only = TRUE;
		}

#ifdef HAVE_SYS_SYSCTL_H
		if (usermnt != 0) {
			uid_t uid = getuid ();
			if (stat (fstab->fs_file, &sb) == 0) {
				if (uid == 0 || sb.st_uid == uid) {
					mount_entry->is_user_mountable = TRUE;
				}
			}
		}
#endif

		*return_list = g_list_prepend (*return_list, mount_entry);
	}

	endfsent ();

	*return_list = g_list_reverse (*return_list);

	return TRUE;
}
#else
#error No _mate_vfs_get_mount_table() implementation for system
#endif
 

static MateVFSMonitorHandle *fstab_monitor = NULL;
static MateVFSUnixMountCallback fstab_callback = NULL;
static MateVFSMonitorHandle *mtab_monitor = NULL;
static MateVFSUnixMountCallback mtab_callback = NULL;
static guint mtab_poll_tag = 0;
static guint fstab_poll_tag = 0;

static void
fstab_monitor_callback (MateVFSMonitorHandle *handle,
			const char *monitor_uri,
			const char *info_uri,
			MateVFSMonitorEventType event_type,
			gpointer user_data)
{
	(*fstab_callback) (user_data);
}

static void
mtab_monitor_callback (MateVFSMonitorHandle *handle,
		       const char *monitor_uri,
		       const char *info_uri,
		       MateVFSMonitorEventType event_type,
		       gpointer user_data)
{
	(*mtab_callback) (user_data);
}

static gboolean
poll_fstab (gpointer user_data)
{
	(*fstab_callback) (user_data);
	return TRUE;
}

static gboolean
poll_mtab (gpointer user_data)
{
	(*mtab_callback) (user_data);
	return TRUE;
}

void
_mate_vfs_monitor_unix_mounts (MateVFSUnixMountCallback mount_table_changed,
				gpointer mount_table_changed_user_data,
				MateVFSUnixMountCallback current_mounts_changed,
				gpointer current_mounts_user_data)
{
	char *fstab_file;
	char *fstab_uri;
	char *mtab_file;
	char *mtab_uri;

	g_assert (fstab_monitor == NULL);
	g_assert (mtab_monitor == NULL);

	fstab_callback = mount_table_changed;
	mtab_callback = current_mounts_changed;
	
	fstab_file = get_fstab_file ();
	if (fstab_file != NULL) {
		fstab_uri = mate_vfs_get_uri_from_local_path (fstab_file);
		mate_vfs_monitor_add (&fstab_monitor,
				       fstab_uri,
				       MATE_VFS_MONITOR_FILE,
				       fstab_monitor_callback,
				       mount_table_changed_user_data);
		g_free (fstab_uri);
	}

	mtab_file = get_mtab_monitor_file ();
	if (mtab_file != NULL) {
		mtab_uri = mate_vfs_get_uri_from_local_path (mtab_file);
		mate_vfs_monitor_add (&mtab_monitor,
				       mtab_uri,
				       MATE_VFS_MONITOR_FILE,
				       mtab_monitor_callback,
				       current_mounts_user_data);
		g_free (mtab_uri);
	}

	if (fstab_monitor == NULL) {
		fstab_poll_tag = g_timeout_add (MOUNT_POLL_INTERVAL,
						poll_fstab,
						mount_table_changed_user_data);
	}
	if (mtab_monitor == NULL) {
		mtab_poll_tag = g_timeout_add (MOUNT_POLL_INTERVAL,
					       poll_mtab,
					       current_mounts_user_data);
	}	
}

void
_mate_vfs_stop_monitoring_unix_mounts (void)
{
	if (fstab_monitor != NULL) {
		mate_vfs_monitor_cancel (fstab_monitor);
		fstab_monitor = NULL;
	}
	if (mtab_monitor != NULL) {
		mate_vfs_monitor_cancel (mtab_monitor);
		mtab_monitor = NULL;
	}

	if (mtab_poll_tag != 0) {
		g_source_remove (mtab_poll_tag);
		mtab_poll_tag = 0;
	}
	if (fstab_poll_tag != 0) {
		g_source_remove (fstab_poll_tag);
		fstab_poll_tag = 0;
	}
}

void
_mate_vfs_unix_mount_free (MateVFSUnixMount *mount_entry)
{
	g_free (mount_entry->mount_path);
	g_free (mount_entry->device_path);
	g_free (mount_entry->filesystem_type);
	g_free (mount_entry);
}

void
_mate_vfs_unix_mount_point_free (MateVFSUnixMountPoint *mount_point)
{
	g_free (mount_point->mount_path);
	g_free (mount_point->device_path);
	g_free (mount_point->filesystem_type);
	g_free (mount_point->dev_opt);
	g_free (mount_point);
}


static int
strcmp_null (const char *str1,
	     const char *str2)
{
	if (str1 == str2) {
		return 0;
	}
	if (str1 == NULL && str2 != NULL) {
		return -1;
	}
	if (str1 != NULL && str2 == NULL) {
		return 1;
	}
	return strcmp (str1, str2);
}

gint
_mate_vfs_unix_mount_compare (MateVFSUnixMount      *mount1,
			       MateVFSUnixMount      *mount2)
{
	int res;

	res = strcmp_null (mount1->mount_path, mount2->mount_path);
	if (res != 0) {
		return res;
	}
	
	res = strcmp_null (mount1->device_path, mount2->device_path);
	if (res != 0) {
		return res;
	}
	
	res = strcmp_null (mount1->filesystem_type, mount2->filesystem_type);
	if (res != 0) {
		return res;
	}

	res =  mount1->is_read_only - mount2->is_read_only;
	if (res != 0) {
		return res;
	}

	return 0;
}

gint
_mate_vfs_unix_mount_point_compare (MateVFSUnixMountPoint *mount1,
				     MateVFSUnixMountPoint *mount2)
{
	int res;

	res = strcmp_null (mount1->mount_path, mount2->mount_path);
	if (res != 0) {
		return res;
	}
	
	res = strcmp_null (mount1->device_path, mount2->device_path);
	if (res != 0) {
		return res;
	}
	
	res = strcmp_null (mount1->filesystem_type, mount2->filesystem_type);
	if (res != 0) {
		return res;
	}

	res = strcmp_null (mount1->dev_opt, mount2->dev_opt);
	if (res != 0) {
		return res;
	}

	res =  mount1->is_read_only - mount2->is_read_only;
	if (res != 0) {
		return res;
	}

	res = mount1->is_user_mountable - mount2->is_user_mountable;
	if (res != 0) {
		return res;
	}

	res = mount1->is_loopback - mount2->is_loopback;
	if (res != 0) {
		return res;
	}

	
	return 0;
}
