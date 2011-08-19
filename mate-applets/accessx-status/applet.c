/* Keyboard Accessibility Status Applet
 * Copyright 2003, 2004 Sun Microsystems Inc.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mate-panel-applet-mateconf.h>
#include <X11/XKBlib.h>
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/keysymdef.h>
#include "applet.h"

static int xkb_base_event_type = 0;

static GtkIconSize icon_size_spec;

#define ALT_GRAPH_LED_MASK (0x10)

typedef struct
{
	char        *stock_id;
	char        *name;
	GtkStateType state;
	gboolean     wildcarded;
} AppletStockIcon;

static AppletStockIcon stock_icons [] = {
        { ACCESSX_APPLET, "ax-applet.png", GTK_STATE_NORMAL, True },
        { ACCESSX_BASE_ICON, "ax-key-base.png", GTK_STATE_NORMAL, True },
        { ACCESSX_BASE_ICON, "ax-key-none.png", GTK_STATE_INSENSITIVE, False },
        { ACCESSX_BASE_ICON, "ax-key-inverse.png", GTK_STATE_SELECTED, False },
        { ACCESSX_ACCEPT_BASE, "ax-key-yes.png", GTK_STATE_NORMAL, True },
        { ACCESSX_REJECT_BASE, "ax-key-no.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_BASE_ICON, "mousekeys-base.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_BUTTON_LEFT, "mousekeys-pressed-left.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_BUTTON_MIDDLE, "mousekeys-pressed-middle.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_BUTTON_RIGHT, "mousekeys-pressed-right.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_DOT_LEFT, "mousekeys-default-left.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_DOT_MIDDLE, "mousekeys-default-middle.png", GTK_STATE_NORMAL, True },
        { MOUSEKEYS_DOT_RIGHT, "mousekeys-default-right.png", GTK_STATE_NORMAL, True },
        { SHIFT_KEY_ICON, "sticky-shift-latched.png", GTK_STATE_NORMAL, False },
        { SHIFT_KEY_ICON, "sticky-shift-locked.png", GTK_STATE_SELECTED, False },
        { SHIFT_KEY_ICON, "sticky-shift-none.png", GTK_STATE_INSENSITIVE, True },
        { CONTROL_KEY_ICON, "sticky-ctrl-latched.png", GTK_STATE_NORMAL, False },
        { CONTROL_KEY_ICON, "sticky-ctrl-locked.png", GTK_STATE_SELECTED, False },
        { CONTROL_KEY_ICON, "sticky-ctrl-none.png", GTK_STATE_INSENSITIVE, True },
        { ALT_KEY_ICON, "sticky-alt-latched.png", GTK_STATE_NORMAL, False },
        { ALT_KEY_ICON, "sticky-alt-locked.png", GTK_STATE_SELECTED, False },
        { ALT_KEY_ICON, "sticky-alt-none.png", GTK_STATE_INSENSITIVE, True },
        { META_KEY_ICON, "sticky-meta-latched.png", GTK_STATE_NORMAL, False },
        { META_KEY_ICON, "sticky-meta-locked.png", GTK_STATE_SELECTED, False },
        { META_KEY_ICON, "sticky-meta-none.png", GTK_STATE_INSENSITIVE, True },
        { SUPER_KEY_ICON, "sticky-super-latched.png", GTK_STATE_NORMAL, False },
        { SUPER_KEY_ICON, "sticky-super-locked.png", GTK_STATE_SELECTED, False },
        { SUPER_KEY_ICON, "sticky-super-none.png", GTK_STATE_INSENSITIVE, True },
        { HYPER_KEY_ICON, "sticky-hyper-latched.png", GTK_STATE_NORMAL, False },
        { HYPER_KEY_ICON, "sticky-hyper-locked.png", GTK_STATE_SELECTED, False },
        { HYPER_KEY_ICON, "sticky-hyper-none.png", GTK_STATE_INSENSITIVE, True },
        { SLOWKEYS_IDLE_ICON, "ax-slowkeys.png", GTK_STATE_NORMAL, True },
        { SLOWKEYS_PENDING_ICON, "ax-slowkeys-pending.png", GTK_STATE_NORMAL, True },
        { SLOWKEYS_ACCEPT_ICON, "ax-slowkeys-yes.png", GTK_STATE_NORMAL, True },
        { SLOWKEYS_REJECT_ICON, "ax-slowkeys-no.png", GTK_STATE_NORMAL, True },
        { BOUNCEKEYS_ICON, "ax-bouncekeys.png", GTK_STATE_NORMAL, True }
};

typedef struct {
	unsigned int mask;
	GtkWidget   *indicator;
} ModifierStruct;

static ModifierStruct modifiers [] = {
	{ShiftMask, NULL},
	{ControlMask, NULL},
	{Mod1Mask, NULL},
	{Mod2Mask, NULL},
	{Mod3Mask, NULL},
	{Mod4Mask, NULL},
	{Mod5Mask, NULL}
};

typedef struct {
	unsigned int mask;
	gchar       *stock_id;
} ButtonIconStruct;

static ButtonIconStruct button_icons [] = {
	{Button1Mask, MOUSEKEYS_BUTTON_LEFT},
	{Button2Mask, MOUSEKEYS_BUTTON_MIDDLE},
	{Button3Mask, MOUSEKEYS_BUTTON_RIGHT}
};

static void popup_error_dialog (AccessxStatusApplet        *sapplet);

/* cribbed from geyes */
static void
about_cb (GtkAction           *action,
	  AccessxStatusApplet *sapplet)
{
        static const gchar *authors [] = {
		"Calum Benson <calum.benson@sun.com>",
		"Bill Haneman <bill.haneman@sun.com>",
		NULL
	};

	const gchar *documenters[] = {
	        "Bill Haneman <bill.haneman@sun.com>",
                "Sun MATE Documentation Team <gdocteam@sun.com>",
		NULL
	};
	
	gtk_show_about_dialog (NULL,
		"version",	VERSION,
		"comments",	_("Shows the state of AccessX features such "
				  "as latched modifiers"),
		"copyright",	"\xC2\xA9 2003 Sun Microsystems",
		"authors",	authors,
		"documenters",	documenters,
		"translator-credits",	_("translator-credits"),
		"logo-icon-name",	"ax-applet",
		NULL);
}

