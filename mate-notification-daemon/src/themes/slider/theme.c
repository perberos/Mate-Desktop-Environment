/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2006-2007 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2009 Red Hat, Inc.
 * Copyright (C) 2011 Perberos <perberos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

typedef void (*ActionInvokedCb) (GtkWindow* nw, const char* key);
typedef void (*UrlClickedCb) (GtkWindow* nw, const char* url);

typedef struct {
	GtkWidget* win;
	GtkWidget* main_hbox;
	GtkWidget* iconbox;
	GtkWidget* icon;
	GtkWidget* content_hbox;
	GtkWidget* summary_label;
	GtkWidget* close_button;
	GtkWidget* body_label;
	GtkWidget* actions_box;
	GtkWidget* last_sep;
	GtkWidget* pie_countdown;

	gboolean has_arrow;
	gboolean composited;

	int width;
	int height;
	int last_width;
	int last_height;

	guchar urgency;
	glong timeout;
	glong remaining;

	UrlClickedCb url_clicked;
} WindowData;

enum {
	URGENCY_LOW,
	URGENCY_NORMAL,
	URGENCY_CRITICAL
};

#define WIDTH          400
#define DEFAULT_X0     0
#define DEFAULT_Y0     0
#define DEFAULT_RADIUS 8
#define IMAGE_SIZE     48
#define PIE_RADIUS     8
#define PIE_WIDTH      (2 * PIE_RADIUS)
#define PIE_HEIGHT     (2 * PIE_RADIUS)
#define BODY_X_OFFSET  (IMAGE_SIZE + 4)
#define BACKGROUND_ALPHA    0.90

#define MAX_ICON_SIZE IMAGE_SIZE

#define USE_COMPOSITE

static void draw_round_rect(cairo_t* cr, gdouble aspect, gdouble x, gdouble y, gdouble corner_radius, gdouble width, gdouble height)
{
	gdouble radius = corner_radius / aspect;

	cairo_move_to(cr, x + radius, y);

	// top-right, left of the corner
	cairo_line_to(cr, x + width - radius, y);

	// top-right, below the corner
	cairo_arc(cr, x + width - radius, y + radius, radius, -90.0f * G_PI / 180.0f, 0.0f * G_PI / 180.0f);

	// bottom-right, above the corner
	cairo_line_to(cr, x + width, y + height - radius);

	// bottom-right, left of the corner
	cairo_arc(cr, x + width - radius, y + height - radius, radius, 0.0f * G_PI / 180.0f, 90.0f * G_PI / 180.0f);

	// bottom-left, right of the corner
	cairo_line_to(cr, x + radius, y + height);

	// bottom-left, above the corner
	cairo_arc(cr, x + radius, y + height - radius, radius, 90.0f * G_PI / 180.0f, 180.0f * G_PI / 180.0f);

	// top-left, below the corner
	cairo_line_to(cr, x, y + radius);

	// top-left, right of the corner
	cairo_arc(cr, x + radius, y + radius, radius, 180.0f * G_PI / 180.0f, 270.0f * G_PI / 180.0f);
}

static void fill_background(GtkWidget* widget, WindowData* windata, cairo_t* cr)
{
	GdkColor color;
	double r, g, b;

	#if GTK_CHECK_VERSION(3, 0, 0)

		GtkAllocation allocation;

		gtk_widget_get_allocation(widget, &allocation);

		draw_round_rect(cr, 1.0f, DEFAULT_X0 + 1, DEFAULT_Y0 + 1, DEFAULT_RADIUS, allocation.width - 2, allocation.height - 2);

		//GdkRGBA color;

		//gtk_style_context_get_background_color();
		color = widget->style->bg[GTK_STATE_NORMAL];
		r = (float) color.red / 65535.0;
		g = (float) color.green / 65535.0;
		b = (float) color.blue / 65535.0;
		cairo_set_source_rgba(cr, r, g, b, BACKGROUND_ALPHA);
		cairo_fill_preserve(cr);

		/* Should we show urgency somehow?  Probably doesn't
		 * have any meaningful value to the user... */

		color = widget->style->text_aa[GTK_STATE_NORMAL];
		r = (float) color.red / 65535.0;
		g = (float) color.green / 65535.0;
		b = (float) color.blue / 65535.0;
		cairo_set_source_rgba(cr, r, g, b, BACKGROUND_ALPHA / 2);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);

	#else

		draw_round_rect(cr, 1.0f, DEFAULT_X0 + 1, DEFAULT_Y0 + 1, DEFAULT_RADIUS, widget->allocation.width - 2, widget->allocation.height - 2);

		color = widget->style->bg[GTK_STATE_NORMAL];
		r = (float) color.red / 65535.0;
		g = (float) color.green / 65535.0;
		b = (float) color.blue / 65535.0;
		cairo_set_source_rgba(cr, r, g, b, BACKGROUND_ALPHA);
		cairo_fill_preserve(cr);

		/* Should we show urgency somehow?  Probably doesn't
		 * have any meaningful value to the user... */

		color = widget->style->text_aa[GTK_STATE_NORMAL];
		r = (float) color.red / 65535.0;
		g = (float) color.green / 65535.0;
		b = (float) color.blue / 65535.0;
		cairo_set_source_rgba(cr, r, g, b, BACKGROUND_ALPHA / 2);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	#endif
}

