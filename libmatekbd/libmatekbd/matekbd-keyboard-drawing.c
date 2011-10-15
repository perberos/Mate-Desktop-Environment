/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <glib/gi18n.h>
#include <libxklavier/xklavier.h>

#include <matekbd-keyboard-drawing.h>
#include <matekbd-keyboard-drawing-marshal.h>
#include <matekbd-util.h>

#define noKBDRAW_DEBUG

#define INVALID_KEYCODE ((guint)(-1))

#define GTK_RESPONSE_PRINT 2

enum {
	BAD_KEYCODE = 0,
	NUM_SIGNALS
};

static guint matekbd_keyboard_drawing_signals[NUM_SIGNALS] = { 0 };

static void matekbd_keyboard_drawing_set_mods (MatekbdKeyboardDrawing * drawing,
					    guint mods);

static gint
xkb_to_pixmap_coord (MatekbdKeyboardDrawingRenderContext * context, gint n)
{
	return n * context->scale_numerator / context->scale_denominator;
}

static gdouble
xkb_to_pixmap_double (MatekbdKeyboardDrawingRenderContext * context,
		      gdouble d)
{
	return d * context->scale_numerator / context->scale_denominator;
}


/* angle is in tenths of a degree; coordinates can be anything as (xkb,
 * pixels, pango) as long as they are all the same */
static void
rotate_coordinate (gint origin_x,
		   gint origin_y,
		   gint x,
		   gint y, gint angle, gint * rotated_x, gint * rotated_y)
{
	*rotated_x =
	    origin_x + (x - origin_x) * cos (M_PI * angle / 1800.0) - (y -
								       origin_y)
	    * sin (M_PI * angle / 1800.0);
	*rotated_y =
	    origin_y + (x - origin_x) * sin (M_PI * angle / 1800.0) + (y -
								       origin_y)
	    * cos (M_PI * angle / 1800.0);
}

static gdouble
length (gdouble x, gdouble y)
{
	return sqrt (x * x + y * y);
}

static gdouble
point_line_distance (gdouble ax, gdouble ay, gdouble nx, gdouble ny)
{
	return ax * nx + ay * ny;
}

static void
normal_form (gdouble ax, gdouble ay,
	     gdouble bx, gdouble by,
	     gdouble * nx, gdouble * ny, gdouble * d)
{
	gdouble l;

	*nx = by - ay;
	*ny = ax - bx;

	l = length (*nx, *ny);

	*nx /= l;
	*ny /= l;

	*d = point_line_distance (ax, ay, *nx, *ny);
}

static void
inverse (gdouble a, gdouble b, gdouble c, gdouble d,
	 gdouble * e, gdouble * f, gdouble * g, gdouble * h)
{
	gdouble det;

	det = a * d - b * c;

	*e = d / det;
	*f = -b / det;
	*g = -c / det;
	*h = a / det;
}

static void
multiply (gdouble a, gdouble b, gdouble c, gdouble d,
	  gdouble e, gdouble f, gdouble * x, gdouble * y)
{
	*x = a * e + b * f;
	*y = c * e + d * f;
}

static void
intersect (gdouble n1x, gdouble n1y, gdouble d1,
	   gdouble n2x, gdouble n2y, gdouble d2, gdouble * x, gdouble * y)
{
	gdouble e, f, g, h;

	inverse (n1x, n1y, n2x, n2y, &e, &f, &g, &h);
	multiply (e, f, g, h, d1, d2, x, y);
}


/* draw an angle from the current point to b and then to c,
 * with a rounded corner of the given radius.
 */
static void
rounded_corner (cairo_t * cr,
		gdouble bx, gdouble by,
		gdouble cx, gdouble cy, gdouble radius)
{
	gdouble ax, ay;
	gdouble n1x, n1y, d1;
	gdouble n2x, n2y, d2;
	gdouble pd1, pd2;
	gdouble ix, iy;
	gdouble dist1, dist2;
	gdouble nx, ny, d;
	gdouble a1x, a1y, c1x, c1y;
	gdouble phi1, phi2;

	cairo_get_current_point (cr, &ax, &ay);
#ifdef KBDRAW_DEBUG
	printf ("        current point: (%f, %f), radius %f:\n", ax, ay,
		radius);
#endif

	/* make sure radius is not too large */
	dist1 = length (bx - ax, by - ay);
	dist2 = length (cx - bx, cy - by);

	radius = MIN (radius, MIN (dist1, dist2));

	/* construct normal forms of the lines */
	normal_form (ax, ay, bx, by, &n1x, &n1y, &d1);
	normal_form (bx, by, cx, cy, &n2x, &n2y, &d2);

	/* find which side of the line a,b the point c is on */
	if (point_line_distance (cx, cy, n1x, n1y) < d1)
		pd1 = d1 - radius;
	else
		pd1 = d1 + radius;

	/* find which side of the line b,c the point a is on */
	if (point_line_distance (ax, ay, n2x, n2y) < d2)
		pd2 = d2 - radius;
	else
		pd2 = d2 + radius;

	/* intersect the parallels to find the center of the arc */
	intersect (n1x, n1y, pd1, n2x, n2y, pd2, &ix, &iy);

	nx = (bx - ax) / dist1;
	ny = (by - ay) / dist1;
	d = point_line_distance (ix, iy, nx, ny);

	/* a1 is the point on the line a-b where the arc starts */
	intersect (n1x, n1y, d1, nx, ny, d, &a1x, &a1y);

	nx = (cx - bx) / dist2;
	ny = (cy - by) / dist2;
	d = point_line_distance (ix, iy, nx, ny);

	/* c1 is the point on the line b-c where the arc ends */
	intersect (n2x, n2y, d2, nx, ny, d, &c1x, &c1y);

	/* determine the first angle */
	if (a1x - ix == 0)
		phi1 = (a1y - iy > 0) ? M_PI_2 : 3 * M_PI_2;
	else if (a1x - ix > 0)
		phi1 = atan ((a1y - iy) / (a1x - ix));
	else
		phi1 = M_PI + atan ((a1y - iy) / (a1x - ix));

	/* determine the second angle */
	if (c1x - ix == 0)
		phi2 = (c1y - iy > 0) ? M_PI_2 : 3 * M_PI_2;
	else if (c1x - ix > 0)
		phi2 = atan ((c1y - iy) / (c1x - ix));
	else
		phi2 = M_PI + atan ((c1y - iy) / (c1x - ix));

	/* compute the difference between phi2 and phi1 mod 2pi */
	d = phi2 - phi1;
	while (d < 0)
		d += 2 * M_PI;
	while (d > 2 * M_PI)
		d -= 2 * M_PI;

#ifdef KBDRAW_DEBUG
	printf ("        line 1 to: (%f, %f):\n", a1x, a1y);
#endif
	if (!(isnan (a1x) || isnan (a1y)))
		cairo_line_to (cr, a1x, a1y);

	/* pick the short arc from phi1 to phi2 */
	if (d < M_PI)
		cairo_arc (cr, ix, iy, radius, phi1, phi2);
	else
		cairo_arc_negative (cr, ix, iy, radius, phi1, phi2);

#ifdef KBDRAW_DEBUG
	printf ("        line 2 to: (%f, %f):\n", cx, cy);
#endif
	cairo_line_to (cr, cx, cy);
}

static void
rounded_polygon (cairo_t * cr,
		 gboolean filled,
		 gdouble radius, GdkPoint * points, gint num_points)
{
	gint i, j;

	cairo_move_to (cr,
		       (gdouble) (points[num_points - 1].x +
				  points[0].x) / 2,
		       (gdouble) (points[num_points - 1].y +
				  points[0].y) / 2);


#ifdef KBDRAW_DEBUG
	printf ("    rounded polygon of radius %f:\n", radius);
#endif
	for (i = 0; i < num_points; i++) {
		j = (i + 1) % num_points;
		rounded_corner (cr, (gdouble) points[i].x,
				(gdouble) points[i].y,
				(gdouble) (points[i].x + points[j].x) / 2,
				(gdouble) (points[i].y + points[j].y) / 2,
				radius);
#ifdef KBDRAW_DEBUG
		printf ("      corner (%d, %d) -> (%d, %d):\n",
			points[i].x, points[i].y, points[j].x,
			points[j].y);
#endif
	};
	cairo_close_path (cr);

	if (filled)
		cairo_fill (cr);
	else
		cairo_stroke (cr);
}

static void
draw_polygon (MatekbdKeyboardDrawingRenderContext * context,
	      GdkColor * fill_color,
	      gint xkb_x,
	      gint xkb_y, XkbPointRec * xkb_points, guint num_points,
	      gdouble radius)
{
	GdkPoint *points;
	gboolean filled;
	gint i;

	if (fill_color) {
		filled = TRUE;
	} else {
		fill_color = context->dark_color;
		filled = FALSE;
	}

	gdk_cairo_set_source_color (context->cr, fill_color);

	points = g_new (GdkPoint, num_points);

#ifdef KBDRAW_DEBUG
	printf ("    Polygon points:\n");
#endif
	for (i = 0; i < num_points; i++) {
		points[i].x =
		    xkb_to_pixmap_coord (context, xkb_x + xkb_points[i].x);
		points[i].y =
		    xkb_to_pixmap_coord (context, xkb_y + xkb_points[i].y);
#ifdef KBDRAW_DEBUG
		printf ("      %d, %d\n", points[i].x, points[i].y);
#endif
	}

	rounded_polygon (context->cr, filled,
			 xkb_to_pixmap_double (context, radius),
			 points, num_points);

	g_free (points);
}

static void
curve_rectangle (cairo_t * cr,
		 gdouble x0,
		 gdouble y0, gdouble width, gdouble height, gdouble radius)
{
	gdouble x1, y1;

	if (!width || !height)
		return;

	x1 = x0 + width;
	y1 = y0 + height;

	radius = MIN (radius, MIN (width / 2, height / 2));

	cairo_move_to (cr, x0, y0 + radius);
	cairo_arc (cr, x0 + radius, y0 + radius, radius, M_PI,
		   3 * M_PI / 2);
	cairo_line_to (cr, x1 - radius, y0);
	cairo_arc (cr, x1 - radius, y0 + radius, radius, 3 * M_PI / 2,
		   2 * M_PI);
	cairo_line_to (cr, x1, y1 - radius);
	cairo_arc (cr, x1 - radius, y1 - radius, radius, 0, M_PI / 2);
	cairo_line_to (cr, x0 + radius, y1);
	cairo_arc (cr, x0 + radius, y1 - radius, radius, M_PI / 2, M_PI);

	cairo_close_path (cr);
}

