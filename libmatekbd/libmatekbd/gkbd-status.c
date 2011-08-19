/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
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

#include <memory.h>

#include <cairo.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include <gkbd-status.h>

#include <gkbd-desktop-config.h>
#include <gkbd-indicator-config.h>

typedef struct _gki_globals {
	XklEngine *engine;
	XklConfigRegistry *registry;

	GkbdDesktopConfig cfg;
	GkbdIndicatorConfig ind_cfg;
	GkbdKeyboardConfig kbd_cfg;

	const gchar *tooltips_format;
	gchar **full_group_names;
	gchar **short_group_names;

	gint current_width;
	gint current_height;
	int real_width;

	GSList *icons;		/* list of GdkPixbuf */
	GSList *widget_instances;	/* list of GkbdStatus */
	gulong state_changed_handler;
	gulong config_changed_handler;
} gki_globals;

static gchar *settings_signal_names[] = {
	"notify::gtk-theme-name",
	"notify::gtk-key-theme-name",
	"notify::gtk-font-name",
	"notify::font-options",
};

struct _GkbdStatusPrivate {
	gdouble angle;
	gulong settings_signal_handlers[sizeof (settings_signal_names) /
					sizeof (settings_signal_names[0])];
};

/* one instance for ALL widgets */
static gki_globals globals;

#define ForAllIndicators() \
	{ \
		GSList* cur; \
		for (cur = globals.widget_instances; cur != NULL; cur = cur->next) { \
			GkbdStatus * gki = (GkbdStatus*)cur->data;
#define NextIndicator() \
		} \
	}

G_DEFINE_TYPE (GkbdStatus, gkbd_status, GTK_TYPE_STATUS_ICON)
static void
gkbd_status_global_init (void);
static void
gkbd_status_global_term (void);
static GdkPixbuf *
gkbd_status_prepare_drawing (GkbdStatus * gki, int group);
static void
gkbd_status_set_current_page_for_group (GkbdStatus * gki, int group);
static void
gkbd_status_set_current_page (GkbdStatus * gki);
static void
gkbd_status_global_cleanup (GkbdStatus * gki);
static void
gkbd_status_global_fill (GkbdStatus * gki);
static void
gkbd_status_set_tooltips (GkbdStatus * gki, const char *str);

void
gkbd_status_set_tooltips (GkbdStatus * gki, const char *str)
{
	g_assert (str == NULL || g_utf8_validate (str, -1, NULL));

	gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (gki), str);
}

void
gkbd_status_global_cleanup (GkbdStatus * gki)
{
	while (globals.icons) {
		if (globals.icons->data)
			g_object_unref (G_OBJECT (globals.icons->data));
		globals.icons =
		    g_slist_delete_link (globals.icons, globals.icons);
	}
}

void
gkbd_status_global_fill (GkbdStatus * gki)
{
	int grp;
	int total_groups = xkl_engine_get_num_groups (globals.engine);

	for (grp = 0; grp < total_groups; grp++) {
		GdkPixbuf *page = gkbd_status_prepare_drawing (gki, grp);
		globals.icons = g_slist_append (globals.icons, page);
	}
}

static void
gkbd_status_activate (GkbdStatus * gki)
{
	xkl_debug (150, "Mouse button pressed on applet\n");
	gkbd_desktop_config_lock_next_group (&globals.cfg);
}

/* hackish xref */
extern gchar *gkbd_indicator_extract_layout_name (int group,
						  XklEngine * engine,
						  GkbdKeyboardConfig *
						  kbd_cfg,
						  gchar **
						  short_group_names,
						  gchar **
						  full_group_names);

extern gchar *gkbd_indicator_create_label_title (int group,
						 GHashTable **
						 ln2cnt_map,
						 gchar * layout_name);