static void update_shape(WindowData* windata)
{
	GdkBitmap* mask;
	cairo_t* cr;

	if (windata->width == windata->last_width && windata->height == windata->last_height)
	{
		return;
	}

	if (windata->width == 0 || windata->height == 0)
	{
		#if GTK_CHECK_VERSION(3, 0, 0)

			GtkAllocation allocation;

			gtk_widget_get_allocation(windata->win, &allocation);

			windata->width = MAX(allocation.width, 1);
			windata->height = MAX(allocation.height, 1);
		#else
			windata->width = MAX(windata->win->allocation.width, 1);
			windata->height = MAX(windata->win->allocation.height, 1);
		#endif
	}

	if (windata->composited)
	{
		gtk_widget_shape_combine_mask(windata->win, NULL, 0, 0);
		return;
	}

	windata->last_width = windata->width;
	windata->last_height = windata->height;
	mask = (GdkBitmap*) gdk_pixmap_new(NULL, windata->width, windata->height, 1);

	if (mask == NULL)
	{
		return;
	}

	cr = gdk_cairo_create(mask);

	if (cairo_status(cr) == CAIRO_STATUS_SUCCESS)
	{
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgb(cr, 1.0f, 1.0f, 1.0f);
		draw_round_rect(cr, 1.0f, DEFAULT_X0, DEFAULT_Y0, DEFAULT_RADIUS, windata->width, windata->height);
		cairo_fill(cr);

		gtk_widget_shape_combine_mask(windata->win, mask, 0, 0);
	}
	cairo_destroy(cr);

	g_object_unref(mask);
}

static void paint_window(GtkWidget* widget, WindowData* windata)
{
	cairo_t* context;
	cairo_surface_t* surface;
	cairo_t* cr;

	if (windata->width == 0 || windata->height == 0)
	{
		#if GTK_CHECK_VERSION(3, 0, 0)

			GtkAllocation allocation;

			gtk_widget_get_allocation(windata->win, &allocation);

			windata->width = MAX(allocation.width, 1);
			windata->height = MAX(allocation.height, 1);
		#else
			windata->width = MAX(windata->win->allocation.width, 1);
			windata->height = MAX(windata->win->allocation.height, 1);
		#endif
	}

	context = gdk_cairo_create(widget->window);

	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);


	#if GTK_CHECK_VERSION(3, 0, 0)

		GtkAllocation allocation;

		gtk_widget_get_allocation(widget, &allocation);

		surface = cairo_surface_create_similar(cairo_get_target(context), CAIRO_CONTENT_COLOR_ALPHA, allocation.width, allocation.height);
	#else

		surface = cairo_surface_create_similar(cairo_get_target(context), CAIRO_CONTENT_COLOR_ALPHA, widget->allocation.width, widget->allocation.height);
	#endif

    cr = cairo_create(surface);

	fill_background(widget, windata, cr);

	cairo_destroy(cr);
	cairo_set_source_surface(context, surface, 0, 0);
	cairo_paint(context);
	cairo_surface_destroy(surface);
	cairo_destroy(context);

	update_shape(windata);
}