static void
draw_curve_rectangle (cairo_t * cr,
		      gboolean filled,
		      GdkColor * fill_color,
		      gint x, gint y, gint width, gint height, gint radius)
{
	curve_rectangle (cr, x, y, width, height, radius);

	gdk_cairo_set_source_color (cr, fill_color);

	if (filled)
		cairo_fill (cr);
	else
		cairo_stroke (cr);
}

/* x, y, width, height are in the xkb coordinate system */
static void
draw_rectangle (MatekbdKeyboardDrawingRenderContext * context,
		GdkColor * fill_color,
		gint angle,
		gint xkb_x, gint xkb_y, gint xkb_width, gint xkb_height,
		gint radius)
{
	if (angle == 0) {
		gint x, y, width, height;
		gboolean filled;

		if (fill_color) {
			filled = TRUE;
		} else {
			fill_color = context->dark_color;
			filled = FALSE;
		}

		x = xkb_to_pixmap_coord (context, xkb_x);
		y = xkb_to_pixmap_coord (context, xkb_y);
		width =
		    xkb_to_pixmap_coord (context, xkb_x + xkb_width) - x;
		height =
		    xkb_to_pixmap_coord (context, xkb_y + xkb_height) - y;

		draw_curve_rectangle (context->cr, filled, fill_color,
				      x, y, width, height,
				      xkb_to_pixmap_double (context,
							    radius));
	} else {
		XkbPointRec points[4];
		gint x, y;

		points[0].x = xkb_x;
		points[0].y = xkb_y;
		rotate_coordinate (xkb_x, xkb_y, xkb_x + xkb_width, xkb_y,
				   angle, &x, &y);
		points[1].x = x;
		points[1].y = y;
		rotate_coordinate (xkb_x, xkb_y, xkb_x + xkb_width,
				   xkb_y + xkb_height, angle, &x, &y);
		points[2].x = x;
		points[2].y = y;
		rotate_coordinate (xkb_x, xkb_y, xkb_x, xkb_y + xkb_height,
				   angle, &x, &y);
		points[3].x = x;
		points[3].y = y;

		/* the points we've calculated are relative to 0,0 */
		draw_polygon (context, fill_color, 0, 0, points, 4,
			      radius);
	}
}

static void
draw_outline (MatekbdKeyboardDrawingRenderContext * context,
	      XkbOutlineRec * outline,
	      GdkColor * color, gint angle, gint origin_x, gint origin_y)
{
#ifdef KBDRAW_DEBUG
	printf (" num_points in %p: %d\n", outline, outline->num_points);
#endif

	if (outline->num_points == 1) {
		if (color)
			draw_rectangle (context, color, angle, origin_x,
					origin_y, outline->points[0].x,
					outline->points[0].y,
					outline->corner_radius);

#ifdef KBDRAW_DEBUG
		printf ("pointsxy:%d %d %d\n", outline->points[0].x,
			outline->points[0].y, outline->corner_radius);
#endif

		draw_rectangle (context, NULL, angle, origin_x, origin_y,
				outline->points[0].x,
				outline->points[0].y,
				outline->corner_radius);
	} else if (outline->num_points == 2) {
		gint rotated_x0, rotated_y0;

		rotate_coordinate (origin_x, origin_y,
				   origin_x + outline->points[0].x,
				   origin_y + outline->points[0].y,
				   angle, &rotated_x0, &rotated_y0);
		if (color)
			draw_rectangle (context, color, angle, rotated_x0,
					rotated_y0, outline->points[1].x,
					outline->points[1].y,
					outline->corner_radius);

		draw_rectangle (context, NULL, angle, rotated_x0,
				rotated_y0, outline->points[1].x,
				outline->points[1].y,
				outline->corner_radius);
	} else {
		if (color)
			draw_polygon (context, color, origin_x, origin_y,
				      outline->points,
				      outline->num_points,
				      outline->corner_radius);

		draw_polygon (context, NULL, origin_x, origin_y,
			      outline->points, outline->num_points,
			      outline->corner_radius);
	}
}

/* see PSColorDef in xkbprint */
static gboolean
parse_xkb_color_spec (gchar * colorspec, GdkColor * color)
{
	glong level;

	if (g_ascii_strcasecmp (colorspec, "black") == 0) {
		color->red = 0;
		color->green = 0;
		color->blue = 0;
	} else if (g_ascii_strcasecmp (colorspec, "white") == 0) {
		color->red = 65535;
		color->green = 65535;
		color->blue = 65535;
	} else if (g_ascii_strncasecmp (colorspec, "grey", 4) == 0 ||
		   g_ascii_strncasecmp (colorspec, "gray", 4) == 0) {
		level = strtol (colorspec + 4, NULL, 10);

		color->red = 65535 - 65535 * level / 100;
		color->green = 65535 - 65535 * level / 100;
		color->blue = 65535 - 65535 * level / 100;
	} else if (g_ascii_strcasecmp (colorspec, "red") == 0) {
		color->red = 65535;
		color->green = 0;
		color->blue = 0;
	} else if (g_ascii_strcasecmp (colorspec, "green") == 0) {
		color->red = 0;
		color->green = 65535;
		color->blue = 0;
	} else if (g_ascii_strcasecmp (colorspec, "blue") == 0) {
		color->red = 0;
		color->green = 0;
		color->blue = 65535;
	} else if (g_ascii_strncasecmp (colorspec, "red", 3) == 0) {
		level = strtol (colorspec + 3, NULL, 10);

		color->red = 65535 * level / 100;
		color->green = 0;
		color->blue = 0;
	} else if (g_ascii_strncasecmp (colorspec, "green", 5) == 0) {
		level = strtol (colorspec + 5, NULL, 10);

		color->red = 0;
		color->green = 65535 * level / 100;;
		color->blue = 0;
	} else if (g_ascii_strncasecmp (colorspec, "blue", 4) == 0) {
		level = strtol (colorspec + 4, NULL, 10);

		color->red = 0;
		color->green = 0;
		color->blue = 65535 * level / 100;
	} else
		return FALSE;

	return TRUE;
}


static guint
find_keycode (MatekbdKeyboardDrawing * drawing, gchar * key_name)
{
#define KEYSYM_NAME_MAX_LENGTH 4
	guint keycode;
	gint i, j;
	XkbKeyNamePtr pkey;
	XkbKeyAliasPtr palias;
	guint is_name_matched;
	gchar *src, *dst;

	if (!drawing->xkb)
		return INVALID_KEYCODE;

#ifdef KBDRAW_DEBUG
	printf ("    looking for keycode for (%c%c%c%c)\n",
		key_name[0], key_name[1], key_name[2], key_name[3]);
#endif

	pkey = drawing->xkb->names->keys + drawing->xkb->min_key_code;
	for (keycode = drawing->xkb->min_key_code;
	     keycode <= drawing->xkb->max_key_code; keycode++) {
		is_name_matched = 1;
		src = key_name;
		dst = pkey->name;
		for (i = KEYSYM_NAME_MAX_LENGTH; --i >= 0;) {
			if ('\0' == *src)
				break;
			if (*src++ != *dst++) {
				is_name_matched = 0;
				break;
			}
		}
		if (is_name_matched) {
#ifdef KBDRAW_DEBUG
			printf ("      found keycode %u\n", keycode);
#endif
			return keycode;
		}
		pkey++;
	}

	palias = drawing->xkb->names->key_aliases;
	for (j = drawing->xkb->names->num_key_aliases; --j >= 0;) {
		is_name_matched = 1;
		src = key_name;
		dst = palias->alias;
		for (i = KEYSYM_NAME_MAX_LENGTH; --i >= 0;) {
			if ('\0' == *src)
				break;
			if (*src++ != *dst++) {
				is_name_matched = 0;
				break;
			}
		}

		if (is_name_matched) {
			keycode = find_keycode (drawing, palias->real);
#ifdef KBDRAW_DEBUG
			printf ("found alias keycode %u\n", keycode);
#endif
			return keycode;
		}
		palias++;
	}

	return INVALID_KEYCODE;
}


