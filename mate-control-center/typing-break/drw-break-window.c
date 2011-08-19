/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002      CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>

 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <mateconf/mateconf-client.h>

#ifdef HAVE_CANBERRA_GTK
#include <canberra-gtk.h>
#endif

#include "drwright.h"
#include "drw-utils.h"
#include "drw-break-window.h"
#include "drw-timer.h"

struct _DrwBreakWindowPrivate {
	GtkWidget *clock_label;
	GtkWidget *break_label;
	GtkWidget *image;

	GtkWidget *postpone_entry;
	GtkWidget *postpone_button;

	DrwTimer  *timer;

	gint       break_time;
	gchar     *break_text;
	guint      clock_timeout_id;
	guint      postpone_timeout_id;
	guint      postpone_sensitize_id;
};

#define DRW_BREAK_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DRW_TYPE_BREAK_WINDOW, DrwBreakWindowPrivate))

#define POSTPONE_CANCEL 30

/* Signals */
enum {
	DONE,
	POSTPONE,
	LAST_SIGNAL
};

static void         drw_break_window_class_init    (DrwBreakWindowClass *klass);
static void         drw_break_window_init          (DrwBreakWindow      *window);
static void         drw_break_window_finalize      (GObject             *object);
static void         drw_break_window_dispose       (GObject             *object);
static gboolean     postpone_sensitize_cb          (DrwBreakWindow      *window);
static gboolean     clock_timeout_cb               (DrwBreakWindow      *window);
static void         postpone_clicked_cb            (GtkWidget           *button,
						    GtkWidget           *window);
static gboolean     label_expose_event_cb          (GtkLabel            *label,
						    GdkEventExpose      *event,
						    gpointer             user_data);
static void         label_size_request_cb          (GtkLabel            *label,
						    GtkRequisition      *requisition,
						    gpointer             user_data);

G_DEFINE_TYPE (DrwBreakWindow, drw_break_window, GTK_TYPE_WINDOW)

static guint signals[LAST_SIGNAL];