static void
help_cb (GtkAction           *action,
	 AccessxStatusApplet *sapplet)
{
	GError *error = NULL;

	gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)),
			"ghelp:accessx-status",
			gtk_get_current_event_time (),
			&error);

	if (error) { 
		GtkWidget *parent =
			gtk_widget_get_parent (GTK_WIDGET (sapplet->applet));

		GtkWidget *dialog = 
			gtk_message_dialog_new (GTK_WINDOW (parent),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						_("There was an error launching the help viewer: %s"), 
						error->message);

		g_signal_connect (G_OBJECT (dialog),
				  "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);

		gtk_window_set_screen (GTK_WINDOW (dialog),
				       gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)));
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show (dialog);
		g_error_free (error);
	}
}

static void
dialog_cb (GtkAction           *action,
	   AccessxStatusApplet *sapplet)
{
	GError *error = NULL;

	if (sapplet->error_type != ACCESSX_STATUS_ERROR_NONE) {
		popup_error_dialog (sapplet);
		return;
	}

	/* This is the old way of calling things, we try this just in case
	 * we're in a mixed-version enviroment. It has to be tried first, 
	 * because the new command doesn't fail in a way useful to
	 * gdk_spawn_command_line_on_screen and its error parameter. */
	gdk_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)),
					  "mate-accessibility-keyboard-properties", 
					  &error);

	if (error != NULL) {
		g_error_free (error);
		error = NULL;
	        gdk_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)),
						  "mate-keyboard-properties --a11y", 
						  &error);
	}

	if (error != NULL) {
		GtkWidget *dialog = 
			gtk_message_dialog_new (NULL,
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						_("There was an error launching the keyboard preferences dialog: %s"), 
						error->message);

		g_signal_connect (G_OBJECT (dialog),
				  "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);

		gtk_window_set_screen (GTK_WINDOW (dialog),
				       gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)));
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);
		g_error_free (error);
	}
}

static const GtkActionEntry accessx_status_applet_menu_actions [] = {
	{ "Dialog", GTK_STOCK_PROPERTIES, N_("_Keyboard Accessibility Preferences"),
	  NULL, NULL,
	  G_CALLBACK (dialog_cb) },
	{ "Help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

static XkbDescPtr 
accessx_status_applet_get_xkb_desc (AccessxStatusApplet *sapplet)
{
	Display *display;

	if (sapplet->xkb == NULL) {
		int ir, reason_return;
		char *display_name = getenv ("DISPLAY");
		display = XkbOpenDisplay (display_name,
					  &xkb_base_event_type,
					  &ir, NULL, NULL, 
					  &reason_return);
		g_assert (display); /* TODO: change error message below to something user-viewable */
		if (display == NULL)
			g_warning ("Xkb extension could not be initialized! (error code %x)", reason_return);
		else 
			sapplet->xkb = XkbGetMap (display, 
						  XkbAllComponentsMask,
						  XkbUseCoreKbd);
		g_assert (sapplet->xkb);
		if (sapplet->xkb == NULL)
			g_warning ("Xkb keyboard description not available!");
		sapplet->xkb_display = display;
	}
	return sapplet->xkb;
}

static gboolean
accessx_status_applet_xkb_select (AccessxStatusApplet *sapplet)
{
	int opcode_rtn, error_rtn;
	gboolean retval = FALSE;
	Display *display = NULL;
        GdkWindow *window;

        window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));
	g_assert (sapplet && sapplet->applet && window);
	display = GDK_WINDOW_XDISPLAY (window);
	g_assert (display);
	retval = XkbQueryExtension (display, &opcode_rtn, &xkb_base_event_type, 
				    &error_rtn, NULL, NULL);
	if (retval) {
		retval = XkbSelectEvents (display, XkbUseCoreKbd, 
					  XkbAllEventsMask, 
					  XkbAllEventsMask);
		sapplet->xkb = accessx_status_applet_get_xkb_desc (sapplet);
	} else {
		sapplet->error_type = ACCESSX_STATUS_ERROR_XKB_DISABLED;
	}

	return retval;
}

static void
accessx_status_applet_init_modifiers (AccessxStatusApplet *sapplet) 
{
	int i;
	unsigned int alt_mask, meta_mask, hyper_mask, super_mask, alt_gr_mask;
	alt_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Alt_L);
	meta_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Meta_L);

	g_assert (sapplet->meta_indicator);
	if (meta_mask && (meta_mask != alt_mask)) gtk_widget_show (sapplet->meta_indicator);
	else gtk_widget_hide (sapplet->meta_indicator);

	hyper_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Hyper_L);
	if (hyper_mask) gtk_widget_show (sapplet->hyper_indicator);
	else gtk_widget_hide (sapplet->hyper_indicator);

	super_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Super_L);
	if (super_mask) gtk_widget_show (sapplet->super_indicator);
	else gtk_widget_hide (sapplet->super_indicator);
	alt_gr_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Mode_switch) | 
	    XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Shift) |
	    XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Latch) | 
	    XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Lock);

	if (alt_gr_mask) gtk_widget_show (sapplet->alt_graph_indicator);
	else gtk_widget_hide (sapplet->alt_graph_indicator);

	for (i = 0; i < G_N_ELEMENTS (modifiers); ++i) {
		if (modifiers[i].mask == ShiftMask)
			modifiers[i].indicator = sapplet->shift_indicator;
		else if (modifiers[i].mask == ControlMask)
			modifiers[i].indicator = sapplet->ctrl_indicator;
		else if (modifiers[i].mask == alt_mask) 
			modifiers[i].indicator = sapplet->alt_indicator;
		else if (modifiers[i].mask == meta_mask)
			modifiers[i].indicator = sapplet->meta_indicator;
		else if (modifiers[i].mask == hyper_mask)
			modifiers[i].indicator = sapplet->hyper_indicator;
		else if (modifiers[i].mask == super_mask)
			modifiers[i].indicator = sapplet->super_indicator;
		else if (modifiers[i].mask == alt_gr_mask)
			modifiers[i].indicator = sapplet->alt_graph_indicator;
	}
}

static guint _sk_timeout = 0, _bk_timeout = 0;

static gboolean
timer_reset_slowkeys_image (gpointer user_data)
{
	GdkPixbuf *pixbuf = gtk_widget_render_icon (GTK_WIDGET (user_data),
						    SLOWKEYS_IDLE_ICON,
						    icon_size_spec,
						    NULL);
	
	gtk_image_set_from_pixbuf (GTK_IMAGE (user_data), pixbuf); 
	g_object_unref (pixbuf);

	return FALSE;
}

static gboolean
timer_reset_bouncekeys_image (gpointer user_data)
{
	GdkPixbuf *pixbuf = gtk_widget_render_icon (GTK_WIDGET (user_data),
						    BOUNCEKEYS_ICON,
						    icon_size_spec,
						    NULL);
	
	gtk_image_set_from_pixbuf (GTK_IMAGE (user_data), pixbuf); 
	g_object_unref (pixbuf);

	return FALSE;
}

