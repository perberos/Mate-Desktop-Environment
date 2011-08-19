#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "eog-save-as-dialog-helper.h"
#include "eog-pixbuf-util.h"
#include "eog-file-chooser.h"

typedef struct {
	GtkWidget *dir_chooser;
	GtkWidget *token_entry;
	GtkWidget *replace_spaces_check;
	GtkWidget *counter_spin;
	GtkWidget *preview_label;
	GtkWidget *format_combobox;

	guint      idle_id;
	gint       n_images;
	EogImage  *image;
	gint       nth_image;
} SaveAsData;

static GdkPixbufFormat *
get_selected_format (GtkComboBox *combobox)
{
	GdkPixbufFormat *format;
	GtkTreeModel *store;
	GtkTreeIter iter;

	gtk_combo_box_get_active_iter (combobox, &iter);
	store = gtk_combo_box_get_model (combobox);
	gtk_tree_model_get (store, &iter, 1, &format, -1);

	return format;
}

static gboolean
update_preview (gpointer user_data)
{
	SaveAsData *data;
	char *preview_str = NULL;
	const char *token_str;
	gboolean convert_spaces;
	gulong   counter_start;
	GdkPixbufFormat *format;

	data = g_object_get_data (G_OBJECT (user_data), "data");
	g_assert (data != NULL);

	if (data->image == NULL) return FALSE;

	/* obtain required dialog data */
	token_str = gtk_entry_get_text (GTK_ENTRY (data->token_entry));
	convert_spaces = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (data->replace_spaces_check));
	counter_start = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data->counter_spin));

	format = get_selected_format (GTK_COMBO_BOX (data->format_combobox));

	if (token_str != NULL) {
		/* generate preview filename */
		preview_str = eog_uri_converter_preview (token_str, data->image, format,
							 (counter_start + data->nth_image),
							 data->n_images,
							 convert_spaces, '_' /* FIXME: make this editable */);
	}

	gtk_label_set_text (GTK_LABEL (data->preview_label), preview_str);

	g_free (preview_str);

	data->idle_id = 0;

	return FALSE;
}

static void
request_preview_update (GtkWidget *dlg)
{
	SaveAsData *data;

	data = g_object_get_data (G_OBJECT (dlg), "data");
	g_assert (data != NULL);

	if (data->idle_id != 0)
		return;

	data->idle_id = g_idle_add (update_preview, dlg);
}

static void
on_format_combobox_changed (GtkComboBox *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
on_token_entry_changed (GtkWidget *widget, gpointer user_data)
{
	SaveAsData *data;
	gboolean enable_save;

	data = g_object_get_data (G_OBJECT (user_data), "data");
	g_assert (data != NULL);

	request_preview_update (GTK_WIDGET (user_data));

	enable_save = (strlen (gtk_entry_get_text (GTK_ENTRY (data->token_entry))) > 0);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (user_data), GTK_RESPONSE_OK,
					   enable_save);
}

static void
on_replace_spaces_check_clicked (GtkWidget *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
on_counter_spin_changed (GtkWidget *widget, gpointer data)
{
	request_preview_update (GTK_WIDGET (data));
}

static void
prepare_format_combobox (SaveAsData *data)
{
	GtkComboBox *combobox;
	GtkCellRenderer *cell;
	GSList *formats;
	GtkListStore *store;
	GSList *it;
	GtkTreeIter iter;

	combobox = GTK_COMBO_BOX (data->format_combobox);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_combo_box_set_model (combobox, GTK_TREE_MODEL (store));

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), cell, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combobox), cell,
				 	"text", 0);

	formats = eog_pixbuf_get_savable_formats ();
	for (it = formats; it != NULL; it = it->next) {
		GdkPixbufFormat *f;

		f = (GdkPixbufFormat*) it->data;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, gdk_pixbuf_format_get_name (f), 1, f, -1);
	}
	g_slist_free (formats);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, _("as is"), 1, NULL, -1);
	gtk_combo_box_set_active_iter (combobox, &iter);
	gtk_widget_show_all (GTK_WIDGET (combobox));
}

static void
destroy_data_cb (gpointer data)
{
	SaveAsData *sd;

	sd = (SaveAsData*) data;

	if (sd->image != NULL)
		g_object_unref (sd->image);

	if (sd->idle_id != 0)
		g_source_remove (sd->idle_id);

	g_slice_free (SaveAsData, sd);
}

