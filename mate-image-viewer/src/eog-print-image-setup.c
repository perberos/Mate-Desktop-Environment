/* Eye Of MATE -- Print Dialog Custom Widget
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>

#include <glib/gi18n.h>
#include <glib/gprintf.h>

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include "eog-print-image-setup.h"
#include "eog-print-preview.h"

/**
 * SECTION:
 * @Title: Printing Custom Widget
 * @Short_Description: a custom widget to setup image prints.
 * @Stability_Level: Internal
 * @See_Also: #EogPrintPreview
 *
 * This widget is to be used as the custom widget in a #GtkPrintUnixDialog in
 * EOG. Through it, you can set the position and scaling of a image
 * interactively.
 */

#define EOG_PRINT_IMAGE_SETUP_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_PRINT_IMAGE_SETUP, EogPrintImageSetupPrivate))

G_DEFINE_TYPE (EogPrintImageSetup, eog_print_image_setup, GTK_TYPE_TABLE);

struct EogPrintImageSetupPrivate {
	GtkWidget *left;
	GtkWidget *right;
	GtkWidget *top;
	GtkWidget *bottom;

	GtkWidget *center;

	GtkWidget *width;
	GtkWidget *height;

	GtkWidget *scaling;
	GtkWidget *unit;

	GtkUnit current_unit;

	EogImage *image;
	GtkPageSetup *page_setup;

	GtkWidget *preview;
};

enum {
	PROP_0,
	PROP_IMAGE,
	PROP_PAGE_SETUP
};

enum {
	CENTER_NONE,
	CENTER_HORIZONTAL,
	CENTER_VERTICAL,
	CENTER_BOTH
};

enum {
	CHANGE_HORIZ,
	CHANGE_VERT
};

enum {
	UNIT_INCH,
	UNIT_MM
};

#define FACTOR_INCH_TO_MM 25.4
#define FACTOR_INCH_TO_PIXEL 72.
#define FACTOR_MM_TO_INCH 0.03937007874015748
#define FACTOR_MM_TO_PIXEL 2.834645669

static void eog_print_image_setup_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void eog_print_image_setup_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void on_left_value_changed   (GtkSpinButton *spinbutton, gpointer user_data);
static void on_right_value_changed  (GtkSpinButton *spinbutton, gpointer user_data);
static void on_top_value_changed    (GtkSpinButton *spinbutton, gpointer user_data);
static void on_bottom_value_changed (GtkSpinButton *spinbutton, gpointer user_data);

static void on_width_value_changed  (GtkSpinButton *spinbutton, gpointer user_data);
static void on_height_value_changed (GtkSpinButton *spinbutton, gpointer user_data);


static void
block_handlers (EogPrintImageSetup *setup)
{
	EogPrintImageSetupPrivate *priv = setup->priv;

	g_signal_handlers_block_by_func (priv->left, on_left_value_changed, setup);
	g_signal_handlers_block_by_func (priv->right, on_right_value_changed, setup);
	g_signal_handlers_block_by_func (priv->width, on_width_value_changed, setup);
	g_signal_handlers_block_by_func (priv->top, on_top_value_changed, setup);
	g_signal_handlers_block_by_func (priv->bottom, on_bottom_value_changed, setup);
	g_signal_handlers_block_by_func (priv->height, on_height_value_changed, setup);
}

static void
unblock_handlers (EogPrintImageSetup *setup)
{
	EogPrintImageSetupPrivate *priv = setup->priv;

	g_signal_handlers_unblock_by_func (priv->left, on_left_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->right, on_right_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->width, on_width_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->top, on_top_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->bottom, on_bottom_value_changed, setup);
	g_signal_handlers_unblock_by_func (priv->height, on_height_value_changed, setup);
}

static gdouble
get_scale_to_px_factor (EogPrintImageSetup *setup)
{
	gdouble factor = 0.;

	switch (setup->priv->current_unit) {
	case GTK_UNIT_MM:
		factor = FACTOR_MM_TO_PIXEL;
		break;
	case GTK_UNIT_INCH:
		factor = FACTOR_INCH_TO_PIXEL;
		break;
	default:
		g_assert_not_reached ();
	}

	return factor;
}

