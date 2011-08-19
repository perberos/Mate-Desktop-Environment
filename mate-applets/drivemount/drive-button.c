/* -*- mode: C; c-basic-offset: 4 -*-
 * Drive Mount Applet
 * Copyright (c) 2004 Canonical Ltd
 * Copyright 2008 Pierre Ossman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 *   James Henstridge <jamesh@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gio/gio.h>
#include "drive-button.h"
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <mateconf/mateconf-client.h>

#include <string.h>

enum {
    CMD_NONE,
    CMD_MOUNT_OR_PLAY,
    CMD_UNMOUNT,
    CMD_EJECT
};

#define MATECONF_ROOT_AUTOPLAY  "/desktop/mate/volume_manager/"

/* type registration boilerplate code */
G_DEFINE_TYPE(DriveButton, drive_button, GTK_TYPE_BUTTON)

static void     drive_button_set_volume   (DriveButton    *self,
				           GVolume        *volume);
static void     drive_button_set_mount    (DriveButton    *self,
				           GMount         *mount);
static void     drive_button_reset_popup  (DriveButton    *self);
static void     drive_button_ensure_popup (DriveButton    *self);

static void     drive_button_destroy      (GtkObject      *object);
#if 0
static void     drive_button_unrealize    (GtkWidget      *widget);
#endif /* 0 */
static gboolean drive_button_button_press (GtkWidget      *widget,
					   GdkEventButton *event);
static gboolean drive_button_key_press    (GtkWidget      *widget,
					   GdkEventKey    *event);
static void drive_button_theme_change     (GtkIconTheme   *icon_theme,
					   gpointer        data);

static void
drive_button_class_init (DriveButtonClass *class)
{
    GTK_OBJECT_CLASS(class)->destroy = drive_button_destroy;
    GTK_WIDGET_CLASS(class)->button_press_event = drive_button_button_press;
    GTK_WIDGET_CLASS(class)->key_press_event = drive_button_key_press;

    gtk_rc_parse_string ("\n"
			 "   style \"drive-button-style\"\n"
			 "   {\n"
			 "      GtkWidget::focus-line-width=0\n"
			 "      GtkWidget::focus-padding=0\n"
			 "      bg_pixmap[NORMAL] = \"<parent>\"\n"
			 "      bg_pixmap[ACTIVE] = \"<parent>\"\n"
			 "      bg_pixmap[PRELIGHT] = \"<parent>\"\n"
			 "      bg_pixmap[INSENSITIVE] = \"<parent>\"\n"
			 "   }\n"
			 "\n"
			 "    class \"DriveButton\" style \"drive-button-style\"\n"
			 "\n");
}

static void
drive_button_init (DriveButton *self)
{
    GtkWidget *image;

    image = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (self), image);
    gtk_widget_show(image);

    self->volume = NULL;
    self->mount = NULL;
    self->icon_size = 24;
    self->update_tag = 0;

    self->popup_menu = NULL;
}

GtkWidget *
drive_button_new (GVolume *volume)
{
    DriveButton *self;

    self = g_object_new (DRIVE_TYPE_BUTTON, NULL);
    drive_button_set_volume (self, volume);
    
    g_signal_connect (gtk_icon_theme_get_default (),
        "changed", G_CALLBACK (drive_button_theme_change),
        self);  

    return (GtkWidget *)self;
}

GtkWidget *
drive_button_new_from_mount (GMount *mount)
{
    DriveButton *self;

    self = g_object_new (DRIVE_TYPE_BUTTON, NULL);
    drive_button_set_mount (self, mount);
    
    g_signal_connect (gtk_icon_theme_get_default (),
        "changed", G_CALLBACK (drive_button_theme_change),
        self);      

    return (GtkWidget *)self;
}

static void
drive_button_destroy (GtkObject *object)
{
    DriveButton *self = DRIVE_BUTTON (object);

    drive_button_set_volume (self, NULL);

    if (self->update_tag)
	g_source_remove (self->update_tag);
    self->update_tag = 0;

    drive_button_reset_popup (self);

    if (GTK_OBJECT_CLASS (drive_button_parent_class)->destroy)
	(* GTK_OBJECT_CLASS (drive_button_parent_class)->destroy) (object);
}

