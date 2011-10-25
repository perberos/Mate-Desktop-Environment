/* -*- mode: C; c-basic-offset: 4 -*-
 * fontilus - a collection of font utilities for MATE
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>

FT_Error FT_New_Face_From_URI(FT_Library library,
			      const gchar *uri,
			      FT_Long face_index,
			      FT_Face *aface);

static const gchar lowercase_text[] = "abcdefghijklmnopqrstuvwxyz";
static const gchar uppercase_text[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const gchar punctuation_text[] = "0123456789.:,;(*!?')";

static inline XftFont *
get_font(Display *xdisplay, FT_Face face, gint size, FcCharSet *charset)
{
    FcPattern *pattern;
    XftFont *font;
    int screen = DefaultScreen (xdisplay);

    pattern = FcPatternBuild(NULL,
			     FC_FT_FACE, FcTypeFTFace, face,
			     FC_PIXEL_SIZE, FcTypeDouble, (double)size,
			     NULL);

    if (charset)
	FcPatternAddCharSet (pattern, "charset", charset);

    FcConfigSubstitute (NULL, pattern, FcMatchPattern);
    XftDefaultSubstitute (xdisplay, screen, pattern);

    font = XftFontOpenPattern(xdisplay, pattern);

    return font;
}

static inline void
draw_string(Display *xdisplay, XftDraw *draw, XftFont *font, XftColor *colour,
	    const gchar *text, gint *pos_y)
{
    XGlyphInfo extents;
    gint len = strlen(text);

    XftTextExtentsUtf8(xdisplay, font, (guchar *)text, len, &extents);
    XftDrawStringUtf8(draw, colour, font, 4, *pos_y + extents.y, (guchar *)text, len);
    *pos_y += extents.height + 4;
}

static gboolean
check_font_contain_text (FT_Face face, const gchar *text)
{
    while (text && *text)
	{
	    gunichar wc = g_utf8_get_char (text);
	    if (!FT_Get_Char_Index (face, wc))
		return FALSE;

	    text = g_utf8_next_char (text);
	}

    return TRUE;
}


static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, GdkPixmap *pixmap);

static GdkPixmap *
create_text_pixmap(GtkWidget *drawing_area, FT_Face face)
{
    gint i, pixmap_width, pixmap_height, pos_y, textlen;
    GdkPixmap *pixmap = NULL;
    const gchar *text;
    Display *xdisplay;
    Drawable xdrawable;
    Visual *xvisual;
    Colormap xcolormap;
    XftDraw *draw;
    XftColor colour;
    XGlyphInfo extents;
    XftFont *font;
    gint *sizes = NULL, n_sizes, alpha_size;
    FcCharSet *charset = NULL;
    GdkWindow *window = gtk_widget_get_window (drawing_area);

    text = pango_language_get_sample_string(NULL);
    if (! check_font_contain_text (face, text))
	{
	    pango_language_get_sample_string (pango_language_from_string ("en_US"));
	}

    textlen = strlen(text);

    /* create the XftDraw */
    xdisplay = GDK_PIXMAP_XDISPLAY(window);

	#if GTK_CHECK_VERSION(3, 0, 0)
		xvisual = GDK_VISUAL_XVISUAL(gdk_window_get_visual(window));
	#else
		xvisual = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(window));
	#endif

    xcolormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(window));
    XftColorAllocName(xdisplay, xvisual, xcolormap, "black", &colour);

    /* work out what sizes to render */
    if (FT_IS_SCALABLE(face)) {
	n_sizes = 8;
	sizes = g_new(gint, n_sizes);
	sizes[0] = 8;
	sizes[1] = 10;
	sizes[2] = 12;
	sizes[3] = 18;
	sizes[4] = 24;
	sizes[5] = 36;
	sizes[6] = 48;
	sizes[7] = 72;
	alpha_size = 24;
    } else {
	/* use fixed sizes */
	n_sizes = face->num_fixed_sizes;
	sizes = g_new(gint, n_sizes);
	alpha_size = 0;
	for (i = 0; i < face->num_fixed_sizes; i++) {
	    sizes[i] = face->available_sizes[i].height;

	    /* work out which font size to render */
	    if (face->available_sizes[i].height <= 24)
		alpha_size = face->available_sizes[i].height;
	}
    }

    /* calculate size of pixmap to use (with 4 pixels padding) ... */
    pixmap_width = 8;
    pixmap_height = 8;

    font = get_font(xdisplay, face, alpha_size, charset);
    charset = FcCharSetCopy (font->charset);
    XftTextExtentsUtf8(xdisplay, font,
		       (guchar *)lowercase_text, strlen(lowercase_text), &extents);
    pixmap_height += extents.height + 4;
    pixmap_width = MAX(pixmap_width, 8 + extents.width);
    XftTextExtentsUtf8(xdisplay, font,
		       (guchar *)uppercase_text, strlen(uppercase_text), &extents);
    pixmap_height += extents.height + 4;
    pixmap_width = MAX(pixmap_width, 8 + extents.width);
    XftTextExtentsUtf8(xdisplay, font,
		       (guchar *)punctuation_text, strlen(punctuation_text), &extents);
    pixmap_height += extents.height + 4;
    pixmap_width = MAX(pixmap_width, 8 + extents.width);
    XftFontClose(xdisplay, font);

    pixmap_height += 8;

    for (i = 0; i < n_sizes; i++) {
	font = get_font(xdisplay, face, sizes[i], charset);
	if (!font) continue;
	XftTextExtentsUtf8(xdisplay, font, (guchar *)text, textlen, &extents);
	pixmap_height += extents.height + 4;
	pixmap_width = MAX(pixmap_width, 8 + extents.width);
	XftFontClose(xdisplay, font);
    }

    /* create pixmap */
    gtk_widget_set_size_request(drawing_area, pixmap_width, pixmap_height);
    pixmap = gdk_pixmap_new(window,
			    pixmap_width, pixmap_height, -1);
    if (!pixmap)
	goto end;
    gdk_draw_rectangle(pixmap, gtk_widget_get_style(drawing_area)->white_gc,
		       TRUE, 0, 0, pixmap_width, pixmap_height);

    xdrawable = GDK_DRAWABLE_XID(pixmap);
    draw = XftDrawCreate(xdisplay, xdrawable, xvisual, xcolormap);

    /* draw text */
    pos_y = 4;
    font = get_font(xdisplay, face, alpha_size, charset);
    draw_string(xdisplay, draw, font, &colour, lowercase_text, &pos_y);
    draw_string(xdisplay, draw, font, &colour, uppercase_text, &pos_y);
    draw_string(xdisplay, draw, font, &colour, punctuation_text, &pos_y);
    XftFontClose(xdisplay, font);

    pos_y += 8;
    for (i = 0; i < n_sizes; i++) {
	font = get_font(xdisplay, face, sizes[i], charset);
	if (!font) continue;
	draw_string(xdisplay, draw, font, &colour, text, &pos_y);
	XftFontClose(xdisplay, font);
    }

    g_signal_connect(drawing_area, "expose-event", G_CALLBACK(expose_event),
                     pixmap);

 end:
    g_free(sizes);
    FcCharSetDestroy (charset);
    return pixmap;
}

