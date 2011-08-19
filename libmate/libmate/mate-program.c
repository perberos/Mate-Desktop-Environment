/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 *               2001 SuSE Linux AG.
 * All rights reserved.
 *
 * This file is part of MATE 2.0.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
  @NOTATION@
 */

#undef TIME_INIT

#define MATE_ACCESSIBILITY_ENV "MATE_ACCESSIBILITY"
#define MATE_ACCESSIBILITY_KEY "/desktop/mate/interface/accessibility"

/* This module takes care of handling application and library
   initialization and command line parsing */

#include <config.h>
#include "mate-macros.h"

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gmodule.h>
#include <locale.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-value.h>
#include <mateconf/mateconf-client.h>

#include <glib/gi18n-lib.h>

#include "mate-program.h"
#include "mate-util.h"
#include "mate-init.h"
#include "mate-url.h"

#include "libmate-private.h"

#ifdef G_OS_WIN32
#define getuid() 42
#define geteuid() getuid()
#define getgid() 42
#define getegid() getgid()
#endif

#define MATE_PROGRAM_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), MATE_TYPE_PROGRAM, MateProgramPrivate))

struct _MateProgramPrivate {
    enum {
	APP_UNINIT=0,
	APP_CREATE_DONE=1,
	APP_PREINIT_DONE=2,
	APP_POSTINIT_DONE=3
    } state;

    /* Construction properties */
    int prop_popt_flags;
    struct poptOptions *prop_popt_table;
    gchar *prop_human_readable_name;
    gchar *prop_mate_prefix;
    gchar *prop_mate_libdir;
    gchar *prop_mate_sysconfdir;
    gchar *prop_mate_datadir;
    gchar *prop_app_prefix;
    gchar *prop_app_libdir;
    gchar *prop_app_sysconfdir;
    gchar *prop_app_datadir;
    gboolean prop_create_directories;
    gboolean prop_enable_sound;
    gchar *prop_espeaker;

    gchar **mate_path;

    /* valid-while: state > APP_CREATE_DONE */
    char *app_id;
    char *app_version;
    char **argv;
    int argc;

    /* valid-while: state >= APP_PREINIT_DONE */
    poptContext arg_context;
    GOptionContext *goption_context;

    /* valid-while: state == APP_PREINIT_DONE */
    GArray *top_options_table;
    GSList *accessibility_modules;
};

enum {
    PROP_0,
    PROP_APP_ID,
    PROP_APP_VERSION,
    PROP_HUMAN_READABLE_NAME,
    PROP_MATE_PATH,
    PROP_MATE_PREFIX,
    PROP_MATE_LIBDIR,
    PROP_MATE_DATADIR,
    PROP_MATE_SYSCONFDIR,
    PROP_APP_PREFIX,
    PROP_APP_LIBDIR,
    PROP_APP_DATADIR,
    PROP_APP_SYSCONFDIR,
    PROP_CREATE_DIRECTORIES,
    PROP_ENABLE_SOUND,
    PROP_ESPEAKER,
    PROP_POPT_TABLE,
    PROP_POPT_FLAGS,
    PROP_POPT_CONTEXT,
    PROP_GOPTION_CONTEXT,
    PROP_LAST
};

static gboolean accessibility_invoke (MateProgram *program, gboolean init);
static void mate_program_finalize      (GObject           *object);

static GQuark quark_get_prop = 0;
static GQuark quark_set_prop = 0;

static GPtrArray *program_modules = NULL;
static GPtrArray *program_module_list = NULL;
static gboolean program_initialized = FALSE;
static MateProgram *global_program = NULL;

static guint last_property_id = PROP_LAST;

#define	PREALLOC_CPARAMS (8)
#define	PREALLOC_MODINFOS (8)

MATE_CLASS_BOILERPLATE (MateProgram, mate_program,
			 GObject, G_TYPE_OBJECT)

static void
global_program_unref (void)
{
    if (global_program) {
        g_object_unref (global_program);
        global_program = NULL;
        program_initialized = FALSE;
    }
}

static void
mate_program_set_property (GObject *object, guint param_id,
			    const GValue *value, GParamSpec *pspec)
{
    MateProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (MATE_IS_PROGRAM (object));

    program = MATE_PROGRAM (object);

    switch (param_id) {
    case PROP_GOPTION_CONTEXT:
	program->_priv->goption_context = g_value_get_pointer (value);
	break;
    case PROP_POPT_TABLE:
	program->_priv->prop_popt_table = g_value_peek_pointer (value);
	break;
    case PROP_POPT_FLAGS:
	program->_priv->prop_popt_flags = g_value_get_int (value);
	break;
    case PROP_HUMAN_READABLE_NAME:
	g_free (program->_priv->prop_human_readable_name);
	program->_priv->prop_human_readable_name = g_value_dup_string (value);
	break;
    case PROP_MATE_PATH:
	if (program->_priv->mate_path) {
	    g_strfreev (program->_priv->mate_path);
	    program->_priv->mate_path = NULL;
	}
	if (g_value_get_string (value))
	    program->_priv->mate_path = g_strsplit
		(g_value_get_string (value), G_SEARCHPATH_SEPARATOR_S, -1);
	break;
    case PROP_MATE_PREFIX:
	g_free (program->_priv->prop_mate_prefix);
	program->_priv->prop_mate_prefix = g_value_dup_string (value);
	break;
    case PROP_MATE_SYSCONFDIR:
	g_free (program->_priv->prop_mate_sysconfdir);
	program->_priv->prop_mate_sysconfdir = g_value_dup_string (value);
	break;
    case PROP_MATE_DATADIR:
	g_free (program->_priv->prop_mate_datadir);
	program->_priv->prop_mate_datadir = g_value_dup_string (value);
	break;
    case PROP_MATE_LIBDIR:
	g_free (program->_priv->prop_mate_libdir);
	program->_priv->prop_mate_libdir = g_value_dup_string (value);
	break;
    case PROP_APP_PREFIX:
	g_free (program->_priv->prop_app_prefix);
	program->_priv->prop_app_prefix = g_value_dup_string (value);
	break;
    case PROP_APP_SYSCONFDIR:
	g_free (program->_priv->prop_app_sysconfdir);
	program->_priv->prop_app_sysconfdir = g_value_dup_string (value);
	break;
    case PROP_APP_DATADIR:
	g_free (program->_priv->prop_app_datadir);
	program->_priv->prop_app_datadir = g_value_dup_string (value);
	break;
    case PROP_APP_LIBDIR:
	g_free (program->_priv->prop_app_libdir);
	program->_priv->prop_app_libdir = g_value_dup_string (value);
	break;
    case PROP_CREATE_DIRECTORIES:
	program->_priv->prop_create_directories = g_value_get_boolean (value);
	break;
    case PROP_ENABLE_SOUND:
	program->_priv->prop_enable_sound = g_value_get_boolean (value);
	break;
    case PROP_ESPEAKER:
	g_free (program->_priv->prop_espeaker);
	program->_priv->prop_espeaker = g_value_dup_string (value);
	break;
    default: {
	    GObjectSetPropertyFunc set_func;

	    set_func = g_param_spec_get_qdata (pspec, quark_set_prop);
	    if (set_func)
		set_func (object, param_id, value, pspec);
	    else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);

	    break;
	}
    }
}

