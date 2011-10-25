/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <mateconf/mateconf-client.h>

#include "gsm-inhibit-dialog.h"
#include "gsm-store.h"
#include "gsm-client.h"
#include "gsm-inhibitor.h"
#include "eggdesktopfile.h"
#include "gsm-util.h"

#ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>
#endif

#define GSM_INHIBIT_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_INHIBIT_DIALOG, GsmInhibitDialogPrivate))

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

#define GTKBUILDER_FILE "gsm-inhibit-dialog.ui"

#ifndef DEFAULT_ICON_SIZE
#define DEFAULT_ICON_SIZE 32
#endif

#ifndef DEFAULT_SNAPSHOT_SIZE
#define DEFAULT_SNAPSHOT_SIZE 128
#endif

#define DIALOG_RESPONSE_LOCK_SCREEN 1

struct GsmInhibitDialogPrivate
{
        GtkBuilder        *xml;
        int                action;
        gboolean           is_done;
        GsmStore          *inhibitors;
        GsmStore          *clients;
        GtkListStore      *list_store;
        gboolean           have_xrender;
        int                xrender_event_base;
        int                xrender_error_base;
};

enum {
        PROP_0,
        PROP_ACTION,
        PROP_INHIBITOR_STORE,
        PROP_CLIENT_STORE
};

enum {
        INHIBIT_IMAGE_COLUMN = 0,
        INHIBIT_NAME_COLUMN,
        INHIBIT_REASON_COLUMN,
        INHIBIT_ID_COLUMN,
        NUMBER_OF_COLUMNS
};

static void     gsm_inhibit_dialog_class_init  (GsmInhibitDialogClass *klass);
static void     gsm_inhibit_dialog_init        (GsmInhibitDialog      *inhibit_dialog);
static void     gsm_inhibit_dialog_finalize    (GObject               *object);

G_DEFINE_TYPE (GsmInhibitDialog, gsm_inhibit_dialog, GTK_TYPE_DIALOG)

static void
lock_screen (GsmInhibitDialog *dialog)
{
        GError *error;
        error = NULL;
        g_spawn_command_line_async ("mate-screensaver-command --lock", &error);
        if (error != NULL) {
                g_warning ("Couldn't lock screen: %s", error->message);
                g_error_free (error);
        }
}

static void
on_response (GsmInhibitDialog *dialog,
             gint              response_id)

{
        if (dialog->priv->is_done) {
                g_signal_stop_emission_by_name (dialog, "response");
                return;
        }

        switch (response_id) {
        case DIALOG_RESPONSE_LOCK_SCREEN:
                g_signal_stop_emission_by_name (dialog, "response");
                lock_screen (dialog);
                break;
        default:
                dialog->priv->is_done = TRUE;
                break;
        }
}

static void
gsm_inhibit_dialog_set_action (GsmInhibitDialog *dialog,
                               int               action)
{
        dialog->priv->action = action;
}

