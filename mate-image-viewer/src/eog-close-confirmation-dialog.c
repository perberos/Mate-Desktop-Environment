/*
 * eog-close-confirmation-dialog.c
 * This file is part of eog
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.c) by gedit Team
 *
 * Copyright (C) 2004-2009 MATE Foundation 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "eog-close-confirmation-dialog.h"
#include <eog-window.h>

/* Properties */
enum 
{
	PROP_0,	
	PROP_UNSAVED_IMAGES
};

/* Mode */
enum
{
	SINGLE_IMG_MODE,
	MULTIPLE_IMGS_MODE
};

/* Columns */
enum
{
	SAVE_COLUMN,
	IMAGE_COLUMN,
	NAME_COLUMN,
	IMG_COLUMN, /* a handy pointer to the image */
	N_COLUMNS
};

struct _EogCloseConfirmationDialogPrivate 
{
	GList       *unsaved_images;
	
	GList       *selected_images;

	GtkTreeModel *list_store;
	GtkCellRenderer *toggle_renderer;
};


#define EOG_CLOSE_CONFIRMATION_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
							EOG_TYPE_CLOSE_CONFIRMATION_DIALOG, \
							EogCloseConfirmationDialogPrivate))

#define GET_MODE(priv) (((priv->unsaved_images != NULL) && \
			 (priv->unsaved_images->next == NULL)) ? \
			  SINGLE_IMG_MODE : MULTIPLE_IMGS_MODE)

#define IMAGE_COLUMN_HEIGHT 40

G_DEFINE_TYPE(EogCloseConfirmationDialog, eog_close_confirmation_dialog, GTK_TYPE_DIALOG)

static void 	 set_unsaved_image 		(EogCloseConfirmationDialog *dlg,
						 const GList                  *list);

static GList 	*get_selected_imgs 		(GtkTreeModel                 *store);

static GdkPixbuf *
eog_close_confirmation_dialog_get_icon (const gchar *icon_name)
{
	GError *error = NULL;
	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf;

	icon_theme = gtk_icon_theme_get_default ();

	pixbuf = gtk_icon_theme_load_icon (icon_theme,
					   icon_name,
					   IMAGE_COLUMN_HEIGHT,
					   0,
					   &error);

	if (!pixbuf) {
		g_warning ("Couldn't load icon: %s", error->message);
		g_error_free (error);
	}

	return pixbuf;
}

static GdkPixbuf*
get_nothumb_pixbuf (void)
{
	static GOnce nothumb_once = G_ONCE_INIT;
	g_once (&nothumb_once, (GThreadFunc) eog_close_confirmation_dialog_get_icon, "image-x-generic");
	return GDK_PIXBUF (g_object_ref (nothumb_once.retval));
}

/*  Since we connect in the costructor we are sure this handler will be called 
 *  before the user ones
 */
static void
response_cb (EogCloseConfirmationDialog *dlg,
             gint                        response_id,
             gpointer                    data)
{
	EogCloseConfirmationDialogPrivate *priv;

	g_return_if_fail (EOG_IS_CLOSE_CONFIRMATION_DIALOG (dlg));

	priv = dlg->priv;
	
	if (priv->selected_images != NULL)
		g_list_free (priv->selected_images);

	if (response_id == GTK_RESPONSE_YES)
	{
		if (GET_MODE (priv) == SINGLE_IMG_MODE)
		{
			priv->selected_images = 
				g_list_copy (priv->unsaved_images);
		}
		else
		{
			g_return_if_fail (priv->list_store);

			priv->selected_images =
				get_selected_imgs (priv->list_store);
		}
	}
	else
		priv->selected_images = NULL;
}

