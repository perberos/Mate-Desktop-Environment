#undef GTK_DISABLE_DEPRECATED

#include <unistd.h>
#include <matecomponent.h>
#include <glib.h>

#define APPNAME "TestContainer"
#define APPVERSION "1.0"

#define UI_XML "MateComponent_Sample_Container-ui.xml"

static gchar **files = NULL;
static gboolean use_gtk_window = FALSE;
static const GOptionEntry options[] = {
	{ "gtk", 'g', 0, G_OPTION_ARG_NONE, &use_gtk_window,
	  "Use GtkWindow instead of MateComponentWindow (default)", NULL },
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, "FILE" },
	{ NULL }
};

/*
 * FIXME: TODO:
 *
 *  + Add a Menu item 'activate' to do a control_frame_activate /
 *  trigger a UI merge / de-merge.
 *  + Add gtk-only mode
 */
static void
window_destroyed (GtkWindow * window, gpointer user_data)
{
        g_warning ("FIXME: should count toplevels");
        matecomponent_main_quit ();
}

static void
window_title (GtkWindow * window, const char *moniker, gboolean use_gtk)
{
        char *title;

        title = g_strdup_printf ("%s - in a %s", moniker,
                                 g_type_name_from_instance ((gpointer)
                                                            window));
        gtk_window_set_title (window, title);
        g_free (title);
}

static void
verb_HelpAbout (MateComponentUIComponent * uic, gpointer user_data,
                 const char *cname)
{
        g_message
            ("Unfortunately I cannot use mate_about API - it would introduce more dependencies to libmatecomponentui");
}

static void
verb_FileExit (MateComponentUIComponent * uic, gpointer user_data,
                const char *cname)
{
        matecomponent_main_quit ();
}

static void
verb_Activate (MateComponentUIComponent * uic, gpointer user_data,
                const char *cname)
{
        matecomponent_control_frame_control_activate
             (matecomponent_widget_get_control_frame
             (MATECOMPONENT_WIDGET
             (matecomponent_window_get_contents
             (MATECOMPONENT_WINDOW (user_data)))));
}

static MateComponentUIVerb matecomponent_app_verbs[] = {
        MATECOMPONENT_UI_VERB ("FileExit", verb_FileExit),
        MATECOMPONENT_UI_VERB ("HelpAbout", verb_HelpAbout),
        MATECOMPONENT_UI_VERB ("Activate", verb_Activate),
        MATECOMPONENT_UI_VERB_END
};

static void
window_create (const char *moniker, gboolean use_gtk)
{
        GtkWidget *window;
        GtkWidget *control;
        MateComponentUIContainer *ui_container;

        if (use_gtk) {
                window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
                ui_container = CORBA_OBJECT_NIL;
        } else {
                MateComponentUIComponent *ui_comp =
                    matecomponent_ui_component_new_default ();
                window = matecomponent_window_new (APPNAME, APPNAME);
                ui_container =
                    matecomponent_window_get_ui_container (MATECOMPONENT_WINDOW
                                                    (window));
                matecomponent_ui_component_set_container (ui_comp,
                                                   MATECOMPONENT_OBJREF
                                                   (ui_container),
                                                   NULL);
                matecomponent_ui_util_set_ui (ui_comp, "", UI_XML,
                                       APPNAME, NULL);
                matecomponent_ui_component_add_verb_list_with_data (ui_comp,
                                                             matecomponent_app_verbs,
                                                             window);
        }

        window_title (GTK_WINDOW (window), moniker, use_gtk);

        control = matecomponent_widget_new_control (moniker,
                                             MATECOMPONENT_OBJREF (ui_container));

        if (!control) {
                g_warning ("Couldn't create a control for '%s'",
                           moniker);
                return;
        }

        if (use_gtk) {
                gtk_container_add (GTK_CONTAINER (window), control);
        } else
                matecomponent_window_set_contents (MATECOMPONENT_WINDOW (window),
                                             control);

        g_signal_connect (window, "destroy",
                           G_CALLBACK (window_destroyed ), NULL);

        gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
        int i;
        GOptionContext *context;
        MateProgram *program;
        
        context = g_option_context_new (NULL);
        g_option_context_add_main_entries (context, options, "matecomponent-sample-container");
        
        program = mate_program_init (APPNAME, APPVERSION,
                                      LIBMATECOMPONENTUI_MODULE,
                                      argc, argv,
                                      MATE_PARAM_GOPTION_CONTEXT, context,
                                      MATE_PARAM_NONE);

        /* Check for argument consistency. */
        if (files == NULL) {
                g_message ("Must specify a filename");
                return 1;
        }

        for (i = 0; i < g_strv_length (files); i++) {
                char *moniker;

                /* FIXME: we should do some auto-detection here */
                moniker = g_strdup_printf ("file:%s", files[i]);

                window_create (moniker, use_gtk_window);
        }

        matecomponent_main ();

        g_object_unref (program);

        return 0;
}
