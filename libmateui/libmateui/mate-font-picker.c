/* MATE font picker button.
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved.
 *
 * Based on mate-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <libmate/mate-macros.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "mate-stock-icons.h"
#include "matetypebuiltins.h"

#include "mate-font-picker.h"

#include <string.h>
#include <stdio.h>

struct _MateFontPickerPrivate {
	gchar* title;

	gchar* font_name;
	gchar* preview_text;

	int use_font_in_label_size;

	guint mode: 2; /* MateFontPickerMode */

	/* Only for MATE_FONT_PICKER_MODE_FONT_INFO */
	guint use_font_in_label: 1;
	guint show_size: 1;

	GtkWidget* font_dialog;
	GtkWidget* inside;
	GtkWidget* font_label;
	GtkWidget* vsep;
	GtkWidget* size_label;
};


#define DEF_FONT_NAME N_("sans 12")
#define DEF_PREVIEW_TEXT N_("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz")
#define DEF_TITLE N_("Pick a Font")

/* Signals */
enum {
	FONT_SET,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_TITLE,
	PROP_MODE,
	PROP_FONT_NAME,
	PROP_PREVIEW_TEXT,
	PROP_USE_FONT_IN_LABEL,
	PROP_LABEL_FONT_SIZE,
	PROP_SHOW_SIZE,
};

/* Prototypes */
static void mate_font_picker_destroy(GtkObject* object);
static void mate_font_picker_finalize(GObject* object);
static void mate_font_picker_get_property(GObject* object, guint param_id, GValue* value, GParamSpec* pspec);
static void mate_font_picker_set_property(GObject* object, guint param_id, const GValue* value, GParamSpec* pspec);

static void mate_font_picker_clicked(GtkButton* button);

/* Dialog response functions */
static void mate_font_picker_dialog_ok_clicked(GtkWidget* widget, gpointer data);
static void mate_font_picker_dialog_cancel_clicked(GtkWidget* widget, gpointer data);

static gboolean mate_font_picker_dialog_delete_event(GtkWidget* widget, GdkEventAny* ev, gpointer data);
static void mate_font_picker_dialog_destroy(GtkWidget* widget, gpointer data);

/* Auxiliary functions */
static GtkWidget *mate_font_picker_create_inside(MateFontPicker* gfs);

static void mate_font_picker_label_use_font_in_label(MateFontPicker* gfs);
static void mate_font_picker_update_font_info(MateFontPicker* gfs);


static guint font_picker_signals[LAST_SIGNAL] = {0};

MATE_CLASS_BOILERPLATE(MateFontPicker, mate_font_picker, GtkButton, GTK_TYPE_BUTTON)

