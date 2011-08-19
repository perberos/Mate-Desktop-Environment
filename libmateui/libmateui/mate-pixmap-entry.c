/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* MatePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 * Author: George Lebl <jirka@5z.com>
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <libmate/mate-macros.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include <glib/gi18n-lib.h>

#include <libmate/mate-util.h>
#include "mate-file-entry.h"
#include "mate-pixmap-entry.h"
#include "mate-pixmap.h"

#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-utils.h>

#include "libmateui-access.h"

struct _MatePixmapEntryPrivate {
	GtkWidget *preview;
	GtkWidget *preview_sw;

	gchar *last_preview;

	guint32 do_preview : 1; /*put a preview frame with the pixmap next to
				  the entry*/
};


static void drag_data_get		  (GtkWidget          *widget,
					   GdkDragContext     *context,
					   GtkSelectionData   *selection_data,
					   guint               info,
					   guint               time,
					   MatePixmapEntry   *pentry);
static void pentry_destroy		  (GtkObject *object);
static void pentry_finalize		  (GObject *object);

static void pentry_set_property (GObject *object, guint param_id,
				 const GValue *value, GParamSpec *pspec);
static void pentry_get_property (GObject *object, guint param_id,
				 GValue *value, GParamSpec *pspec);

/* Property IDs */
enum {
	PROP_0,
	PROP_DO_PREVIEW
};

MATE_CLASS_BOILERPLATE (MatePixmapEntry, mate_pixmap_entry,
			 MateFileEntry, MATE_TYPE_FILE_ENTRY)

static void
mate_pixmap_entry_class_init (MatePixmapEntryClass *class)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(class);
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);

	gobject_class->finalize = pentry_finalize;
	gobject_class->set_property = pentry_set_property;
	gobject_class->get_property = pentry_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_DO_PREVIEW,
					 g_param_spec_boolean (
						 "do_preview",
						 _("Do Preview"),
						 _("Whether the pixmap entry should have a preview."),
						 FALSE,
						 G_PARAM_READWRITE));

	object_class->destroy = pentry_destroy;
}