static gdouble
get_max_percentage (EogPrintImageSetup *setup)
{
	EogPrintImageSetupPrivate *priv = setup->priv;
	gdouble p_width, p_height;
	gdouble width, height;
	gint pix_width, pix_height;
	gdouble perc;

	p_width = gtk_page_setup_get_page_width (priv->page_setup, GTK_UNIT_INCH);
	p_height = gtk_page_setup_get_page_height (priv->page_setup, GTK_UNIT_INCH);

	eog_image_get_size (priv->image, &pix_width, &pix_height);

	width  = (gdouble)pix_width/FACTOR_INCH_TO_PIXEL;
	height = (gdouble)pix_height/FACTOR_INCH_TO_PIXEL;

	if (p_width > width && p_height > height) {
		perc = 1.;
	} else {
		perc = MIN (p_width/width, p_height/height);
	}

	return perc;
}

static void
center (gdouble page_width,
	gdouble width,
	GtkSpinButton *s_left,
	GtkSpinButton *s_right)
{
	gdouble left, right;

	left = (page_width - width)/2;
	right = page_width - left - width;
	gtk_spin_button_set_value (s_left, left);
	gtk_spin_button_set_value (s_right, right);
}

static void
on_center_changed (GtkComboBox *combobox,
		   gpointer user_data)
{
	EogPrintImageSetup *setup;
	EogPrintImageSetupPrivate *priv;
	gint active;

	setup = EOG_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	active = gtk_combo_box_get_active (combobox);

	switch (active) {
	case CENTER_HORIZONTAL:
		center (gtk_page_setup_get_page_width (priv->page_setup,
						       priv->current_unit),
			gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->width)),
			GTK_SPIN_BUTTON (priv->left),
			GTK_SPIN_BUTTON (priv->right));
		break;
	case CENTER_VERTICAL:
		center (gtk_page_setup_get_page_height (priv->page_setup,
							priv->current_unit),
			gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->height)),
			GTK_SPIN_BUTTON (priv->top),
			GTK_SPIN_BUTTON (priv->bottom));
		break;
	case CENTER_BOTH:
		center (gtk_page_setup_get_page_width (priv->page_setup,
						       priv->current_unit),
			gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->width)),
			GTK_SPIN_BUTTON (priv->left),
			GTK_SPIN_BUTTON (priv->right));
		center (gtk_page_setup_get_page_height (priv->page_setup,
							priv->current_unit),
			gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->height)),
			GTK_SPIN_BUTTON (priv->top),
			GTK_SPIN_BUTTON (priv->bottom));
		break;
	case CENTER_NONE:
	default:
		break;
	}

	gtk_combo_box_set_active (combobox, active);
}

static void
update_image_pos_ranges (EogPrintImageSetup *setup,
			 gdouble page_width,
			 gdouble page_height,
			 gdouble width,
			 gdouble height)
{
	EogPrintImageSetupPrivate *priv;

	priv = setup->priv;

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->left),
				   0, page_width - width);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->right),
				   0, page_width - width);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->top),
				   0, page_height - height);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->bottom),
				   0, page_height - height);
}

static gboolean
on_scale_changed (GtkRange     *range,
		  gpointer      user_data)
{
	gdouble scale;
	gdouble width, height;
	gint pix_width, pix_height;
	gdouble left, right, top, bottom;
	gdouble page_width, page_height;
	EogPrintImageSetupPrivate *priv;
	EogPrintImageSetup *setup;
	gdouble factor;
	EogImage *image;

	setup = EOG_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->center), CENTER_NONE);

	image = priv->image;
	eog_image_get_size (image, &pix_width, &pix_height);

	factor = get_scale_to_px_factor (setup);

	width = (gdouble)pix_width/factor;
	height = (gdouble)pix_height/factor;

	left = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->left));
	top = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->top));

	scale = CLAMP (0.01*gtk_range_get_value (range), 0, get_max_percentage (setup));

 	eog_print_preview_set_scale (EOG_PRINT_PREVIEW (priv->preview), scale);

	width  *= scale;
	height *= scale;

	page_width = gtk_page_setup_get_page_width (priv->page_setup, priv->current_unit);
	page_height = gtk_page_setup_get_page_height (priv->page_setup, priv->current_unit);

	update_image_pos_ranges (setup, page_width, page_height, width, height);

	right = page_width - left - width;
	bottom = page_height - top - height;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->width), width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->height), height);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->right), right);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->bottom), bottom);

	return FALSE;
}