static gboolean
find_inhibitor (GsmInhibitDialog *dialog,
                const char       *id,
                GtkTreeIter      *iter)
{
        GtkTreeModel *model;
        gboolean      found_item;

        g_assert (GSM_IS_INHIBIT_DIALOG (dialog));

        found_item = FALSE;
        model = GTK_TREE_MODEL (dialog->priv->list_store);

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                char *item_id;

                gtk_tree_model_get (model,
                                    iter,
                                    INHIBIT_ID_COLUMN, &item_id,
                                    -1);
                if (item_id != NULL
                    && id != NULL
                    && strcmp (item_id, id) == 0) {
                        found_item = TRUE;
                }
                g_free (item_id);
        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

/* copied from mate-panel panel-util.c */
static char *
_util_icon_remove_extension (const char *icon)
{
        char *icon_no_extension;
        char *p;

        icon_no_extension = g_strdup (icon);
        p = strrchr (icon_no_extension, '.');
        if (p &&
            (strcmp (p, ".png") == 0 ||
             strcmp (p, ".xpm") == 0 ||
             strcmp (p, ".svg") == 0)) {
            *p = 0;
        }

        return icon_no_extension;
}

/* copied from mate-panel panel-util.c */
static char *
_find_icon (GtkIconTheme  *icon_theme,
            const char    *icon_name,
            gint           size)
{
        GtkIconInfo *info;
        char        *retval;
        char        *icon_no_extension;

        if (icon_name == NULL || strcmp (icon_name, "") == 0)
                return NULL;

        if (g_path_is_absolute (icon_name)) {
                if (g_file_test (icon_name, G_FILE_TEST_EXISTS)) {
                        return g_strdup (icon_name);
                } else {
                        char *basename;

                        basename = g_path_get_basename (icon_name);
                        retval = _find_icon (icon_theme, basename,
                                             size);
                        g_free (basename);

                        return retval;
                }
        }

        /* This is needed because some .desktop files have an icon name *and*
         * an extension as icon */
        icon_no_extension = _util_icon_remove_extension (icon_name);

        info = gtk_icon_theme_lookup_icon (icon_theme, icon_no_extension,
                                           size, 0);

        g_free (icon_no_extension);

        if (info) {
                retval = g_strdup (gtk_icon_info_get_filename (info));
                gtk_icon_info_free (info);
        } else
                retval = NULL;

        return retval;
}

/* copied from mate-panel panel-util.c */
static GdkPixbuf *
_load_icon (GtkIconTheme  *icon_theme,
            const char    *icon_name,
            int            size,
            int            desired_width,
            int            desired_height,
            char         **error_msg)
{
        GdkPixbuf *retval;
        char      *file;
        GError    *error;

        g_return_val_if_fail (error_msg == NULL || *error_msg == NULL, NULL);

        file = _find_icon (icon_theme, icon_name, size);
        if (!file) {
                if (error_msg)
                        *error_msg = g_strdup_printf (_("Icon '%s' not found"),
                                                      icon_name);

                return NULL;
        }

        error = NULL;
        retval = gdk_pixbuf_new_from_file_at_size (file,
                                                   desired_width,
                                                   desired_height,
                                                   &error);
        if (error) {
                if (error_msg)
                        *error_msg = g_strdup (error->message);
                g_error_free (error);
        }

        g_free (file);

        return retval;
}


static GdkPixbuf *
scale_pixbuf (GdkPixbuf *pixbuf,
              int        max_width,
              int        max_height,
              gboolean   no_stretch_hint)
{
        int        pw;
        int        ph;
        float      scale_factor_x = 1.0;
        float      scale_factor_y = 1.0;
        float      scale_factor = 1.0;

        pw = gdk_pixbuf_get_width (pixbuf);
        ph = gdk_pixbuf_get_height (pixbuf);

        /* Determine which dimension requires the smallest scale. */
        scale_factor_x = (float) max_width / (float) pw;
        scale_factor_y = (float) max_height / (float) ph;

        if (scale_factor_x > scale_factor_y) {
                scale_factor = scale_factor_y;
        } else {
                scale_factor = scale_factor_x;
        }

        /* always scale down, allow to disable scaling up */
        if (scale_factor < 1.0 || !no_stretch_hint) {
                int scale_x = (int) (pw * scale_factor);
                int scale_y = (int) (ph * scale_factor);
                g_debug ("Scaling to %dx%d", scale_x, scale_y);
                return gdk_pixbuf_scale_simple (pixbuf,
                                                scale_x,
                                                scale_y,
                                                GDK_INTERP_BILINEAR);
        } else {
                return g_object_ref (pixbuf);
        }
}

#ifdef HAVE_XRENDER

/* adapted from marco */
static GdkColormap*
get_cmap (GdkPixmap *pixmap)
{
        GdkColormap *cmap;

        cmap = gdk_drawable_get_colormap (pixmap);
        if (cmap) {
                g_object_ref (G_OBJECT (cmap));
        }

        if (cmap == NULL) {
                if (gdk_drawable_get_depth (pixmap) == 1) {
                        g_debug ("Using NULL colormap for snapshotting bitmap\n");
                        cmap = NULL;
                } else {
                        g_debug ("Using system cmap to snapshot pixmap\n");
                        cmap = gdk_screen_get_system_colormap (gdk_drawable_get_screen (pixmap));

                        g_object_ref (G_OBJECT (cmap));
                }
        }

        /* Be sure we aren't going to blow up due to visual mismatch */
        if (cmap &&
            (gdk_visual_get_depth (gdk_colormap_get_visual (cmap)) !=
             gdk_drawable_get_depth (pixmap))) {
                cmap = NULL;
                g_debug ("Switching back to NULL cmap because of depth mismatch\n");
        }

        return cmap;
}

static GdkPixbuf *
pixbuf_get_from_pixmap (Pixmap xpixmap)
{
        GdkDrawable *drawable;
        GdkPixbuf   *retval;
        GdkColormap *cmap;
        int          width;
        int          height;

        retval = NULL;
        cmap = NULL;

        g_debug ("GsmInhibitDialog: getting foreign pixmap for %u", (guint)xpixmap);
        drawable = gdk_pixmap_foreign_new (xpixmap);
        if (GDK_IS_PIXMAP (drawable)) {
                cmap = get_cmap (drawable);

		#if GTK_CHECK_VERSION(3, 0, 0)
			width = gdk_window_get_width(drawable);
			height = gdk_window_get_height(drawable);
		#else
			gdk_drawable_get_size(drawable, &width, &height);
		#endif

                g_debug ("GsmInhibitDialog: getting pixbuf w=%d h=%d", width, height);

                retval = gdk_pixbuf_get_from_drawable (NULL,
                                                       drawable,
                                                       cmap,
                                                       0, 0,
                                                       0, 0,
                                                       width, height);
        }
        if (cmap) {
                g_object_unref (G_OBJECT (cmap));
        }
        if (drawable) {
                g_object_unref (G_OBJECT (drawable));
        }

        return retval;
}

static Pixmap
get_pixmap_for_window (Window window)
{
        XWindowAttributes        attr;
        XRenderPictureAttributes pa;
        Pixmap                   pixmap;
        XRenderPictFormat       *format;
        Picture                  src_picture;
        Picture                  dst_picture;
        gboolean                 has_alpha;
        int                      x;
        int                      y;
        int                      width;
        int                      height;

        XGetWindowAttributes (GDK_DISPLAY (), window, &attr);

        format = XRenderFindVisualFormat (GDK_DISPLAY (), attr.visual);
        has_alpha = (format->type == PictTypeDirect && format->direct.alphaMask);
        x = attr.x;
        y = attr.y;
        width = attr.width;
        height = attr.height;

        pa.subwindow_mode = IncludeInferiors; /* Don't clip child widgets */

        src_picture = XRenderCreatePicture (GDK_DISPLAY (), window, format, CPSubwindowMode, &pa);

        pixmap = XCreatePixmap (GDK_DISPLAY (),
                                window,
                                width, height,
                                attr.depth);

        dst_picture = XRenderCreatePicture (GDK_DISPLAY (), pixmap, format, 0, 0);
        XRenderComposite (GDK_DISPLAY (),
                          has_alpha ? PictOpOver : PictOpSrc,
                          src_picture,
                          None,
                          dst_picture,
                          0, 0, 0, 0,
                          0, 0,
                          width, height);


        return pixmap;
}

#endif /* HAVE_COMPOSITE */

static GdkPixbuf *
get_pixbuf_for_window (guint xid,
                       int   width,
                       int   height)
{
        GdkPixbuf *pixbuf = NULL;
#ifdef HAVE_XRENDER
        Window     xwindow;
        Pixmap     xpixmap;

        xwindow = (Window) xid;
        xpixmap = get_pixmap_for_window (xwindow);
        if (xpixmap == None) {
                g_debug ("GsmInhibitDialog: Unable to get window snapshot for %u", xid);
                return NULL;
        } else {
                g_debug ("GsmInhibitDialog: Got xpixmap %u", (guint)xpixmap);
        }

        pixbuf = pixbuf_get_from_pixmap (xpixmap);

        if (xpixmap != None) {
                gdk_error_trap_push ();
                XFreePixmap (GDK_DISPLAY (), xpixmap);
                gdk_display_sync (gdk_display_get_default ());
                gdk_error_trap_pop ();
        }

        if (pixbuf != NULL) {
                GdkPixbuf *scaled;
                g_debug ("GsmInhibitDialog: scaling pixbuf to w=%d h=%d", width, height);
                scaled = scale_pixbuf (pixbuf, width, height, TRUE);
                g_object_unref (pixbuf);
                pixbuf = scaled;
        }
#else
        g_debug ("GsmInhibitDialog: no support for getting window snapshot");
#endif
        return pixbuf;
}

static void
add_inhibitor (GsmInhibitDialog *dialog,
               GsmInhibitor     *inhibitor)
{
        const char     *name;
        const char     *icon_name;
        const char     *app_id;
        char           *desktop_filename;
        GdkPixbuf      *pixbuf;
        EggDesktopFile *desktop_file;
        GError         *error;
        char          **search_dirs;
        guint           xid;
        char           *freeme;

        /* FIXME: get info from xid */

        desktop_file = NULL;
        name = NULL;
        pixbuf = NULL;
        freeme = NULL;

        app_id = gsm_inhibitor_peek_app_id (inhibitor);

        if (IS_STRING_EMPTY (app_id)) {
                desktop_filename = NULL;
        } else if (! g_str_has_suffix (app_id, ".desktop")) {
                desktop_filename = g_strdup_printf ("%s.desktop", app_id);
        } else {
                desktop_filename = g_strdup (app_id);
        }

        xid = gsm_inhibitor_peek_toplevel_xid (inhibitor);
        g_debug ("GsmInhibitDialog: inhibitor has XID %u", xid);
        if (xid > 0 && dialog->priv->have_xrender) {
                pixbuf = get_pixbuf_for_window (xid, DEFAULT_SNAPSHOT_SIZE, DEFAULT_SNAPSHOT_SIZE);
                if (pixbuf == NULL) {
                        g_debug ("GsmInhibitDialog: unable to read pixbuf from %u", xid);
                }
        }

        if (desktop_filename != NULL) {
                search_dirs = gsm_util_get_desktop_dirs ();

                if (g_path_is_absolute (desktop_filename)) {
                        char *basename;

                        error = NULL;
                        desktop_file = egg_desktop_file_new (desktop_filename,
                                                             &error);
                        if (desktop_file == NULL) {
                                if (error) {
                                        g_warning ("Unable to load desktop file '%s': %s",
                                                   desktop_filename, error->message);
                                        g_error_free (error);
                                } else {
                                        g_warning ("Unable to load desktop file '%s'",
                                                   desktop_filename);
                                }

                                basename = g_path_get_basename (desktop_filename);
                                g_free (desktop_filename);
                                desktop_filename = basename;
                        }
                }

                if (desktop_file == NULL) {
                        error = NULL;
                        desktop_file = egg_desktop_file_new_from_dirs (desktop_filename,
                                                                       (const char **)search_dirs,
                                                                       &error);
                }

                /* look for a file with a vendor prefix */
                if (desktop_file == NULL) {
                        if (error) {
                                g_warning ("Unable to find desktop file '%s': %s",
                                           desktop_filename, error->message);
                                g_error_free (error);
                        } else {
                                g_warning ("Unable to find desktop file '%s'",
                                           desktop_filename);
                        }
                        g_free (desktop_filename);
                        desktop_filename = g_strdup_printf ("mate-%s.desktop", app_id);
                        error = NULL;
                        desktop_file = egg_desktop_file_new_from_dirs (desktop_filename,
                                                                       (const char **)search_dirs,
                                                                       &error);
                }
                g_strfreev (search_dirs);

                if (desktop_file == NULL) {
                        if (error) {
                                g_warning ("Unable to find desktop file '%s': %s",
                                           desktop_filename, error->message);
                                g_error_free (error);
                        } else {
                                g_warning ("Unable to find desktop file '%s'",
                                           desktop_filename);
                        }
                } else {
                        name = egg_desktop_file_get_name (desktop_file);
                        icon_name = egg_desktop_file_get_icon (desktop_file);

                        if (pixbuf == NULL) {
                                pixbuf = _load_icon (gtk_icon_theme_get_default (),
                                                     icon_name,
                                                     DEFAULT_ICON_SIZE,
                                                     DEFAULT_ICON_SIZE,
                                                     DEFAULT_ICON_SIZE,
                                                     NULL);
                        }
                }
        }

        /* try client info */
        if (name == NULL) {
                const char *client_id;
                client_id = gsm_inhibitor_peek_client_id (inhibitor);
                if (! IS_STRING_EMPTY (client_id)) {
                        GsmClient *client;
                        client = GSM_CLIENT (gsm_store_lookup (dialog->priv->clients, client_id));
                        if (client != NULL) {
                                freeme = gsm_client_get_app_name (client);
                                name = freeme;
                        }
                }
        }

        if (name == NULL) {
                if (! IS_STRING_EMPTY (app_id)) {
                        name = app_id;
                } else {
                        name = _("Unknown");
                }
        }

        if (pixbuf == NULL) {
                pixbuf = _load_icon (gtk_icon_theme_get_default (),
                                     "mate-windows",
                                     DEFAULT_ICON_SIZE,
                                     DEFAULT_ICON_SIZE,
                                     DEFAULT_ICON_SIZE,
                                     NULL);
        }

        gtk_list_store_insert_with_values (dialog->priv->list_store,
                                           NULL, 0,
                                           INHIBIT_IMAGE_COLUMN, pixbuf,
                                           INHIBIT_NAME_COLUMN, name,
                                           INHIBIT_REASON_COLUMN, gsm_inhibitor_peek_reason (inhibitor),
                                           INHIBIT_ID_COLUMN, gsm_inhibitor_peek_id (inhibitor),
                                           -1);

        g_free (desktop_filename);
        g_free (freeme);
        if (pixbuf != NULL) {
                g_object_unref (pixbuf);
        }
        if (desktop_file != NULL) {
                egg_desktop_file_free (desktop_file);
        }
}

static gboolean
model_has_one_entry (GtkTreeModel *model)
{
        guint n_rows;

        n_rows = gtk_tree_model_iter_n_children (model, NULL);
        g_debug ("Model has %d rows", n_rows);

        return (n_rows > 0 && n_rows < 2);
}

static void
update_dialog_text (GsmInhibitDialog *dialog)
{
        const char *description_text;
        const char *header_text;
        GtkWidget  *widget;

        if (model_has_one_entry (GTK_TREE_MODEL (dialog->priv->list_store))) {
                g_debug ("Found one entry in model");
                header_text = _("A program is still running:");
                description_text = _("Waiting for the program to finish.  Interrupting the program may cause you to lose work.");
        } else {
                g_debug ("Found multiple entries (or none) in model");
                header_text = _("Some programs are still running:");
                description_text = _("Waiting for programs to finish.  Interrupting these programs may cause you to lose work.");
        }

        widget = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     "header-label"));
        if (widget != NULL) {
                char *markup;
                markup = g_strdup_printf ("<b>%s</b>", header_text);
                gtk_label_set_markup (GTK_LABEL (widget), markup);
                g_free (markup);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     "description-label"));
        if (widget != NULL) {
                gtk_label_set_text (GTK_LABEL (widget), description_text);
        }
}

