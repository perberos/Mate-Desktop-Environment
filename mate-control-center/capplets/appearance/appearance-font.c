/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Jonathan Blandford <jrb@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"

#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifdef HAVE_XFT2
	#include <gdk/gdkx.h>
	#include <X11/Xft/Xft.h>
#endif /* HAVE_XFT2 */

#include <glib/gi18n.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"

#define GTK_FONT_KEY           "/desktop/mate/interface/font_name"
#define DESKTOP_FONT_KEY       "/apps/caja/preferences/desktop_font"

#define MARCO_DIR "/apps/marco/general"
#define WINDOW_TITLE_FONT_KEY MARCO_DIR "/titlebar_font"
#define WINDOW_TITLE_USES_SYSTEM_KEY MARCO_DIR "/titlebar_uses_system_font"
#define MONOSPACE_FONT_KEY "/desktop/mate/interface/monospace_font_name"
#define DOCUMENT_FONT_KEY "/desktop/mate/interface/document_font_name"

#ifdef HAVE_XFT2
#define FONT_RENDER_DIR "/desktop/mate/font_rendering"
#define FONT_ANTIALIASING_KEY FONT_RENDER_DIR "/antialiasing"
#define FONT_HINTING_KEY      FONT_RENDER_DIR "/hinting"
#define FONT_RGBA_ORDER_KEY   FONT_RENDER_DIR "/rgba_order"
#define FONT_DPI_KEY          FONT_RENDER_DIR "/dpi"

/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_mateconf_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500
#endif /* HAVE_XFT2 */

static gboolean in_change = FALSE;
static gchar* old_font = NULL;

#define MAX_FONT_POINT_WITHOUT_WARNING 32
#define MAX_FONT_SIZE_WITHOUT_WARNING MAX_FONT_POINT_WITHOUT_WARNING * 1024

#ifdef HAVE_XFT2

/*
 * Code for displaying previews of font rendering with various Xft options
 */

static void sample_size_request(GtkWidget* darea, GtkRequisition* requisition)
{
	GdkPixbuf* pixbuf = g_object_get_data(G_OBJECT(darea), "sample-pixbuf");

	requisition->width = gdk_pixbuf_get_width(pixbuf) + 2;
	requisition->height = gdk_pixbuf_get_height(pixbuf) + 2;
}

static void sample_expose(GtkWidget* darea, GdkEventExpose* expose)
{
	GtkAllocation allocation;
	GdkPixbuf* pixbuf = g_object_get_data(G_OBJECT(darea), "sample-pixbuf");
	GdkWindow* window = gtk_widget_get_window(darea);
	GtkStyle* style = gtk_widget_get_style(darea);
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);

	gtk_widget_get_allocation (darea, &allocation);

	int x = (allocation.width - width) / 2;
	int y = (allocation.height - height) / 2;

	gdk_draw_rectangle(window, style->white_gc, TRUE, 0, 0, allocation.width, allocation.height);
	gdk_draw_rectangle(window, style->black_gc, FALSE, 0, 0, allocation.width - 1, allocation.height - 1);

	gdk_draw_pixbuf(window, NULL, pixbuf, 0, 0, x, y, width, height, GDK_RGB_DITHER_NORMAL, 0, 0);
}

typedef enum {
	ANTIALIAS_NONE,
	ANTIALIAS_GRAYSCALE,
	ANTIALIAS_RGBA
} Antialiasing;

static MateConfEnumStringPair antialias_enums[] = {
	{ANTIALIAS_NONE,      "none"},
	{ANTIALIAS_GRAYSCALE, "grayscale"},
	{ANTIALIAS_RGBA,      "rgba"},
	{-1,                  NULL}
};

typedef enum {
	HINT_NONE,
	HINT_SLIGHT,
	HINT_MEDIUM,
	HINT_FULL
} Hinting;

static MateConfEnumStringPair hint_enums[] = {
	{HINT_NONE,   "none"},
	{HINT_SLIGHT, "slight"},
	{HINT_MEDIUM, "medium"},
	{HINT_FULL,   "full"},
	{-1,          NULL}
};