static void
drw_break_window_class_init (DrwBreakWindowClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = drw_break_window_finalize;
        object_class->dispose = drw_break_window_dispose;

	signals[POSTPONE] =
		g_signal_new ("postpone",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (klass, sizeof (DrwBreakWindowPrivate));
}

static void
drw_break_window_init (DrwBreakWindow *window)
{
	DrwBreakWindowPrivate *priv;
	GtkWidget             *vbox;
	GtkWidget             *hbox;
	GtkWidget             *align;
	GtkWidget             *monitor_box;
	gchar                 *str;
	GtkWidget             *outer_vbox;
	GtkWidget             *button_box;
	gboolean               allow_postpone;

	gint                   root_monitor = 0;
	GdkScreen             *screen = NULL;
	GdkRectangle           monitor;
	gint                   right_padding;
	gint                   bottom_padding;
	MateConfClient	      *client;

	priv = DRW_BREAK_WINDOW_GET_PRIVATE (window);
	window->priv = priv;

	client = mateconf_client_get_default ();

	priv->break_time = 60 * mateconf_client_get_int (client,
						      MATECONF_PATH "/break_time",
						      NULL);

	allow_postpone = mateconf_client_get_bool (client,
					      MATECONF_PATH "/allow_postpone",
					      NULL);
	g_object_unref (client);

	g_object_set (window, "type", GTK_WINDOW_POPUP, NULL);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_fullscreen (GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);

	screen = gdk_screen_get_default ();
	gdk_screen_get_monitor_geometry (screen, root_monitor, &monitor);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     gdk_screen_get_width (screen),
				     gdk_screen_get_height (screen));

	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
	drw_setup_background (GTK_WIDGET (window));

	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_widget_show (align);

	outer_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (outer_vbox);

	right_padding = gdk_screen_get_width (screen) - monitor.width - monitor.x;
	bottom_padding = gdk_screen_get_height (screen) - monitor.height - monitor.y;

	monitor_box = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (monitor_box),
				   monitor.y,
				   bottom_padding,
				   monitor.x,
				   right_padding);
	gtk_widget_show (monitor_box);

	gtk_container_add (GTK_CONTAINER (window), monitor_box);
	gtk_container_add (GTK_CONTAINER (monitor_box), outer_vbox);

	gtk_box_pack_start (GTK_BOX (outer_vbox), align, TRUE, TRUE, 0);

	if (allow_postpone) {
		button_box = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (button_box);

		gtk_container_set_border_width (GTK_CONTAINER (button_box), 12);

		priv->postpone_button = gtk_button_new_with_mnemonic (_("_Postpone Break"));
		gtk_widget_show (priv->postpone_button);

		gtk_widget_set_sensitive (priv->postpone_button, FALSE);

		if (priv->postpone_sensitize_id) {
			g_source_remove (priv->postpone_sensitize_id);
		}

		priv->postpone_sensitize_id = g_timeout_add_seconds (5,
								     (GSourceFunc) postpone_sensitize_cb,
								     window);

		g_signal_connect (priv->postpone_button,
				  "clicked",
				  G_CALLBACK (postpone_clicked_cb),
				  window);

		gtk_box_pack_end (GTK_BOX (button_box), priv->postpone_button, FALSE, TRUE, 0);

		priv->postpone_entry = gtk_entry_new ();
		gtk_entry_set_has_frame (GTK_ENTRY (priv->postpone_entry), FALSE);

		gtk_box_pack_end (GTK_BOX (button_box), priv->postpone_entry, FALSE, TRUE, 4);

		gtk_box_pack_end (GTK_BOX (outer_vbox), button_box, FALSE, TRUE, 0);
	}

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

	gtk_container_add (GTK_CONTAINER (align), vbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

	priv->image = gtk_image_new_from_stock (GTK_STOCK_STOP, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (priv->image), 1, 0.5);
	gtk_widget_show (priv->image);
	gtk_box_pack_start (GTK_BOX (hbox), priv->image, TRUE, TRUE, 8);

	priv->break_label = gtk_label_new (NULL);
	gtk_widget_show (priv->break_label);

	g_signal_connect (priv->break_label,
			  "expose_event",
			  G_CALLBACK (label_expose_event_cb),
			  NULL);

	g_signal_connect_after (priv->break_label,
				"size_request",
				G_CALLBACK (label_size_request_cb),
				NULL);

	str = g_strdup_printf ("<span size=\"xx-large\" foreground=\"white\"><b>%s</b></span>",
			       _("Take a break!"));
	gtk_label_set_markup (GTK_LABEL (priv->break_label), str);
	g_free (str);

	gtk_box_pack_start (GTK_BOX (hbox), priv->break_label, FALSE, FALSE, 12);


	priv->clock_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->clock_label), 0.5, 0.5);
	gtk_widget_show (priv->clock_label);
	gtk_box_pack_start (GTK_BOX (vbox), priv->clock_label, TRUE, TRUE, 8);

	g_signal_connect (priv->clock_label,
			  "expose_event",
			  G_CALLBACK (label_expose_event_cb),
			  NULL);

	g_signal_connect_after (priv->clock_label,
				"size_request",
				G_CALLBACK (label_size_request_cb),
				NULL);

	gtk_window_stick (GTK_WINDOW (window));

	priv->timer = drw_timer_new ();

	/* Make sure we have a valid time label from the start. */
	clock_timeout_cb (window);

	priv->clock_timeout_id = g_timeout_add (1000,
						(GSourceFunc) clock_timeout_cb,
						window);
#ifdef HAVE_CANBERRA_GTK
	ca_context_play (ca_gtk_context_get (), 0, CA_PROP_EVENT_ID, "desktop-screen-lock", NULL);
#endif
}

static void
drw_break_window_finalize (GObject *object)
{
	DrwBreakWindow        *window = DRW_BREAK_WINDOW (object);
	DrwBreakWindowPrivate *priv;

	priv = window->priv;

	if (priv->clock_timeout_id != 0) {
		g_source_remove (priv->clock_timeout_id);
	}

	if (priv->postpone_timeout_id != 0) {
		g_source_remove (priv->postpone_timeout_id);
	}

	if (priv->postpone_sensitize_id != 0) {
		g_source_remove (priv->postpone_sensitize_id);
	}

	window->priv = NULL;

	if (G_OBJECT_CLASS (drw_break_window_parent_class)->finalize) {
		(* G_OBJECT_CLASS (drw_break_window_parent_class)->finalize) (object);
        }
}