static void
add_buttons (EogCloseConfirmationDialog *dlg)
{
	gtk_dialog_add_button (GTK_DIALOG (dlg),
			       _("Close _without Saving"),
			       GTK_RESPONSE_NO);

	gtk_dialog_add_button (GTK_DIALOG (dlg),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	
	gtk_dialog_add_button (GTK_DIALOG (dlg),
			       GTK_STOCK_SAVE,
			       GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (dlg), 
					 GTK_RESPONSE_YES);
}

static void 
eog_close_confirmation_dialog_init (EogCloseConfirmationDialog *dlg)
{
	AtkObject *atk_obj;

	dlg->priv = EOG_CLOSE_CONFIRMATION_DIALOG_GET_PRIVATE (dlg);

	gtk_container_set_border_width (GTK_CONTAINER (dlg), 5);		
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), 14);
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dlg), TRUE);
	
	gtk_window_set_title (GTK_WINDOW (dlg), "");

	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	atk_obj = gtk_widget_get_accessible (GTK_WIDGET (dlg));
	atk_object_set_role (atk_obj, ATK_ROLE_ALERT);
	atk_object_set_name (atk_obj, _("Question"));
	
	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (response_cb),
			  NULL);
}

static void 
eog_close_confirmation_dialog_finalize (GObject *object)
{
	EogCloseConfirmationDialogPrivate *priv;

	priv = EOG_CLOSE_CONFIRMATION_DIALOG (object)->priv;

	if (priv->unsaved_images != NULL)
		g_list_free (priv->unsaved_images);

	if (priv->selected_images != NULL)
		g_list_free (priv->selected_images);

	/* Call the parent's destructor */
	G_OBJECT_CLASS (eog_close_confirmation_dialog_parent_class)->finalize (object);
}

static void
eog_close_confirmation_dialog_set_property (GObject      *object, 
					      guint         prop_id, 
					      const GValue *value, 
					      GParamSpec   *pspec)
{
	EogCloseConfirmationDialog *dlg;

	dlg = EOG_CLOSE_CONFIRMATION_DIALOG (object);

	switch (prop_id)
	{
		case PROP_UNSAVED_IMAGES:
			set_unsaved_image (dlg, g_value_get_pointer (value));
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
eog_close_confirmation_dialog_get_property (GObject    *object, 
					      guint       prop_id, 
					      GValue     *value, 
					      GParamSpec *pspec)
{
	EogCloseConfirmationDialogPrivate *priv;

	priv = EOG_CLOSE_CONFIRMATION_DIALOG (object)->priv;

	switch( prop_id )
	{
		case PROP_UNSAVED_IMAGES:
			g_value_set_pointer (value, priv->unsaved_images);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void 
eog_close_confirmation_dialog_class_init (EogCloseConfirmationDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = eog_close_confirmation_dialog_set_property;
	gobject_class->get_property = eog_close_confirmation_dialog_get_property;
	gobject_class->finalize = eog_close_confirmation_dialog_finalize;

	g_type_class_add_private (klass, sizeof (EogCloseConfirmationDialogPrivate));

	g_object_class_install_property (gobject_class,
					 PROP_UNSAVED_IMAGES,
					 g_param_spec_pointer ("unsaved_images",
						 	       "Unsaved Images",
							       "List of Unsaved Images",
							       (G_PARAM_READWRITE | 
							        G_PARAM_CONSTRUCT_ONLY)));
}

static GList *
get_selected_imgs (GtkTreeModel *store)
{
	GList       *list;
	gboolean     valid;
	GtkTreeIter  iter;

	list = NULL;
	valid = gtk_tree_model_get_iter_first (store, &iter);

	while (valid)
	{
		gboolean to_save;
		EogImage *img;

		gtk_tree_model_get (store, &iter, 
				    SAVE_COLUMN, &to_save,
				    IMG_COLUMN, &img,
				    -1);
		if (to_save)
			list = g_list_prepend (list, img);

		valid = gtk_tree_model_iter_next (store, &iter);
	}

	list = g_list_reverse (list);

	return list;
}

GList *
eog_close_confirmation_dialog_get_selected_images (EogCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (EOG_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return g_list_copy (dlg->priv->selected_images);
}

GtkWidget *
eog_close_confirmation_dialog_new (GtkWindow *parent, 
				   GList     *unsaved_images)
{
	GtkWidget *dlg;
	GtkWindowGroup *wg;

	g_return_val_if_fail (unsaved_images != NULL, NULL);

	dlg = GTK_WIDGET (g_object_new (EOG_TYPE_CLOSE_CONFIRMATION_DIALOG,
				        "unsaved_images", unsaved_images,
				        NULL));
	g_return_val_if_fail (dlg != NULL, NULL);

	if (parent != NULL)
	{
		wg = gtk_window_get_group (parent);

		/* gtk_window_get_group returns a default group when the given
		 * window doesn't have a group. Explicitly add the window to
		 * the group here to make sure it's actually in the returned
		 * group. It makes no difference if it is already. */
		gtk_window_group_add_window (wg, parent);
		gtk_window_group_add_window (wg, GTK_WINDOW (dlg));
		
		gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);					     
	}

	return dlg;
}

GtkWidget *
eog_close_confirmation_dialog_new_single (GtkWindow     *parent, 
					  EogImage	*image)
{
	GtkWidget *dlg;
	GList *unsaved_images;
	g_return_val_if_fail (image != NULL, NULL);
	
	unsaved_images = g_list_prepend (NULL, image);

	dlg = eog_close_confirmation_dialog_new (parent, 
						 unsaved_images);
	
	g_list_free (unsaved_images);

	return dlg;
}

static gchar *
get_text_secondary_label (EogImage *image)
{
	gchar *secondary_msg;
	
	secondary_msg = g_strdup (_("If you don't save, your changes will be lost."));
	
	return secondary_msg;
}

static void
build_single_img_dialog (EogCloseConfirmationDialog *dlg)
{
	GtkWidget     *hbox;
	GtkWidget     *vbox;
	GtkWidget     *primary_label;
	GtkWidget     *secondary_label;
	GtkWidget     *image;
	EogImage      *img;
	const gchar   *image_name;
	gchar         *str;
	gchar         *markup_str;

	g_return_if_fail (dlg->priv->unsaved_images->data != NULL);
	img = EOG_IMAGE (dlg->priv->unsaved_images->data);

	/* Image */
	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, 
					  GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	/* Primary label */
	primary_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	image_name = eog_image_get_caption (img);

	str = g_markup_printf_escaped (_("Save changes to image \"%s\" before closing?"),
				       image_name);
	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);

	gtk_label_set_markup (GTK_LABEL (primary_label), markup_str);
	g_free (markup_str);

	/* Secondary label */
	str = get_text_secondary_label (img);

	secondary_label = gtk_label_new (str);
	g_free (str);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 12);
	
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), primary_label, FALSE, FALSE, 0);
		      
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), 
			    hbox, 
	                    FALSE, 
			    FALSE, 
			    0);

	add_buttons (dlg);

	gtk_widget_show_all (hbox);
}