static gchar *
on_scale_format_value (GtkScale *scale,
		       gdouble value)
{
	return g_strdup_printf ("%i%%", (gint)value);
}

static void
position_values_changed (EogPrintImageSetup *setup,
			 GtkWidget *w_changed,
			 GtkWidget *w_to_update,
			 GtkWidget *w_size,
			 gdouble total_size,
			 gint change)
{
	EogPrintImageSetupPrivate *priv;
	gdouble changed, to_update, size;
	gdouble pos;

	priv = setup->priv;
	size = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w_size));
	changed = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w_changed));

	to_update = total_size - changed - size;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w_to_update), to_update);
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->center), CENTER_NONE);

	switch (change) {
	case CHANGE_HORIZ:
		pos = gtk_spin_button_get_value (GTK_SPIN_BUTTON (setup->priv->left));
		if (setup->priv->current_unit == GTK_UNIT_MM) {
			pos *= FACTOR_MM_TO_INCH;
		}
 		eog_print_preview_set_image_position (EOG_PRINT_PREVIEW (priv->preview), pos, -1);
		break;
	case CHANGE_VERT:
		pos = gtk_spin_button_get_value (GTK_SPIN_BUTTON (setup->priv->top));
		if (setup->priv->current_unit == GTK_UNIT_MM) {
			pos *= FACTOR_MM_TO_INCH;
		}
		eog_print_preview_set_image_position (EOG_PRINT_PREVIEW (priv->preview), -1, pos);
		break;
	}
}

static void
on_left_value_changed (GtkSpinButton *spinbutton,
		       gpointer       user_data)
{
	EogPrintImageSetup *setup;
	EogPrintImageSetupPrivate *priv;

	setup = EOG_PRINT_IMAGE_SETUP (user_data);
	priv = setup->priv;

	position_values_changed (setup,
				 priv->left, priv->right, priv->width,
				 gtk_page_setup_get_page_width (priv->page_setup,
								priv->current_unit),
				 CHANGE_HORIZ);
}

static void
on_right_value_changed (GtkSpinButton *spinbutton,
			gpointer       user_data)
{
	EogPrintImageSetupPrivate *priv;

	priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOG_PRINT_IMAGE_SETUP (user_data),
				 priv->right, priv->left, priv->width,
				 gtk_page_setup_get_page_width (priv->page_setup,
								priv->current_unit),
				 CHANGE_HORIZ);
}

static void
on_top_value_changed (GtkSpinButton *spinbutton,
		      gpointer       user_data)
{
	EogPrintImageSetupPrivate *priv;

	priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOG_PRINT_IMAGE_SETUP (user_data),
				 priv->top, priv->bottom, priv->height,
				 gtk_page_setup_get_page_height (priv->page_setup,
								 priv->current_unit),
				 CHANGE_VERT);
}

static void
on_bottom_value_changed (GtkSpinButton *spinbutton,
			 gpointer       user_data)
{
	EogPrintImageSetupPrivate *priv;

	priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;

	position_values_changed (EOG_PRINT_IMAGE_SETUP (user_data),
				 priv->bottom, priv->top, priv->height,
				 gtk_page_setup_get_page_height (priv->page_setup,
								 priv->current_unit),
				 CHANGE_VERT);
}

