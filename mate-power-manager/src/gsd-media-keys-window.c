/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gsd-media-keys-window.h"

#define DIALOG_TIMEOUT 2000     /* dialog timeout in ms */
#define DIALOG_FADE_TIMEOUT 1500 /* timeout before fade starts */
#define FADE_TIMEOUT 10        /* timeout in ms between each frame of the fade */

#define BG_ALPHA 0.75
#define FG_ALPHA 1.00

#define GSD_MEDIA_KEYS_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MEDIA_KEYS_WINDOW, GsdMediaKeysWindowPrivate))

struct GsdMediaKeysWindowPrivate
{
        guint                    is_composited : 1;
        guint                    hide_timeout_id;
        guint                    fade_timeout_id;
        double                   fade_out_alpha;
        GsdMediaKeysWindowAction action;
        char                    *icon_name;
        gboolean                 show_level;

        guint                    volume_muted : 1;
        int                      volume_level;

        GtkImage                *image;
        GtkWidget               *progress;
};

G_DEFINE_TYPE (GsdMediaKeysWindow, gsd_media_keys_window, GTK_TYPE_WINDOW)

static gboolean
fade_timeout (GsdMediaKeysWindow *window)
{
        if (window->priv->fade_out_alpha <= 0.0) {
                gtk_widget_hide (GTK_WIDGET (window));

                /* Reset it for the next time */
                window->priv->fade_out_alpha = 1.0;
                window->priv->fade_timeout_id = 0;

                return FALSE;
        } else {
                GdkRectangle rect;
                GtkWidget *win = GTK_WIDGET (window);
                GtkAllocation allocation;

                window->priv->fade_out_alpha -= 0.10;

                rect.x = 0;
                rect.y = 0;
                gtk_widget_get_allocation (win, &allocation);
                rect.width = allocation.width;
                rect.height = allocation.height;

                gtk_widget_realize (win);
                gdk_window_invalidate_rect (gtk_widget_get_window (win), &rect, FALSE);
        }

        return TRUE;
}

static gboolean
hide_timeout (GsdMediaKeysWindow *window)
{
        if (window->priv->is_composited) {
                window->priv->hide_timeout_id = 0;
                window->priv->fade_timeout_id = g_timeout_add (FADE_TIMEOUT,
                                                               (GSourceFunc) fade_timeout,
                                                               window);
        } else {
                gtk_widget_hide (GTK_WIDGET (window));
        }

        return FALSE;
}

static void
remove_hide_timeout (GsdMediaKeysWindow *window)
{
        if (window->priv->hide_timeout_id != 0) {
                g_source_remove (window->priv->hide_timeout_id);
                window->priv->hide_timeout_id = 0;
        }

        if (window->priv->fade_timeout_id != 0) {
                g_source_remove (window->priv->fade_timeout_id);
                window->priv->fade_timeout_id = 0;
                window->priv->fade_out_alpha = 1.0;
        }
}

static void
add_hide_timeout (GsdMediaKeysWindow *window)
{
        int timeout;

        if (window->priv->is_composited) {
                timeout = DIALOG_FADE_TIMEOUT;
        } else {
                timeout = DIALOG_TIMEOUT;
        }
        window->priv->hide_timeout_id = g_timeout_add (timeout,
                                                       (GSourceFunc) hide_timeout,
                                                       window);
}

static void
update_window (GsdMediaKeysWindow *window)
{
        remove_hide_timeout (window);
        add_hide_timeout (window);

        if (window->priv->is_composited) {
                gtk_widget_queue_draw (GTK_WIDGET (window));
        }
}

static void
volume_controls_set_visible (GsdMediaKeysWindow *window,
                             gboolean            visible)
{
        if (window->priv->progress == NULL)
                return;

        if (visible) {
                gtk_widget_show (window->priv->progress);
        } else {
                gtk_widget_hide (window->priv->progress);
        }
}

