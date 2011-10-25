/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
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
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* Blame Elliot for most of this stuff*/

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <glib.h>
#ifndef G_OS_WIN32
#include <sys/wait.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <gdkconfig.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <libmate/libmate.h>
#include <matecomponent/matecomponent-ui-main.h>
#include <mateconf/mateconf-client.h>

#include "mate-client.h"
#include "mate-mateconf-ui.h"
#include "mate-ui-init.h"
#include "mate-stock-icons.h"
#include "mate-url.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libmateuiP.h"

#include <gtk/gtk.h>

/*****************************************************************************
 * libmateui
 *****************************************************************************/

static void libmateui_arg_callback 	(poptContext con,
					 enum poptCallbackReason reason,
					 const struct poptOption * opt,
					 const char * arg,
					 void * data);
static gboolean libmateui_goption_disable_crash_dialog (const gchar *option_name,
							 const gchar *value,
							 gpointer data,
							 GError **error);
static void libmateui_init_pass	(const MateModuleInfo *mod_info);
static void libmateui_class_init	(MateProgramClass *klass,
					 const MateModuleInfo *mod_info);
static void libmateui_instance_init	(MateProgram *program,
					 MateModuleInfo *mod_info);
static void libmateui_post_args_parse	(MateProgram *app,
					 MateModuleInfo *mod_info);
static void libmateui_rc_parse		(MateProgram *program);

/* Prototype for a private mate_stock function */
G_GNUC_INTERNAL void _mate_stock_icons_init (void);

/* Whether to make noises when the user clicks a button, etc.  We cache it
 * in a boolean rather than querying MateConf every time.
 */
static gboolean use_event_sounds;

/* MateConf client for monitoring the event sounds option */
static MateConfClient *mateconf_client = NULL;

enum { ARG_DISABLE_CRASH_DIALOG=1, ARG_DISPLAY };

G_GNUC_INTERNAL void _mate_ui_gettext_init (gboolean);

void G_GNUC_INTERNAL
_mate_ui_gettext_init (gboolean bind_codeset)
{
	static gboolean initialized = FALSE;
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	static gboolean codeset_bound = FALSE;
#endif

	if (!initialized)
	{
		bindtextdomain (GETTEXT_PACKAGE, MATEUILOCALEDIR);
		initialized = TRUE;
	}
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	if (!codeset_bound && bind_codeset)
	{
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
		codeset_bound = TRUE;
	}
#endif
}

static GOptionGroup *
libmateui_get_goption_group (void)
{
	const GOptionEntry libmateui_goptions[] = {
		{ "disable-crash-dialog", '\0', G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
		  G_OPTION_ARG_CALLBACK, libmateui_goption_disable_crash_dialog,
		  N_("Disable Crash Dialog"), NULL },
		  /* --display is already handled by gtk+ */
		{ NULL }
	};
	GOptionGroup *option_group;

	_mate_ui_gettext_init (TRUE);

	option_group = g_option_group_new ("mate-ui",
					   N_("MATE GUI Library:"),
					   N_("Show MATE GUI options"),
					   NULL, NULL);
	g_option_group_set_translation_domain (option_group, GETTEXT_PACKAGE);
	g_option_group_add_entries(option_group, libmateui_goptions);

	return option_group;
}

