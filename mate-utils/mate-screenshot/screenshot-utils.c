/* screenshot-utils.c - common functions for MATE Screenshot
 *
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#include "config.h"
#include "screenshot-utils.h"

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif

static GtkWidget *selection_window;

#define SELECTION_NAME "_MATE_PANEL_SCREENSHOT"

static char *
get_utf8_property (GdkWindow *window,
		   GdkAtom    atom)
{
  gboolean res;
  GdkAtom utf8_string;
  GdkAtom type;
  int actual_format, actual_length;
  guchar *data;
  char *retval;
  
  utf8_string = gdk_x11_xatom_to_atom (gdk_x11_get_xatom_by_name ("UTF8_STRING"));
  res = gdk_property_get (window, atom, utf8_string,
                          0, G_MAXLONG, FALSE,
                          &type,
                          &actual_format, &actual_length,
                          &data);
  if (!res)
    return NULL;

  if (type != utf8_string || actual_format != 8 || actual_length == 0)
    {
      g_free (data);
      return NULL;
    }

  if (!g_utf8_validate ((gchar *) data, actual_length, NULL))
    {
      char *atom_name = gdk_atom_name (atom);

      g_warning ("Property `%s' (format: %d, length: %d) contained "
                 "invalid UTF-8",
                 atom_name,
                 actual_format,
                 actual_length);

      g_free (atom_name);
      g_free (data);

      return NULL;
    }
  
  retval = g_strndup ((gchar *) data, actual_length);

  g_free (data);
  
  return retval;
}

/* To make sure there is only one screenshot taken at a time,
 * (Imagine key repeat for the print screen key) we hold a selection
 * until we are done taking the screenshot
 */
gboolean
screenshot_grab_lock (void)
{
  GdkAtom selection_atom;
  gboolean result = FALSE;

  selection_atom = gdk_atom_intern (SELECTION_NAME, FALSE);
  gdk_x11_grab_server ();

  if (gdk_selection_owner_get (selection_atom) != NULL)
    goto out;

  selection_window = gtk_invisible_new ();
  gtk_widget_show (selection_window);

  if (!gtk_selection_owner_set (selection_window,
				gdk_atom_intern (SELECTION_NAME, FALSE),
				GDK_CURRENT_TIME))
    {
      gtk_widget_destroy (selection_window);
      selection_window = NULL;
      goto out;
    }

  result = TRUE;

 out:
  gdk_x11_ungrab_server ();
  gdk_flush ();

  return result;
}

void
screenshot_release_lock (void)
{
  if (selection_window)
    {
      gtk_widget_destroy (selection_window);
      selection_window = NULL;
    }

  gdk_flush ();
}

static GdkWindow *
screenshot_find_active_window (void)
{
  GdkWindow *window;
  GdkScreen *default_screen;

  default_screen = gdk_screen_get_default ();
  window = gdk_screen_get_active_window (default_screen);

  return window;
}

static gboolean
screenshot_window_is_desktop (GdkWindow *window)
{
  GdkWindow *root_window = gdk_get_default_root_window ();
  GdkWindowTypeHint window_type_hint;

  if (window == root_window)
    return TRUE;

  window_type_hint = gdk_window_get_type_hint (window);
  if (window_type_hint == GDK_WINDOW_TYPE_HINT_DESKTOP)
    return TRUE;

  return FALSE;
      
}

#define MAXIMUM_WM_REPARENTING_DEPTH 4

static GdkWindow *
look_for_hint_helper (GdkWindow *window,
		      GdkAtom    property,
		      int       depth)
{
  gboolean res;
  GdkAtom actual_type;
  int actual_format, actual_length;
  guchar *data;
  
  res = gdk_property_get (window, property, GDK_NONE,
                          0, 1, FALSE,
                          &actual_type,
                          &actual_format, &actual_length,
                          &data);

  if (res == TRUE &&
      data != NULL &&
      actual_format == 32 &&
      data[0] == 1)
    {
      g_free (data);

      return window;
    }

  if (depth < MAXIMUM_WM_REPARENTING_DEPTH)
    {
      GList *children, *l;

      children = gdk_window_get_children (window);
      if (children != NULL)
        {
          for (l = children; l; l = l->next)
            {
              window = look_for_hint_helper (l->data, property, depth + 1);
              if (window)
                break;
            }

          g_list_free (children);

          if (window)
            return window;
        }
    }

  return NULL;
}