static void
window_set_icon_name (GsdMediaKeysWindow *window,
                      const char         *name)
{
        if (window->priv->image == NULL)
                return;

        gtk_image_set_from_icon_name (window->priv->image,
                                      name, GTK_ICON_SIZE_DIALOG);
}

static void
action_changed (GsdMediaKeysWindow *window)
{
        if (! window->priv->is_composited) {
                switch (window->priv->action) {
                case GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME:
                        volume_controls_set_visible (window, TRUE);

                        if (window->priv->volume_muted) {
                                window_set_icon_name (window, "audio-volume-muted");
                        } else {
                                window_set_icon_name (window, "audio-volume-high");
                        }

                        break;
                case GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM:
                        volume_controls_set_visible (window, window->priv->show_level);
                        window_set_icon_name (window, window->priv->icon_name);
                        break;
                default:
                        g_assert_not_reached ();
                        break;
                }
        }

        update_window (window);
}

static void
volume_level_changed (GsdMediaKeysWindow *window)
{
        update_window (window);

        if (!window->priv->is_composited && window->priv->progress != NULL) {
                double fraction;

                fraction = (double) window->priv->volume_level / 100.0;

                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->progress),
                                               fraction);
        }
}

static void
volume_muted_changed (GsdMediaKeysWindow *window)
{
        update_window (window);

        if (! window->priv->is_composited) {
                if (window->priv->volume_muted) {
                        window_set_icon_name (window, "audio-volume-muted");
                } else {
                        window_set_icon_name (window, "audio-volume-high");
                }
        }
}

void
gsd_media_keys_window_set_action (GsdMediaKeysWindow      *window,
                                  GsdMediaKeysWindowAction action)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));
        g_return_if_fail (action == GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME);

        if (window->priv->action != action) {
                window->priv->action = action;
                action_changed (window);
        } else {
                update_window (window);
        }
}

void
gsd_media_keys_window_set_action_custom (GsdMediaKeysWindow      *window,
                                         const char              *icon_name,
                                         gboolean                 show_level)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));
        g_return_if_fail (icon_name != NULL);

        if (window->priv->action != GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM ||
            g_strcmp0 (window->priv->icon_name, icon_name) != 0 ||
            window->priv->show_level != show_level) {
                window->priv->action = GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM;
                g_free (window->priv->icon_name);
                window->priv->icon_name = g_strdup (icon_name);
                window->priv->show_level = show_level;
                action_changed (window);
        } else {
                update_window (window);
        }
}

void
gsd_media_keys_window_set_volume_muted (GsdMediaKeysWindow *window,
                                        gboolean            muted)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));

        if (window->priv->volume_muted != muted) {
                window->priv->volume_muted = muted;
                volume_muted_changed (window);
        }
}

void
gsd_media_keys_window_set_volume_level (GsdMediaKeysWindow *window,
                                        int                 level)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));

        if (window->priv->volume_level != level) {
                window->priv->volume_level = level;
                volume_level_changed (window);
        }
}

static void
rounded_rectangle (cairo_t* cr,
                   gdouble  aspect,
                   gdouble  x,
                   gdouble  y,
                   gdouble  corner_radius,
                   gdouble  width,
                   gdouble  height)
{
        gdouble radius = corner_radius / aspect;

        cairo_move_to (cr, x + radius, y);

        cairo_line_to (cr,
                       x + width - radius,
                       y);
        cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90.0f * G_PI / 180.0f,
                   0.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + width,
                       y + height - radius);
        cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0.0f * G_PI / 180.0f,
                   90.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + radius,
                       y + height);
        cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90.0f * G_PI / 180.0f,
                   180.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x,
                       y + radius);
        cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180.0f * G_PI / 180.0f,
                   270.0f * G_PI / 180.0f);
        cairo_close_path (cr);
}