typedef enum {
	RGBA_RGB,
	RGBA_BGR,
	RGBA_VRGB,
	RGBA_VBGR
} RgbaOrder;

static MateConfEnumStringPair rgba_order_enums[] = {
	{RGBA_RGB,  "rgb" },
	{RGBA_BGR,  "bgr" },
	{RGBA_VRGB, "vrgb" },
	{RGBA_VBGR, "vbgr" },
	{-1,         NULL }
};

static XftFont* open_pattern(FcPattern* pattern, Antialiasing antialiasing, Hinting hinting)
{
	#ifdef FC_HINT_STYLE
		static const int hintstyles[] = {
			FC_HINT_NONE, FC_HINT_SLIGHT, FC_HINT_MEDIUM, FC_HINT_FULL
		};
	#endif /* FC_HINT_STYLE */

	FcPattern* res_pattern;
	FcResult result;
	XftFont* font;

	Display* xdisplay = gdk_x11_get_default_xdisplay();
	int screen = gdk_x11_get_default_screen();

	res_pattern = XftFontMatch(xdisplay, screen, pattern, &result);
	
	if (res_pattern == NULL)
	{
		return NULL;
	}

	FcPatternDel(res_pattern, FC_HINTING);
	FcPatternAddBool(res_pattern, FC_HINTING, hinting != HINT_NONE);

	#ifdef FC_HINT_STYLE
		FcPatternDel(res_pattern, FC_HINT_STYLE);
		FcPatternAddInteger(res_pattern, FC_HINT_STYLE, hintstyles[hinting]);
	#endif /* FC_HINT_STYLE */

	FcPatternDel(res_pattern, FC_ANTIALIAS);
	FcPatternAddBool(res_pattern, FC_ANTIALIAS, antialiasing != ANTIALIAS_NONE);

	FcPatternDel(res_pattern, FC_RGBA);
	FcPatternAddInteger(res_pattern, FC_RGBA, antialiasing == ANTIALIAS_RGBA ? FC_RGBA_RGB : FC_RGBA_NONE);

	FcPatternDel(res_pattern, FC_DPI);
	FcPatternAddInteger(res_pattern, FC_DPI, 96);

	font = XftFontOpenPattern(xdisplay, res_pattern);
	
	if (!font)
	{
		FcPatternDestroy(res_pattern);
	}

	return font;
}