static GdkPixbuf *
accessx_status_applet_get_glyph_pixbuf (AccessxStatusApplet *sapplet, 
					GtkWidget *widget,
					GdkPixbuf *base, 
					GdkColor  *fg,
					GdkColor  *bg,
					gchar *glyphstring)
{
	GdkPixbuf *glyph_pixbuf, *alpha_pixbuf;
	GdkPixmap *pixmap;
	PangoLayout *layout;
	PangoRectangle ink, logic;
	PangoContext *pango_context;
	GdkColormap *cmap;
	GdkGC       *gc;
	GdkVisual   *visual = gdk_visual_get_best ();
	gint w = gdk_pixbuf_get_width (base);
	gint h = gdk_pixbuf_get_height (base);
	pixmap = gdk_pixmap_new (NULL,
				 gdk_pixbuf_get_width (base),
				 gdk_pixbuf_get_height (base), visual->depth);
	pango_context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (pango_context);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text (layout, glyphstring, -1);
	gc = gdk_gc_new (GDK_DRAWABLE (pixmap));
	cmap = gdk_drawable_get_colormap (GDK_DRAWABLE (pixmap));
	if (!cmap) {
	  cmap = gdk_colormap_new (visual, FALSE);
	  gdk_drawable_set_colormap (GDK_DRAWABLE (pixmap), cmap);
	} else g_object_ref (cmap);
	gdk_colormap_alloc_color (cmap, fg, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, bg, FALSE, TRUE);
	gdk_gc_set_foreground (gc, bg);
	gdk_draw_rectangle (pixmap, gc, True, 0, 0, w, h);
	gdk_gc_set_foreground (gc, fg);
	gdk_gc_set_background (gc, bg);
	gdk_gc_set_function (gc, GDK_COPY);
	pango_layout_get_pixel_extents (layout, &ink, &logic);
	gdk_draw_layout (GDK_DRAWABLE (pixmap), 
			 gc, 
			 (w - ink.x - ink.width)/2, 
			 (h - ink.y - ink.height)/2,
			 layout);
	g_object_unref (layout);
	glyph_pixbuf = gdk_pixbuf_get_from_drawable (NULL, pixmap, 
						     cmap, 0, 0, 0, 0, w, h);
	g_object_unref (pixmap);
	alpha_pixbuf = gdk_pixbuf_add_alpha (glyph_pixbuf, TRUE, bg->red >> 8, bg->green >> 8, bg->blue >> 8);
	g_object_unref (G_OBJECT (glyph_pixbuf));
	g_object_unref (G_OBJECT (cmap));
	g_object_unref (G_OBJECT (gc));
	return alpha_pixbuf;
}

static GdkPixbuf *
accessx_status_applet_slowkeys_image (AccessxStatusApplet *sapplet, 
				      XkbAccessXNotifyEvent *event)
{
	GdkPixbuf *ret_pixbuf;
	GdkWindow *window;
	gboolean is_idle = TRUE;
	gchar *stock_id = SLOWKEYS_IDLE_ICON;
	GtkStyle *style = gtk_widget_get_style (GTK_WIDGET (sapplet->applet));
	GdkColor bg = style->bg[GTK_STATE_NORMAL];

	if (event != NULL) {
		is_idle = FALSE;
		switch (event->detail) {
		case XkbAXN_SKPress:
			stock_id = ACCESSX_BASE_ICON;
			if (_sk_timeout) {
				g_source_remove (_sk_timeout);
				_sk_timeout = 0;
			}
			break;
		case XkbAXN_SKAccept:
			stock_id = ACCESSX_ACCEPT_BASE;
			gdk_color_parse ("#009900", &bg);
			break;
		case XkbAXN_SKReject:
			stock_id = ACCESSX_REJECT_BASE;
			gdk_color_parse ("#990000", &bg);
			_sk_timeout = g_timeout_add_full (G_PRIORITY_HIGH_IDLE, 
							  MAX (event->sk_delay, 150),
							  timer_reset_slowkeys_image, 
							  sapplet->slowfoo, NULL);
			break;
		case XkbAXN_SKRelease:
		default:
			stock_id = SLOWKEYS_IDLE_ICON;
			is_idle = TRUE;
			break;
		}
	}
	ret_pixbuf = gtk_widget_render_icon (GTK_WIDGET (sapplet->applet),
					     stock_id,
					     icon_size_spec,
					     NULL);
	if (!is_idle) {
		GdkPixbuf *glyph_pixbuf, *tmp_pixbuf;
		GdkColor  fg;
		gchar * glyphstring = N_("a");
		tmp_pixbuf = ret_pixbuf;
		ret_pixbuf = gdk_pixbuf_copy (tmp_pixbuf);
		g_object_unref (tmp_pixbuf);

		window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));

		if (event && window) {
			KeySym keysym = XKeycodeToKeysym (
				GDK_WINDOW_XDISPLAY (window),
				event->keycode, 
				0); 
			glyphstring = XKeysymToString (keysym);
			if ((!g_utf8_validate (glyphstring, -1, NULL)) || 
			    (g_utf8_strlen (glyphstring, -1) > 1))
				glyphstring = "";
		}
		fg = style->fg[gtk_widget_get_state (GTK_WIDGET (sapplet->applet))];
		glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (sapplet, 
								       GTK_WIDGET (sapplet->applet),
								       ret_pixbuf, 
								       &fg,
								       &bg,
								       glyphstring);
		gdk_pixbuf_composite (glyph_pixbuf, ret_pixbuf, 0, 0, 
				      gdk_pixbuf_get_width (glyph_pixbuf),
				      gdk_pixbuf_get_height (glyph_pixbuf), 
				      0., 0., 1.0, 1.0, GDK_INTERP_NEAREST, 255);
		g_object_unref (glyph_pixbuf);
	}
	return ret_pixbuf;
}