static void
populate_model (GtkTreeModel *store, GList *imgs)
{
	GtkTreeIter iter;

	while (imgs != NULL)
	{
		EogImage *img;
		const gchar *name;
		GdkPixbuf *buf = NULL;
		GdkPixbuf *buf_scaled = NULL;
		int width;
		double ratio;

		img = EOG_IMAGE (imgs->data);

		name = eog_image_get_caption (img);
		buf = eog_image_get_thumbnail (img);

		if (buf) {
			ratio = IMAGE_COLUMN_HEIGHT / (double) gdk_pixbuf_get_height (buf);
			width = (int) (gdk_pixbuf_get_width (buf) * ratio);
			buf_scaled = gdk_pixbuf_scale_simple (buf, width, IMAGE_COLUMN_HEIGHT, GDK_INTERP_BILINEAR);
		} else
			buf_scaled = get_nothumb_pixbuf ();

		gtk_list_store_append (GTK_LIST_STORE (store), &iter);
		gtk_list_store_set (GTK_LIST_STORE (store), &iter,
				    SAVE_COLUMN, TRUE,
				    IMAGE_COLUMN, buf_scaled,
				    NAME_COLUMN, name,
				    IMG_COLUMN, img,
			            -1);

		imgs = g_list_next (imgs);
		g_object_unref (buf_scaled);
	}
}

static void
save_toggled (GtkCellRendererToggle *renderer, gchar *path_str, GtkTreeModel *store)
{
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gboolean active;

	gtk_tree_model_get_iter (store, &iter, path);
	gtk_tree_model_get (store, &iter, SAVE_COLUMN, &active, -1);

	active ^= 1;

	gtk_list_store_set (GTK_LIST_STORE (store), &iter,
			    SAVE_COLUMN, active, -1);

	gtk_tree_path_free (path);
}

