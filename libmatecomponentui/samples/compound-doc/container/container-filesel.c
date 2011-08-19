#include "config.h"
#include <gtk/gtk.h>
#include "container-filesel.h"
#include "container.h"

void
container_request_file (SampleApp    *app,
			gboolean      save,
			GCallback     cb,
			gpointer      user_data)
{
	GtkWidget *fs;

	app->fileselection = fs =
	    gtk_file_chooser_dialog_new ("Select file",
					 GTK_WINDOW (app->win),
					 save ? GTK_FILE_CHOOSER_ACTION_SAVE
					      : GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
					 GTK_RESPONSE_OK,
					 NULL);

	g_signal_connect (G_OBJECT (fs),
			  "response", G_CALLBACK (cb), user_data);

	gtk_window_set_modal (GTK_WINDOW (fs), TRUE);

	gtk_widget_show (fs);
}
