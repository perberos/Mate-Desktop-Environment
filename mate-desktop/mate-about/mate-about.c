#ifndef __MATE_ABOUT_C__
#define __MATE_ABOUT_C__

#include "mate-about.h"

void mate_about_dialog(void)
{
	/**
	 * Es importante llamar gtk_init, si no, no se puede iniciar bien el dialogo
	 */
	GtkWidget* dialog = gtk_about_dialog_new();

	gtk_window_set_default_icon_name(icon);

	// logo
	#if GTK_CHECK_VERSION(3, 0, 0) || GTK_CHECK_VERSION(2, 6, 0)
		gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), icon);
	#else

		GtkIconTheme* icon_theme = gtk_icon_theme_get_default();

		if (gtk_icon_theme_has_icon(icon_theme, icon))
		{
			//

			GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme,
				icon, 64, 0, NULL);
			gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
			g_object_unref(pixbuf);
		}

	#endif

	//name
	#if GTK_CHECK_VERSION(3, 0, 0) || GTK_CHECK_VERSION(2, 12, 0)
		gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), gettext(program_name));
	#else
		gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), gettext(program_name));
	#endif

	// version
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), version);

	// creditos y pagina web
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), copyright);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), website);

	/**
	 * This generate a random message.
	 * The comments index must not be more than comments_count - 1
	 */
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
		gettext(comments[g_random_int_range(0, comments_count - 1)]));

	// autores
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	// I comment this because the list is empty
	//gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), artists);
	//gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), documenters);

	#if GTK_CHECK_VERSION(3, 0, 0)
		//gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_3_0);
		//gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), FALSE);
	#endif

	// start and destroy
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void mate_about_release_version(void)
{
	g_printf("%s %s\n", program_name, version);
}

// ...
gint main(gint argc, gchar** argv)
{
	/**
	 * Solo utilizado para option parse
	 */
	GError* error = NULL;

	/**
	 * Con esto se inicia gettext
	 */
	bindtextdomain(gettext_package, locale_dir);
	bind_textdomain_codeset(gettext_package, "UTF-8");
	textdomain(gettext_package);

	/**
	 * http://www.gtk.org/api/2.6/glib/glib-Commandline-option-parser.html
	 */
	GOptionContext* context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, command_entries, gettext_package);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);

	/**
	 * Not necesary at all, program just run and die.
	 * But it free a little memory.
	 */
	g_option_context_free(context);

	gtk_init(&argc, &argv);

	if (mate_about_nogui == FALSE)
	{
		mate_about_dialog();
	}
	else
	{
		mate_about_release_version();
	}

	return 0;
}

#endif