static void setup_font_sample(GtkWidget* darea, Antialiasing antialiasing, Hinting hinting)
{
	const char* string1 = "abcfgop AO ";
	const char* string2 = "abcfgop";

	XftColor black, white;
	XRenderColor rendcolor;

	Display* xdisplay = gdk_x11_get_default_xdisplay();

	GdkColormap* colormap = gdk_rgb_get_colormap();
	Colormap xcolormap = GDK_COLORMAP_XCOLORMAP(colormap);

	GdkVisual* visual = gdk_colormap_get_visual(colormap);
	Visual* xvisual = GDK_VISUAL_XVISUAL(visual);

	FcPattern* pattern;
	XftFont* font1;
	XftFont* font2;
	XGlyphInfo extents1 = { 0 };
	XGlyphInfo extents2 = { 0 };
	GdkPixmap* pixmap;
	XftDraw* draw;
	GdkPixbuf* tmp_pixbuf;
	GdkPixbuf* pixbuf;

	int width, height;
	int ascent, descent;

	pattern = FcPatternBuild (NULL,
		FC_FAMILY, FcTypeString, "Serif",
		FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN,
		FC_SIZE, FcTypeDouble, 18.,
		NULL);
	font1 = open_pattern (pattern, antialiasing, hinting);
	FcPatternDestroy (pattern);

	pattern = FcPatternBuild (NULL,
		FC_FAMILY, FcTypeString, "Serif",
		FC_SLANT, FcTypeInteger, FC_SLANT_ITALIC,
		FC_SIZE, FcTypeDouble, 20.,
		NULL);
	font2 = open_pattern (pattern, antialiasing, hinting);
	FcPatternDestroy (pattern);

	ascent = 0;
	descent = 0;
	
	if (font1)
	{
		XftTextExtentsUtf8 (xdisplay, font1, (unsigned char*) string1,
		strlen (string1), &extents1);
		ascent = MAX (ascent, font1->ascent);
		descent = MAX (descent, font1->descent);
	}

	if (font2)
	{
		XftTextExtentsUtf8 (xdisplay, font2, (unsigned char*) string2, strlen (string2), &extents2);
		ascent = MAX (ascent, font2->ascent);
		descent = MAX (descent, font2->descent);
	}

	width = extents1.xOff + extents2.xOff + 4;
	height = ascent + descent + 2;

	pixmap = gdk_pixmap_new (NULL, width, height, visual->depth);

	draw = XftDrawCreate (xdisplay, GDK_DRAWABLE_XID (pixmap), xvisual, xcolormap);

	rendcolor.red = 0;
	rendcolor.green = 0;
	rendcolor.blue = 0;
	rendcolor.alpha = 0xffff;
	
	XftColorAllocValue(xdisplay, xvisual, xcolormap, &rendcolor, &black);

	rendcolor.red = 0xffff;
	rendcolor.green = 0xffff;
	rendcolor.blue = 0xffff;
	rendcolor.alpha = 0xffff;
	
	XftColorAllocValue(xdisplay, xvisual, xcolormap, &rendcolor, &white);
	XftDrawRect(draw, &white, 0, 0, width, height);
	
	if (font1)
	{
		XftDrawStringUtf8(draw, &black, font1, 2, 2 + ascent, (unsigned char*) string1, strlen(string1));
	}
	
	if (font2)
	{
		XftDrawStringUtf8(draw, &black, font2, 2 + extents1.xOff, 2 + ascent, (unsigned char*) string2, strlen(string2));
	}

	XftDrawDestroy(draw);

	if (font1)
	{
		XftFontClose(xdisplay, font1);
	}
	
	if (font2)
	{
		XftFontClose(xdisplay, font2);
	}

	tmp_pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, colormap, 0, 0, 0, 0, width, height);
	pixbuf = gdk_pixbuf_scale_simple(tmp_pixbuf, 1 * width, 1 * height, GDK_INTERP_TILES);

	g_object_unref(pixmap);
	g_object_unref(tmp_pixbuf);

	g_object_set_data_full(G_OBJECT(darea), "sample-pixbuf", pixbuf, (GDestroyNotify) g_object_unref);

	g_signal_connect(darea, "size_request", G_CALLBACK(sample_size_request), NULL);
	g_signal_connect(darea, "expose_event", G_CALLBACK(sample_expose), NULL);
}

/*
 * Code implementing a group of radio buttons with different Xft option combinations.
 * If one of the buttons is matched by the MateConf key, we pick it. Otherwise we
 * show the group as inconsistent.
 */
static void
font_render_get_mateconf (MateConfClient  *client,
		       Antialiasing *antialiasing,
		       Hinting      *hinting)
{
  gchar *antialias_str = mateconf_client_get_string (client, FONT_ANTIALIASING_KEY, NULL);
  gchar *hint_str = mateconf_client_get_string (client, FONT_HINTING_KEY, NULL);
  gint val;

  val = ANTIALIAS_GRAYSCALE;
  if (antialias_str) {
    mateconf_string_to_enum (antialias_enums, antialias_str, &val);
    g_free (antialias_str);
  }
  *antialiasing = val;

  val = HINT_FULL;
  if (hint_str) {
    mateconf_string_to_enum (hint_enums, hint_str, &val);
    g_free (hint_str);
  }
  *hinting = val;
}

typedef struct {
  Antialiasing antialiasing;
  Hinting hinting;
  GtkToggleButton *radio;
} FontPair;

static GSList *font_pairs = NULL;

static void
font_render_load (MateConfClient *client)
{
  Antialiasing antialiasing;
  Hinting hinting;
  gboolean inconsistent = TRUE;
  GSList *tmp_list;

  font_render_get_mateconf (client, &antialiasing, &hinting);

  in_change = TRUE;

  for (tmp_list = font_pairs; tmp_list; tmp_list = tmp_list->next) {
    FontPair *pair = tmp_list->data;

    if (antialiasing == pair->antialiasing && hinting == pair->hinting) {
      gtk_toggle_button_set_active (pair->radio, TRUE);
      inconsistent = FALSE;
      break;
    }
  }

  for (tmp_list = font_pairs; tmp_list; tmp_list = tmp_list->next) {
    FontPair *pair = tmp_list->data;

    gtk_toggle_button_set_inconsistent (pair->radio, inconsistent);
  }

  in_change = FALSE;
}