static GdkPixbuf *
accessx_status_applet_bouncekeys_image (AccessxStatusApplet *sapplet, XkbAccessXNotifyEvent *event)
{
	GtkStyle  *style;
	GdkColor   fg, bg;
	GdkPixbuf *icon_base, *tmp_pixbuf;
	/* Note to translators: the first letter of the alphabet, not the indefinite article */
	gchar     *glyphstring = N_("a");
	gchar *stock_id = ACCESSX_BASE_ICON;

	g_assert (sapplet->applet);
	style = gtk_widget_get_style (GTK_WIDGET (sapplet->applet));
	fg = style->text[gtk_widget_get_state (GTK_WIDGET (sapplet->applet))];
	bg = style->base[GTK_STATE_NORMAL];

	if (event != NULL) {
		switch (event->detail) {
		case XkbAXN_BKReject:
			stock_id = SLOWKEYS_REJECT_ICON;
			_bk_timeout = g_timeout_add_full (G_PRIORITY_HIGH_IDLE, 
							  MAX (event->debounce_delay, 150),
							  timer_reset_bouncekeys_image, 
							  sapplet->bouncefoo, NULL); 
			break;
		default:
			stock_id = ACCESSX_BASE_ICON;
			break;
		}
	}
	tmp_pixbuf = gtk_widget_render_icon (GTK_WIDGET (sapplet->applet),
					     stock_id,
					     icon_size_spec,
					     NULL);
	if (tmp_pixbuf) {
		GdkPixbuf   *glyph_pixbuf;
		icon_base = gdk_pixbuf_copy (tmp_pixbuf);
		g_object_unref (tmp_pixbuf);
		glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (sapplet, 
								       GTK_WIDGET (sapplet->applet),
								       icon_base, 
								       &fg,
								       &bg,
								       glyphstring);
		gdk_pixbuf_composite (glyph_pixbuf, icon_base, 2, 2, 
				      gdk_pixbuf_get_width (glyph_pixbuf) - 2,
				      gdk_pixbuf_get_height (glyph_pixbuf) - 2, 
				      -2., -2., 1.0, 1.0, GDK_INTERP_NEAREST, 96);
		gdk_pixbuf_composite (glyph_pixbuf, icon_base, 1, 1, 
				      gdk_pixbuf_get_width (glyph_pixbuf) - 1,
				      gdk_pixbuf_get_height (glyph_pixbuf) - 1, 
				      1., 1., 1.0, 1.0, GDK_INTERP_NEAREST, 255);

		g_object_unref (glyph_pixbuf);
	}
	return icon_base;
}

static GdkPixbuf *
accessx_status_applet_mousekeys_image (AccessxStatusApplet *sapplet, XkbStateNotifyEvent *event)
{
	GdkPixbuf  *mouse_pixbuf = NULL, *button_pixbuf, *dot_pixbuf, *tmp_pixbuf;
	gchar *which_dot = MOUSEKEYS_DOT_LEFT;
	tmp_pixbuf = gtk_widget_render_icon (GTK_WIDGET (sapplet->applet),
					       MOUSEKEYS_BASE_ICON,
					       icon_size_spec, 
					       NULL);
	mouse_pixbuf = gdk_pixbuf_copy (tmp_pixbuf);
	g_object_unref (tmp_pixbuf);
	/* composite in the buttons */
	if (mouse_pixbuf && event && event->ptr_buttons) {
		gint i;
		for (i = 0; i < G_N_ELEMENTS (button_icons); ++i) {
			if (event->ptr_buttons & button_icons[i].mask) {
				button_pixbuf = gtk_widget_render_icon (GTK_WIDGET (sapplet->applet),
									button_icons[i].stock_id,
									icon_size_spec,
									NULL);
				gdk_pixbuf_composite (button_pixbuf, mouse_pixbuf, 0, 0, 
						      gdk_pixbuf_get_width (button_pixbuf),
						      gdk_pixbuf_get_height (button_pixbuf), 
						      0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
			}
		}
	}
	if (event) {
		switch (sapplet->xkb->ctrls->mk_dflt_btn) {
		case Button2:
			which_dot = MOUSEKEYS_DOT_MIDDLE;
			break;
		case Button3: 
			which_dot = MOUSEKEYS_DOT_RIGHT;
			break;
		case Button1:
		default:
			which_dot = MOUSEKEYS_DOT_LEFT;
			break;
		}
	}
	dot_pixbuf = gtk_widget_render_icon (GTK_WIDGET (sapplet->applet),
					     which_dot,
					     icon_size_spec,
					     NULL);

	gdk_pixbuf_composite (dot_pixbuf, mouse_pixbuf, 0, 0, 
			      gdk_pixbuf_get_width (dot_pixbuf),
			      gdk_pixbuf_get_height (dot_pixbuf), 
			      0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
	return mouse_pixbuf;
}


static void
accessx_status_applet_update (AccessxStatusApplet *sapplet, 
			      AccessxStatusNotifyType notify_type,
			      XkbEvent *event)
{
	GtkWindow * window;
	gint i;

	window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));

	if (notify_type & ACCESSX_STATUS_MODIFIERS) {
		unsigned int locked_mods = 0, latched_mods = 0;
		if (event != NULL) {
			locked_mods = event->state.locked_mods;
			latched_mods = event->state.latched_mods;
		}
		else if (sapplet->applet && window) {
			XkbStateRec state;			
			XkbGetState (GDK_WINDOW_XDISPLAY (window), 
				     XkbUseCoreKbd, &state); 
			locked_mods = state.locked_mods;
			latched_mods = state.latched_mods;
		}
		/* determine which modifiers are locked, and set state accordingly */
		for (i = 0; i < G_N_ELEMENTS (modifiers); ++i) {
			if (modifiers[i].indicator != NULL && modifiers[i].mask) {
				if (locked_mods & modifiers[i].mask) {
					gtk_widget_set_sensitive (modifiers[i].indicator, TRUE);
					gtk_widget_set_state (modifiers[i].indicator, GTK_STATE_SELECTED);
				}
				else if (latched_mods & modifiers[i].mask) {
					gtk_widget_set_sensitive (modifiers[i].indicator, TRUE);
					gtk_widget_set_state (modifiers[i].indicator, GTK_STATE_NORMAL);
				}
				else {
					gtk_widget_set_sensitive (modifiers[i].indicator, FALSE);
				}
			}
		}
	}
	if ((notify_type & ACCESSX_STATUS_SLOWKEYS) && (event != NULL)) {
		GdkPixbuf *pixbuf = accessx_status_applet_slowkeys_image (
			sapplet, &event->accessx);
		gtk_image_set_from_pixbuf (GTK_IMAGE (sapplet->slowfoo), pixbuf);
		g_object_unref (pixbuf);
	}
	if ((notify_type & ACCESSX_STATUS_BOUNCEKEYS) && (event != NULL)) {
		GdkPixbuf *pixbuf = accessx_status_applet_bouncekeys_image (
			sapplet, &event->accessx);
		gtk_image_set_from_pixbuf (GTK_IMAGE (sapplet->bouncefoo), pixbuf);
		g_object_unref (pixbuf);
	}
	if (notify_type & ACCESSX_STATUS_MOUSEKEYS) {
		GdkPixbuf *pixbuf = accessx_status_applet_mousekeys_image (
			sapplet, &event->state);
		gtk_image_set_from_pixbuf (GTK_IMAGE (sapplet->mousefoo),  pixbuf);
		g_object_unref (pixbuf);
	}

	if (notify_type & ACCESSX_STATUS_ENABLED) {
		/* Update the visibility of widgets in the box */
		XkbGetControls (GDK_WINDOW_XDISPLAY (window), 
				/* XkbMouseKeysMask | XkbStickyKeysMask | 
				   XkbSlowKeysMask | XkbBounceKeysMask, */
				XkbAllControlsMask,
				sapplet->xkb);
		if (!(sapplet->xkb->ctrls->enabled_ctrls & 
		      (XkbMouseKeysMask | XkbStickyKeysMask | XkbSlowKeysMask | XkbBounceKeysMask)))
			gtk_widget_show (sapplet->idlefoo);
		else gtk_widget_hide (sapplet->idlefoo);
		if (sapplet->xkb->ctrls->enabled_ctrls & XkbMouseKeysMask) 
			gtk_widget_show (sapplet->mousefoo);
		else
			gtk_widget_hide (sapplet->mousefoo);
		if (sapplet->xkb->ctrls->enabled_ctrls & XkbStickyKeysMask) 
			gtk_widget_show (sapplet->stickyfoo);
		else
			gtk_widget_hide (sapplet->stickyfoo);
		if (sapplet->xkb->ctrls->enabled_ctrls & XkbSlowKeysMask) 
			gtk_widget_show (sapplet->slowfoo);
		else
			gtk_widget_hide (sapplet->slowfoo);
		if (sapplet->xkb->ctrls->enabled_ctrls & XkbBounceKeysMask) 
			gtk_widget_show (sapplet->bouncefoo);
		else
			gtk_widget_hide (sapplet->bouncefoo);
	}
	return;
}

