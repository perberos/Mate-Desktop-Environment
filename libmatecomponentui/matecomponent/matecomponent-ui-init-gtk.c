#include <config.h>
#include <popt.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

/* We need the undeprecated form of MateModuleInfo and also the popt.h include */
#undef MATE_DISABLE_DEPRECATED
#include <libmate/mate-init.h>

#include <matecomponent/matecomponent-ui-main.h>

typedef struct {
	GPtrArray *gtk_args;
} matecomponent_ui_gtk_init_info_t;

static void
matecomponent_ui_gtk_pre_args_parse (MateProgram    *program,
			      MateModuleInfo *mod_info)
{
	GOptionContext *context;
	matecomponent_ui_gtk_init_info_t *init_info;

	/* Only do this for popt option parsing */
	g_object_get (G_OBJECT (program), MATE_PARAM_GOPTION_CONTEXT,
		      &context, NULL);
	if (context) return;
	
	init_info = g_new0 (matecomponent_ui_gtk_init_info_t, 1);
	init_info->gtk_args = g_ptr_array_new ();

	g_object_set_data (G_OBJECT (program),
			   "Libmatecomponentui-Gtk-Module-init-info",
			   init_info);
}

static void
matecomponent_ui_gtk_post_args_parse (MateProgram    *program,
			       MateModuleInfo *mod_info)
{
	GOptionContext *context;
	matecomponent_ui_gtk_init_info_t *init_info;
	int final_argc;
	char **final_argv;
	int i;

	/* Only do this for popt option parsing */
	g_object_get (G_OBJECT (program), MATE_PARAM_GOPTION_CONTEXT,
		      &context, NULL);
	if (context) return;

	init_info = g_object_get_data (G_OBJECT (program),
				       "Libmatecomponentui-Gtk-Module-init-info");

	g_ptr_array_add (init_info->gtk_args, NULL);

	final_argc = init_info->gtk_args->len - 1;
	final_argv = g_memdup (init_info->gtk_args->pdata,
			       sizeof (char *) * init_info->gtk_args->len);

	gtk_init (&final_argc, &final_argv);

	g_free (final_argv);

	for (i = 0; g_ptr_array_index (init_info->gtk_args, i) != NULL; i++) {
		g_free (g_ptr_array_index (init_info->gtk_args, i));
		g_ptr_array_index (init_info->gtk_args, i) = NULL;
	}

	g_ptr_array_free (init_info->gtk_args, TRUE);
	init_info->gtk_args = NULL;
	g_free (init_info);

	g_object_set_data (G_OBJECT (program),
			   "Libmatecomponentui-Gtk-Module-init-info",
			   NULL);
}

static void
add_gtk_arg_callback (poptContext con, enum poptCallbackReason reason,
		      const struct poptOption * opt,
		      const char * arg, void * data)
{
	MateProgram *program;
	matecomponent_ui_gtk_init_info_t *init_info;
	char *newstr;

	program = g_dataset_get_data (con, "MateProgram");
	g_assert (program != NULL);

	init_info = g_object_get_data (G_OBJECT (program),
				       "Libmatecomponentui-Gtk-Module-init-info");
	g_assert (init_info != NULL);


	switch (reason) {
	case POPT_CALLBACK_REASON_PRE:
		/* Note that the value of argv[0] passed to main() may be
		 * different from the value that this passes to gtk
		 */
		g_ptr_array_add (init_info->gtk_args,
				 (char *) g_strdup (poptGetInvocationName (con)));
		break;
		
	case POPT_CALLBACK_REASON_OPTION:
		switch (opt->argInfo) {
		case POPT_ARG_STRING:
		case POPT_ARG_INT:
		case POPT_ARG_LONG:
			newstr = g_strconcat ("--", opt->longName, "=", arg, NULL);
			break;
		default:
			newstr = g_strconcat ("--", opt->longName, NULL);
			break;
		}

		g_ptr_array_add (init_info->gtk_args, newstr);
		/* XXX matecomponent-client tie-in */
		break;
	default:
		break;
	}
}

static struct poptOption matecomponent_ui_gtk_options [] = {
	{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE,
	  &add_gtk_arg_callback, 0, NULL, NULL },

	{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL },

	{ "gdk-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gdk debugging flags to set"), N_("FLAGS")},

	{ "gdk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gdk debugging flags to unset"), N_("FLAGS")},

	/* X11 only */
	{ "display", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("X display to use"), N_("DISPLAY")},

	/* X11 & multi-head only */
	{ "screen", '\0', POPT_ARG_INT, NULL, 0,
	  N_("X screen to use"), N_("SCREEN")},

	/* X11 only */
	{ "sync", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Make X calls synchronous"), NULL},

	/* FIXME: this doesn't seem to exist */
#if 0
	/* X11 only */
	{ "no-xshm", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Don't use X shared memory extension"), NULL},
#endif

	{ "name", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Program name as used by the window manager"), N_("NAME")},

	{ "class", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Program class as used by the window manager"), N_("CLASS")},

	{ "gtk-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gtk+ debugging flags to set"), N_("FLAGS")},

	{ "gtk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gtk+ debugging flags to unset"), N_("FLAGS")},

	{ "g-fatal-warnings", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Make all warnings fatal"), NULL},

	{ "gtk-module", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Load an additional Gtk module"), N_("MODULE")},

	{ NULL, '\0', 0, NULL, 0}
};

static GOptionGroup *
matecomponent_ui_gtk_module_get_goption_group (void)
{
	/* FIXME: TRUE or FALSE (open default display)? */
	return gtk_get_option_group (TRUE);
}

const MateModuleInfo *
matecomponent_ui_gtk_module_info_get (void)
{
	static MateModuleInfo module_info = {
		"gtk", NULL, N_("GTK+"),
		NULL, NULL,
		matecomponent_ui_gtk_pre_args_parse, matecomponent_ui_gtk_post_args_parse, matecomponent_ui_gtk_options,
		NULL,
		NULL, NULL, NULL
	};
	
	module_info.get_goption_group_func = matecomponent_ui_gtk_module_get_goption_group;

	if (module_info.version == NULL) {
		module_info.version = g_strdup_printf ("%d.%d.%d",
						       GTK_MAJOR_VERSION,
						       GTK_MINOR_VERSION,
						       GTK_MICRO_VERSION);
	}

	return &module_info;
}