static void
font_render_changed (MateConfClient *client,
                     guint        cnxn_id,
                     MateConfEntry  *entry,
                     gpointer     user_data)
{
  font_render_load (client);
}

static void
font_radio_toggled (GtkToggleButton *toggle_button,
		    FontPair        *pair)
{
  if (!in_change) {
    MateConfClient *client = mateconf_client_get_default ();

    mateconf_client_set_string (client, FONT_ANTIALIASING_KEY,
			     mateconf_enum_to_string (antialias_enums, pair->antialiasing),
			     NULL);
    mateconf_client_set_string (client, FONT_HINTING_KEY,
			     mateconf_enum_to_string (hint_enums, pair->hinting),
			     NULL);

    /* Restore back to the previous state until we get notification */
    font_render_load (client);
    g_object_unref (client);
  }
}

static void
setup_font_pair (GtkWidget    *radio,
		 GtkWidget    *darea,
		 Antialiasing  antialiasing,
		 Hinting       hinting)
{
  FontPair *pair = g_new (FontPair, 1);

  pair->antialiasing = antialiasing;
  pair->hinting = hinting;
  pair->radio = GTK_TOGGLE_BUTTON (radio);

  setup_font_sample (darea, antialiasing, hinting);
  font_pairs = g_slist_prepend (font_pairs, pair);

  g_signal_connect (radio, "toggled",
		    G_CALLBACK (font_radio_toggled), pair);
}
#endif /* HAVE_XFT2 */

static void
marco_titlebar_load_sensitivity (AppearanceData *data)
{
  gtk_widget_set_sensitive (appearance_capplet_get_widget (data, "window_title_font"),
			    !mateconf_client_get_bool (data->client,
						    WINDOW_TITLE_USES_SYSTEM_KEY,
						    NULL));
}

static void
marco_changed (MateConfClient *client,
		  guint        cnxn_id,
		  MateConfEntry  *entry,
		  gpointer     user_data)
{
  marco_titlebar_load_sensitivity (user_data);
}

/* returns 0 if the font is safe, otherwise returns the size in points. */
static gint
font_dangerous (const char *font)
{
  PangoFontDescription *pfd;
  gboolean retval = 0;

  pfd = pango_font_description_from_string (font);
  if (pfd == NULL)
    /* an invalid font was passed in.  This isn't our problem. */
    return 0;

  if ((pango_font_description_get_set_fields (pfd) & PANGO_FONT_MASK_SIZE) &&
      (pango_font_description_get_size (pfd) >= MAX_FONT_SIZE_WITHOUT_WARNING)) {
    retval = pango_font_description_get_size (pfd)/1024;
  }
  pango_font_description_free (pfd);

  return retval;
}