/* set_property handler for the pixmap entry */
static void
pentry_set_property (GObject *object, guint param_id,
		     const GValue *value, GParamSpec *pspec)
{
	MatePixmapEntry *pentry;

	pentry = MATE_PIXMAP_ENTRY (object);

	switch (param_id) {
	case PROP_DO_PREVIEW:
		mate_pixmap_entry_set_preview(pentry, g_value_get_boolean (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* get_property handler for the pixmap entry */
static void
pentry_get_property (GObject *object, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	MatePixmapEntry *pentry;
	MatePixmapEntryPrivate *priv;

	pentry = MATE_PIXMAP_ENTRY (object);
	priv = pentry->_priv;

	switch (param_id) {
	case PROP_DO_PREVIEW:
		g_value_set_boolean (value, priv->do_preview);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
refresh_preview(MatePixmapEntry *pentry)
{
	char *t;
        GdkPixbuf *pixbuf;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	if( ! pentry->_priv->preview)
		return;

	t = mate_file_entry_get_full_path(MATE_FILE_ENTRY(pentry), FALSE);

	if(pentry->_priv->last_preview && t
	   &&
	   strcmp(t,pentry->_priv->last_preview)==0) {
		g_free(t);
		return;
	}
	if(!t || !g_file_test(t,(G_FILE_TEST_IS_SYMLINK & !G_FILE_TEST_IS_DIR)|G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (t, NULL))) {
		if (GTK_IS_IMAGE (pentry->_priv->preview)) {
			gtk_drag_source_unset (pentry->_priv->preview_sw);
			gtk_widget_destroy(pentry->_priv->preview->parent);
			pentry->_priv->preview = gtk_label_new(_("No Image"));
			gtk_widget_show(pentry->_priv->preview);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
							      pentry->_priv->preview);
			g_free(pentry->_priv->last_preview);
			pentry->_priv->last_preview = NULL;
		}
		g_free(t);
		return;
	}
	if(GTK_IS_IMAGE(pentry->_priv->preview)) {
                gtk_image_set_from_pixbuf (GTK_IMAGE (pentry->_priv->preview),
					   pixbuf);
        } else {
		gtk_widget_destroy(pentry->_priv->preview->parent);
		pentry->_priv->preview = gtk_image_new_from_pixbuf (pixbuf);
		_add_atk_name_desc (pentry->_priv->preview,
				    _("Image Preview"),
				    _("A preview of the image currently specified"));

		gtk_widget_show(pentry->_priv->preview);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
						      pentry->_priv->preview);
		if(!GTK_WIDGET_NO_WINDOW(pentry->_priv->preview)) {
			g_signal_connect (G_OBJECT (pentry->_priv->preview), "drag_data_get",
					  G_CALLBACK (drag_data_get), pentry);
			gtk_drag_source_set (pentry->_priv->preview,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     NULL, 0,
					     GDK_ACTION_COPY);
			gtk_drag_source_add_uri_targets (pentry->_priv->preview);
		}
		g_signal_connect (G_OBJECT (pentry->_priv->preview->parent), "drag_data_get",
				  G_CALLBACK (drag_data_get), pentry);
		gtk_drag_source_set (pentry->_priv->preview->parent,
				     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
				     NULL, 0,
				     GDK_ACTION_COPY);
		gtk_drag_source_add_uri_targets (pentry->_priv->preview->parent);
	}
	g_free(pentry->_priv->last_preview);
	pentry->_priv->last_preview = t;
        g_object_unref (G_OBJECT (pixbuf));
}

static int change_timeout;
static GSList *changed_pentries = NULL;

static int
changed_timeout_func(gpointer data)
{
	GSList *li,*tmp;

	tmp = changed_pentries;
	changed_pentries = NULL;
	if(tmp) {
		GDK_THREADS_ENTER();
		for(li=tmp;li!=NULL;li=g_slist_next(li)) {
			refresh_preview(li->data);
		}
		GDK_THREADS_LEAVE();
		g_slist_free(tmp);
		return TRUE;
	}
	change_timeout = 0;

	return FALSE;
}

static void
entry_changed(GtkWidget *w, MatePixmapEntry *pentry)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	if(change_timeout == 0) {
		refresh_preview(pentry);
		change_timeout =
			gtk_timeout_add(1000,changed_timeout_func,NULL);
	} else {
		if(!g_slist_find(changed_pentries,pentry))
			changed_pentries = g_slist_prepend(changed_pentries,
							   pentry);
	}
}

static void
setup_preview(GtkTreeSelection *selection, GtkWidget *widget)
{
	const char *p;
	GList *l;
	GtkWidget *pp = NULL;
        GdkPixbuf *pixbuf, *scaled;
	int w,h;
	GtkWidget *frame, *fw;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	if (GTK_IS_FILE_CHOOSER (widget)) {
		fw = widget;
		frame = gtk_file_chooser_get_preview_widget (GTK_FILE_CHOOSER (fw));

		p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (fw));
	} else {
		frame = g_object_get_data (G_OBJECT (widget), "frame");
		fw = g_object_get_data (G_OBJECT (frame), "fs");

		p = gtk_file_selection_get_filename(GTK_FILE_SELECTION (fw));
	}

	if((l = gtk_container_get_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);

	if(!p || !g_file_test(p,(G_FILE_TEST_IS_SYMLINK & !G_FILE_TEST_IS_DIR) |G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (p, NULL)))
		return;

	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	if(w>h) {
		if(w>100) {
			h = h*(100.0/w);
			w = 100;
		}
	} else {
		if(h>100) {
			w = w*(100.0/h);
			h = 100;
		}
	}
	scaled = gdk_pixbuf_scale_simple
		(pixbuf, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (pixbuf));
	pp = gtk_image_new_from_pixbuf (scaled);
        g_object_unref (G_OBJECT (scaled));

	gtk_widget_show(pp);

	gtk_container_add(GTK_CONTAINER(frame),pp);
}

static void
pentry_destroy (GtkObject *object)
{
	MatePixmapEntry *pentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (object));

	/* remember, destroy can be run multiple times! */

	pentry = MATE_PIXMAP_ENTRY (object);

	changed_pentries = g_slist_remove (changed_pentries, pentry);

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
pentry_finalize (GObject *object)
{
	MatePixmapEntry *pentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (object));

	pentry = MATE_PIXMAP_ENTRY (object);

	g_free (pentry->_priv->last_preview);
	pentry->_priv->last_preview = NULL;

	g_free (pentry->_priv);
	pentry->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
browse_clicked(MateFileEntry *fentry, MatePixmapEntry *pentry)
{
	GtkWidget *w, *hbox;
	GtkFileSelection *fs;
	GtkFileChooser *fc;
	gchar *path;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (MATE_IS_FILE_ENTRY (fentry));
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	if(!fentry->fsw)
		return;

	if (GTK_IS_FILE_CHOOSER (fentry->fsw)) {
		fc = GTK_FILE_CHOOSER (fentry->fsw);

		if (gtk_file_chooser_get_preview_widget (fc) != NULL)
			return;

		w = gtk_frame_new ("");
		gtk_frame_set_shadow_type (GTK_FRAME (w), GTK_SHADOW_NONE);
		gtk_widget_show (w);
		
		gtk_file_chooser_set_preview_widget (fc, w);

		g_signal_connect (G_OBJECT (fc), "update-preview",
				  G_CALLBACK (setup_preview), fentry->fsw);

		/* the path can be already set */
		if ((path = gtk_file_chooser_get_filename (fc)) != NULL)
			setup_preview (NULL, fentry->fsw);

		g_free (path);
	} else {
		fs = GTK_FILE_SELECTION (fentry->fsw);
		
		/* we already got this */
		if (g_object_get_data (G_OBJECT (fs->file_list), "frame") != NULL)
			return;

		hbox = fs->file_list;
		do {
			hbox = hbox->parent;
			if(!hbox) {
				g_warning(_("Can't find an hbox, using a normal file "
					    "selection"));
				return;
			}
		} while(!GTK_IS_HBOX(hbox));

		w = gtk_frame_new(_("Preview"));
		gtk_widget_show(w);
		gtk_box_pack_end(GTK_BOX(hbox),w,FALSE,FALSE,0);
		gtk_widget_set_size_request (w, 110, 110);
		g_object_set_data (G_OBJECT (w), "fs", fs);

		g_object_set_data (G_OBJECT (fs->file_list), "frame", w);
		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (fs->file_list))), "changed",
				  G_CALLBACK (setup_preview), fs->file_list);
		g_object_set_data (G_OBJECT (fs->selection_entry), "frame", w);
		g_signal_connect (fs->selection_entry, "changed",
				  G_CALLBACK (setup_preview), fs->selection_entry);
	}
}

