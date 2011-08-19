/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-ops.c - mount/unmount/eject handling

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
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#ifndef G_OS_WIN32
#include <sys/wait.h>
#include <pthread.h>
#endif

#include <mateconf/mateconf-client.h>
#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-volume.h"
#include "mate-vfs-utils.h"
#include "mate-vfs-drive.h"

#include "mate-vfs-volume-monitor-daemon.h"
#include "mate-vfs-volume-monitor-client.h"

#include "mate-vfs-private.h"
#include "mate-vfs-standard-callbacks.h"
#include "mate-vfs-module-callback-module-api.h"
#include "mate-vfs-module-callback-private.h"
#include "mate-vfs-pty.h"

#ifdef USE_HAL
#include "mate-vfs-hal-mounts.h"
#endif

#ifdef HAVE_GRANTPT
/* We only use this on systems with unix98 ptys */
#define USE_PTY 1
#endif

#if 0
#define DEBUG_MOUNT_ENABLE
#endif

#ifdef DEBUG_MOUNT_ENABLE
#define DEBUG_MOUNT(x) g_print x
#else
#define DEBUG_MOUNT(x) 
#endif

#ifndef G_OS_WIN32

#ifdef USE_VOLRMMOUNT

static const char *volrmmount_locations [] = {
       "/usr/bin/volrmmount",
       NULL
};

#define MOUNT_COMMAND volrmmount_locations
#define MOUNT_SEPARATOR " -i "
#define UMOUNT_COMMAND volrmmount_locations
#define UMOUNT_SEPARATOR " -e "

#else

static const char *mount_known_locations [] = {
	"/sbin/mount", "/bin/mount",
	"/usr/sbin/mount", "/usr/bin/mount",
	NULL
};

static const char *pmount_known_locations [] = {
	"/usr/sbin/pmount", "/usr/bin/pmount",
	"/sbin/mount", "/bin/mount",
	"/usr/sbin/mount", "/usr/bin/mount",
	NULL
};

static const char *pumount_known_locations [] = {
	"/usr/sbin/pumount", "/usr/bin/pumount",
	"/sbin/umount", "/bin/umount",
	"/usr/sbin/umount", "/usr/bin/umount",
	NULL
};

static const char *umount_known_locations [] = {
	"/sbin/umount", "/bin/umount",
	"/usr/sbin/umount", "/usr/bin/umount",
	NULL
};


#define MOUNT_COMMAND mount_known_locations
#define MOUNT_SEPARATOR " "
#define PMOUNT_COMMAND pmount_known_locations
#define PMOUNT_SEPARATOR " "
#define UMOUNT_COMMAND umount_known_locations
#define UMOUNT_SEPARATOR " "
#define PUMOUNT_COMMAND pumount_known_locations
#define PUMOUNT_SEPARATOR " "

#endif /* USE_VOLRMMOUNT */

/* Returns the full path to the queried command */
static const char *
find_command (const char **known_locations)
{
	int i;

	for (i = 0; known_locations [i]; i++){
		if (g_file_test (known_locations [i], G_FILE_TEST_IS_EXECUTABLE))
			return known_locations [i];
	}
	return NULL;
}

typedef struct {
	char *argv[4];
	char *mount_point;
	char *device_path;
	char *hal_udi;
	MateVFSDeviceType device_type;
	gboolean should_mount;
	gboolean should_unmount;
	gboolean should_eject;
	MateVFSVolumeOpCallback callback;
	gpointer user_data;

	/* Result: */
	gboolean succeeded;
	char *error_message;
	char *detailed_error_message;
} MountThreadInfo;

/* Since we're not a module handling jobs, we need to do our own marshalling
   of the authentication callbacks */
typedef struct _MountThreadAuth {
	const gchar *callback;
	gconstpointer in_args;
	gsize in_size;
	gpointer out_args;
	gsize out_size;
	gboolean invoked;

	GCond *cond;
	GMutex *mutex;
} MountThreadAuth;


/* mate-mount programs display their own dialogs so no use for these functions */
static char *
generate_mount_error_message (char *standard_error,
			      MateVFSDeviceType device_type)
{
	char *message;
	
	if ((strstr (standard_error, "is not a valid block device") != NULL) ||
	    (strstr (standard_error, "No medium found") != NULL)) {
		/* No media in drive */
		if (device_type == MATE_VFS_DEVICE_TYPE_FLOPPY) {
			/* Handle floppy case */
			message = g_strdup_printf (_("Unable to mount the floppy drive. "
						     "There is probably no floppy in the drive."));
		} else {
			/* All others */
			message = g_strdup_printf (_("Unable to mount the volume. "
						     "There is probably no media in the device."));
		}
	} else if (strstr (standard_error, "wrong fs type, bad option, bad superblock on") != NULL) {
		/* Unknown filesystem */
		if (device_type == MATE_VFS_DEVICE_TYPE_FLOPPY) {
			message = g_strdup_printf (_("Unable to mount the floppy drive. "
						     "The floppy is probably in a format that cannot be mounted."));
		} else if (device_type == MATE_VFS_DEVICE_TYPE_LOOPBACK) {
			/* Probably a wrong password */
			message = g_strdup_printf (_("Unable to mount the volume. "
						     "If this is an encrypted drive, then the wrong password or key was used."));
		} else {
			message = g_strdup_printf (_("Unable to mount the selected volume. "
						     "The volume is probably in a format that cannot be mounted."));
		}
	} else {
		if (device_type == MATE_VFS_DEVICE_TYPE_FLOPPY) {
			message = g_strdup (_("Unable to mount the selected floppy drive."));
		} else {
			message = g_strdup (_("Unable to mount the selected volume."));
		}
	}
	return message;
}