static GtkWidget *
create_treeview (EogCloseConfirmationDialogPrivate *priv)
{
	GtkListStore *store;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	treeview = gtk_tree_view_new ();
	gtk_widget_set_size_request (treeview, 260, 120);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);

	/* Create and populate the model */
	store = gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	populate_model (GTK_TREE_MODEL (store), priv->unsaved_images);

	/* Set model to the treeview */
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));
	g_object_unref (store);

	priv->list_store = GTK_TREE_MODEL (store);
	
	/* Add columns */
	priv->toggle_renderer = renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled",
			  G_CALLBACK (save_toggled), store);

	column = gtk_tree_view_column_new_with_attributes ("Save?",
							   renderer,
							   "active",
							   SAVE_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new_with_attributes ("Image",
							   renderer,
							   "pixbuf",
							   IMAGE_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name",
							   renderer,
							   "text",
							   NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	return treeview;
}

static void
build_multiple_imgs_dialog (EogCloseConfirmationDialog *dlg)
{
	EogCloseConfirmationDialogPrivate *priv;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *primary_label;
	GtkWidget *vbox2;
	GtkWidget *select_label;
	GtkWidget *scrolledwindow;
	GtkWidget *treeview;
	GtkWidget *secondary_label;
	gchar     *str;
	gchar     *markup_str;

	priv = dlg->priv;

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), 
			    hbox, TRUE, TRUE, 0);

	/* Image */
	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, 
					  GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	/* Primary label */
	primary_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	str = g_strdup_printf (
			ngettext ("There is %d image with unsaved changes. "
				  "Save changes before closing?",
				  "There are %d images with unsaved changes. "
				  "Save changes before closing?",
				  g_list_length (priv->unsaved_images)),
			g_list_length (priv->unsaved_images));

	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);
	
	gtk_label_set_markup (GTK_LABEL (primary_label), markup_str);
	g_free (markup_str);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

	select_label = gtk_label_new_with_mnemonic (_("S_elect the images you want to save:"));

	gtk_box_pack_start (GTK_BOX (vbox2), select_label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (select_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (select_label), 0.0, 0.5);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox2), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
					GTK_POLICY_AUTOMATIC, 
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), 
					     GTK_SHADOW_IN);

	treeview = create_treeview (priv);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);

	/* Secondary label */
	secondary_label = gtk_label_new (_("If you don't save, "
					   "all your changes will be lost."));

	gtk_box_pack_start (GTK_BOX (vbox2), secondary_label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (select_label), treeview);

	add_buttons (dlg);

	gtk_widget_show_all (hbox);
}

static void
set_unsaved_image (EogCloseConfirmationDialog *dlg,
		   const GList                *list)
{
	EogCloseConfirmationDialogPrivate *priv;

	g_return_if_fail (list != NULL);	

	priv = dlg->priv;
	g_return_if_fail (priv->unsaved_images == NULL);

	priv->unsaved_images = g_list_copy ((GList *)list);

	if (GET_MODE (priv) == SINGLE_IMG_MODE)
	{
		build_single_img_dialog (dlg);
	}
	else
	{
		build_multiple_imgs_dialog (dlg);
	}	
}

const GList *
eog_close_confirmation_dialog_get_unsaved_images (EogCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (EOG_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return dlg->priv->unsaved_images;
}

void
eog_close_confirmation_dialog_set_sensitive (EogCloseConfirmationDialog *dlg,
					     gboolean value)
{
	GtkWidget *action_area;

	g_return_if_fail (EOG_IS_CLOSE_CONFIRMATION_DIALOG (dlg));

	action_area = gtk_dialog_get_action_area (GTK_DIALOG (dlg));
	gtk_widget_set_sensitive (action_area, value);

	if (dlg->priv->toggle_renderer)
		gtk_cell_renderer_toggle_set_activatable (GTK_CELL_RENDERER_TOGGLE (dlg->priv->toggle_renderer), value);
}