static GdkWindow *
look_for_hint (GdkWindow *window,
	       GdkAtom property)
{
  GdkWindow *retval;

  retval = look_for_hint_helper (window, property, 0);

  return retval;
}

GdkWindow *
screenshot_find_current_window ()
{
  GdkWindow *current_window;

  current_window = screenshot_find_active_window ();
  
  /* If there's no active window, we fall back to returning the
   * window that the cursor is in.
   */
  if (!current_window)
    current_window = gdk_window_at_pointer (NULL, NULL);

  if (current_window)
    {
      if (screenshot_window_is_desktop (current_window))
	/* if the current window is the desktop (e.g. caja), we
	 * return NULL, as getting the whole screen makes more sense.
         */
        return NULL;

      /* Once we have a window, we take the toplevel ancestor. */
      current_window = gdk_window_get_toplevel (current_window);
    }

  return current_window;
}

static void
select_area_button_press (XKeyEvent    *event,
                          GdkRectangle *rect,
                          GdkRectangle *draw_rect)
{
  rect->x = event->x_root;
  rect->y = event->y_root;

  draw_rect->x = rect->x;
  draw_rect->y = rect->y;
  draw_rect->width  = 0;
  draw_rect->height = 0;
}

static void
select_area_update_rect (GtkWidget    *window,
                         GdkRectangle *rect)
{
  if (rect->width <= 0 || rect->height <= 0)
    {
      gtk_widget_hide (window);
      return;
    }

  gtk_window_move (GTK_WINDOW (window), rect->x, rect->y);
  gtk_window_resize (GTK_WINDOW (window), rect->width, rect->height);
  gtk_widget_show (window);
  
  /* We (ab)use app-paintable to indicate if we have an RGBA window */
  if (!gtk_widget_get_app_paintable (window))
    {
      GdkWindow *gdkwindow = gtk_widget_get_window (window);

      /* Shape the window to make only the outline visible */
      if (rect->width > 2 && rect->height > 2)
        {
          GdkRegion *region, *region2;
          GdkRectangle region_rect = { 0, 0,
                                       rect->width - 2, rect->height - 2 };

          region = gdk_region_rectangle (&region_rect);
          region_rect.x++;
          region_rect.y++;
          region_rect.width -= 2;
          region_rect.height -= 2;
          region2 = gdk_region_rectangle (&region_rect);
          gdk_region_subtract (region, region2);

          gdk_window_shape_combine_region (gdkwindow, region, 0, 0);

          gdk_region_destroy (region);
          gdk_region_destroy (region2);
        }
      else
        gdk_window_shape_combine_region (gdkwindow, NULL, 0, 0);
    }
}

static void
select_area_button_release (XKeyEvent    *event,
                            GdkRectangle *rect,
                            GdkRectangle *draw_rect,
                            GtkWidget    *window)
{
  gtk_widget_hide (window);

  rect->width  = ABS (rect->x - event->x_root);
  rect->height = ABS (rect->y - event->y_root);

  rect->x = MIN (rect->x, event->x_root);
  rect->y = MIN (rect->y, event->y_root);
}

static void
select_area_motion_notify (XKeyEvent    *event,
                           GdkRectangle *rect,
                           GdkRectangle *draw_rect,
                           GtkWidget    *window)
{
  draw_rect->width  = ABS (rect->x - event->x_root);
  draw_rect->height = ABS (rect->y - event->y_root);

  draw_rect->x = MIN (rect->x, event->x_root);
  draw_rect->y = MIN (rect->y, event->y_root);

  select_area_update_rect (window, draw_rect);
}