static void
on_store_inhibitor_added (GsmStore          *store,
                          const char        *id,
                          GsmInhibitDialog  *dialog)
{
        GsmInhibitor *inhibitor;
        GtkTreeIter   iter;

        g_debug ("GsmInhibitDialog: inhibitor added: %s", id);

        if (dialog->priv->is_done) {
                return;
        }

        inhibitor = (GsmInhibitor *)gsm_store_lookup (store, id);

        /* Add to model */
        if (! find_inhibitor (dialog, id, &iter)) {
                add_inhibitor (dialog, inhibitor);
                update_dialog_text (dialog);
        }

}

static void
on_store_inhibitor_removed (GsmStore          *store,
                            const char        *id,
                            GsmInhibitDialog  *dialog)
{
        GtkTreeIter   iter;

        g_debug ("GsmInhibitDialog: inhibitor removed: %s", id);

        if (dialog->priv->is_done) {
                return;
        }

        /* Remove from model */
        if (find_inhibitor (dialog, id, &iter)) {
                gtk_list_store_remove (dialog->priv->list_store, &iter);
                update_dialog_text (dialog);
        }

        /* if there are no inhibitors left then trigger response */
        if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->priv->list_store), &iter)) {
                gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
        }
}

static void
gsm_inhibit_dialog_set_inhibitor_store (GsmInhibitDialog *dialog,
                                        GsmStore         *store)
{
        g_return_if_fail (GSM_IS_INHIBIT_DIALOG (dialog));

        if (store != NULL) {
                g_object_ref (store);
        }

        if (dialog->priv->inhibitors != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->inhibitors,
                                                      on_store_inhibitor_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->inhibitors,
                                                      on_store_inhibitor_removed,
                                                      dialog);

                g_object_unref (dialog->priv->inhibitors);
        }


        g_debug ("GsmInhibitDialog: setting store %p", store);

        dialog->priv->inhibitors = store;

        if (dialog->priv->inhibitors != NULL) {
                g_signal_connect (dialog->priv->inhibitors,
                                  "added",
                                  G_CALLBACK (on_store_inhibitor_added),
                                  dialog);
                g_signal_connect (dialog->priv->inhibitors,
                                  "removed",
                                  G_CALLBACK (on_store_inhibitor_removed),
                                  dialog);
        }
}