static void
gkbd_status_render_cairo (cairo_t * cr, int group)
{
	double r, g, b;
	PangoFontDescription *pfd;
	PangoContext *pcc;
	PangoLayout *pl;
	int lwidth, lheight;
	gchar *layout_name, *lbl_title;
	cairo_font_options_t *fo;
	static GHashTable *ln2cnt_map = NULL;

	xkl_debug (160, "Rendering cairo for group %d\n", group);
	if (globals.ind_cfg.background_color != NULL &&
	    globals.ind_cfg.background_color[0] != 0) {
		if (sscanf
		    (globals.ind_cfg.background_color, "%lg %lg %lg", &r,
		     &g, &b) == 3) {
			cairo_set_source_rgb (cr, r, g, b);
			cairo_rectangle (cr, 0, 0, globals.current_width,
					 globals.current_height);
			cairo_fill (cr);
		}
	}

	if (globals.ind_cfg.foreground_color != NULL &&
	    globals.ind_cfg.foreground_color[0] != 0) {
		if (sscanf
		    (globals.ind_cfg.foreground_color, "%lg %lg %lg", &r,
		     &g, &b) == 3) {
			cairo_set_source_rgb (cr, r, g, b);
		}
	}

	if (globals.ind_cfg.font_family != NULL &&
	    globals.ind_cfg.font_family[0] != 0) {
		cairo_select_font_face (cr, globals.ind_cfg.font_family,
					CAIRO_FONT_SLANT_NORMAL,
					CAIRO_FONT_WEIGHT_NORMAL);
	}

	pfd = pango_font_description_new ();
	pango_font_description_set_family (pfd,
					   globals.ind_cfg.font_family);
	pango_font_description_set_style (pfd, PANGO_STYLE_NORMAL);
	pango_font_description_set_weight (pfd, PANGO_WEIGHT_NORMAL);
	pango_font_description_set_size (pfd,
					 globals.ind_cfg.font_size *
					 PANGO_SCALE);

	pcc = pango_cairo_create_context (cr);

	fo = cairo_font_options_copy (gdk_screen_get_font_options
				      (gdk_screen_get_default ()));
	/* SUBPIXEL antialiasing gives bad results on in-memory images */
	if (cairo_font_options_get_antialias (fo) ==
	    CAIRO_ANTIALIAS_SUBPIXEL)
		cairo_font_options_set_antialias (fo,
						  CAIRO_ANTIALIAS_GRAY);
	pango_cairo_context_set_font_options (pcc, fo);

	pl = pango_layout_new (pcc);

	layout_name = gkbd_indicator_extract_layout_name (group,
							  globals.engine,
							  &globals.kbd_cfg,
							  globals.short_group_names,
							  globals.full_group_names);
	lbl_title =
	    gkbd_indicator_create_label_title (group, &ln2cnt_map,
					       layout_name);

	if (group + 1 == xkl_engine_get_num_groups (globals.engine)) {
		g_hash_table_destroy (ln2cnt_map);
		ln2cnt_map = NULL;
	}

	pango_layout_set_text (pl, lbl_title, -1);

	g_free (lbl_title);

	pango_layout_set_font_description (pl, pfd);
	pango_layout_get_size (pl, &lwidth, &lheight);

	cairo_move_to (cr,
		       (globals.current_width - lwidth / PANGO_SCALE) / 2,
		       (globals.current_height -
			lheight / PANGO_SCALE) / 2);

	pango_cairo_show_layout (cr, pl);

	pango_font_description_free (pfd);
	g_object_unref (pl);
	g_object_unref (pcc);
	cairo_font_options_destroy (fo);
	cairo_destroy (cr);

	globals.real_width = (lwidth / PANGO_SCALE) + 4;
	if (globals.real_width > globals.current_width)
		globals.real_width = globals.current_width;
	if (globals.real_width < globals.current_height)
		globals.real_width = globals.current_height;
}

static inline guint8
convert_color_channel (guint8 src, guint8 alpha)
{
	return alpha ? ((((guint) src) << 8) - src) / alpha : 0;
}

static void
convert_bgra_to_rgba (guint8 const *src, guint8 * dst, int width,
		      int height, int new_width)
{
	int xoffset = width - new_width;

	/* *4 */
	int ptr_step = xoffset << 2;

	int x, y;

	/* / 2 * 4 */
	src = src + ((xoffset >> 1) << 2);

	for (y = height; --y >= 0; src += ptr_step) {
		for (x = new_width; --x >= 0;) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
			dst[0] = convert_color_channel (src[2], src[3]);
			dst[1] = convert_color_channel (src[1], src[3]);
			dst[2] = convert_color_channel (src[0], src[3]);
			dst[3] = src[3];
#else
			dst[0] = convert_color_channel (src[1], src[0]);
			dst[1] = convert_color_channel (src[2], src[0]);
			dst[2] = convert_color_channel (src[3], src[0]);
			dst[3] = src[0];
#endif
			dst += 4;
			src += 4;
		}
	}
}