static char *
generate_unmount_error_message (char *standard_error,
				MateVFSDeviceType device_type)
{
	char *message;
	
	if ((strstr (standard_error, "busy") != NULL)) {
		message = g_strdup_printf (_("Unable to unmount the selected volume. "
					     "The volume is in use by one or more programs."));
	} else {
		message = g_strdup (_("Unable to unmount the selected volume."));
	}	

	return message;
}

static void
force_probe (void)
{
	MateVFSVolumeMonitor *volume_monitor;
	MateVFSDaemonForceProbeCallback callback;
	
	volume_monitor = mate_vfs_get_volume_monitor ();

	if (mate_vfs_get_is_daemon ()) {
		callback = _mate_vfs_get_daemon_force_probe_callback();
		(*callback) (MATE_VFS_VOLUME_MONITOR (volume_monitor));
	} else {
		_mate_vfs_volume_monitor_client_dbus_force_probe (
			MATE_VFS_VOLUME_MONITOR_CLIENT (volume_monitor));
	}
}

static gboolean
report_mount_result (gpointer callback_data)
{
	MountThreadInfo *info;
	int i;

	info = callback_data;

	/* We want to force probing here so that the daemon
	   can refresh and tell us (and everyone else) of the new
	   volume before we call the callback */
	force_probe ();
	
	(info->callback) (info->succeeded,
			  info->error_message,
			  info->detailed_error_message,
			  info->user_data);

	i = 0;
	while (info->argv[i] != NULL) {
		g_free (info->argv[i]);
		i++;
	}
	
	g_free (info->mount_point);
	g_free (info->device_path);
	g_free (info->hal_udi);
	g_free (info->error_message);
	g_free (info->detailed_error_message);
	g_free (info);

	return FALSE;
}

static gboolean
invoke_async_auth_cb (MountThreadAuth *auth)
{
	g_mutex_lock (auth->mutex);
	auth->invoked = mate_vfs_module_callback_invoke (auth->callback, 
					auth->in_args, auth->in_size, 
					auth->out_args, auth->out_size);
	g_cond_signal (auth->cond);
	g_mutex_unlock (auth->mutex);
	return FALSE;
}

static gboolean
invoke_async_auth (const gchar *callback_name, gconstpointer in,
		   gsize in_size, gpointer out, gsize out_size)
{
	MountThreadAuth auth;
	memset (&auth, 0, sizeof(auth));
	auth.callback = callback_name;
	auth.in_args = in;
	auth.in_size = in_size;
	auth.out_args = out;
	auth.out_size = out_size;
	auth.invoked = FALSE;
	auth.mutex = g_mutex_new ();
	auth.cond = g_cond_new ();

	DEBUG_MOUNT (("mount invoking auth callback: %s\n", callback_name));

	g_mutex_lock (auth.mutex);
	g_idle_add_full (G_PRIORITY_HIGH_IDLE, (GSourceFunc)invoke_async_auth_cb, &auth, NULL);
	g_cond_wait (auth.cond, auth.mutex);

	g_mutex_unlock (auth.mutex);
	g_mutex_free (auth.mutex);
	g_cond_free (auth.cond);

	DEBUG_MOUNT (("mount invoked auth callback: %s %d\n", callback_name, auth.invoked));
	
	return auth.invoked;
}

static gboolean
invoke_full_auth (MountThreadInfo *info, char **password_out)
{
	MateVFSModuleCallbackFullAuthenticationIn in_args;
	MateVFSModuleCallbackFullAuthenticationOut out_args;
	gboolean invoked;

	*password_out = NULL;

	memset (&in_args, 0, sizeof (in_args));
	in_args.flags = MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD;
	in_args.uri = mate_vfs_get_uri_from_local_path (info->mount_point);
	in_args.protocol = "file";
	in_args.object = NULL;
	in_args.authtype = "password";
	in_args.domain = NULL;
	in_args.port = 0;
	in_args.server = (gchar*)info->device_path;
	in_args.username = NULL;

	memset (&out_args, 0, sizeof (out_args));

	invoked = invoke_async_auth (MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
				     &in_args, sizeof (in_args), &out_args, sizeof (out_args));

	if (invoked && !out_args.abort_auth) {
		*password_out = out_args.password;
		out_args.password = NULL;
	}

	g_free (in_args.uri);
	g_free (out_args.username);
	g_free (out_args.password);
	g_free (out_args.keyring);
	g_free (out_args.domain);

	return invoked && !out_args.abort_auth;
}

static gint
read_data (GString *str, gint *fd, GError **error)
{
	gssize bytes;
	gchar buf[4096];

again:
	bytes = read (*fd, buf, 4096);
	if (bytes < 0) {
		if (errno == EINTR)
			goto again;
		else if (errno == EIO) /* This seems to occur on a tty */
			bytes = 0;
		else {
			g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ,
			             _("Failed to read data from child process %d (%s)"), 
			             *fd, g_strerror (errno));
			return -1;
		}
	}

	if (str)
		str = g_string_append_len (str, buf, bytes);
	/* Close when no more data */
	if (bytes == 0) {
		g_printerr("closing\n");
		close (*fd);
		*fd = -1;
	}
	
	return bytes;
}