static void
mate_program_get_property (GObject *object, guint param_id, GValue *value,
			    GParamSpec *pspec)
{
    MateProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (MATE_IS_PROGRAM (object));

    program = MATE_PROGRAM (object);

    switch (param_id) {
    case PROP_APP_ID:
	g_value_set_string (value, program->_priv->app_id);
	break;
    case PROP_APP_VERSION:
	g_value_set_string (value, program->_priv->app_version);
	break;
    case PROP_HUMAN_READABLE_NAME:
	g_value_set_string (value, program->_priv->prop_human_readable_name);
	break;
    case PROP_POPT_CONTEXT:
	g_value_set_pointer (value, program->_priv->arg_context);
	break;
    case PROP_GOPTION_CONTEXT:
	g_value_set_pointer (value, program->_priv->goption_context);
	break;
    case PROP_MATE_PATH:
	if (program->_priv->mate_path)
	    g_value_take_string (value, g_strjoinv (G_SEARCHPATH_SEPARATOR_S, program->_priv->mate_path));
	else
	    g_value_set_string (value, NULL);
	break;
    case PROP_MATE_PREFIX:
	g_value_set_string (value, program->_priv->prop_mate_prefix);
	break;
    case PROP_MATE_SYSCONFDIR:
	g_value_set_string (value, program->_priv->prop_mate_sysconfdir);
	break;
    case PROP_MATE_DATADIR:
	g_value_set_string (value, program->_priv->prop_mate_datadir);
	break;
    case PROP_MATE_LIBDIR:
	g_value_set_string (value, program->_priv->prop_mate_libdir);
	break;
    case PROP_APP_PREFIX:
	g_value_set_string (value, program->_priv->prop_app_prefix);
	break;
    case PROP_APP_SYSCONFDIR:
	g_value_set_string (value, program->_priv->prop_app_sysconfdir);
	break;
    case PROP_APP_DATADIR:
	g_value_set_string (value, program->_priv->prop_app_datadir);
	break;
    case PROP_APP_LIBDIR:
	g_value_set_string (value, program->_priv->prop_app_libdir);
	break;
    case PROP_CREATE_DIRECTORIES:
	g_value_set_boolean (value, program->_priv->prop_create_directories);
	break;
    case PROP_ENABLE_SOUND:
	g_value_set_boolean (value, program->_priv->prop_enable_sound);
	break;
    case PROP_ESPEAKER:
	g_value_set_string (value, program->_priv->prop_espeaker);
	break;
    default: {
	    GObjectSetPropertyFunc get_func;

	    get_func = g_param_spec_get_qdata (pspec, quark_get_prop);
	    if (get_func)
		get_func (object, param_id, value, pspec);
	    else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);

	    break;
	}
    }
}

static void
add_to_module_list (GPtrArray *module_list, const gchar *module_name)
{
    char **modnames;
    int i, j;

    if (!module_name)
	return;

    modnames = g_strsplit (module_name, ",", -1);
    for (i = 0; modnames && modnames[i]; i++) {
	for (j = 0; j < module_list->len; j++)
	    if (strcmp (modnames[i], (char *) g_ptr_array_index (module_list, j)) == 0)
		break;

	g_ptr_array_add (module_list, g_strdup (modnames[i]));
    }
    g_strfreev (modnames);
}

static int
find_module_in_array (const MateModuleInfo *ptr, MateModuleInfo **array)
{
    int i;

    for (i = 0; array[i] && array[i] != ptr; i++) {
	if (array[i] == ptr)
	    break;
    }

    if (array[i])
	return i;
    else
	return -1;
}

static void /* recursive */
mate_program_module_addtolist (MateModuleInfo **new_list,
				int *times_visited,
				int *num_items_used,
				int new_item_idx)
{
    MateModuleInfo *new_item;
    int i;

    g_assert (new_item_idx >= 0);

    new_item = g_ptr_array_index (program_modules, new_item_idx);
    if(!new_item)
	return;

    if (find_module_in_array (new_item, new_list) >= 0)
	return; /* already cared for */

    /* Does this item have any dependencies? */
    if (times_visited[new_item_idx] > 0) {
	/* We already tried to satisfy all the dependencies for this module,
	 *  and we've come back to it again. There's obviously a loop going on.
	 */
	g_error ("Module '%s' version '%s' has a requirements loop.",
		 new_item->name, new_item->version);
    }
    times_visited[new_item_idx]++;

    if (new_item->requirements) {
	for (i = 0; new_item->requirements[i].required_version; i++) {
	    int n;

	    n = find_module_in_array (new_item->requirements[i].module_info,
				      (MateModuleInfo **)program_modules->pdata);
	    mate_program_module_addtolist
		(new_list, times_visited, num_items_used, n);
	}
    }

    /* now add this module on */
    new_list[*num_items_used] = new_item;
    (*num_items_used)++;
    new_list[*num_items_used] = NULL;
}

static void
mate_program_module_list_order (void)
{
    int i;
    MateModuleInfo **new_list;
    int *times_visited; /* Detects dependency loops */
    int num_items_used;

    new_list = g_alloca (program_modules->len * sizeof(gpointer));
    new_list[0] = NULL;
    num_items_used = 0;

    times_visited = g_alloca (program_modules->len * sizeof(int));
    memset(times_visited, '\0', program_modules->len * sizeof(int));

    /* Create the new list with proper ordering */
    for(i = 0; i < (program_modules->len - 1); i++) {
	mate_program_module_addtolist (new_list, times_visited,
					&num_items_used, i);
    }

    /* Now stick the new, ordered list in place */
    memcpy (program_modules->pdata, new_list,
	    program_modules->len * sizeof(gpointer));
}