static void
accessx_status_applet_notify_xkb_ax (AccessxStatusApplet *sapplet, 
				     XkbAccessXNotifyEvent *event) 
{
	AccessxStatusNotifyType notify_mask = 0;

	switch (event->detail) {
	case XkbAXN_SKPress:
	case XkbAXN_SKAccept:
	case XkbAXN_SKRelease:
	case XkbAXN_SKReject:
		notify_mask |= ACCESSX_STATUS_SLOWKEYS;
		break;
	case XkbAXN_BKAccept:
	case XkbAXN_BKReject:
		notify_mask |= ACCESSX_STATUS_BOUNCEKEYS;
		break;
	case XkbAXN_AXKWarning:
		break;
	default:
		break;
	}
	accessx_status_applet_update (sapplet, notify_mask, (XkbEvent *) event);
}

static void
accessx_status_applet_notify_xkb_state (AccessxStatusApplet *sapplet, 
					XkbStateNotifyEvent *event) 
{
	AccessxStatusNotifyType notify_mask = 0;

	if (event->changed & XkbPointerButtonMask) {
		notify_mask |= ACCESSX_STATUS_MOUSEKEYS;
	} 
	if (event->changed & (XkbModifierLatchMask | XkbModifierLockMask)) {
		notify_mask |= ACCESSX_STATUS_MODIFIERS;
	}
	accessx_status_applet_update (sapplet, notify_mask, (XkbEvent *) event);
}

static void
accessx_status_applet_notify_xkb_device (AccessxStatusApplet *sapplet, 
					 XkbExtensionDeviceNotifyEvent *event) 
{
	if (event->reason == XkbXI_IndicatorStateMask) {
		if (event->led_state &= ALT_GRAPH_LED_MASK) {
			gtk_widget_set_sensitive (sapplet->alt_graph_indicator, TRUE);
			gtk_widget_set_state (sapplet->alt_graph_indicator, GTK_STATE_NORMAL);
		}
		else {
			gtk_widget_set_sensitive (sapplet->alt_graph_indicator, FALSE);
		}
	}
}

static void
accessx_status_applet_notify_xkb_controls (AccessxStatusApplet *sapplet, 
					   XkbControlsNotifyEvent *event) 
{
	unsigned int mask = XkbStickyKeysMask | XkbSlowKeysMask | XkbBounceKeysMask | XkbMouseKeysMask;
	unsigned int notify_mask = 0;
	XkbGetControls (sapplet->xkb_display, 
			XkbMouseKeysMask,
			sapplet->xkb);
	if (event->enabled_ctrl_changes & mask)
		notify_mask = ACCESSX_STATUS_ENABLED;
	if (event->changed_ctrls & XkbMouseKeysMask)
		notify_mask |= ACCESSX_STATUS_MOUSEKEYS;
	if (notify_mask)
		accessx_status_applet_update (sapplet, 
					      notify_mask, 
					      (XkbEvent *) event);
}

static void
accessx_status_applet_notify_xkb_event (AccessxStatusApplet *sapplet,
					XkbEvent *event)
{
	switch (event->any.xkb_type) {
	case XkbStateNotify:
		accessx_status_applet_notify_xkb_state (sapplet, &event->state);
		break;
	case XkbAccessXNotify:
		accessx_status_applet_notify_xkb_ax (sapplet, &event->accessx);
		break;
	case XkbControlsNotify:
		accessx_status_applet_notify_xkb_controls (sapplet, &event->ctrls);
		break;
	case XkbExtensionDeviceNotify:
		/* This is a hack around the fact that XFree86's XKB doesn't give AltGr notifications */
		accessx_status_applet_notify_xkb_device (sapplet, &event->device);
		break;
	default:
		break;
	}
}

static GdkFilterReturn
accessx_status_xkb_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;
	XkbEvent *xevent = gdk_xevent;

	if (xevent->any.type == xkb_base_event_type)
	{
		accessx_status_applet_notify_xkb_event (sapplet, xevent);
	}
	return GDK_FILTER_CONTINUE;
}