static void
set_key_label_in_layout (MatekbdKeyboardDrawingRenderContext * context,
			 guint keyval)
{
	gchar buf[5];
	gunichar uc;
	PangoLayout *layout = context->layout;

	switch (keyval) {
	case GDK_Scroll_Lock:
		pango_layout_set_text (layout, "Scroll\nLock", -1);
		break;

	case GDK_space:
		pango_layout_set_text (layout, "", -1);
		break;

	case GDK_Sys_Req:
		pango_layout_set_text (layout, "Sys Rq", -1);
		break;

	case GDK_Page_Up:
		pango_layout_set_text (layout, "Page\nUp", -1);
		break;

	case GDK_Page_Down:
		pango_layout_set_text (layout, "Page\nDown", -1);
		break;

	case GDK_Num_Lock:
		pango_layout_set_text (layout, "Num\nLock", -1);
		break;

	case GDK_KP_Page_Up:
		pango_layout_set_text (layout, "Pg Up", -1);
		break;

	case GDK_KP_Page_Down:
		pango_layout_set_text (layout, "Pg Dn", -1);
		break;

	case GDK_KP_Home:
		pango_layout_set_text (layout, "Home", -1);
		break;

	case GDK_KP_Left:
		pango_layout_set_text (layout, "Left", -1);
		break;

	case GDK_KP_End:
		pango_layout_set_text (layout, "End", -1);
		break;

	case GDK_KP_Up:
		pango_layout_set_text (layout, "Up", -1);
		break;

	case GDK_KP_Begin:
		pango_layout_set_text (layout, "Begin", -1);
		break;

	case GDK_KP_Right:
		pango_layout_set_text (layout, "Right", -1);
		break;

	case GDK_KP_Enter:
		pango_layout_set_text (layout, "Enter", -1);
		break;

	case GDK_KP_Down:
		pango_layout_set_text (layout, "Down", -1);
		break;

	case GDK_KP_Insert:
		pango_layout_set_text (layout, "Ins", -1);
		break;

	case GDK_KP_Delete:
		pango_layout_set_text (layout, "Del", -1);
		break;

	case GDK_dead_grave:
		pango_layout_set_text (layout, "ˋ", -1);
		break;

	case GDK_dead_acute:
		pango_layout_set_text (layout, "ˊ", -1);
		break;

	case GDK_dead_circumflex:
		pango_layout_set_text (layout, "ˆ", -1);
		break;

	case GDK_dead_tilde:
		pango_layout_set_text (layout, "~", -1);
		break;

	case GDK_dead_macron:
		pango_layout_set_text (layout, "ˉ", -1);
		break;

	case GDK_dead_breve:
		pango_layout_set_text (layout, "˘", -1);
		break;

	case GDK_dead_abovedot:
		pango_layout_set_text (layout, "˙", -1);
		break;

	case GDK_dead_diaeresis:
		pango_layout_set_text (layout, "¨", -1);
		break;

	case GDK_dead_abovering:
		pango_layout_set_text (layout, "˚", -1);
		break;

	case GDK_dead_doubleacute:
		pango_layout_set_text (layout, "˝", -1);
		break;

	case GDK_dead_caron:
		pango_layout_set_text (layout, "ˇ", -1);
		break;

	case GDK_dead_cedilla:
		pango_layout_set_text (layout, "¸", -1);
		break;

	case GDK_dead_ogonek:
		pango_layout_set_text (layout, "˛", -1);
		break;

		/* case GDK_dead_iota:
		 * case GDK_dead_voiced_sound:
		 * case GDK_dead_semivoiced_sound: */

	case GDK_dead_belowdot:
		pango_layout_set_text (layout, " ̣", -1);
		break;

	case GDK_horizconnector:
		pango_layout_set_text (layout, "horiz\nconn", -1);
		break;

	case GDK_Mode_switch:
		pango_layout_set_text (layout, "AltGr", -1);
		break;

	case GDK_Multi_key:
		pango_layout_set_text (layout, "Compose", -1);
		break;

	default:
		uc = gdk_keyval_to_unicode (keyval);
		if (uc != 0 && g_unichar_isgraph (uc)) {
			buf[g_unichar_to_utf8 (uc, buf)] = '\0';
			pango_layout_set_text (layout, buf, -1);
		} else {
			gchar *name = gdk_keyval_name (keyval);
			if (name)
				pango_layout_set_text (layout, name, -1);
			else
				pango_layout_set_text (layout, "", -1);
		}
	}
}


static void
draw_pango_layout (MatekbdKeyboardDrawingRenderContext * context,
		   MatekbdKeyboardDrawing * drawing,
		   gint angle, gint x, gint y)
{
	PangoLayout *layout = context->layout;
	GdkColor *color;
	PangoLayoutLine *line;
	gint x_off, y_off;
	gint i;

	color =
	    drawing->colors + (drawing->xkb->geom->label_color -
			       drawing->xkb->geom->colors);

	if (angle != context->angle) {
		PangoMatrix matrix = PANGO_MATRIX_INIT;
		pango_matrix_rotate (&matrix, -angle / 10.0);
		pango_context_set_matrix (pango_layout_get_context
					  (layout), &matrix);
		pango_layout_context_changed (layout);
		context->angle = angle;
	}

	i = 0;
	y_off = 0;
	for (line = pango_layout_get_line (layout, i);
	     line != NULL; line = pango_layout_get_line (layout, ++i)) {
		GSList *runp;
		PangoRectangle line_extents;

		x_off = 0;

		for (runp = line->runs; runp != NULL; runp = runp->next) {
			PangoGlyphItem *run = runp->data;
			gint j;

			for (j = 0; j < run->glyphs->num_glyphs; j++) {
				PangoGlyphGeometry *geometry;

				geometry =
				    &run->glyphs->glyphs[j].geometry;

				x_off += geometry->width;
			}
		}

		pango_layout_line_get_extents (line, NULL, &line_extents);
		y_off +=
		    line_extents.height +
		    pango_layout_get_spacing (layout);
	}

	cairo_move_to (context->cr, x, y);
	gdk_cairo_set_source_color (context->cr, color);
	pango_cairo_show_layout (context->cr, layout);
}

static void
draw_key_label_helper (MatekbdKeyboardDrawingRenderContext * context,
		       MatekbdKeyboardDrawing * drawing,
		       KeySym keysym,
		       gint angle,
		       MatekbdKeyboardDrawingGroupLevelPosition glp,
		       gint x,
		       gint y, gint width, gint height, gint padding)
{
	gint label_x, label_y, label_max_width, ycell;

	if (keysym == 0)
		return;
#ifdef KBDRAW_DEBUG
	printf ("keysym: %04X(%c) at glp: %d\n",
		(unsigned) keysym, (char) keysym, (int) glp);
#endif

	switch (glp) {
	case MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT:
	case MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT:
		{
			ycell =
			    glp == MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT;

			rotate_coordinate (x, y, x + padding,
					   y + padding + (height -
							  2 * padding) *
					   ycell * 4 / 7, angle, &label_x,
					   &label_y);
			label_max_width =
			    PANGO_SCALE * (width - 2 * padding);
			break;
		}
	case MATEKBD_KEYBOARD_DRAWING_POS_TOPRIGHT:
	case MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT:
		{
			ycell =
			    glp == MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT;

			rotate_coordinate (x, y,
					   x + padding + (width -
							  2 * padding) *
					   4 / 7,
					   y + padding + (height -
							  2 * padding) *
					   ycell * 4 / 7, angle, &label_x,
					   &label_y);
			label_max_width =
			    PANGO_SCALE * ((width - 2 * padding) -
					   (width - 2 * padding) * 4 / 7);
			break;
		}
	default:
		return;
	}
	set_key_label_in_layout (context, keysym);
	pango_layout_set_width (context->layout, label_max_width);
	label_y -= (pango_layout_get_line_count (context->layout) - 1) *
	    (pango_font_description_get_size (context->font_desc) /
	     PANGO_SCALE);
	cairo_save (context->cr);
	cairo_rectangle (context->cr, x + padding / 2, y + padding / 2,
			 width - padding, height - padding);
	cairo_clip (context->cr);
	draw_pango_layout (context, drawing, angle, label_x, label_y);
	cairo_restore (context->cr);
}

static void
draw_key_label (MatekbdKeyboardDrawingRenderContext * context,
		MatekbdKeyboardDrawing * drawing,
		guint keycode,
		gint angle,
		gint xkb_origin_x,
		gint xkb_origin_y, gint xkb_width, gint xkb_height)
{
	gint x, y, width, height;
	gint padding;
	gint g, l, glp;

	if (!drawing->xkb)
		return;

	padding = 23 * context->scale_numerator / context->scale_denominator;	/* 2.3mm */

	x = xkb_to_pixmap_coord (context, xkb_origin_x);
	y = xkb_to_pixmap_coord (context, xkb_origin_y);
	width =
	    xkb_to_pixmap_coord (context, xkb_origin_x + xkb_width) - x;
	height =
	    xkb_to_pixmap_coord (context, xkb_origin_y + xkb_height) - y;

	for (glp = MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT;
	     glp < MATEKBD_KEYBOARD_DRAWING_POS_TOTAL; glp++) {
		if (drawing->groupLevels[glp] == NULL)
			continue;
		g = drawing->groupLevels[glp]->group;
		l = drawing->groupLevels[glp]->level;

		if (g < 0 || g >= XkbKeyNumGroups (drawing->xkb, keycode))
			continue;
		if (l < 0
		    || l >= XkbKeyGroupWidth (drawing->xkb, keycode, g))
			continue;

		/* Skip "exotic" levels like the "Ctrl" level in PC_SYSREQ */
		if (l > 0) {
			guint mods = XkbKeyKeyType (drawing->xkb, keycode,
						    g)->mods.mask;
			if ((mods & (ShiftMask | drawing->l3mod)) == 0)
				continue;
		}

		if (drawing->track_modifiers) {
			guint mods_rtrn;
			KeySym keysym;

			if (XkbTranslateKeyCode (drawing->xkb, keycode,
						 XkbBuildCoreState
						 (drawing->mods, g),
						 &mods_rtrn, &keysym)) {
				draw_key_label_helper (context, drawing,
						       keysym, angle, glp,
						       x, y, width, height,
						       padding);
				/* reverse y order */
			}
		} else {
			KeySym keysym;

			keysym =
			    XkbKeySymEntry (drawing->xkb, keycode, l, g);

			draw_key_label_helper (context, drawing, keysym,
					       angle, glp, x, y, width,
					       height, padding);
			/* reverse y order */
		}
	}
}

/* groups are from 0-3 */
static void
draw_key (MatekbdKeyboardDrawingRenderContext * context,
	  MatekbdKeyboardDrawing * drawing, MatekbdKeyboardDrawingKey * key)
{
	XkbShapeRec *shape;
	GdkColor *color;
	/* gint i; */

	if (!drawing->xkb)
		return;

#ifdef KBDRAW_DEBUG
	printf ("shape: %p (base %p, index %d)\n",
		drawing->xkb->geom->shapes + key->xkbkey->shape_ndx,
		drawing->xkb->geom->shapes, key->xkbkey->shape_ndx);
#endif

	shape = drawing->xkb->geom->shapes + key->xkbkey->shape_ndx;

	if (key->pressed)
		color =
		    &gtk_widget_get_style (GTK_WIDGET (drawing))->base
		    [GTK_STATE_SELECTED];
	else
		color = drawing->colors + key->xkbkey->color_ndx;

#ifdef KBDRAW_DEBUG
	printf
	    (" outlines base in the shape: %p (total: %d), origin: (%d, %d), angle %d, colored: %s\n",
	     shape->outlines, shape->num_outlines, key->origin_x,
	     key->origin_y, key->angle, color ? "yes" : "no");
#endif

	/* draw the primary outline */
	draw_outline (context,
		      shape->primary ? shape->primary : shape->outlines,
		      color, key->angle, key->origin_x, key->origin_y);
#if 0
	/* don't draw other outlines for now, since
	 * the text placement does not take them into account
	 */
	for (i = 0; i < shape->num_outlines; i++) {
		if (shape->outlines + i == shape->approx ||
		    shape->outlines + i == shape->primary)
			continue;
		draw_outline (context, shape->outlines + i, NULL,
			      key->angle, key->origin_x, key->origin_y);
	}
#endif

	draw_key_label (context, drawing, key->keycode, key->angle,
			key->origin_x, key->origin_y,
			shape->bounds.x2, shape->bounds.y2);
}