static gboolean on_window_map(GtkWidget* widget, GdkEvent* event, WindowData* windata)
{
	return FALSE;
}

static gboolean on_window_expose(GtkWidget* widget, GdkEventExpose* event, WindowData* windata)
{
	paint_window(widget, windata);

	return FALSE;
}

static void destroy_windata(WindowData* windata)
{
	g_free(windata);
}

static void update_content_hbox_visibility(WindowData* windata)
{
	if (GTK_WIDGET_VISIBLE(windata->icon) || GTK_WIDGET_VISIBLE(windata->body_label) || GTK_WIDGET_VISIBLE(windata->actions_box))
	{
		gtk_widget_show(windata->content_hbox);
	}
	else
	{
		gtk_widget_hide(windata->content_hbox);
	}
}

static gboolean on_configure_event(GtkWidget* widget, GdkEventConfigure* event, WindowData* windata)
{
	windata->width = event->width;
	windata->height = event->height;

	gtk_widget_queue_draw(widget);

	return FALSE;
}

static void on_window_realize(GtkWidget* widget, WindowData* windata)
{
	/* Nothing */
}

static void color_reverse(const GdkColor* a, GdkColor* b)
{
	gdouble red;
	gdouble green;
	gdouble blue;
	gdouble h;
	gdouble s;
	gdouble v;

	red = (gdouble) a->red / 65535.0;
	green = (gdouble) a->green / 65535.0;
	blue = (gdouble) a->blue / 65535.0;

	gtk_rgb_to_hsv(red, green, blue, &h, &s, &v);

	/* pivot brightness around the center */
	v = 0.5 + (0.5 - v);

	if (v > 1.0)
	{
		v = 1.0;
	}
	else if (v < 0.0)
	{
		v = 0.0;
	}

	/* reduce saturation by 50% */
	s *= 0.5;

	gtk_hsv_to_rgb(h, s, v, &red, &green, &blue);

	b->red = red * 65535.0;
	b->green = green * 65535.0;
	b->blue = blue * 65535.0;
}

static void override_style(GtkWidget* widget, GtkStyle* previous_style)
{
	GtkStateType state;
	GtkStyle* style;
	GdkColor fg;
	GdkColor bg;

	style = gtk_style_copy(widget->style);

	if (previous_style == NULL || (previous_style != NULL && (previous_style->bg[GTK_STATE_NORMAL].red != style->bg[GTK_STATE_NORMAL].red || previous_style->bg[GTK_STATE_NORMAL].green != style->bg[GTK_STATE_NORMAL].green || previous_style->bg[GTK_STATE_NORMAL].blue != style->bg[GTK_STATE_NORMAL].blue)))
	{
		state = (GtkStateType) 0;

		while (state < (GtkStateType) G_N_ELEMENTS(widget->style->bg))
		{
			color_reverse(&style->bg[state], &bg);
			gtk_widget_modify_bg(widget, state, &bg);
			state++;
		}
	}

	if (previous_style == NULL || (previous_style != NULL && (previous_style->fg[GTK_STATE_NORMAL].red != style->fg[GTK_STATE_NORMAL].red || previous_style->fg[GTK_STATE_NORMAL].green != style->fg[GTK_STATE_NORMAL].green || previous_style->fg[GTK_STATE_NORMAL].blue != style->fg[GTK_STATE_NORMAL].blue)))
	{
		state = (GtkStateType) 0;

		while (state < (GtkStateType) G_N_ELEMENTS(widget->style->fg))
		{
			color_reverse(&style->fg[state], &fg);
			gtk_widget_modify_fg(widget, state, &fg);
			state++;
		}
	}

	g_object_unref(style);
}

static void on_style_set(GtkWidget* widget, GtkStyle* previous_style, WindowData* windata)
{
	g_signal_handlers_block_by_func(G_OBJECT(widget), on_style_set, windata);
	override_style(widget, previous_style);

	gtk_widget_queue_draw(widget);

	g_signal_handlers_unblock_by_func(G_OBJECT(widget), on_style_set, windata);
}

static void on_composited_changed(GtkWidget* window, WindowData* windata)
{
	windata->composited = gdk_screen_is_composited(gtk_widget_get_screen(window));
	update_shape(windata);
}