#if 0
static void
drive_button_unrealize (GtkWidget *widget)
{
    DriveButton *self = DRIVE_BUTTON (widget);

    drive_button_reset_popup (self);

    if (GTK_WIDGET_CLASS (drive_button_parent_class)->unrealize)
	(* GTK_WIDGET_CLASS (drive_button_parent_class)->unrealize) (widget);
}
#endif /* 0 */

/* the following function is adapted from gtkmenuitem.c */
static void
position_menu (GtkMenu *menu, gint *x, gint *y,
	       gboolean *push_in, gpointer user_data)
{
    GtkWidget *widget = GTK_WIDGET (user_data);
    GdkScreen *screen;
    gint twidth, theight, tx, ty;
    GtkAllocation allocation;
    GtkRequisition requisition;
    GtkTextDirection direction;
    GdkRectangle monitor;
    gint monitor_num;

    g_return_if_fail (menu != NULL);
    g_return_if_fail (x != NULL);
    g_return_if_fail (y != NULL);

    if (push_in) *push_in = FALSE;

    direction = gtk_widget_get_direction (widget);

    gtk_widget_get_requisition (GTK_WIDGET (menu), &requisition);
    twidth = requisition.width;
    theight = requisition.height;

    screen = gtk_widget_get_screen (GTK_WIDGET (menu));
    monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget));
    if (monitor_num < 0) monitor_num = 0;
    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

    if (!gdk_window_get_origin (gtk_widget_get_window (widget), &tx, &ty)) {
	g_warning ("Menu not on screen");
	return;
    }

    gtk_widget_get_allocation (widget, &allocation);
    tx += allocation.x;
    ty += allocation.y;

    if (direction == GTK_TEXT_DIR_RTL)
	tx += allocation.width - twidth;

    if ((ty + allocation.height + theight) <= monitor.y + monitor.height)
	ty += allocation.height;
    else if ((ty - theight) >= monitor.y)
	ty -= theight;
    else if (monitor.y + monitor.height - (ty + allocation.height) > ty)
	ty += allocation.height;
    else
	ty -= theight;

    *x = CLAMP (tx, monitor.x, MAX (monitor.x, monitor.x + monitor.width - twidth));
    *y = ty;
    gtk_menu_set_monitor (menu, monitor_num);
}

static gboolean
drive_button_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
    DriveButton *self = DRIVE_BUTTON (widget);

    /* don't consume non-button1 presses */
    if (event->button == 1) {
	drive_button_ensure_popup (self);
	if (self->popup_menu) {
	    gtk_menu_popup (GTK_MENU (self->popup_menu), NULL, NULL,
			    position_menu, self,
			    event->button, event->time);
	}
	return TRUE;
    }
    return FALSE;
}

static gboolean
drive_button_key_press (GtkWidget      *widget,
			GdkEventKey    *event)
{
    DriveButton *self = DRIVE_BUTTON (widget);

    switch (event->keyval) {
    case GDK_KP_Space:
    case GDK_space:
    case GDK_KP_Enter:
    case GDK_Return:
	drive_button_ensure_popup (self);
	if (self->popup_menu) {
	    gtk_menu_popup (GTK_MENU (self->popup_menu), NULL, NULL,
			    position_menu, self,
			    0, event->time);
	}
	return TRUE;
    }
    return FALSE;
}

static void
drive_button_theme_change (GtkIconTheme *icon_theme, gpointer data)
{
    drive_button_queue_update (data);
}

static void
drive_button_set_volume (DriveButton *self, GVolume *volume)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->volume) {
	g_object_unref (self->volume);
    }
    self->volume = NULL;
    if (self->mount) {
	g_object_unref (self->mount);
    }
    self->mount = NULL;

    if (volume) {
	self->volume = g_object_ref (volume);
    }
    drive_button_queue_update (self);
}

static void
drive_button_set_mount (DriveButton *self, GMount *mount)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->volume) {
	g_object_unref (self->volume);
    }
    self->volume = NULL;
    if (self->mount) {
	g_object_unref (self->mount);
    }
    self->mount = NULL;

    if (mount) {
	self->mount = g_object_ref (mount);
    }
    drive_button_queue_update (self);
}