static void
size_changed (EogPrintImageSetup *setup,
	      GtkWidget *w_size_x,
	      GtkWidget *w_size_y,
	      GtkWidget *w_margin_x_1,
	      GtkWidget *w_margin_x_2,
	      GtkWidget *w_margin_y_1,
	      GtkWidget *w_margin_y_2,
	      gdouble page_size_x,
	      gdouble page_size_y,
	      gint change)
{
	EogPrintImageSetupPrivate *priv;
	gdouble margin_x_1, margin_x_2;
	gdouble margin_y_1, margin_y_2;
	gdouble orig_size_x = -1, orig_size_y = -1, scale;
	gdouble size_x, size_y;
	gint pix_width, pix_height;
	gdouble factor;

	priv = setup->priv;

	size_x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w_size_x));
	margin_x_1 = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w_margin_x_1));
	margin_y_1 = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w_margin_y_1));

	eog_image_get_size (priv->image, &pix_width, &pix_height);

	factor = get_scale_to_px_factor (setup);

	switch (change) {
	case CHANGE_HORIZ:
		orig_size_x = (gdouble) pix_width / factor;
		orig_size_y = (gdouble) pix_height / factor;
		break;
	case CHANGE_VERT:
		orig_size_y = (gdouble) pix_width / factor;
		orig_size_x = (gdouble) pix_height / factor;
		break;
	}

	scale = CLAMP (size_x / orig_size_x, 0, 1);

	size_y = scale * orig_size_y;

	margin_x_2 = page_size_x - margin_x_1 - size_x;
	margin_y_2 = page_size_y - margin_y_1 - size_y;

 	eog_print_preview_set_scale (EOG_PRINT_PREVIEW (priv->preview), scale);

	switch (change) {
	case CHANGE_HORIZ:
		update_image_pos_ranges (setup, page_size_x, page_size_y, size_x, size_y);
		break;
	case CHANGE_VERT:
		update_image_pos_ranges (setup, page_size_y, page_size_x, size_y, size_x);
		break;
	}

	gtk_range_set_value (GTK_RANGE (priv->scaling), 100*scale);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w_margin_x_2), margin_x_2);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w_size_y), size_y);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w_margin_y_2), margin_y_2);

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->center), CENTER_NONE);
}

static void
on_width_value_changed (GtkSpinButton *spinbutton,
			gpointer       user_data)
{
	EogPrintImageSetupPrivate *priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;

	size_changed (EOG_PRINT_IMAGE_SETUP (user_data),
		      priv->width, priv->height,
		      priv->left, priv->right,
		      priv->top, priv->bottom,
		      gtk_page_setup_get_page_width (priv->page_setup,
						     priv->current_unit),
		      gtk_page_setup_get_page_height (priv->page_setup,
						      priv->current_unit),
		      CHANGE_HORIZ);
}

static void
on_height_value_changed (GtkSpinButton *spinbutton,
			 gpointer       user_data)
{
	EogPrintImageSetupPrivate *priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;

	size_changed (EOG_PRINT_IMAGE_SETUP (user_data),
		      priv->height, priv->width,
		      priv->top, priv->bottom,
		      priv->left, priv->right,
		      gtk_page_setup_get_page_height (priv->page_setup,
						     priv->current_unit),
		      gtk_page_setup_get_page_width (priv->page_setup,
						     priv->current_unit),
		      CHANGE_VERT);
}

static void
change_unit (GtkSpinButton *spinbutton,
	     gdouble factor,
	     gint digits,
	     gdouble step,
	     gdouble page)
{
	gdouble value;
	gdouble range;

	gtk_spin_button_get_range (spinbutton, NULL, &range);
	range *= factor;

	value = gtk_spin_button_get_value (spinbutton);
	value *= factor;

	gtk_spin_button_set_range (spinbutton, 0, range);
	gtk_spin_button_set_value (spinbutton, value);
	gtk_spin_button_set_digits (spinbutton, digits);
	gtk_spin_button_set_increments  (spinbutton, step, page);
}