static void
gsm_inhibit_dialog_set_client_store (GsmInhibitDialog *dialog,
                                     GsmStore         *store)
{
        g_return_if_fail (GSM_IS_INHIBIT_DIALOG (dialog));

        if (store != NULL) {
                g_object_ref (store);
        }

        if (dialog->priv->clients != NULL) {
                g_object_unref (dialog->priv->clients);
        }

        dialog->priv->clients = store;
}

static void
gsm_inhibit_dialog_set_property (GObject        *object,
                                 guint           prop_id,
                                 const GValue   *value,
                                 GParamSpec     *pspec)
{
        GsmInhibitDialog *dialog = GSM_INHIBIT_DIALOG (object);

        switch (prop_id) {
        case PROP_ACTION:
                gsm_inhibit_dialog_set_action (dialog, g_value_get_int (value));
                break;
        case PROP_INHIBITOR_STORE:
                gsm_inhibit_dialog_set_inhibitor_store (dialog, g_value_get_object (value));
                break;
        case PROP_CLIENT_STORE:
                gsm_inhibit_dialog_set_client_store (dialog, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_inhibit_dialog_get_property (GObject        *object,
                                 guint           prop_id,
                                 GValue         *value,
                                 GParamSpec     *pspec)
{
        GsmInhibitDialog *dialog = GSM_INHIBIT_DIALOG (object);

        switch (prop_id) {
        case PROP_ACTION:
                g_value_set_int (value, dialog->priv->action);
                break;
        case PROP_INHIBITOR_STORE:
                g_value_set_object (value, dialog->priv->inhibitors);
                break;
        case PROP_CLIENT_STORE:
                g_value_set_object (value, dialog->priv->clients);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
name_cell_data_func (GtkTreeViewColumn *tree_column,
                     GtkCellRenderer   *cell,
                     GtkTreeModel      *model,
                     GtkTreeIter       *iter,
                     GsmInhibitDialog  *dialog)
{
        char    *name;
        char    *reason;
        char    *markup;

        name = NULL;
        reason = NULL;
        gtk_tree_model_get (model,
                            iter,
                            INHIBIT_NAME_COLUMN, &name,
                            INHIBIT_REASON_COLUMN, &reason,
                            -1);

        markup = g_strdup_printf ("<b>%s</b>\n"
                                  "<span size=\"small\">%s</span>",
                                  name ? name : "(null)",
                                  reason ? reason : "(null)");

        g_free (name);
        g_free (reason);

        g_object_set (cell, "markup", markup, NULL);
        g_free (markup);
}

static gboolean
add_to_model (const char       *id,
              GsmInhibitor     *inhibitor,
              GsmInhibitDialog *dialog)
{
        add_inhibitor (dialog, inhibitor);
        return FALSE;
}

static void
populate_model (GsmInhibitDialog *dialog)
{
        gsm_store_foreach_remove (dialog->priv->inhibitors,
                                  (GsmStoreFunc)add_to_model,
                                  dialog);
        update_dialog_text (dialog);
}

static void
setup_dialog (GsmInhibitDialog *dialog)
{
        const char        *button_text;
        GtkWidget         *treeview;
        GtkTreeViewColumn *column;
        GtkCellRenderer   *renderer;

        switch (dialog->priv->action) {
        case GSM_LOGOUT_ACTION_SWITCH_USER:
                button_text = _("Switch User Anyway");
                break;
        case GSM_LOGOUT_ACTION_LOGOUT:
                button_text = _("Log Out Anyway");
                break;
        case GSM_LOGOUT_ACTION_SLEEP:
                button_text = _("Suspend Anyway");
                break;
        case GSM_LOGOUT_ACTION_HIBERNATE:
                button_text = _("Hibernate Anyway");
                break;
        case GSM_LOGOUT_ACTION_SHUTDOWN:
                button_text = _("Shut Down Anyway");
                break;
        case GSM_LOGOUT_ACTION_REBOOT:
                button_text = _("Reboot Anyway");
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               _("Lock Screen"),
                               DIALOG_RESPONSE_LOCK_SCREEN);
        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               _("Cancel"),
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               button_text,
                               GTK_RESPONSE_ACCEPT);
        g_signal_connect (dialog,
                          "response",
                          G_CALLBACK (on_response),
                          dialog);

        dialog->priv->list_store = gtk_list_store_new (NUMBER_OF_COLUMNS,
                                                       GDK_TYPE_PIXBUF,
                                                       G_TYPE_STRING,
                                                       G_TYPE_STRING,
                                                       G_TYPE_STRING);

        treeview = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                       "inhibitors-treeview"));
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (dialog->priv->list_store));

        /* IMAGE COLUMN */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        gtk_tree_view_column_set_attributes (column,
                                             renderer,
                                             "pixbuf", INHIBIT_IMAGE_COLUMN,
                                             NULL);

        g_object_set (renderer, "xalign", 1.0, NULL);

        /* NAME COLUMN */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 (GtkTreeCellDataFunc) name_cell_data_func,
                                                 dialog,
                                                 NULL);

        gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (treeview),
                                          INHIBIT_REASON_COLUMN);

        populate_model (dialog);
}

