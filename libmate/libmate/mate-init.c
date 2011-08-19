/* matelib-init.c - Implement libmate module
   Copyright (C) 1997, 1998, 1999 Free Software Foundation
                 1999, 2000 Red Hat, Inc.
		 2001 SuSE Linux AG.
   All rights reserved.

   This file is part of MATE 2.0.

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
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */
/*
  @NOTATION@
 */

#include <config.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "mate-i18n.h"
#include "mate-init.h"
#include "mate-mateconfP.h"
#include "mate-util.h"
#include "mate-sound.h"
#include "mate-triggers.h"
#include "libmate-private.h"

#include <matecomponent-activation/matecomponent-activation.h>
#include <matecomponent-activation/matecomponent-activation-version.h>
#include <libmatecomponent.h>

#include <libmatevfs/mate-vfs-init.h>

/* implemented in mate-sound.c */
G_GNUC_INTERNAL extern void _mate_sound_set_enabled (gboolean);

/*****************************************************************************
 * matecomponent
 *****************************************************************************/

static void
matecomponent_post_args_parse (MateProgram *program, MateModuleInfo *mod_info)
{
	int dumb_argc = 1;
	char *dumb_argv[] = {NULL};

	dumb_argv[0] = g_get_prgname ();

	matecomponent_init (&dumb_argc, dumb_argv);
}