static void
invalidate_region (MatekbdKeyboardDrawing * drawing,
		   gdouble angle,
		   gint origin_x, gint origin_y, XkbShapeRec * shape)
{
	GdkPoint points[4];
	gint x_min, x_max, y_min, y_max;
	gint x, y, width, height;
	gint xx, yy;

	rotate_coordinate (0, 0, 0, 0, angle, &xx, &yy);
	points[0].x = xx;
	points[0].y = yy;
	rotate_coordinate (0, 0, shape->bounds.x2, 0, angle, &xx, &yy);
	points[1].x = xx;
	points[1].y = yy;
	rotate_coordinate (0, 0, shape->bounds.x2, shape->bounds.y2, angle,
			   &xx, &yy);
	points[2].x = xx;
	points[2].y = yy;
	rotate_coordinate (0, 0, 0, shape->bounds.y2, angle, &xx, &yy);
	points[3].x = xx;
	points[3].y = yy;

	x_min =
	    MIN (MIN (points[0].x, points[1].x),
		 MIN (points[2].x, points[3].x));
	x_max =
	    MAX (MAX (points[0].x, points[1].x),
		 MAX (points[2].x, points[3].x));
	y_min =
	    MIN (MIN (points[0].y, points[1].y),
		 MIN (points[2].y, points[3].y));
	y_max =
	    MAX (MAX (points[0].y, points[1].y),
		 MAX (points[2].y, points[3].y));

	x = xkb_to_pixmap_coord (drawing->renderContext,
				 origin_x + x_min) - 6;
	y = xkb_to_pixmap_coord (drawing->renderContext,
				 origin_y + y_min) - 6;
	width =
	    xkb_to_pixmap_coord (drawing->renderContext,
				 x_max - x_min) + 12;
	height =
	    xkb_to_pixmap_coord (drawing->renderContext,
				 y_max - y_min) + 12;

	gtk_widget_queue_draw_area (GTK_WIDGET (drawing), x, y, width,
				    height);
}

static void
invalidate_indicator_doodad_region (MatekbdKeyboardDrawing * drawing,
				    MatekbdKeyboardDrawingDoodad * doodad)
{
	if (!drawing->xkb)
		return;

	invalidate_region (drawing,
			   doodad->angle,
			   doodad->origin_x +
			   doodad->doodad->indicator.left,
			   doodad->origin_y +
			   doodad->doodad->indicator.top,
			   &drawing->xkb->geom->shapes[doodad->
						       doodad->indicator.shape_ndx]);
}

static void
invalidate_key_region (MatekbdKeyboardDrawing * drawing,
		       MatekbdKeyboardDrawingKey * key)
{
	if (!drawing->xkb)
		return;

	invalidate_region (drawing,
			   key->angle,
			   key->origin_x,
			   key->origin_y,
			   &drawing->xkb->geom->shapes[key->
						       xkbkey->shape_ndx]);
}

static void
draw_text_doodad (MatekbdKeyboardDrawingRenderContext * context,
		  MatekbdKeyboardDrawing * drawing,
		  MatekbdKeyboardDrawingDoodad * doodad,
		  XkbTextDoodadRec * text_doodad)
{
	gint x, y;
	if (!drawing->xkb)
		return;

	x = xkb_to_pixmap_coord (context,
				 doodad->origin_x + text_doodad->left);
	y = xkb_to_pixmap_coord (context,
				 doodad->origin_y + text_doodad->top);

	pango_layout_set_text (context->layout, text_doodad->text, -1);
	draw_pango_layout (context, drawing, doodad->angle, x, y);
}

static void
draw_indicator_doodad (MatekbdKeyboardDrawingRenderContext * context,
		       MatekbdKeyboardDrawing * drawing,
		       MatekbdKeyboardDrawingDoodad * doodad,
		       XkbIndicatorDoodadRec * indicator_doodad)
{
	GdkColor *color;
	XkbShapeRec *shape;
	gint i;

	if (!drawing->xkb)
		return;

	shape = drawing->xkb->geom->shapes + indicator_doodad->shape_ndx;

	color = drawing->colors + (doodad->on ?
				   indicator_doodad->on_color_ndx :
				   indicator_doodad->off_color_ndx);

	for (i = 0; i < 1; i++)
		draw_outline (context, shape->outlines + i, color,
			      doodad->angle,
			      doodad->origin_x + indicator_doodad->left,
			      doodad->origin_y + indicator_doodad->top);
}

static void
draw_shape_doodad (MatekbdKeyboardDrawingRenderContext * context,
		   MatekbdKeyboardDrawing * drawing,
		   MatekbdKeyboardDrawingDoodad * doodad,
		   XkbShapeDoodadRec * shape_doodad)
{
	XkbShapeRec *shape;
	GdkColor *color;
	gint i;

	if (!drawing->xkb)
		return;

	shape = drawing->xkb->geom->shapes + shape_doodad->shape_ndx;
	color = drawing->colors + shape_doodad->color_ndx;

	/* draw the primary outline filled */
	draw_outline (context,
		      shape->primary ? shape->primary : shape->outlines,
		      color, doodad->angle,
		      doodad->origin_x + shape_doodad->left,
		      doodad->origin_y + shape_doodad->top);

	/* stroke the other outlines */
	for (i = 0; i < shape->num_outlines; i++) {
		if (shape->outlines + i == shape->approx ||
		    shape->outlines + i == shape->primary)
			continue;
		draw_outline (context, shape->outlines + i, NULL,
			      doodad->angle,
			      doodad->origin_x + shape_doodad->left,
			      doodad->origin_y + shape_doodad->top);
	}
}

static void
draw_doodad (MatekbdKeyboardDrawingRenderContext * context,
	     MatekbdKeyboardDrawing * drawing,
	     MatekbdKeyboardDrawingDoodad * doodad)
{
	switch (doodad->doodad->any.type) {
	case XkbOutlineDoodad:
	case XkbSolidDoodad:
		draw_shape_doodad (context, drawing, doodad,
				   &doodad->doodad->shape);
		break;

	case XkbTextDoodad:
		draw_text_doodad (context, drawing, doodad,
				  &doodad->doodad->text);
		break;

	case XkbIndicatorDoodad:
		draw_indicator_doodad (context, drawing, doodad,
				       &doodad->doodad->indicator);
		break;

	case XkbLogoDoodad:
		/* g_print ("draw_doodad: logo: %s\n", doodad->doodad->logo.logo_name); */
		/* XkbLogoDoodadRec is essentially a subclass of XkbShapeDoodadRec */
		draw_shape_doodad (context, drawing, doodad,
				   &doodad->doodad->shape);
		break;
	}
}

typedef struct {
	MatekbdKeyboardDrawing *drawing;
	MatekbdKeyboardDrawingRenderContext *context;
} DrawKeyboardItemData;

static void
redraw_overlapping_doodads (MatekbdKeyboardDrawingRenderContext * context,
			    MatekbdKeyboardDrawing * drawing,
			    MatekbdKeyboardDrawingKey * key)
{
	GList *list;
	gboolean do_draw = FALSE;

	for (list = drawing->keyboard_items; list; list = list->next) {
		MatekbdKeyboardDrawingItem *item = list->data;

		if (do_draw
		    && item->type ==
		    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD)
			draw_doodad (context, drawing,
				     (MatekbdKeyboardDrawingDoodad *) item);

		if (list->data == key)
			do_draw = TRUE;
	}
}

static void
draw_keyboard_item (MatekbdKeyboardDrawingItem * item,
		    DrawKeyboardItemData * data)
{
	MatekbdKeyboardDrawing *drawing = data->drawing;
	MatekbdKeyboardDrawingRenderContext *context = data->context;

	if (!drawing->xkb)
		return;

	switch (item->type) {
	case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_INVALID:
		break;

	case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY:
	case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA:
		draw_key (context, drawing,
			  (MatekbdKeyboardDrawingKey *) item);
		break;

	case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD:
		draw_doodad (context, drawing,
			     (MatekbdKeyboardDrawingDoodad *) item);
		break;
	}
}

static void
draw_keyboard_to_context (MatekbdKeyboardDrawingRenderContext * context,
			  MatekbdKeyboardDrawing * drawing)
{
	DrawKeyboardItemData data = { drawing, context };
#ifdef KBDRAW_DEBUG
	printf ("mods: %d\n", drawing->mods);
#endif
	g_list_foreach (drawing->keyboard_items,
			(GFunc) draw_keyboard_item, &data);
}

static gboolean
create_cairo (MatekbdKeyboardDrawing * drawing)
{
	GtkStateType state;
	if (drawing == NULL || drawing->pixmap == NULL)
		return FALSE;
	drawing->renderContext->cr =
	    gdk_cairo_create (GDK_DRAWABLE (drawing->pixmap));

	state = gtk_widget_get_state (GTK_WIDGET (drawing));
	drawing->renderContext->dark_color =
	    &gtk_widget_get_style (GTK_WIDGET (drawing))->dark[state];
	return TRUE;
}

static void
destroy_cairo (MatekbdKeyboardDrawing * drawing)
{
	cairo_destroy (drawing->renderContext->cr);
	drawing->renderContext->cr = NULL;
	drawing->renderContext->dark_color = NULL;
}

static void
draw_keyboard (MatekbdKeyboardDrawing * drawing)
{
	GtkStateType state = gtk_widget_get_state (GTK_WIDGET (drawing));
        GtkStyle *style = gtk_widget_get_style (GTK_WIDGET (drawing));
	GtkAllocation allocation;

	if (!drawing->xkb)
		return;

	gtk_widget_get_allocation (GTK_WIDGET (drawing), &allocation);

	drawing->pixmap =
	    gdk_pixmap_new (gtk_widget_get_window (GTK_WIDGET (drawing)),
			    allocation.width, allocation.height, -1);

	if (create_cairo (drawing)) {
	        /* blank background */
                gdk_cairo_set_source_color (drawing->renderContext->cr,
                                            &style->base[state]);
                cairo_paint (drawing->renderContext->cr);

		draw_keyboard_to_context (drawing->renderContext, drawing);
		destroy_cairo (drawing);
	}
}