static GdkPixbuf *
gkbd_status_prepare_drawing (GkbdStatus * gki, int group)
{
	GError *gerror = NULL;
	char *image_filename;
	GdkPixbuf *image;

	if (globals.current_width == 0)
		return NULL;

	if (globals.ind_cfg.show_flags) {

		image_filename =
		    (char *) g_slist_nth_data (globals.
					       ind_cfg.image_filenames,
					       group);

		image = gdk_pixbuf_new_from_file_at_size (image_filename,
							  globals.current_width,
							  globals.current_height,
							  &gerror);

		if (image == NULL) {
			GtkWidget *dialog = gtk_message_dialog_new (NULL,
								    GTK_DIALOG_DESTROY_WITH_PARENT,
								    GTK_MESSAGE_ERROR,
								    GTK_BUTTONS_OK,
								    _
								    ("There was an error loading an image: %s"),
								    gerror
								    ==
								    NULL ?
								    "Unknown"
								    :
								    gerror->message);
			g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  NULL);

			gtk_window_set_resizable (GTK_WINDOW (dialog),
						  FALSE);

			gtk_widget_show (dialog);
			g_error_free (gerror);

			return NULL;
		}
		xkl_debug (150,
			   "Image %d[%s] loaded -> %p[%dx%d], alpha: %d\n",
			   group, image_filename, image,
			   gdk_pixbuf_get_width (image),
			   gdk_pixbuf_get_height (image),
			   gdk_pixbuf_get_has_alpha (image));

		return image;
	} else {
		cairo_surface_t *cs =
		    cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						globals.current_width,
						globals.current_height);
		unsigned char *cairo_data;
		guchar *pixbuf_data;
		gkbd_status_render_cairo (cairo_create (cs), group);
		cairo_data = cairo_image_surface_get_data (cs);
#if 0
		char pngfilename[20];
		g_sprintf (pngfilename, "label%d.png", group);
		cairo_surface_write_to_png (cs, pngfilename);
#endif
		pixbuf_data =
		    g_new0 (guchar,
			    4 * globals.real_width *
			    globals.current_height);
		convert_bgra_to_rgba (cairo_data, pixbuf_data,
				      globals.current_width,
				      globals.current_height,
				      globals.real_width);

		cairo_surface_destroy (cs);

		image = gdk_pixbuf_new_from_data (pixbuf_data,
						  GDK_COLORSPACE_RGB,
						  TRUE,
						  8,
						  globals.real_width,
						  globals.current_height,
						  globals.real_width *
						  4,
						  (GdkPixbufDestroyNotify)
						  g_free, NULL);
		xkl_debug (150,
			   "Image %d created -> %p[%dx%d], alpha: %d\n",
			   group, image, gdk_pixbuf_get_width (image),
			   gdk_pixbuf_get_height (image),
			   gdk_pixbuf_get_has_alpha (image));

		return image;
	}
	return NULL;
}

static void
gkbd_status_update_tooltips (GkbdStatus * gki)
{
	XklState *state = xkl_engine_get_current_state (globals.engine);
	gchar *buf;
	if (state == NULL || state->group < 0
	    || state->group >= g_strv_length (globals.full_group_names))
		return;

	buf = g_strdup_printf (globals.tooltips_format,
			       globals.full_group_names[state->group]);

	gkbd_status_set_tooltips (gki, buf);
	g_free (buf);
}

void
gkbd_status_reinit_ui (GkbdStatus * gki)
{
	gkbd_status_global_cleanup (gki);
	gkbd_status_global_fill (gki);

	gkbd_status_set_current_page (gki);
}

/* Should be called once for all widgets */
static void
gkbd_status_cfg_changed (MateConfClient * client,
			 guint cnxn_id, MateConfEntry * entry)
{
	xkl_debug (100,
		   "General configuration changed in MateConf - reiniting...\n");
	gkbd_desktop_config_load_from_mateconf (&globals.cfg);
	gkbd_desktop_config_activate (&globals.cfg);
	ForAllIndicators () {
		gkbd_status_reinit_ui (gki);
	} NextIndicator ();
}

/* Should be called once for all widgets */
static void
gkbd_status_ind_cfg_changed (MateConfClient * client,
			     guint cnxn_id, MateConfEntry * entry)
{
	xkl_debug (100,
		   "Applet configuration changed in MateConf - reiniting...\n");
	gkbd_indicator_config_load_from_mateconf (&globals.ind_cfg);

	gkbd_indicator_config_free_image_filenames (&globals.ind_cfg);
	gkbd_indicator_config_load_image_filenames (&globals.ind_cfg,
						    &globals.kbd_cfg);

	gkbd_indicator_config_activate (&globals.ind_cfg);

	ForAllIndicators () {
		gkbd_status_reinit_ui (gki);
	} NextIndicator ();
}