GtkWindow* create_notification(UrlClickedCb url_clicked)
{
	GtkWidget* win;
	GtkWidget* main_vbox;
	GtkWidget* vbox;
	GtkWidget* close_button;
	GtkWidget* image;
	GtkWidget* alignment;
	AtkObject* atkobj;
	GtkRcStyle* rcstyle;
	WindowData* windata;
	#ifdef USE_COMPOSITE
		GdkColormap* colormap;
		GdkScreen* screen;
	#endif

	windata = g_new0(WindowData, 1);
	windata->urgency = URGENCY_NORMAL;
	windata->url_clicked = url_clicked;

	win = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
	gtk_widget_set_app_paintable(win, TRUE);
	g_signal_connect(G_OBJECT(win), "style-set", G_CALLBACK(on_style_set), windata);
	g_signal_connect(G_OBJECT(win), "map-event", G_CALLBACK(on_window_map), windata);
	g_signal_connect(G_OBJECT(win), "expose-event", G_CALLBACK(on_window_expose), windata);
	g_signal_connect(G_OBJECT(win), "realize", G_CALLBACK(on_window_realize), windata);

	windata->win = win;

	windata->composited = FALSE;

	#ifdef USE_COMPOSITE
		screen = gtk_window_get_screen(GTK_WINDOW(win));
		colormap = gdk_screen_get_rgba_colormap(screen);

		if (colormap != NULL)
		{
			gtk_widget_set_colormap(win, colormap);

			if (gdk_screen_is_composited(screen))
			{
				windata->composited = TRUE;
			}
		}

		g_signal_connect(win, "composited-changed", G_CALLBACK(on_composited_changed), windata);
	#endif

	gtk_window_set_title(GTK_WINDOW(win), "Notification");
	gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_NOTIFICATION);
	gtk_widget_add_events(win, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

	g_object_set_data_full(G_OBJECT(win), "windata", windata, (GDestroyNotify) destroy_windata);
	atk_object_set_role(gtk_widget_get_accessible(win), ATK_ROLE_ALERT);

	g_signal_connect(G_OBJECT(win), "configure-event", G_CALLBACK(on_configure_event), windata);

	main_vbox = gtk_vbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(main_vbox), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(win), main_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);

	windata->main_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(windata->main_hbox);
	gtk_box_pack_start(GTK_BOX(main_vbox), windata->main_hbox, FALSE, FALSE, 0);

	/* First row (icon, vbox, close) */
	windata->iconbox = gtk_alignment_new(0.5, 0, 0, 0);
	gtk_widget_show(windata->iconbox);
	gtk_alignment_set_padding(GTK_ALIGNMENT(windata->iconbox), 5, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(windata->main_hbox), windata->iconbox, FALSE, FALSE, 0);
	gtk_widget_set_size_request(windata->iconbox, BODY_X_OFFSET, -1);

	windata->icon = gtk_image_new();
	gtk_widget_show(windata->icon);
	gtk_container_add(GTK_CONTAINER(windata->iconbox), windata->icon);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(windata->main_hbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

	/* Add the close button */
	alignment = gtk_alignment_new(0.5, 0, 0, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(windata->main_hbox), alignment, FALSE, FALSE, 0);

	close_button = gtk_button_new();
	g_signal_connect(G_OBJECT(close_button), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(close_button);
	windata->close_button = close_button;
	gtk_container_add(GTK_CONTAINER(alignment), close_button);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(close_button), 0);
	g_signal_connect_swapped(G_OBJECT(close_button), "clicked", G_CALLBACK(gtk_widget_destroy), win);

	rcstyle = gtk_rc_style_new();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style(close_button, rcstyle);
	g_object_unref(rcstyle);

	atkobj = gtk_widget_get_accessible(close_button);
	atk_action_set_description(ATK_ACTION(atkobj), 0, "Closes the notification.");
	atk_object_set_name(atkobj, "");
	atk_object_set_description(atkobj, "Closes the notification.");

	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(close_button), image);

	/* center vbox */
	windata->summary_label = gtk_label_new(NULL);
	g_signal_connect(G_OBJECT(windata->summary_label), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(windata->summary_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->summary_label, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->summary_label), 0, 0);
	gtk_label_set_line_wrap(GTK_LABEL(windata->summary_label), TRUE);

	atkobj = gtk_widget_get_accessible(windata->summary_label);
	atk_object_set_description(atkobj, "Notification summary text.");

	windata->content_hbox = gtk_hbox_new(FALSE, 6);
	gtk_widget_show(windata->content_hbox);
	gtk_box_pack_start(GTK_BOX(vbox), windata->content_hbox, FALSE, FALSE, 0);


	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(windata->content_hbox), vbox, TRUE, TRUE, 0);

	windata->body_label = gtk_label_new(NULL);
	g_signal_connect(G_OBJECT(windata->body_label), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(windata->body_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->body_label, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->body_label), 0, 0);
	gtk_label_set_line_wrap(GTK_LABEL(windata->body_label), TRUE);
	g_signal_connect_swapped(G_OBJECT(windata->body_label), "activate-link", G_CALLBACK(windata->url_clicked), win);

	atkobj = gtk_widget_get_accessible(windata->body_label);
	atk_object_set_description(atkobj, "Notification body text.");

	alignment = gtk_alignment_new(1, 0.5, 0, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);

	windata->actions_box = gtk_hbox_new(FALSE, 6);
	gtk_widget_show(windata->actions_box);
	gtk_container_add(GTK_CONTAINER(alignment), windata->actions_box);

	return GTK_WINDOW(win);
}