static void
set_default_values (GtkWidget *dlg, GFile *base_file)
{
	SaveAsData *sd;

	sd = (SaveAsData*) g_object_get_data (G_OBJECT (dlg), "data");

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (sd->counter_spin), 0.0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sd->replace_spaces_check),
				      FALSE);
	if (base_file != NULL) {
		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (sd->dir_chooser), base_file, NULL);
	}

	/*gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg), GTK_RESPONSE_OK, FALSE);*/

	request_preview_update (dlg);
}

GtkWidget*
eog_save_as_dialog_new (GtkWindow *main, GList *images, GFile *base_file)
{
	char *filepath;
	GtkBuilder  *xml;
	GtkWidget *dlg;
	SaveAsData *data;
	GtkWidget *label;

	filepath = g_build_filename (EOG_DATA_DIR,
				     "eog-multiple-save-as-dialog.ui",
				     NULL);

	xml = gtk_builder_new ();
	gtk_builder_set_translation_domain (xml, GETTEXT_PACKAGE);
	g_assert (gtk_builder_add_from_file (xml, filepath, NULL));

	g_free (filepath);

	dlg = GTK_WIDGET (g_object_ref (gtk_builder_get_object (xml, "eog_multiple_save_as_dialog")));
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (main));
	gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER_ON_PARENT);

	data = g_slice_new0 (SaveAsData);
	/* init widget references */
	data->dir_chooser = GTK_WIDGET (gtk_builder_get_object (xml,
								"dir_chooser"));
	data->token_entry = GTK_WIDGET (gtk_builder_get_object (xml,
								"token_entry"));
	data->replace_spaces_check = GTK_WIDGET (gtk_builder_get_object (xml,
						       "replace_spaces_check"));
	data->counter_spin = GTK_WIDGET (gtk_builder_get_object (xml,
							       "counter_spin"));
	data->preview_label = GTK_WIDGET (gtk_builder_get_object (xml,
							      "preview_label"));
	data->format_combobox = GTK_WIDGET (gtk_builder_get_object (xml,
							    "format_combobox"));

	/* init preview information */
	data->idle_id = 0;
	data->n_images = g_list_length (images);
	data->nth_image = (int) ((float) data->n_images * rand() / (float) (RAND_MAX+1.0));
	g_assert (data->nth_image >= 0 && data->nth_image < data->n_images);
	data->image = g_object_ref (G_OBJECT (g_list_nth_data (images, data->nth_image)));
	g_object_set_data_full (G_OBJECT (dlg), "data", data, destroy_data_cb);

	g_signal_connect (G_OBJECT (data->format_combobox), "changed",
			  (GCallback) on_format_combobox_changed, dlg);

	g_signal_connect (G_OBJECT (data->token_entry), "changed",
			  (GCallback) on_token_entry_changed, dlg);

	g_signal_connect (G_OBJECT (data->replace_spaces_check), "toggled",
			  (GCallback) on_replace_spaces_check_clicked, dlg);

	g_signal_connect (G_OBJECT (data->counter_spin), "changed",
			  (GCallback) on_counter_spin_changed, dlg);

	label = GTK_WIDGET (gtk_builder_get_object (xml, "preview_label_from"));
	gtk_label_set_text (GTK_LABEL (label), eog_image_get_caption (data->image));

	prepare_format_combobox (data);

	set_default_values (dlg, base_file);
	g_object_unref (xml);
	return dlg;
}

EogURIConverter*
eog_save_as_dialog_get_converter (GtkWidget *dlg)
{
	EogURIConverter *conv;

	SaveAsData *data;
	const char *format_str;
	gboolean convert_spaces;
	gulong   counter_start;
	GdkPixbufFormat *format;
	GFile *base_file;

	data = g_object_get_data (G_OBJECT (dlg), "data");
	g_assert (data != NULL);

	/* obtain required dialog data */
	format_str = gtk_entry_get_text (GTK_ENTRY (data->token_entry));

	convert_spaces = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (data->replace_spaces_check));

	counter_start = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data->counter_spin));

	format = get_selected_format (GTK_COMBO_BOX (data->format_combobox));

	base_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (data->dir_chooser));

	/* create converter object */
	conv = eog_uri_converter_new (base_file, format, format_str);

	/* set other properties */
	g_object_set (G_OBJECT (conv),
		      "convert-spaces", convert_spaces,
		      "space-character", '_',
		      "counter-start", counter_start,
		      "n-images", data->n_images,
		      NULL);

	g_object_unref (base_file);

	return conv;
}