static MateConfValue *
application_font_to_mateconf (MateConfPropertyEditor *peditor,
			   MateConfValue          *value)
{
  MateConfValue *new_value;
  const char *new_font;
  GtkWidget *font_button;
  gint danger_level;

  font_button = GTK_WIDGET (mateconf_property_editor_get_ui_control (peditor));
  g_return_val_if_fail (font_button != NULL, NULL);

  new_value = mateconf_value_new (MATECONF_VALUE_STRING);
  new_font = mateconf_value_get_string (value);
  if (font_dangerous (old_font)) {
    /* If we're already too large, we don't warn again. */
    mateconf_value_set_string (new_value, new_font);
    return new_value;
  }

  danger_level = font_dangerous (new_font);
  if (danger_level) {
    GtkWidget *warning_dialog, *apply_button;
    const gchar *warning_label;
    gchar *warning_label2;

    warning_label = _("Font may be too large");

    if (danger_level > MAX_FONT_POINT_WITHOUT_WARNING) {
      warning_label2 = g_strdup_printf (ngettext (
			"The font selected is %d point large, "
			"and may make it difficult to effectively "
			"use the computer.  It is recommended that "
			"you select a size smaller than %d.",
			"The font selected is %d points large, "
			"and may make it difficult to effectively "
			"use the computer. It is recommended that "
			"you select a size smaller than %d.",
			danger_level),
			danger_level,
			MAX_FONT_POINT_WITHOUT_WARNING);
    } else {
      warning_label2 = g_strdup_printf (ngettext (
			"The font selected is %d point large, "
			"and may make it difficult to effectively "
			"use the computer.  It is recommended that "
			"you select a smaller sized font.",
			"The font selected is %d points large, "
			"and may make it difficult to effectively "
			"use the computer. It is recommended that "
			"you select a smaller sized font.",
			danger_level),
			danger_level);
    }

    warning_dialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_WARNING,
					     GTK_BUTTONS_NONE,
					     "%s",
					     warning_label);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (warning_dialog),
					      "%s", warning_label2);

    gtk_dialog_add_button (GTK_DIALOG (warning_dialog),
			   _("Use previous font"), GTK_RESPONSE_CLOSE);

    apply_button = gtk_button_new_with_label (_("Use selected font"));

    gtk_button_set_image (GTK_BUTTON (apply_button), gtk_image_new_from_stock (GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON));
    gtk_dialog_add_action_widget (GTK_DIALOG (warning_dialog), apply_button, GTK_RESPONSE_APPLY);
    gtk_widget_set_can_default (apply_button, TRUE);
    gtk_widget_show (apply_button);

    gtk_dialog_set_default_response (GTK_DIALOG (warning_dialog), GTK_RESPONSE_CLOSE);

    g_free (warning_label2);

    if (gtk_dialog_run (GTK_DIALOG (warning_dialog)) == GTK_RESPONSE_APPLY) {
      mateconf_value_set_string (new_value, new_font);
    } else {
      mateconf_value_set_string (new_value, old_font);
      gtk_font_button_set_font_name (GTK_FONT_BUTTON (font_button), old_font);
    }

    gtk_widget_destroy (warning_dialog);
  } else {
    mateconf_value_set_string (new_value, new_font);
  }

  return new_value;
}

static void
application_font_changed (GtkWidget *font_button)
{
  const gchar *font;

  font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (font_button));
  g_free (old_font);
  old_font = g_strdup (font);
}

#ifdef HAVE_XFT2
/*
 * EnumGroup - a group of radio buttons tied to a string enumeration
 *             value. We add this here because the mateconf peditor
 *             equivalent of this is both painful to use (you have
 *             to supply functions to convert from enums to indices)
 *             and conceptually broken (the order of radio buttons
 *             in a group when using Glade is not predictable.
 */
typedef struct
{
  MateConfClient *client;
  GSList *items;
  gchar *mateconf_key;
  MateConfEnumStringPair *enums;
  int default_value;
} EnumGroup;

typedef struct
{
  EnumGroup *group;
  GtkToggleButton *widget;
  int value;
} EnumItem;

static void
enum_group_load (EnumGroup *group)
{
  gchar *str = mateconf_client_get_string (group->client, group->mateconf_key, NULL);
  gint val = group->default_value;
  GSList *tmp_list;

  if (str)
    mateconf_string_to_enum (group->enums, str, &val);

  g_free (str);

  in_change = TRUE;

  for (tmp_list = group->items; tmp_list; tmp_list = tmp_list->next) {
    EnumItem *item = tmp_list->data;

    if (val == item->value)
      gtk_toggle_button_set_active (item->widget, TRUE);
  }

  in_change = FALSE;
}

static void
enum_group_changed (MateConfClient *client,
		    guint        cnxn_id,
		    MateConfEntry  *entry,
		    gpointer     user_data)
{
  enum_group_load (user_data);
}

static void
enum_item_toggled (GtkToggleButton *toggle_button,
		   EnumItem        *item)
{
  EnumGroup *group = item->group;

  if (!in_change) {
    mateconf_client_set_string (group->client, group->mateconf_key,
			     mateconf_enum_to_string (group->enums, item->value),
			     NULL);
  }

  /* Restore back to the previous state until we get notification */
  enum_group_load (group);
}

