/*
 * (C) Copyright 2007 Bastien Nocera <hadess@hadess.net>
 *
 * Glow code from libwnck/libwnck/tasklist.c:
 * Copyright © 2001 Havoc Pennington
 * Copyright © 2003 Kim Woelders
 * Copyright © 2003 Red Hat, Inc.
 *
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <gtk/gtk.h>
#include "totem-glow-button.h"

#define FADE_OPACITY_DEFAULT 0.6
#define ENTER_SPEEDUP_RATIO 0.4
#define FADE_MAX_LOOPS 4

struct _TotemGlowButton {
	GtkButton parent;

	GdkPixmap *screenshot;
	GdkPixmap *screenshot_faded;

	gdouble glow_start_time;

	guint button_glow;

	guint glow : 1;
	guint anim_enabled : 1;
	guint pointer_entered : 1;
	/* Set when we don't want to play animation
	 * anymore in pointer entered mode */
	guint anim_finished :1;

	guint in_expose : 1;
};

static void	totem_glow_button_set_timeout	(TotemGlowButton *button,
						 gboolean set_timeout);

static GtkButtonClass *parent_class;

G_DEFINE_TYPE (TotemGlowButton, totem_glow_button, GTK_TYPE_BUTTON);

static void
totem_glow_button_do_expose (TotemGlowButton *button)
{
	GdkRectangle area;
	GtkWidget *buttonw;
	GtkAllocation allocation;

	buttonw = GTK_WIDGET (button);
	if (gtk_widget_get_window (buttonw) == NULL)
		return;

	gtk_widget_get_allocation (buttonw, &allocation);
	area.x = allocation.x;
	area.y = allocation.y;
	area.width = allocation.width;
	area.height = allocation.height;

	/* Send a fake expose event */
	gdk_window_invalidate_rect (gtk_widget_get_window (buttonw), &area, TRUE);
	gdk_window_process_updates (gtk_widget_get_window (buttonw), TRUE);
}

static gboolean
totem_glow_button_glow (TotemGlowButton *button)
{ 
	GtkWidget *buttonw;
	GtkAllocation allocation;
	GTimeVal tv;
	gdouble glow_factor, now;
	gfloat fade_opacity, loop_time;
	cairo_t *cr;

	buttonw = GTK_WIDGET (button);

	if (gtk_widget_get_realized (buttonw) == FALSE)
		return TRUE;

	if (button->screenshot == NULL) {
		/* Send a fake expose event to get a screenshot */
		totem_glow_button_do_expose (button);
		if (button->screenshot == NULL)
			return TRUE;
	}

	if (button->anim_enabled != FALSE) {
		g_get_current_time (&tv); 
		now = (tv.tv_sec * (1.0 * G_USEC_PER_SEC) +
		       tv.tv_usec) / G_USEC_PER_SEC;

		/* Hard-coded values */
		fade_opacity = FADE_OPACITY_DEFAULT;
		loop_time = 3.0;

		if (button->glow_start_time <= G_MINDOUBLE) {
			button->glow_start_time = now;
			/* If the pointer entered, we want to start with 'dark' */
			if (button->pointer_entered != FALSE) {
				button->glow_start_time -= loop_time / 4.0;
			}
		}

		/* Timing for mouse hover animation
		   [light]......[dark]......[light]......[dark]...
		   {mouse hover event}
		   [dark]..[light]..[dark]..[light]
		   {mouse leave event}
		   [light]......[dark]......[light]......[dark]
		*/
		if (button->pointer_entered != FALSE) {
			/* pointer entered animation should be twice as fast */
			loop_time *= ENTER_SPEEDUP_RATIO;
		}
		if ((now - button->glow_start_time) > loop_time * FADE_MAX_LOOPS) {
			button->anim_finished = TRUE;
			glow_factor = FADE_OPACITY_DEFAULT * 0.5;
		} else {
			glow_factor = fade_opacity * (0.5 - 0.5 * cos ((now - button->glow_start_time) * M_PI * 2.0 / loop_time));
		}
	} else {
		glow_factor = FADE_OPACITY_DEFAULT * 0.5;
	}

	gtk_widget_get_allocation (buttonw, &allocation);
	gdk_window_begin_paint_rect (gtk_widget_get_window (buttonw),
				     &allocation);

	cr = gdk_cairo_create (gtk_widget_get_window (buttonw));
	gdk_cairo_rectangle (cr, &allocation);
	cairo_translate (cr, allocation.x, allocation.y);
	cairo_clip (cr);

	cairo_save (cr);

	gdk_cairo_set_source_pixmap (cr, button->screenshot, 0., 0.);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);

	cairo_restore (cr);

	gdk_cairo_set_source_pixmap (cr, button->screenshot_faded, 0., 0.);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_paint_with_alpha (cr, glow_factor);

	cairo_destroy (cr);

	gdk_window_end_paint (gtk_widget_get_window (buttonw));

	if (button->anim_finished != FALSE)
		totem_glow_button_set_timeout (button, FALSE);

	return button->anim_enabled;
}