void set_notification_hints(GtkWindow* nw, GHashTable* hints)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	g_assert(windata != NULL);

	GValue* value = (GValue*) g_hash_table_lookup(hints, "urgency");

	if (value != NULL && G_VALUE_HOLDS_UCHAR(value))
	{
		windata->urgency = g_value_get_uchar(value);

		if (windata->urgency == URGENCY_CRITICAL)
		{
			gtk_window_set_title(GTK_WINDOW(nw), "Critical Notification");
		}
		else
		{
			gtk_window_set_title(GTK_WINDOW(nw), "Notification");
		}
	}
}

void set_notification_timeout(GtkWindow *nw, glong timeout)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	g_assert(windata != NULL);

	windata->timeout = timeout;
}

void notification_tick(GtkWindow* nw, glong remaining)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	windata->remaining = remaining;

	if (windata->pie_countdown != NULL)
	{
		gtk_widget_queue_draw_area(windata->pie_countdown, 0, 0, PIE_WIDTH, PIE_HEIGHT);
	}
}

void set_notification_text(GtkWindow* nw, const char* summary, const char* body)
{
	char* str;
	char* quoted;
	GtkRequisition req;
	WindowData* windata;
	int summary_width;

	windata = g_object_get_data(G_OBJECT(nw), "windata");

	g_assert(windata != NULL);

	quoted = g_markup_escape_text(summary, -1);
	str = g_strdup_printf("<b><big>%s</big></b>", quoted);
	g_free(quoted);

	gtk_label_set_markup(GTK_LABEL(windata->summary_label), str);
	g_free(str);

	gtk_label_set_markup(GTK_LABEL(windata->body_label), body);

	if (body == NULL || *body == '\0')
	{
		gtk_widget_hide(windata->body_label);
	}
	else
	{
		gtk_widget_show(windata->body_label);
	}

	update_content_hbox_visibility(windata);

	gtk_widget_size_request(windata->close_button, &req);
	/* -1: main_vbox border width
	   -10: vbox border width
	   -6: spacing for hbox */
	summary_width = WIDTH - (1 * 2) - (10 * 2) - BODY_X_OFFSET - req.width - (6 * 2);

	if (body != NULL && *body != '\0')
	{
		gtk_widget_set_size_request(windata->body_label, summary_width, -1);
	}

	gtk_widget_set_size_request(windata->summary_label, summary_width, -1);
}