static GdkPixbuf *
load_pixbuf (GsdMediaKeysWindow *window,
             const char         *name,
             int                 icon_size)
{
        GtkIconTheme *theme;
        GdkPixbuf    *pixbuf;

        if (window != NULL && gtk_widget_has_screen (GTK_WIDGET (window))) {
                theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)));
        } else {
                theme = gtk_icon_theme_get_default ();
        }

        pixbuf = gtk_icon_theme_load_icon (theme,
                                           name,
                                           icon_size,
                                           GTK_ICON_LOOKUP_FORCE_SVG,
                                           NULL);

        /* make sure the pixbuf is close to the requested size
         * this is necessary because GTK_ICON_LOOKUP_FORCE_SVG
         * seems to be broken */
        if (pixbuf != NULL) {
                int width;

                width = gdk_pixbuf_get_width (pixbuf);
                if (width < (float)icon_size * 0.75) {
                        g_object_unref (pixbuf);
                        pixbuf = NULL;
                }
        }

        return pixbuf;
}

static void
draw_eject (cairo_t *cr,
            double   _x0,
            double   _y0,
            double   width,
            double   height)
{
        int box_height;
        int tri_height;
        int separation;

        box_height = height * 0.2;
        separation = box_height / 3;
        tri_height = height - box_height - separation;

        cairo_rectangle (cr, _x0, _y0 + height - box_height, width, box_height);

        cairo_move_to (cr, _x0, _y0 + tri_height);
        cairo_rel_line_to (cr, width, 0);
        cairo_rel_line_to (cr, -width / 2, -tri_height);
        cairo_rel_line_to (cr, -width / 2, tri_height);
        cairo_close_path (cr);
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, FG_ALPHA);
        cairo_fill_preserve (cr);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, FG_ALPHA / 2);
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
}

static void
draw_waves (cairo_t *cr,
            double   cx,
            double   cy,
            double   max_radius,
            int      volume_level)
{
        const int n_waves = 3;
        int last_wave;
        int i;

        last_wave = n_waves * volume_level / 100;

        for (i = 0; i < n_waves; i++) {
                double angle1;
                double angle2;
                double radius;
                double alpha;

                angle1 = -M_PI / 4;
                angle2 = M_PI / 4;

                if (i < last_wave)
                        alpha = 1.0;
                else if (i > last_wave)
                        alpha = 0.1;
                else alpha = 0.1 + 0.9 * (n_waves * volume_level % 100) / 100.0;

                radius = (i + 1) * (max_radius / n_waves);
                cairo_arc (cr, cx, cy, radius, angle1, angle2);
                cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, alpha / 2);
                cairo_set_line_width (cr, 14);
                cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke_preserve (cr);

                cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, alpha);
                cairo_set_line_width (cr, 10);
                cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke (cr);
        }
}

static void
draw_cross (cairo_t *cr,
            double   cx,
            double   cy,
            double   size)
{
        cairo_move_to (cr, cx, cy - size/2.0);
        cairo_rel_line_to (cr, size, size);

        cairo_move_to (cr, cx, cy + size/2.0);
        cairo_rel_line_to (cr, size, -size);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, FG_ALPHA / 2);
        cairo_set_line_width (cr, 14);
        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke_preserve (cr);

        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, FG_ALPHA);
        cairo_set_line_width (cr, 10);
        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke (cr);
}

static void
draw_speaker (cairo_t *cr,
              double   cx,
              double   cy,
              double   width,
              double   height)
{
        double box_width;
        double box_height;
        double _x0;
        double _y0;

        box_width = width / 3;
        box_height = height / 3;

        _x0 = cx - (width / 2) + box_width;
        _y0 = cy - box_height / 2;

        cairo_move_to (cr, _x0, _y0);
        cairo_rel_line_to (cr, - box_width, 0);
        cairo_rel_line_to (cr, 0, box_height);
        cairo_rel_line_to (cr, box_width, 0);

        cairo_line_to (cr, cx + box_width, cy + height / 2);
        cairo_rel_line_to (cr, 0, -height);
        cairo_line_to (cr, _x0, _y0);
        cairo_close_path (cr);

        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, FG_ALPHA);
        cairo_fill_preserve (cr);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, FG_ALPHA / 2);
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
}