typedef struct {
  GdkRectangle  rect;
  GdkRectangle  draw_rect;
  gboolean      button_pressed;
  GtkWidget    *window;
} select_area_filter_data;

static gboolean
expose (GtkWidget *window, GdkEventExpose *event, gpointer unused)
{
  GtkAllocation allocation;
  GtkStyle *style;
  cairo_t *cr;

  cr = gdk_cairo_create (event->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  style = gtk_widget_get_style (window);

  if (gtk_widget_get_app_paintable (window))
    {
      cairo_set_line_width (cr, 1.0);

      gtk_widget_get_allocation (window, &allocation);

      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, 0, 0, 0, 0);
      cairo_paint (cr);

      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_SELECTED]);
      cairo_paint_with_alpha (cr, 0.25);

      cairo_rectangle (cr, 
                       allocation.x + 0.5, allocation.y + 0.5,
                       allocation.width - 1, allocation.height - 1);
      cairo_stroke (cr);
    }
  else
    {
      gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_SELECTED]);
      cairo_paint (cr);
    }

  cairo_destroy (cr);

  return TRUE;
}

static GtkWidget *
create_select_window (void)
{
  GtkWidget *window;
  GdkScreen *screen;

  screen = gdk_screen_get_default ();

  window = gtk_window_new (GTK_WINDOW_POPUP);
  if (gdk_screen_is_composited (screen) &&
      gdk_screen_get_rgba_colormap (screen))
    {
      gtk_widget_set_colormap (window, gdk_screen_get_rgba_colormap (screen));
      gtk_widget_set_app_paintable (window, TRUE);
    }
  g_signal_connect (window, "expose-event", G_CALLBACK (expose), NULL);

  return window;
}

static GdkFilterReturn
select_area_filter (GdkXEvent *gdk_xevent,
                    GdkEvent  *event,
                    gpointer   user_data)
{
  select_area_filter_data *data = user_data;
  XEvent *xevent = (XEvent *) gdk_xevent;

  switch (xevent->type)
    {
    case ButtonPress:
      if (!data->button_pressed)
        {
          select_area_button_press (&xevent->xkey,
                                    &data->rect, &data->draw_rect);
          data->button_pressed = TRUE;
        }
      return GDK_FILTER_REMOVE;
    case ButtonRelease:
      if (data->button_pressed)
      {
        select_area_button_release (&xevent->xkey,
                                    &data->rect, &data->draw_rect,
                                    data->window);
        gtk_main_quit ();
      }
      return GDK_FILTER_REMOVE;
    case MotionNotify:
      if (data->button_pressed)
        select_area_motion_notify (&xevent->xkey,
                                   &data->rect, &data->draw_rect,
                                   data->window);
      return GDK_FILTER_REMOVE;
    case KeyPress:
      if (xevent->xkey.keycode == XKeysymToKeycode (gdk_display, XK_Escape))
        {
          data->rect.x = 0;
          data->rect.y = 0;
          data->rect.width  = 0;
          data->rect.height = 0;
          gtk_main_quit ();
          return GDK_FILTER_REMOVE;
        }
      break;
    default:
      break;
    }
 
  return GDK_FILTER_CONTINUE;
}