static GObject *
gsm_inhibit_dialog_constructor (GType                  type,
                                guint                  n_construct_properties,
                                GObjectConstructParam *construct_properties)
{
        GsmInhibitDialog *dialog;

        dialog = GSM_INHIBIT_DIALOG (G_OBJECT_CLASS (gsm_inhibit_dialog_parent_class)->constructor (type,
                                                                                                                  n_construct_properties,
                                                                                                                  construct_properties));

#ifdef HAVE_XRENDER
        gdk_error_trap_push ();
        if (XRenderQueryExtension (GDK_DISPLAY (), &dialog->priv->xrender_event_base, &dialog->priv->xrender_error_base)) {
                g_debug ("GsmInhibitDialog: Initialized XRender extension");
                dialog->priv->have_xrender = TRUE;
        } else {
                g_debug ("GsmInhibitDialog: Unable to initialize XRender extension");
                dialog->priv->have_xrender = FALSE;
        }
        gdk_display_sync (gdk_display_get_default ());
        gdk_error_trap_pop ();
#endif /* HAVE_XRENDER */

        /* FIXME: turn this on when it is ready */
        dialog->priv->have_xrender = FALSE;

        setup_dialog (dialog);

        gtk_widget_show_all (GTK_WIDGET (dialog));

        return G_OBJECT (dialog);
}