static gboolean
drive_button_update (gpointer user_data)
{
    DriveButton *self;
    GdkScreen *screen;
    GtkIconTheme *icon_theme;
    GtkIconInfo *icon_info;
    GIcon *icon;
    int width, height;
    GdkPixbuf *pixbuf = NULL, *scaled;
    GtkRequisition button_req, image_req;
    char *display_name, *tip;

    g_return_val_if_fail (DRIVE_IS_BUTTON (user_data), FALSE);
    self = DRIVE_BUTTON (user_data);
    self->update_tag = 0;

    drive_button_reset_popup (self);

    /* if no volume or mount, unset image */
    if (!self->volume && !self->mount) {
	if (gtk_bin_get_child (GTK_BIN (self)) != NULL)
	    gtk_image_set_from_pixbuf (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (self))), NULL);
	return FALSE;
    }

    if (self->volume) {
	GMount *mount;

	display_name = g_volume_get_name (self->volume);
	mount = g_volume_get_mount (self->volume);

	if (mount)
	    tip = g_strdup_printf ("%s\n%s", display_name, _("(mounted)"));
	else
	    tip = g_strdup_printf ("%s\n%s", display_name, _("(not mounted)"));

	if (mount)
	    icon = g_mount_get_icon (mount);
	else
	    icon = g_volume_get_icon (self->volume);

	if (mount)
	    g_object_unref (mount);
    } else {
	display_name = g_mount_get_name (self->mount);
	tip = g_strdup_printf ("%s\n%s", display_name, _("(mounted)"));
	icon = g_mount_get_icon (self->mount);
    }

    gtk_widget_set_tooltip_text (GTK_WIDGET (self), tip);
    g_free (tip);
    g_free (display_name);

    /* base the icon size on the desired button size */
    gtk_widget_size_request (GTK_WIDGET (self), &button_req);
    gtk_widget_size_request (gtk_bin_get_child (GTK_BIN (self)), &image_req);
    width = self->icon_size - (button_req.width - image_req.width);
    height = self->icon_size - (button_req.height - image_req.height);

    screen = gtk_widget_get_screen (GTK_WIDGET (self));
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme, icon,
    						MIN (width, height),
    						GTK_ICON_LOOKUP_USE_BUILTIN);
    if (icon_info)
    {
	pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
	gtk_icon_info_free (icon_info);
    }

    g_object_unref (icon);

    if (!pixbuf)
	return FALSE;

    scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
    if (scaled) {
	g_object_unref (pixbuf);
	pixbuf = scaled;
    }

    gtk_image_set_from_pixbuf (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (self))), pixbuf);
    g_object_unref (pixbuf);

    gtk_widget_size_request (GTK_WIDGET (self), &button_req);

    return FALSE;
}

void
drive_button_queue_update (DriveButton *self)
{
    if (!self->update_tag) {
	self->update_tag = g_idle_add (drive_button_update, self);
    }
}

void
drive_button_set_size (DriveButton *self, int icon_size)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->icon_size != icon_size) {
	self->icon_size = icon_size;
	drive_button_queue_update (self);
    }
}

int
drive_button_compare (DriveButton *button, DriveButton *other_button)
{
    /* sort drives before driveless volumes volumes */
    if (button->volume) {
	if (other_button->volume)
	{
	    int cmp;
	    gchar *str1, *str2;

	    str1 = g_volume_get_name (button->volume);
	    str2 = g_volume_get_name (other_button->volume);
	    cmp = g_utf8_collate (str1, str2);
	    g_free (str2);
	    g_free (str1);

	    return cmp;
	} else {
	    return -1;
	}
    } else {
	if (other_button->volume)
	{
	    return 1;
	} else {
	    int cmp;
	    gchar *str1, *str2;

	    str1 = g_mount_get_name (button->mount);
	    str2 = g_mount_get_name (other_button->mount);
	    cmp = g_utf8_collate (str1, str2);
	    g_free (str2);
	    g_free (str1);

	    return cmp;
	}
    }
}