static void
gkbd_status_load_group_names (const gchar ** layout_ids,
			      const gchar ** variant_ids)
{
	if (!gkbd_desktop_config_load_group_descriptions
	    (&globals.cfg, globals.registry, layout_ids, variant_ids,
	     &globals.short_group_names, &globals.full_group_names)) {
		/* We just populate no short names (remain NULL) - 
		 * full names are going to be used anyway */
		gint i, total_groups =
		    xkl_engine_get_num_groups (globals.engine);
		xkl_debug (150, "group descriptions loaded: %d!\n",
			   total_groups);
		globals.full_group_names =
		    g_new0 (char *, total_groups + 1);

		if (xkl_engine_get_features (globals.engine) &
		    XKLF_MULTIPLE_LAYOUTS_SUPPORTED) {
			GSList *lst = globals.kbd_cfg.layouts_variants;
			for (i = 0; lst; lst = lst->next, i++) {
				globals.full_group_names[i] =
				    g_strdup ((char *) lst->data);
			}
		} else {
			for (i = total_groups; --i >= 0;) {
				globals.full_group_names[i] =
				    g_strdup_printf ("Group %d", i);
			}
		}
	}
}

/* Should be called once for all widgets */
static void
gkbd_status_kbd_cfg_callback (GkbdStatus * gki)
{
	XklConfigRec *xklrec = xkl_config_rec_new ();
	xkl_debug (100,
		   "XKB configuration changed on X Server - reiniting...\n");

	gkbd_keyboard_config_load_from_x_current (&globals.kbd_cfg,
						  xklrec);

	gkbd_indicator_config_free_image_filenames (&globals.ind_cfg);
	gkbd_indicator_config_load_image_filenames (&globals.ind_cfg,
						    &globals.kbd_cfg);

	g_strfreev (globals.full_group_names);
	globals.full_group_names = NULL;

	if (globals.short_group_names != NULL) {
		g_strfreev (globals.short_group_names);
		globals.short_group_names = NULL;
	}

	gkbd_status_load_group_names ((const gchar **) xklrec->layouts,
				      (const gchar **) xklrec->variants);

	ForAllIndicators () {
		gkbd_status_reinit_ui (gki);
	} NextIndicator ();
	g_object_unref (G_OBJECT (xklrec));
}

/* Should be called once for all applets */
static void
gkbd_status_state_callback (XklEngine * engine,
			    XklEngineStateChange changeType,
			    gint group, gboolean restore)
{
	xkl_debug (150, "group is now %d, restore: %d\n", group, restore);

	if (changeType == GROUP_CHANGED) {
		ForAllIndicators () {
			xkl_debug (200, "do repaint\n");
			gkbd_status_set_current_page_for_group (gki,
								group);
		}
		NextIndicator ();
	}
}


void
gkbd_status_set_current_page (GkbdStatus * gki)
{
	XklState *cur_state;
	cur_state = xkl_engine_get_current_state (globals.engine);
	if (cur_state->group >= 0)
		gkbd_status_set_current_page_for_group (gki,
							cur_state->group);
}

void
gkbd_status_set_current_page_for_group (GkbdStatus * gki, int group)
{
	xkl_debug (200, "Revalidating for group %d\n", group);

	gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (gki),
					 GDK_PIXBUF (g_slist_nth_data
						     (globals.icons,
						      group)));

	gkbd_status_update_tooltips (gki);
}

/* Should be called once for all widgets */
static GdkFilterReturn
gkbd_status_filter_x_evt (GdkXEvent * xev, GdkEvent * event)
{
	XEvent *xevent = (XEvent *) xev;

	xkl_engine_filter_events (globals.engine, xevent);
	switch (xevent->type) {
	case ReparentNotify:
		{
			XReparentEvent *rne = (XReparentEvent *) xev;

			ForAllIndicators () {
				guint32 xid =
				    gtk_status_icon_get_x11_window_id
				    (GTK_STATUS_ICON (gki));

				/* compare the indicator's parent window with the even window */
				if (xid == rne->window) {
					/* if so - make it transparent... */
					xkl_engine_set_window_transparent
					    (globals.engine, rne->window,
					     TRUE);
				}
			}
		NextIndicator ()}
		break;
	}
	return GDK_FILTER_CONTINUE;
}


