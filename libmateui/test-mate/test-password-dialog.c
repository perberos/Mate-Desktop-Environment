#undef GTK_DISABLE_DEPRECATED

#include <config.h>

#include <gtk/gtk.h>
#include <libmateui.h>

#include "prop-editor.h"

static GtkWidget *password_dialog = NULL;
static GtkWidget *property_editor = NULL;

static gdouble
quality_func (MatePasswordDialog *dialog, const char *text, gpointer ptr)
{
	const char *p;
	gsize length;
	int uppercase = 0, symbols = 0, numbers = 0, strength;
	gunichar uc;

	if (text == NULL) return 0.0;

	/* Get the length */
	length = g_utf8_strlen (text, -1);

	/* Count the number of number, symbols and uppercase chars */
	for (p = text; *p; p = g_utf8_find_next_char (p, NULL))
	{
		uc = g_utf8_get_char (p);

		if (g_unichar_isdigit (uc))
		{
			numbers++;
		}
		else if (g_unichar_isupper (uc))
		{
			uppercase++;
		}
		else if (g_unichar_islower (uc))
		{
			/* Not counted */
		}
		else if (g_unichar_isgraph (uc))
		{
			symbols++;
		}
	}

	if (length    > 5) length = 5;
	if (numbers   > 3) numbers = 3;
	if (symbols   > 3) symbols = 3;
	if (uppercase > 3) uppercase = 3;

	strength = (length * 10 - 20) + (numbers * 10) + (symbols * 15) + (uppercase * 10);
	strength = CLAMP (strength, 0, 100);

	return (double) strength / 100.0;
}

static void
authenticate_boink_callback (GtkWidget *button, gpointer user_data)
{
	GtkWindowGroup *group;
	gboolean  result;
	char	  *username;
	char	  *password;

	if (password_dialog == NULL) {

		password_dialog = g_object_new (MATE_TYPE_PASSWORD_DIALOG,
						"title", "Authenticate Me",
						"message", "Authenticate Me and make this text as long as possible so it starts to wrap",
						NULL);
		g_signal_connect (password_dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &password_dialog);

/*		gtk_window_set_resizable (GTK_WINDOW (password_dialog), TRUE); */

		mate_password_dialog_set_username (MATE_PASSWORD_DIALOG (password_dialog), "foouser");
		mate_password_dialog_set_password (MATE_PASSWORD_DIALOG (password_dialog), "sekret");
		mate_password_dialog_set_password_quality_func (MATE_PASSWORD_DIALOG (password_dialog),
								 quality_func, NULL, NULL);

		group = gtk_window_group_new ();
		gtk_window_group_add_window (group, GTK_WINDOW (password_dialog));
		g_object_unref (group);
	}
	if (property_editor == NULL)
	{
		property_editor = create_prop_editor (G_OBJECT (password_dialog), MATE_TYPE_PASSWORD_DIALOG);
		g_signal_connect (property_editor, "destroy", G_CALLBACK (gtk_widget_destroyed), &property_editor);

		/* Add the dialog and the prop editor to different window groups,
		 * so we can actually use the prop editor with the modal dialog.
		 */
		group = gtk_window_group_new ();
		gtk_window_group_add_window (group, GTK_WINDOW (property_editor));
		g_object_unref (group);
	}

	result = mate_password_dialog_run_and_block (MATE_PASSWORD_DIALOG (password_dialog));

	if (result) {
		username = mate_password_dialog_get_username (MATE_PASSWORD_DIALOG (password_dialog));
		password = mate_password_dialog_get_password (MATE_PASSWORD_DIALOG (password_dialog));

		g_print ("authentication submitted: username='%s' , password='%s'\n",
			 username,
			 password);

	        g_free (username);
	        g_free (password);
	}
	else {
		g_print ("authentication cancelled:\n");
	}
}

static void
exit_callback (GtkWidget *button, gpointer user_data)
{
	if (password_dialog != NULL) {
		gtk_dialog_response (GTK_DIALOG (password_dialog), GTK_RESPONSE_CANCEL);
	}

	gtk_main_quit ();
}

int
main (int argc, char * argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *authenticate_button;
	GtkWidget *exit_button;
	MateProgram *program;

	program = mate_program_init ("test-mate-password-dialog", VERSION,
				      libmateui_module_info_get (), argc, argv,
				      NULL);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	authenticate_button = gtk_button_new_with_label ("Boink me to authenticate");
	exit_button = gtk_button_new_with_label ("Exit");

	g_signal_connect (authenticate_button,
			    "clicked",
			    G_CALLBACK (authenticate_boink_callback),
			    NULL);

	g_signal_connect (exit_button,
			    "clicked",
			    G_CALLBACK (exit_callback),
			    NULL);
	
	gtk_box_pack_start (GTK_BOX (vbox), authenticate_button, TRUE, TRUE, 4);
	gtk_box_pack_end (GTK_BOX (vbox), exit_button, TRUE, TRUE, 0);
	
	gtk_widget_show_all (window);

	gtk_main ();

	g_object_unref (program);

	return 0;
}