static void mate_font_picker_class_init(MateFontPickerClass* class)
{
	GtkObjectClass* object_class;
	GObjectClass* gobject_class;
	GtkButtonClass* button_class;

	object_class = (GtkObjectClass*) class;
	gobject_class = (GObjectClass*) class;
	button_class = (GtkButtonClass*) class;

	object_class->destroy = mate_font_picker_destroy;

	gobject_class->finalize = mate_font_picker_finalize;
	gobject_class->set_property = mate_font_picker_set_property;
	gobject_class->get_property = mate_font_picker_get_property;

	button_class->clicked = mate_font_picker_clicked;

	class->font_set = NULL;

	g_object_class_install_property(gobject_class, PROP_TITLE, g_param_spec_string("title", _("Title"), _("The title of the selection dialog box"), _(DEF_TITLE), (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property(gobject_class, PROP_MODE, g_param_spec_enum("mode", _("Mode"), _("The mode of operation of the font picker"), MATE_TYPE_FONT_PICKER_MODE, MATE_FONT_PICKER_MODE_PIXMAP, (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property(gobject_class, PROP_FONT_NAME, g_param_spec_string("font_name", _("Font name"), _("Name of the selected font"), _(DEF_FONT_NAME), (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property(gobject_class, PROP_PREVIEW_TEXT, g_param_spec_string("preview_text", _("Preview text"), _("Preview text shown in the dialog"), _(DEF_PREVIEW_TEXT), (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property(gobject_class, PROP_USE_FONT_IN_LABEL, g_param_spec_boolean("use-font-in-label", _("Use font in label"), _("Use font in the label in font info mode"), FALSE, (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	/* 8 min, 72 max, 14 default */
	g_object_class_install_property(gobject_class, PROP_LABEL_FONT_SIZE, g_param_spec_int("label-font-size", _("Font size for label"), _("Font size for label in font info mode"), 8 , 72, 14, (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property(gobject_class, PROP_SHOW_SIZE, g_param_spec_boolean("show-size", _("Show size"), _("Show size in font info mode"), TRUE, (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	font_picker_signals[FONT_SET] = g_signal_new("font_set", G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(MateFontPickerClass, font_set), NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void mate_font_picker_instance_init(MateFontPicker *gfp)
{
	gfp->_priv = g_new0(MateFontPickerPrivate, 1);

	/* Initialize fields */
	gfp->_priv->mode = MATE_FONT_PICKER_MODE_PIXMAP;
	gfp->_priv->font_name = NULL;
	gfp->_priv->preview_text = NULL;
	gfp->_priv->use_font_in_label = FALSE;
	gfp->_priv->use_font_in_label_size = 14;
	gfp->_priv->show_size = TRUE;
	gfp->_priv->font_dialog = NULL;
	gfp->_priv->title = g_strdup(_(DEF_TITLE));


	/* Create pixmap or info widgets */
	gfp->_priv->inside = mate_font_picker_create_inside(gfp);

	if (gfp->_priv->inside)
	{
		gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);
	}

	mate_font_picker_set_font_name(gfp, _(DEF_FONT_NAME));
	mate_font_picker_set_preview_text(gfp, _(DEF_PREVIEW_TEXT));

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO)
	{
		/* Update */
		mate_font_picker_update_font_info(gfp);
	}
}

static void mate_font_picker_destroy(GtkObject* object)
{
	MateFontPicker* gfp;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(object));

	/* remember, destroy can be run multiple times! */

	gfp = MATE_FONT_PICKER(object);

	if (gfp->_priv->font_dialog != NULL)
	{
		gtk_widget_destroy(gfp->_priv->font_dialog);
		gfp->_priv->font_dialog = NULL;
	}

	MATE_CALL_PARENT(GTK_OBJECT_CLASS, destroy, (object));
} /* mate_font_picker_destroy */

static void mate_font_picker_finalize(GObject* object)
{
	MateFontPicker* gfp;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(object));

	gfp = MATE_FONT_PICKER(object);

	/* g_free handles NULL */
	g_free(gfp->_priv->font_name);
	gfp->_priv->font_name = NULL;

	/* g_free handles NULL */
	g_free(gfp->_priv->preview_text);
	gfp->_priv->preview_text = NULL;

	/* g_free handles NULL */
	g_free(gfp->_priv->title);
	gfp->_priv->title = NULL;

	/* g_free handles NULL */
	g_free(gfp->_priv);
	gfp->_priv = NULL;

	MATE_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
} /* mate_font_picker_finalize */

static void mate_font_picker_set_property(GObject *object, guint param_id, const GValue* value, GParamSpec* pspec)
{
	MateFontPicker* self;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(object));

	self = MATE_FONT_PICKER(object);

	switch (param_id)
	{
		case PROP_TITLE:
			mate_font_picker_set_title(self, g_value_get_string(value));
			break;
		case PROP_MODE:
			mate_font_picker_set_mode(self, g_value_get_enum(value));
			break;
		case PROP_FONT_NAME:
			mate_font_picker_set_font_name(self, g_value_get_string(value));
			break;
		case PROP_PREVIEW_TEXT:
			mate_font_picker_set_preview_text(self, g_value_get_string(value));
			break;
		case PROP_USE_FONT_IN_LABEL:
			mate_font_picker_fi_set_use_font_in_label(self, g_value_get_boolean(value), self->_priv->use_font_in_label_size);
			break;
		case PROP_LABEL_FONT_SIZE:
			mate_font_picker_fi_set_use_font_in_label(self, self->_priv->use_font_in_label, g_value_get_int(value));
			break;
		case PROP_SHOW_SIZE:
			mate_font_picker_fi_set_show_size(self, g_value_get_boolean(value));
			break;
		default:
			break;
	}
} /* mate_font_picker_set_property */

static void mate_font_picker_get_property(GObject* object, guint param_id, GValue* value, GParamSpec* pspec)
{
	MateFontPicker* self;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(object));

	self = MATE_FONT_PICKER(object);

	switch (param_id)
	{
		case PROP_TITLE:
			g_value_set_string(value, mate_font_picker_get_title(self));
			break;
		case PROP_MODE:
			g_value_set_string(value, mate_font_picker_get_title(self));
			break;
		case PROP_FONT_NAME:
			g_value_set_string(value, mate_font_picker_get_font_name(self));
			break;
		case PROP_PREVIEW_TEXT:
			g_value_set_string(value, mate_font_picker_get_preview_text(self));
			break;
		case PROP_USE_FONT_IN_LABEL:
			g_value_set_boolean(value, self->_priv->use_font_in_label);
			break;
		case PROP_LABEL_FONT_SIZE:
			g_value_set_int(value, self->_priv->use_font_in_label_size);
			break;
		case PROP_SHOW_SIZE:
			g_value_set_boolean(value, self->_priv->show_size);
			break;

		default:
			break;
	}
} /* mate_font_picker_get_property */

/*************************************************************************
 Public functions
 *************************************************************************/


/**
 * mate_font_picker_new
 *
 * Description:
 * Create new font picker widget.
 *
 * Returns:
 * Pointer to new font picker widget.
 */

GtkWidget* mate_font_picker_new(void)
{
	return g_object_new(MATE_TYPE_FONT_PICKER, NULL);
} /* mate_font_picker_new */



/**
 * mate_font_picker_set_title
 * @gfp: Pointer to MATE font picker widget.
 * @title: String containing font selection dialog title.
 *
 * Description:
 * Sets the title for the font selection dialog.  If @title is %NULL,
 * then the default is used.
 */

void mate_font_picker_set_title(MateFontPicker* gfp, const gchar* title)
{
	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));

	if (!title)
	{
		title = _(DEF_TITLE);
	}

	/* g_free handles NULL */
	g_free(gfp->_priv->title);
	gfp->_priv->title = g_strdup(title);

	/* If FontDialog is created change title */
	if (gfp->_priv->font_dialog)
	{
		gtk_window_set_title(GTK_WINDOW(gfp->_priv->font_dialog), gfp->_priv->title);
	}
} /* mate_font_picker_set_title */

/**
 * mate_font_picker_get_title
 * @gfp: Pointer to MATE font picker widget.
 *
 * Description:
 * Retrieve name of the font selection dialog title
 *
 * Returns:
 * Pointer to an internal copy of the title string
 */

const gchar* mate_font_picker_get_title(MateFontPicker* gfp)
{
	g_return_val_if_fail(gfp != NULL, NULL);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), NULL);

	return gfp->_priv->title;
} /* mate_font_picker_get_title */

/**
 * mate_font_picker_get_mode
 * @gfp: Pointer to MATE font picker widget.
 *
 * Description:
 * Returns current font picker button mode (or what to show).  Possible
 * values include %MATE_FONT_PICKER_MODE_PIXMAP,
 * %MATE_FONT_PICKER_MODE_FONT_INFO, and
 * %MATE_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns:
 * Button mode currently set in font picker widget, or
 * %MATE_FONT_PICKER_MODE_UNKNOWN on error.
 */

MateFontPickerMode mate_font_picker_get_mode(MateFontPicker* gfp)
{
	g_return_val_if_fail(gfp != NULL, MATE_FONT_PICKER_MODE_UNKNOWN);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), MATE_FONT_PICKER_MODE_UNKNOWN);

	return gfp->_priv->mode;
} /* mate_font_picker_get_mode */


/**
 * mate_font_picker_set_mode
 * @gfp: Pointer to MATE font picker widget.
 * @mode: Value of subsequent font picker button mode (or what to show)
 *
 * Description:
 * Set value of subsequent font picker button mode (or what to show).
 */

void mate_font_picker_set_mode(MateFontPicker* gfp, MateFontPickerMode mode)
{
	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));
	g_return_if_fail(mode >= 0 && mode < MATE_FONT_PICKER_MODE_UNKNOWN);

	if (gfp->_priv->mode != mode)
	{
		gfp->_priv->mode = mode;

		/* Next sentence will destroy gfp->_priv->inside after removing it */
		gtk_container_remove(GTK_CONTAINER(gfp), gfp->_priv->inside);
		gfp->_priv->inside = mate_font_picker_create_inside(gfp);

		if (gfp->_priv->inside)
		{
			gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);
		}

		if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO)
		{
			/* Update */
			mate_font_picker_update_font_info(gfp);
		}
	}
} /* mate_font_picker_set_mode */


/**
 * mate_font_picker_fi_set_use_font_in_label
 * @gfp: Pointer to MATE font picker widget.
 * @use_font_in_label: If %TRUE, font name will be written using font chosen.
 * @size: Display font using this point size.
 *
 * Description:
 * If @use_font_in_label is %TRUE, font name will be written using font chosen
 * by user and using @size passed to this function.  This only applies if
 * current button mode is %MATE_FONT_PICKER_MODE_FONT_INFO.
 */

void  mate_font_picker_fi_set_use_font_in_label(MateFontPicker* gfp, gboolean use_font_in_label, gint size)
{
	gboolean old_use_font_in_label;
	int old_size;

	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));

	old_use_font_in_label = gfp->_priv->use_font_in_label;
	old_size = gfp->_priv->use_font_in_label_size;
	gfp->_priv->use_font_in_label = use_font_in_label;
	gfp->_priv->use_font_in_label_size = size;

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO && (old_use_font_in_label != use_font_in_label || old_size != size))
	{
		if (gfp->_priv->use_font_in_label)
		{
			mate_font_picker_label_use_font_in_label(gfp);
		}
		else
		{
			gtk_widget_set_style(gfp->_priv->font_label, NULL);
		}
	}
} /* mate_font_picker_fi_set_use_font_in_label */


/**
 * mate_font_picker_fi_set_show_size
 * @gfp: Pointer to MATE font picker widget.
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * Description:
 * If @show_size is %TRUE, font size will be displayed along with font chosen
 * by user.  This only applies if current button mode is
 * %MATE_FONT_PICKER_MODE_FONT_INFO.
 */

void mate_font_picker_fi_set_show_size(MateFontPicker* gfp, gboolean show_size)
{
	gboolean old_show_size;

	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));

	/* Make sure it's normalized to 1/0 */
	old_show_size = gfp->_priv->show_size ? 1 : 0;
	gfp->_priv->show_size = show_size ? 1 : 0;

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO && old_show_size != gfp->_priv->show_size)
	{
		/* Next sentence will destroy gfp->_priv->inside
		 * after removing it */
		if (gfp->_priv->inside != NULL)
		{
			gtk_container_remove(GTK_CONTAINER(gfp), gfp->_priv->inside);
		}

		gfp->_priv->inside = mate_font_picker_create_inside(gfp);

		if (gfp->_priv->inside != NULL)
		{
			gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);
		}

		mate_font_picker_update_font_info(gfp);
	}
} /* mate_font_picker_fi_set_show_size */