gboolean
screenshot_select_area (int *px,
                        int *py,
                        int *pwidth,
                        int *pheight)
{
  GdkWindow               *root;
  GdkCursor               *cursor;
  select_area_filter_data  data;

  root = gdk_get_default_root_window ();
  cursor = gdk_cursor_new (GDK_CROSSHAIR);

  if (gdk_pointer_grab (root, FALSE,
                        GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK,
                        NULL, cursor,
                        GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    {
      gdk_cursor_unref (cursor);
      return FALSE;
    }

  if (gdk_keyboard_grab (root, FALSE, GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      gdk_cursor_unref (cursor);
      return FALSE;
    }

  gdk_window_add_filter (root, (GdkFilterFunc) select_area_filter, &data);

  gdk_flush ();

  data.rect.x = 0;
  data.rect.y = 0;
  data.rect.width  = 0;
  data.rect.height = 0;
  data.button_pressed = FALSE;
  data.window = create_select_window();

  gtk_main ();

  gdk_window_remove_filter (root, (GdkFilterFunc) select_area_filter, &data);

  gtk_widget_destroy (data.window);

  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gdk_cursor_unref (cursor);

  *px = data.rect.x;
  *py = data.rect.y;
  *pwidth  = data.rect.width;
  *pheight = data.rect.height;

  return TRUE;
}

static Window
find_wm_window (Window xid)
{
  Window root, parent, *children;
  unsigned int nchildren;

  do
    {
      if (XQueryTree (GDK_DISPLAY (), xid, &root,
		      &parent, &children, &nchildren) == 0)
	{
	  g_warning ("Couldn't find window manager window");
	  return None;
	}

      if (root == parent)
	return xid;

      xid = parent;
    }
  while (TRUE);
}

static GdkRegion *
make_region_with_monitors (GdkScreen *screen)
{
  GdkRegion *region;
  int num_monitors;
  int i;

  num_monitors = gdk_screen_get_n_monitors (screen);

  region = gdk_region_new ();

  for (i = 0; i < num_monitors; i++)
    {
      GdkRectangle rect;

      gdk_screen_get_monitor_geometry (screen, i, &rect);
      gdk_region_union_with_rect (region, &rect);
    }

  return region;
}

static void
blank_rectangle_in_pixbuf (GdkPixbuf *pixbuf, GdkRectangle *rect)
{
  int x, y;
  int x2, y2;
  guchar *pixels;
  int rowstride;
  int n_channels;
  guchar *row;
  gboolean has_alpha;

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  
  x2 = rect->x + rect->width;
  y2 = rect->y + rect->height;

  pixels = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  for (y = rect->y; y < y2; y++)
    {
      guchar *p;

      row = pixels + y * rowstride;
      p = row + rect->x * n_channels;

      for (x = rect->x; x < x2; x++)
	{
	  *p++ = 0;
	  *p++ = 0;
	  *p++ = 0;

	  if (has_alpha)
	    *p++ = 255; /* opaque black */
	}
    }
}

static void
blank_region_in_pixbuf (GdkPixbuf *pixbuf, GdkRegion *region)
{
  GdkRectangle *rects;
  int n_rects;
  int i;
  int width, height;
  GdkRectangle pixbuf_rect;

  gdk_region_get_rectangles (region, &rects, &n_rects);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  pixbuf_rect.x	     = 0;
  pixbuf_rect.y	     = 0;
  pixbuf_rect.width  = width;
  pixbuf_rect.height = height;

  for (i = 0; i < n_rects; i++)
    {
      GdkRectangle dest;

      if (gdk_rectangle_intersect (rects + i, &pixbuf_rect, &dest))
	blank_rectangle_in_pixbuf (pixbuf, &dest);
    }

  g_free (rects);
}

/* When there are multiple monitors with different resolutions, the visible area
 * within the root window may not be rectangular (it may have an L-shape, for
 * example).  In that case, mask out the areas of the root window which would
 * not be visible in the monitors, so that screenshot do not end up with content
 * that the user won't ever see.
 */
static void
mask_monitors (GdkPixbuf *pixbuf, GdkWindow *root_window)
{
  GdkScreen *screen;
  GdkRegion *region_with_monitors;
  GdkRegion *invisible_region;
  GdkRectangle rect;

  screen = gdk_drawable_get_screen (GDK_DRAWABLE (root_window));

  region_with_monitors = make_region_with_monitors (screen);

  rect.x = 0;
  rect.y = 0;
  rect.width = gdk_screen_get_width (screen);
  rect.height = gdk_screen_get_height (screen);

  invisible_region = gdk_region_rectangle (&rect);
  gdk_region_subtract (invisible_region, region_with_monitors);

  blank_region_in_pixbuf (pixbuf, invisible_region);

  gdk_region_destroy (region_with_monitors);
  gdk_region_destroy (invisible_region);
}

GdkPixbuf *
screenshot_get_pixbuf (GdkWindow    *window,
                       GdkRectangle *rectangle,
                       gboolean      include_pointer,
                       gboolean      include_border)
{
  GdkWindow *root;
  GdkPixbuf *screenshot;
  gint x_real_orig, y_real_orig, x_orig, y_orig;
  gint width, real_width, height, real_height;

  /* If the screenshot should include the border, we look for the WM window. */

  if (include_border)
    {
      Window xid, wm;

      xid = GDK_WINDOW_XWINDOW (window);
      wm = find_wm_window (xid);

      if (wm != None)
        window = gdk_window_foreign_new (wm);

      /* fallback to no border if we can't find the WM window. */
    }

  root = gdk_get_default_root_window ();

  gdk_drawable_get_size (window, &real_width, &real_height);      
  gdk_window_get_origin (window, &x_real_orig, &y_real_orig);

  x_orig = x_real_orig;
  y_orig = y_real_orig;
  width  = real_width;
  height = real_height;

  if (x_orig < 0)
    {
      width = width + x_orig;
      x_orig = 0;
    }

  if (y_orig < 0)
    {
      height = height + y_orig;
      y_orig = 0;
    }

  if (x_orig + width > gdk_screen_width ())
    width = gdk_screen_width () - x_orig;

  if (y_orig + height > gdk_screen_height ())
    height = gdk_screen_height () - y_orig;

  if (rectangle)
    {
      x_orig = rectangle->x - x_orig;
      y_orig = rectangle->y - y_orig;
      width  = rectangle->width;
      height = rectangle->height;
    }
  
  screenshot = gdk_pixbuf_get_from_drawable (NULL, root, NULL,
                                             x_orig, y_orig, 0, 0,
                                             width, height);

  mask_monitors (screenshot, root);

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
  if (include_border)
    {
      XRectangle *rectangles;
      GdkPixbuf *tmp;
      int rectangle_count, rectangle_order, i;

      /* we must use XShape to avoid showing what's under the rounder corners
       * of the WM decoration.
       */

      rectangles = XShapeGetRectangles (GDK_DISPLAY (),
                                        GDK_WINDOW_XWINDOW (window),
                                        ShapeBounding,
                                        &rectangle_count,
                                        &rectangle_order);
      if (rectangles && rectangle_count > 0 && window != root)
        {
          gboolean has_alpha = gdk_pixbuf_get_has_alpha (screenshot);
          
          tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
          gdk_pixbuf_fill (tmp, 0);
          
          for (i = 0; i < rectangle_count; i++)
            {
              gint rec_x, rec_y;
              gint rec_width, rec_height;
              gint y;

              rec_x = rectangles[i].x;
              rec_y = rectangles[i].y;
              rec_width = rectangles[i].width;
              rec_height = rectangles[i].height;

              if (x_real_orig < 0)
                {
                  rec_x += x_real_orig;
                  rec_x = MAX(rec_x, 0);
                  rec_width += x_real_orig;
                }

              if (y_real_orig < 0)
                {
                  rec_y += y_real_orig;
                  rec_y = MAX(rec_y, 0);
                  rec_height += y_real_orig;
                }

              if (x_orig + rec_x + rec_width > gdk_screen_width ())
                rec_width = gdk_screen_width () - x_orig - rec_x;

              if (y_orig + rec_y + rec_height > gdk_screen_height ())
                rec_height = gdk_screen_height () - y_orig - rec_y;

              for (y = rec_y; y < rec_y + rec_height; y++)
                {
                  guchar *src_pixels, *dest_pixels;
                  gint x;

                  src_pixels = gdk_pixbuf_get_pixels (screenshot)
                             + y * gdk_pixbuf_get_rowstride(screenshot)
                             + rec_x * (has_alpha ? 4 : 3);
                  dest_pixels = gdk_pixbuf_get_pixels (tmp)
                              + y * gdk_pixbuf_get_rowstride (tmp)
                              + rec_x * 4;

                  for (x = 0; x < rec_width; x++)
                    {
                      *dest_pixels++ = *src_pixels++;
                      *dest_pixels++ = *src_pixels++;
                      *dest_pixels++ = *src_pixels++;

                      if (has_alpha)
                        *dest_pixels++ = *src_pixels++;
                      else
                        *dest_pixels++ = 255;
                    }
                }
            }

          g_object_unref (screenshot);
          screenshot = tmp;
        }
    }
#endif /* HAVE_X11_EXTENSIONS_SHAPE_H */

  /* if we have a selected area, there were by definition no cursor in the
   * screenshot */
  if (include_pointer && !rectangle) 
    {
      GdkCursor *cursor;
      GdkPixbuf *cursor_pixbuf;

      cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_LEFT_PTR);
      cursor_pixbuf = gdk_cursor_get_image (cursor);

      if (cursor_pixbuf != NULL) 
        {
          GdkRectangle r1, r2;
          gint cx, cy, xhot, yhot;

          gdk_window_get_pointer (window, &cx, &cy, NULL);
          sscanf (gdk_pixbuf_get_option (cursor_pixbuf, "x_hot"), "%d", &xhot);
          sscanf (gdk_pixbuf_get_option (cursor_pixbuf, "y_hot"), "%d", &yhot);

          /* in r1 we have the window coordinates */
          r1.x = x_real_orig;
          r1.y = y_real_orig;
          r1.width = real_width;
          r1.height = real_height;

          /* in r2 we have the cursor window coordinates */
          r2.x = cx + x_real_orig;
          r2.y = cy + y_real_orig;
          r2.width = gdk_pixbuf_get_width (cursor_pixbuf);
          r2.height = gdk_pixbuf_get_height (cursor_pixbuf);

          /* see if the pointer is inside the window */
          if (gdk_rectangle_intersect (&r1, &r2, &r2)) 
            {
              gdk_pixbuf_composite (cursor_pixbuf, screenshot,
                                    cx - xhot, cy - yhot,
                                    r2.width, r2.height,
                                    cx - xhot, cy - yhot,
                                    1.0, 1.0, 
                                    GDK_INTERP_BILINEAR,
                                    255);
            }

          g_object_unref (cursor_pixbuf);
          gdk_cursor_unref (cursor);
        }
    }

  return screenshot;
}

gchar *
screenshot_get_window_title (GdkWindow *win)
{
  gchar *name;

  win = gdk_window_get_toplevel (win);
  win = look_for_hint (win, gdk_x11_xatom_to_atom (gdk_x11_get_xatom_by_name ("WM_STATE")));

  name = get_utf8_property (win, gdk_x11_xatom_to_atom (gdk_x11_get_xatom_by_name ("_NET_WM_NAME")));
  if (name)
    return name;

  /* TODO: maybe we should also look at WM_NAME and WM_CLASS? */

  return g_strdup (_("Untitled Window"));
}

void
screenshot_show_error_dialog (GtkWindow   *parent,
                              const gchar *message,
                              const gchar *detail)
{
  GtkWidget *dialog;
  
  g_return_if_fail ((parent == NULL) || (GTK_IS_WINDOW (parent)));
  g_return_if_fail (message != NULL);
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent && parent->group)
    gtk_window_group_add_window (parent->group, GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

void
screenshot_show_gerror_dialog (GtkWindow   *parent,
                               const gchar *message,
                               GError      *error)
{
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
  g_return_if_fail (message != NULL);
  g_return_if_fail (error != NULL);

  screenshot_show_error_dialog (parent, message, error->message);
}