static GtkIconSet *
accessx_status_applet_altgraph_icon_set (AccessxStatusApplet *sapplet, GtkWidget *widget)
{
	GtkIconSet  *icon_set = gtk_icon_set_new ();
	gint         i;
	GtkStateType states[3] = {GTK_STATE_NORMAL, GTK_STATE_INSENSITIVE, GTK_STATE_SELECTED};
	GtkStyle    *style = gtk_widget_get_style (widget);
	GdkPixbuf   *icon_base;

	gtk_widget_set_sensitive (widget, TRUE);
	for (i = 0; i < 3; ++i) {
		int alpha;
		GdkColor *fg, *bg;
		GtkIconSource *source = gtk_icon_source_new ();
		GdkPixbuf *pixbuf, *glyph_pixbuf;
		gboolean wildcarded = FALSE;
		fg = &style->text[states[i]];
		bg = &style->white;
		switch (states[i]) {
		case GTK_STATE_NORMAL:
			alpha = 255;
			gtk_widget_set_sensitive (widget, TRUE);
			gtk_widget_set_state (widget, GTK_STATE_NORMAL);
			break;
		case GTK_STATE_SELECTED:
		    /* FIXME: should use text/base here, for selected ? */
			fg = &style->white; 
			bg = &style->black;
			alpha = 255;
			gtk_widget_set_sensitive (widget, TRUE);
			gtk_widget_set_state (widget, GTK_STATE_SELECTED);
			break;
		case GTK_STATE_INSENSITIVE:
		default:
			alpha = 63;
			gtk_widget_set_sensitive (widget, FALSE);
			wildcarded = TRUE;
			break;
		}
		icon_base = gtk_widget_render_icon (widget,
			ACCESSX_BASE_ICON, icon_size_spec, NULL);
		pixbuf = gdk_pixbuf_copy (icon_base);
		g_object_unref (icon_base);
	       /* 
		* should be N_("ae"));
		* need en_ locale for this.
		*/
	       /* 
		* Translators: substitute an easily-recognized single glyph 
		* from Level 2, i.e. an AltGraph character from a common keyboard 
		* in your locale. 
		*/
		glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (sapplet, 
								       widget,
								       pixbuf, 
								       fg, 
								       bg,
								       ("æ"));
		gdk_pixbuf_composite (glyph_pixbuf, pixbuf, 0, 0, 
				      gdk_pixbuf_get_width (glyph_pixbuf),
				      gdk_pixbuf_get_height (glyph_pixbuf),
				      0., 0., 1.0, 1.0, GDK_INTERP_NEAREST, alpha);
		g_object_unref (glyph_pixbuf);
		gtk_icon_source_set_pixbuf (source, pixbuf);
		gtk_icon_source_set_state (source, states[i]);
		gtk_icon_source_set_state_wildcarded (source, wildcarded);
		gtk_icon_set_add_source (icon_set, source);
		gtk_icon_source_free (source);
	}
	/* we mucked about with the box's state to create the icons; restore it to normal */
	gtk_widget_set_state (widget, GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (widget, TRUE);
	return icon_set;
}

static void
accessx_applet_add_stock_icons (AccessxStatusApplet *sapplet, GtkWidget *widget)
{
	GtkIconFactory *factory = gtk_icon_factory_new ();
        gint            i = 0;
        GtkIconSet     *icon_set;
                                                                                
	gtk_icon_factory_add_default (factory);
       
        while (i <  G_N_ELEMENTS (stock_icons)) {
		gchar *set_name = stock_icons[i].stock_id;
                icon_set = gtk_icon_set_new ();
		do {
			char *filename;
			GtkIconSource *source = gtk_icon_source_new ();
			filename = g_build_filename (ACCESSX_PIXMAPS_DIR,
						     stock_icons[i].name,
						     NULL);
			if (g_file_test (filename, G_FILE_TEST_EXISTS) &&
			    g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
				gtk_icon_source_set_filename (source, filename);
			} else {
				GtkIconSet *default_set = 
					gtk_icon_factory_lookup_default (GTK_STOCK_MISSING_IMAGE);
				gtk_icon_source_set_pixbuf (source,
							    gtk_icon_set_render_icon (
								    default_set, 
								    gtk_widget_get_style (widget),
								    GTK_TEXT_DIR_NONE,
								    GTK_STATE_NORMAL,
								    icon_size_spec,
								    widget,
								    NULL));
			}
			g_free (filename);
			gtk_icon_source_set_state (source, stock_icons[i].state);
			gtk_icon_source_set_state_wildcarded (source, stock_icons[i].wildcarded);
			gtk_icon_set_add_source (icon_set, source);
			gtk_icon_source_free (source);
			++i;
		} while (set_name == stock_icons[i].stock_id);
		gtk_icon_factory_add (factory, set_name, icon_set);
		gtk_icon_set_unref (icon_set);
	}
	/* now create the stock icons for AltGr, which are internationalized */
	icon_set = accessx_status_applet_altgraph_icon_set (sapplet, widget);
	gtk_icon_factory_add (factory, ALTGRAPH_KEY_ICON, icon_set);
	gtk_icon_set_unref (icon_set);
	sapplet->icon_factory = factory;
}

static void
accessx_status_applet_reparent_widget (GtkWidget *widget, GtkContainer *container) 
{
	if (widget) {
		if (gtk_widget_get_parent (widget)) {
			g_object_ref (G_OBJECT (widget));
			gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (widget)), widget);
		}
		gtk_container_add (container, widget);
	}
}

static void
accessx_status_applet_layout_box (AccessxStatusApplet *sapplet, GtkWidget *box, GtkWidget *stickyfoo) 
{
        AtkObject *atko;

	accessx_status_applet_reparent_widget (sapplet->shift_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->ctrl_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->alt_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->meta_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->hyper_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->super_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->alt_graph_indicator, GTK_CONTAINER (stickyfoo));
	accessx_status_applet_reparent_widget (sapplet->idlefoo, GTK_CONTAINER (box));
	accessx_status_applet_reparent_widget (sapplet->mousefoo, GTK_CONTAINER (box));
	accessx_status_applet_reparent_widget (stickyfoo, GTK_CONTAINER (box));
	accessx_status_applet_reparent_widget (sapplet->slowfoo, GTK_CONTAINER (box));
	accessx_status_applet_reparent_widget (sapplet->bouncefoo, GTK_CONTAINER (box));

	if (sapplet->stickyfoo) {
		gtk_widget_destroy (sapplet->stickyfoo); 
	}

	if (sapplet->box) {
		gtk_container_remove (GTK_CONTAINER (sapplet->applet), sapplet->box);
	}

	gtk_container_add (GTK_CONTAINER (sapplet->applet), box);
	sapplet->stickyfoo = stickyfoo;
	sapplet->box = box;

	atko = gtk_widget_get_accessible (sapplet->box);
	atk_object_set_name (atko, _("AccessX Status"));
	atk_object_set_description (atko, _("Shows keyboard status when accessibility features are used."));

	gtk_widget_show (sapplet->box);
	gtk_widget_show (GTK_WIDGET (sapplet->applet));

	if (gtk_widget_get_realized (sapplet->box) &&
            sapplet->initialized)
		accessx_status_applet_update (sapplet, ACCESSX_STATUS_ALL, NULL);
}