static void
drw_break_window_dispose (GObject *object)
{
	DrwBreakWindow        *window = DRW_BREAK_WINDOW (object);
	DrwBreakWindowPrivate *priv;

	priv = window->priv;

	if (priv->timer) {
		drw_timer_destroy (priv->timer);
		priv->timer = NULL;
	}

	if (priv->clock_timeout_id != 0) {
		g_source_remove (priv->clock_timeout_id);
		priv->clock_timeout_id = 0;
	}

	if (priv->postpone_timeout_id != 0) {
		g_source_remove (priv->postpone_timeout_id);
		priv->postpone_timeout_id = 0;
	}

	if (priv->postpone_sensitize_id != 0) {
		g_source_remove (priv->postpone_sensitize_id);
	}

	if (G_OBJECT_CLASS (drw_break_window_parent_class)->dispose) {
		(* G_OBJECT_CLASS (drw_break_window_parent_class)->dispose) (object);
	}
}

GtkWidget *
drw_break_window_new (void)
{
	GObject *object;

	object = g_object_new (DRW_TYPE_BREAK_WINDOW,
			       "type", GTK_WINDOW_POPUP,
			       "skip-taskbar-hint", TRUE,
			       "skip-pager-hint", TRUE,
			       "focus-on-map", TRUE,
			       NULL);

	return GTK_WIDGET (object);
}

static gboolean
postpone_sensitize_cb (DrwBreakWindow *window)
{
	DrwBreakWindowPrivate *priv;

	priv = window->priv;

	gtk_widget_set_sensitive (priv->postpone_button, TRUE);

	priv->postpone_sensitize_id = 0;
	return FALSE;
}

static gboolean
clock_timeout_cb (DrwBreakWindow *window)
{
	DrwBreakWindowPrivate *priv;
	gchar                 *txt;
	gint                   minutes;
	gint                   seconds;

	g_return_val_if_fail (DRW_IS_BREAK_WINDOW (window), FALSE);

	priv = window->priv;

	seconds = 1 + priv->break_time - drw_timer_elapsed (priv->timer);
	seconds = MAX (0, seconds);

	if (seconds == 0) {
		/* Zero this out so the finalizer doesn't try to remove the
		 * source, which would be done in the timeout callback ==
		 * no-no.
		 */
		priv->clock_timeout_id = 0;

#ifdef HAVE_CANBERRA_GTK
		ca_context_play (ca_gtk_context_get (), 0, CA_PROP_EVENT_ID, "alarm-clock-elapsed", NULL);
#endif
		g_signal_emit (window, signals[DONE], 0, NULL);

		return FALSE;
	}

	minutes = seconds / 60;
	seconds -= minutes * 60;

	txt = g_strdup_printf ("<span size=\"25000\" foreground=\"white\"><b>%d:%02d</b></span>",
			       minutes,
			       seconds);
	gtk_label_set_markup (GTK_LABEL (priv->clock_label), txt);
	g_free (txt);

	return TRUE;
}