static GdkPixbuf* scale_pixbuf(GdkPixbuf* pixbuf, int max_width, int max_height, gboolean no_stretch_hint)
{
	float scale_factor_x = 1.0;
	float scale_factor_y = 1.0;
	float scale_factor = 1.0;

	int pw = gdk_pixbuf_get_width(pixbuf);
	int ph = gdk_pixbuf_get_height(pixbuf);

	/* Determine which dimension requires the smallest scale. */
	scale_factor_x = (float) max_width / (float) pw;
	scale_factor_y = (float) max_height / (float) ph;

	if (scale_factor_x > scale_factor_y)
	{
		scale_factor = scale_factor_y;
	}
	else
	{
		scale_factor = scale_factor_x;
	}

	/* always scale down, allow to disable scaling up */
	if (scale_factor < 1.0 || !no_stretch_hint)
	{
		int scale_x;
		int scale_y;

		scale_x = (int) (pw * scale_factor);
		scale_y = (int) (ph * scale_factor);

		return gdk_pixbuf_scale_simple(pixbuf, scale_x, scale_y, GDK_INTERP_BILINEAR);
	}
	else
	{
		return g_object_ref(pixbuf);
	}
}

void set_notification_icon(GtkWindow* nw, GdkPixbuf* pixbuf)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	g_assert(windata != NULL);

	GdkPixbuf* scaled = NULL;

	if (pixbuf != NULL)
	{
		scaled = scale_pixbuf(pixbuf, MAX_ICON_SIZE, MAX_ICON_SIZE, TRUE);
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(windata->icon), scaled);

	if (scaled != NULL)
	{
		int pixbuf_width = gdk_pixbuf_get_width(scaled);

		gtk_widget_show(windata->icon);
		gtk_widget_set_size_request(windata->iconbox, MAX(BODY_X_OFFSET, pixbuf_width), -1);
		g_object_unref(scaled);
	}
	else
	{
		gtk_widget_hide(windata->icon);

		gtk_widget_set_size_request(windata->iconbox, BODY_X_OFFSET, -1);
	}

	update_content_hbox_visibility(windata);
}

void set_notification_arrow(GtkWidget* nw, gboolean visible, int x, int y)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	g_assert(windata != NULL);
}

static gboolean on_countdown_expose(GtkWidget* pie, GdkEventExpose* event, WindowData* windata)
{
	GtkStyle* style;
	cairo_t* context;
	cairo_surface_t* surface;
	cairo_t* cr;
	GdkColor color;
	double r, g, b;

	style = gtk_widget_get_style(windata->win);

	#if GTK_CHECK_VERSION(3, 0, 0)
		context = gdk_cairo_create(GDK_WINDOW(windata->pie_countdown->window));
	#else
		context = gdk_cairo_create(GDK_DRAWABLE(windata->pie_countdown->window));
	#endif

	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);


	#if GTK_CHECK_VERSION(3, 0, 0)

		GtkAllocation allocation;

		gtk_widget_get_allocation(pie, &allocation);

		surface = cairo_surface_create_similar(cairo_get_target(context), CAIRO_CONTENT_COLOR_ALPHA, allocation.width, allocation.height);

	#else

		surface = cairo_surface_create_similar(cairo_get_target(context), CAIRO_CONTENT_COLOR_ALPHA, pie->allocation.width, pie->allocation.height);

	#endif

	cr = cairo_create(surface);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	color = windata->win->style->bg[GTK_STATE_NORMAL];
	r = (float) color.red / 65535.0;
	g = (float) color.green / 65535.0;
	b = (float) color.blue / 65535.0;
	cairo_set_source_rgba(cr, r, g, b, BACKGROUND_ALPHA);
	cairo_paint(cr);

	if (windata->timeout > 0)
	{
		gdouble pct = (gdouble) windata->remaining / (gdouble) windata->timeout;

		gdk_cairo_set_source_color(cr, &style->fg[GTK_STATE_NORMAL]);

		cairo_move_to(cr, PIE_RADIUS, PIE_RADIUS);
		cairo_arc_negative(cr, PIE_RADIUS, PIE_RADIUS, PIE_RADIUS, -G_PI_2, -(pct * G_PI * 2) - G_PI_2);
		cairo_line_to(cr, PIE_RADIUS, PIE_RADIUS);
		cairo_fill(cr);
	}

	cairo_destroy(cr);
	cairo_set_source_surface(context, surface, 0, 0);
	cairo_paint(context);
	cairo_surface_destroy(surface);
	cairo_destroy(context);

	return TRUE;
}

static void on_action_clicked(GtkWidget* w, GdkEventButton *event, ActionInvokedCb action_cb)
{
	GtkWindow* nw = g_object_get_data(G_OBJECT(w), "_nw");
	const char* key = g_object_get_data(G_OBJECT(w), "_action_key");

	action_cb(nw, key);
}