static EnumGroup *
enum_group_create (const gchar         *mateconf_key,
		   MateConfEnumStringPair *enums,
		   int                  default_value,
		   GtkWidget           *first_widget,
		   ...)
{
  EnumGroup *group;
  GtkWidget *widget;
  va_list args;

  group = g_new (EnumGroup, 1);

  group->client = mateconf_client_get_default ();
  group->mateconf_key = g_strdup (mateconf_key);
  group->enums = enums;
  group->default_value = default_value;
  group->items = NULL;

  va_start (args, first_widget);

  widget = first_widget;
  while (widget) {
    EnumItem *item;

    item = g_new (EnumItem, 1);
    item->group = group;
    item->widget = GTK_TOGGLE_BUTTON (widget);
    item->value = va_arg (args, int);

    g_signal_connect (item->widget, "toggled",
		      G_CALLBACK (enum_item_toggled), item);

    group->items = g_slist_prepend (group->items, item);

    widget = va_arg (args, GtkWidget *);
  }

  va_end (args);

  enum_group_load (group);

  mateconf_client_notify_add (group->client, mateconf_key,
			   enum_group_changed,
			   group, NULL, NULL);

  return group;
}

static void
enum_group_destroy (EnumGroup *group)
{
  g_object_unref (group->client);
  g_free (group->mateconf_key);

  g_slist_foreach (group->items, (GFunc) g_free, NULL);
  g_slist_free (group->items);

  g_free (group);
}

static double
dpi_from_pixels_and_mm (int pixels, int mm)
{
  double dpi;

  if (mm >= 1)
    dpi = pixels / (mm / 25.4);
  else
    dpi = 0;

  return dpi;
}

static double
get_dpi_from_x_server (void)
{
  GdkScreen *screen;
  double dpi;

  screen = gdk_screen_get_default ();
  if (screen) {
    double width_dpi, height_dpi;

    width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen),
					gdk_screen_get_width_mm (screen));
    height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen),
					 gdk_screen_get_height_mm (screen));

    if (width_dpi < DPI_LOW_REASONABLE_VALUE || width_dpi > DPI_HIGH_REASONABLE_VALUE ||
        height_dpi < DPI_LOW_REASONABLE_VALUE || height_dpi > DPI_HIGH_REASONABLE_VALUE)
      dpi = DPI_FALLBACK;
    else
      dpi = (width_dpi + height_dpi) / 2.0;
  } else {
    /* Huh!?  No screen? */
    dpi = DPI_FALLBACK;
  }

  return dpi;
}

/*
 * The font rendering details dialog
 */
static void
dpi_load (MateConfClient   *client,
	  GtkSpinButton *spinner)
{
  MateConfValue *value;
  gdouble dpi;

  value = mateconf_client_get_without_default (client, FONT_DPI_KEY, NULL);

  if (value) {
    dpi = mateconf_value_get_float (value);
    mateconf_value_free (value);
  } else
    dpi = get_dpi_from_x_server ();

  if (dpi < DPI_LOW_REASONABLE_VALUE)
    dpi = DPI_LOW_REASONABLE_VALUE;

  in_change = TRUE;
  gtk_spin_button_set_value (spinner, dpi);
  in_change = FALSE;
}

static void
dpi_changed (MateConfClient *client,
	     guint        cnxn_id,
	     MateConfEntry  *entry,
	     gpointer     user_data)
{
  dpi_load (client, user_data);
}

static void
dpi_value_changed (GtkSpinButton *spinner,
		   MateConfClient   *client)
{
  /* Like any time when using a spin button with MateConf, there is
   * a race condition here. When we change, we send the new
   * value to MateConf, then restore to the old value until
   * we get a response to emulate the proper model/view behavior.
   *
   * If the user changes the value faster than responses are
   * received from MateConf, this may cause mildly strange effects.
   */
  if (!in_change) {
    gdouble new_dpi = gtk_spin_button_get_value (spinner);

    mateconf_client_set_float (client, FONT_DPI_KEY, new_dpi, NULL);

    dpi_load (client, spinner);
  }
}