static void
totem_glow_button_clear_glow_start_timeout_id (TotemGlowButton *button)
{ 
	button->button_glow = 0;
}

static void
cleanup_screenshots (TotemGlowButton *button)
{
	if (button->screenshot != NULL) {
		g_object_unref (button->screenshot);
		button->screenshot = NULL;
	}
	if (button->screenshot_faded != NULL) {
		g_object_unref (button->screenshot_faded);
		button->screenshot_faded = NULL;
	}
}

static void
fake_expose_widget (GtkWidget *widget,
		    GdkPixmap *pixmap,
		    gint       x,
		    gint       y)
{
	GtkAllocation allocation;
	GdkWindow *tmp_window;
	GdkEventExpose event;

	event.type = GDK_EXPOSE;
	event.window = pixmap;
	event.send_event = FALSE;
	event.region = NULL;
	event.count = 0;

	tmp_window = gtk_widget_get_window (widget);
	gtk_widget_set_window (widget, pixmap);
	gtk_widget_get_allocation (widget, &allocation);
	allocation.x += x;
	allocation.y += y;
	gtk_widget_set_allocation (widget, &allocation);

	event.area = allocation;

	gtk_widget_send_expose (widget, (GdkEvent *) &event);

	gtk_widget_set_window (widget, tmp_window);
	gtk_widget_get_allocation (widget, &allocation);
	allocation.x -= x;
	allocation.y -= y;
	gtk_widget_set_allocation (widget, &allocation);
}

static GdkPixmap *
take_screenshot (TotemGlowButton *button)
{
	GtkWidget *buttonw;
	GtkAllocation allocation;
	GtkStyle *style;
	GdkPixmap *pixmap;
	gint width, height;
	cairo_t *cr;

	buttonw = GTK_WIDGET (button);
	gtk_widget_get_allocation (buttonw, &allocation);

	width = allocation.width;
	height = allocation.height;

	pixmap = gdk_pixmap_new (gtk_widget_get_window (buttonw),
				 width, height, -1);
	cr = gdk_cairo_create (pixmap);

	/* Draw a rectangle with bg[SELECTED] */
	style = gtk_widget_get_style (buttonw);
	gdk_cairo_set_source_color (cr, &(style->bg[GTK_STATE_SELECTED]));
	cairo_rectangle (cr, 0.0, 0.0, width + 1.0, height + 1.0);
	cairo_fill (cr);

	/* then the image */
	fake_expose_widget (gtk_button_get_image (GTK_BUTTON (button)), pixmap,
			    -allocation.x, -allocation.y);

	cairo_destroy (cr);

	return pixmap;
}

static gboolean
totem_glow_button_expose (GtkWidget        *buttonw,
			  GdkEventExpose   *event)
{
	TotemGlowButton *button;

	button = TOTEM_GLOW_BUTTON (buttonw);

	(* GTK_WIDGET_CLASS (parent_class)->expose_event) (buttonw, event);

	if (button->glow != FALSE && button->screenshot == NULL && button->in_expose == FALSE &&
	    /* Don't take screenshots if we finished playing animation after
	       pointer entered, or if we're already in an expose event. */
	    (button->pointer_entered != FALSE && 
	     button->anim_finished != FALSE) == FALSE) {
		button->in_expose = TRUE;

		button->screenshot = gtk_widget_get_snapshot (buttonw, NULL);;
		button->screenshot_faded = take_screenshot (button);

		button->in_expose = FALSE;
	}

	return FALSE;
}