void add_notification_action(GtkWindow* nw, const char* text, const char* key, ActionInvokedCb cb)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");
	GtkWidget* label;
	GtkWidget* button;
	GtkWidget* hbox;
	GdkPixbuf* pixbuf;
	char* buf;

	g_assert(windata != NULL);

	if (!GTK_WIDGET_VISIBLE(windata->actions_box))
	{
		GtkWidget* alignment;

		gtk_widget_show(windata->actions_box);
		update_content_hbox_visibility(windata);

		alignment = gtk_alignment_new(1, 0.5, 0, 0);
		gtk_widget_show(alignment);
		gtk_box_pack_end(GTK_BOX(windata->actions_box), alignment, FALSE, TRUE, 0);

		windata->pie_countdown = gtk_drawing_area_new();
		g_signal_connect(G_OBJECT(windata->pie_countdown), "style-set", G_CALLBACK(on_style_set), windata);

		gtk_widget_show(windata->pie_countdown);
		gtk_container_add(GTK_CONTAINER(alignment), windata->pie_countdown);
		gtk_widget_set_size_request(windata->pie_countdown, PIE_WIDTH, PIE_HEIGHT);
		g_signal_connect(G_OBJECT(windata->pie_countdown), "expose_event", G_CALLBACK(on_countdown_expose), windata);
	}

	button = gtk_button_new();
	g_signal_connect(G_OBJECT(button), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(windata->actions_box), button, FALSE, FALSE, 0);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(button), 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(button), hbox);

	/* Try to be smart and find a suitable icon. */
	buf = g_strdup_printf("stock_%s", key);
	pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_for_screen(gdk_drawable_get_screen(GTK_WIDGET(nw)->window)), buf, 16, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
	g_free(buf);

	if (pixbuf != NULL)
	{
		GtkWidget* image = gtk_image_new_from_pixbuf(pixbuf);
		g_signal_connect(G_OBJECT(image), "style-set", G_CALLBACK(on_style_set), windata);
		gtk_widget_show(image);
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
		gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.5);
	}

	label = gtk_label_new(NULL);
	g_signal_connect(G_OBJECT(label), "style-set", G_CALLBACK(on_style_set), windata);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	buf = g_strdup_printf("<small>%s</small>", text);
	gtk_label_set_markup(GTK_LABEL(label), buf);
	g_free(buf);

	g_object_set_data(G_OBJECT(button), "_nw", nw);
	g_object_set_data_full(G_OBJECT(button), "_action_key", g_strdup(key), g_free);
	g_signal_connect(G_OBJECT(button), "button-release-event", G_CALLBACK(on_action_clicked), cb);
}

void clear_notification_actions(GtkWindow* nw)
{
	WindowData* windata = g_object_get_data(G_OBJECT(nw), "windata");

	windata->pie_countdown = NULL;

	gtk_widget_hide(windata->actions_box);
	gtk_container_foreach(GTK_CONTAINER(windata->actions_box), (GtkCallback) gtk_object_destroy, NULL);
}

void move_notification(GtkWidget* widget, int x, int y)
{
	WindowData* windata = g_object_get_data(G_OBJECT(widget), "windata");

	g_assert(windata != NULL);

	gtk_window_move(GTK_WINDOW(windata->win), x, y);
}

void get_theme_info(char** theme_name, char** theme_ver, char** author, char** homepage)
{
	*theme_name = g_strdup("Slider");
	*theme_ver  = g_strdup_printf("%d.%d.%d", NOTIFICATION_DAEMON_MAJOR_VERSION, NOTIFICATION_DAEMON_MINOR_VERSION, NOTIFICATION_DAEMON_MICRO_VERSION);
	*author = g_strdup("William Jon McCann");
	*homepage = g_strdup("http://www.gnome.org/");
}

gboolean get_always_stack(GtkWidget* nw)
{
	return TRUE;
}

gboolean theme_check_init(unsigned int major_ver, unsigned int minor_ver, unsigned int micro_ver)
{
	return major_ver == 0 && (minor_ver > 3 || (minor_ver == 3 && micro_ver >= 2));
}