static void
drive_button_reset_popup (DriveButton *self)
{
    if (self->popup_menu)
	gtk_object_destroy (GTK_OBJECT (self->popup_menu));
    self->popup_menu = NULL;
}

#if 0
static void
popup_menu_detach (GtkWidget *attach_widget, GtkMenu *menu)
{
    DRIVE_BUTTON (attach_widget)->popup_menu = NULL;
}
#endif /* 0 */

static char *
escape_underscores (const char *str)
{
    char *new_str;
    int i, j, count;

    /* count up how many underscores are in the string */
    count = 0;
    for (i = 0; str[i] != '\0'; i++) {
	if (str[i] == '_')
	    count++;
    }
    /* copy to new string, doubling up underscores */
    new_str = g_new (char, i + count + 1);
    for (i = j = 0; str[i] != '\0'; i++, j++) {
	new_str[j] = str[i];
	if (str[i] == '_')
	    new_str[++j] = '_';
    }
    new_str[j] = '\0';
    return new_str;
}
static GtkWidget *
create_menu_item (DriveButton *self, const gchar *icon_name,
		  const gchar *label, GCallback callback,
		  gboolean sensitive)
{
    GtkWidget *item, *image;

    item = gtk_image_menu_item_new_with_mnemonic (label);
    if (icon_name) {
	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_widget_show (image);
    }
    if (callback)
	g_signal_connect_object (item, "activate", callback, self,
				 G_CONNECT_SWAPPED);
    gtk_widget_set_sensitive (item, sensitive);
    gtk_widget_show (item);
    return item;
}

static void
open_drive (DriveButton *self, GtkWidget *item)
{
    GdkScreen *screen;
    GtkWidget *dialog;
    GError *error = NULL;
    char *argv[3] = { "caja", NULL, NULL };

    screen = gtk_widget_get_screen (GTK_WIDGET (self));

    if (self->volume) {
	GMount *mount;

	mount = g_volume_get_mount (self->volume);
	if (mount) {
	    GFile *file;

	    file = g_mount_get_root (mount);

	    argv[1] = g_file_get_uri (file);
	    g_object_unref(file);

	    g_object_unref(mount);
	}
    } else if (self->mount) {
	GFile *file;

	file = g_mount_get_root (self->mount);
	argv[1] = g_file_get_uri (file);
	g_object_unref(file);
    } else
	g_return_if_reached();

    if (!gdk_spawn_on_screen (screen, NULL, argv, NULL,
			      G_SPAWN_SEARCH_PATH,
			      NULL, NULL, NULL, &error)) {
	dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     _("Cannot execute '%s'"),
						     argv[0]);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), error->message, NULL);
	g_signal_connect (dialog, "response",
			  G_CALLBACK (gtk_object_destroy), NULL);
	gtk_widget_show (dialog);
	g_error_free (error);
    }
    g_free (argv[1]);
}

/* copied from mate-volume-manager/src/manager.c maybe there is a better way than
 * duplicating this code? */

/*
 * gvm_run_command - run the given command, replacing %d with the device node
 * and %m with the given path
 */
static void
gvm_run_command (const char *device, const char *command, const char *path)
{
	char *argv[4];
	gchar *new_command;
	GError *error = NULL;
	GString *exec = g_string_new (NULL);
	char *p, *q;

	/* perform s/%d/device/ and s/%m/path/ */
	new_command = g_strdup (command);
	q = new_command;
	p = new_command;
	while ((p = strchr (p, '%')) != NULL) {
		if (*(p + 1) == 'd') {
			*p = '\0';
			g_string_append (exec, q);
			g_string_append (exec, device);
			q = p + 2;
			p = p + 2;
		} else if (*(p + 1) == 'm') {
			*p = '\0';
			g_string_append (exec, q);
			g_string_append (exec, path);
			q = p + 2;
			p = p + 2;
		} else {
		        /* Ignore anything else. */
		        p++;
		}
	}
	g_string_append (exec, q);

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = exec->str;
	argv[3] = NULL;

	g_spawn_async (g_get_home_dir (), argv, NULL, 0, NULL, NULL,
		       NULL, &error);
	if (error) {
		g_warning ("failed to exec %s: %s\n", exec->str, error->message);
		g_error_free (error);
	}

	g_string_free (exec, TRUE);
	g_free (new_command);
}