static void
set_scale_unit (EogPrintImageSetup *setup,
		GtkUnit unit)
{
	EogPrintImageSetupPrivate *priv = setup->priv;
	gdouble factor;
	gdouble step, page;
	gint digits;

	if (G_UNLIKELY (priv->current_unit == unit))
		return;

	switch (unit) {
	case GTK_UNIT_MM:
		factor = FACTOR_INCH_TO_MM;
		digits = 0;
		step = 1;
		page = 10;
		break;
	case GTK_UNIT_INCH:
		factor = FACTOR_MM_TO_INCH;
		digits = 2;
		step = 0.01;
		page = 0.1;
		break;
	default:
		g_assert_not_reached ();
	}

 	block_handlers (setup);

	change_unit (GTK_SPIN_BUTTON (priv->width), factor, digits, step, page);
	change_unit (GTK_SPIN_BUTTON (priv->height), factor, digits, step, page);
	change_unit (GTK_SPIN_BUTTON (priv->left), factor, digits, step, page);
	change_unit (GTK_SPIN_BUTTON (priv->right), factor, digits, step, page);
	change_unit (GTK_SPIN_BUTTON (priv->top), factor, digits, step, page);
	change_unit (GTK_SPIN_BUTTON (priv->bottom), factor, digits, step, page);

 	unblock_handlers (setup);

	priv->current_unit = unit;
}

static void
on_unit_changed (GtkComboBox *combobox,
		 gpointer user_data)
{
	GtkUnit unit = GTK_UNIT_INCH;

	switch (gtk_combo_box_get_active (combobox)) {
	case UNIT_INCH:
		unit = GTK_UNIT_INCH;
		break;
	case UNIT_MM:
		unit = GTK_UNIT_MM;
		break;
	default:
		g_assert_not_reached ();
	}

	set_scale_unit (EOG_PRINT_IMAGE_SETUP (user_data), unit);
}

static void
on_preview_image_moved (EogPrintPreview *preview,
			gpointer user_data)
{
	EogPrintImageSetupPrivate *priv = EOG_PRINT_IMAGE_SETUP (user_data)->priv;
	gdouble x, y;

	eog_print_preview_get_image_position (preview, &x, &y);

	if (priv->current_unit == GTK_UNIT_MM) {
		x *= FACTOR_INCH_TO_MM;
		y *= FACTOR_INCH_TO_MM;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->left), x);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->top), y);
}

/* Function taken from gtkprintunixdialog.c */
static GtkWidget *
wrap_in_frame (const gchar *label,
               GtkWidget   *child)
{
	GtkWidget *frame, *alignment, *label_widget;
	gchar *bold_text;

	label_widget = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.5);
	gtk_widget_show (label_widget);

	bold_text = g_markup_printf_escaped ("<b>%s</b>", label);
	gtk_label_set_markup (GTK_LABEL (label_widget), bold_text);
	g_free (bold_text);

	frame = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (frame), label_widget, FALSE, FALSE, 0);

	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
				   0, 0, 12, 0);
	gtk_box_pack_start (GTK_BOX (frame), alignment, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (alignment), child);

	gtk_widget_show (frame);
	gtk_widget_show (alignment);

	return frame;
}

static GtkWidget *
table_attach_spin_button_with_label (GtkWidget *table,
				     const gchar* text_label,
				     gint left, gint top)
{
	GtkWidget *label, *spin_button;

	label = gtk_label_new_with_mnemonic (text_label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	spin_button = gtk_spin_button_new_with_range (0, 100, 0.01);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin_button), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (spin_button), 6);
	gtk_table_attach (GTK_TABLE (table), label, left, left + 1,
			  top, top + 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach (GTK_TABLE (table), spin_button, left + 1, left + 2,
			  top, top + 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);

	return spin_button;
}