static void
alloc_render_context (MatekbdKeyboardDrawing * drawing)
{
	MatekbdKeyboardDrawingRenderContext *context =
	    drawing->renderContext =
	    g_new0 (MatekbdKeyboardDrawingRenderContext, 1);

	PangoContext *pangoContext =
	    gtk_widget_get_pango_context (GTK_WIDGET (drawing));
	context->layout = pango_layout_new (pangoContext);
	pango_layout_set_ellipsize (context->layout, PANGO_ELLIPSIZE_END);

	context->font_desc =
	    pango_font_description_copy (gtk_widget_get_style
					 (GTK_WIDGET
					  (drawing))->font_desc);
	context->angle = 0;
	context->scale_numerator = 1;
	context->scale_denominator = 1;
}

static void
free_render_context (MatekbdKeyboardDrawing * drawing)
{
	MatekbdKeyboardDrawingRenderContext *context = drawing->renderContext;
	g_object_unref (G_OBJECT (context->layout));
	pango_font_description_free (context->font_desc);

	g_free (drawing->renderContext);
	drawing->renderContext = NULL;
}

static gboolean
expose_event (GtkWidget * widget,
	      GdkEventExpose * event, MatekbdKeyboardDrawing * drawing)
{
	GtkAllocation allocation;
        cairo_t *cr;

	if (!drawing->xkb)
		return FALSE;

	if (drawing->pixmap == NULL)
		return FALSE;

        cr = gdk_cairo_create (event->window);
        gdk_cairo_region (cr, event->region);
        cairo_clip (cr);

        gdk_cairo_set_source_pixmap (cr, drawing->pixmap, 0, 0);
        cairo_paint (cr);

        cairo_destroy (cr);

	if (gtk_widget_has_focus (widget)) {
		gtk_widget_get_allocation (widget, &allocation);
		gtk_paint_focus (gtk_widget_get_style (widget),
				 gtk_widget_get_window (widget),
				 gtk_widget_get_state (widget),
				 &event->area, widget, "keyboard-drawing",
				 0, 0, allocation.width,
				 allocation.height);
	}

	return FALSE;
}

static gboolean
idle_redraw (gpointer user_data)
{
	MatekbdKeyboardDrawing *drawing = user_data;

	drawing->idle_redraw = 0;
	draw_keyboard (drawing);
	gtk_widget_queue_draw (GTK_WIDGET (drawing));
	return FALSE;
}

static gboolean
context_setup_scaling (MatekbdKeyboardDrawingRenderContext * context,
		       MatekbdKeyboardDrawing * drawing,
		       gdouble width, gdouble height,
		       gdouble dpi_x, gdouble dpi_y)
{
	if (!drawing->xkb)
		return FALSE;

	if (drawing->xkb->geom->width_mm <= 0
	    || drawing->xkb->geom->height_mm <= 0) {
		g_critical
		    ("keyboard geometry reports width or height as zero!");
		return FALSE;
	}

	if (width * drawing->xkb->geom->height_mm <
	    height * drawing->xkb->geom->width_mm) {
		context->scale_numerator = width;
		context->scale_denominator = drawing->xkb->geom->width_mm;
	} else {
		context->scale_numerator = height;
		context->scale_denominator = drawing->xkb->geom->height_mm;
	}

	pango_font_description_set_size (context->font_desc,
					 720 * dpi_x *
					 context->scale_numerator /
					 context->scale_denominator);
	pango_layout_set_spacing (context->layout,
				  -160 * dpi_y * context->scale_numerator /
				  context->scale_denominator);
	pango_layout_set_font_description (context->layout,
					   context->font_desc);

	return TRUE;
}

static void
size_allocate (GtkWidget * widget,
	       GtkAllocation * allocation, MatekbdKeyboardDrawing * drawing)
{
	MatekbdKeyboardDrawingRenderContext *context = drawing->renderContext;

	if (drawing->pixmap) {
		g_object_unref (drawing->pixmap);
		drawing->pixmap = NULL;
	}

	if (!context_setup_scaling (context, drawing,
				    allocation->width, allocation->height,
				    50, 50))
		return;

	if (!drawing->idle_redraw)
		drawing->idle_redraw = g_idle_add (idle_redraw, drawing);
}

static gint
key_event (GtkWidget * widget,
	   GdkEventKey * event, MatekbdKeyboardDrawing * drawing)
{
	MatekbdKeyboardDrawingKey *key;
	if (!drawing->xkb)
		return FALSE;

	key = drawing->keys + event->hardware_keycode;

	if (event->hardware_keycode > drawing->xkb->max_key_code ||
	    event->hardware_keycode < drawing->xkb->min_key_code ||
	    key->xkbkey == NULL) {
		g_signal_emit (drawing,
			       matekbd_keyboard_drawing_signals[BAD_KEYCODE],
			       0, event->hardware_keycode);
		return TRUE;
	}

	if ((event->type == GDK_KEY_PRESS && key->pressed) ||
	    (event->type == GDK_KEY_RELEASE && !key->pressed))
		return TRUE;
	/* otherwise this event changes the state we believed we had before */

	key->pressed = (event->type == GDK_KEY_PRESS);

	if (create_cairo (drawing)) {
		draw_key (drawing->renderContext, drawing, key);
		redraw_overlapping_doodads (drawing->renderContext,
					    drawing, key);
		destroy_cairo (drawing);
	}

	invalidate_key_region (drawing, key);
	return FALSE;
}

static gint
button_press_event (GtkWidget * widget,
		    GdkEventButton * event, MatekbdKeyboardDrawing * drawing)
{
	if (!drawing->xkb)
		return FALSE;

	gtk_widget_grab_focus (widget);
	return FALSE;
}

static gboolean
unpress_keys (MatekbdKeyboardDrawing * drawing)
{
	gint i;

	if (!drawing->xkb)
		return FALSE;

	if (create_cairo (drawing)) {
		for (i = drawing->xkb->min_key_code;
		     i <= drawing->xkb->max_key_code; i++)
			if (drawing->keys[i].pressed) {
				drawing->keys[i].pressed = FALSE;
				draw_key (drawing->renderContext, drawing,
					  drawing->keys + i);
				invalidate_key_region (drawing,
						       drawing->keys + i);
			}
		destroy_cairo (drawing);
	}

	return FALSE;
}

static gint
focus_event (GtkWidget * widget,
	     GdkEventFocus * event, MatekbdKeyboardDrawing * drawing)
{
	if (event->in && drawing->timeout > 0) {
		g_source_remove (drawing->timeout);
		drawing->timeout = 0;
	} else
		drawing->timeout =
		    g_timeout_add (120, (GSourceFunc) unpress_keys,
				   drawing);

	return FALSE;
}

static gint
compare_keyboard_item_priorities (MatekbdKeyboardDrawingItem * a,
				  MatekbdKeyboardDrawingItem * b)
{
	if (a->priority > b->priority)
		return 1;
	else if (a->priority < b->priority)
		return -1;
	else
		return 0;
}

static void
init_indicator_doodad (MatekbdKeyboardDrawing * drawing,
		       XkbDoodadRec * xkbdoodad,
		       MatekbdKeyboardDrawingDoodad * doodad)
{
	if (!drawing->xkb)
		return;

	if (xkbdoodad->any.type == XkbIndicatorDoodad) {
		gint index;
		Atom iname = 0;
		Atom sname = xkbdoodad->indicator.name;
		unsigned long phys_indicators =
		    drawing->xkb->indicators->phys_indicators;
		Atom *pind = drawing->xkb->names->indicators;

#ifdef KBDRAW_DEBUG
		printf ("Looking for %d[%s]\n",
			(int) sname, XGetAtomName (drawing->display,
						   sname));
#endif

		for (index = 0; index < XkbNumIndicators; index++) {
			iname = *pind++;
			/* name matches and it is real */
			if (iname == sname
			    && (phys_indicators & (1 << index)))
				break;
			if (iname == 0)
				break;
		}
		if (iname == 0)
			g_warning ("Could not find indicator %d [%s]\n",
				   (int) sname,
				   XGetAtomName (drawing->display, sname));
		else {
#ifdef KBDRAW_DEBUG
			printf ("Found in xkbdesc as %d\n", index);
#endif
			drawing->physical_indicators[index] = doodad;
			/* Trying to obtain the real state, but if fail - just assume OFF */
			if (!XkbGetNamedIndicator
			    (drawing->display, sname, NULL, &doodad->on,
			     NULL, NULL))
				doodad->on = 0;
		}
	}
}

