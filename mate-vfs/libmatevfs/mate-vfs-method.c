/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-method.c - Handling of access methods in the MATE
   Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>
#include "mate-vfs-method.h"

#include "mate-vfs-configuration.h"
#include "mate-vfs-private.h"
#ifdef G_OS_WIN32
#include "mate-vfs-private-utils.h"
#endif
#include <gmodule.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-transform.h>
#include <libmatevfs/mate-vfs-method.h>
#ifdef USE_DAEMON
#include "mate-vfs-daemon-method.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MATE_VFS_MODULE_INIT      "vfs_module_init"
#define MATE_VFS_MODULE_TRANSFORM "vfs_module_transform"
#define MATE_VFS_MODULE_SHUTDOWN  "vfs_module_shutdown"

struct _ModuleElement {
	char *name;
	const char *args;
	MateVFSMethod *method;
	MateVFSTransform *transform;
	MateVFSMethodShutdownFunc shutdown_function;
	gboolean run_in_daemon;
};
typedef struct _ModuleElement ModuleElement;

static gboolean method_already_initialized = FALSE;

static gboolean mate_vfs_is_daemon = FALSE;
static GType daemon_volume_monitor_type = 0;
static void (*daemon_force_probe_callback) (MateVFSVolumeMonitor *volume_monitor) = NULL;

static GHashTable *module_hash = NULL;
G_LOCK_DEFINE_STATIC (mate_vfs_method_init);
static GStaticRecMutex module_hash_lock = G_STATIC_REC_MUTEX_INIT;

static GList *module_path_list = NULL;

/* Pass some integration stuff here, so we can make the library not depend
   on the daemon-side code */
void
mate_vfs_set_is_daemon (GType volume_monitor_type,
			 MateVFSDaemonForceProbeCallback force_probe_callback)
{
	mate_vfs_is_daemon = TRUE;
	daemon_volume_monitor_type = volume_monitor_type;
	daemon_force_probe_callback = force_probe_callback;
}

gboolean
mate_vfs_get_is_daemon (void)
{
	return mate_vfs_is_daemon;
}

GType
mate_vfs_get_daemon_volume_monitor_type (void)
{
	return daemon_volume_monitor_type;
}

MateVFSDaemonForceProbeCallback
_mate_vfs_get_daemon_force_probe_callback (void)
{
	return daemon_force_probe_callback;
}

static void
module_element_free (gpointer elementp)
{
	ModuleElement *element = (ModuleElement *) elementp;
	if (element->shutdown_function != NULL) {
		(* element->shutdown_function) (element->method);
	}

	/* NB: this is the key */
	g_free (element->name);
	g_free (element);
}

static gboolean
init_hash_table (void)
{
	module_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					     NULL, module_element_free);

	return TRUE;
}

static gboolean
install_path_list (const gchar *user_path_list)
{
	const gchar *p, *oldp;

	/* Notice that this assumes the list has already been locked.  */

	oldp = user_path_list;
	while (1) {
		gchar *elem;

		p = strchr (oldp, G_SEARCHPATH_SEPARATOR);

		if (p == NULL) {
			if (*oldp != '\0') {
				elem = g_strdup (oldp);
				module_path_list = g_list_append
						       (module_path_list, elem);
			}
			break;
		} else if (p != oldp) {
			elem = g_strndup (oldp, p - oldp);
			module_path_list = g_list_append (module_path_list,
							  elem);
		} else {
			elem = NULL;
		}

		oldp = p + 1;
	}

	return TRUE;
}

static gboolean
init_path_list (void)
{
	const gchar *user_path_list;

	if (module_path_list != NULL)
		return TRUE;

	/* User-supplied path.  */

	user_path_list = getenv ("MATE_VFS_MODULE_PATH");
	if (user_path_list != NULL) {
		if (! install_path_list (user_path_list))
			return FALSE;
	}

	/* Default path.  It comes last so that users can override it.  */

	module_path_list = g_list_append (module_path_list,
					  g_build_filename (MATE_VFS_LIBDIR,
							    MATE_VFS_MODULE_SUBDIR,
							    NULL));

	return TRUE;
}

/**
 * mate_vfs_method_init:
 *
 * Initializes the mate-vfs methods. If already initialized then will simply return
 * %TRUE.
 *
 * Return value: Returns %TRUE.
 */