static void
add_row(GtkWidget *table, gint *row_p,
	const gchar *name, const gchar *value, gboolean multiline,
        gboolean expand)
{
    gchar *bold_name;
    GtkWidget *name_w;

    bold_name = g_strconcat("<b>", name, "</b>", NULL);
    name_w = gtk_label_new(bold_name);
    g_free(bold_name);
    gtk_misc_set_alignment(GTK_MISC(name_w), 0.0, 0.0);
    gtk_label_set_use_markup(GTK_LABEL(name_w), TRUE);

    gtk_table_attach(GTK_TABLE(table), name_w, 0, 1, *row_p, *row_p + 1,
		     GTK_FILL, GTK_FILL, 0, 0);

    if (multiline) {
	GtkWidget *label, *viewport;
	GtkScrolledWindow *swin;
        guint flags;

        label = gtk_label_new (value);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_label_set_selectable (GTK_LABEL (label), TRUE);
        gtk_widget_set_size_request (label, 200, -1);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);


        swin = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
        gtk_scrolled_window_set_policy(swin,
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (swin),
                                     gtk_scrolled_window_get_vadjustment (swin));
        gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

        gtk_container_add (GTK_CONTAINER(swin), viewport);
        (*row_p)++;
        if (expand)
          flags = GTK_FILL|GTK_EXPAND;
        else
          flags = GTK_FILL;
        gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(swin), 0, 2, *row_p, *row_p + 1,
                         GTK_FILL|GTK_EXPAND, flags, 0, 0);

        gtk_container_add (GTK_CONTAINER (viewport), label);
    } else {
        GtkWidget *label = gtk_label_new(value);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, *row_p, *row_p + 1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
    }


    (*row_p)++;
}

