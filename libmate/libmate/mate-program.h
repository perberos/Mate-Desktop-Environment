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


/* This module takes care of handling application and library
   initialization and command line parsing */

#ifndef MATE_PROGRAM_H
#define MATE_PROGRAM_H

#include <glib.h>
#include <stdarg.h>
#include <errno.h>

#include <glib-object.h>

#ifndef MATE_DISABLE_DEPRECATED
#include <popt.h>
#endif

G_BEGIN_DECLS

#define MATE_TYPE_PROGRAM (mate_program_get_type())
#define MATE_PROGRAM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), MATE_TYPE_PROGRAM, MateProgram))
#define MATE_PROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), MATE_TYPE_PROGRAM, MateProgramClass))
#define MATE_IS_PROGRAM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), MATE_TYPE_PROGRAM))
#define MATE_IS_PROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MATE_TYPE_PROGRAM))

typedef struct _MateProgram MateProgram;
typedef struct _MateProgramPrivate MateProgramPrivate;
typedef struct _MateProgramClass MateProgramClass;

typedef enum {
	MATE_FILE_DOMAIN_UNKNOWN = 0,

	/* Mate installed files */
	MATE_FILE_DOMAIN_LIBDIR,
	MATE_FILE_DOMAIN_DATADIR,
	MATE_FILE_DOMAIN_SOUND,
	MATE_FILE_DOMAIN_PIXMAP,
	MATE_FILE_DOMAIN_CONFIG,
	MATE_FILE_DOMAIN_HELP,

	/* Application files */
	MATE_FILE_DOMAIN_APP_LIBDIR,
	MATE_FILE_DOMAIN_APP_DATADIR,
	MATE_FILE_DOMAIN_APP_SOUND,
	MATE_FILE_DOMAIN_APP_PIXMAP,
	MATE_FILE_DOMAIN_APP_CONFIG,
	MATE_FILE_DOMAIN_APP_HELP
} MateFileDomain;

struct _MateProgram {
	GObject object;

	MateProgramPrivate* _priv;
};

struct _MateProgramClass {
	GObjectClass object_class;

	/* we may want to add stuff in the future */
	gpointer padding1;
	gpointer padding2;
};

GType mate_program_get_type(void);

MateProgram* mate_program_get(void);

const char* mate_program_get_human_readable_name(MateProgram* program);
const char* mate_program_get_app_id(MateProgram* program);

const char* mate_program_get_app_version(MateProgram* program);

gchar* mate_program_locate_file(MateProgram* program, MateFileDomain domain, const gchar* file_name, gboolean only_if_exists, GSList** ret_locations);

#define MATE_PARAM_NONE NULL
#define MATE_PARAM_GOPTION_CONTEXT "goption-context"
#define MATE_PARAM_CREATE_DIRECTORIES "create-directories"
#define MATE_PARAM_ENABLE_SOUND "enable-sound"
#define MATE_PARAM_ESPEAKER "espeaker"
#define MATE_PARAM_APP_ID "app-id"
#define MATE_PARAM_APP_VERSION "app-version"
#define MATE_PARAM_MATE_PREFIX "mate-prefix"
#define MATE_PARAM_MATE_SYSCONFDIR "mate-sysconfdir"
#define MATE_PARAM_MATE_DATADIR "mate-datadir"
#define MATE_PARAM_MATE_LIBDIR "mate-libdir"
#define MATE_PARAM_APP_PREFIX "app-prefix"
#define MATE_PARAM_APP_SYSCONFDIR "app-sysconfdir"
#define MATE_PARAM_APP_DATADIR  "app-datadir"
#define MATE_PARAM_APP_LIBDIR "app-libdir"
#define MATE_PARAM_HUMAN_READABLE_NAME "human-readable-name"
#define MATE_PARAM_MATE_PATH "mate-path"

#ifndef MATE_DISABLE_DEPRECATED
	#define MATE_PARAM_POPT_TABLE "popt-table"
	#define MATE_PARAM_POPT_FLAGS "popt-flags"
	#define MATE_PARAM_POPT_CONTEXT "popt-context"
#endif

/***** application modules (aka libraries :) ******/
#define MATE_TYPE_MODULE_INFO (mate_module_info_get_type())

GType mate_module_info_get_type(void);

typedef struct _MateModuleInfo MateModuleInfo;
typedef struct _MateModuleRequirement MateModuleRequirement;

struct _MateModuleRequirement {
	const char* required_version;
	const MateModuleInfo* module_info;
};

typedef void (*MateModuleInitHook)(const MateModuleInfo* mod_info);
typedef void (*MateModuleClassInitHook)(MateProgramClass* klass, const MateModuleInfo* mod_info);
typedef void (*MateModuleHook) (MateProgram* program, MateModuleInfo* mod_info);
typedef GOptionGroup* (*MateModuleGetGOptionGroupFunc)(void);

struct _MateModuleInfo {
	const char* name;
	const char* version;
	const char* description;
	MateModuleRequirement* requirements; /* last element has NULL version */

	MateModuleHook instance_init;
	MateModuleHook pre_args_parse, post_args_parse;

	#ifdef MATE_DISABLE_DEPRECATED
		void* _options;
	#else
		struct poptOption* options;
	#endif

	MateModuleInitHook init_pass;

	MateModuleClassInitHook class_init;

	const char* opt_prefix;
	MateModuleGetGOptionGroupFunc get_goption_group_func;
};

/* This function should be called before matelib_preinit() - it's an
 * alternative to the "module" property passed by the app.
 */
void mate_program_module_register(const MateModuleInfo* module_info);

gboolean mate_program_module_registered(const MateModuleInfo* module_info);

const MateModuleInfo* mate_program_module_load(const char* mod_name);

guint mate_program_install_property(MateProgramClass* pclass, GObjectGetPropertyFunc get_fn, GObjectSetPropertyFunc set_fn, GParamSpec* pspec);

#ifndef MATE_DISABLE_DEPRECATED

/*
 * If the application writer wishes to use getopt()-style arg
 * processing, they can do it using a while looped sandwiched between
 * calls to these two functions.
 */
poptContext mate_program_preinit(MateProgram* program, const char* app_id, const char* app_version, int argc, char** argv);

void mate_program_parse_args(MateProgram* program);

void mate_program_postinit(MateProgram* program);

#endif /* MATE_DISABLE_DEPRECATED */

/* If you have your auto* define PREFIX, SYSCONFDIR, DATADIR and LIBDIR,
 * Use this macro in your init code. */
#define MATE_PROGRAM_STANDARD_PROPERTIES   \
	MATE_PARAM_APP_PREFIX, PREFIX,         \
	MATE_PARAM_APP_SYSCONFDIR, SYSCONFDIR, \
	MATE_PARAM_APP_DATADIR, DATADIR,       \
	MATE_PARAM_APP_LIBDIR, LIBDIR

MateProgram* mate_program_init (const char* app_id, const char* app_version, const MateModuleInfo* module_info, int argc, char** argv, const char* first_property_name, ...);

MateProgram* mate_program_initv(GType type, const char* app_id, const char* app_version, const MateModuleInfo* module_info, int argc, char** argv, const char* first_property_name, va_list args);
MateProgram* mate_program_init_paramv(GType type, const char* app_id, const char* app_version, const MateModuleInfo* module_info, int argc, char** argv, guint nparams, GParameter* params);

G_END_DECLS

#endif /* MATE_PROGRAM_H */