static void
cb_details_response (GtkDialog *dialog, gint response_id)
{
  if (response_id == GTK_RESPONSE_HELP) {
    capplet_help (GTK_WINDOW (dialog),
		  "goscustdesk-38");
  } else
    gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
cb_show_details (GtkWidget *button,
		 AppearanceData *data)
{
  if (!data->font_details) {
    GtkAdjustment *adjustment;
    GtkWidget *widget;
    EnumGroup *group;

    data->font_details = appearance_capplet_get_widget (data, "render_details");

    gtk_window_set_transient_for (GTK_WINDOW (data->font_details),
                                  GTK_WINDOW (appearance_capplet_get_widget (data, "appearance_window")));

    widget = appearance_capplet_get_widget (data, "dpi_spinner");

    /* pick a sensible maximum dpi */
    adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
    gtk_adjustment_set_lower (adjustment, DPI_LOW_REASONABLE_VALUE);
    gtk_adjustment_set_upper (adjustment, DPI_HIGH_REASONABLE_VALUE);
    gtk_adjustment_set_step_increment (adjustment, 1);

    dpi_load (data->client, GTK_SPIN_BUTTON (widget));
    g_signal_connect (widget, "value_changed",
		      G_CALLBACK (dpi_value_changed), data->client);

    mateconf_client_notify_add (data->client, FONT_DPI_KEY,
			     dpi_changed, widget, NULL, NULL);

    setup_font_sample (appearance_capplet_get_widget (data, "antialias_none_sample"),      ANTIALIAS_NONE,      HINT_FULL);
    setup_font_sample (appearance_capplet_get_widget (data, "antialias_grayscale_sample"), ANTIALIAS_GRAYSCALE, HINT_FULL);
    setup_font_sample (appearance_capplet_get_widget (data, "antialias_subpixel_sample"),  ANTIALIAS_RGBA,      HINT_FULL);

    group = enum_group_create (
    	FONT_ANTIALIASING_KEY, antialias_enums, ANTIALIAS_GRAYSCALE,
	appearance_capplet_get_widget (data, "antialias_none_radio"),      ANTIALIAS_NONE,
	appearance_capplet_get_widget (data, "antialias_grayscale_radio"), ANTIALIAS_GRAYSCALE,
	appearance_capplet_get_widget (data, "antialias_subpixel_radio"),  ANTIALIAS_RGBA,
	NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    setup_font_sample (appearance_capplet_get_widget (data, "hint_none_sample"),   ANTIALIAS_GRAYSCALE, HINT_NONE);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_slight_sample"), ANTIALIAS_GRAYSCALE, HINT_SLIGHT);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_medium_sample"), ANTIALIAS_GRAYSCALE, HINT_MEDIUM);
    setup_font_sample (appearance_capplet_get_widget (data, "hint_full_sample"),   ANTIALIAS_GRAYSCALE, HINT_FULL);

    group = enum_group_create (FONT_HINTING_KEY, hint_enums, HINT_FULL,
                               appearance_capplet_get_widget (data, "hint_none_radio"),   HINT_NONE,
                               appearance_capplet_get_widget (data, "hint_slight_radio"), HINT_SLIGHT,
                               appearance_capplet_get_widget (data, "hint_medium_radio"), HINT_MEDIUM,
                               appearance_capplet_get_widget (data, "hint_full_radio"),   HINT_FULL,
                               NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_rgb_image")),
                             MATECC_PIXMAP_DIR "/subpixel-rgb.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_bgr_image")),
                             MATECC_PIXMAP_DIR "/subpixel-bgr.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_vrgb_image")),
                             MATECC_PIXMAP_DIR "/subpixel-vrgb.png");
    gtk_image_set_from_file (GTK_IMAGE (appearance_capplet_get_widget (data, "subpixel_vbgr_image")),
                             MATECC_PIXMAP_DIR "/subpixel-vbgr.png");

    group = enum_group_create (FONT_RGBA_ORDER_KEY, rgba_order_enums, RGBA_RGB,
                               appearance_capplet_get_widget (data, "subpixel_rgb_radio"),  RGBA_RGB,
                               appearance_capplet_get_widget (data, "subpixel_bgr_radio"),  RGBA_BGR,
                               appearance_capplet_get_widget (data, "subpixel_vrgb_radio"), RGBA_VRGB,
                               appearance_capplet_get_widget (data, "subpixel_vbgr_radio"), RGBA_VBGR,
                               NULL);
    data->font_groups = g_slist_prepend (data->font_groups, group);

    g_signal_connect (G_OBJECT (data->font_details),
		      "response",
		      G_CALLBACK (cb_details_response), NULL);
    g_signal_connect (G_OBJECT (data->font_details),
		      "delete_event",
		      G_CALLBACK (gtk_true), NULL);
  }

  gtk_window_present (GTK_WINDOW (data->font_details));
}
#endif /* HAVE_XFT2 */