static void
postpone_entry_activate_cb (GtkWidget      *entry,
			  DrwBreakWindow *window)
{
	const gchar *str;
	gchar *phrase;
	MateConfClient *client = mateconf_client_get_default();

	str = gtk_entry_get_text (GTK_ENTRY (entry));

	phrase = mateconf_client_get_string (client,
					  MATECONF_PATH "/unlock_phrase",
					  NULL);
	g_object_unref (client);

	if (!strcmp (str, phrase)) {
		g_signal_emit (window, signals[POSTPONE], 0, NULL);
		g_free (phrase);
		return;
	}

	g_free (phrase);
	gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static gboolean
grab_on_window (GdkWindow *window,
		guint32    activate_time)
{
	if ((gdk_pointer_grab (window, TRUE,
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK |
			       GDK_POINTER_MOTION_MASK,
			       NULL, NULL, activate_time) == 0)) {
		if (gdk_keyboard_grab (window, TRUE,
			       activate_time) == 0)
			return TRUE;
		else {
			gdk_pointer_ungrab (activate_time);
			return FALSE;
		}
	}

	return FALSE;
}

static gboolean
postpone_cancel_cb (DrwBreakWindow *window)
{
	DrwBreakWindowPrivate *priv;

	priv = window->priv;

	gtk_entry_set_text (GTK_ENTRY (priv->postpone_entry), "");
	gtk_widget_hide (priv->postpone_entry);

	priv->postpone_timeout_id = 0;

	return FALSE;
}

static gboolean
postpone_entry_key_press_event_cb (GtkEntry       *entry,
				   GdkEventKey    *event,
				   DrwBreakWindow *window)
{
	DrwBreakWindowPrivate *priv;

	priv = window->priv;

	if (event->keyval == GDK_Escape) {
		if (priv->postpone_timeout_id) {
			g_source_remove (priv->postpone_timeout_id);
		}

		postpone_cancel_cb (window);

		return TRUE;
	}

	g_source_remove (priv->postpone_timeout_id);

	priv->postpone_timeout_id = g_timeout_add_seconds (POSTPONE_CANCEL, (GSourceFunc) postpone_cancel_cb, window);

	return FALSE;
}

static void
postpone_clicked_cb (GtkWidget *button,
		     GtkWidget *window)
{
	DrwBreakWindow        *bw = DRW_BREAK_WINDOW (window);
	DrwBreakWindowPrivate *priv = bw->priv;
	gchar                 *phrase;

	/* Disable the phrase for now. */
	phrase = NULL; /*mateconf_client_get_string (mateconf_client_get_default (),
					  MATECONF_PATH "/unlock_phrase",
					  NULL);*/

	if (!phrase || !phrase[0]) {
		g_signal_emit (window, signals[POSTPONE], 0, NULL);
		return;
	}

	if (gtk_widget_get_visible (priv->postpone_entry)) {
		gtk_widget_activate (priv->postpone_entry);
		return;
	}

	gtk_widget_show (priv->postpone_entry);

	priv->postpone_timeout_id = g_timeout_add_seconds (POSTPONE_CANCEL, (GSourceFunc) postpone_cancel_cb, bw);

	grab_on_window (gtk_widget_get_window (priv->postpone_entry),  gtk_get_current_event_time ());

	gtk_widget_grab_focus (priv->postpone_entry);

	g_signal_connect (priv->postpone_entry,
			  "activate",
			  G_CALLBACK (postpone_entry_activate_cb),
			  bw);

	g_signal_connect (priv->postpone_entry,
			  "key_press_event",
			  G_CALLBACK (postpone_entry_key_press_event_cb),
			  bw);
}

static void
get_layout_location (GtkLabel *label,
                     gint     *xp,
                     gint     *yp)
{
	GtkMisc        *misc;
	GtkWidget      *widget;
	GtkAllocation  widget_allocation;
	GtkRequisition widget_requisition;
	gfloat         xalign, yalign;
	gint           x, y;
	gint           xpad, ypad;

	misc = GTK_MISC (label);
	widget = GTK_WIDGET (label);

	gtk_misc_get_alignment (misc, &xalign, &yalign);
	gtk_misc_get_padding (misc, &xpad, &ypad);
	gtk_widget_get_allocation (widget, &widget_allocation);
	gtk_widget_get_requisition (widget, &widget_requisition);

	if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_LTR)
		xalign = 1.0 - xalign;

	x = floor (widget_allocation.x + (int)xpad
		   + ((widget_allocation.width - widget_requisition.width - 1) * xalign)
		   + 0.5);

	y = floor (widget_allocation.y + (int)ypad
		   + ((widget_allocation.height - widget_requisition.height - 1) * yalign)
		   + 0.5);

	if (xp) {
		*xp = x;
	}

	if (yp) {
		*yp = y;
	}
}

static gboolean
label_expose_event_cb (GtkLabel       *label,
		       GdkEventExpose *event,
		       gpointer        user_data)
{
	gint       x, y;
	GdkColor   color;
	GtkWidget *widget;
	GdkWindow *window;
	GdkGC     *gc;

	color.red = 0;
	color.green = 0;
	color.blue = 0;
	color.pixel = 0;

	get_layout_location (label, &x, &y);

	widget = GTK_WIDGET (label);
	window = gtk_widget_get_window (widget);

	gc = gdk_gc_new (window);
	gdk_gc_set_rgb_fg_color (gc, &color);
	gdk_gc_set_clip_rectangle (gc, &event->area);

	gdk_draw_layout_with_colors (window,
				     gc,
				     x + 1,
				     y + 1,
				     gtk_label_get_layout (label),
				     &color,
				     NULL);
	g_object_unref (gc);

	gtk_paint_layout (gtk_widget_get_style (widget),
			  window,
			  gtk_widget_get_state (widget),
			  FALSE,
			  &event->area,
			  widget,
			  "label",
			  x, y,
			  gtk_label_get_layout (label));

	return TRUE;
}

static void
label_size_request_cb (GtkLabel       *label,
		       GtkRequisition *requisition,
		       gpointer        user_data)
{
	requisition->width += 1;
	requisition->height += 1;
}
