#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <stdlib.h>
#include <libmatecomponentui.h>
#include <gtk/gtk.h>
#include <libmate/mate-init.h>

#include "container.h"
#include "document.h"
#include "container-menu.h"

void
sample_app_exit (SampleApp *app)
{
	GList *l;

	for (l = app->doc_views; l; l = l->next)
		g_object_unref (G_OBJECT (l->data));

	g_object_unref (G_OBJECT (app->doc));
	gtk_widget_destroy (app->win);

	g_free (app);

	matecomponent_main_quit ();
}

static gint
delete_cb (GtkWidget *caller, GdkEvent *event, SampleApp *app)
{
	sample_app_exit (app);

	return TRUE;
}

static SampleApp *
sample_app_new (SampleDoc *doc)
{
	SampleApp *app = g_new0 (SampleApp, 1);
	GtkWidget *view;

	/* Create a document */
	app->doc = sample_doc_new ();
	if (!app->doc) {
		g_free (app);
		return NULL;
	}

	/* Create toplevel window */
	app->win = matecomponent_window_new ("sample-doc-container",
				      "Sample Document container");

	gtk_window_set_default_size (GTK_WINDOW (app->win), 400, 600);
	g_signal_connect_data (G_OBJECT (app->win), "delete_event",
			       G_CALLBACK (delete_cb), app, NULL, 0);

	/* Create and merge the UI elements. */
	sample_app_fill_menu (app);

	/* Create a doc view and stuff it into a box in the toplevel. */
	app->box = gtk_vbox_new (FALSE, 10);
	matecomponent_window_set_contents (MATECOMPONENT_WINDOW (app->win), app->box);

	view = sample_doc_view_new (app->doc, MATECOMPONENT_OBJREF (
		matecomponent_window_get_ui_container (MATECOMPONENT_WINDOW (app->win))));
	app->doc_views = g_list_prepend (app->doc_views, view);
	app->curr_view = view;

	gtk_widget_show_all (app->win);

	return app;
}

int
main (int argc, char **argv)
{
	SampleDoc *doc;
	SampleApp *app;

	free (malloc (8));

	if (!matecomponent_ui_init ("container", VERSION, &argc, argv))
		g_error ("Could not initialize libmatecomponentui!\n");

	doc = sample_doc_new ();
	app = sample_app_new (doc);

	matecomponent_main ();

	return 0;
}