static void
eog_print_image_setup_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	EogPrintImageSetup *setup = EOG_PRINT_IMAGE_SETUP (object);
	EogPrintImageSetupPrivate *priv = setup->priv;
	GdkPixbuf *pixbuf;

	switch (prop_id) {
	case PROP_IMAGE:
		if (priv->image) {
			g_object_unref (priv->image);
		}
		priv->image = EOG_IMAGE (g_value_dup_object (value));
		if (EOG_IS_IMAGE (priv->image)) {
			pixbuf = eog_image_get_pixbuf (priv->image);
			g_object_set (priv->preview, "image",
				      pixbuf, NULL);
			g_object_unref (pixbuf);
		}
		break;
	case PROP_PAGE_SETUP:
		priv->page_setup = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eog_print_image_setup_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	EogPrintImageSetup *setup = EOG_PRINT_IMAGE_SETUP (object);
	EogPrintImageSetupPrivate *priv = setup->priv;

	switch (prop_id) {
	case PROP_IMAGE:
		g_value_set_object (value, priv->image);
		break;
	case PROP_PAGE_SETUP:
		g_value_set_object (value, priv->page_setup);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
set_initial_values (EogPrintImageSetup *setup)
{
	EogPrintImageSetupPrivate *priv;
	GtkPageSetup *page_setup;
	EogImage *image;
	gdouble page_width, page_height;
	gint pix_width, pix_height;
	gdouble factor;
	gdouble width, height;
	gdouble max_perc;

	priv = setup->priv;
	page_setup = priv->page_setup;
	image = priv->image;

	factor = get_scale_to_px_factor (setup);

	eog_image_get_size (image, &pix_width, &pix_height);
	width = (gdouble)pix_width/factor;
	height = (gdouble)pix_height/factor;

	max_perc = get_max_percentage (setup);

	width *= max_perc;
	height *= max_perc;

	gtk_range_set_range (GTK_RANGE (priv->scaling), 1, 100*max_perc);
	gtk_range_set_increments (GTK_RANGE (priv->scaling), max_perc, 10*max_perc);
	gtk_range_set_value (GTK_RANGE (priv->scaling), 100*max_perc);

	eog_print_preview_set_scale (EOG_PRINT_PREVIEW (priv->preview), max_perc);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->width), 0, width);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (priv->height), 0, height);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->width), width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->height), height);

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->center),
				  CENTER_BOTH);

	center (gtk_page_setup_get_page_width (priv->page_setup, priv->current_unit),
		gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->width)),
		GTK_SPIN_BUTTON (priv->left), GTK_SPIN_BUTTON (priv->right));
	center (gtk_page_setup_get_page_height (priv->page_setup, priv->current_unit),
		gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->height)),
		GTK_SPIN_BUTTON (priv->top), GTK_SPIN_BUTTON (priv->bottom));

	page_width = gtk_page_setup_get_page_width (page_setup, priv->current_unit);
	page_height = gtk_page_setup_get_page_height (page_setup, priv->current_unit);

	update_image_pos_ranges (setup, page_width, page_height, width, height);


}

static void
connect_signals (EogPrintImageSetup *setup)
{
	EogPrintImageSetupPrivate *priv;

	priv = setup->priv;

	g_signal_connect (G_OBJECT (priv->left), "value-changed",
			  G_CALLBACK (on_left_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->right), "value-changed",
			  G_CALLBACK (on_right_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->top), "value-changed",
			  G_CALLBACK (on_top_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->bottom), "value-changed",
			  G_CALLBACK (on_bottom_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->width), "value-changed",
			  G_CALLBACK (on_width_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->height), "value-changed",
			  G_CALLBACK (on_height_value_changed), setup);
	g_signal_connect (G_OBJECT (priv->scaling), "value-changed",
			  G_CALLBACK (on_scale_changed), setup);
	g_signal_connect (G_OBJECT (priv->scaling), "format-value",
			  G_CALLBACK (on_scale_format_value), NULL);
	g_signal_connect (G_OBJECT (priv->preview), "image-moved",
			  G_CALLBACK (on_preview_image_moved), setup);
}