/**
 * mate_font_picker_uw_set_widget
 * @gfp: Pointer to MATE font picker widget.
 * @widget: User widget to display for inside of font picker.
 *
 * Set the user-supplied @widget as the inside of the font picker.
 * This only applies with MATE_FONT_PICKER_MODE_USER_WIDGET.
 */

void mate_font_picker_uw_set_widget(MateFontPicker* gfp, GtkWidget* widget)
{
	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_USER_WIDGET && gfp->_priv->inside != widget)
	{
		if (gfp->_priv->inside != NULL)
		{
			gtk_container_remove(GTK_CONTAINER(gfp), gfp->_priv->inside);
		}

		gfp->_priv->inside = widget;

		if (gfp->_priv->inside != NULL)
		{
			gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);
		}
	}
} /* mate_font_picker_uw_set_widget */

/**
 * mate_font_picker_uw_get_widget
 * @gfp: Pointer to MATE font picker widget.
 *
 * Get the user-supplied widget inside of the font picker.
 * This only applies with MATE_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns: a #GtkWidget, or %NULL if
 * not in MATE_FONT_PICKER_MODE_USER_WIDGET mode
 */

GtkWidget* mate_font_picker_uw_get_widget(MateFontPicker* gfp)
{
	g_return_val_if_fail(gfp != NULL, NULL);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), NULL);

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_USER_WIDGET)
	{
		return gfp->_priv->inside;
	}
	else
	{
		return NULL;
	}
} /* mate_font_picker_uw_set_widget */