static void
init_keys_and_doodads (MatekbdKeyboardDrawing * drawing)
{
	gint i, j, k;
	gint x, y;

	if (!drawing->xkb)
		return;

	for (i = 0; i < drawing->xkb->geom->num_doodads; i++) {
		XkbDoodadRec *xkbdoodad = drawing->xkb->geom->doodads + i;
		MatekbdKeyboardDrawingDoodad *doodad =
		    g_new (MatekbdKeyboardDrawingDoodad, 1);

		doodad->type = MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD;
		doodad->origin_x = 0;
		doodad->origin_y = 0;
		doodad->angle = 0;
		doodad->priority = xkbdoodad->any.priority * 256 * 256;
		doodad->doodad = xkbdoodad;

		init_indicator_doodad (drawing, xkbdoodad, doodad);

		drawing->keyboard_items =
		    g_list_append (drawing->keyboard_items, doodad);
	}

	for (i = 0; i < drawing->xkb->geom->num_sections; i++) {
		XkbSectionRec *section = drawing->xkb->geom->sections + i;
		guint priority;

#ifdef KBDRAW_DEBUG
		printf ("initing section %d containing %d rows\n", i,
			section->num_rows);
#endif
		x = section->left;
		y = section->top;
		priority = section->priority * 256 * 256;

		for (j = 0; j < section->num_rows; j++) {
			XkbRowRec *row = section->rows + j;

#ifdef KBDRAW_DEBUG
			printf ("  initing row %d\n", j);
#endif
			x = section->left + row->left;
			y = section->top + row->top;

			for (k = 0; k < row->num_keys; k++) {
				XkbKeyRec *xkbkey = row->keys + k;
				MatekbdKeyboardDrawingKey *key;
				XkbShapeRec *shape =
				    drawing->xkb->geom->shapes +
				    xkbkey->shape_ndx;
				guint keycode = find_keycode (drawing,
							      xkbkey->
							      name.name);

				if (keycode == INVALID_KEYCODE)
					continue;
#ifdef KBDRAW_DEBUG
				printf
				    ("    initing key %d, shape: %p(%p + %d), code: %u\n",
				     k, shape, drawing->xkb->geom->shapes,
				     xkbkey->shape_ndx, keycode);
#endif
				if (row->vertical)
					y += xkbkey->gap;
				else
					x += xkbkey->gap;

				if (keycode >= drawing->xkb->min_key_code
				    && keycode <=
				    drawing->xkb->max_key_code) {
					key = drawing->keys + keycode;
					if (key->type ==
					    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_INVALID)
					{
						key->type =
						    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY;
					} else {
						/* duplicate key for the same keycode,
						   already defined as MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY */
						key =
						    g_new0
						    (MatekbdKeyboardDrawingKey,
						     1);
						key->type =
						    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA;
					}
				} else {
					g_warning
					    ("key %4.4s: keycode = %u; not in range %d..%d\n",
					     xkbkey->name.name, keycode,
					     drawing->xkb->min_key_code,
					     drawing->xkb->max_key_code);

					key =
					    g_new0 (MatekbdKeyboardDrawingKey,
						    1);
					key->type =
					    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA;
				}

				key->xkbkey = xkbkey;
				key->angle = section->angle;
				rotate_coordinate (section->left,
						   section->top, x, y,
						   section->angle,
						   &key->origin_x,
						   &key->origin_y);
				key->priority = priority;
				key->keycode = keycode;

				drawing->keyboard_items =
				    g_list_append (drawing->keyboard_items,
						   key);

				if (row->vertical)
					y += shape->bounds.y2;
				else
					x += shape->bounds.x2;

				priority++;
			}
		}

		for (j = 0; j < section->num_doodads; j++) {
			XkbDoodadRec *xkbdoodad = section->doodads + j;
			MatekbdKeyboardDrawingDoodad *doodad =
			    g_new (MatekbdKeyboardDrawingDoodad, 1);

			doodad->type =
			    MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD;
			doodad->origin_x = x;
			doodad->origin_y = y;
			doodad->angle = section->angle;
			doodad->priority =
			    priority + xkbdoodad->any.priority;
			doodad->doodad = xkbdoodad;

			init_indicator_doodad (drawing, xkbdoodad, doodad);

			drawing->keyboard_items =
			    g_list_append (drawing->keyboard_items,
					   doodad);
		}
	}

	drawing->keyboard_items = g_list_sort (drawing->keyboard_items,
					       (GCompareFunc)
					       compare_keyboard_item_priorities);
}

static void
init_colors (MatekbdKeyboardDrawing * drawing)
{
	gboolean result;
	gint i;

	if (!drawing->xkb)
		return;

	drawing->colors = g_new (GdkColor, drawing->xkb->geom->num_colors);

	for (i = 0; i < drawing->xkb->geom->num_colors; i++) {
		result =
		    parse_xkb_color_spec (drawing->xkb->geom->
					  colors[i].spec,
					  drawing->colors + i);

		if (!result)
			g_warning
			    ("init_colors: unable to parse color %s\n",
			     drawing->xkb->geom->colors[i].spec);
	}
}

static void
free_cdik (			/*colors doodads indicators keys */
		  MatekbdKeyboardDrawing * drawing)
{
	GList *itemp;

	if (!drawing->xkb)
		return;

	for (itemp = drawing->keyboard_items; itemp; itemp = itemp->next) {
		MatekbdKeyboardDrawingItem *item = itemp->data;

		switch (item->type) {
		case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_INVALID:
		case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY:
			break;

		case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA:
		case MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD:
			g_free (item);
			break;
		}
	}

	g_list_free (drawing->keyboard_items);
	drawing->keyboard_items = NULL;

	g_free (drawing->keys);
	g_free (drawing->colors);
}

static void
alloc_cdik (MatekbdKeyboardDrawing * drawing)
{
	drawing->physical_indicators_size =
	    drawing->xkb->indicators->phys_indicators + 1;
	drawing->physical_indicators =
	    g_new0 (MatekbdKeyboardDrawingDoodad *,
		    drawing->physical_indicators_size);
	drawing->keys =
	    g_new0 (MatekbdKeyboardDrawingKey,
		    drawing->xkb->max_key_code + 1);
}

static void
process_indicators_state_notify (XkbIndicatorNotifyEvent * iev,
				 MatekbdKeyboardDrawing * drawing)
{
	/* Good question: should we track indicators when the keyboard is
	   NOT really taken from the screen */
	gint i;

	for (i = 0; i <= drawing->xkb->indicators->phys_indicators; i++)
		if (drawing->physical_indicators[i] != NULL
		    && (iev->changed & 1 << i)) {
			gint state = (iev->state & 1 << i) != FALSE;

			if ((state && !drawing->physical_indicators[i]->on)
			    || (!state
				&& drawing->physical_indicators[i]->on)) {
				drawing->physical_indicators[i]->on =
				    state;
				if (create_cairo (drawing)) {
					draw_doodad
					    (drawing->renderContext,
					     drawing,
					     drawing->physical_indicators
					     [i]);
					destroy_cairo (drawing);
				}
				invalidate_indicator_doodad_region
				    (drawing,
				     drawing->physical_indicators[i]);
			}
		}
}

static GdkFilterReturn
xkb_state_notify_event_filter (GdkXEvent * gdkxev,
			       GdkEvent * event,
			       MatekbdKeyboardDrawing * drawing)
{
#define group_change_mask (XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask)
#define modifier_change_mask (XkbModifierStateMask | XkbModifierBaseMask | XkbModifierLatchMask | XkbModifierLockMask)

	if (!drawing->xkb)
		return GDK_FILTER_CONTINUE;

	if (((XEvent *) gdkxev)->type == drawing->xkb_event_type) {
		XkbEvent *kev = (XkbEvent *) gdkxev;
		GtkAllocation allocation;
		switch (kev->any.xkb_type) {
		case XkbStateNotify:
			if (((kev->state.changed & modifier_change_mask) &&
			     drawing->track_modifiers)) {
				free_cdik (drawing);
				if (drawing->track_modifiers)
					matekbd_keyboard_drawing_set_mods
					    (drawing,
					     kev->state.compat_state);
				drawing->keys =
				    g_new0 (MatekbdKeyboardDrawingKey,
					    drawing->xkb->max_key_code +
					    1);

				gtk_widget_get_allocation (GTK_WIDGET
							   (drawing),
							   &allocation);
				size_allocate (GTK_WIDGET (drawing),
					       &allocation, drawing);

				init_keys_and_doodads (drawing);
				init_colors (drawing);
			}
			break;

		case XkbIndicatorStateNotify:
			{
				process_indicators_state_notify (&
								 ((XkbEvent
								   *)
								  gdkxev)->indicators,
drawing);
			}
			break;

		case XkbIndicatorMapNotify:
		case XkbControlsNotify:
		case XkbNamesNotify:
		case XkbNewKeyboardNotify:
			{
				XkbStateRec state;
				memset (&state, 0, sizeof (state));
				XkbGetState (drawing->display,
					     XkbUseCoreKbd, &state);
				if (drawing->track_modifiers)
					matekbd_keyboard_drawing_set_mods
					    (drawing, state.compat_state);
				if (drawing->track_config)
					matekbd_keyboard_drawing_set_keyboard
					    (drawing, NULL);
			}
			break;
		}
	}

	return GDK_FILTER_CONTINUE;
}

static void
destroy (MatekbdKeyboardDrawing * drawing)
{
	free_render_context (drawing);
	gdk_window_remove_filter (NULL, (GdkFilterFunc)
				  xkb_state_notify_event_filter, drawing);
	if (drawing->timeout > 0) {
		g_source_remove (drawing->timeout);
		drawing->timeout = 0;
	}
	if (drawing->idle_redraw > 0) {
		g_source_remove (drawing->idle_redraw);
		drawing->idle_redraw = 0;
	}

	g_object_unref (drawing->pixmap);
}

static void
style_changed (MatekbdKeyboardDrawing * drawing)
{
	pango_layout_context_changed (drawing->renderContext->layout);
}