gboolean
mate_vfs_method_init (void)
{
	G_LOCK (mate_vfs_method_init);

	if (method_already_initialized)
		goto mate_vfs_method_init_out;

	if (! init_hash_table ())
		goto mate_vfs_method_init_out;
	if (! init_path_list ())
		goto mate_vfs_method_init_out;

	method_already_initialized = TRUE;

 mate_vfs_method_init_out:
	G_UNLOCK (mate_vfs_method_init);

	return method_already_initialized;
}

static void
load_module (const gchar *module_name, const char *method_name, const char *args,
	     MateVFSMethod **method, MateVFSTransform **transform,
	     MateVFSMethodShutdownFunc *shutdown_function)
{
	GModule *module;
	MateVFSMethod *temp_method = NULL;
	MateVFSTransform *temp_transform = NULL;
	
	MateVFSMethodInitFunc init_function = NULL;
	MateVFSTransformInitFunc transform_function = NULL;

	*method = NULL;
	*transform = NULL;
	*shutdown_function = NULL;

	module = g_module_open (module_name, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
	if (module == NULL) {
		g_warning ("Cannot load module `%s' (%s)", module_name, g_module_error ());
		return;
	}

        g_module_symbol (module, MATE_VFS_MODULE_INIT,
			 (gpointer *) &init_function);
	g_module_symbol (module, MATE_VFS_MODULE_TRANSFORM,
			 (gpointer *) &transform_function);
	g_module_symbol (module, MATE_VFS_MODULE_SHUTDOWN,
			 (gpointer *) shutdown_function);
	
	if ((init_function == NULL || *shutdown_function == NULL) &&
	    (transform_function == NULL)) {
		g_warning ("module '%s' does not have init, transform and shutfown functions; may be an out-of-date module", module_name);
		return;
	}

	if (init_function)
		temp_method = (* init_function) (method_name, args);

	if (temp_method == NULL && init_function) {
		g_warning ("module '%s' returned a NULL handle", module_name);
		return;
	}

	if (temp_method != NULL) {
		/* Some basic checks */
		if (temp_method->method_table_size == 0) {
			g_warning ("module '%s' has 0 table size", module_name);
			return;
		} else if (temp_method->method_table_size > (0x100 * sizeof (MateVFSMethod))) {
			g_warning ("module '%s' has unreasonable table size, perhaps it is using the old MateVFSMethod struct?", module_name);
			return;
		} else if (!VFS_METHOD_HAS_FUNC(temp_method, open)) {
			g_warning ("module '%s' has no open fn", module_name);
			return;
#if 0
		} else if (!VFS_METHOD_HAS_FUNC(temp_method, create)) {
			g_warning ("module '%s' has no create fn", module_name);
			return;
#endif
		} else if (!VFS_METHOD_HAS_FUNC(temp_method, is_local)) {
			g_warning ("module '%s' has no is-local fn", module_name);
			return;
#if 0
		} else if (!VFS_METHOD_HAS_FUNC(temp_method, get_file_info)) {
			g_warning ("module '%s' has no get-file-info fn", module_name);
			return;
#endif
		}

		/* More advanced assumptions.  */
		if (VFS_METHOD_HAS_FUNC(temp_method, tell) && !VFS_METHOD_HAS_FUNC(temp_method, seek)) {
			g_warning ("module '%s' has tell and no seek", module_name);
			return;
		}

		if (VFS_METHOD_HAS_FUNC(temp_method, seek) && !VFS_METHOD_HAS_FUNC(temp_method, tell)) {
			g_warning ("module '%s' has seek and no tell", module_name);
			return;
		}
	}

	if (transform_function)
		temp_transform = (* transform_function) (method_name, args);
	if (temp_transform) {
		if (temp_transform->transform == NULL) {
			g_warning ("module '%s' has no transform method", module_name);
			return;
		}
	}

	*method = temp_method;
	*transform = temp_transform;
}

static void
load_module_in_path_list (const gchar *base_name, const char *method_name, const char *args,
			  MateVFSMethod **method, MateVFSTransform **transform,
			  MateVFSMethodShutdownFunc *shutdown_function)
{
	GList *p;

	*method = NULL;
	*transform = NULL;
	
	for (p = module_path_list; p != NULL; p = p->next) {
		const gchar *path;
		gchar *name;

		path = p->data;
		name = g_module_build_path (path, base_name);

		load_module (name, method_name, args, method, transform, shutdown_function);
		g_free (name);

		if (*method != NULL || *transform != NULL)
			return;
	}
}

static ModuleElement *
mate_vfs_add_module_to_hash_table (const gchar *name)
{
	MateVFSMethod *method = NULL;
	MateVFSTransform *transform = NULL;
	MateVFSMethodShutdownFunc shutdown_function = NULL;
	ModuleElement *module_element;
	const char *module_name;
#if defined (HAVE_SETEUID) || defined (HAVE_SETRESUID)
	uid_t saved_uid;
#endif
#if defined (HAVE_SETEGID) || defined (HAVE_SETRESGID)
	gid_t saved_gid;
#endif
	const char *args;
	gboolean run_in_daemon;

	g_static_rec_mutex_lock (&module_hash_lock);

	module_element = g_hash_table_lookup (module_hash, name);

	if (module_element != NULL)
		goto add_module_out;

	module_name = _mate_vfs_configuration_get_module_path (name, &args, &run_in_daemon);
	if (module_name == NULL)
		goto add_module_out;

	if (mate_vfs_is_daemon || !run_in_daemon) {
		/* Set the effective UID/GID to the user UID/GID to prevent attacks to
		   setuid/setgid executables.  */
		
#if defined (HAVE_SETEUID) || defined (HAVE_SETRESUID)
		saved_uid = geteuid ();
#endif
#if defined (HAVE_SETEGID) || defined (HAVE_SETRESGID)
		saved_gid = getegid ();
#endif
#if defined(HAVE_SETEUID)
		seteuid (getuid ());
#elif defined(HAVE_SETRESUID)
		setresuid (-1, getuid (), -1);
#endif
#if defined(HAVE_SETEGID)
		setegid (getgid ());
#elif defined(HAVE_SETRESGID)
		setresgid (-1, getgid (), -1);
#endif
		
		if (g_path_is_absolute (module_name))
			load_module (module_name, name, args, &method, &transform, &shutdown_function);
		else
			load_module_in_path_list (module_name, name, args, &method, &transform, &shutdown_function);
		
#if defined(HAVE_SETEUID)
		seteuid (saved_uid);
#elif defined(HAVE_SETRESUID)
		setresuid (-1, saved_uid, -1);
#endif
#if defined(HAVE_SETEGID)
		setegid (saved_gid);
#elif defined(HAVE_SETRESGID)
		setresgid (-1, saved_gid, -1);
#endif
	} else {
#ifdef USE_DAEMON
		method = _mate_vfs_daemon_method_get ();
#endif
	}

	if (method == NULL && transform == NULL)
		goto add_module_out;
	module_element = g_new (ModuleElement, 1);
	module_element->name = g_strdup (name);
	module_element->method = method;
	module_element->transform = transform;
	module_element->shutdown_function = shutdown_function;
	module_element->run_in_daemon = run_in_daemon;

	g_hash_table_insert (module_hash, module_element->name, module_element);

 add_module_out:
	g_static_rec_mutex_unlock (&module_hash_lock);

	return module_element;
}

/**
 * mate_vfs_method_get:
 * @name: name of the protocol.
 *
 * Returns the method handle for the given protocol @name. @name could be any protocol
 * which mate-vfs implements. Like ftp, http, smb etc..
 */
MateVFSMethod *
mate_vfs_method_get (const gchar *name)
{
	ModuleElement *module_element;

	g_return_val_if_fail (name != NULL, NULL);

	module_element = mate_vfs_add_module_to_hash_table (name);
	return module_element ? module_element->method : NULL;
}

/**
 * mate_vfs_transform_get:
 * @name: name of the method to get the transform of.
 *
 * Get the transform for the method @name.
 *
 * Return value: a #MateVFSTransform handle for @name.
 */

MateVFSTransform *
mate_vfs_transform_get (const gchar *name)
{
	ModuleElement *module_element;

	g_return_val_if_fail (name != NULL, NULL);

	module_element = mate_vfs_add_module_to_hash_table (name);
	return module_element ? module_element->transform : NULL;
}

void
_mate_vfs_method_shutdown (void)
{
	G_LOCK (mate_vfs_method_init);

	if (module_hash != NULL) {
		g_hash_table_destroy (module_hash);
		module_hash = NULL;
	}

	method_already_initialized = FALSE;

	G_UNLOCK (mate_vfs_method_init);
}