static void
add_face_info(GtkWidget *table, gint *row_p, const gchar *uri, FT_Face face)
{
    gchar *s;
    GFile *file;
    GFileInfo *info;
    PS_FontInfoRec ps_info;

    add_row(table, row_p, _("Name:"), face->family_name, FALSE, FALSE);

    if (face->style_name)
	add_row(table, row_p, _("Style:"), face->style_name, FALSE, FALSE);

    file = g_file_new_for_uri (uri);

    info = g_file_query_info (file,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                              G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE,
                              NULL, NULL);
    g_object_unref (file);

    if (info) {
        s = g_content_type_get_description (g_file_info_get_content_type (info));
        add_row (table, row_p, _("Type:"), s, FALSE, FALSE);
        g_free (s);

        s = g_format_size_for_display (g_file_info_get_size (info));
        add_row (table, row_p, _("Size:"), s, FALSE, FALSE);
        g_free (s);

        g_object_unref (info);
    }

    if (FT_IS_SFNT(face)) {
	gint i, len;
	gchar *version = NULL, *copyright = NULL, *description = NULL;

	len = FT_Get_Sfnt_Name_Count(face);
	for (i = 0; i < len; i++) {
	    FT_SfntName sname;

	    if (FT_Get_Sfnt_Name(face, i, &sname) != 0)
		continue;

	    /* only handle the unicode names for US langid */
	    if (!(sname.platform_id == TT_PLATFORM_MICROSOFT &&
		  sname.encoding_id == TT_MS_ID_UNICODE_CS &&
		  sname.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
		continue;

	    switch (sname.name_id) {
	    case TT_NAME_ID_COPYRIGHT:
		g_free(copyright);
		copyright = g_convert((gchar *)sname.string, sname.string_len,
				      "UTF-8", "UTF-16BE", NULL, NULL, NULL);
		break;
	    case TT_NAME_ID_VERSION_STRING:
		g_free(version);
		version = g_convert((gchar *)sname.string, sname.string_len,
				    "UTF-8", "UTF-16BE", NULL, NULL, NULL);
		break;
	    case TT_NAME_ID_DESCRIPTION:
		g_free(description);
		description = g_convert((gchar *)sname.string, sname.string_len,
					"UTF-8", "UTF-16BE", NULL, NULL, NULL);
		break;
	    default:
		break;
	    }
	}
	if (version) {
	    add_row(table, row_p, _("Version:"), version, FALSE, FALSE);
	    g_free(version);
	}
	if (copyright) {
	    add_row(table, row_p, _("Copyright:"), copyright, TRUE, TRUE);
	    g_free(copyright);
	}
	if (description) {
	    add_row(table, row_p, _("Description:"), description, TRUE, TRUE);
	    g_free(description);
	}
    } else if (FT_Get_PS_Font_Info(face, &ps_info) == 0) {
	if (ps_info.version && g_utf8_validate(ps_info.version, -1, NULL))
	    add_row(table, row_p, _("Version:"), ps_info.version, FALSE, FALSE);
	if (ps_info.notice && g_utf8_validate(ps_info.notice, -1, NULL))
	    add_row(table, row_p, _("Copyright:"), ps_info.notice, TRUE, FALSE);
    }
}

static gboolean
expose_event(GtkWidget *widget, GdkEventExpose *event, GdkPixmap *pixmap)
{
    gdk_draw_drawable(gtk_widget_get_window (widget),
		      gtk_widget_get_style (widget)->fg_gc[gtk_widget_get_state (widget)],
		      pixmap,
		      event->area.x, event->area.y,
		      event->area.x, event->area.y,
		      event->area.width, event->area.height);
    return FALSE;
}

static void
set_icon(GtkWindow *window, const gchar *uri)
{
    GFile *file;
    GIcon *icon;
    GFileInfo *info;
    GdkScreen *screen;
    GtkIconTheme *icon_theme;
    const gchar *icon_name = NULL, *content_type;

    screen = gtk_widget_get_screen (GTK_WIDGET (window));
    icon_theme = gtk_icon_theme_get_for_screen (screen);

    file = g_file_new_for_uri (uri);

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);
    g_object_unref (file);

    if (! info)
	return;

    content_type = g_file_info_get_content_type (info);
    icon = g_content_type_get_icon (content_type);

    if (G_IS_THEMED_ICON (icon)) {
       const gchar * const *names = NULL;

       names = g_themed_icon_get_names (G_THEMED_ICON (icon));
       if (names) {
          gint i;
          for (i = 0; names[i]; i++) {
	      if (gtk_icon_theme_has_icon (icon_theme, names[i])) {
		  icon_name = names[i];
		  break;
	      }
          }
       }
    }

    if (icon_name) {
        gtk_window_set_icon_name (window, icon_name);
    }

    g_object_unref (icon);
}

static void
font_install_finished_cb (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      data)
{
    GError *err = NULL;

    g_file_copy_finish (G_FILE (source_object), res, &err);

    if (!err) {
        gtk_button_set_label (GTK_BUTTON (data), _("Installed"));
    }
    else {
        gtk_button_set_label (GTK_BUTTON (data), _("Install Failed"));
        g_debug ("Install failed: %s", err->message);
        g_error_free (err);
    }
    gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);
}