static void
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		MatePixmapEntry   *pentry)
{
	char *string;
	char *file;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	file = mate_file_entry_get_full_path(MATE_FILE_ENTRY(pentry), TRUE);

	if(!file) {
		/*FIXME: cancel the drag*/
		return;
	}

	string = g_strdup_printf("file:%s\r\n",file);
	g_free(file);
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, (unsigned char *)string, strlen(string)+1);
	g_free(string);
}

static void
turn_on_previewbox(MatePixmapEntry *pentry)
{
	pentry->_priv->preview_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(pentry->_priv->preview_sw);
	/*for some reason we can't do this*/
	/*g_signal_connect (G_OBJECT (pentry->_priv->preview_sw), "drag_data_get",
			    G_CALLBACK (drag_data_get), pentry);*/
	gtk_box_pack_start (GTK_BOX (pentry), pentry->_priv->preview_sw, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (pentry->_priv->preview_sw, 100, 100);
	gtk_widget_show (pentry->_priv->preview_sw);

	pentry->_priv->preview = gtk_label_new(_("No Image"));
	gtk_widget_ref(pentry->_priv->preview);
	gtk_widget_show(pentry->_priv->preview);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
					      pentry->_priv->preview);

	/*just in case there is a default that is an image*/
	refresh_preview(pentry);
}