const MateModuleInfo *
libmateui_module_info_get (void)
{
	static struct poptOption libmateui_options[] = {
		{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL },
		{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
		  &libmateui_arg_callback, 0, NULL, NULL },
		{ "disable-crash-dialog", '\0', POPT_ARG_NONE, NULL, ARG_DISABLE_CRASH_DIALOG,
		  N_("Disable Crash Dialog"), NULL },
		{ "display", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, ARG_DISPLAY,
		  N_("X display to use"), N_("DISPLAY") },
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	static MateModuleInfo module_info = {
		"libmateui", VERSION, N_("MATE GUI Library"),
		NULL, libmateui_instance_init,
		NULL, libmateui_post_args_parse,
		libmateui_options,
		libmateui_init_pass, libmateui_class_init,
		NULL, NULL
	};

	module_info.get_goption_group_func = libmateui_get_goption_group;

	if (module_info.requirements == NULL) {
		static MateModuleRequirement req[6];

		_mate_ui_gettext_init (FALSE);

		req[0].required_version = "1.101.2";
		req[0].module_info = LIBMATECOMPONENTUI_MODULE;

		req[1].required_version = VERSION;
		req[1].module_info = mate_client_module_info_get ();

		req[2].required_version = "1.1.1";
		req[2].module_info = mate_mateconf_ui_module_info_get ();

		req[3].required_version = NULL;
		req[3].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}

typedef struct {
        guint crash_dialog_id;
        guint display_id;
        guint default_icon_id;
} MateProgramClass_libmateui;

typedef struct {
	gchar	*default_icon;
	guint constructed : 1;
	guint show_crash_dialog : 1;
} MateProgramPrivate_libmateui;

static GQuark quark_mate_program_private_libmateui = 0;
static GQuark quark_mate_program_class_libmateui = 0;

#if !GTK_CHECK_VERSION(3, 0, 0)

	static void show_url(GtkWidget* parent, const char* url)
	{
		GdkScreen* screen;
		GError* error = NULL;

		screen = gtk_widget_get_screen (parent);

		if (!mate_url_show_on_screen (url, screen, &error))
		{
			GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", _("Could not open link"));
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", error->message);
			g_error_free(error);

			g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(dialog);
		}
	}

	static void about_url_hook(GtkAboutDialog* about, const char* link, gpointer data)
	{
		show_url(GTK_WIDGET(about), link);
	}

	static void about_email_hook(GtkAboutDialog* about, const char* email, gpointer data)
	{
		/* FIXME: escaping? */
		char* address = g_strdup_printf("mailto:%s", email);
		show_url(GTK_WIDGET(about), address);
		g_free(address);
	}

#endif


static void
libmateui_private_free (MateProgramPrivate_libmateui *priv)
{
	g_free (priv->default_icon);
	g_free (priv);
}

static void
libmateui_get_property (GObject *object, guint param_id, GValue *value,
                         GParamSpec *pspec)
{
        MateProgramClass_libmateui *cdata;
        MateProgramPrivate_libmateui *priv;
        MateProgram *program;

        g_return_if_fail(object != NULL);
        g_return_if_fail(MATE_IS_PROGRAM (object));

        program = MATE_PROGRAM(object);

        cdata = g_type_get_qdata(G_OBJECT_TYPE(program), quark_mate_program_class_libmateui);
        priv = g_object_get_qdata(G_OBJECT(program), quark_mate_program_private_libmateui);

	if (param_id == cdata->default_icon_id)
		g_value_set_string (value, priv->default_icon);
	else if (param_id == cdata->crash_dialog_id)
		g_value_set_boolean (value, priv->show_crash_dialog);
	else if (param_id == cdata->display_id)
		g_value_set_string (value, gdk_get_display_arg_name ());
	else
        	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
}

static void
libmateui_set_property (GObject *object, guint param_id,
                         const GValue *value, GParamSpec *pspec)
{
        MateProgramClass_libmateui *cdata;
        MateProgramPrivate_libmateui *priv;
        MateProgram *program;

        g_return_if_fail(object != NULL);
        g_return_if_fail(MATE_IS_PROGRAM (object));

        program = MATE_PROGRAM(object);

        cdata = g_type_get_qdata(G_OBJECT_TYPE(program), quark_mate_program_class_libmateui);
        priv = g_object_get_qdata(G_OBJECT(program), quark_mate_program_private_libmateui);

	if (param_id == cdata->default_icon_id)
		priv->default_icon = g_value_dup_string (value);
	else if (param_id == cdata->crash_dialog_id)
		priv->show_crash_dialog = g_value_get_boolean (value) != FALSE;
	else if (param_id == cdata->display_id)
		; /* We don't need to store it, we get it from gdk when we need it */
	else
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
}

static void
libmateui_init_pass (const MateModuleInfo *mod_info)
{
    if (!quark_mate_program_private_libmateui)
	quark_mate_program_private_libmateui = g_quark_from_static_string
	    ("mate-program-private:libmateui");

    if (!quark_mate_program_class_libmateui)
	quark_mate_program_class_libmateui = g_quark_from_static_string
	    ("mate-program-class:libmateui");
}

static void
libmateui_class_init (MateProgramClass *klass, const MateModuleInfo *mod_info)
{
        MateProgramClass_libmateui *cdata = g_new0 (MateProgramClass_libmateui, 1);

        g_type_set_qdata (G_OBJECT_CLASS_TYPE (klass), quark_mate_program_class_libmateui, cdata);

        cdata->crash_dialog_id = mate_program_install_property (
                klass,
                libmateui_get_property,
                libmateui_set_property,
                g_param_spec_boolean (LIBMATEUI_PARAM_CRASH_DIALOG, NULL, NULL,
                                      TRUE, (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT)));
        cdata->display_id = mate_program_install_property (
                klass,
                libmateui_get_property,
                libmateui_set_property,
                g_param_spec_string (LIBMATEUI_PARAM_DISPLAY, NULL, NULL,
				     g_getenv ("DISPLAY"),
                                     (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY)));

        cdata->default_icon_id = mate_program_install_property (
                klass,
                libmateui_get_property,
                libmateui_set_property,
                g_param_spec_string (LIBMATEUI_PARAM_DEFAULT_ICON, NULL, NULL, NULL,
                                     (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY)));

	#if !GTK_CHECK_VERSION(3, 0, 0)
		gtk_about_dialog_set_url_hook(about_url_hook, NULL, NULL);
		gtk_about_dialog_set_email_hook(about_email_hook, NULL, NULL);
	#endif
}

static void
libmateui_instance_init (MateProgram *program, MateModuleInfo *mod_info)
{
    MateProgramPrivate_libmateui *priv = g_new0 (MateProgramPrivate_libmateui, 1);

    g_object_set_qdata_full (G_OBJECT (program),
			     quark_mate_program_private_libmateui,
			     priv, (GDestroyNotify) libmateui_private_free);
}

static gboolean
relay_mate_signal (GSignalInvocationHint *hint,
              	     guint n_param_values,
                    const GValue *param_values,
                    gchar *signame)
{
        char *pieces[3] = {"mate-2", NULL, NULL};
        static GQuark disable_sound_quark = 0;
        GObject *object = g_value_get_object (&param_values[0]);

        if (!use_event_sounds)
                return TRUE;

        if (!disable_sound_quark)
                disable_sound_quark = g_quark_from_static_string ("mate_disable_sound_events");

        if (g_object_get_qdata (object, disable_sound_quark))
                return TRUE;

        if (GTK_IS_WIDGET (object)) {

                if (!GTK_WIDGET_DRAWABLE (object))
                        return TRUE;

                if (GTK_IS_MESSAGE_DIALOG (object)) {
                        GtkMessageType message_type;
                        g_object_get (object, "message_type", &message_type, NULL);
                        switch (message_type)
                        {
                                case GTK_MESSAGE_INFO:
                                        pieces[1] = "info";
                                        break;

                                case GTK_MESSAGE_QUESTION:
                                        pieces[1] = "question";
                                        break;

                                case GTK_MESSAGE_WARNING:
                                        pieces[1] = "warning";
                                        break;

                                case GTK_MESSAGE_ERROR:
                                        pieces[1] = "error";
                                        break;

                                default:
                                        pieces[1] = NULL;
                        }
                }
        }

        if (mate_sound_connection_get () < 0)
                return TRUE;

        mate_triggers_vdo ("", NULL, (const char **) pieces);

        return TRUE;
}

static void
initialize_mate_signal_relay (void)
{
        static gboolean initialized = FALSE;
        int signum;

        if (initialized)
                return;

        initialized = TRUE;

        signum = g_signal_lookup ("show", GTK_TYPE_MESSAGE_DIALOG);
        g_signal_add_emission_hook (signum, 0,
                                    (GSignalEmissionHook) relay_mate_signal,
                                    NULL, NULL);

}

static gboolean
relay_gtk_signal(GSignalInvocationHint *hint,
		 guint n_param_values,
		 const GValue *param_values,
		 gchar *signame)
{
  char *pieces[3] = {"gtk-events-2", NULL, NULL};
  static GQuark disable_sound_quark = 0;
  GObject *object = g_value_get_object (&param_values[0]);

  pieces[1] = signame;

  if (!use_event_sounds)
    return TRUE;

  if(!disable_sound_quark)
    disable_sound_quark = g_quark_from_static_string("mate_disable_sound_events");

  if(g_object_get_qdata (object, disable_sound_quark))
    return TRUE;

  if(GTK_IS_WIDGET(object)) {
    if(!GTK_WIDGET_DRAWABLE(object))
      return TRUE;

    if(GTK_IS_MENU_ITEM(object) && GTK_MENU_ITEM(object)->submenu)
      return TRUE;
  }

  if (mate_sound_connection_get() < 0)
    return TRUE;

  mate_triggers_vdo("", NULL, (const char **)pieces);

  return TRUE;
}

static void
initialize_gtk_signal_relay (void)
{
	gpointer iter_signames;
	char *signame;
	char *ctmp, *ctmp2;
        static gboolean initialized = FALSE;

        if (initialized)
                return;

        initialized = TRUE;

	ctmp = mate_config_file ("/sound/events/gtk-events-2.soundlist");
	ctmp2 = g_strconcat ("=", ctmp, "=", NULL);
	g_free (ctmp);
	iter_signames = mate_config_init_iterator_sections (ctmp2);
	mate_config_push_prefix (ctmp2);
	g_free (ctmp2);

	while ((iter_signames = mate_config_iterator_next (iter_signames,
							    &signame, NULL))) {
		int signums [5];
		int nsigs, i;
		gboolean used_signame;

		/*
		 * XXX this is an incredible hack based on a compile-time
		 * knowledge of what gtk widgets do what, rather than
		 * anything based on the info available at runtime.
		 */
		if (!strcmp (signame, "activate")) {
			g_type_class_unref (g_type_class_ref (gtk_menu_item_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_menu_item_get_type ());

			g_type_class_unref (g_type_class_ref (gtk_entry_get_type ()));
			signums [1] = g_signal_lookup (signame, gtk_entry_get_type ());
			nsigs = 2;
		} else if (!strcmp(signame, "toggled")) {
			g_type_class_unref (g_type_class_ref (gtk_toggle_button_get_type ()));
			signums [0] = g_signal_lookup (signame,
							 gtk_toggle_button_get_type ());

			g_type_class_unref (g_type_class_ref (gtk_check_menu_item_get_type ()));
			signums [1] = g_signal_lookup (signame,
							 gtk_check_menu_item_get_type ());
			nsigs = 2;
		} else if (!strcmp (signame, "clicked")) {
			g_type_class_unref (g_type_class_ref (gtk_button_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_button_get_type ());
			nsigs = 1;
		} else {
			g_type_class_unref (g_type_class_ref (gtk_widget_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_widget_get_type ());
			nsigs = 1;
		}

		used_signame = FALSE;
		for (i = 0; i < nsigs; i++) {
			if (signums [i] > 0) {
				g_signal_add_emission_hook (
                                        signums [i], 0,
                                        (GSignalEmissionHook) relay_gtk_signal,
                                        signame, NULL);
                                used_signame = TRUE;
                        }
                }

                if (!used_signame)
                        g_free (signame);
	}
	mate_config_pop_prefix ();
}

/* Callback used when the MateConf event_sounds key's value changes */
static void
event_sounds_changed_cb (MateConfClient* client, guint cnxn_id, MateConfEntry *entry, gpointer data)
{
        gboolean new_use_event_sounds;

        new_use_event_sounds = (mate_mateconf_get_bool ("/desktop/mate/sound/enable_esd") &&
                                mate_mateconf_get_bool ("/desktop/mate/sound/event_sounds"));

        if (new_use_event_sounds && !use_event_sounds) {
                GDK_THREADS_ENTER();
                initialize_gtk_signal_relay ();
                initialize_mate_signal_relay ();
                GDK_THREADS_LEAVE();
	}

        use_event_sounds = new_use_event_sounds;
}

static void
setup_event_listener (void)
{
        mateconf_client = mateconf_client_get_default ();
        if (!mateconf_client)
                return;

        mateconf_client_add_dir (mateconf_client, "/desktop/mate/sound",
                              MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

        mateconf_client_notify_add (mateconf_client, "/desktop/mate/sound/event_sounds",
                                 event_sounds_changed_cb,
                                 NULL, NULL, NULL);
        mateconf_client_notify_add (mateconf_client, "/desktop/mate/sound/enable_esd",
                                 event_sounds_changed_cb,
                                 NULL, NULL, NULL);

        use_event_sounds = (mate_mateconf_get_bool ("/desktop/mate/sound/enable_esd") &&
                            mate_mateconf_get_bool ("/desktop/mate/sound/event_sounds"));

        if (use_event_sounds) {
                initialize_gtk_signal_relay ();
                initialize_mate_signal_relay ();
	 }
}

static void
libmateui_post_args_parse(MateProgram *program, MateModuleInfo *mod_info)
{
        MateProgramPrivate_libmateui *priv;
        gchar *filename;

        mate_type_init();
        libmateui_rc_parse (program);
        priv = g_object_get_qdata(G_OBJECT(program), quark_mate_program_private_libmateui);

        priv->constructed = TRUE;

        /* load the accelerators */
        filename = g_build_filename (mate_user_accels_dir_get (),
                                     mate_program_get_app_id (program),
                                     NULL);
        gtk_accel_map_load (filename);
        g_free (filename);

	_mate_ui_gettext_init (TRUE);

        _mate_stock_icons_init ();

        setup_event_listener ();
}

static void
libmateui_arg_callback(poptContext con,
                        enum poptCallbackReason reason,
                        const struct poptOption * opt,
                        const char * arg, void * data)
{
        MateProgram *program = g_dataset_get_data (con, "MateProgram");
	g_assert (program != NULL);

        switch(reason) {
        case POPT_CALLBACK_REASON_OPTION:
                switch(opt->val) {
                case ARG_DISABLE_CRASH_DIALOG:
                        g_object_set (G_OBJECT (program),
                                      LIBMATEUI_PARAM_CRASH_DIALOG,
                                      FALSE, NULL);
                        break;
                case ARG_DISPLAY:
                        g_object_set (G_OBJECT (program),
                                      LIBMATEUI_PARAM_DISPLAY,
                                      arg, NULL);
                        break;
                }
                break;
        default:
                break;
        }
}

static gboolean
libmateui_goption_disable_crash_dialog (const gchar *option_name,
					 const gchar *value,
					 gpointer data,
					 GError **error)
{
	g_object_set (G_OBJECT (mate_program_get ()),
		      LIBMATEUI_PARAM_CRASH_DIALOG, FALSE, NULL);

	return TRUE;
}

/* automagically parse all the gtkrc files for us.
 *
 * Parse:
 * $matedatadir/gtkrc
 * $matedatadir/$apprc
 * ~/.mate/gtkrc
 * ~/.mate/$apprc
 *
 * appname is derived from argv[0].  IMHO this is a great solution.
 * It provides good consistancy (you always know the rc file will be
 * the same name as the executable), and it's easy for the programmer.
 *
 * If you don't like it.. give me a good reason.
 */
static void
libmateui_rc_parse (MateProgram *program)
{
        gchar *command = g_get_prgname ();
	gint i;
	gint buf_len;
	const gchar *buf = NULL;
	gchar *file;
	gchar *apprc;

	buf_len = strlen(command);

	for (i = 0; i < buf_len; i++) {
		if (G_IS_DIR_SEPARATOR (command[buf_len - i])) {
			buf = &command[buf_len - i + 1];
			break;
		}
	}

	if (!buf)
                buf = command;

        apprc = g_alloca (strlen(buf) + 4);
	sprintf(apprc, "%src", buf);

	/* <matedatadir>/gtkrc */
        file = mate_program_locate_file (mate_program_get (),
                                          MATE_FILE_DOMAIN_DATADIR,
                                          "gtkrc-2.0", TRUE, NULL);
  	if (file) {
  		gtk_rc_parse (file);
		g_free (file);
	}

	/* <matedatadir>/<progname> */
        file = mate_program_locate_file (mate_program_get (),
                                          MATE_FILE_DOMAIN_DATADIR,
                                          apprc, TRUE, NULL);
	if (file) {
                gtk_rc_parse (file);
                g_free (file);
        }

	/* ~/.mate/gtkrc */
	file = mate_util_home_file("gtkrc-2.0");
	if (file) {
		gtk_rc_parse (file);
		g_free (file);
	}

	/* ~/.mate/<progname> */
	file = mate_util_home_file(apprc);
	if (file) {
		gtk_rc_parse (file);
		g_free (file);
	}
}

/* #warning "Solve the sound events situation" */

/* backwards compat */
/**
 * mate_init_with_popt_table:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: argument count (for example argc as received by main)
 * @argv: argument vector (for example argv as received by main)
 * @options: poptOption table with options to parse
 * @flags: popt flags.
 * @return_ctx: if non-NULL, the popt context is returned here.
 *
 * Initializes the application.  This sets up all of the MATE
 * internals and prepares them (imlib, gdk, session-management, triggers,
 * sound, user preferences).
 *
 * Unlike #mate_init, with #mate_init_with_popt_table you can provide
 * a table of popt options (popt is the command line argument parsing
 * library).
 *
 * Deprecated, use #mate_program_init with the LIBMATEUI_MODULE.
 *
 * Returns: 0 (always)
 */
int
mate_init_with_popt_table (const char *app_id,
			    const char *app_version,
			    int argc, char **argv,
			    const struct poptOption *options,
			    int flags,
			    poptContext *return_ctx)
{
	MateProgram *program;
        program = mate_program_init (app_id, app_version,
				      LIBMATEUI_MODULE,
				      argc, argv,
				      MATE_PARAM_POPT_TABLE, options,
				      MATE_PARAM_POPT_FLAGS, flags,
				      NULL);

        if(return_ctx) {
                poptContext context;
                g_object_get (program, MATE_PARAM_POPT_CONTEXT, &context, NULL);
                *return_ctx = context;
        }

        return 0;
}

const MateModuleInfo *
mate_gtk_module_info_get (void)
{
        return matecomponent_ui_gtk_module_info_get ();
}