static void
eog_print_image_setup_class_init (EogPrintImageSetupClass *class)
{
	GObjectClass *object_class = (GObjectClass *)class;

	object_class->set_property = eog_print_image_setup_set_property;
	object_class->get_property = eog_print_image_setup_get_property;

	g_object_class_install_property (object_class, PROP_IMAGE,
					 g_param_spec_object ("image",
							      _("Image"),
							      _("The image whose printing properties will be set up"),
							      EOG_TYPE_IMAGE,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_PAGE_SETUP,
					 g_param_spec_object ("page-setup",
							      _("Page Setup"),
							      _("The information for the page where the image will be printed"),
							      GTK_TYPE_PAGE_SETUP,
							      G_PARAM_READWRITE));

	g_type_class_add_private (class, sizeof (EogPrintImageSetupPrivate));
}

static void
eog_print_image_setup_init (EogPrintImageSetup *setup)
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *hscale;
	GtkWidget *combobox;
	EogPrintImageSetupPrivate *priv;

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
	gchar *locale_scale = NULL;
#endif

	priv = setup->priv = EOG_PRINT_IMAGE_SETUP_GET_PRIVATE (setup);

	priv->image = NULL;

	table = gtk_table_new (3, 4, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	frame = wrap_in_frame (_("Position"), table);
	gtk_table_attach (GTK_TABLE (setup), frame,
			  0, 1, 0, 1,  GTK_FILL, 0,
			  0, 0);

	priv->left = table_attach_spin_button_with_label (table, _("_Left:"), 0, 0);
	priv->right = table_attach_spin_button_with_label (table, _("_Right:"), 0, 1);
	priv->top = table_attach_spin_button_with_label (table, _("_Top:"), 2, 0);
	priv->bottom = table_attach_spin_button_with_label (table, _("_Bottom:"), 2, 1);

	label = gtk_label_new_with_mnemonic (_("C_enter:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	combobox = gtk_combo_box_new_text ();
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox),
				   CENTER_NONE, _("None"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox),
				   CENTER_HORIZONTAL, _("Horizontal"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox),
				   CENTER_VERTICAL, _("Vertical"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox),
				   CENTER_BOTH, _("Both"));
	gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), CENTER_NONE);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 2, 3, GTK_FILL, GTK_FILL,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table), combobox,
			  1, 4, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL,
			  0, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), combobox);
	priv->center = combobox;
	g_signal_connect (G_OBJECT (combobox), "changed",
			  G_CALLBACK (on_center_changed), setup);

	table = gtk_table_new (3, 4, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	frame = wrap_in_frame (_("Size"), table);
	gtk_table_attach (GTK_TABLE (setup), frame,
			  0, 1, 1, 2,  GTK_FILL, 0,
			  0, 0);

	priv->width = table_attach_spin_button_with_label (table, _("_Width:"),
							   0, 0);
	priv->height = table_attach_spin_button_with_label (table, _("_Height:"),
							    2, 0);

	label = gtk_label_new_with_mnemonic (_("_Scaling:"));
	hscale = gtk_hscale_new_with_range (1, 100, 1);
	gtk_scale_set_value_pos (GTK_SCALE (hscale), GTK_POS_RIGHT);
	gtk_range_set_value (GTK_RANGE (hscale), 100);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 1, 2, GTK_FILL, GTK_FILL,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table), hscale,
			  1, 4, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL,
			  0, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), hscale);
	priv->scaling = hscale;

	label = gtk_label_new_with_mnemonic (_("_Unit:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	combobox = gtk_combo_box_new_text ();
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox), UNIT_MM,
				   _("Millimeters"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (combobox), UNIT_INCH,
				   _("Inches"));

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
	locale_scale = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
	if (locale_scale && locale_scale[0] == 2) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), UNIT_INCH);
		set_scale_unit (setup, GTK_UNIT_INCH);
	} else