static gboolean
render_speaker (GsdMediaKeysWindow *window,
                cairo_t            *cr,
                double              _x0,
                double              _y0,
                double              width,
                double              height)
{
        GdkPixbuf         *pixbuf;
        int                icon_size;
        int                n;
        static const char *icon_names[] = {
                "audio-volume-muted",
                "audio-volume-low",
                "audio-volume-medium",
                "audio-volume-high",
                NULL
        };

        if (window->priv->volume_muted) {
                n = 0;
        } else {
                /* select image */
                n = 3 * window->priv->volume_level / 100 + 1;
                if (n < 1) {
                        n = 1;
                } else if (n > 3) {
                        n = 3;
                }
        }

        icon_size = (int)width;

        pixbuf = load_pixbuf (window, icon_names[n], icon_size);

        if (pixbuf == NULL) {
                return FALSE;
        }

        gdk_cairo_set_source_pixbuf (cr, pixbuf, _x0, _y0);
        cairo_paint_with_alpha (cr, FG_ALPHA);

        g_object_unref (pixbuf);

        return TRUE;
}

static void
color_reverse (const GdkColor *a,
               GdkColor       *b)
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

        gtk_rgb_to_hsv (red, green, blue, &h, &s, &v);

        v = 0.5 + (0.5 - v);
        if (v > 1.0)
                v = 1.0;
        else if (v < 0.0)
                v = 0.0;

        gtk_hsv_to_rgb (h, s, v, &red, &green, &blue);

        b->red = red * 65535.0;
        b->green = green * 65535.0;
        b->blue = blue * 65535.0;
}