/*
 * gvm_check_dvd_only - is this a Video DVD?
 *
 * Returns TRUE if this was a Video DVD and FALSE otherwise.
 * (the original in gvm was also running the autoplay action,
 * I removed that code, so I renamed from gvm_check_dvd to
 * gvm_check_dvd_only)
 */
static gboolean
gvm_check_dvd_only (const char *udi, const char *device, const char *mount_point)
{
	char *path;
	gboolean retval;
	
	path = g_build_path (G_DIR_SEPARATOR_S, mount_point, "video_ts", NULL);
	retval = g_file_test (path, G_FILE_TEST_IS_DIR);
	g_free (path);
	
	/* try the other name, if needed */
	if (retval == FALSE) {
		path = g_build_path (G_DIR_SEPARATOR_S, mount_point,
				     "VIDEO_TS", NULL);
		retval = g_file_test (path, G_FILE_TEST_IS_DIR);
		g_free (path);
	}
	
	return retval;
}
/* END copied from mate-volume-manager/src/manager.c */

static gboolean
check_dvd_video (DriveButton *self)
{
	GFile *file;
	char *udi, *device_path, *mount_path;
	gboolean result;
	GMount *mount;

	if (!self->volume)
		return FALSE;

	mount = g_volume_get_mount (self->volume);
	if (!mount)
	    return FALSE;

	file = g_mount_get_root (mount);
	g_object_unref (mount);
	
	if (!file)
		return FALSE;

	mount_path = g_file_get_path (file);

	g_object_unref (file);

	device_path = g_volume_get_identifier (self->volume,
					       G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	udi = g_volume_get_identifier (self->volume,
				       G_VOLUME_IDENTIFIER_KIND_HAL_UDI);

	result = gvm_check_dvd_only (udi, device_path, mount_path);

	g_free (device_path);
	g_free (udi);
	g_free (mount_path);

	return result;
}

static gboolean
check_audio_cd (DriveButton *self)
{
	GFile *file;
	char *activation_uri;
	GMount *mount;

	if (!self->volume)
		return FALSE;

	mount = g_volume_get_mount (self->volume);
	if (!mount)
	    return FALSE;

	file = g_mount_get_root (mount);
	g_object_unref (mount);

	if (!file)
		return FALSE;

	activation_uri = g_file_get_uri (file);

	g_object_unref (file);

	/* we have an audioCD if the activation URI starts by 'cdda://' */
	gboolean result = (strncmp ("cdda://", activation_uri, 7) == 0);
	g_free (activation_uri);
	return result;
}

static void
run_command (DriveButton *self, const char *command)
{
	GFile *file;
	char *mount_path, *device_path;
	GMount *mount;

	if (!self->volume)
		return;

	mount = g_volume_get_mount (self->volume);
	if (!mount)
	    return;

	file = g_mount_get_root (mount);
	g_object_unref (mount);

	g_assert (file);

	mount_path = g_file_get_path (file);

	g_object_unref (file);

	device_path = g_volume_get_identifier (self->volume,
					       G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

	gvm_run_command (device_path, command, mount_path);

	g_free (mount_path);
	g_free (device_path);
}

static void
mount_drive (DriveButton *self, GtkWidget *item)
{
    if (self->volume) {
        GMountOperation *mount_op = gtk_mount_operation_new (NULL);
	g_volume_mount (self->volume, G_MOUNT_MOUNT_NONE,
			mount_op, NULL, NULL, NULL);
        g_object_unref (mount_op);
    } else {
	g_return_if_reached();
    }
}
static void
unmount_drive (DriveButton *self, GtkWidget *item)
{
    if (self->volume) {
	GMount *mount;

	mount = g_volume_get_mount (self->volume);
	if (mount)
	{
	    g_mount_unmount_with_operation (mount, G_MOUNT_UNMOUNT_NONE,
	                                    NULL, NULL, NULL, NULL);
	    g_object_unref (mount);
	}
    } else if (self->mount) {
	g_mount_unmount_with_operation (self->mount, G_MOUNT_UNMOUNT_NONE,
			                NULL, NULL, NULL, NULL);
    } else {
	g_return_if_reached();
    }
}

static void eject_finish (DriveButton *self, GAsyncResult *res,
			  gpointer user_data)
{
    /* Do nothing. We shouldn't need this according to the GIO
     * docs, but the applet crashes without it using glib 2.18.0 */
}

static void
eject_drive (DriveButton *self, GtkWidget *item)
{
    if (self->volume) {
	g_volume_eject_with_operation (self->volume, G_MOUNT_UNMOUNT_NONE,
	                               NULL, NULL,
	                               (GAsyncReadyCallback) eject_finish,
	                               NULL);
    } else if (self->mount) {
	g_mount_eject_with_operation (self->mount, G_MOUNT_UNMOUNT_NONE,
	                              NULL, NULL,
	                              (GAsyncReadyCallback) eject_finish,
	                              NULL);
    } else {
	g_return_if_reached();
    }
}
static void
play_autoplay_media (DriveButton *self, const char *autoplay_key, 
		     const char *dflt)
{
	MateConfClient *mateconf_client = mateconf_client_get_default ();
	char *command = mateconf_client_get_string (mateconf_client,
			autoplay_key, NULL);

	if (!command)
	    command = g_strdup (dflt);

	run_command (self, command);

	g_free (command);
	g_object_unref (mateconf_client);
}

static void
play_dvd (DriveButton *self, GtkWidget *item)
{
        play_autoplay_media (self, MATECONF_ROOT_AUTOPLAY "autoplay_dvd_command",
			     "totem %d");
}

static void
play_cda (DriveButton *self, GtkWidget *item)
{
        play_autoplay_media (self, MATECONF_ROOT_AUTOPLAY "autoplay_cda_command",
			     "sound-juicer -d %d");
}

static void
drive_button_ensure_popup (DriveButton *self)
{
    char *display_name, *tmp, *label;
    GtkWidget *item;
    gboolean mounted, ejectable;

    if (self->popup_menu) return;

    mounted = FALSE;

    if (self->volume) {
	GMount *mount = NULL;

	display_name = g_volume_get_name (self->volume);
	ejectable = g_volume_can_eject (self->volume);

	mount = g_volume_get_mount (self->volume);
	if (mount) {
	    mounted = TRUE;
	    g_object_unref (mount);
	}
    } else {
	display_name = g_mount_get_name (self->mount);
	ejectable = g_mount_can_eject (self->mount);
	mounted = TRUE;
    }

    self->popup_menu = gtk_menu_new ();

    /* make sure the display name doesn't look like a mnemonic */
    tmp = escape_underscores (display_name ? display_name : "(none)");
    g_free (display_name);
    display_name = tmp;

    if (check_dvd_video (self)) {
	item = create_menu_item (self, "media-playback-start",
				 _("_Play DVD"), G_CALLBACK (play_dvd),
				 TRUE);
    } else if (check_audio_cd (self)) {
	item = create_menu_item (self, "media-playback-start",
				 _("_Play CD"), G_CALLBACK (play_cda),
				 TRUE);
    } else {
	label = g_strdup_printf (_("_Open %s"), display_name);
	item = create_menu_item (self, "document-open", label,
			     G_CALLBACK (open_drive), mounted);
	g_free (label);
    }
    gtk_container_add (GTK_CONTAINER (self->popup_menu), item);

    if (mounted) {
	if (!ejectable) {
	    label = g_strdup_printf (_("Un_mount %s"), display_name);
	    item = create_menu_item (self, NULL, label,
				     G_CALLBACK (unmount_drive), TRUE);
	    g_free (label);
	    gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
	}
    } else {
	label = g_strdup_printf (_("_Mount %s"), display_name);
	item = create_menu_item (self, NULL, label,
				 G_CALLBACK (mount_drive), TRUE);
	g_free (label);
	gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
    }

    if (ejectable) {
	label = g_strdup_printf (_("_Eject %s"), display_name);
	item = create_menu_item (self, "media-eject", label,
				 G_CALLBACK (eject_drive), TRUE);
	g_free (label);
	gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
    }
}