/**
 * mate_font_picker_get_font_name
 * @gfp: Pointer to MATE font picker widget.
 *
 * Description:
 * Retrieve name of font from font selection dialog.
 *
 * Returns:
 * Pointer to an internal copy of the font name.
 */

const gchar* mate_font_picker_get_font_name(MateFontPicker* gfp)
{
	g_return_val_if_fail(gfp != NULL, NULL);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), NULL);

	if (gfp->_priv->font_dialog)
	{
		/* g_free handles NULL */
		g_free(gfp->_priv->font_name);

		#if GTK_CHECK_VERSION(3, 2, 0)
			gfp->_priv->font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog));
		#else
			gfp->_priv->font_name = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog));
		#endif
	}

	return gfp->_priv->font_name;
} /* mate_font_picker_get_font_name */

/**
 * mate_font_picker_set_font_name
 * @gfp: Pointer to MATE font picker widget.
 * @fontname: Name of font to display in font selection dialog
 *
 * Description:
 * Set or update currently-displayed font in font picker dialog.
 *
 * Returns:
 * Return value of gtk_font_selection_dialog_set_font_name if the
 * font selection dialog exists, otherwise %FALSE.
 */

gboolean mate_font_picker_set_font_name(MateFontPicker* gfp, const gchar* fontname)
{
	g_return_val_if_fail(gfp != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), FALSE);
	g_return_val_if_fail(fontname != NULL, FALSE);

	if (gfp->_priv->font_name != fontname)
	{
		g_free(gfp->_priv->font_name);
		gfp->_priv->font_name = g_strdup(fontname);
	}

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO)
	mate_font_picker_update_font_info(gfp);

	if (gfp->_priv->font_dialog)
	{
		#if GTK_CHECK_VERSION(3, 2, 0)
			gtk_font_chooser_set_font(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog), gfp->_priv->font_name);

			return TRUE;
		#else
			return gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->font_name);
		#endif
	}
	else
	{
		return FALSE;
	}
} /* mate_font_picker_set_font_name */