static void
gsm_inhibit_dialog_dispose (GObject *object)
{
        GsmInhibitDialog *dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_INHIBIT_DIALOG (object));

        dialog = GSM_INHIBIT_DIALOG (object);

        g_debug ("GsmInhibitDialog: dispose called");

        if (dialog->priv->list_store != NULL) {
                g_object_unref (dialog->priv->list_store);
                dialog->priv->list_store = NULL;
        }

        if (dialog->priv->inhibitors != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->inhibitors,
                                                      on_store_inhibitor_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->inhibitors,
                                                      on_store_inhibitor_removed,
                                                      dialog);

                g_object_unref (dialog->priv->inhibitors);
                dialog->priv->inhibitors = NULL;
        }

        if (dialog->priv->xml != NULL) {
                g_object_unref (dialog->priv->xml);
                dialog->priv->xml = NULL;
        }

        G_OBJECT_CLASS (gsm_inhibit_dialog_parent_class)->dispose (object);
}

static void
gsm_inhibit_dialog_class_init (GsmInhibitDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsm_inhibit_dialog_get_property;
        object_class->set_property = gsm_inhibit_dialog_set_property;
        object_class->constructor = gsm_inhibit_dialog_constructor;
        object_class->dispose = gsm_inhibit_dialog_dispose;
        object_class->finalize = gsm_inhibit_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_ACTION,
                                         g_param_spec_int ("action",
                                                           "action",
                                                           "action",
                                                           -1,
                                                           G_MAXINT,
                                                           -1,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_INHIBITOR_STORE,
                                         g_param_spec_object ("inhibitor-store",
                                                              NULL,
                                                              NULL,
                                                              GSM_TYPE_STORE,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_CLIENT_STORE,
                                         g_param_spec_object ("client-store",
                                                              NULL,
                                                              NULL,
                                                              GSM_TYPE_STORE,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GsmInhibitDialogPrivate));
}

static void
gsm_inhibit_dialog_init (GsmInhibitDialog *dialog)
{
        GtkWidget *content_area;
        GtkWidget *widget;
        GError    *error;

        dialog->priv = GSM_INHIBIT_DIALOG_GET_PRIVATE (dialog);

        dialog->priv->xml = gtk_builder_new ();
        gtk_builder_set_translation_domain (dialog->priv->xml, GETTEXT_PACKAGE);

        error = NULL;
        if (!gtk_builder_add_from_file (dialog->priv->xml,
                                        GTKBUILDER_DIR "/" GTKBUILDER_FILE,
                                        &error)) {
                if (error) {
                        g_warning ("Could not load inhibitor UI file: %s",
                                   error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Could not load inhibitor UI file.");
                }
        }

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        widget = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     "main-box"));
        gtk_container_add (GTK_CONTAINER (content_area), widget);

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
        gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "system-log-out");
        gtk_window_set_title (GTK_WINDOW (dialog), "");
        g_object_set (dialog,
                      "allow-shrink", FALSE,
                      "allow-grow", FALSE,
                      NULL);
}

static void
gsm_inhibit_dialog_finalize (GObject *object)
{
        GsmInhibitDialog *dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_INHIBIT_DIALOG (object));

        dialog = GSM_INHIBIT_DIALOG (object);

        g_return_if_fail (dialog->priv != NULL);

        g_debug ("GsmInhibitDialog: finalizing");

        G_OBJECT_CLASS (gsm_inhibit_dialog_parent_class)->finalize (object);
}

GtkWidget *
gsm_inhibit_dialog_new (GsmStore *inhibitors,
                        GsmStore *clients,
                        int       action)
{
        GObject *object;

        object = g_object_new (GSM_TYPE_INHIBIT_DIALOG,
                               "action", action,
                               "inhibitor-store", inhibitors,
                               "client-store", clients,
                               NULL);

        return GTK_WIDGET (object);
}