static void
install_button_clicked_cb (GtkButton   *button,
                           const gchar *font_file)
{
    GFile *src, *dest;
    gchar *dest_path, *dest_filename;

    GError *err = NULL;

    /* first check if ~/.fonts exists */
    dest_path = g_build_filename (g_get_home_dir (), ".fonts", NULL);
    if (!g_file_test (dest_path, G_FILE_TEST_EXISTS)) {
        GFile *f = g_file_new_for_path (dest_path);
        g_file_make_directory_with_parents (f, NULL, &err);
        g_object_unref (f);
        if (err) {
            /* TODO: show error dialog */
            g_warning ("Could not create fonts directory: %s", err->message);
            g_error_free (err);
            g_free (dest_path);
            return;
        }
    }
    g_free (dest_path);

    /* create destination filename */
    src = g_file_new_for_uri (font_file);

    dest_filename = g_file_get_basename (src);
    dest_path = g_build_filename (g_get_home_dir (), ".fonts", dest_filename, NULL);
    g_free (dest_filename);

    dest = g_file_new_for_path (dest_path);

    /* TODO: show error dialog if file exists */
    g_file_copy_async (src, dest, G_FILE_COPY_NONE, 0, NULL, NULL, NULL,
                       font_install_finished_cb, button);

    g_object_unref (src);
    g_object_unref (dest);
    g_free (dest_path);
}

int
main(int argc, char **argv)
{
    FT_Error error;
    FT_Library library;
    FT_Face face;
    GFile *file;
    gchar *font_file, *title;
    gint row;
    GtkWidget *window, *hbox, *table, *swin, *drawing_area;
    GdkPixmap *pixmap;
    GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
    GtkWidget *button, *align;

    bindtextdomain(GETTEXT_PACKAGE, MATELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    gtk_init(&argc, &argv);

    if (argc != 2) {
	g_printerr(_("usage: %s fontfile\n"), argv[0]);
	return 1;
    }

    if (!XftInitFtLibrary()) {
	g_printerr("could not initialise freetype library\n");
	return 1;
    }

    error = FT_Init_FreeType(&library);
    if (error) {
	g_printerr("could not initialise freetype\n");
	return 1;
    }

    file = g_file_new_for_commandline_arg (argv[1]);
    font_file = g_file_get_uri (file);
    g_object_unref (file);

    if (!font_file) {
	g_printerr("could not parse argument into a URI\n");
	return 1;
    }

    error = FT_New_Face_From_URI(library, font_file, 0, &face);
    if (error) {
	g_printerr("could not load face '%s'\n", font_file);
	return 1;
    }

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    title = g_strconcat(face->family_name,
			face->style_name ? ", " : "",
			face->style_name, NULL);
    gtk_window_set_title(GTK_WINDOW(window), title);
    set_icon(GTK_WINDOW(window), font_file);
    g_free(title);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 0);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &white);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin),
					  drawing_area);
    g_signal_connect (drawing_area, "realize", create_text_pixmap, face);

    /* set the minimum size on the scrolled window to prevent
     * unnecessary scrolling */
    gtk_widget_set_size_request(swin, 500, -1);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    table = gtk_table_new(1, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, TRUE, 0);

    row = 0;
    add_face_info(table, &row, font_file, face);

    /* add install button */
    align = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
    gtk_table_attach (GTK_TABLE (table), align, 0, 2, row, row + 1,
                      GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);

    button = gtk_button_new_with_mnemonic (_("I_nstall Font"));
    g_signal_connect (button, "clicked",
                      G_CALLBACK (install_button_clicked_cb), font_file);
    gtk_container_add (GTK_CONTAINER (align), button);


    gtk_table_set_col_spacings(GTK_TABLE(table), 8);
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}