#endif
	{
		gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), UNIT_MM);
		set_scale_unit (setup, GTK_UNIT_MM);
	}

	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 2, 3, GTK_FILL, GTK_FILL,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table), combobox,
			  1, 4, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL,
			  0, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), combobox);
	priv->unit = combobox;
	g_signal_connect (G_OBJECT (combobox), "changed",
			  G_CALLBACK (on_unit_changed), setup);

	priv->preview = eog_print_preview_new ();

	/* FIXME: This shouldn't be set by hand */
	gtk_widget_set_size_request (priv->preview, 250, 250);

	frame = wrap_in_frame (_("Preview"), priv->preview);
	gtk_table_attach (GTK_TABLE (setup), frame,
			  1, 2, 0, 2, GTK_FILL, GTK_FILL,
			  0, 0);

	gtk_widget_show_all (GTK_WIDGET (setup));
}


/**
 * eog_print_image_setup_new:
 * @image: the #EogImage to print
 * @page_setup: a #GtkPageSetup specifying the page where
 * the image will be print
 *
 * Creates a new #EogPrintImageSetup widget, to be used as a custom
 * widget in a #GtkPrintUnixDialog. This widgets allows to set
 * the image position and scale in a page.
 *
 * Returns: a new #EogPrintImageSetup
 **/
GtkWidget *
eog_print_image_setup_new (EogImage *image, GtkPageSetup *page_setup)
{
	GtkWidget *setup;
	GtkWidget *preview;

	setup = g_object_new (EOG_TYPE_PRINT_IMAGE_SETUP,
			     "n-rows", 2,
			     "n-columns", 2,
			     "homogeneous", FALSE,
			     "row-spacing", 18,
			     "column-spacing", 18,
			     "border-width", 12,
			     "image", image,
			     "page-setup", page_setup,
			     NULL);

	set_initial_values (EOG_PRINT_IMAGE_SETUP (setup));

	preview = EOG_PRINT_IMAGE_SETUP (setup)->priv->preview;
	eog_print_preview_set_from_page_setup (EOG_PRINT_PREVIEW (preview),
					       page_setup);

	connect_signals (EOG_PRINT_IMAGE_SETUP (setup));

	return setup;
}

/**
 * eog_print_image_setup_get_options:
 * @setup: a #EogPrintImageSetup
 * @left: a pointer where to store the image's left position
 * @top: a pointer where to store the image's top position
 * @scale: a pointer where to store the image's scale
 * @unit: a pointer where to store the #GtkUnit used by the @left and @top values.
 *
 * Gets the options set by the #EogPrintImageSetup.
 **/
void
eog_print_image_setup_get_options (EogPrintImageSetup *setup,
				   gdouble *left,
				   gdouble *top,
				   gdouble *scale,
				   GtkUnit *unit)
{
	EogPrintImageSetupPrivate *priv;

	g_return_if_fail (EOG_IS_PRINT_IMAGE_SETUP (setup));

	priv = setup->priv;

	*left = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->left));
	*top = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->top));
	*scale = gtk_range_get_value (GTK_RANGE (priv->scaling));
	*unit = priv->current_unit;
}

void
eog_print_image_setup_update (GtkPrintOperation *operation,
			      GtkWidget         *custom_widget,
			      GtkPageSetup      *page_setup,
			      GtkPrintSettings  *print_settings,
			      gpointer           user_data)
{
	GtkWidget *preview;
	gdouble    pos_x;
	gdouble    pos_y;
	EogPrintImageSetup *setup;

	setup = EOG_PRINT_IMAGE_SETUP (custom_widget);

	setup->priv->page_setup = gtk_page_setup_copy (page_setup);

	set_initial_values (EOG_PRINT_IMAGE_SETUP (setup));

	preview = EOG_PRINT_IMAGE_SETUP (setup)->priv->preview;
	eog_print_preview_set_from_page_setup (EOG_PRINT_PREVIEW (preview),
					       setup->priv->page_setup);

	pos_x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (setup->priv->left));
	pos_y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (setup->priv->top));
	if (setup->priv->current_unit == GTK_UNIT_MM) {
		pos_x *= FACTOR_MM_TO_INCH;
		pos_y *= FACTOR_MM_TO_INCH;
	}
	eog_print_preview_set_image_position (EOG_PRINT_PREVIEW (setup->priv->preview), pos_x, pos_y);
}