/**
 * mate_font_picker_get_preview_text
 * @gfp: Pointer to MATE font picker widget.
 *
 * Description:
 * Retrieve preview text from font selection dialog if available.
 *
 * Returns:
 * Reference to internal copy of preview text string, or %NULL if no
 * font dialog is being displayed.
 */

const gchar* mate_font_picker_get_preview_text(MateFontPicker* gfp)
{
	g_return_val_if_fail(gfp != NULL, NULL);
	g_return_val_if_fail(MATE_IS_FONT_PICKER(gfp), NULL);

	if (gfp->_priv->font_dialog)
	{
		/* g_free handles NULL */
		g_free(gfp->_priv->preview_text);

		#if GTK_CHECK_VERSION(3, 2, 0)
			gfp->_priv->preview_text = g_strdup(gtk_font_chooser_get_preview_text(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog)));
		#else
			gfp->_priv->preview_text = g_strdup(gtk_font_selection_dialog_get_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog)));
		#endif
	}

	return gfp->_priv->preview_text;
} /* mate_font_picker_get_preview_text */


/**
 * mate_font_picker_set_preview_text
 * @gfp: Pointer to MATE font picker widget.
 * @text: New preview text
 *
 * Description:
 * Set preview text in font picker, and in font selection dialog if one
 * is being displayed.
 */