static void
mate_pixmap_entry_instance_init (MatePixmapEntry *pentry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);

	pentry->_priv = g_new0(MatePixmapEntryPrivate, 1);

	pentry->_priv->do_preview = 0;

	pentry->_priv->last_preview = NULL;

	pentry->_priv->preview = NULL;
	pentry->_priv->preview_sw = NULL;

	g_signal_connect_after (pentry, "browse_clicked",
				G_CALLBACK (browse_clicked),
				pentry);

	p = mate_program_locate_file (NULL /* program */,
				       MATE_FILE_DOMAIN_PIXMAP,
				       ".",
				       FALSE /* only_if_exists */,
				       NULL /* ret_locations */);
	mate_file_entry_set_default_path(MATE_FILE_ENTRY(pentry),p);
	g_free(p);

	w = mate_file_entry_gtk_entry(MATE_FILE_ENTRY(pentry));
	g_signal_connect (G_OBJECT (w), "changed",
			  G_CALLBACK (entry_changed), pentry);

	/*just in case there is a default that is an image*/
	refresh_preview(pentry);
}

/**
 * mate_pixmap_entry_construct:
 * @pentry: A #MatePixmapEntry object to construct
 * @history_id: The id given to #mate_entry_new
 * @browse_dialog_title: Title of the browse dialog
 * @do_preview: %TRUE if preview is desired, %FALSE if not.
 *
 * Description: Constructs the @pentry object.  If do_preview is %FALSE,
 * there is no preview, the files are not loaded and thus are not
 * checked to be real image files.
 **/
void
mate_pixmap_entry_construct (MatePixmapEntry *pentry, const gchar *history_id,
			      const gchar *browse_dialog_title, gboolean do_preview)
{
	mate_file_entry_construct (MATE_FILE_ENTRY (pentry),
				    history_id,
				    browse_dialog_title);

	pentry->_priv->do_preview = do_preview?1:0;
	if(do_preview)
		turn_on_previewbox(pentry);
}

/**
 * mate_pixmap_entry_new:
 * @history_id: The id given to #mate_entry_new
 * @browse_dialog_title: Title of the browse dialog
 * @do_preview: boolean
 *
 * Description: Creates a new pixmap entry widget, if do_preview is false,
 * there is no preview, the files are not loaded and thus are not
 * checked to be real image files.
 *
 * Returns: New MatePixmapEntry object.
 **/
GtkWidget *
mate_pixmap_entry_new (const gchar *history_id, const gchar *browse_dialog_title, gboolean do_preview)
{
	MatePixmapEntry *pentry;

	pentry = g_object_new (MATE_TYPE_PIXMAP_ENTRY, NULL);

	mate_pixmap_entry_construct (pentry, history_id, browse_dialog_title, do_preview);
	return GTK_WIDGET (pentry);
}

#ifndef MATE_DISABLE_DEPRECATED_SOURCE
/**
 * mate_pixmap_entry_mate_file_entry:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Get the #MateFileEntry component of the
 * MatePixmapEntry widget for lower-level manipulation.
 *
 * Returns: #MateFileEntry widget
 **/
GtkWidget *
mate_pixmap_entry_mate_file_entry (MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	return GTK_WIDGET (pentry);
}

/**
 * mate_pixmap_entry_mate_entry:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Get the #MateEntry component of the
 * MatePixmapEntry widget for lower-level manipulation.
 *
 * Returns: #MateEntry widget
 **/
GtkWidget *
mate_pixmap_entry_mate_entry (MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	return mate_file_entry_mate_entry(MATE_FILE_ENTRY(pentry));
}

/**
 * mate_pixmap_entry_gtk_entry:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Get the #GtkEntry component of the
 * MatePixmapEntry for Gtk+-level manipulation.
 *
 * Returns: #GtkEntry widget
 **/
GtkWidget *
mate_pixmap_entry_gtk_entry (MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	return mate_file_entry_gtk_entry
		(MATE_FILE_ENTRY (pentry));
}
#endif /* MATE_DISABLE_DEPRECATED_SOURCE */

/**
 * mate_pixmap_entry_scrolled_window:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Get the #GtkScrolledWindow widget that the preview
 * is contained in.  Could be %NULL
 *
 * Returns: #GtkScrolledWindow widget or %NULL
 **/
GtkWidget *
mate_pixmap_entry_scrolled_window(MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->_priv->preview_sw;
}

/**
 * mate_pixmap_entry_preview_widget:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Get the widget that is the preview.  Don't assume any
 * type of widget.  Currently either #MatePixmap or #GtkLabel, but it
 * could change in the future. Could be %NULL
 *
 * Returns: the preview widget pointer or %NULL
 **/
GtkWidget *
mate_pixmap_entry_preview_widget(MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->_priv->preview;
}