static void 
disable_applet (AccessxStatusApplet* sapplet)
{
	gtk_widget_hide (sapplet->meta_indicator);
	gtk_widget_hide (sapplet->hyper_indicator);
	gtk_widget_hide (sapplet->super_indicator);
	gtk_widget_hide (sapplet->alt_graph_indicator);
	gtk_widget_hide (sapplet->shift_indicator);
	gtk_widget_hide (sapplet->ctrl_indicator);
	gtk_widget_hide (sapplet->alt_indicator);
	gtk_widget_hide (sapplet->meta_indicator);
	gtk_widget_hide (sapplet->mousefoo);
	gtk_widget_hide (sapplet->stickyfoo);
	gtk_widget_hide (sapplet->slowfoo);
	gtk_widget_hide (sapplet->bouncefoo);
}

static void 
popup_error_dialog (AccessxStatusApplet* sapplet)
{
	GtkWidget *dialog;
	gchar *error_txt;

	switch (sapplet->error_type) {
		case ACCESSX_STATUS_ERROR_XKB_DISABLED : 
			error_txt = g_strdup (_("XKB Extension is not enabled"));
			break;

		case ACCESSX_STATUS_ERROR_UNKNOWN :

		default	: error_txt = g_strdup (_("Unknown error"));
			  break;				
	}

	dialog = gtk_message_dialog_new (NULL,
				   	 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("Error: %s"),
					 error_txt);

	g_signal_connect (G_OBJECT (dialog),
			  "response",
			  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_window_set_screen (GTK_WINDOW (dialog),
	gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)));

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_widget_show (dialog);
	g_free (error_txt);
}

static AccessxStatusApplet *
create_applet (MatePanelApplet *applet)
{
	AccessxStatusApplet *sapplet = g_new0 (AccessxStatusApplet, 1);
	GtkWidget           *box, *stickyfoo;
	AtkObject           *atko;
	GdkPixbuf	    *pixbuf;
        gint                 large_toolbar_pixels;

	g_set_application_name (_("AccessX Status"));

	sapplet->xkb = NULL;
	sapplet->xkb_display = NULL;
	sapplet->box = NULL;
	sapplet->initialized = False; /* there must be a better way */
	sapplet->error_type = ACCESSX_STATUS_ERROR_NONE;
	sapplet->applet = applet;
	mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	sapplet->orient = mate_panel_applet_get_orient (applet);
	if (sapplet->orient == MATE_PANEL_APPLET_ORIENT_LEFT || 
	    sapplet->orient == MATE_PANEL_APPLET_ORIENT_RIGHT) {
		box = gtk_vbox_new (FALSE, 0); 
		stickyfoo = gtk_vbox_new (TRUE, 0);
	}
	else {
		box = gtk_hbox_new (FALSE, 0);
		stickyfoo = gtk_hbox_new (TRUE, 0);
	}
	large_toolbar_pixels = 24; /* FIXME */
	if (mate_panel_applet_get_size (sapplet->applet) >= large_toolbar_pixels)
		icon_size_spec = GTK_ICON_SIZE_LARGE_TOOLBAR;       
        else 
		icon_size_spec = GTK_ICON_SIZE_SMALL_TOOLBAR;
  
	accessx_applet_add_stock_icons (sapplet, box);
	pixbuf = accessx_status_applet_mousekeys_image (sapplet, NULL);
	sapplet->mousefoo = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_widget_hide (sapplet->mousefoo);

	sapplet->shift_indicator = 
		gtk_image_new_from_stock (SHIFT_KEY_ICON, icon_size_spec); 
	gtk_widget_hide (sapplet->mousefoo);
	sapplet->ctrl_indicator = 
		gtk_image_new_from_stock (CONTROL_KEY_ICON, icon_size_spec); 
	sapplet->alt_indicator = 
		gtk_image_new_from_stock (ALT_KEY_ICON, icon_size_spec); 
	sapplet->meta_indicator =
		gtk_image_new_from_stock (META_KEY_ICON, icon_size_spec); 
	gtk_widget_set_sensitive (sapplet->meta_indicator, FALSE);
	gtk_widget_hide (sapplet->meta_indicator);
	sapplet->hyper_indicator =
		gtk_image_new_from_stock (HYPER_KEY_ICON, icon_size_spec); 
	gtk_widget_set_sensitive (sapplet->hyper_indicator, FALSE);
	gtk_widget_hide (sapplet->hyper_indicator);
	sapplet->super_indicator =
		gtk_image_new_from_stock (SUPER_KEY_ICON, icon_size_spec); 
	gtk_widget_set_sensitive (sapplet->super_indicator, FALSE);
	gtk_widget_hide (sapplet->super_indicator);
	sapplet->alt_graph_indicator =
		gtk_image_new_from_stock (ALTGRAPH_KEY_ICON, icon_size_spec);
	gtk_widget_set_sensitive (sapplet->alt_graph_indicator, FALSE);

	pixbuf = accessx_status_applet_slowkeys_image (sapplet, NULL);
	sapplet->slowfoo = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_widget_hide (sapplet->slowfoo);

	pixbuf = accessx_status_applet_bouncekeys_image (sapplet, NULL);
	sapplet->bouncefoo = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_widget_hide (sapplet->bouncefoo);

	sapplet->idlefoo = gtk_image_new_from_stock (ACCESSX_APPLET, icon_size_spec);
	gtk_widget_show (sapplet->slowfoo);

	accessx_status_applet_layout_box (sapplet, box, stickyfoo);
	atko = gtk_widget_get_accessible (GTK_WIDGET (sapplet->applet));
	atk_object_set_name (atko, _("AccessX Status"));
	atk_object_set_description (atko, _("Shows keyboard status when accessibility features are used."));
	return sapplet;
}

static void
accessx_status_applet_destroy (GtkWidget *widget, gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;
	/* do we need to free the icon factory ? */

	gdk_window_remove_filter (NULL, accessx_status_xkb_filter, sapplet);

	if (sapplet->xkb)
		XkbFreeKeyboard (sapplet->xkb, 0, True);
	if (sapplet->xkb_display) 
		XCloseDisplay (sapplet->xkb_display);
}

static void
accessx_status_applet_reorient (GtkWidget *widget, MatePanelAppletOrient o, gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;
	GtkWidget           *box, *stickyfoo;

	sapplet->orient = o;

	if (o == MATE_PANEL_APPLET_ORIENT_LEFT || 
	    o == MATE_PANEL_APPLET_ORIENT_RIGHT) {
		box = gtk_vbox_new (FALSE, 0); 
		stickyfoo = gtk_vbox_new (TRUE, 0);
	}
	else {
		box = gtk_hbox_new (FALSE, 0);
		stickyfoo = gtk_hbox_new (TRUE, 0);
	}
	accessx_status_applet_layout_box (sapplet, box, stickyfoo);
}