/* Should be called once for all widgets */
static void
gkbd_status_start_listen (void)
{
	gdk_window_add_filter (NULL, (GdkFilterFunc)
			       gkbd_status_filter_x_evt, NULL);
	gdk_window_add_filter (gdk_get_default_root_window (),
			       (GdkFilterFunc) gkbd_status_filter_x_evt,
			       NULL);

	xkl_engine_start_listen (globals.engine,
				 XKLL_TRACK_KEYBOARD_STATE);
}

/* Should be called once for all widgets */
static void
gkbd_status_stop_listen (void)
{
	xkl_engine_stop_listen (globals.engine, XKLL_TRACK_KEYBOARD_STATE);

	gdk_window_remove_filter (NULL, (GdkFilterFunc)
				  gkbd_status_filter_x_evt, NULL);
	gdk_window_remove_filter
	    (gdk_get_default_root_window (),
	     (GdkFilterFunc) gkbd_status_filter_x_evt, NULL);
}

static void
gkbd_status_size_changed (GkbdStatus * gki, gint size)
{
	if (globals.current_height != size) {
		globals.current_height = size;
		globals.current_width = size * 3 / 2;
		gkbd_status_reinit_ui (gki);
	}
}

static void
gkbd_status_theme_changed (GtkSettings * settings, GParamSpec * pspec,
			   GkbdStatus * gki)
{
	gkbd_indicator_config_refresh_style (&globals.ind_cfg);
	gkbd_status_reinit_ui (gki);
}

static void
gkbd_status_init (GkbdStatus * gki)
{
	int i;

	if (!g_slist_length (globals.widget_instances))
		gkbd_status_global_init ();

	gki->priv = g_new0 (GkbdStatusPrivate, 1);

	/* This should give NA a hint about the order */
	gtk_status_icon_set_name (GTK_STATUS_ICON (gki), "keyboard");

	xkl_debug (100, "Initiating the widget startup process for %p\n",
		   gki);

	if (globals.engine == NULL) {
		gkbd_status_set_tooltips (gki,
					  _("XKB initialization error"));
		return;
	}

	gkbd_status_set_tooltips (gki, NULL);

	gkbd_status_global_fill (gki);
	gkbd_status_set_current_page (gki);

	/* append AFTER all initialization work is finished */
	globals.widget_instances =
	    g_slist_append (globals.widget_instances, gki);

	g_signal_connect (gki, "size-changed",
			  G_CALLBACK (gkbd_status_size_changed), NULL);
	g_signal_connect (gki, "activate",
			  G_CALLBACK (gkbd_status_activate), NULL);

	for (i = sizeof (settings_signal_names) /
	     sizeof (settings_signal_names[0]); --i >= 0;)
		gki->priv->settings_signal_handlers[i] =
		    g_signal_connect_after (gtk_settings_get_default (),
					    settings_signal_names[i],
					    G_CALLBACK
					    (gkbd_status_theme_changed),
					    gki);
}

static void
gkbd_status_finalize (GObject * obj)
{
	int i;
	GkbdStatus *gki = GKBD_STATUS (obj);
	xkl_debug (100,
		   "Starting the mate-kbd-status widget shutdown process for %p\n",
		   gki);

	for (i = sizeof (settings_signal_names) /
	     sizeof (settings_signal_names[0]); --i >= 0;)
		g_signal_handler_disconnect (gtk_settings_get_default (),
					     gki->
					     priv->settings_signal_handlers
					     [i]);

	/* remove BEFORE all termination work is finished */
	globals.widget_instances =
	    g_slist_remove (globals.widget_instances, gki);

	gkbd_status_global_cleanup (gki);

	xkl_debug (100,
		   "The instance of mate-kbd-status successfully finalized\n");

	g_free (gki->priv);

	G_OBJECT_CLASS (gkbd_status_parent_class)->finalize (obj);

	if (!g_slist_length (globals.widget_instances))
		gkbd_status_global_term ();
}