static gboolean
spawn_mount (MountThreadInfo *info, gchar **envp, gchar **standard_error, 
	     gint *exit_status, GError **error)
{
	gint tty_fd, in_fd, out_fd, err_fd;
	fd_set rfds, wfds;
	GPid pid;
	gint ret, status;
	GString *outstr = NULL;
	GString *errstr = NULL;
	gboolean failed = FALSE;
	gboolean with_password = FALSE;
	gboolean need_password = FALSE;
	gchar* password = NULL;
	
	tty_fd = in_fd = out_fd = err_fd = -1;

#ifdef USE_PTY
	with_password = info->should_mount;
	
	if (with_password) {
		tty_fd = mate_vfs_pty_open ((pid_t *) &pid, MATE_VFS_PTY_LOGIN_TTY, envp, 
					     info->argv[0], info->argv, 
					     NULL, 300, 300, &in_fd, &out_fd, 
					     standard_error ? &err_fd : NULL);
		if (tty_fd == -1) {
			g_warning (_("Couldn't run mount process in a pty"));
			with_password = FALSE;
		}
	} 
#endif
	
	if (!with_password) {
		if (!g_spawn_async_with_pipes (NULL, info->argv, envp, 
					       G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDOUT_TO_DEV_NULL, 
					       NULL, NULL, &pid, NULL, NULL, 
					       standard_error ? &err_fd : NULL, error)) {
			g_critical ("Could not launch mount command: %s", (*error)->message);
			return FALSE;
		}
	}
	
	outstr = g_string_new (NULL);
	if (standard_error)
		errstr = g_string_new (NULL);

	/* Read data until we get EOF on both pipes. */
	while (!failed && (err_fd >= 0 || tty_fd >= 0)) {
		ret = 0;

		FD_ZERO (&rfds);
		FD_ZERO (&wfds);
		
		if (out_fd >= 0)
			FD_SET (out_fd, &rfds);
		if (tty_fd >= 0)
			FD_SET (tty_fd, &rfds);
		if (err_fd >= 0)
			FD_SET (err_fd, &rfds);
		
		if (need_password) {
			if (in_fd >= 0)
				FD_SET (in_fd, &wfds);
			if (tty_fd >= 0)
				FD_SET (tty_fd, &wfds);
		}
          
		ret = select (FD_SETSIZE, &rfds, &wfds, NULL, NULL /* no timeout */);
		if (ret < 0 && errno != EINTR) {
			failed = TRUE;
			g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ, 
				     _("Unexpected error in select() reading data from a child process (%s)"),
				     g_strerror (errno));
			break;
		}
		
		/* Read possible password prompt */
		if (tty_fd >= 0 && FD_ISSET (tty_fd, &rfds)) {
			if (read_data (outstr, &tty_fd, error) == -1) {
				failed = TRUE;
				break;
			}
		}
		
		/* Read possible password prompt */
		if (out_fd >= 0 && FD_ISSET (out_fd, &rfds)) {
			if (read_data(outstr, &out_fd, error) == -1) {
				failed = TRUE;
				break;
			}
		}

		/* Look for a password prompt */
		g_string_ascii_down (outstr);
		if (g_strstr_len (outstr->str, outstr->len, "password")) {
			DEBUG_MOUNT (("mount needs a password\n"));
			g_string_erase (outstr, 0, -1);
			need_password = TRUE;
		}
		
		if (need_password && ((tty_fd >= 0 && FD_ISSET (tty_fd, &wfds)) ||
				      (in_fd >= 0 && FD_ISSET (in_fd, &wfds)))) {

			g_free (password);
			password = NULL;

			/* Prompt for a password */
			if (!invoke_full_auth (info, &password) || password == NULL) {
				
				DEBUG_MOUNT (("user cancelled mount password prompt\n"));
				
				/* User cancelled, empty string returned */
				g_string_erase (errstr, 0, -1);
				kill (pid, SIGTERM);
				break;
				
			/* Got a password */
			} else {
				
				DEBUG_MOUNT (("got password: %s\n", password));
				
				if (write(tty_fd, password, strlen(password)) == -1 || 
				    write(tty_fd, "\n", 1) == -1) {
					g_warning ("couldn't send password to child mount process: %s\n", g_strerror (errno));
					g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ, 
						     _("Couldn't send password to mount process."));
					failed = TRUE;
				}
				
			} 

			need_password = FALSE;
		}
	
		if (err_fd >= 0 && FD_ISSET (err_fd, &rfds)) {
			if (read_data (errstr, &err_fd, error) == -1) {
				failed = TRUE;
				break;
			}
		}
	}

	if (in_fd >= 0)
		close (in_fd);
	if (out_fd >= 0)
		close (out_fd);
	if (err_fd >= 0)
		close (err_fd);
	if (tty_fd >= 0)
		close (tty_fd);

	/* Wait for child to exit, even if we have
	 * an error pending.
	 */