static void
accessx_status_applet_resize (GtkWidget *widget, int size, gpointer user_data)
{
	; /* TODO: either rescale icons to fit panel, or tile them when possible */
}

static void
accessx_status_applet_background (MatePanelApplet *a,
				  MatePanelAppletBackgroundType type,
				  GdkColor *color,
				  GdkPixmap *pixmap,
				  gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;

        GtkRcStyle *rc_style;
        GtkStyle *style;

        /* reset style */
        gtk_widget_set_style (GTK_WIDGET (sapplet->applet), NULL);
        rc_style = gtk_rc_style_new ();
        gtk_widget_modify_style (GTK_WIDGET (sapplet->applet), rc_style);
        g_object_unref (rc_style);

        switch (type) {
                case PANEL_COLOR_BACKGROUND:
                        gtk_widget_modify_bg (GTK_WIDGET (sapplet->applet),
                                        GTK_STATE_NORMAL, color);
                        break;

                case PANEL_PIXMAP_BACKGROUND:
                        style = gtk_style_copy (
                                        gtk_widget_get_style (GTK_WIDGET (sapplet->applet)));
                        if (style->bg_pixmap[GTK_STATE_NORMAL])
                                g_object_unref
                                        (style->bg_pixmap[GTK_STATE_NORMAL]);
                        style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref
                                (pixmap);
                        gtk_widget_set_style (GTK_WIDGET (sapplet->applet),
                                        style);
                        g_object_unref (style);
                        break;

                case PANEL_NO_BACKGROUND:
                default:
                        break;
        }
}

static gboolean
button_press_cb (GtkWidget *widget, GdkEventButton *event, AccessxStatusApplet *sapplet)
{
	if (event->button == 1 && event->type == GDK_BUTTON_PRESS) 
		dialog_cb (NULL, sapplet);

	return FALSE;
}

static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, AccessxStatusApplet *sapplet)
{
	switch (event->keyval) {
	case GDK_KP_Enter:
	case GDK_ISO_Enter:
	case GDK_3270_Enter:
	case GDK_Return:
	case GDK_space:
	case GDK_KP_Space:
		dialog_cb (NULL, sapplet);
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

static gboolean
accessx_status_applet_reset (gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;
	g_assert (sapplet->applet);
	accessx_status_applet_reorient (GTK_WIDGET (sapplet->applet), 
					mate_panel_applet_get_orient (sapplet->applet), sapplet);
	return FALSE;
}

static gboolean 
accessx_status_applet_initialize (AccessxStatusApplet *sapplet)
{
	if (!sapplet->initialized) {
		sapplet->initialized = True;
		if (!accessx_status_applet_xkb_select (sapplet)) {
			disable_applet (sapplet);
			popup_error_dialog (sapplet);
			return FALSE ;
		}
		gdk_window_add_filter (NULL, accessx_status_xkb_filter, sapplet);
	}
	accessx_status_applet_init_modifiers (sapplet);
	accessx_status_applet_update (sapplet, ACCESSX_STATUS_ALL, NULL);

	return TRUE;
}

static void
accessx_status_applet_realize (GtkWidget *widget, gpointer user_data)
{
	AccessxStatusApplet *sapplet = user_data;

	if (!accessx_status_applet_initialize (sapplet))
		return;

	g_idle_add (accessx_status_applet_reset, sapplet);
	return;
}

static gboolean
accessx_status_applet_fill (MatePanelApplet *applet)
{
	AccessxStatusApplet *sapplet;
	AtkObject           *atk_object;
	GtkActionGroup      *action_group;
	gchar               *ui_path;
	gboolean was_realized = FALSE;

	sapplet = create_applet (applet);

	if (!gtk_widget_get_realized (sapplet->box)) {
		g_signal_connect_after (G_OBJECT (sapplet->box), 
					"realize", G_CALLBACK (accessx_status_applet_realize), 
					sapplet);
	} else {
		accessx_status_applet_initialize (sapplet);
		was_realized = TRUE;
	}

	g_object_connect (sapplet->applet,
			  "signal::destroy", accessx_status_applet_destroy, sapplet,
			  "signal::change_orient", accessx_status_applet_reorient, sapplet,
			  "signal::change_size", accessx_status_applet_resize, sapplet,
			  "signal::change_background", accessx_status_applet_background, sapplet,
			  NULL);
			  
	g_signal_connect (sapplet->applet, "button_press_event",
				   G_CALLBACK (button_press_cb), sapplet);
	g_signal_connect (sapplet->applet, "key_press_event",
				   G_CALLBACK (key_press_cb), sapplet);				   

	action_group = gtk_action_group_new ("Accessx Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      accessx_status_applet_menu_actions,
				      G_N_ELEMENTS (accessx_status_applet_menu_actions),
				      sapplet);
	ui_path = g_build_filename (ACCESSX_MENU_UI_DIR, "accessx-status-applet-menu.xml", NULL);
	mate_panel_applet_setup_menu_from_file (sapplet->applet, ui_path, action_group);
	g_free (ui_path);

	if (mate_panel_applet_get_locked_down (sapplet->applet)) {
		GtkAction *action;

		action = gtk_action_group_get_action (action_group, "Dialog");
		gtk_action_set_visible (action, FALSE);
	}
	g_object_unref (action_group);

	gtk_widget_set_tooltip_text (GTK_WIDGET (sapplet->applet), _("Keyboard Accessibility Status"));

	atk_object = gtk_widget_get_accessible (GTK_WIDGET (sapplet->applet));
	atk_object_set_name (atk_object, _("AccessX Status"));
	atk_object_set_description (atk_object,
				    _("Displays current state of keyboard accessibility features"));
	gtk_widget_show_all (GTK_WIDGET (sapplet->applet));
	if (was_realized) {
		accessx_status_applet_reset (sapplet);
	}

	return TRUE;
}

static gboolean
accessx_status_applet_factory (MatePanelApplet *applet,
			       const gchar *iid,
			       gpointer     data)
{
	gboolean retval = FALSE;
	if (!strcmp (iid, "AccessxStatusApplet"))
		retval = accessx_status_applet_fill (applet);
	return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("AccessxStatusAppletFactory",
				  PANEL_TYPE_APPLET,
				  "accessx-status",
				  accessx_status_applet_factory,
				  NULL)