void mate_font_picker_set_preview_text(MateFontPicker* gfp, const gchar* text)
{
	g_return_if_fail(gfp != NULL);
	g_return_if_fail(MATE_IS_FONT_PICKER(gfp));
	g_return_if_fail(text != NULL);

	if (gfp->_priv->preview_text != text)
	{
		g_free(gfp->_priv->preview_text);
		gfp->_priv->preview_text = g_strdup(text);
	}

	if (gfp->_priv->font_dialog)
	{
		#if GTK_CHECK_VERSION(3, 2, 0)
			gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog), gfp->_priv->preview_text);
		#else
			gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->preview_text);
		#endif
	}
} /* mate_font_picker_set_preview_text */

/* ************************************************************************
 Internal functions
 **************************************************************************/

static void mate_font_picker_clicked(GtkButton* button)
{
	MateFontPicker* gfp;

	#if GTK_CHECK_VERSION(3, 2, 0)
		GtkFontChooserDialog* fsd;
	#else
		GtkFontSelectionDialog* fsd;
	#endif

	gfp = MATE_FONT_PICKER(button);

	if (!gfp->_priv->font_dialog)
	{
		GtkWidget* parent;

		parent = gtk_widget_get_toplevel(GTK_WIDGET(gfp));

		#if GTK_CHECK_VERSION(3, 2, 0)
			gfp->_priv->font_dialog = gtk_font_chooser_dialog_new(gfp->_priv->title, GTK_WINDOW(parent));
		#else
			gfp->_priv->font_dialog = gtk_font_selection_dialog_new(gfp->_priv->title);

			if (parent)
			{
				gtk_window_set_transient_for(GTK_WINDOW(gfp->_priv->font_dialog), GTK_WINDOW(parent));
			}

		#endif

		#if GTK_CHECK_VERSION(3, 2, 0)
			fsd = GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog);
		#else
			fsd = GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog);
		#endif

		/* If there is a grabed window, set new dialog as modal */
		if (gtk_grab_get_current())
		{
			gtk_window_set_modal(GTK_WINDOW(gfp->_priv->font_dialog), TRUE);
		}

		g_signal_connect(fsd->ok_button, "clicked", G_CALLBACK(mate_font_picker_dialog_ok_clicked), gfp);
		g_signal_connect(fsd->cancel_button, "clicked", G_CALLBACK(mate_font_picker_dialog_cancel_clicked), gfp);
		g_signal_connect(fsd,"delete_event", G_CALLBACK(mate_font_picker_dialog_delete_event), gfp);
		g_signal_connect(fsd, "destroy", G_CALLBACK(mate_font_picker_dialog_destroy), gfp);
	} /* if */

	if (!GTK_WIDGET_VISIBLE(gfp->_priv->font_dialog))
	{
		/* Set font and preview text */
		#if GTK_CHECK_VERSION(3, 2, 0)
			gtk_font_chooser_set_font(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog), gfp->_priv->font_name);
			gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER_DIALOG(gfp->_priv->font_dialog), gfp->_priv->preview_text);
		#else
			gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->font_name);
			gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->preview_text);
		#endif

		gtk_widget_show(gfp->_priv->font_dialog);
	}
	else if (gfp->_priv->font_dialog->window)
	{
		/*raise the window so that if it is obscured that we see it*/
		gdk_window_raise(gfp->_priv->font_dialog->window);
	}/* if */
} /* mate_font_picker_clicked */

static void mate_font_picker_dialog_ok_clicked(GtkWidget* widget, gpointer data)
{
	MateFontPicker* gfp;

	gfp = MATE_FONT_PICKER(data);

	gtk_widget_hide(gfp->_priv->font_dialog);

	/* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
	mate_font_picker_get_font_name(gfp);
	mate_font_picker_get_preview_text(gfp);

	/* Set label font */
	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO)
	{
		mate_font_picker_update_font_info(gfp);
	}

	/* Emit font_set signal */
	g_signal_emit(gfp, font_picker_signals[FONT_SET], 0, gfp->_priv->font_name);
} /* mate_font_picker_dialog_ok_clicked */


static void mate_font_picker_dialog_cancel_clicked(GtkWidget* widget, gpointer data)
{
	MateFontPicker* gfp;

	gfp = MATE_FONT_PICKER(data);

	gtk_widget_hide(gfp->_priv->font_dialog);

	/* FIXME: is this necessary? bug #67587 */
	/* Restore old values */
	mate_font_picker_set_font_name(gfp,gfp->_priv->font_name);
	mate_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

} /* mate_font_picker_dialog_cancel_clicked */