/**
 * mate_pixmap_entry_set_pixmap_subdir:
 * @pentry: Pointer to MatePixmapEntry widget
 * @subdir: Subdirectory
 *
 * Description: Sets the default path for the file entry. The new
 * subdirectory should be specified relative to the default MATE
 * pixmap directory.
 **/
void
mate_pixmap_entry_set_pixmap_subdir(MatePixmapEntry *pentry,
				     const gchar *subdir)
{
	char *p;
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	if(!subdir)
		subdir = ".";

	p = mate_program_locate_file (NULL /* program */,
				       MATE_FILE_DOMAIN_PIXMAP,
				       subdir,
				       FALSE /* only_if_exists */,
				       NULL /* ret_locations */);
	mate_file_entry_set_default_path(MATE_FILE_ENTRY(pentry),p);
	g_free(p);
}

/**
 * mate_pixmap_entry_set_preview:
 * @pentry: Pointer to MatePixmapEntry widget
 * @do_preview: %TRUE to show previews, %FALSE to not show them.
 *
 * Description: Sets whether or not the preview box is shown above
 * the entry.  If the preview is on, we also load the files and check
 * for them being real images.  If it is off, we don't check files
 * to be real image files.
 **/
void
mate_pixmap_entry_set_preview (MatePixmapEntry *pentry, gboolean do_preview)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));

	if(pentry->_priv->do_preview ? 1 : 0 == do_preview ? 1 : 0)
		return;

	pentry->_priv->do_preview = do_preview ? 1 : 0;

	if(do_preview) {
		g_assert(pentry->_priv->preview_sw == NULL);
		g_assert(pentry->_priv->preview == NULL);
		turn_on_previewbox(pentry);
	} else {
		g_assert(pentry->_priv->preview_sw != NULL);
		g_assert(pentry->_priv->preview != NULL);
		gtk_widget_destroy(pentry->_priv->preview_sw);

		gtk_widget_unref(pentry->_priv->preview_sw);
		pentry->_priv->preview_sw = NULL;

		gtk_widget_unref(pentry->_priv->preview);
		pentry->_priv->preview = NULL;
	}
}

/**
 * mate_pixmap_entry_set_preview_size:
 * @pentry: Pointer to MatePixmapEntry widget
 * @preview_w: Preview width in pixels
 * @preview_h: Preview height in pixels
 *
 * Description: Sets the minimum size of the preview frame in pixels.
 * This works only if the preview is enabled.
 **/
void
mate_pixmap_entry_set_preview_size(MatePixmapEntry *pentry,
				    int preview_w,
				    int preview_h)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (MATE_IS_PIXMAP_ENTRY (pentry));
	g_return_if_fail (preview_w>=0 && preview_h>=0);

	if(pentry->_priv->preview_sw)
		gtk_widget_set_size_request (pentry->_priv->preview_sw,
					     preview_w, preview_h);
}

/* Ensures that a pixmap entry is not in the waiting list for a preview update.  */
static void
ensure_update (MatePixmapEntry *pentry)
{
	GSList *l;

	if (change_timeout == 0)
		return;

	l = g_slist_find (changed_pentries, pentry);
	if (!l)
		return;

	refresh_preview (pentry);
	changed_pentries = g_slist_remove_link (changed_pentries, l);
	g_slist_free_1 (l);

	if (!changed_pentries) {
		gtk_timeout_remove (change_timeout);
		change_timeout = 0;
	}
}

/**
 * mate_pixmap_entry_get_filename:
 * @pentry: Pointer to MatePixmapEntry widget
 *
 * Description: Gets the filename of the image if the preview
 * successfully loaded if preview is disabled.  If the preview is
 * disabled, the file is only checked if it exists or not.
 *
 * Returns: Newly allocated string containing path, or %NULL on error.
 **/
char *
mate_pixmap_entry_get_filename(MatePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP_ENTRY (pentry), NULL);

	if (pentry->_priv->do_preview) {
		ensure_update (pentry);

		/*this happens if it doesn't exist or isn't an image*/
		if (!GTK_IS_IMAGE (pentry->_priv->preview))
			return NULL;
	}

	return mate_file_entry_get_full_path (MATE_FILE_ENTRY (pentry), TRUE);
}