/**
* mate_matecomponent_module_info_get:
*
* Retrieves the matecomponent module version and indicate that it requires the current
* libmate and its dependencies (although libmatecomponent does not depend on
* libmate, libmatecomponentui does and this will also be initialised when
* initialising a MATE app).
*
* Returns: a new #MateModuleInfo structure describing the version of the
* matecomponent modules and its dependents.
*/
const MateModuleInfo *
mate_matecomponent_module_info_get (void)
{
	static MateModuleInfo module_info = {
		"matecomponent",
		/* FIXME: get this from matecomponent */"1.101.2",
		N_("MateComponent Support"),
		NULL, NULL,
		NULL, matecomponent_post_args_parse,
		NULL, NULL, NULL, NULL, NULL
	};

	if (module_info.requirements == NULL) {
		static MateModuleRequirement req[2];

		req[0].required_version = VERSION;
		req[0].module_info = LIBMATE_MODULE;

		req[1].required_version = NULL;
		req[1].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}

/*****************************************************************************
 * matecomponent-activation
 *****************************************************************************/

static void
matecomponent_activation_pre_args_parse (MateProgram *program, MateModuleInfo *mod_info)
{
        if (!g_thread_supported ())
		g_thread_init (NULL);

	if (!matecomponent_activation_is_initialized ())
		matecomponent_activation_preinit (program, mod_info);
}

static void
matecomponent_activation_post_args_parse (MateProgram *program, MateModuleInfo *mod_info)
{
	if (!matecomponent_activation_is_initialized ()) {
		int dumb_argc = 1;
		char *dumb_argv[] = {NULL};

		dumb_argv[0] = g_get_prgname ();
		(void) matecomponent_activation_orb_init (&dumb_argc, dumb_argv);

		matecomponent_activation_postinit (program, mod_info);
	}
}

/* No need to make this public, always pulled in */
static const MateModuleInfo *
mate_matecomponent_activation_module_info_get (void)
{
	static MateModuleInfo module_info = {
		"matecomponent-activation", NULL, N_("MateComponent activation Support"),
		NULL, NULL,
		matecomponent_activation_pre_args_parse, matecomponent_activation_post_args_parse,
		matecomponent_activation_popt_options, NULL, NULL, NULL,
		matecomponent_activation_get_goption_group
	};

	if (module_info.version == NULL) {
		module_info.version = g_strdup_printf
			("%d.%d.%d",
			 MATECOMPONENT_ACTIVATION_MAJOR_VERSION,
			 MATECOMPONENT_ACTIVATION_MINOR_VERSION,
			 MATECOMPONENT_ACTIVATION_MICRO_VERSION);
	}
	return &module_info;
}

/*****************************************************************************
 * libmate
 *****************************************************************************/

enum { ARG_DISABLE_SOUND = 1, ARG_ENABLE_SOUND, ARG_ESPEAKER, ARG_VERSION };

static char *mate_user_dir = NULL;
static char *mate_user_private_dir = NULL;
static char *mate_user_accels_dir = NULL;

/**
* mate_user_dir_get:
*
* Retrieves the user-specific directory for MATE apps to use ($HOME/.mate2
* is the usual MATE 2 value).
*
* Returns: An absolute path to the directory.
*/
const char *
mate_user_dir_get (void)
{
	return mate_user_dir;
}

/**
* mate_user_private_dir_get:
*
* Differs from mate_user_dir_get() in that the directory returned here will
* have had permissions of 0700 (rwx------) enforced when it was created.  Of
* course, the permissions may have been altered since creation, so care still
* needs to be taken.
*
* Returns: An absolute path to the user-specific private directory that MATE
* apps can use.
*/
const char *
mate_user_private_dir_get (void)
{
	return mate_user_private_dir;
}

/**
* mate_user_accels_dir_get:
*
* Retrieves the user-specific directory that stores the keyboard shortcut files
* for each MATE app. Note that most applications should be using MateConf for
* storing this information, but it may be necessary to use the
* mate_user_accels_dir_get() directory for legacy applications.
*
* Returns: The absolute path to the directory.
*/
const char *
mate_user_accels_dir_get (void)
{
	return mate_user_accels_dir;
}

static void
libmate_option_cb (poptContext ctx, enum poptCallbackReason reason,
		    const struct poptOption *opt, const char *arg,
		    void *data)
{
	MateProgram *program;
	GValue value = { 0 };

	program = mate_program_get ();

	switch(reason) {
	case POPT_CALLBACK_REASON_OPTION:
		switch(opt->val) {
		case ARG_ESPEAKER:
			g_value_init (&value, G_TYPE_STRING);
			g_value_set_string (&value, arg);
			g_object_set_property (G_OBJECT (program),
					       MATE_PARAM_ESPEAKER, &value);
			g_value_unset (&value);
			break;

		case ARG_DISABLE_SOUND:
			g_value_init (&value, G_TYPE_BOOLEAN);
			g_value_set_boolean (&value, FALSE);
			g_object_set_property (G_OBJECT (program),
					       MATE_PARAM_ENABLE_SOUND, &value);
			g_value_unset (&value);
			break;

		case ARG_ENABLE_SOUND:
			g_value_init (&value, G_TYPE_BOOLEAN);
			g_value_set_boolean (&value, TRUE);
			g_object_set_property (G_OBJECT (program),
					       MATE_PARAM_ENABLE_SOUND, &value);
			g_value_unset (&value);
			break;

		case ARG_VERSION:
			g_print ("MATE %s %s\n",
				 mate_program_get_app_id (program),
				 mate_program_get_app_version (program));
			exit(0);
			break;
		}
	default:
		/* do nothing */
		break;
	}
}

static gboolean
libmate_goption_epeaker (const gchar *option_name,
			  const gchar *value,
			  gpointer data,
			  GError **error)
{
	g_object_set (G_OBJECT (mate_program_get ()),
		      MATE_PARAM_ESPEAKER, value, NULL);

	return TRUE;
}

static gboolean
libmate_goption_disable_sound (const gchar *option_name,
				const gchar *value,
				gpointer data,
				GError **error)
{
	g_object_set (G_OBJECT (mate_program_get ()),
		      MATE_PARAM_ENABLE_SOUND, FALSE, NULL);

	return TRUE;
}

static gboolean
libmate_goption_enable_sound (const gchar *option_name,
			       const gchar *value,
			       gpointer data,
			       GError **error)
{
	g_object_set (G_OBJECT (mate_program_get ()),
		      MATE_PARAM_ENABLE_SOUND, TRUE, NULL);

	return TRUE;
}

static void
libmate_goption_version (void)
{
	MateProgram *program;

	program = mate_program_get ();

	g_print ("MATE %s %s\n",
		 mate_program_get_app_id (program),
		 mate_program_get_app_version (program));

	exit (0);
}

static int
safe_mkdir (const char *pathname, mode_t mode)
{
	char *safe_pathname;
	int len, ret;

	safe_pathname = g_strdup (pathname);
	len = strlen (safe_pathname);

	if (len > 1 && G_IS_DIR_SEPARATOR (safe_pathname[len - 1]))
		safe_pathname[len - 1] = '\0';

	ret = g_mkdir (safe_pathname, mode);

	g_free (safe_pathname);

	return ret;
}

static void
libmate_userdir_setup (gboolean create_dirs)
{
	struct stat statbuf;
	
	if(!mate_user_dir) {
                const char *override;

                /* FIXME this env variable should be changed
                 * for each major MATE release, would be easier to
                 * remember if not hardcoded.
                 */
                override = g_getenv ("MATE22_USER_DIR");

                if (override != NULL) {
                        int len;

                        mate_user_dir = g_strdup (override);

                        /* chop trailing slash */
                        len = strlen (mate_user_dir);
                        if (len > 1 && G_IS_DIR_SEPARATOR (mate_user_dir[len - 1]))
                                mate_user_dir[len - 1] = '\0';

                        mate_user_private_dir = g_strconcat (mate_user_dir,
                                                              "_private",
                                                              NULL);
                } else {
                        mate_user_dir = g_build_filename (g_get_home_dir(), MATE_DOT_MATE, NULL);
                        mate_user_private_dir = g_build_filename (g_get_home_dir(),
                                                                   MATE_DOT_MATE_PRIVATE, NULL);
                }

		mate_user_accels_dir = g_build_filename (mate_user_dir,
							  "accels", NULL);
	}

	if (!create_dirs)
		return;

	if (safe_mkdir (mate_user_dir, 0700) < 0) { /* private permissions, but we
							don't check that we got them */
		if (errno != EEXIST) {
			g_printerr (_("Could not create per-user mate configuration directory `%s': %s\n"),
				mate_user_dir, strerror(errno));
			exit(1);
		}
	}

	if (safe_mkdir (mate_user_private_dir, 0700) < 0) { /* This is private
								per-user info mode
								700 will be
								enforced!  maybe
								even other security
								meassures will be
								taken */
		if (errno != EEXIST) {
			g_printerr (_("Could not create per-user mate configuration directory `%s': %s\n"),
				 mate_user_private_dir, strerror(errno));
			exit(1);
		}
	}


       if (g_stat (mate_user_private_dir, &statbuf) < 0) {
               g_printerr (_("Could not stat private per-user mate configuration directory `%s': %s\n"),
                       mate_user_private_dir, strerror(errno));
               exit(1);
       }

	/* change mode to 0700 on the private directory */
	if (((statbuf.st_mode & 0700) != 0700 )  &&
            g_chmod (mate_user_private_dir, 0700) < 0) {
		g_printerr (_("Could not set mode 0700 on private per-user mate configuration directory `%s': %s\n"),
			mate_user_private_dir, strerror(errno));
		exit(1);
	}

	if (safe_mkdir (mate_user_accels_dir, 0700) < 0) {
		if (errno != EEXIST) {
			g_printerr (_("Could not create mate accelerators directory `%s': %s\n"),
				mate_user_accels_dir, strerror(errno));
			exit(1);
		}
	}
}

static void
libmate_post_args_parse (MateProgram *program,
			  MateModuleInfo *mod_info)
{
        gboolean enable_sound = TRUE, create_dirs = TRUE;
        char *espeaker = NULL;

        g_object_get (program,
                      MATE_PARAM_CREATE_DIRECTORIES, &create_dirs,
                      MATE_PARAM_ENABLE_SOUND, &enable_sound,
                      MATE_PARAM_ESPEAKER, &espeaker,
                      NULL);

        mate_sound_init (espeaker);
        g_free (espeaker);

        _mate_sound_set_enabled (enable_sound);

        libmate_userdir_setup (create_dirs);
}

static void
mate_vfs_post_args_parse (MateProgram *program, MateModuleInfo *mod_info)
{
	mate_vfs_init ();
}

/* No need for this to be public */
static const MateModuleInfo *
mate_vfs_module_info_get (void)
{
	static MateModuleInfo module_info = {
		"mate-vfs", MATEVFSVERSION, N_("MATE Virtual Filesystem"),
		NULL, NULL,
		NULL, mate_vfs_post_args_parse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
	return &module_info;
}

static GOptionGroup *
libmate_module_get_goption_group (void)
{
	GOptionGroup *option_group;
	const GOptionEntry matelib_goptions [] = {
		{ "disable-sound", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  libmate_goption_disable_sound, N_("Disable sound server usage"), NULL },

		{ "enable-sound", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  libmate_goption_enable_sound, N_("Enable sound server usage"), NULL },

		{ "espeaker", '\0',0, G_OPTION_ARG_CALLBACK,
		  libmate_goption_epeaker,
		  N_("Host:port on which the sound server to use is running"),
		  N_("HOSTNAME:PORT") },

		{ "version", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  (GOptionArgFunc) libmate_goption_version, NULL, NULL },

		{ NULL }
	};

	option_group = g_option_group_new ("mate",
					   N_("MATE Library"),
					   N_("Show MATE options"),
					   NULL, NULL);
	g_option_group_set_translation_domain (option_group, GETTEXT_PACKAGE);
	g_option_group_add_entries (option_group, matelib_goptions);

	return option_group;
}

/**
* libmate_module_info_get:
*
* Retrieves the current libmate version and the modules it depends on.
*
* Returns: a new #MateModuleInfo structure describing the version and
* the versions of the dependents.
*/
const MateModuleInfo *
libmate_module_info_get (void)
{
	static const struct poptOption matelib_options [] = {
		{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL},

		{ NULL, '\0', POPT_ARG_CALLBACK, (void *) libmate_option_cb, 0, NULL, NULL},

		{ "disable-sound", '\0', POPT_ARG_NONE,
		  NULL, ARG_DISABLE_SOUND, N_("Disable sound server usage"), NULL},

		{ "enable-sound", '\0', POPT_ARG_NONE,
		  NULL, ARG_ENABLE_SOUND, N_("Enable sound server usage"), NULL},

		{ "espeaker", '\0', POPT_ARG_STRING,
		  NULL, ARG_ESPEAKER, N_("Host:port on which the sound server to use is"
					" running"),
		N_("HOSTNAME:PORT")},

		{ "version", '\0', POPT_ARG_NONE,
		  NULL, ARG_VERSION, VERSION, NULL},

		{ NULL, '\0', 0,
		  NULL, 0 , NULL, NULL}
	};

	static MateModuleInfo module_info = {
		"libmate", VERSION, N_("MATE Library"),
		NULL, NULL,
		NULL, libmate_post_args_parse,
		matelib_options,
		NULL, NULL, NULL,
		libmate_module_get_goption_group
	};
	int i = 0;

	if (module_info.requirements == NULL) {
		static MateModuleRequirement req[4];

		bindtextdomain (GETTEXT_PACKAGE, LIBMATE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
		req[i].required_version = "0.9.1";
		req[i].module_info = mate_matecomponent_activation_module_info_get ();
		i++;

		req[i].required_version = "0.3.0";
		req[i].module_info = mate_vfs_module_info_get ();
		i++;

		req[i].required_version = "1.1.1";
		req[i].module_info = mate_mateconf_module_info_get ();
		i++;

		req[i].required_version = NULL;
		req[i].module_info = NULL;
		i++;

		module_info.requirements = req;
	}

	return &module_info;
}