static void
matekbd_keyboard_drawing_init (MatekbdKeyboardDrawing * drawing)
{
	gint opcode = 0, error = 0, major = 1, minor = 0;
	gint mask;

	drawing->display =
	    GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
	printf ("dpy: %p\n", (void *) drawing->display);

	if (!XkbQueryExtension
	    (drawing->display, &opcode, &drawing->xkb_event_type, &error,
	     &major, &minor))
		g_critical
		    ("XkbQueryExtension failed! Stuff probably won't work.");

	printf ("evt/error/major/minor: %d/%d/%d/%d\n",
		drawing->xkb_event_type, error, major, minor);

	/* XXX: this stuff probably doesn't matter.. also, gdk_screen_get_default can fail */
	if (gtk_widget_has_screen (GTK_WIDGET (drawing)))
		drawing->screen_num =
		    gdk_screen_get_number (gtk_widget_get_screen
					   (GTK_WIDGET (drawing)));
	else
		drawing->screen_num =
		    gdk_screen_get_number (gdk_screen_get_default ());

	drawing->pixmap = NULL;
	alloc_render_context (drawing);

	drawing->keyboard_items = NULL;
	drawing->colors = NULL;

	drawing->track_modifiers = 0;
	drawing->track_config = 0;

	gtk_widget_set_double_buffered (GTK_WIDGET (drawing), FALSE);

	/* XXX: XkbClientMapMask | XkbIndicatorMapMask | XkbNamesMask | XkbGeometryMask */
	drawing->xkb = XkbGetKeyboard (drawing->display,
				       XkbGBN_GeometryMask |
				       XkbGBN_KeyNamesMask |
				       XkbGBN_OtherNamesMask |
				       XkbGBN_SymbolsMask |
				       XkbGBN_IndicatorMapMask,
				       XkbUseCoreKbd);
	if (drawing->xkb == NULL) {
		g_critical
		    ("XkbGetKeyboard failed to get keyboard from the server!");
		return;
	}

	XkbGetNames (drawing->display, XkbAllNamesMask, drawing->xkb);
	drawing->l3mod = XkbKeysymToModifiers (drawing->display,
					       GDK_ISO_Level3_Shift);

	drawing->xkbOnDisplay = TRUE;

	alloc_cdik (drawing);

	XkbSelectEventDetails (drawing->display, XkbUseCoreKbd,
			       XkbIndicatorStateNotify,
			       drawing->xkb->indicators->phys_indicators,
			       drawing->xkb->indicators->phys_indicators);

	mask =
	    (XkbStateNotifyMask | XkbNamesNotifyMask |
	     XkbControlsNotifyMask | XkbIndicatorMapNotifyMask |
	     XkbNewKeyboardNotifyMask);
	XkbSelectEvents (drawing->display, XkbUseCoreKbd, mask, mask);

	mask = XkbGroupStateMask | XkbModifierStateMask;
	XkbSelectEventDetails (drawing->display, XkbUseCoreKbd,
			       XkbStateNotify, mask, mask);

	mask = (XkbGroupNamesMask | XkbIndicatorNamesMask);
	XkbSelectEventDetails (drawing->display, XkbUseCoreKbd,
			       XkbNamesNotify, mask, mask);
	init_keys_and_doodads (drawing);
	init_colors (drawing);

	/* required to get key events */
	gtk_widget_set_can_focus (GTK_WIDGET (drawing), TRUE);

	gtk_widget_set_events (GTK_WIDGET (drawing),
			       GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK |
			       GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
			       | GDK_FOCUS_CHANGE_MASK);
	g_signal_connect (G_OBJECT (drawing), "expose-event",
			  G_CALLBACK (expose_event), drawing);
	g_signal_connect_after (G_OBJECT (drawing), "key-press-event",
				G_CALLBACK (key_event), drawing);
	g_signal_connect_after (G_OBJECT (drawing), "key-release-event",
				G_CALLBACK (key_event), drawing);
	g_signal_connect (G_OBJECT (drawing), "button-press-event",
			  G_CALLBACK (button_press_event), drawing);
	g_signal_connect (G_OBJECT (drawing), "focus-out-event",
			  G_CALLBACK (focus_event), drawing);
	g_signal_connect (G_OBJECT (drawing), "focus-in-event",
			  G_CALLBACK (focus_event), drawing);
	g_signal_connect (G_OBJECT (drawing), "size-allocate",
			  G_CALLBACK (size_allocate), drawing);
	g_signal_connect (G_OBJECT (drawing), "destroy",
			  G_CALLBACK (destroy), drawing);
	g_signal_connect (G_OBJECT (drawing), "style-set",
			  G_CALLBACK (style_changed), drawing);

	gdk_window_add_filter (NULL, (GdkFilterFunc)
			       xkb_state_notify_event_filter, drawing);
}

GtkWidget *
matekbd_keyboard_drawing_new (void)
{
	return
	    GTK_WIDGET (g_object_new
			(matekbd_keyboard_drawing_get_type (), NULL));
}

static void
matekbd_keyboard_drawing_class_init (MatekbdKeyboardDrawingClass * klass)
{
	klass->bad_keycode = NULL;

	matekbd_keyboard_drawing_signals[BAD_KEYCODE] =
	    g_signal_new ("bad-keycode", matekbd_keyboard_drawing_get_type (),
			  G_SIGNAL_RUN_FIRST,
			  G_STRUCT_OFFSET (MatekbdKeyboardDrawingClass,
					   bad_keycode), NULL, NULL,
			  matekbd_keyboard_drawing_VOID__UINT, G_TYPE_NONE, 1,
			  G_TYPE_UINT);
}

GType
matekbd_keyboard_drawing_get_type (void)
{
	static GType matekbd_keyboard_drawing_type = 0;

	if (!matekbd_keyboard_drawing_type) {
		static const GTypeInfo matekbd_keyboard_drawing_info = {
			sizeof (MatekbdKeyboardDrawingClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) matekbd_keyboard_drawing_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (MatekbdKeyboardDrawing),
			0,	/* n_preallocs */
			(GInstanceInitFunc) matekbd_keyboard_drawing_init,
		};

		matekbd_keyboard_drawing_type =
		    g_type_register_static (GTK_TYPE_DRAWING_AREA,
					    "MatekbdKeyboardDrawing",
					    &matekbd_keyboard_drawing_info,
					    0);
	}

	return matekbd_keyboard_drawing_type;
}

void
matekbd_keyboard_drawing_set_mods (MatekbdKeyboardDrawing * drawing, guint mods)
{
#ifdef KBDRAW_DEBUG
	printf ("set_mods: %d\n", mods);
#endif
	if (mods != drawing->mods) {
		drawing->mods = mods;
		gtk_widget_queue_draw (GTK_WIDGET (drawing));
	}
}

/* returns a pixbuf with the keyboard drawing at the current pixel size
 * (which can then be saved to disk, etc) */
GdkPixbuf *
matekbd_keyboard_drawing_get_pixbuf (MatekbdKeyboardDrawing * drawing)
{
	MatekbdKeyboardDrawingRenderContext *context = drawing->renderContext;

	if (drawing->pixmap == NULL)
		draw_keyboard (drawing);

	return gdk_pixbuf_get_from_drawable (NULL, drawing->pixmap, NULL,
					     0, 0, 0, 0,
					     xkb_to_pixmap_coord (context,
								  drawing->xkb->geom->width_mm),
					     xkb_to_pixmap_coord (context,
								  drawing->xkb->geom->height_mm));
}

/**
 * matekbd_keyboard_drawing_render:
 * @drawing: keyboard layout to render
 * @cr:        Cairo context to render to
 * @layout:    Pango layout to use to render text
 * @x:         left coordinate (pixels) of region to render in
 * @y:         top coordinate (pixels) of region to render in
 * @width:     width (pixels) of region to render in
 * @height:    height (pixels) of region to render in
 *
 * Renders a keyboard layout to a cairo_t context.  @cr and @layout can be got
 * from e.g. a GtkWidget or a GtkPrintContext.  @cr and @layout may be modified
 * by the function but will not be unreffed.
 *
 * @returns:   %TRUE on success, %FALSE on failure
 */
gboolean
matekbd_keyboard_drawing_render (MatekbdKeyboardDrawing * drawing,
			      cairo_t * cr,
			      PangoLayout * layout,
			      double x, double y,
			      double width, double height,
			      double dpi_x, double dpi_y)
{
	MatekbdKeyboardDrawingRenderContext context = {
		cr,
		drawing->renderContext->angle,
		layout,
		pango_font_description_copy (gtk_widget_get_style
					     (GTK_WIDGET
					      (drawing))->font_desc),
		1, 1,
		&gtk_widget_get_style (GTK_WIDGET (drawing))->dark
		    [gtk_widget_get_state (GTK_WIDGET (drawing))]
	};

	if (!context_setup_scaling (&context, drawing, width, height,
				    dpi_x, dpi_y))
		return FALSE;
	cairo_translate (cr, x, y);

	draw_keyboard_to_context (&context, drawing);

	pango_font_description_free (context.font_desc);

	return TRUE;
}

gboolean
matekbd_keyboard_drawing_set_keyboard (MatekbdKeyboardDrawing * drawing,
				    XkbComponentNamesRec * names)
{
	GtkAllocation allocation;

	free_cdik (drawing);
	if (drawing->xkb)
		XkbFreeKeyboard (drawing->xkb, 0, TRUE);	/* free_all = TRUE */
	drawing->xkb = NULL;

	if (names) {
		drawing->xkb =
		    XkbGetKeyboardByName (drawing->display, XkbUseCoreKbd,
					  names, 0,
					  XkbGBN_GeometryMask |
					  XkbGBN_KeyNamesMask |
					  XkbGBN_OtherNamesMask |
					  XkbGBN_ClientSymbolsMask |
					  XkbGBN_IndicatorMapMask, FALSE);
		drawing->xkbOnDisplay = FALSE;
	} else {
		drawing->xkb = XkbGetKeyboard (drawing->display,
					       XkbGBN_GeometryMask |
					       XkbGBN_KeyNamesMask |
					       XkbGBN_OtherNamesMask |
					       XkbGBN_SymbolsMask |
					       XkbGBN_IndicatorMapMask,
					       XkbUseCoreKbd);
		XkbGetNames (drawing->display, XkbAllNamesMask,
			     drawing->xkb);
		drawing->xkbOnDisplay = TRUE;
	}

	if (drawing->xkb == NULL)
		return FALSE;

	alloc_cdik (drawing);

	init_keys_and_doodads (drawing);
	init_colors (drawing);

	gtk_widget_get_allocation (GTK_WIDGET (drawing), &allocation);
	size_allocate (GTK_WIDGET (drawing), &allocation, drawing);
	gtk_widget_queue_draw (GTK_WIDGET (drawing));

	return TRUE;
}

const gchar* matekbd_keyboard_drawing_get_keycodes(MatekbdKeyboardDrawing* drawing)
{
	if (!drawing->xkb || drawing->xkb->names->keycodes <= 0)
	{
		return NULL;
	}
	else
	{
		return XGetAtomName(drawing->display, drawing->xkb->names->keycodes);
	}
}

const gchar* matekbd_keyboard_drawing_get_geometry(MatekbdKeyboardDrawing* drawing)
{
	if (!drawing->xkb || drawing->xkb->names->geometry <= 0)
	{
		return NULL;
	}
	else
	{
		return XGetAtomName(drawing->display, drawing->xkb->names->geometry);
	}
}

const gchar* matekbd_keyboard_drawing_get_symbols(MatekbdKeyboardDrawing* drawing)
{
	if (!drawing->xkb || drawing->xkb->names->symbols <= 0)
	{
		return NULL;
	}
	else
	{
		return XGetAtomName(drawing->display, drawing->xkb->names->symbols);
	}
}

const gchar* matekbd_keyboard_drawing_get_types(MatekbdKeyboardDrawing* drawing)
{
	if (!drawing->xkb || drawing->xkb->names->types <= 0)
	{
		return NULL;
	}
	else
	{
		return XGetAtomName(drawing->display, drawing->xkb->names->types);
	}
}