static gboolean mate_font_picker_dialog_delete_event(GtkWidget* widget, GdkEventAny* ev, gpointer data)
{
	MateFontPicker* gfp;

	gfp = MATE_FONT_PICKER(data);

	/* FIXME: is this necessary? bug #67587 */
	/* Here we restore old values */
	mate_font_picker_set_font_name(gfp,gfp->_priv->font_name);
	mate_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

	return FALSE;
} /* mate_font_picker_dialog_delete_event */

static void mate_font_picker_dialog_destroy(GtkWidget* widget, gpointer data)
{
	MateFontPicker* gfp;

	gfp = MATE_FONT_PICKER(data);

	/* Query GtkFontSelectionDialog before it get destroyed */
	/* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
	mate_font_picker_get_font_name(gfp);
	mate_font_picker_get_preview_text(gfp);

	/* Dialog will get destroyed so reference is no valid now */
	gfp->_priv->font_dialog = NULL;
} /* mate_font_picker_dialog_destroy */

static GtkWidget* mate_font_picker_create_inside(MateFontPicker* gfp)
{
	GtkWidget* widget;

	if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_PIXMAP)
	{
		widget=gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT, GTK_ICON_SIZE_BUTTON);
		gtk_widget_show(widget);
	}
	else if (gfp->_priv->mode == MATE_FONT_PICKER_MODE_FONT_INFO)
	{
		widget=gtk_hbox_new(FALSE,0);

		gfp->_priv->font_label = gtk_label_new(_("Font"));

		gtk_label_set_justify(GTK_LABEL(gfp->_priv->font_label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(widget), gfp->_priv->font_label, TRUE, TRUE, 5);

		if (gfp->_priv->show_size)
		{
			gfp->_priv->vsep = gtk_vseparator_new();
			gtk_box_pack_start(GTK_BOX(widget), gfp->_priv->vsep, FALSE, FALSE, 0);

			gfp->_priv->size_label = gtk_label_new("14");
			gtk_box_pack_start(GTK_BOX(widget), gfp->_priv->size_label, FALSE, FALSE, 5);
		}

		gtk_widget_show_all(widget);

	}
	else /* MATE_FONT_PICKER_MODE_USER_WIDGET */
	{
		widget = NULL;
	}

	return widget;

} /* mate_font_picker_create_inside */

static void mate_font_picker_label_use_font_in_label(MateFontPicker* gfp)
{
	PangoFontDescription* desc;

	desc = pango_font_description_from_string(gfp->_priv->font_name);

	if (desc == NULL)
	{
		return; /* Use widget default */
	}

	/* Change size */
	pango_font_description_set_size(desc, gfp->_priv->use_font_in_label_size * PANGO_SCALE);
	/* Modify font style */
	gtk_widget_modify_font(gfp->_priv->font_label, desc);

	pango_font_description_free(desc);
} /* mate_font_picker_set_label_font */

static void mate_font_picker_update_font_info(MateFontPicker* gfp)
{
	PangoFontDescription* desc;
	const char* family;

	desc = pango_font_description_from_string(gfp->_priv->font_name);

	if (desc == NULL)
	{
		/* FIXME: eek does this ever happen? probably
		 * so we should handle it somehow*/
		g_warning("eeek!");
		return;
	}

	family = pango_font_description_get_family(desc);

	gtk_label_set_text(GTK_LABEL(gfp->_priv->font_label), family);

	/* Extract font size */
	if (gfp->_priv->show_size)
	{
		int size = pango_font_description_get_size(desc);
		char* size_str = g_strdup_printf("%d", size / PANGO_SCALE);

		gtk_label_set_text(GTK_LABEL(gfp->_priv->size_label), size_str);

		g_free(size_str);
	}

	if (gfp->_priv->use_font_in_label)
	{
		mate_font_picker_label_use_font_in_label(gfp);
	}

	pango_font_description_free(desc);
} /* mate_font_picker_update_font_info */