static void
totem_glow_button_map (GtkWidget *buttonw)
{
	TotemGlowButton *button;

	(* GTK_WIDGET_CLASS (parent_class)->map) (buttonw);

	button = TOTEM_GLOW_BUTTON (buttonw);

	if (button->glow != FALSE && button->button_glow == 0) {
		totem_glow_button_set_glow (button, TRUE);
	}
}

static void
totem_glow_button_unmap (GtkWidget *buttonw)
{
	TotemGlowButton *button;

	button = TOTEM_GLOW_BUTTON (buttonw);

	if (button->button_glow > 0) {
		g_source_remove (button->button_glow);
		button->button_glow = 0;
	}

	cleanup_screenshots (button);

	(* GTK_WIDGET_CLASS (parent_class)->unmap) (buttonw);
}

static void
totem_glow_button_enter (GtkButton *buttonw)
{
	TotemGlowButton *button;

	button = TOTEM_GLOW_BUTTON (buttonw);

	(* GTK_BUTTON_CLASS (parent_class)->enter) (buttonw);
	
	button->pointer_entered = TRUE;
	button->anim_finished = FALSE;
	button->glow_start_time = G_MINDOUBLE;
}

static void
totem_glow_button_leave (GtkButton *buttonw)
{
	TotemGlowButton *button;

	button = TOTEM_GLOW_BUTTON (buttonw);

	(* GTK_BUTTON_CLASS (parent_class)->leave) (buttonw);

	button->pointer_entered = FALSE;
	button->glow_start_time = G_MINDOUBLE;
	button->anim_finished = FALSE;
	if (button->glow != FALSE)
		totem_glow_button_set_timeout (button, TRUE);
}

static void
totem_glow_button_finalize (GObject *object)
{
	TotemGlowButton *button = TOTEM_GLOW_BUTTON (object);

	totem_glow_button_set_glow (button, FALSE);
	cleanup_screenshots (button);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
totem_glow_button_class_init (TotemGlowButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = totem_glow_button_finalize;
	widget_class->expose_event = totem_glow_button_expose;
	widget_class->map = totem_glow_button_map;
	widget_class->unmap = totem_glow_button_unmap;
	button_class->enter = totem_glow_button_enter;
	button_class->leave = totem_glow_button_leave;
}

static void
totem_glow_button_init (TotemGlowButton *button)
{
	button->glow_start_time = 0.0;
	button->button_glow = 0;
	button->screenshot = NULL;
	button->screenshot_faded = NULL;
}

GtkWidget *
totem_glow_button_new (void)
{
	return g_object_new (TOTEM_TYPE_GLOW_BUTTON, NULL);
}

static void
totem_glow_button_set_timeout (TotemGlowButton *button, gboolean set_timeout)
{
	if (set_timeout != FALSE) {
		button->glow_start_time = 0.0;

		/* The animation doesn't speed up or slow down based on the
		 * timeout value, but instead will just appear smoother or 
		 * choppier.
		 */
		button->button_glow =
			g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
					    100,
					    (GSourceFunc) totem_glow_button_glow, button,
					    (GDestroyNotify) totem_glow_button_clear_glow_start_timeout_id);
	} else {
		if (button->button_glow > 0) {
			g_source_remove (button->button_glow);
			button->button_glow = 0;
		}
		cleanup_screenshots (button);
		totem_glow_button_do_expose (button);
	}
}

void
totem_glow_button_set_glow (TotemGlowButton *button, gboolean glow)
{
	GtkSettings *settings;
	gboolean anim_enabled;

	g_return_if_fail (TOTEM_IS_GLOW_BUTTON (button));

	if (gtk_widget_get_mapped (GTK_WIDGET (button)) == FALSE
	    && glow != FALSE) {
		button->glow = glow;
		return;
	}

	settings = gtk_settings_get_for_screen
		(gtk_widget_get_screen (GTK_WIDGET (button)));
	g_object_get (G_OBJECT (settings),
		      "gtk-enable-animations", &anim_enabled,
		      NULL);
	button->anim_enabled = anim_enabled;

	if (glow == FALSE && button->button_glow == 0 && button->anim_enabled != FALSE)
		return;
	if (glow != FALSE && button->button_glow != 0)
		return;

	button->glow = glow;

	totem_glow_button_set_timeout (button, glow);
}

gboolean
totem_glow_button_get_glow (TotemGlowButton *button)
{
	return button->glow != FALSE;
}