void font_init(AppearanceData* data)
{
	GObject* peditor;
	GtkWidget* widget;

	data->font_details = NULL;
	data->font_groups = NULL;

	mateconf_client_add_dir(data->client, "/desktop/mate/interface", MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_add_dir(data->client, "/apps/caja/preferences", MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_add_dir(data->client, MARCO_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
  
	#ifdef HAVE_XFT2
		mateconf_client_add_dir(data->client, FONT_RENDER_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	#endif /* HAVE_XFT2 */

	widget = appearance_capplet_get_widget(data, "application_font");
	peditor = mateconf_peditor_new_font(NULL, GTK_FONT_KEY, widget, "conv-from-widget-cb", application_font_to_mateconf, NULL);
	g_signal_connect_swapped(peditor, "value-changed", G_CALLBACK (application_font_changed), widget);
	application_font_changed(widget);

	peditor = mateconf_peditor_new_font(NULL, DOCUMENT_FONT_KEY, appearance_capplet_get_widget (data, "document_font"), NULL);

	peditor = mateconf_peditor_new_font(NULL, DESKTOP_FONT_KEY, appearance_capplet_get_widget (data, "desktop_font"), NULL);

	peditor = mateconf_peditor_new_font(NULL, WINDOW_TITLE_FONT_KEY, appearance_capplet_get_widget (data, "window_title_font"), NULL);

	peditor = mateconf_peditor_new_font(NULL, MONOSPACE_FONT_KEY, appearance_capplet_get_widget (data, "monospace_font"), NULL);

	mateconf_client_notify_add (data->client, WINDOW_TITLE_USES_SYSTEM_KEY, marco_changed, data, NULL, NULL);

	marco_titlebar_load_sensitivity(data);

	#ifdef HAVE_XFT2
		setup_font_pair(appearance_capplet_get_widget(data, "monochrome_radio"), appearance_capplet_get_widget (data, "monochrome_sample"), ANTIALIAS_NONE, HINT_FULL);
		setup_font_pair(appearance_capplet_get_widget(data, "best_shapes_radio"), appearance_capplet_get_widget (data, "best_shapes_sample"), ANTIALIAS_GRAYSCALE, HINT_MEDIUM);
		setup_font_pair(appearance_capplet_get_widget(data, "best_contrast_radio"), appearance_capplet_get_widget (data, "best_contrast_sample"), ANTIALIAS_GRAYSCALE, HINT_FULL);
		setup_font_pair(appearance_capplet_get_widget(data, "subpixel_radio"), appearance_capplet_get_widget (data, "subpixel_sample"), ANTIALIAS_RGBA, HINT_FULL);

		font_render_load (data->client);

		mateconf_client_notify_add (data->client, FONT_RENDER_DIR, font_render_changed, data->client, NULL, NULL);

		g_signal_connect (appearance_capplet_get_widget (data, "details_button"), "clicked", G_CALLBACK (cb_show_details), data);
	#else /* !HAVE_XFT2 */
		gtk_widget_hide (appearance_capplet_get_widget (data, "font_render_frame"));
	#endif /* HAVE_XFT2 */
}

void font_shutdown(AppearanceData* data)
{
	g_slist_foreach(data->font_groups, (GFunc) enum_group_destroy, NULL);
	g_slist_free(data->font_groups);
	g_slist_foreach(font_pairs, (GFunc) g_free, NULL);
	g_slist_free(font_pairs);
	g_free(old_font);
}