static void
draw_volume_boxes (GsdMediaKeysWindow *window,
                   cairo_t            *cr,
                   double              percentage,
                   double              _x0,
                   double              _y0,
                   double              width,
                   double              height)
{
        gdouble   x1;
        GdkColor  color;
        double    r, g, b;
        GtkStyle *style;

        _x0 += 0.5;
        _y0 += 0.5;
        height = round (height) - 1;
        width = round (width) - 1;
        x1 = round ((width - 1) * percentage);
        style = gtk_widget_get_style (GTK_WIDGET (window));

        /* bar background */
        color_reverse (&style->dark[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        rounded_rectangle (cr, 1.0, _x0, _y0, height / 6, width, height);
        cairo_set_source_rgba (cr, r, g, b, FG_ALPHA / 2);
        cairo_fill_preserve (cr);

        /* bar border */
        color_reverse (&style->light[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, FG_ALPHA / 2);
        cairo_set_line_width (cr, 1);
        cairo_stroke (cr);

        /* bar progress */
        if (percentage < 0.01)
                return;
        color = style->bg[GTK_STATE_NORMAL];
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        rounded_rectangle (cr, 1.0, _x0 + 0.5, _y0 + 0.5, height / 6 - 0.5, x1, height - 1);
        cairo_set_source_rgba (cr, r, g, b, FG_ALPHA);
        cairo_fill (cr);
}

static void
draw_action_volume (GsdMediaKeysWindow *window,
                    cairo_t            *cr)
{
        int window_width;
        int window_height;
        double icon_box_width;
        double icon_box_height;
        double icon_box_x0;
        double icon_box_y0;
        double volume_box_x0;
        double volume_box_y0;
        double volume_box_width;
        double volume_box_height;
        gboolean res;

        gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

        icon_box_width = round (window_width * 0.65);
        icon_box_height = round (window_height * 0.65);
        volume_box_width = icon_box_width;
        volume_box_height = round (window_height * 0.05);

        icon_box_x0 = (window_width - icon_box_width) / 2;
        icon_box_y0 = (window_height - icon_box_height - volume_box_height) / 2;
        volume_box_x0 = round (icon_box_x0);
        volume_box_y0 = round (icon_box_height + icon_box_y0);

#if 0
        g_message ("icon box: w=%f h=%f _x0=%f _y0=%f",
                   icon_box_width,
                   icon_box_height,
                   icon_box_x0,
                   icon_box_y0);
        g_message ("volume box: w=%f h=%f _x0=%f _y0=%f",
                   volume_box_width,
                   volume_box_height,
                   volume_box_x0,
                   volume_box_y0);
#endif

        res = render_speaker (window,
                              cr,
                              icon_box_x0, icon_box_y0,
                              icon_box_width, icon_box_height);
        if (! res) {
                double speaker_width;
                double speaker_height;
                double speaker_cx;
                double speaker_cy;

                speaker_width = icon_box_width * 0.5;
                speaker_height = icon_box_height * 0.75;
                speaker_cx = icon_box_x0 + speaker_width / 2;
                speaker_cy = icon_box_y0 + speaker_height / 2;

#if 0
                g_message ("speaker box: w=%f h=%f cx=%f cy=%f",
                           speaker_width,
                           speaker_height,
                           speaker_cx,
                           speaker_cy);
#endif

                /* draw speaker symbol */
                draw_speaker (cr, speaker_cx, speaker_cy, speaker_width, speaker_height);

                if (! window->priv->volume_muted) {
                        /* draw sound waves */
                        double wave_x0;
                        double wave_y0;
                        double wave_radius;

                        wave_x0 = window_width / 2;
                        wave_y0 = speaker_cy;
                        wave_radius = icon_box_width / 2;

                        draw_waves (cr, wave_x0, wave_y0, wave_radius, window->priv->volume_level);
                } else {
                        /* draw 'mute' cross */
                        double cross_x0;
                        double cross_y0;
                        double cross_size;

                        cross_size = speaker_width * 3 / 4;
                        cross_x0 = icon_box_x0 + icon_box_width - cross_size;
                        cross_y0 = speaker_cy;

                        draw_cross (cr, cross_x0, cross_y0, cross_size);
                }
        }

        /* draw volume meter */
        draw_volume_boxes (window,
                           cr,
                           (double)window->priv->volume_level / 100.0,
                           volume_box_x0,
                           volume_box_y0,
                           volume_box_width,
                           volume_box_height);
}

static gboolean
render_custom (GsdMediaKeysWindow *window,
               cairo_t            *cr,
               double              _x0,
               double              _y0,
               double              width,
               double              height)
{
        GdkPixbuf         *pixbuf;
        int                icon_size;

        icon_size = (int)width;

        pixbuf = load_pixbuf (window, window->priv->icon_name, icon_size);

        if (pixbuf == NULL) {
                char *name;
                if (gtk_widget_get_direction (GTK_WIDGET (window)) == GTK_TEXT_DIR_RTL)
                        name = g_strdup_printf ("%s-rtl", window->priv->icon_name);
                else
                        name = g_strdup_printf ("%s-ltr", window->priv->icon_name);
                pixbuf = load_pixbuf (window, name, icon_size);
                g_free (name);
                if (pixbuf == NULL)
                        return FALSE;
        }

        gdk_cairo_set_source_pixbuf (cr, pixbuf, _x0, _y0);
        cairo_paint_with_alpha (cr, FG_ALPHA);

        g_object_unref (pixbuf);

        return TRUE;
}

static void
draw_action_custom (GsdMediaKeysWindow *window,
                    cairo_t            *cr)
{
        int window_width;
        int window_height;
        double icon_box_width;
        double icon_box_height;
        double icon_box_x0;
        double icon_box_y0;
        double bright_box_x0;
        double bright_box_y0;
        double bright_box_width;
        double bright_box_height;
        gboolean res;

        gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

        icon_box_width = round (window_width * 0.65);
        icon_box_height = round (window_height * 0.65);
        bright_box_width = round (icon_box_width);
        bright_box_height = round (window_height * 0.05);

        icon_box_x0 = (window_width - icon_box_width) / 2;
        icon_box_y0 = (window_height - icon_box_height - bright_box_height) / 2;
        bright_box_x0 = round (icon_box_x0);
        bright_box_y0 = round (icon_box_height + icon_box_y0);

#if 0
        g_message ("icon box: w=%f h=%f _x0=%f _y0=%f",
                   icon_box_width,
                   icon_box_height,
                   icon_box_x0,
                   icon_box_y0);
        g_message ("brightness box: w=%f h=%f _x0=%f _y0=%f",
                   bright_box_width,
                   bright_box_height,
                   bright_box_x0,
                   bright_box_y0);
#endif

        res = render_custom (window,
                             cr,
                             icon_box_x0, icon_box_y0,
                             icon_box_width, icon_box_height);
        if (! res && g_strcmp0 (window->priv->icon_name, "media-eject") == 0) {
                /* draw eject symbol */
                draw_eject (cr,
                            icon_box_x0, icon_box_y0,
                            icon_box_width, icon_box_height);
        }

        if (window->priv->show_level != FALSE) {
                /* draw volume meter */
                draw_volume_boxes (window,
                                   cr,
                                   (double)window->priv->volume_level / 100.0,
                                   bright_box_x0,
                                   bright_box_y0,
                                   bright_box_width,
                                   bright_box_height);
        }
}

static void
draw_action (GsdMediaKeysWindow *window,
             cairo_t            *cr)
{
        switch (window->priv->action) {
        case GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME:
                draw_action_volume (window, cr);
                break;
        case GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM:
                draw_action_custom (window, cr);
                break;
        default:
                break;
        }
}

static gboolean
on_expose_event (GtkWidget          *widget,
                 GdkEventExpose     *event,
                 GsdMediaKeysWindow *window)
{
        cairo_t         *context;
        cairo_t         *cr;
        cairo_surface_t *surface;
        int              width;
        int              height;
        GtkStyle        *style;
        GdkColor         color;
        double           r, g, b;

        context = gdk_cairo_create (gtk_widget_get_window (widget));

        style = gtk_widget_get_style (widget);
        cairo_set_operator (context, CAIRO_OPERATOR_SOURCE);
        gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

        surface = cairo_surface_create_similar (cairo_get_target (context),
                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                width,
                                                height);

        if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }

        cr = cairo_create (surface);
        if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_paint (cr);

        /* draw a box */
        rounded_rectangle (cr, 1.0, 0.5, 0.5, height / 10, width-1, height-1);
        color_reverse (&style->bg[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA);
        cairo_fill_preserve (cr);

        color_reverse (&style->text_aa[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA / 2);
        cairo_set_line_width (cr, 1);
        cairo_stroke (cr);

        /* draw action */
        draw_action (window, cr);

        cairo_destroy (cr);

        /* Make sure we have a transparent background */
        cairo_rectangle (context, 0, 0, width, height);
        cairo_set_source_rgba (context, 0.0, 0.0, 0.0, 0.0);
        cairo_fill (context);

        cairo_set_source_surface (context, surface, 0, 0);
        cairo_paint_with_alpha (context, window->priv->fade_out_alpha);

 done:
        if (surface != NULL) {
                cairo_surface_destroy (surface);
        }
        cairo_destroy (context);

        return FALSE;
}

static void
gsd_media_keys_window_real_show (GtkWidget *widget)
{
        GsdMediaKeysWindow *window;

        if (GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->show) {
                GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->show (widget);
        }

        window = GSD_MEDIA_KEYS_WINDOW (widget);
        remove_hide_timeout (window);
        add_hide_timeout (window);
}

static void
gsd_media_keys_window_real_hide (GtkWidget *widget)
{
        GsdMediaKeysWindow *window;

        if (GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->hide) {
                GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->hide (widget);
        }

        window = GSD_MEDIA_KEYS_WINDOW (widget);
        remove_hide_timeout (window);
}

static void
gsd_media_keys_window_real_realize (GtkWidget *widget)
{
        GdkColormap *colormap;
        GtkAllocation allocation;
        GdkBitmap *mask;
        cairo_t *cr;

        colormap = gdk_screen_get_rgba_colormap (gtk_widget_get_screen (widget));

        if (colormap != NULL) {
                gtk_widget_set_colormap (widget, colormap);
        }

        if (GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->realize) {
                GTK_WIDGET_CLASS (gsd_media_keys_window_parent_class)->realize (widget);
        }

        gtk_widget_get_allocation (widget, &allocation);
        mask = gdk_pixmap_new (gtk_widget_get_window (widget),
                               allocation.width,
                               allocation.height,
                               1);
        cr = gdk_cairo_create (mask);

        cairo_set_source_rgba (cr, 1., 1., 1., 0.);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);

        /* make the whole window ignore events */
        gdk_window_input_shape_combine_mask (gtk_widget_get_window (widget), mask, 0, 0);
        g_object_unref (mask);
        cairo_destroy (cr);
}

static void
gsd_media_keys_window_class_init (GsdMediaKeysWindowClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        widget_class->show = gsd_media_keys_window_real_show;
        widget_class->hide = gsd_media_keys_window_real_hide;
        widget_class->realize = gsd_media_keys_window_real_realize;

        g_type_class_add_private (klass, sizeof (GsdMediaKeysWindowPrivate));
}

gboolean
gsd_media_keys_window_is_valid (GsdMediaKeysWindow *window)
{
        GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (window));
        return gdk_screen_is_composited (screen) == window->priv->is_composited;
}

static void
gsd_media_keys_window_init (GsdMediaKeysWindow *window)
{
        GdkScreen *screen;

        window->priv = GSD_MEDIA_KEYS_WINDOW_GET_PRIVATE (window);

        screen = gtk_widget_get_screen (GTK_WIDGET (window));

        window->priv->is_composited = gdk_screen_is_composited (screen);

        if (window->priv->is_composited) {
                gdouble scalew, scaleh, scale;
                gint size;

                gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
                gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

                /* assume 130x130 on a 640x480 display and scale from there */
                scalew = gdk_screen_get_width (screen) / 640.0;
                scaleh = gdk_screen_get_height (screen) / 480.0;
                scale = MIN (scalew, scaleh);
                size = 130 * MAX (1, scale);

                gtk_window_set_default_size (GTK_WINDOW (window), size, size);
                g_signal_connect (window, "expose-event", G_CALLBACK (on_expose_event), window);

                window->priv->fade_out_alpha = 1.0;
        } else {
                GtkBuilder *builder;
                const gchar *objects[] = {"acme_frame", NULL};
                GtkWidget *frame;

                builder = gtk_builder_new ();
                gtk_builder_add_objects_from_file (builder,
                                                   GTKBUILDERDIR "/acme.ui",
                                                   (char **) objects,
                                                   NULL);

                window->priv->image = GTK_IMAGE (gtk_builder_get_object (builder, "acme_image"));
                window->priv->progress = GTK_WIDGET (gtk_builder_get_object (builder, "acme_volume_progressbar"));
                frame = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "acme_frame"));

                if (frame != NULL) {
                        gtk_container_add (GTK_CONTAINER (window), frame);
                        gtk_widget_show_all (frame);
                }

                /* The builder needs to stay alive until the window
                   takes ownership of the frame (and its children)  */
                g_object_unref (builder);
        }
}

GtkWidget *
gsd_media_keys_window_new (void)
{
        GObject *object;

        object = g_object_new (GSD_TYPE_MEDIA_KEYS_WINDOW,
                               "type", GTK_WINDOW_POPUP,
                               "type-hint", GDK_WINDOW_TYPE_HINT_NOTIFICATION,
                               "skip-taskbar-hint", TRUE,
                               "skip-pager-hint", TRUE,
                               "focus-on-map", FALSE,
                               NULL);

        return GTK_WIDGET (object);
}