again:

	ret = waitpid (pid, &status, 0);

	if (ret < 0) {
		if (errno == EINTR)
			goto again;
		else if (!failed) {
			failed = TRUE;
			g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ,
				     _("Unexpected error in waitpid() (%s)"), g_strerror (errno));
		}
	}

	g_free (password);
	
	if (failed) {
		if (errstr)
			g_string_free (errstr, TRUE);
		g_string_free (outstr, TRUE);
		return FALSE;
	} else {
		if (exit_status)
			*exit_status = status;
		if (standard_error) {
			/* Sad as it may be, certain mount helpers (like mount.cifs)
			 * on Linux and perhaps other OSs return their error 
			 * output on stdout. */
			if (outstr->len && !errstr->len) {
				*standard_error = g_string_free (outstr, FALSE);
				g_string_free (errstr, TRUE);
			} else {
				*standard_error = g_string_free (errstr, FALSE);
				g_string_free (outstr, TRUE);
			}
		}
		return TRUE;
	}
}

static void *
mount_unmount_thread (void *arg)
{
	MountThreadInfo *info;
	char *standard_error;
	gint exit_status;
	GError *error;
	char *envp[] = {
		"LC_ALL=C",
		NULL
	};

	info = arg;

	info->succeeded = TRUE;
	info->error_message = NULL;
	info->detailed_error_message = NULL;

	if (info->should_mount || info->should_unmount) {
		error = NULL;
 		if (spawn_mount (info, 
#if defined(USE_HAL)
				  /* do pass our environment when using mate-mount progams */
				  ((strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-mount") == 0) ||
				   (strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-umount") == 0) ||
				   (strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-eject") == 0) ||
# if defined(HAL_MOUNT) && defined(HAL_UMOUNT)
				  /* do pass our environment when using hal mount progams */
				   (strcmp (info->argv[0], HAL_MOUNT) == 0) ||
				   (strcmp (info->argv[0], HAL_UMOUNT) == 0) ||
# endif
				   FALSE) ? NULL : envp,
#else
				   envp,
#endif
				   &standard_error,
				   &exit_status,
				   &error)) {
			if (exit_status != 0) {
				info->succeeded = FALSE;

				if ((strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-mount") == 0) ||
				    (strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-umount") == 0) ||
				    (strcmp (info->argv[0], MATE_VFS_BINDIR "/mate-eject") == 0)) {
					/* mate-mount programs display their own dialogs */
					info->error_message = g_strdup ("");
					info->detailed_error_message = g_strdup ("");
				} else {
					if (strlen (standard_error) > 0) {
						if (info->should_mount) {
							info->error_message = generate_mount_error_message (
								standard_error,
								info->device_type);
						} else {
							info->error_message = generate_unmount_error_message (
								standard_error,
								info->device_type);
						}
						info->detailed_error_message = g_strdup (standard_error);
					} else {
						/* As of 2.12 we introduce a new contract between mate-vfs clients
						 * invoking mount/unmount and the mate-vfs-daemon:
						 *
						 *       "don't display an error dialog if error_message and
						 *        detailed_error_message are both empty strings".
						 *
						 * We want this as we may use mount/unmount/ejects programs that
						 * shows it's own dialogs. 
						 */
						info->error_message = g_strdup ("");
						info->detailed_error_message = g_strdup ("");
					}
				}

			}

			g_free (standard_error);
		} else {
			/* spawn failure */
			info->succeeded = FALSE;
			info->error_message = g_strdup (_("Failed to start command"));
			info->detailed_error_message = g_strdup (error->message);
			g_error_free (error);
		}
	}

	if (info->should_eject) {
		char *argv[5];

		argv[0] = NULL;

#if defined(USE_HAL)
		if (info->hal_udi != NULL) {
			argv[0] = MATE_VFS_BINDIR "/mate-eject";
			argv[1] = "--hal-udi";
			argv[2] = info->hal_udi;
			argv[3] = NULL;
			
			if (!g_file_test (argv [0], G_FILE_TEST_IS_EXECUTABLE))
				argv[0] = NULL;
		}
# if defined(HAL_EJECT)
		if (argv[0] == NULL && info->hal_udi != NULL) {
			argv[0] = HAL_EJECT;
			argv[1] = info->device_path;
			argv[2] = NULL;
			
			if (!g_file_test (argv [0], G_FILE_TEST_IS_EXECUTABLE))
				argv[0] = NULL;
		}
# endif
#endif

		if (argv[0] == NULL) {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
			argv[0] = "cdcontrol";
			argv[1] = "-f";
			argv[2] = info->device_path?info->device_path:info->mount_point;
			argv[3] = "eject";
			argv[4] = NULL;
#else
			argv[0] = "eject";
			argv[1] = info->device_path?info->device_path:info->mount_point;
			argv[2] = NULL;
#endif
		}

		error = NULL;
		if (g_spawn_sync (NULL,
				  argv,
				  NULL,
				  G_SPAWN_SEARCH_PATH,
				  NULL, NULL,
				  NULL, &standard_error,
				  &exit_status,
				  &error)) {

			if (info->succeeded == FALSE) {
				g_free(info->error_message);
				info->error_message = NULL;
				g_free(info->detailed_error_message);
				info->detailed_error_message = NULL;
			}

			if (exit_status != 0) {
				info->succeeded = FALSE;
				if ((strcmp (argv[0], MATE_VFS_BINDIR "/mate-mount") == 0) ||
				    (strcmp (argv[0], MATE_VFS_BINDIR "/mate-umount") == 0) ||
				    (strcmp (argv[0], MATE_VFS_BINDIR "/mate-eject") == 0)) {
					/* mate-mount programs display their own dialogs */
					info->error_message = g_strdup ("");
					info->detailed_error_message = g_strdup ("");
				} else {
					info->error_message = g_strdup (_("Unable to eject media"));
					info->detailed_error_message = g_strdup (standard_error);
				}
			} else {
				/* If the eject succeed then ignore the previous unmount error (if any) */
				info->succeeded = TRUE;
			}

			g_free (standard_error);
		} else {
			/* Spawn failure */
			if (info->succeeded) {
				info->succeeded = FALSE;
				info->error_message = g_strdup (_("Failed to start command"));
				info->detailed_error_message = g_strdup (error->message);
			}
			g_error_free (error);
		}
	}

	g_idle_add (report_mount_result, info);	
	
	pthread_exit (NULL); 	
	
	return NULL;
}

#endif	/* !G_OS_WIN32 */

static void
mount_unmount_operation (const char *mount_point,
			 const char *device_path,
			 const char *hal_udi,
			 MateVFSDeviceType device_type,
			 gboolean should_mount,
			 gboolean should_unmount,
			 gboolean should_eject,
			 MateVFSVolumeOpCallback  callback,
			 gpointer                  user_data)
{
#ifndef G_OS_WIN32
	const char *command = NULL;
	const char *argument = NULL;
	MountThreadInfo *mount_info;
	pthread_t mount_thread;
	const char *name;
	int i;
	gboolean is_in_media;

#ifdef USE_VOLRMMOUNT
	if (mount_point != NULL) {
		name = strrchr (mount_point, '/');
	if (name != NULL) {
		name = name + 1;
	} else {
		name = mount_point;
	}
#else

# ifdef USE_HAL
	if (hal_udi != NULL) {
		name = device_path;
	} else
		name = mount_point;
# else
	name = mount_point;
# endif

#endif

	is_in_media = mount_point != NULL ? g_str_has_prefix (mount_point, "/media") : FALSE;

	command = NULL;

	if (should_mount) {
#if defined(USE_HAL)
	       if (hal_udi != NULL && g_file_test (MATE_VFS_BINDIR "/mate-mount", G_FILE_TEST_IS_EXECUTABLE)) {
		       command = MATE_VFS_BINDIR "/mate-mount";
		       argument = "--hal-udi";
		       name = hal_udi;
	       } else {
# if defined(HAL_MOUNT)
		       if (hal_udi != NULL && g_file_test (HAL_MOUNT, G_FILE_TEST_IS_EXECUTABLE))
			       command = HAL_MOUNT;
		       else
			       command = find_command (MOUNT_COMMAND);
# else
		       ;
# endif
	       }
#endif
	       if (command == NULL)
		       command = find_command (is_in_media ? PMOUNT_COMMAND : MOUNT_COMMAND);
#ifdef  MOUNT_ARGUMENT
	       argument = MOUNT_ARGUMENT;
#endif
       }

       if (should_unmount) {
#if defined(USE_HAL)
	       if (hal_udi != NULL && g_file_test (MATE_VFS_BINDIR "/mate-umount", G_FILE_TEST_IS_EXECUTABLE)) {
		       command = MATE_VFS_BINDIR "/mate-umount";
		       argument = "--hal-udi";
		       name = hal_udi;
	       } else {
# if defined(HAL_UMOUNT)
		       if (hal_udi != NULL && g_file_test (HAL_UMOUNT, G_FILE_TEST_IS_EXECUTABLE))
			       command = HAL_UMOUNT;
		       else
			       command = find_command (UMOUNT_COMMAND);
# else
		       ;
#endif
	       }
#endif
	       if (command == NULL)
		       command = find_command (is_in_media ? PUMOUNT_COMMAND : UMOUNT_COMMAND);
#ifdef  UNMOUNT_ARGUMENT
		argument = UNMOUNT_ARGUMENT;
#endif
       }

	mount_info = g_new0 (MountThreadInfo, 1);

	if (command) {
		i = 0;
		mount_info->argv[i++] = g_strdup (command);
		if (argument) {
			mount_info->argv[i++] = g_strdup (argument);
		}
		mount_info->argv[i++] = g_strdup (name);
		mount_info->argv[i++] = NULL;
	}
	
	mount_info->mount_point = g_strdup (mount_point != NULL ? mount_point : "");
	mount_info->device_path = g_strdup (device_path);
	mount_info->device_type = device_type;
	mount_info->hal_udi = g_strdup (hal_udi);
	mount_info->should_mount = should_mount;
	mount_info->should_unmount = should_unmount;
	mount_info->should_eject = should_eject;
	mount_info->callback = callback;
	mount_info->user_data = user_data;
	
	pthread_create (&mount_thread, NULL, mount_unmount_thread, mount_info);
#else
	g_warning ("Not implemented: mount_unmount_operation()\n");
#endif
}


/* TODO: check if already mounted/unmounted, emit pre_unmount, check for mount types */

static void
emit_pre_unmount (MateVFSVolume *volume)
{
	MateVFSVolumeMonitor *volume_monitor;
	
	volume_monitor = mate_vfs_get_volume_monitor ();

	if (mate_vfs_get_is_daemon ()) {
		mate_vfs_volume_monitor_emit_pre_unmount (volume_monitor,
							   volume);
	} else {
		_mate_vfs_volume_monitor_client_dbus_emit_pre_unmount (
			MATE_VFS_VOLUME_MONITOR_CLIENT (volume_monitor), volume);

		/* Do a synchronous pre_unmount for this client too, avoiding
		 * races at least in this process
		 */
		mate_vfs_volume_monitor_emit_pre_unmount (volume_monitor,
							   volume);
		/* sleep for a while to get other apps to release their
		 * hold on the device */
		g_usleep (0.5*G_USEC_PER_SEC);
				
	}
}

static void
unmount_connected_server (MateVFSVolume *volume,
			  MateVFSVolumeOpCallback  callback,
			  gpointer                   user_data)
{
	MateConfClient *client;
	gboolean res;
	char *key;
	gboolean success;
	char *detailed_error;
	GError *error;

	success = TRUE;
	detailed_error = NULL;
	
	client = mateconf_client_get_default ();

	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   volume->priv->mateconf_id,
			   "/uri", NULL);
	error = NULL;
	res = mateconf_client_unset (client, key, &error);
	g_free (key);
	if (!res) {
		if (success) {
			detailed_error = g_strdup (error->message);
		}
		success = FALSE;
		g_error_free (error);
	}
	
	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   volume->priv->mateconf_id,
			   "/icon", NULL);
	res = mateconf_client_unset (client, key, &error);
	g_free (key);
	if (!res) {
		if (success) {
			detailed_error = g_strdup (error->message);
		}
		success = FALSE;
		g_error_free (error);
	}
	
	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   volume->priv->mateconf_id,
			   "/display_name", NULL);
	res = mateconf_client_unset (client, key, &error);
	g_free (key);
	if (!res) {
		if (success) {
			detailed_error = g_strdup (error->message);
		}
		success = FALSE;
		g_error_free (error);
	}
	
	g_object_unref (client);

	if (success) {
		(*callback) (success, NULL, NULL, user_data);
	} else {
		(*callback) (success, (char *) _("Unable to unmount connected server"), detailed_error, user_data);
	}
	g_free (detailed_error);
}

/** 
 * mate_vfs_volume_unmount:
 * @volume: the #MateVFSVolume that should be unmounted.
 * @callback: the #MateVFSVolumeOpCallback that should be invoked after unmounting @volume.
 * @user_data: the user data to pass to @callback.
 *
 * Note that mate_vfs_volume_unmount() may also invoke mate_vfs_volume_eject(),
 * if the @volume signals that it should be ejected when it is unmounted.
 * This may be true for CD-ROMs, USB sticks and other devices, depending on the
 * backend providing the @volume.
 *
 * Since: 2.6
 */
void
mate_vfs_volume_unmount (MateVFSVolume *volume,
			  MateVFSVolumeOpCallback  callback,
			  gpointer                   user_data)
{
	char *mount_path, *device_path;
	char *uri;
	MateVFSVolumeType type;

	if (volume->priv->drive != NULL) {
		if (volume->priv->drive->priv->must_eject_at_unmount) {
			mate_vfs_volume_eject (volume, callback, user_data);
			return;
		}
	}

	emit_pre_unmount (volume);

	type = mate_vfs_volume_get_volume_type (volume);
	if (type == MATE_VFS_VOLUME_TYPE_MOUNTPOINT) {
		char *hal_udi;

		uri = mate_vfs_volume_get_activation_uri (volume);
		mount_path = mate_vfs_get_local_path_from_uri (uri);
		g_free (uri);
		device_path = mate_vfs_volume_get_device_path (volume);
		hal_udi = mate_vfs_volume_get_hal_udi (volume);

		/* Volumes from drives that are not polled may not
		 * have a hal_udi.. take the one from HAL to get
		 * mate-mount working */
		if (hal_udi == NULL) {
			MateVFSDrive *drive;
			drive = mate_vfs_volume_get_drive (volume);
			if (drive != NULL) {
				hal_udi = mate_vfs_drive_get_hal_udi (drive);
				mate_vfs_drive_unref (drive);
			}
		}

		mount_unmount_operation (mount_path,
					 device_path,
					 hal_udi,
					 mate_vfs_volume_get_device_type (volume),
					 FALSE, TRUE, FALSE,
					 callback, user_data);
		g_free (mount_path);
		g_free (device_path);
		g_free (hal_udi);
	} else if (type == MATE_VFS_VOLUME_TYPE_VFS_MOUNT) {
		/* left intentionally blank as these cannot be mounted and thus not unmounted */
	} else if (type == MATE_VFS_VOLUME_TYPE_CONNECTED_SERVER) {
		unmount_connected_server (volume, callback, user_data);
	}
}

/** 
 * mate_vfs_volume_eject:
 * @volume: the #MateVFSVolume that should be ejected.
 * @callback: the #MateVFSVolumeOpCallback that should be invoked after ejecting of @volume.
 * @user_data: the user data to pass to @callback.
 *
 * Requests ejection of a #MateVFSVolume.
 *
 * Before the unmount operation is executed, the
 * #MateVFSVolume::pre-unmount signal is emitted.
 *
 * If the @volume is a mount point (its type is
 * #MATE_VFS_VOLUME_TYPE_MOUNTPOINT), it is unmounted,
 * and if it refers to a disc, it is also ejected.
 *
 * If the @volume is a special VFS mount, i.e.
 * its type is #MATE_VFS_VOLUME_TYPE_VFS_MOUNT, it
 * is ejected.
 *
 * If the @volume is a connected server, it
 * is removed from the list of connected servers.
 *
 * Otherwise, no further action is done.
 *
 * Since: 2.6
 */
void
mate_vfs_volume_eject (MateVFSVolume *volume,
			MateVFSVolumeOpCallback  callback,
			gpointer                   user_data)
{
	char *mount_path, *device_path;
	char *uri;
	char *hal_udi;
	MateVFSVolumeType type;
	
	emit_pre_unmount (volume);

	type = mate_vfs_volume_get_volume_type (volume);
	if (type == MATE_VFS_VOLUME_TYPE_MOUNTPOINT) {
		uri = mate_vfs_volume_get_activation_uri (volume);
		mount_path = mate_vfs_get_local_path_from_uri (uri);
		g_free (uri);
		device_path = mate_vfs_volume_get_device_path (volume);
		hal_udi = mate_vfs_volume_get_hal_udi (volume);

		/* Volumes from drives that are not polled may not
		 * have a hal_udi.. take the one from HAL to get
		 * mate-mount working */
		if (hal_udi == NULL) {
			MateVFSDrive *drive;
			drive = mate_vfs_volume_get_drive (volume);
			if (drive != NULL) {
				hal_udi = mate_vfs_drive_get_hal_udi (drive);
				mate_vfs_drive_unref (drive);
			}
		}

		mount_unmount_operation (mount_path,
					 device_path,
					 hal_udi,
					 mate_vfs_volume_get_device_type (volume),
					 FALSE, TRUE, TRUE,
					 callback, user_data);
		g_free (mount_path);
		g_free (device_path);
		g_free (hal_udi);
	} else if (type == MATE_VFS_VOLUME_TYPE_VFS_MOUNT) {
		hal_udi = mate_vfs_volume_get_hal_udi (volume);
		uri = mate_vfs_volume_get_activation_uri (volume);
		device_path = mate_vfs_volume_get_device_path (volume);

		/* special handling for optical disc VFS_MOUNT created by the hal backend */
		if (hal_udi != NULL &&
		    (g_str_has_prefix (uri, "cdda://") || g_str_has_prefix (uri, "burn:///"))) {
			device_path = mate_vfs_volume_get_device_path (volume);
			mount_unmount_operation (NULL,
						 device_path,
						 hal_udi,
						 mate_vfs_volume_get_device_type (volume),
						 FALSE, FALSE, TRUE,
						 callback, user_data);
			g_free (device_path);
		    }
		g_free (uri);
		g_free (hal_udi);
	} else if (type == MATE_VFS_VOLUME_TYPE_CONNECTED_SERVER) {
		unmount_connected_server (volume, callback, user_data);
	}
}

/** 
 * mate_vfs_drive_mount:
 * @drive: the #MateVFSDrive that should be mounted.
 * @callback: the #MateVFSVolumeOpCallback that should be invoked after mounting @drive.
 * @user_data: the user data to pass to @callback.
 *
 * Since: 2.6
 */
void
mate_vfs_drive_mount (MateVFSDrive  *drive,
		       MateVFSVolumeOpCallback  callback,
		       gpointer                   user_data)
{
	char *mount_path, *device_path, *uri;

	uri = mate_vfs_drive_get_activation_uri (drive);
	mount_path = mate_vfs_get_local_path_from_uri (uri);
	g_free (uri);
	device_path = mate_vfs_drive_get_device_path (drive);
	mount_unmount_operation (mount_path,
				 device_path,
				 mate_vfs_drive_get_hal_udi (drive),
				 mate_vfs_drive_get_device_type (drive),
				 TRUE, FALSE, FALSE,
				 callback, user_data);
	g_free (mount_path);
	g_free (device_path);
}

/** 
 * mate_vfs_drive_unmount:
 * @drive: the #MateVFSDrive that should be unmounted.
 * @callback: the #MateVFSVolumeOpCallback that should be invoked after unmounting @drive.
 * @user_data: the user data to pass to @callback.
 *
 * mate_vfs_drive_unmount() invokes mate_vfs_drive_eject(), if the @drive signals
 * that it should be ejected when it is unmounted. This may be true for CD-ROMs,
 * USB sticks and other devices, depending on the backend providing the #MateVFSDrive @drive.
 *
 * If the @drive does not signal that it should be ejected when it is unmounted,
 * mate_vfs_drive_unmount() calls mate_vfs_volume_unmount() for each of the
 * @drive's mounted #MateVFSVolumes, which can be queried using
 * mate_vfs_drive_get_mounted_volumes().
 *
 * Since: 2.6
 */
void
mate_vfs_drive_unmount (MateVFSDrive  *drive,
			 MateVFSVolumeOpCallback  callback,
			 gpointer                   user_data)
{
	GList *vol_list;
	GList *current_vol;

	if (drive->priv->must_eject_at_unmount) {
		mate_vfs_drive_eject (drive, callback, user_data);
		return;
	}

	vol_list = mate_vfs_drive_get_mounted_volumes (drive);

	for (current_vol = vol_list; current_vol != NULL; current_vol = current_vol->next) {
		MateVFSVolume *vol;
		vol = MATE_VFS_VOLUME (current_vol->data);

		mate_vfs_volume_unmount (vol,
					  callback,
					  user_data);
	}

	mate_vfs_drive_volume_list_free (vol_list);
}

/** 
 * mate_vfs_drive_eject:
 * @drive: the #MateVFSDrive that should be ejcted.
 * @callback: the #MateVFSVolumeOpCallback that should be invoked after ejecting @drive.
 * @user_data: the user data to pass to @callback.
 *
 * If @drive has associated #MateVFSVolume objects, all of them will be
 * unmounted by calling mate_vfs_volume_unmount() for each volume in
 * mate_vfs_drive_get_mounted_volumes(), except for the last one,
 * for which mate_vfs_volume_eject() is called to ensure that the
 * @drive's media is ejected.
 *
 * If @drive however has no associated #MateVFSVolume objects, it
 * simply calls an unmount helper on the @drive.
 *
 * Since: 2.6
 */
void
mate_vfs_drive_eject (MateVFSDrive  *drive,
		       MateVFSVolumeOpCallback  callback,
		       gpointer                   user_data)
{
	GList *vol_list;
	GList * current_vol;

	vol_list = mate_vfs_drive_get_mounted_volumes (drive);

	for (current_vol = vol_list; current_vol != NULL; current_vol = current_vol->next) {
		MateVFSVolume *vol;
		vol = MATE_VFS_VOLUME (current_vol->data);

		/* Check to see if this is the last volume */
		/* If not simply unmount it */
		/* If so the eject the media along with the unmount */
		if (current_vol->next != NULL) { 
			mate_vfs_volume_unmount (vol,
						  callback,
						  user_data);
		} else { 
			mate_vfs_volume_eject (vol,
						callback,
						user_data);
		}

	}

	if (vol_list == NULL) { /* no mounted volumes */
		char *mount_path, *device_path, *uri;
	
		uri = mate_vfs_drive_get_activation_uri (drive);
		mount_path = mate_vfs_get_local_path_from_uri (uri);
		g_free (uri);
		device_path = mate_vfs_drive_get_device_path (drive);
		mount_unmount_operation (mount_path,
					 device_path,
					 mate_vfs_drive_get_hal_udi (drive),
					 MATE_VFS_DEVICE_TYPE_UNKNOWN,
					 FALSE, FALSE, TRUE,
					 callback, user_data);
		g_free (mount_path);
		g_free (device_path);
	}

	mate_vfs_drive_volume_list_free (vol_list);
}


/** 
 * mate_vfs_connect_to_server:
 * @uri: The string representation of the server to connect to.
 * @display_name: The display name that is used to identify the server connection.
 * @icon: The icon that is used to identify the server connection.
 *
 * This function adds a server connection to the specified @uri, which is displayed
 * in user interfaces with the specified @display_name and @icon.
 *
 * If this function is invoked successfully, the created server shows up in the
 * list of mounted volumes of the #MateVFSVolumeMonitor, which can be queried
 * using mate_vfs_volume_monitor_get_mounted_volumes().
 *
 * <note>
 * <para>
 * This function does not have a return value. Hence, you can't easily detect
 * whether the specified server was successfully created. The actual creation and
 * consumption of the new server through the #MateVFSVolumeMonitor is done
 * asynchronously.
 * </para>
 * <para>
 * @uri, @display_name, and @icon can be freely chosen, but should be meaningful:
 * </para>
 * <para>
 * @uri should refer to a valid location. You can check the validity of the
 * location by calling mate_vfs_uri_new() with @uri, and checking whether
 * the return value is not %NULL.
 * </para>
 * <para>
 * The @display_name should be queried from the user, and an empty string
 * should not be considered valid.
 * </para>
 * <para>
 * @icon typically references an icon from the icon theme. Some
 * implementations currently use <literal>mate-fs-smb</literal>,
 * <literal>mate-fs-ssh</literal>, <literal>mate-fs-ftp</literal> and
 * <literal>mate-fs-share</literal>, depending on the type of the server
 * referenced by @uri. The icon naming conventions might change in the
 * future, though. Obeying the <ulink
 * url="http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html">
 * freedesktop.org Icon Naming Specification</ulink> is suggested.
 * </para>
 * </note>
 *
 * Since: 2.6
 */
void
mate_vfs_connect_to_server (const char               *uri,
			     const char               *display_name,
			     const char               *icon)
{
	MateConfClient *client;
	GSList *dirs, *l;
	char *dir, *dir_id;
	int max_id, mateconf_id;
	char *key;
	char *id;

	client = mateconf_client_get_default ();

	max_id = 0;
	dirs = mateconf_client_all_dirs (client,
				      CONNECTED_SERVERS_DIR, NULL);
	for (l = dirs; l != NULL; l = l->next) {
		dir = l->data;

		dir_id = strrchr (dir, '/');
		if (dir_id != NULL) {
			dir_id++;
			mateconf_id = strtol (dir_id, NULL, 10);
			max_id = MAX (max_id, mateconf_id);
		}
		
		g_free (dir);
	}
	g_slist_free (dirs);

	id = g_strdup_printf ("%d", max_id + 1);
	
	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   id,
			   "/icon", NULL);
	mateconf_client_set_string (client, key, icon, NULL);
	g_free (key);
	
	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   id,
			   "/display_name", NULL);
	mateconf_client_set_string (client, key, display_name, NULL);
	g_free (key);

	/* Uri key creation triggers creation, do this last */
	key = g_strconcat (CONNECTED_SERVERS_DIR "/",
			   id,
			   "/uri", NULL);
	mateconf_client_set_string (client, key, uri, NULL);
	g_free (key);
	
	g_free (id);
	g_object_unref (client);
}