static void
gkbd_status_global_term (void)
{
	xkl_debug (100, "*** Last  GkbdStatus instance *** \n");
	gkbd_status_stop_listen ();

	gkbd_desktop_config_stop_listen (&globals.cfg);
	gkbd_indicator_config_stop_listen (&globals.ind_cfg);

	gkbd_indicator_config_term (&globals.ind_cfg);
	gkbd_keyboard_config_term (&globals.kbd_cfg);
	gkbd_desktop_config_term (&globals.cfg);

	if (g_signal_handler_is_connected
	    (globals.engine, globals.state_changed_handler)) {
		g_signal_handler_disconnect (globals.engine,
					     globals.state_changed_handler);
		globals.state_changed_handler = 0;
	}
	if (g_signal_handler_is_connected
	    (globals.engine, globals.config_changed_handler)) {
		g_signal_handler_disconnect (globals.engine,
					     globals.config_changed_handler);
		globals.config_changed_handler = 0;
	}

	g_object_unref (G_OBJECT (globals.registry));
	globals.registry = NULL;
	g_object_unref (G_OBJECT (globals.engine));
	globals.engine = NULL;
	xkl_debug (100, "*** Terminated globals *** \n");
}

static void
gkbd_status_class_init (GkbdStatusClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	xkl_debug (100, "*** First GkbdStatus instance *** \n");

	memset (&globals, 0, sizeof (globals));

	/* Initing some global vars */
	globals.tooltips_format = "%s";

	/* Initing vtable */
	object_class->finalize = gkbd_status_finalize;

	/* Signals */
}

static void
gkbd_status_global_init (void)
{
	MateConfClient *mateconf_client;
	XklConfigRec *xklrec = xkl_config_rec_new ();

	globals.engine = xkl_engine_get_instance (GDK_DISPLAY ());
	if (globals.engine == NULL) {
		xkl_debug (0, "Libxklavier initialization error");
		return;
	}

	mateconf_client = mateconf_client_get_default ();

	globals.state_changed_handler =
	    g_signal_connect (globals.engine, "X-state-changed",
			      G_CALLBACK (gkbd_status_state_callback),
			      NULL);
	globals.config_changed_handler =
	    g_signal_connect (globals.engine, "X-config-changed",
			      G_CALLBACK (gkbd_status_kbd_cfg_callback),
			      NULL);

	gkbd_desktop_config_init (&globals.cfg, mateconf_client,
				  globals.engine);
	gkbd_keyboard_config_init (&globals.kbd_cfg, mateconf_client,
				   globals.engine);
	gkbd_indicator_config_init (&globals.ind_cfg, mateconf_client,
				    globals.engine);

	g_object_unref (mateconf_client);

	gkbd_desktop_config_load_from_mateconf (&globals.cfg);
	gkbd_desktop_config_activate (&globals.cfg);

	globals.registry =
	    xkl_config_registry_get_instance (globals.engine);
	xkl_config_registry_load (globals.registry,
				  globals.cfg.load_extra_items);

	gkbd_keyboard_config_load_from_x_current (&globals.kbd_cfg,
						  xklrec);

	gkbd_indicator_config_load_from_mateconf (&globals.ind_cfg);

	gkbd_indicator_config_load_image_filenames (&globals.ind_cfg,
						    &globals.kbd_cfg);

	gkbd_indicator_config_activate (&globals.ind_cfg);

	gkbd_status_load_group_names ((const gchar **) xklrec->layouts,
				      (const gchar **) xklrec->variants);
	g_object_unref (G_OBJECT (xklrec));

	gkbd_desktop_config_start_listen (&globals.cfg,
					  (MateConfClientNotifyFunc)
					  gkbd_status_cfg_changed, NULL);
	gkbd_indicator_config_start_listen (&globals.ind_cfg,
					    (MateConfClientNotifyFunc)
					    gkbd_status_ind_cfg_changed,
					    NULL);
	gkbd_status_start_listen ();

	xkl_debug (100, "*** Inited globals *** \n");
}

GtkStatusIcon *
gkbd_status_new (void)
{
	return
	    GTK_STATUS_ICON (g_object_new (gkbd_status_get_type (), NULL));
}

XklEngine *
gkbd_status_get_xkl_engine ()
{
	return globals.engine;
}

gchar **
gkbd_status_get_group_names ()
{
	return globals.full_group_names;
}

gchar *
gkbd_status_get_image_filename (guint group)
{
	if (!globals.ind_cfg.show_flags)
		return NULL;
	return gkbd_indicator_config_get_images_file (&globals.ind_cfg,
						      &globals.kbd_cfg,
						      group);
}

void
gkbd_status_set_angle (GkbdStatus * gki, gdouble angle)
{
	gki->priv->angle = angle;
}