static void
mate_program_class_init (MateProgramClass *klass)
{
    GObjectClass *object_class;

    object_class = (GObjectClass*) klass;
    parent_class = g_type_class_peek_parent (klass);

    quark_set_prop = g_quark_from_static_string ("mate-program-set-property");
    quark_get_prop = g_quark_from_static_string ("mate-program-get-property");

    object_class->set_property = mate_program_set_property;
    object_class->get_property = mate_program_get_property;
    object_class->finalize  = mate_program_finalize;

    g_object_class_install_property
	(object_class,
	 PROP_POPT_TABLE,
	 g_param_spec_pointer (MATE_PARAM_POPT_TABLE,
			       _("Popt Table"),
			       _("The table of options for popt"),
			       (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_POPT_FLAGS,
	 g_param_spec_int (MATE_PARAM_POPT_FLAGS,
			   _("Popt Flags"),
			   _("The flags to use for popt"),
			   G_MININT, G_MAXINT, 0,
			   (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_POPT_CONTEXT,
	 g_param_spec_pointer (MATE_PARAM_POPT_CONTEXT,
			      _("Popt Context"),
			      _("The popt context pointer that MateProgram "
				"is using"),
			       (G_PARAM_READABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_GOPTION_CONTEXT,
	 g_param_spec_pointer (MATE_PARAM_GOPTION_CONTEXT,
			       _("GOption Context"),
			       _("The goption context pointer that MateProgram "
				 "is using"),
			       (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_HUMAN_READABLE_NAME,
	 g_param_spec_string (MATE_PARAM_HUMAN_READABLE_NAME,
			      _("Human readable name"),
			      _("Human readable name of this application"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_MATE_PATH,
	 g_param_spec_string (MATE_PARAM_MATE_PATH,
			      _("MATE path"),
			      _("Path in which to look for installed files"),
			      g_getenv ("MATE2_PATH"),
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_ID,
	 g_param_spec_string (MATE_PARAM_APP_ID,
			      _("App ID"),
			      _("ID string to use for this application"),
			      NULL, G_PARAM_READABLE));

    g_object_class_install_property
	(object_class,
	 PROP_APP_VERSION,
	 g_param_spec_string (MATE_PARAM_APP_VERSION,
			      _("App version"),
			      _("Version of this application"),
			      NULL, G_PARAM_READABLE));

    g_object_class_install_property
	(object_class,
	 PROP_MATE_PREFIX,
	 g_param_spec_string (MATE_PARAM_MATE_PREFIX,
			      _("MATE Prefix"),
			      _("Prefix where MATE was installed"),
			      LIBMATE_PREFIX,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_MATE_LIBDIR,
	 g_param_spec_string (MATE_PARAM_MATE_LIBDIR,
			      _("MATE Libdir"),
			      _("Library prefix where MATE was installed"),
			      LIBMATE_LIBDIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_MATE_DATADIR,
	 g_param_spec_string (MATE_PARAM_MATE_DATADIR,
			      _("MATE Datadir"),
			      _("Data prefix where MATE was installed"),
			      LIBMATE_DATADIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_MATE_SYSCONFDIR,
	 g_param_spec_string (MATE_PARAM_MATE_SYSCONFDIR,
			      _("MATE Sysconfdir"),
			      _("Configuration prefix where MATE "
				"was installed"),
			      LIBMATE_SYSCONFDIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_PREFIX,
	 g_param_spec_string (MATE_PARAM_APP_PREFIX,
			      _("MATE App Prefix"),
			      _("Prefix where this application was installed"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_LIBDIR,
	 g_param_spec_string (MATE_PARAM_APP_LIBDIR,
			      _("MATE App Libdir"),
			      _("Library prefix where this application "
				"was installed"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_DATADIR,
	 g_param_spec_string (MATE_PARAM_APP_DATADIR,
			      _("MATE App Datadir"),
			      _("Data prefix where this application "
				"was installed"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_SYSCONFDIR,
	 g_param_spec_string (MATE_PARAM_APP_SYSCONFDIR,
			      _("MATE App Sysconfdir"),
			      _("Configuration prefix where this application "
				"was installed"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_CREATE_DIRECTORIES,
	 g_param_spec_boolean (MATE_PARAM_CREATE_DIRECTORIES,
			      _("Create Directories"),
			      _("Create standard MATE directories on startup"),
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_ENABLE_SOUND,
	 g_param_spec_boolean (MATE_PARAM_ENABLE_SOUND,
			      _("Enable Sound"),
			      _("Enable sound on startup"),
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_ESPEAKER,
	 g_param_spec_string (MATE_PARAM_ESPEAKER,
			      _("Espeaker"),
			      _("How to connect to esd"),
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_type_class_add_private (klass, sizeof (MateProgramPrivate));
}

static void
mate_program_instance_init (MateProgram *program)
{
    guint i;

    program->_priv = MATE_PROGRAM_GET_PRIVATE (program);

    program->_priv->state = APP_CREATE_DONE;

    program->_priv->prop_enable_sound = TRUE;

    for (i = 0; i < program_modules->len; i++) {
	MateModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	if (a_module && a_module->instance_init) {
#ifdef TIME_INIT
	    GTimer *timer = g_timer_new ();
	    g_timer_start (timer);
	    g_print ("Running class_init for: %s ...", a_module->name);
#endif
	    a_module->instance_init (program, a_module);
#ifdef TIME_INIT
	    g_timer_stop (timer);
	    g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
	    g_timer_destroy (timer);
#endif
	}
    }
}

static void
mate_program_finalize (GObject* object)
{
	MateProgram *self = MATE_PROGRAM (object);

	/* no free */
	self->_priv->prop_popt_table = NULL;

	g_free (self->_priv->prop_human_readable_name);
	self->_priv->prop_human_readable_name = NULL;
	g_free (self->_priv->prop_mate_prefix);
	self->_priv->prop_mate_prefix = NULL;
	g_free (self->_priv->prop_mate_libdir);
	self->_priv->prop_mate_libdir = NULL;
	g_free (self->_priv->prop_mate_sysconfdir);
	self->_priv->prop_mate_sysconfdir = NULL;
	g_free (self->_priv->prop_mate_datadir);
	self->_priv->prop_mate_datadir = NULL;
	g_free (self->_priv->prop_app_prefix);
	self->_priv->prop_app_prefix = NULL;
	g_free (self->_priv->prop_app_libdir);
	self->_priv->prop_app_libdir = NULL;
	g_free (self->_priv->prop_app_sysconfdir);
	self->_priv->prop_app_sysconfdir = NULL;
	g_free (self->_priv->prop_app_datadir);
	self->_priv->prop_app_datadir = NULL;
	g_free (self->_priv->prop_espeaker);
	self->_priv->prop_espeaker = NULL;

	g_strfreev (self->_priv->mate_path);
	self->_priv->mate_path = NULL;

	g_free (self->_priv->app_id);
	self->_priv->app_id = NULL;
	g_free (self->_priv->app_version);
	self->_priv->app_version = NULL;

	g_strfreev (self->_priv->argv);
	self->_priv->argv = NULL;

	if (self->_priv->arg_context != NULL) {
		poptFreeContext (self->_priv->arg_context);
		g_dataset_destroy (self->_priv->arg_context);
		self->_priv->arg_context = NULL;
	}

	if (self->_priv->goption_context != NULL) {
		g_option_context_free (self->_priv->goption_context);
		self->_priv->goption_context = NULL;
	}

	if (self->_priv->top_options_table != NULL)
		g_array_free (self->_priv->top_options_table, TRUE);
	self->_priv->top_options_table = NULL;

	g_slist_free (self->_priv->accessibility_modules);

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static gpointer
mate_module_info_copy (gpointer boxed)
{
    return g_memdup (boxed, sizeof (MateModuleInfo));
}

static void
mate_module_info_free (gpointer boxed)
{
    g_free (boxed);
}

GType
mate_module_info_get_type (void)
{
    static GType module_info_type = 0;

    if (!module_info_type)
	module_info_type = g_boxed_type_register_static	("MateModuleInfo",
		 mate_module_info_copy, mate_module_info_free);


    return module_info_type;
}

/**
 * mate_program_get:
 *
 * Retrieves an object that stored information about the application's state.
 * Other functions assume this will always return a #MateProgram object which
 * (if not %NULL) has already been initialized.
 *
 * Returns: The application's #MateProgram instance, or %NULL if it does not
 * exist.
 */

MateProgram *
mate_program_get (void)
{
    return global_program;
}

/**
 * mate_program_get_app_id
 * @program: The program object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as an identifier. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: Application ID string.
 */
const char *
mate_program_get_app_id (MateProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (MATE_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    return program->_priv->app_id;
}

/**
 * mate_program_get_app_version
 * @program: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as a version number. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: Application version string.
 */
const char *
mate_program_get_app_version (MateProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (MATE_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    return program->_priv->app_version;
}

/**
 * mate_program_get_human_readable_name
 * @program: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as a human readable name. The app
 * should provide the name with the #MATE_PARAM_HUMAN_READABLE_NAME
 * init argument. Returns %NULL if no name was set.
 *
 * Returns: Application human-readable name string.
 */
const char *
mate_program_get_human_readable_name (MateProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (MATE_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    if (program->_priv->prop_human_readable_name == NULL)
      return g_get_prgname ();

    return program->_priv->prop_human_readable_name;
}

/**
 * mate_program_install_property:
 * @pclass: A #MateProgramClass.
 * @get_fn: A function to get property values.
 * @set_fn: A function to set property values.
 * @pspec: A collection of properties.
 *
 * Install a collection of available properties, their default values and the
 * functions to set and retrieve these properties.
 *
 * Normal applications will never need to call this function, it is mostly for
 * use by other platform library authors.
 *
 * Returns: The number of properties installed.
 */
guint
mate_program_install_property (MateProgramClass *pclass,
				GObjectGetPropertyFunc get_fn,
				GObjectSetPropertyFunc set_fn,
				GParamSpec *pspec)
{
    g_return_val_if_fail (pclass != NULL, -1);
    g_return_val_if_fail (MATE_IS_PROGRAM_CLASS (pclass), -1);
    g_return_val_if_fail (pspec != NULL, -1);

    g_param_spec_set_qdata (pspec, quark_get_prop, (gpointer)get_fn);
    g_param_spec_set_qdata (pspec, quark_set_prop, (gpointer)set_fn);

    g_object_class_install_property (G_OBJECT_CLASS (pclass),
				     last_property_id, pspec);

    return last_property_id++;
}

/**
 * mate_program_locate_file:
 * @program: A valid #MateProgram object or %NULL (in which case the current
 * application is used).
 * @domain: A #MateFileDomain.
 * @file_name: A file name or path inside the 'domain' to find.
 * @only_if_exists: Only return a full pathname if the specified file
 *                  actually exists
 * @ret_locations: If this is not %NULL, a list of all the possible locations
 *                 of the file will be returned.
 *
 * This function finds a full path to the @file_name located in the specified
 * "domain". A domain is a name for a collection of related files.
 * For example, common domains are "libdir", "pixmap", and "config".
 *
 * If @ret_locations is %NULL, only one pathname is returned. Otherwise,
 * alternative paths are returned in @ret_locations.
 *
 * User applications should store files in the MATE_FILE_DOMAIN_APP_*
 * domains. However you MUST set the correct attributes for #MateProgram for
 * the APP specific prefixes (during the initialization part of the
 * application).
 *
 * The @ret_locations list and its contents should be freed by the caller, as
 * should the returned string.
 *
 * Returns: The full path to the file (if it exists or only_if_exists is
 *          %FALSE) or %NULL.
 */
gchar *
mate_program_locate_file (MateProgram *program, MateFileDomain domain,
			   const gchar *file_name, gboolean only_if_exists,
			   GSList **ret_locations)
{
    gchar *prefix_rel = NULL, *attr_name = NULL, *attr_rel = NULL;
    gchar fnbuf [PATH_MAX], *retval = NULL, **ptr;
    gboolean search_path = TRUE;

    if (program == NULL)
	program = mate_program_get ();

    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (MATE_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);
    g_return_val_if_fail (file_name != NULL, NULL);

#define ADD_FILENAME(x) { \
	if (x != NULL) { \
		if (ret_locations != NULL) \
			*ret_locations = g_slist_append (*ret_locations, g_strdup (x)); \
		if (retval == NULL && ret_locations == NULL) \
			retval = g_strdup (x); \
	} \
}

    /* Potentially add an absolute path */
    if (g_path_is_absolute (file_name))
      {
        if (!only_if_exists || g_file_test (file_name, G_FILE_TEST_EXISTS))
          ADD_FILENAME (file_name);
      }

    switch (domain) {
    case MATE_FILE_DOMAIN_LIBDIR:
	prefix_rel = "/lib";
	attr_name = MATE_PARAM_MATE_LIBDIR;
	attr_rel = "";
	break;
    case MATE_FILE_DOMAIN_DATADIR:
	prefix_rel = "/share";
	attr_name = MATE_PARAM_MATE_DATADIR;
	attr_rel = "";
	break;
    case MATE_FILE_DOMAIN_SOUND:
	prefix_rel = "/share/sounds";
	attr_name = MATE_PARAM_MATE_DATADIR;
	attr_rel = "/sounds";
	break;
    case MATE_FILE_DOMAIN_PIXMAP:
	prefix_rel = "/share/pixmaps";
	attr_name = MATE_PARAM_MATE_DATADIR;
	attr_rel = "/pixmaps";
	break;
    case MATE_FILE_DOMAIN_CONFIG:
	prefix_rel = "/etc";
	attr_name = MATE_PARAM_MATE_SYSCONFDIR;
	attr_rel = "";
	break;
    case MATE_FILE_DOMAIN_HELP:
	prefix_rel = "/share/mate/help";
	attr_name = MATE_PARAM_MATE_DATADIR;
	attr_rel = "/mate/help";
	break;
    case MATE_FILE_DOMAIN_APP_LIBDIR:
	prefix_rel = "/lib";
	attr_name = MATE_PARAM_APP_LIBDIR;
	attr_rel = "";
	search_path = FALSE;
	break;
    case MATE_FILE_DOMAIN_APP_DATADIR:
	prefix_rel = "/share";
	attr_name = MATE_PARAM_APP_DATADIR;
	attr_rel = "";
	search_path = FALSE;
	break;
    case MATE_FILE_DOMAIN_APP_SOUND:
	prefix_rel = "/share/sounds";
	attr_name = MATE_PARAM_APP_DATADIR;
	attr_rel = "/sounds";
	search_path = FALSE;
	break;
    case MATE_FILE_DOMAIN_APP_PIXMAP:
	prefix_rel = "/share/pixmaps";
	attr_name = MATE_PARAM_APP_DATADIR;
	attr_rel = "/pixmaps";
	search_path = FALSE;
	break;
    case MATE_FILE_DOMAIN_APP_CONFIG:
	prefix_rel = "/etc";
	attr_name = MATE_PARAM_APP_SYSCONFDIR;
	attr_rel = "";
	search_path = FALSE;
	break;
    case MATE_FILE_DOMAIN_APP_HELP:
	prefix_rel = "/share/mate/help";
	attr_name = MATE_PARAM_APP_DATADIR;
	attr_rel = "/mate/help";
	search_path = FALSE;
	break;
    default:
	g_warning (G_STRLOC ": unknown file domain %u", domain);
	return NULL;
    }

    if (attr_name != NULL) {
	gchar *dir;

	g_object_get (G_OBJECT (program), attr_name, &dir, NULL);

	/* use the prefix */
	if (dir == NULL) {
		g_warning (G_STRLOC ": Directory properties not set correctly.  "
			   "Cannot locate application specific files.");
		return NULL;
	}

	if (dir != NULL) {
	    g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s",
			dir, attr_rel, file_name);

	    g_free (dir);
	    if (!only_if_exists || g_file_test (fnbuf, G_FILE_TEST_EXISTS))
		ADD_FILENAME (fnbuf);
	}
    }
    if (retval != NULL && ret_locations == NULL)
	goto out;

    /* Now check the MATE_PATH. */
    for (ptr = program->_priv->mate_path; search_path && ptr && *ptr; ptr++) {
	g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s",
		    *ptr, prefix_rel, file_name);

	if (!only_if_exists || g_file_test (fnbuf, G_FILE_TEST_EXISTS))
	    ADD_FILENAME (fnbuf);
    }
    if (retval && !ret_locations)
	goto out;

#undef ADD_FILENAME

 out:
    return retval;
}

/******** modules *******/

/* Stolen verbatim from rpm/lib/misc.c
   RPM is Copyright (c) 1998 by Red Hat Software, Inc.,
   and may be distributed under the terms of the GPL and LGPL.
*/
/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
static int rpmvercmp(const char * a, const char * b) {
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;

    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    str1 = g_alloca(strlen(a) + 1);
    str2 = g_alloca(strlen(b) + 1);

    strcpy(str1, a);
    strcpy(str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one && *two) {
	while (*one && !g_ascii_isalnum(*one)) one++;
	while (*two && !g_ascii_isalnum(*two)) two++;

	str1 = one;
	str2 = two;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (g_ascii_isdigit(*str1)) {
	    while (*str1 && g_ascii_isdigit(*str1)) str1++;
	    while (*str2 && g_ascii_isdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && g_ascii_isalpha(*str1)) str1++;
	    while (*str2 && g_ascii_isalpha(*str2)) str2++;
	    isnum = 0;
	}

	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';

	/* take care of the case where the two version segments are */
	/* different types: one numeric and one alpha */
	if (one == str1) return -1;	/* arbitrary */
	if (two == str2) return -1;

	if (isnum) {
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */

	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    if (strlen(one) > strlen(two)) return 1;
	    if (strlen(two) > strlen(one)) return -1;
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) return rc;

	/* restore character that was replaced by null above */
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}

static gboolean
mate_program_version_check (const char *required_version,
			     const char *provided_version)
{
    if (required_version && provided_version)
	return (rpmvercmp (provided_version, required_version) >= 0);
    else
	return TRUE;
}

/**
 * mate_program_module_registered:
 * @module_info: A pointer to a MateModuleInfo structure describing the module
 *               to be queried
 *
 * Description: This method checks to see whether a specific module has been
 *              initialized in the specified program.
 *
 * Returns: A value indicating whether the specified module has been
 *          registered/initialized in the current program
 */
gboolean
mate_program_module_registered (const MateModuleInfo *module_info)
{
    int i;
    MateModuleInfo *curmod;

    g_return_val_if_fail (module_info, FALSE);

    if (!program_modules)
	    return FALSE;

    for(i = 0; i < program_modules->len; i++) {
	curmod = g_ptr_array_index (program_modules, i);

	/* array is NULL-terminated, so break on NULL */
	if (curmod == NULL)
		break;

	if (curmod == module_info)
	    return TRUE;
    }

    return FALSE;
}

/**
 * mate_program_module_register:
 * @module_info: A pointer to a MateModuleInfo structure describing the module
 *               to be initialized
 *
 * Description:
 * This function is used to register a module to be initialized by the
 * MATE library framework. The memory pointed to by @module_info must be
 * valid during the whole application initialization process, and the module
 * described by @module_info must only use the @module_info pointer to
 * register itself.
 *
 */
void
mate_program_module_register (const MateModuleInfo *module_info)
{
    int i;

    g_return_if_fail (module_info);

    if (program_initialized) {
	g_warning (G_STRLOC ": cannot load modules after program is initialized");
	return;
    }

    /* Check that it's not already registered. */

    if (mate_program_module_registered (module_info))
	return;

    if (!program_modules)
	program_modules = g_ptr_array_new();

    /* if the last entry is NULL, stick it there instead */
    if (program_modules->len > 0 &&
	g_ptr_array_index (program_modules, program_modules->len - 1) == NULL) {
	    g_ptr_array_index (program_modules, program_modules->len - 1) =
		    (MateModuleInfo *)module_info;
    } else {
	    g_ptr_array_add (program_modules, (MateModuleInfo *)module_info);
    }
    /* keep array NULL terminated */
    g_ptr_array_add (program_modules, NULL);

    /* We register requirements *after* the module itself to avoid loops.
       Initialization order gets sorted out later on. */
    if (module_info->requirements) {
	for(i = 0; module_info->requirements[i].required_version; i++) {
	    const MateModuleInfo *dep_mod;

	    dep_mod = module_info->requirements[i].module_info;
	    if (mate_program_version_check (module_info->requirements[i].required_version,
					     dep_mod->version))
		mate_program_module_register (dep_mod);
	    else
		/* The required version is not installed */
		/* I18N needed */
		g_error ("Module '%s' requires version '%s' of module '%s' "
			 "to be installed, and you only have version '%s' of '%s'. "
			 "Aborting application.",
			 module_info->name,
			 module_info->requirements[i].required_version,
			 dep_mod->name,
			 dep_mod->version,
			 dep_mod->name);
	}
    }
}

static void
set_context_data (poptContext con,
		  enum poptCallbackReason reason,
		  const struct poptOption * opt,
		  const char * arg, void * data)
{
	MateProgram *program = data;

	if (reason == POPT_CALLBACK_REASON_PRE)
		g_dataset_set_data (con, "MateProgram", program);
}


/**
 * mate_program_preinit:
 * @program: Application object
 * @app_id: application ID string
 * @app_version: application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 *
 * Description:
 * This function performs the portion of application initialization that
 * needs to be done prior to command line argument parsing. The poptContext
 * returned can be used for getopt()-style option processing.
 *
 * Returns: A poptContext representing the argument parsing state,
 * or %NULL if using GOption argument parsing.
 */
poptContext
mate_program_preinit (MateProgram *program,
		       const char *app_id, const char *app_version,
		       int argc, char **argv)
{
    MateModuleInfo *a_module;
    poptContext argctx = NULL;
    int i;
    char *prgname;

    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (MATE_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (argv != NULL, NULL);

    if (program->_priv->state != APP_CREATE_DONE)
	return NULL;

    /* Store invocation name */
    prgname = g_path_get_basename (argv[0]);
    g_set_prgname (prgname);
    g_free (prgname);

    /* 0. Misc setup */
    g_free (program->_priv->app_id);
    program->_priv->app_id = g_strdup (app_id);
    g_free (program->_priv->app_version);
    program->_priv->app_version = g_strdup (app_version);
    program->_priv->argc = argc;

    /* Make a copy of argv, the thing is that while we know the
     * original argv will live until the end of the program,
     * there are those evil people out there that modify it.
     * Also, this may be some other argv, think 'fake argv' here */
    program->_priv->argv = g_new (char *, argc + 1);
    for (i = 0; i < argc; i++)
        program->_priv->argv[i] = g_strdup (argv[i]);
    program->_priv->argv[argc] = NULL;

    if (!program_modules) {
	program_modules = g_ptr_array_new();
	/* keep array NULL terminated */
	g_ptr_array_add (program_modules, NULL);
    }

    /* Major steps in this function:
       1. Process all framework attributes in 'attrs'
       2. Order the module list for dependencies
       3. Call the preinit functions for the modules
       4. Process other attributes
       5a. Add the modules' GOptionGroup:s to our context, or
       5b. Create a top-level 'struct poptOption *' for use in arg-parsing.
       6. Create a poptContext
       7. Cleanup/return
    */

    /* 3. call the pre-init functions */
    for (i = 0; (a_module = g_ptr_array_index (program_modules, i)); i++) {
	if (a_module->pre_args_parse) {
#ifdef TIME_INIT
	    GTimer *timer = g_timer_new ();
	    g_timer_start (timer);
	    g_print ("Running pre_args_parse for: %s ...", a_module->name);
#endif
	    a_module->pre_args_parse (program, a_module);
#ifdef TIME_INIT
	    g_timer_stop (timer);
	    g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
	    g_timer_destroy (timer);
#endif
	}
    }

    if (program->_priv->goption_context) {
        /* 5a. Add the modules' GOptionGroup:s to our context */

	for (i = 0; (a_module = g_ptr_array_index (program_modules, i)); i++) {
		if (a_module->get_goption_group_func) {
			g_option_context_add_group (program->_priv->goption_context,
						    a_module->get_goption_group_func ());
		}
	}

    } else {
        /* 5b. Create a top-level 'struct poptOption for use in arg-parsing. */

	struct poptOption includer = {NULL, '\0', POPT_ARG_INCLUDE_TABLE,
				      NULL, 0, NULL, NULL};
	struct poptOption callback =
		{ NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_PRE,
		  &set_context_data, 0,
		  /* GOD THIS IS EVIL!  But popt is so UTERLY FUCKED IT'S
		   * NOT EVEN FUNNY.  For some reason it passes 'descrip'
		   * as 'data' to the callback, and there is no other way
		   * to pass data.  Fun, eh? */
		  NULL, NULL };

	callback.descrip = (const char *)program;

	program->_priv->top_options_table = g_array_new
	    (TRUE, TRUE, sizeof (struct poptOption));

	g_array_append_val (program->_priv->top_options_table, callback);

	/* Put the special popt table in first */
	includer.arg = poptHelpOptions;
	includer.descrip = _("Help options");
	g_array_append_val (program->_priv->top_options_table, includer);

	if (program->_priv->prop_popt_table) {
	    includer.arg = program->_priv->prop_popt_table;
	    includer.descrip = _("Application options");
	    g_array_append_val (program->_priv->top_options_table,
				includer);
	}

	for (i = 0; (a_module = g_ptr_array_index(program_modules, i)); i++) {
	    if (a_module->options) {
		includer.arg = a_module->options;
		includer.descrip = (char *)a_module->description;

		g_array_append_val (program->_priv->top_options_table, includer);
	    }
	}

	includer.longName = "load-modules";
	includer.argInfo = POPT_ARG_STRING;
	includer.descrip = _("Dynamic modules to load");
	includer.argDescrip = _("MODULE1,MODULE2,...");
	g_array_append_val (program->_priv->top_options_table, includer);

    	argctx = program->_priv->arg_context = poptGetContext
		(program->_priv->app_id, argc, (const char **) argv,
		 (struct poptOption *) program->_priv->top_options_table->data,
		 program->_priv->prop_popt_flags);
    }
    /* 7. Cleanup/return */
    program->_priv->state = APP_PREINIT_DONE;

    return argctx;
}

/**
 * split_file_list:
 * @str: a %G_SEARCHPATH_SEPARATOR separated list of filenames
 *
 * Splits a %G_SEARCHPATH_SEPARATOR-separated list of files, stripping
 * white space and substituting ~/ with $HOME/.
 *
 * Stolen from pango.
 *
 * Return value: a list of strings to be freed with g_strfreev()
 **/
static char **
split_file_list (const char *str)
{
  int i = 0;
  int j;
  char **files;

  files = g_strsplit (str, G_SEARCHPATH_SEPARATOR_S, -1);

  while (files[i])
    {
      char *file;
      
      file = g_strdup (files[i]);
      g_strstrip (file);

      /* If the resulting file is empty, skip it */
      if (file[0] == '\0')
	{
	  g_free(file);
	  g_free (files[i]);

	  for (j = i + 1; files[j]; j++)
	    files[j - 1] = files[j];

	  files[j - 1] = NULL;

	  continue;
	}
#ifndef G_OS_WIN32
      /* '~' is a quite normal and common character in file names on
       * Windows, especially in the 8.3 versions of long file names, which
       * still occur now and then. Also, few Windows user are aware of the
       * Unix shell convention that '~' stands for the home directory,
       * even if they happen to have a home directory.
       */
      if (file[0] == '~' && file[1] == G_DIR_SEPARATOR)
	{
	  char *tmp = g_strconcat (g_get_home_dir(), file + 1, NULL);
	  g_free (file);
	  file = tmp;
	}
      else if (file[0] == '~' && file[1] == '\0')
	{
	  g_free (file);
	  file = g_strdup (g_get_home_dir());
	}
#endif
      g_free (files[i]);
      files[i] = file;

      i++;
    }

  return files;
}

static gchar **
get_module_path (void)
{
  const gchar *module_path_env;
  gchar *module_path;
  static gchar **result = NULL;

  if (result)
    return result;

  module_path_env = g_getenv ("MATE_MODULE_PATH");

  if (module_path_env)
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				module_path_env, LIBMATE_LIBDIR, NULL);
  else
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				LIBMATE_LIBDIR, NULL);

  result = split_file_list (module_path);
  g_free (module_path);

  return result;
}

static gchar *
find_module (const gchar *name)
{
  gchar **paths;
  gchar **path;
  gchar *module_name;

  if (g_path_is_absolute (name))
    {
      return g_strdup (name);
    }

  module_name = NULL;
  paths = get_module_path ();
  for (path = paths; *path; path++)
    {
      gchar *tmp_name;

      tmp_name = g_module_build_path (*path, name);
      if (g_file_test (tmp_name, G_FILE_TEST_EXISTS))
        {
          module_name = tmp_name;
          goto found;
        }
      g_free (tmp_name);
    }
found:
  g_strfreev (paths);
  return module_name;
}

/**
 * mate_program_module_load:
 * @mod_name: module name
 *
 * Loads a shared library that contains a
 * #MateModuleInfo dynamic_module_info structure.
 *
 * Returns: The #MateModuleInfo structure that was loaded, or %NULL if the
 * module could not be loaded.
 */
const MateModuleInfo *
mate_program_module_load (const char *mod_name)
{
    GModule *mh;
    const MateModuleInfo *gmi;
    char *full_mod_name;

    g_return_val_if_fail (mod_name != NULL, NULL);

    full_mod_name = find_module (mod_name);
    if (!full_mod_name)
      {
        return NULL;
      }

    mh = g_module_open (full_mod_name, G_MODULE_BIND_LAZY);
    if (!mh)
      {
        return NULL;
      }

    if (g_module_symbol (mh, "dynamic_module_info", (gpointer *)&gmi)) 
      {
	mate_program_module_register (gmi);
	g_module_make_resident (mh);
	return gmi;
      } 
    else 
      {
	g_module_close (mh);
	return NULL;
      }
}

/**
 * mate_program_parse_args:
 * @program: Application object
 *
 * Description: Parses the command line arguments for the application
 */
void
mate_program_parse_args (MateProgram *program)
{
	MateProgramPrivate *priv;

	g_return_if_fail (program != NULL);
	g_return_if_fail (MATE_IS_PROGRAM (program));

	if (program->_priv->state != APP_PREINIT_DONE)
		return;

	priv = program->_priv;
	g_return_if_fail ((priv->arg_context != NULL && priv->goption_context == NULL) ||
			  (priv->arg_context == NULL && priv->goption_context != NULL));

	if (priv->goption_context) {
		GError *error = NULL;
		char **arguments;
		int n_arguments;

		/* We need to free priv->argv in finalize, but g_option_context_parse
		 * modifies the array. So we just pass it like this.
		 */
		arguments = g_memdup (priv->argv, priv->argc * sizeof (char *));
		n_arguments = priv->argc;

		if (!g_option_context_parse (priv->goption_context,
		     			     &n_arguments, &arguments,
					     &error)) {
			/* Translators: the first %s is the error message, 2nd %s the program name */
			g_print(_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
				error->message, program->_priv->argv[0]);
			g_error_free (error);
			g_free (arguments);

			exit (1);
		}

		g_free (arguments);
	} else {
		/* translate popt output by default */
		int nextopt;
		poptContext ctx;
#ifdef ENABLE_NLS
		setlocale (LC_ALL, "");
#endif
		ctx = program->_priv->arg_context;
		while ((nextopt = poptGetNextOpt (ctx)) > 0 || nextopt == POPT_ERROR_BADOPT)
			/* do nothing */ ;

		if (nextopt != -1) {
			g_print ("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n",
			poptBadOption (ctx, 0),
			poptStrerror (nextopt),
			program->_priv->argv[0]);
			exit (1);
		}
	}
}

#ifndef G_OS_WIN32 /* No MATE a11y stuff is implemented for real on Windows anyway yet. */

static char *
find_accessibility_module (MateProgram *program, const char *libname)
{
	char *sub;
	char *path;
	char *fname;
	char *retval;

	fname = g_strconcat (libname, "." G_MODULE_SUFFIX, NULL);
	sub = g_strconcat ("gtk-2.0/modules", G_DIR_SEPARATOR_S, fname, NULL);

	path = mate_program_locate_file (
		program, MATE_FILE_DOMAIN_LIBDIR, sub, TRUE, NULL);

	g_free (sub);

	if (path)
		retval = path;
	else
		retval = mate_program_locate_file (
			program, MATE_FILE_DOMAIN_LIBDIR,
			fname, TRUE, NULL);

	g_free (fname);

	return retval;
}

static gboolean
accessibility_invoke_module (MateProgram *program,
			     const char   *libname,
			     gboolean      init)
{
	GModule    *handle;
	void      (*invoke_fn) (void);
	const char *method;
	gboolean    retval = FALSE;
	char       *module_name;

	if (init)
		method = "mate_accessibility_module_init";
	else
		method = "mate_accessibility_module_shutdown";

	module_name = find_accessibility_module (program, libname);

	if (!module_name) {
		g_warning ("Accessibility: failed to find module '%s' which "
			   "is needed to make this application accessible",
			   libname);

	} else if (!(handle = g_module_open (module_name, G_MODULE_BIND_LAZY))) {
		g_warning ("Accessibility: failed to load module '%s': '%s'",
			   libname, g_module_error ());

	} else if (!g_module_symbol (handle, method, (gpointer *)&invoke_fn)) {
		g_warning ("Accessibility: error library '%s' does not include "
			   "method '%s' required for accessibility support",
			   libname, method);
		g_module_close (handle);

	} else {
		retval = TRUE;
		invoke_fn ();
	}

	g_free (module_name);

	return retval;
}

static gboolean
accessibility_invoke (MateProgram *program, gboolean init)
{
	GSList *l;
	gboolean use_gui = FALSE;

	if (!program->_priv->accessibility_modules)
		return FALSE;

	for (l = program->_priv->accessibility_modules; l; l = l->next) {
		MateModuleInfo *module = l->data;

		if (!strcmp (module->name, "gtk")) {
			accessibility_invoke_module (program, "libgail", init);
			use_gui = TRUE;

		} else if (!strcmp (module->name, "libmateui")) {
			accessibility_invoke_module (program, "libgail-mate", init);
			use_gui = TRUE;
		}
	}

	if (use_gui) {
		accessibility_invoke_module (program, "libatk-bridge", init);
	}

	return TRUE;
}

#endif

static void
accessibility_init (MateProgram *program)
{
#ifndef G_OS_WIN32
	int i;
	gboolean do_init;
	const char *env_var;
	GSList *list = NULL;

	/* Seek the module list we need */
	for (i = 0; i < program_modules->len; i++) {
		MateModuleInfo *module = g_ptr_array_index (program_modules, i);

		if (!module)
			continue;

		if (!strcmp (module->name, "gtk"))
			list = g_slist_prepend (list, module);

		else if (!strcmp (module->name, "libmateui"))
			list = g_slist_prepend (list, module);
	}

	program->_priv->accessibility_modules = list;

	do_init = FALSE;

	if ((env_var = g_getenv (MATE_ACCESSIBILITY_ENV)))
		do_init = atoi (env_var);
	else {
		MateConfClient* gc = mateconf_client_get_default ();
		do_init = mateconf_client_get_bool (
			gc, MATE_ACCESSIBILITY_KEY, NULL);
		g_object_unref (gc);
	}

	if (do_init)
		accessibility_invoke (program, TRUE);
#endif
}

/**
 * mate_program_postinit:
 * @program: Application object
 *
 * Description: Called after mate_program_parse_args(), this function
 * takes care of post-parse initialization and cleanup
 */
void
mate_program_postinit (MateProgram *program)
{
    int i;
    MateModuleInfo *a_module;

    g_return_if_fail (program != NULL);
    g_return_if_fail (MATE_IS_PROGRAM (program));

    if (program->_priv->state != APP_PREINIT_DONE)
	return;

    /* Call post-parse functions */
    for (i = 0; (a_module = g_ptr_array_index(program_modules, i)); i++) {
	if (a_module->post_args_parse) {
#ifdef TIME_INIT
	    GTimer *timer = g_timer_new ();
	    g_timer_start (timer);
	    g_print ("Running post_args_parse for: %s ...", a_module->name);
#endif
	    a_module->post_args_parse (program, a_module);
#ifdef TIME_INIT
	    g_timer_stop (timer);
	    g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
	    g_timer_destroy (timer);
#endif
	}
    }

    /* Accessibility magic */
    accessibility_init (program);

    program->_priv->state = APP_POSTINIT_DONE;
}

/**
 * mate_program_init:
 * @app_id: Application ID string.
 * @app_version: Application version string.
 * @module_info: The module to init with this program.
 * @argc: The number of commmand line arguments contained in @argv.
 * @argv: A string array of command line arguments.
 * @first_property_name: The first item in a %NULL-terminated list of attribute
 * name and value pairs (so this will be an attribute name or %NULL).
 * @...: The continuation of a %NULL-terminated list of attribute name/value
 * pairs.
 *
 * Initialises the current MATE libraries for use by the application.
 * @app_id is used for the following purposes:
 * - to find the programme's help files by mate_help_*()
 * - to load the app-specific gtkrc file from ~/.mate2/$(APPID)rc
 * - to load/save the app's accelerators map from ~/.mate2/accelerators/$(APPID)
 * - to load/save a MateEntry's history from mateconf/apps/mate-settings/$(APPID)/history-$(ENTRYID)
 *
 * Returns: A new #MateProgram instance representing the current application.
 * Unref the returned reference right before exiting your application.
 */
MateProgram *
mate_program_init (const char *app_id, const char *app_version,
		    const MateModuleInfo *module_info,
		    int argc, char **argv,
		    const char *first_property_name, ...)
{
    MateProgram *program;
    va_list args;

    /* g_thread_init() has to be the first GLib function called ever */
    if (!g_threads_got_initialized)
        g_thread_init (NULL);

    g_type_init ();

    va_start(args, first_property_name);
    program = mate_program_initv (MATE_TYPE_PROGRAM,
				   app_id, app_version, module_info,
				   argc, argv, first_property_name, args);
    va_end(args);

    return program;
}


static MateProgram*
mate_program_init_common (GType type,
			   const char *app_id, const char *app_version,
			   const MateModuleInfo *module_info,
			   int argc, char **argv,
			   const char *first_property_name, va_list args,
			   gint nparams, GParameter *params)
{
    MateProgram *program;
    MateProgramClass *klass;
    int i;

#ifdef TIME_INIT
    GTimer *global_timer = g_timer_new ();

    g_timer_start (global_timer);
    g_print ("Starting mate_program_init:\n\n");
#endif

    g_type_init ();

    klass = g_type_class_ref (type);

    if (!program_initialized) {
	const char *ctmp;
	const MateModuleInfo *libmate_module;

	if (!program_module_list)
	    program_module_list = g_ptr_array_new ();

	if (!program_modules) {
	    program_modules = g_ptr_array_new ();
	      /* keep array NULL terminated */
	    g_ptr_array_add (program_modules, NULL);
	}
	/* Register the requested modules. */
	mate_program_module_register (module_info);

	/*
	 * make sure libmate is always registered.
	 */
	libmate_module = libmate_module_info_get ();
	if (!mate_program_module_registered (libmate_module))
		mate_program_module_register (libmate_module);

	/* Only load shlib modules and do all that other good
	 * stuff when not setuid/setgid, for obvious reasons */
	if (geteuid () == getuid () &&
	    getegid () == getgid ()) {
	    /* We have to handle --load-modules=foo,bar,baz specially */
	    for (i = 0; i < argc; i++) {
	        /* the --foo=bar format */
	        if (strncmp (argv[i], "--load-modules=", strlen ("--load-modules=")) == 0)
		    add_to_module_list (program_module_list, argv[i] + strlen("--load-modules="));
	        /* the --foo bar format */
	        if (strcmp (argv[i], "--load-modules") == 0 && i+1 < argc)
		    add_to_module_list (program_module_list, argv[i+1]);
	    }

	    ctmp = g_getenv ("MATE_MODULES");
	    if (ctmp != NULL)
	        add_to_module_list (program_module_list, ctmp);
	}

	/*
	 * Load all the modules.
	 */
	for (i = 0; i < program_module_list->len; i++) {
	    gchar *modname = g_ptr_array_index (program_module_list, i);

	    mate_program_module_load (modname);
	}

	for (i = 0; i < program_modules->len; i++) {
	    MateModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->init_pass) {
#ifdef TIME_INIT
	        GTimer *timer = g_timer_new ();
	        g_timer_start (timer);
	        g_print ("Running init_pass for: %s ...", a_module->name);
#endif
		a_module->init_pass (a_module);
#ifdef TIME_INIT
		g_timer_stop (timer);
		g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
#endif
	    }
	}

	/* Order the module list for dependencies */
	mate_program_module_list_order ();

	for (i = 0; i < program_modules->len; i++) {
	    MateModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->class_init) {
#ifdef TIME_INIT
	        GTimer *timer = g_timer_new ();
	        g_timer_start (timer);
	        g_print ("Running class_init for: %s ...", a_module->name);
#endif
		a_module->class_init (klass, a_module);
#ifdef TIME_INIT
		g_timer_stop (timer);
		g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
#endif
	    }
	}
    } else if ( ! mate_program_module_registered (module_info)) {
	/* Register the requested modules. */
	mate_program_module_register (module_info);

	/* Init ALL modules, note that this runs the init over ALL modules
	 * even old ones.  Not really desirable, but unavoidable right now */
	for (i = 0; i < program_modules->len; i++) {
	    MateModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->init_pass) {
#ifdef TIME_INIT
	        GTimer *timer = g_timer_new ();
	        g_timer_start (timer);
	        g_print ("Running init_pass for: %s ...", a_module->name);
#endif
		a_module->init_pass (a_module);
#ifdef TIME_INIT
		g_timer_stop (timer);
		g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
#endif
	    }
	}

	/* Order the module list for dependencies */
	mate_program_module_list_order ();

	/* Same deal as for init_pass */
	for (i = 0; i < program_modules->len; i++) {
	    MateModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->class_init) {
#ifdef TIME_INIT
	        GTimer *timer = g_timer_new ();
	        g_timer_start (timer);
	        g_print ("Running class_init for: %s ...", a_module->name);
#endif
		a_module->class_init (klass, a_module);
#ifdef TIME_INIT
		g_timer_stop (timer);
		g_print ("done (%f seconds)\n", g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
#endif
	    }
	}
    }

    if (nparams == -1)
        program = (MateProgram *) g_object_new_valist (type,
                                                        first_property_name, args);
    else
        program = (MateProgram *) g_object_newv (type, nparams, params);

    if (!program_initialized) {
	global_program = program;
	g_object_ref (G_OBJECT (global_program));

	program_initialized = TRUE;

	g_atexit (global_program_unref);
    }

    mate_program_preinit (program, app_id, app_version, argc, argv);
    mate_program_parse_args (program);
    mate_program_postinit (program);

#ifdef TIME_INIT
    g_timer_stop (global_timer);
    g_print ("\nGlobal init done in: %f seconds\n\n", g_timer_elapsed (global_timer, NULL));
    g_timer_destroy (global_timer);
#endif

    return program;
}

/**
 * mate_program_initv:
 * @type: The type of application to be initialized (usually
 * #MATE_TYPE_PROGRAM).
 * @app_id: Application ID string.
 * @app_version: Application version string.
 * @module_info: The modules to init with the application.
 * @argc: The number of command line arguments contained in @argv.
 * @argv: A string array of command line arguments.
 * @first_property_name: The first item in a %NULL-terminated list of attribute
 * name/value.
 * @args: The remaining elements in the %NULL terminated list (of which
 * @first_property_name is the first element).
 *
 * Provides a non-varargs form of mate_program_init(). Users will rarely need
 * to call this function directly.
 *
 * Returns: A #MateProgram instance representing the current application.
 */
MateProgram*
mate_program_initv (GType type,
		     const char *app_id, const char *app_version,
		     const MateModuleInfo *module_info,
		     int argc, char **argv,
		     const char *first_property_name, va_list args)
{
    return mate_program_init_common (type, app_id, app_version, module_info,
				      argc, argv, first_property_name, args,
				      -1, NULL);
}

/**
 * mate_program_init_paramv:
 * @type: The type of application to be initialized (usually
 * #MATE_TYPE_PROGRAM).
 * @app_id: Application ID string.
 * @app_version: Application version string.
 * @module_info: The modules to init with the application.
 * @argc: The number of command line arguments contained in @argv.
 * @argv: A string array of command line arguments.
 * @nparams: Number of parameters.
 * @args: GParameter array.
 *
 * Provides a GParameter form of mate_program_init(). Useful only for
 * language bindings, mostly.
 *
 * Returns: A #MateProgram instance representing the current application.
 *
 * Since: 2.8
 */
MateProgram*
mate_program_init_paramv (GType type,
                           const char *app_id, const char *app_version,
                           const MateModuleInfo *module_info,
                           int argc, char **argv,
                           guint nparams, GParameter *params)
{
    va_list args;

    return mate_program_init_common (type, app_id, app_version, module_info,
				      argc, argv, NULL, args, nparams, params);

}