const gchar* matekbd_keyboard_drawing_get_compat(MatekbdKeyboardDrawing* drawing)
{
	if (!drawing->xkb || drawing->xkb->names->compat <= 0)
	{
		return NULL;
	}
	else
	{
		return XGetAtomName(drawing->display, drawing->xkb->names->compat);
	}
}

void
matekbd_keyboard_drawing_set_track_modifiers (MatekbdKeyboardDrawing * drawing,
					   gboolean enable)
{
	if (enable) {
		XkbStateRec state;
		drawing->track_modifiers = 1;
		memset (&state, 0, sizeof (state));
		XkbGetState (drawing->display, XkbUseCoreKbd, &state);
		matekbd_keyboard_drawing_set_mods (drawing,
						state.compat_state);
	} else
		drawing->track_modifiers = 0;
}

void
matekbd_keyboard_drawing_set_track_config (MatekbdKeyboardDrawing * drawing,
					gboolean enable)
{
	if (enable)
		drawing->track_config = 1;
	else
		drawing->track_config = 0;
}

void
matekbd_keyboard_drawing_set_groups_levels (MatekbdKeyboardDrawing * drawing,
					 MatekbdKeyboardDrawingGroupLevel *
					 groupLevels[])
{
#ifdef KBDRAW_DEBUG
	printf ("set_group_levels [topLeft]: %d %d \n",
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT]->group,
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT]->level);
	printf ("set_group_levels [topRight]: %d %d \n",
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_TOPRIGHT]->group,
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_TOPRIGHT]->level);
	printf ("set_group_levels [bottomLeft]: %d %d \n",
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT]->group,
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT]->level);
	printf ("set_group_levels [bottomRight]: %d %d \n",
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT]->group,
		groupLevels[MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT]->level);
#endif
	drawing->groupLevels = groupLevels;

	gtk_widget_queue_draw (GTK_WIDGET (drawing));
}

typedef struct {
	MatekbdKeyboardDrawing *drawing;
	const gchar *description;
} XkbLayoutPreviewPrintData;

static void
matekbd_keyboard_drawing_begin_print (GtkPrintOperation * operation,
				   GtkPrintContext * context,
				   XkbLayoutPreviewPrintData * data)
{
	/* We always print single-page documents */
	GtkPrintSettings *settings =
	    gtk_print_operation_get_print_settings (operation);
	gtk_print_operation_set_n_pages (operation, 1);
	if (!gtk_print_settings_has_key
	    (settings, GTK_PRINT_SETTINGS_ORIENTATION))
		gtk_print_settings_set_orientation (settings,
						    GTK_PAGE_ORIENTATION_LANDSCAPE);
}

static void
matekbd_keyboard_drawing_draw_page (GtkPrintOperation * operation,
				 GtkPrintContext * context,
				 gint page_nr,
				 XkbLayoutPreviewPrintData * data)
{
	cairo_t *cr = gtk_print_context_get_cairo_context (context);
	PangoLayout *layout =
	    gtk_print_context_create_pango_layout (context);
	PangoFontDescription *desc =
	    pango_font_description_from_string ("sans 8");
	gdouble width = gtk_print_context_get_width (context);
	gdouble height = gtk_print_context_get_height (context);
	gdouble dpi_x = gtk_print_context_get_dpi_x (context);
	gdouble dpi_y = gtk_print_context_get_dpi_y (context);
	gchar *header;

	gtk_print_operation_set_unit (operation, GTK_PIXELS);

	header = g_strdup_printf
	    (_("Keyboard layout \"%s\"\n"
	       "Copyright &#169; X.Org Foundation and "
	       "XKeyboardConfig contributors\n"
	       "For licensing see package metadata"), data->description);
	pango_layout_set_markup (layout, header, -1);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);
	pango_layout_set_width (layout, pango_units_from_double (width));
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_move_to (cr, 0, 0);
	pango_cairo_show_layout (cr, layout);

	matekbd_keyboard_drawing_render (MATEKBD_KEYBOARD_DRAWING
				      (data->drawing), cr, layout, 0.0,
				      0.0, width, height, dpi_x, dpi_y);

	g_object_unref (layout);
}

void
matekbd_keyboard_drawing_print (MatekbdKeyboardDrawing * drawing,
			     GtkWindow * parent_window,
			     const gchar * description)
{
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	static GtkPrintSettings *settings = NULL;
	XkbLayoutPreviewPrintData data = { drawing, description };

	print = gtk_print_operation_new ();

	if (settings != NULL)
		gtk_print_operation_set_print_settings (print, settings);

	g_signal_connect (print, "begin_print",
			  G_CALLBACK (matekbd_keyboard_drawing_begin_print),
			  &data);
	g_signal_connect (print, "draw_page",
			  G_CALLBACK (matekbd_keyboard_drawing_draw_page),
			  &data);

	res = gtk_print_operation_run (print,
				       GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       parent_window, NULL);

	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (settings != NULL)
			g_object_unref (settings);
		settings = gtk_print_operation_get_print_settings (print);
		g_object_ref (settings);
	}

	g_object_unref (print);
}

static void
show_layout_response (GtkWidget * dialog, gint resp)
{
	GdkRectangle rect;
	GtkWidget *kbdraw;
	const gchar *groupName;

	switch (resp) {
	case GTK_RESPONSE_HELP:
		gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
			      "ghelp:gswitchit?layout-view",
			      gtk_get_current_event_time (), NULL);
		return;
	case GTK_RESPONSE_CLOSE:
		gtk_window_get_position (GTK_WINDOW (dialog), &rect.x,
					 &rect.y);
		gtk_window_get_size (GTK_WINDOW (dialog), &rect.width,
				     &rect.height);
		matekbd_preview_save_position (&rect);
		gtk_widget_destroy (dialog);
		break;
	case GTK_RESPONSE_PRINT:
		kbdraw =
		    GTK_WIDGET (g_object_get_data
				(G_OBJECT (dialog), "kbdraw"));
		groupName =
		    (const gchar *) g_object_get_data (G_OBJECT (dialog),
						       "groupName");
		matekbd_keyboard_drawing_print (MATEKBD_KEYBOARD_DRAWING
					     (kbdraw), GTK_WINDOW (dialog),
					     groupName ? groupName :
					     _("Unknown"));
	}
}

GtkWidget *
matekbd_keyboard_drawing_new_dialog (gint group, gchar * group_name)
{
	static MatekbdKeyboardDrawingGroupLevel groupsLevels[] = { {
								 0, 1}, {
									 0,
									 3},
	{
	 0, 0}, {
		 0, 2}
	};
	static MatekbdKeyboardDrawingGroupLevel *pGroupsLevels[] = {
		groupsLevels, groupsLevels + 1, groupsLevels + 2,
		groupsLevels + 3
	};

	GtkBuilder *builder;
	GtkWidget *dialog, *kbdraw;
	XkbComponentNamesRec component_names;
	XklConfigRec *xkl_data;
	GdkRectangle *rect;
	GError *error = NULL;
	char title[128] = "";
	XklEngine *engine = xkl_engine_get_instance (GDK_DISPLAY ());

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, UIDIR "/show-layout.ui",
				   &error);

	if (error) {
		g_error ("building ui from %s failed: %s",
			 UIDIR "/show-layout.ui", error->message);
		g_clear_error (&error);
	}


	dialog =
	    GTK_WIDGET (gtk_builder_get_object
			(builder, "gswitchit_layout_view"));
	kbdraw = matekbd_keyboard_drawing_new ();

	snprintf (title, sizeof (title), _("Keyboard Layout \"%s\""),
		  group_name);
	gtk_window_set_title (GTK_WINDOW (dialog), title);
	g_object_set_data_full (G_OBJECT (dialog), "group_name",
				g_strdup (group_name), g_free);

	matekbd_keyboard_drawing_set_groups_levels (MATEKBD_KEYBOARD_DRAWING
						 (kbdraw), pGroupsLevels);

	xkl_data = xkl_config_rec_new ();
	if (xkl_config_rec_get_from_server (xkl_data, engine)) {
		int num_layouts = g_strv_length (xkl_data->layouts);
		int num_variants = g_strv_length (xkl_data->variants);
		if (group >= 0 && group < num_layouts
		    && group < num_variants) {
			char *l = g_strdup (xkl_data->layouts[group]);
			char *v = g_strdup (xkl_data->variants[group]);
			char **p;
			int i;

			if ((p = xkl_data->layouts) != NULL)
				for (i = num_layouts; --i >= 0;)
					g_free (*p++);

			if ((p = xkl_data->variants) != NULL)
				for (i = num_variants; --i >= 0;)
					g_free (*p++);

			xkl_data->layouts =
			    g_realloc (xkl_data->layouts,
				       sizeof (char *) * 2);
			xkl_data->variants =
			    g_realloc (xkl_data->variants,
				       sizeof (char *) * 2);
			xkl_data->layouts[0] = l;
			xkl_data->variants[0] = v;
			xkl_data->layouts[1] = xkl_data->variants[1] =
			    NULL;
		}

		if (xkl_xkb_config_native_prepare
		    (engine, xkl_data, &component_names)) {
			matekbd_keyboard_drawing_set_keyboard
			    (MATEKBD_KEYBOARD_DRAWING (kbdraw),
			     &component_names);
			xkl_xkb_config_native_cleanup (engine,
						       &component_names);
		}
	}
	g_object_unref (G_OBJECT (xkl_data));

	g_object_set_data (G_OBJECT (dialog), "builderData", builder);
	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (show_layout_response), NULL);

	rect = matekbd_preview_load_position ();
	if (rect != NULL) {
		gtk_window_move (GTK_WINDOW (dialog), rect->x, rect->y);
		gtk_window_resize (GTK_WINDOW (dialog), rect->width,
				   rect->height);
		g_free (rect);
	} else
		gtk_window_resize (GTK_WINDOW (dialog), 700, 400);

	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

	gtk_container_add (GTK_CONTAINER
			   (gtk_builder_get_object
			    (builder, "preview_vbox")), kbdraw);

	g_object_set_data (G_OBJECT (dialog), "kbdraw", kbdraw);

	g_signal_connect_swapped (GTK_OBJECT (dialog), "destroy",
				  G_CALLBACK (g_object_unref),
				  g_object_get_data (G_OBJECT (dialog),
						     "builderData"));

	gtk_widget_show_all (dialog);

	return dialog;
}
