/* Eye Of Mate - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnu.org>
 *	- Jens Finke <jens@mate.org>
 * Based on evince code (shell/ev-window.c) by:
 * 	- Martin Kretzschmar <martink@mate.org>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "eog-window.h"
#include "eog-scroll-view.h"
#include "eog-debug.h"
#include "eog-file-chooser.h"
#include "eog-thumb-view.h"
#include "eog-list-store.h"
#include "eog-sidebar.h"
#include "eog-statusbar.h"
#include "eog-preferences-dialog.h"
#include "eog-properties-dialog.h"
#include "eog-print.h"
#include "eog-error-message-area.h"
#include "eog-application.h"
#include "eog-thumb-nav.h"
#include "eog-config-keys.h"
#include "eog-job-queue.h"
#include "eog-jobs.h"
#include "eog-util.h"
#include "eog-save-as-dialog-helper.h"
#include "eog-plugin-engine.h"
#include "eog-close-confirmation-dialog.h"

#include "eog-enum-types.h"

#include "egg-toolbar-editor.h"
#include "egg-editable-toolbar.h"
#include "egg-toolbars-model.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#if HAVE_LCMS
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <lcms.h>
#endif

#define EOG_WINDOW_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_WINDOW, EogWindowPrivate))

G_DEFINE_TYPE (EogWindow, eog_window, GTK_TYPE_WINDOW);

#define EOG_WINDOW_MIN_WIDTH  440
#define EOG_WINDOW_MIN_HEIGHT 350

#define EOG_WINDOW_DEFAULT_WIDTH  540
#define EOG_WINDOW_DEFAULT_HEIGHT 450

#define EOG_WINDOW_FULLSCREEN_TIMEOUT 5 * 1000
#define EOG_WINDOW_FULLSCREEN_POPUP_THRESHOLD 5

#define EOG_RECENT_FILES_GROUP  "Graphics"
#define EOG_RECENT_FILES_APP_NAME "Eye of MATE Image Viewer"
#define EOG_RECENT_FILES_LIMIT  5

#define EOG_WALLPAPER_FILENAME "eog-wallpaper"

#define is_rtl (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)

typedef enum {
	EOG_WINDOW_STATUS_UNKNOWN,
	EOG_WINDOW_STATUS_INIT,
	EOG_WINDOW_STATUS_NORMAL
} EogWindowStatus;

enum {
	PROP_0,
	PROP_STARTUP_FLAGS
};

enum {
	SIGNAL_PREPARED,
	SIGNAL_LAST
};

static gint signals[SIGNAL_LAST];

/* MateConfNotifications */
enum {
	EOG_WINDOW_NOTIFY_INTERPOLATE,
	EOG_WINDOW_NOTIFY_EXTRAPOLATE,
	EOG_WINDOW_NOTIFY_SCROLLWHEEL_ZOOM,
	EOG_WINDOW_NOTIFY_ZOOM_MULTIPLIER,
	EOG_WINDOW_NOTIFY_BACKGROUND_COLOR,
	EOG_WINDOW_NOTIFY_USE_BG_COLOR,
	EOG_WINDOW_NOTIFY_TRANSPARENCY,
	EOG_WINDOW_NOTIFY_TRANS_COLOR,
	EOG_WINDOW_NOTIFY_SCROLL_BUTTONS,
	EOG_WINDOW_NOTIFY_COLLECTION_POS,
	EOG_WINDOW_NOTIFY_COLLECTION_RESIZABLE,
	EOG_WINDOW_NOTIFY_CAN_SAVE,
	EOG_WINDOW_NOTIFY_PROPSDIALOG_NETBOOK_MODE,
	EOG_WINDOW_NOTIFY_LENGTH
};

struct _EogWindowPrivate {
        MateConfClient         *client;
        guint                client_notifications[EOG_WINDOW_NOTIFY_LENGTH];

        EogListStore        *store;
        EogImage            *image;
	EogWindowMode        mode;
	EogWindowStatus      status;

        GtkUIManager        *ui_mgr;
        GtkWidget           *box;
        GtkWidget           *layout;
        GtkWidget           *cbox;
        GtkWidget           *view;
        GtkWidget           *sidebar;
        GtkWidget           *thumbview;
        GtkWidget           *statusbar;
        GtkWidget           *nav;
	GtkWidget           *message_area;
	GtkWidget           *toolbar;
	GObject             *properties_dlg;

        GtkActionGroup      *actions_window;
        GtkActionGroup      *actions_image;
        GtkActionGroup      *actions_collection;
        GtkActionGroup      *actions_recent;

	GtkWidget           *fullscreen_popup;
	GSource             *fullscreen_timeout_source;

	gboolean             slideshow_loop;
	gint                 slideshow_switch_timeout;
	GSource             *slideshow_switch_source;

        guint		     recent_menu_id;

        EogJob              *load_job;
        EogJob              *transform_job;
	EogJob              *save_job;
	GFile               *last_save_as_folder;
	EogJob              *copy_job;

        guint                image_info_message_cid;
        guint                tip_message_cid;
	guint                copy_file_cid;

        EogStartupFlags      flags;
	GSList              *file_list;

	gint                 collection_position;
	gboolean             collection_resizable;

        GtkActionGroup      *actions_open_with;
	guint                open_with_menu_id;

	gboolean	     save_disabled;
	gboolean             needs_reload_confirmation;

#ifdef HAVE_LCMS
        cmsHPROFILE         *display_profile;
#endif
};

static void eog_window_cmd_fullscreen (GtkAction *action, gpointer user_data);
static void eog_window_run_fullscreen (EogWindow *window, gboolean slideshow);
static void eog_window_cmd_slideshow (GtkAction *action, gpointer user_data);
static void eog_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data);
static void eog_window_stop_fullscreen (EogWindow *window, gboolean slideshow);
static void eog_job_load_cb (EogJobLoad *job, gpointer data);
static void eog_job_save_progress_cb (EogJobSave *job, float progress, gpointer data);
static void eog_job_progress_cb (EogJobLoad *job, float progress, gpointer data);
static void eog_job_transform_cb (EogJobTransform *job, gpointer data);
static void fullscreen_set_timeout (EogWindow *window);
static void fullscreen_clear_timeout (EogWindow *window);
static void update_action_groups_state (EogWindow *window);
static void open_with_launch_application_cb (GtkAction *action, gpointer callback_data);
static void eog_window_update_openwith_menu (EogWindow *window, EogImage *image);
static void eog_window_list_store_image_added (GtkTreeModel *tree_model,
					       GtkTreePath  *path,
					       GtkTreeIter  *iter,
					       gpointer      user_data);
static void eog_window_list_store_image_removed (GtkTreeModel *tree_model,
                 				 GtkTreePath  *path,
						 gpointer      user_data);
static void eog_window_set_wallpaper (EogWindow *window, const gchar *filename, const gchar *visible_filename);
static gboolean eog_window_save_images (EogWindow *window, GList *images);
static void eog_window_finish_saving (EogWindow *window);

static GQuark
eog_window_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("eog-window-error-quark");

	return q;
}

static void
eog_window_interp_in_type_changed_cb (MateConfClient *client,
				      guint       cnxn_id,
				      MateConfEntry  *entry,
				      gpointer    user_data)
{
	EogWindowPrivate *priv;
	gboolean interpolate_in = TRUE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		interpolate_in = mateconf_value_get_bool (entry->value);
	}

	eog_scroll_view_set_antialiasing_in (EOG_SCROLL_VIEW (priv->view),
					  interpolate_in);
}

static void
eog_window_interp_out_type_changed_cb (MateConfClient *client,
				       guint       cnxn_id,
				       MateConfEntry  *entry,
				       gpointer    user_data)
{
	EogWindowPrivate *priv;
	gboolean interpolate_out = TRUE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		interpolate_out = mateconf_value_get_bool (entry->value);
	}

	eog_scroll_view_set_antialiasing_out (EOG_SCROLL_VIEW (priv->view),
					  interpolate_out);
}

static void
eog_window_scroll_wheel_zoom_changed_cb (MateConfClient *client,
				         guint       cnxn_id,
				         MateConfEntry  *entry,
				         gpointer    user_data)
{
	EogWindowPrivate *priv;
	gboolean scroll_wheel_zoom = FALSE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		scroll_wheel_zoom = mateconf_value_get_bool (entry->value);
	}

	eog_scroll_view_set_scroll_wheel_zoom (EOG_SCROLL_VIEW (priv->view),
					       scroll_wheel_zoom);
}

static void
eog_window_zoom_multiplier_changed_cb (MateConfClient *client,
				       guint       cnxn_id,
				       MateConfEntry  *entry,
				       gpointer    user_data)
{
	EogWindowPrivate *priv;
	gdouble multiplier = 0.05;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_FLOAT) {
		multiplier = mateconf_value_get_float (entry->value);
	}

	eog_scroll_view_set_zoom_multiplier (EOG_SCROLL_VIEW (priv->view),
					     multiplier);
}

static void
eog_window_transparency_changed_cb (MateConfClient *client,
				    guint       cnxn_id,
				    MateConfEntry  *entry,
				    gpointer    user_data)
{
	EogWindowPrivate *priv;
	const char *value = NULL;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_PREFERENCES);

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_STRING) {
		value = mateconf_value_get_string (entry->value);
	}

	if (value == NULL) {
		return;
	} else if (g_ascii_strcasecmp (value, "COLOR") == 0) {
		GdkColor color;
		char *color_str;

		color_str = mateconf_client_get_string (priv->client,
						     EOG_CONF_VIEW_TRANS_COLOR, NULL);
		if (gdk_color_parse (color_str, &color)) {
			eog_scroll_view_set_transparency (EOG_SCROLL_VIEW (priv->view),
							  EOG_TRANSP_COLOR, &color);
		}
		g_free (color_str);
	} else if (g_ascii_strcasecmp (value, "CHECK_PATTERN") == 0) {
		eog_scroll_view_set_transparency (EOG_SCROLL_VIEW (priv->view),
						  EOG_TRANSP_CHECKED, NULL);
	} else {
		eog_scroll_view_set_transparency (EOG_SCROLL_VIEW (priv->view),
						  EOG_TRANSP_BACKGROUND, NULL);
	}
}

static void
eog_window_bg_color_changed_cb (MateConfClient *client,
				guint       cnxn_id,
				MateConfEntry  *entry,
				gpointer    user_data)
{
	EogWindowPrivate *priv;
	GdkColor color;
	const char *color_str;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_PREFERENCES);

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_STRING) {
		color_str = mateconf_value_get_string (entry->value);

		if (gdk_color_parse (color_str, &color)) {
			eog_scroll_view_set_background_color (EOG_SCROLL_VIEW (priv->view), &color);
		}
	}
}

static void
eog_window_use_bg_color_changed_cb (MateConfClient *client,
				      guint       cnxn_id,
				      MateConfEntry  *entry,
				      gpointer    user_data)
{
	EogWindowPrivate *priv;
	gboolean use_bg_color = TRUE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		use_bg_color = mateconf_value_get_bool (entry->value);
	}

	eog_scroll_view_set_use_bg_color (EOG_SCROLL_VIEW (priv->view),
					  use_bg_color);
}


static void
eog_window_trans_color_changed_cb (MateConfClient *client,
				   guint       cnxn_id,
				   MateConfEntry  *entry,
				   gpointer    user_data)
{
	EogWindowPrivate *priv;
	GdkColor color;
	const char *color_str;
	char *value;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_PREFERENCES);

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	value = mateconf_client_get_string (priv->client,
					 EOG_CONF_VIEW_TRANSPARENCY,
					 NULL);

	if (!value || g_ascii_strcasecmp (value, "COLOR") != 0) {
		g_free (value);
		return;
	}

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_STRING) {
		color_str = mateconf_value_get_string (entry->value);

		if (gdk_color_parse (color_str, &color)) {
			eog_scroll_view_set_transparency (EOG_SCROLL_VIEW (priv->view),
							  EOG_TRANSP_COLOR, &color);
		}
	}
	g_free (value);
}

static void
eog_window_scroll_buttons_changed_cb (MateConfClient *client,
				      guint       cnxn_id,
				      MateConfEntry  *entry,
				      gpointer    user_data)
{
	EogWindowPrivate *priv;
	gboolean show_buttons = TRUE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (priv->view));

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		show_buttons = mateconf_value_get_bool (entry->value);
	}

	eog_thumb_nav_set_show_buttons (EOG_THUMB_NAV (priv->nav),
					show_buttons);
}

static void
eog_window_collection_mode_changed_cb (MateConfClient *client,
				       guint       cnxn_id,
				       MateConfEntry  *entry,
				       gpointer    user_data)
{
	EogWindowPrivate *priv;
	MateConfEntry *mode_entry;
	GtkWidget *hpaned;
	EogThumbNavMode mode = EOG_THUMB_NAV_MODE_ONE_ROW;
	gint position = 0;
	gboolean resizable = FALSE;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	priv = EOG_WINDOW (user_data)->priv;

	mode_entry = mateconf_client_get_entry (priv->client,
		 			     EOG_CONF_UI_IMAGE_COLLECTION_POSITION,
					     NULL, TRUE, NULL);

	if (G_LIKELY (mode_entry != NULL)) {
		if (mode_entry->value != NULL &&
		    mode_entry->value->type == MATECONF_VALUE_INT) {
			position = mateconf_value_get_int (mode_entry->value);
		}
		mateconf_entry_unref (mode_entry);
	}

	mode_entry = mateconf_client_get_entry (priv->client,
					     EOG_CONF_UI_IMAGE_COLLECTION_RESIZABLE,
					     NULL, TRUE, NULL);

	if (G_LIKELY (mode_entry != NULL)) {
		if (mode_entry->value != NULL &&
		    mode_entry->value->type == MATECONF_VALUE_BOOL) {
			resizable = mateconf_value_get_bool (mode_entry->value);
		}
		mateconf_entry_unref (mode_entry);
	}

	if (priv->collection_position == position &&
	    priv->collection_resizable == resizable)
		return;

	priv->collection_position = position;
	priv->collection_resizable = resizable;

	hpaned = gtk_widget_get_parent (priv->sidebar);

	g_object_ref (hpaned);
	g_object_ref (priv->nav);

	gtk_container_remove (GTK_CONTAINER (priv->layout), hpaned);
	gtk_container_remove (GTK_CONTAINER (priv->layout), priv->nav);

	gtk_widget_destroy (priv->layout);

	switch (position) {
	case 0:
	case 2:
		if (resizable) {
			mode = EOG_THUMB_NAV_MODE_MULTIPLE_ROWS;

			priv->layout = gtk_vpaned_new ();

			if (position == 0) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			}
		} else {
			mode = EOG_THUMB_NAV_MODE_ONE_ROW;

			priv->layout = gtk_vbox_new (FALSE, 2);

			if (position == 0) {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			}
		}
		break;

	case 1:
	case 3:
		if (resizable) {
			mode = EOG_THUMB_NAV_MODE_MULTIPLE_COLUMNS;

			priv->layout = gtk_hpaned_new ();

			if (position == 1) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			}
		} else {
			mode = EOG_THUMB_NAV_MODE_ONE_COLUMN;

			priv->layout = gtk_hbox_new (FALSE, 2);

			if (position == 1) {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			}
		}

		break;
	}

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	eog_thumb_nav_set_mode (EOG_THUMB_NAV (priv->nav), mode);

	if (priv->mode != EOG_WINDOW_STATUS_UNKNOWN) {
		update_action_groups_state (EOG_WINDOW (user_data));
	}
}

static void
eog_window_can_save_changed_cb (MateConfClient *client,
				guint       cnxn_id,
				MateConfEntry  *entry,
				gpointer    user_data)
{
	EogWindowPrivate *priv;
	EogWindow *window;
	gboolean save_disabled = FALSE;
	GtkAction *action_save, *action_save_as;

	eog_debug (DEBUG_PREFERENCES);

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = EOG_WINDOW (user_data)->priv;

	if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
		save_disabled = mateconf_value_get_bool (entry->value);
	}

	priv->save_disabled = save_disabled;

	action_save =
		gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_save_as =
		gtk_action_group_get_action (priv->actions_image, "ImageSaveAs");

	if (priv->save_disabled) {
		gtk_action_set_sensitive (action_save, FALSE);
		gtk_action_set_sensitive (action_save_as, FALSE);
	} else {
		EogImage *image = eog_window_get_image (window);

		if (EOG_IS_IMAGE (image)) {
			gtk_action_set_sensitive (action_save,
						  eog_image_is_modified (image));

			gtk_action_set_sensitive (action_save_as, TRUE);
		}
	}
}

static void
eog_window_pd_nbmode_changed_cb (MateConfClient *client,
				 guint       cnxn_id,
				 MateConfEntry  *entry,
				 gpointer    user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	if (window->priv->properties_dlg != NULL) {
		gboolean netbook_mode;
		EogPropertiesDialog *dlg;

		netbook_mode = mateconf_value_get_bool (entry->value);
		dlg = EOG_PROPERTIES_DIALOG (window->priv->properties_dlg);

		eog_properties_dialog_set_netbook_mode (dlg, netbook_mode);
	}
}

#ifdef HAVE_LCMS
static cmsHPROFILE *
eog_window_get_display_profile (GdkScreen *screen)
{
	Display *dpy;
	Atom icc_atom, type;
	int format;
	gulong nitems;
	gulong bytes_after;
	gulong length;
	guchar *str;
	int result;
	cmsHPROFILE *profile;
	char *atom_name;
	int lcms_error_action;

	dpy = GDK_DISPLAY_XDISPLAY (gdk_screen_get_display (screen));

	if (gdk_screen_get_number (screen) > 0)
		atom_name = g_strdup_printf ("_ICC_PROFILE_%d", gdk_screen_get_number (screen));
	else
		atom_name = g_strdup ("_ICC_PROFILE");

	icc_atom = gdk_x11_get_xatom_by_name_for_display (gdk_screen_get_display (screen), atom_name);

	g_free (atom_name);

	result = XGetWindowProperty (dpy,
				     GDK_WINDOW_XID (gdk_screen_get_root_window (screen)),
				     icc_atom,
				     0,
				     G_MAXLONG,
				     False,
				     XA_CARDINAL,
				     &type,
				     &format,
				     &nitems,
				     &bytes_after,
                                     (guchar **)&str);

	/* TODO: handle bytes_after != 0 */

	if ((result == Success) && (type == XA_CARDINAL) && (nitems > 0)) {
		switch (format)
		{
			case 8:
				length = nitems;
				break;
			case 16:
				length = sizeof(short) * nitems;
				break;
			case 32:
				length = sizeof(long) * nitems;
				break;
			default:
				eog_debug_message (DEBUG_LCMS, "Unable to read profile, not correcting");

				XFree (str);
				return NULL;
		}

		/* Make lcms errors non-fatal here, as it is possible
		 * to load invalid profiles with XICC.
		 * We don't want lcms to abort EOG in that case.
		 */
		lcms_error_action = cmsErrorAction (LCMS_ERROR_IGNORE);

		profile = cmsOpenProfileFromMem (str, length);

		// Restore the previous error setting
		cmsErrorAction (lcms_error_action);

		if (G_UNLIKELY (profile == NULL)) {
			eog_debug_message (DEBUG_LCMS,
					   "Invalid display profile, "
					   "not correcting");
		}

		XFree (str);
	} else {
		profile = NULL;
		eog_debug_message (DEBUG_LCMS, "No profile, not correcting");
	}

	return profile;
}
#endif

static void
update_image_pos (EogWindow *window)
{
	EogWindowPrivate *priv;
	gint pos, n_images;

	priv = window->priv;

	n_images = eog_list_store_length (EOG_LIST_STORE (priv->store));

	if (n_images > 0) {
		pos = eog_list_store_get_pos_by_image (EOG_LIST_STORE (priv->store),
						       priv->image);

		/* Images: (image pos) / (n_total_images) */
		eog_statusbar_set_image_number (EOG_STATUSBAR (priv->statusbar),
						pos + 1,
						n_images);
	}
}

static void
update_status_bar (EogWindow *window)
{
	EogWindowPrivate *priv;
	char *str = NULL;

	g_return_if_fail (EOG_IS_WINDOW (window));

	eog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->image != NULL &&
	    eog_image_has_data (priv->image, EOG_IMAGE_DATA_ALL)) {
		int zoom, width, height;
		goffset bytes = 0;

		zoom = floor (100 * eog_scroll_view_get_zoom (EOG_SCROLL_VIEW (priv->view)) + 0.5);

		eog_image_get_size (priv->image, &width, &height);

		bytes = eog_image_get_bytes (priv->image);

		if ((width > 0) && (height > 0)) {
			char *size_string;

			size_string = g_format_size_for_display (bytes);

			/* Translators: This is the string displayed in the statusbar
			 * The tokens are from left to right:
			 * - image width
			 * - image height
			 * - image size in bytes
			 * - zoom in percent */
			str = g_strdup_printf (ngettext("%i × %i pixel  %s    %i%%",
							"%i × %i pixels  %s    %i%%", height),
						width,
						height,
						size_string,
						zoom);

			g_free (size_string);
		}

		update_image_pos (window);
	}

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, str ? str : "");

	g_free (str);
}

static void
eog_window_set_message_area (EogWindow *window,
		             GtkWidget *message_area)
{
	if (window->priv->message_area == message_area)
		return;

	if (window->priv->message_area != NULL)
		gtk_widget_destroy (window->priv->message_area);

	window->priv->message_area = message_area;

	if (message_area == NULL) return;

	gtk_box_pack_start (GTK_BOX (window->priv->cbox),
			    window->priv->message_area,
			    FALSE,
			    FALSE,
			    0);

	g_object_add_weak_pointer (G_OBJECT (window->priv->message_area),
				   (void *) &window->priv->message_area);
}

static void
update_action_groups_state (EogWindow *window)
{
	EogWindowPrivate *priv;
	GtkAction *action_collection;
	GtkAction *action_sidebar;
	GtkAction *action_fscreen;
	GtkAction *action_sshow;
	GtkAction *action_print;
	gboolean print_disabled = FALSE;
	gboolean page_setup_disabled = FALSE;
	gboolean show_image_collection = FALSE;
	gint n_images = 0;

	g_return_if_fail (EOG_IS_WINDOW (window));

	eog_debug (DEBUG_WINDOW);

	priv = window->priv;

	action_collection =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewImageCollection");

	action_sidebar =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewSidebar");

	action_fscreen =
		gtk_action_group_get_action (priv->actions_image,
					     "ViewFullscreen");

	action_sshow =
		gtk_action_group_get_action (priv->actions_collection,
					     "ViewSlideshow");

	action_print =
		gtk_action_group_get_action (priv->actions_image,
					     "ImagePrint");

	g_assert (action_collection != NULL);
	g_assert (action_sidebar != NULL);
	g_assert (action_fscreen != NULL);
	g_assert (action_sshow != NULL);
	g_assert (action_print != NULL);

	if (priv->store != NULL) {
		n_images = eog_list_store_length (EOG_LIST_STORE (priv->store));
	}

	if (n_images == 0) {
		gtk_widget_hide (priv->layout);

		gtk_action_group_set_sensitive (priv->actions_window,      TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,       FALSE);
		gtk_action_group_set_sensitive (priv->actions_collection,  FALSE);

		gtk_action_set_sensitive (action_fscreen, FALSE);
		gtk_action_set_sensitive (action_sshow,   FALSE);

		/* If there are no images on model, initialization
 		   stops here. */
		if (priv->status == EOG_WINDOW_STATUS_INIT) {
			priv->status = EOG_WINDOW_STATUS_NORMAL;
		}
	} else {
		if (priv->flags & EOG_STARTUP_DISABLE_COLLECTION) {
			mateconf_client_set_bool (priv->client,
					       EOG_CONF_UI_IMAGE_COLLECTION,
					       FALSE,
					       NULL);

			show_image_collection = FALSE;
		} else {
			show_image_collection =
				mateconf_client_get_bool (priv->client,
						       EOG_CONF_UI_IMAGE_COLLECTION,
						       NULL);
		}

		show_image_collection = show_image_collection &&
					n_images > 1 &&
					priv->mode != EOG_WINDOW_MODE_SLIDESHOW;

		gtk_widget_show (priv->layout);

		if (show_image_collection)
			gtk_widget_show (priv->nav);

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_collection),
					      show_image_collection);

		gtk_action_group_set_sensitive (priv->actions_window, TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,  TRUE);

		gtk_action_set_sensitive (action_fscreen, TRUE);

		if (n_images == 1) {
			gtk_action_group_set_sensitive (priv->actions_collection,  FALSE);
			gtk_action_set_sensitive (action_collection, FALSE);
			gtk_action_set_sensitive (action_sshow, FALSE);
		} else {
			gtk_action_group_set_sensitive (priv->actions_collection,  TRUE);
			gtk_action_set_sensitive (action_sshow, TRUE);
		}

		if (show_image_collection)
			gtk_widget_grab_focus (priv->thumbview);
		else
			gtk_widget_grab_focus (priv->view);
	}

	print_disabled = mateconf_client_get_bool (priv->client,
						EOG_CONF_DESKTOP_CAN_PRINT,
						NULL);

	if (print_disabled) {
		gtk_action_set_sensitive (action_print, FALSE);
	}

	page_setup_disabled = mateconf_client_get_bool (priv->client,
						     EOG_CONF_DESKTOP_CAN_SETUP_PAGE,
						     NULL);

	if (eog_sidebar_is_empty (EOG_SIDEBAR (priv->sidebar))) {
		gtk_action_set_sensitive (action_sidebar, FALSE);
		gtk_widget_hide (priv->sidebar);
	}
}

static void
update_selection_ui_visibility (EogWindow *window)
{
	EogWindowPrivate *priv;
	GtkAction *wallpaper_action;
	gint n_selected;

	priv = window->priv;

	n_selected = eog_thumb_view_get_n_selected (EOG_THUMB_VIEW (priv->thumbview));

	wallpaper_action =
		gtk_action_group_get_action (priv->actions_image,
					     "ImageSetAsWallpaper");

	if (n_selected == 1) {
		gtk_action_set_sensitive (wallpaper_action, TRUE);
	} else {
		gtk_action_set_sensitive (wallpaper_action, FALSE);
	}
}

static gboolean
add_file_to_recent_files (GFile *file)
{
	gchar *text_uri;
	GFileInfo *file_info;
	GtkRecentData *recent_data;
	static gchar *groups[2] = { EOG_RECENT_FILES_GROUP , NULL };

	if (file == NULL) return FALSE;

	/* The password gets stripped here because ~/.recently-used.xbel is
	 * readable by everyone (chmod 644). It also makes the workaround
	 * for the bug with gtk_recent_info_get_uri_display() easier
	 * (see the comment in eog_window_update_recent_files_menu()). */
	text_uri = g_file_get_uri (file);

	if (text_uri == NULL)
		return FALSE;

	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL)
		return FALSE;

	recent_data = g_slice_new (GtkRecentData);
	recent_data->display_name = NULL;
	recent_data->description = NULL;
	recent_data->mime_type = (gchar *) g_file_info_get_content_type (file_info);
	recent_data->app_name = EOG_RECENT_FILES_APP_NAME;
	recent_data->app_exec = g_strjoin(" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;
	recent_data->is_private = FALSE;

	gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
				     text_uri,
				     recent_data);

	g_free (recent_data->app_exec);
	g_free (text_uri);
	g_object_unref (file_info);

	g_slice_free (GtkRecentData, recent_data);

	return FALSE;
}

static void
image_thumb_changed_cb (EogImage *image, gpointer data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
	GdkPixbuf *thumb;

	g_return_if_fail (EOG_IS_WINDOW (data));

	window = EOG_WINDOW (data);
	priv = window->priv;

	thumb = eog_image_get_thumbnail (image);

	if (thumb != NULL) {
		gtk_window_set_icon (GTK_WINDOW (window), thumb);

		if (window->priv->properties_dlg != NULL) {
			eog_properties_dialog_update (EOG_PROPERTIES_DIALOG (priv->properties_dlg),
						      image);
		}

		g_object_unref (thumb);
	} else if (!gtk_widget_get_visible (window->priv->nav)) {
		gint img_pos = eog_list_store_get_pos_by_image (window->priv->store, image);
		GtkTreePath *path = gtk_tree_path_new_from_indices (img_pos,-1);
		GtkTreeIter iter;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->store), &iter, path);
		eog_list_store_thumbnail_set (window->priv->store, &iter);
		gtk_tree_path_free (path);
	}
}

static void
file_changed_info_bar_response (GtkInfoBar *info_bar,
				gint response,
				EogWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		eog_window_reload_image (window);
	}

	window->priv->needs_reload_confirmation = TRUE;

	eog_window_set_message_area (window, NULL);
}
static void
image_file_changed_cb (EogImage *img, EogWindow *window)
{
	GtkWidget *info_bar;
	gchar *text, *markup;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;

	if (window->priv->needs_reload_confirmation == FALSE)
		return;

	window->priv->needs_reload_confirmation = FALSE;

	info_bar = gtk_info_bar_new_with_buttons (_("_Reload"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea", "Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);
	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
					  GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been modified by an external application."
				  "\nWould you like to reload it?"), eog_image_get_caption (img));
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (text);
	g_free (markup);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);

	eog_window_set_message_area (window, info_bar);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (file_changed_info_bar_response), window);
}

static void
eog_window_display_image (EogWindow *window, EogImage *image)
{
	EogWindowPrivate *priv;
	GFile *file;

	g_return_if_fail (EOG_IS_WINDOW (window));
	g_return_if_fail (EOG_IS_IMAGE (image));

	eog_debug (DEBUG_WINDOW);

	g_assert (eog_image_has_data (image, EOG_IMAGE_DATA_ALL));

	priv = window->priv;

	if (image != NULL) {
		g_signal_connect (image,
				  "thumbnail_changed",
				  G_CALLBACK (image_thumb_changed_cb),
				  window);
		g_signal_connect (image, "file-changed",
				  G_CALLBACK (image_file_changed_cb),
				  window);

		image_thumb_changed_cb (image, window);
	}

	priv->needs_reload_confirmation = TRUE;

	eog_scroll_view_set_image (EOG_SCROLL_VIEW (priv->view), image);

	gtk_window_set_title (GTK_WINDOW (window), eog_image_get_caption (image));

	update_status_bar (window);

	file = eog_image_get_file (image);
	g_idle_add_full (G_PRIORITY_LOW,
			 (GSourceFunc) add_file_to_recent_files,
			 file,
			 (GDestroyNotify) g_object_unref);

	eog_window_update_openwith_menu (window, image);
}

static void
open_with_launch_application_cb (GtkAction *action, gpointer data) {
	EogImage *image;
	GAppInfo *app;
	GFile *file;
	GList *files = NULL;

	image = EOG_IMAGE (data);
	file = eog_image_get_file (image);

	app = g_object_get_data (G_OBJECT (action), "app");
	files = g_list_append (files, file);
	g_app_info_launch (app,
			   files,
			   NULL, NULL);

	g_object_unref (file);
	g_list_free (files);
}

static void
eog_window_update_openwith_menu (EogWindow *window, EogImage *image)
{
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	gchar *label, *tip;
	const gchar *mime_type;
	GtkAction *action;
	EogWindowPrivate *priv;
        GList *apps;
        guint action_id = 0;
        GIcon *app_icon;
        char *path;
        GtkWidget *menuitem;

	priv = window->priv;

	file = eog_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);

	if (file_info == NULL)
		return;
	else {
		mime_type = g_file_info_get_content_type (file_info);
	}

        if (priv->open_with_menu_id != 0) {
               gtk_ui_manager_remove_ui (priv->ui_mgr, priv->open_with_menu_id);
               priv->open_with_menu_id = 0;
        }

        if (priv->actions_open_with != NULL) {
              gtk_ui_manager_remove_action_group (priv->ui_mgr, priv->actions_open_with);
              priv->actions_open_with = NULL;
        }

        if (mime_type == NULL) {
                g_object_unref (file_info);
                return;
	}

        apps = g_app_info_get_all_for_type (mime_type);

	g_object_unref (file_info);

        if (!apps)
                return;

        priv->actions_open_with = gtk_action_group_new ("OpenWithActions");
        gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_open_with, -1);

        priv->open_with_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);

        for (iter = apps; iter; iter = iter->next) {
                GAppInfo *app = iter->data;
                gchar name[64];

                /* Do not include eog itself */
                if (g_ascii_strcasecmp (g_app_info_get_executable (app),
                                        g_get_prgname ()) == 0) {
                        g_object_unref (app);
                        continue;
                }

                g_snprintf (name, sizeof (name), "OpenWith%u", action_id++);

                label = g_strdup (g_app_info_get_name (app));
                tip = g_strdup_printf (_("Use \"%s\" to open the selected image"), g_app_info_get_name (app));

                action = gtk_action_new (name, label, tip, NULL);

		app_icon = g_app_info_get_icon (app);
		if (G_LIKELY (app_icon != NULL)) {
			g_object_ref (app_icon);
                	gtk_action_set_gicon (action, app_icon);
                	g_object_unref (app_icon);
		}

                g_free (label);
                g_free (tip);

                g_object_set_data_full (G_OBJECT (action), "app", app,
                                        (GDestroyNotify) g_object_unref);

                g_signal_connect (action,
                                  "activate",
                                  G_CALLBACK (open_with_launch_application_cb),
                                  image);

                gtk_action_group_add_action (priv->actions_open_with, action);
                g_object_unref (action);

                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/MainMenu/Image/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);

                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/ThumbnailPopup/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);
                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/ViewPopup/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);

                path = g_strdup_printf ("/MainMenu/Image/ImageOpenWith/Applications Placeholder/%s", name);	

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);

                path = g_strdup_printf ("/ThumbnailPopup/ImageOpenWith/Applications Placeholder/%s", name);	

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);

                path = g_strdup_printf ("/ViewPopup/ImageOpenWith/Applications Placeholder/%s", name);	

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);
        }

        g_list_free (apps);
}

static void
eog_window_clear_load_job (EogWindow *window)
{
	EogWindowPrivate *priv = window->priv;

	if (priv->load_job != NULL) {
		if (!priv->load_job->finished)
			eog_job_queue_remove_job (priv->load_job);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      eog_job_progress_cb,
						      window);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      eog_job_load_cb,
						      window);

		eog_image_cancel_load (EOG_JOB_LOAD (priv->load_job)->image);

		g_object_unref (priv->load_job);
		priv->load_job = NULL;

		/* Hide statusbar */
		eog_statusbar_set_progress (EOG_STATUSBAR (priv->statusbar), 0);
	}
}

static void
eog_job_progress_cb (EogJobLoad *job, float progress, gpointer user_data)
{
	EogWindow *window;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

	eog_statusbar_set_progress (EOG_STATUSBAR (window->priv->statusbar),
				    progress);
}

static void
eog_job_save_progress_cb (EogJobSave *job, float progress, gpointer user_data)
{
	EogWindowPrivate *priv;
	EogWindow *window;

	static EogImage *image = NULL;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = window->priv;

	eog_statusbar_set_progress (EOG_STATUSBAR (priv->statusbar),
				    progress);

	if (image != job->current_image) {
		gchar *str_image, *status_message;
		guint n_images;

		image = job->current_image;

		n_images = g_list_length (job->images);

		str_image = eog_image_get_uri_for_display (image);

		/* Translators: This string is displayed in the statusbar
		 * while saving images. The tokens are from left to right:
		 * - the original filename
		 * - the current image's position in the queue
		 * - the total number of images queued for saving */
		status_message = g_strdup_printf (_("Saving image \"%s\" (%u/%u)"),
					          str_image,
						  job->current_pos + 1,
						  n_images);
		g_free (str_image);

		gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
				   priv->image_info_message_cid);

		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->image_info_message_cid,
				    status_message);

		g_free (status_message);
	}

	if (progress == 1.0)
		image = NULL;
}

static void
eog_window_obtain_desired_size (EogImage  *image,
				gint       width,
				gint       height,
				EogWindow *window)
{
	GdkScreen *screen;
	GdkRectangle monitor;
	GtkAllocation allocation;
	gint final_width, final_height;
	gint screen_width, screen_height;
	gint window_width, window_height;
	gint img_width, img_height;
	gint view_width, view_height;
	gint deco_width, deco_height;

	update_action_groups_state (window);

	img_width = width;
	img_height = height;

#if GTK_CHECK_VERSION (2, 20, 0)
	if (!gtk_widget_get_realized (window->priv->view)) {
#else
	if (!GTK_WIDGET_REALIZED (window->priv->view)) {
#endif
		gtk_widget_realize (window->priv->view);
	}

	gtk_widget_get_allocation (window->priv->view, &allocation);
	view_width  = allocation.width;
	view_height = allocation.height;

#if GTK_CHECK_VERSION (2, 20, 0)
	if (!gtk_widget_get_realized (GTK_WIDGET (window))) {
#else
	if (!GTK_WIDGET_REALIZED (GTK_WIDGET (window))) {
#endif
		gtk_widget_realize (GTK_WIDGET (window));
	}

	gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);
	window_width  = allocation.width;
	window_height = allocation.height;

	screen = gtk_window_get_screen (GTK_WINDOW (window));

	gdk_screen_get_monitor_geometry (screen,
			gdk_screen_get_monitor_at_window (screen,
				gtk_widget_get_window (GTK_WIDGET (window))),
			&monitor);

	screen_width  = monitor.width;
	screen_height = monitor.height;

	deco_width = window_width - view_width;
	deco_height = window_height - view_height;

	if (img_width > 0 && img_height > 0) {
		if ((img_width + deco_width > screen_width) ||
		    (img_height + deco_height > screen_height))
		{
			double factor;

			if (img_width > img_height) {
				factor = (screen_width * 0.75 - deco_width) / (double) img_width;
			} else {
				factor = (screen_height * 0.75 - deco_height) / (double) img_height;
			}

			img_width = img_width * factor;
			img_height = img_height * factor;
		}
	}

	final_width = MAX (EOG_WINDOW_MIN_WIDTH, img_width + deco_width);
	final_height = MAX (EOG_WINDOW_MIN_HEIGHT, img_height + deco_height);

	eog_debug_message (DEBUG_WINDOW, "Setting window size: %d x %d", final_width, final_height);

	gtk_window_set_default_size (GTK_WINDOW (window), final_width, final_height);

	g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
}

static void
eog_window_error_message_area_response (GtkInfoBar       *message_area,
					gint              response_id,
					EogWindow        *window)
{
	if (response_id != GTK_RESPONSE_OK) {
		eog_window_set_message_area (window, NULL);

		return;
	}

	/* Trigger loading for current image again */
	eog_thumb_view_select_single (EOG_THUMB_VIEW (window->priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_CURRENT);
}

static void
eog_job_load_cb (EogJobLoad *job, gpointer data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
	GtkAction *action_undo, *action_save;

        g_return_if_fail (EOG_IS_WINDOW (data));

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (data);
	priv = window->priv;

	eog_statusbar_set_progress (EOG_STATUSBAR (priv->statusbar), 0.0);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   priv->image_info_message_cid);

	if (priv->image != NULL) {
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);

		g_object_unref (priv->image);
	}

	priv->image = g_object_ref (job->image);

	if (EOG_JOB (job)->error == NULL) {
#ifdef HAVE_LCMS
		eog_image_apply_display_profile (job->image,
						 priv->display_profile);
#endif

		gtk_action_group_set_sensitive (priv->actions_image, TRUE);

		eog_window_display_image (window, job->image);
	} else {
		GtkWidget *message_area;

		message_area = eog_image_load_error_message_area_new (
					eog_image_get_caption (job->image),
					EOG_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (eog_window_error_message_area_response),
				  window);

		gtk_window_set_icon (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW (window),
				      eog_image_get_caption (job->image));

		eog_window_set_message_area (window, message_area);

		gtk_info_bar_set_default_response (GTK_INFO_BAR (message_area),
						   GTK_RESPONSE_CANCEL);

		gtk_widget_show (message_area);

		update_status_bar (window);

		eog_scroll_view_set_image (EOG_SCROLL_VIEW (priv->view), NULL);

        	if (window->priv->status == EOG_WINDOW_STATUS_INIT) {
			update_action_groups_state (window);

			g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
		}

		gtk_action_group_set_sensitive (priv->actions_image, FALSE);
	}

	eog_window_clear_load_job (window);

        if (window->priv->status == EOG_WINDOW_STATUS_INIT) {
		window->priv->status = EOG_WINDOW_STATUS_NORMAL;

		g_signal_handlers_disconnect_by_func
			(job->image,
			 G_CALLBACK (eog_window_obtain_desired_size),
			 window);
	}

	action_save = gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_undo = gtk_action_group_get_action (priv->actions_image, "EditUndo");

	/* Set Save and Undo sensitive according to image state.
	 * Respect lockdown in case of Save.*/
	gtk_action_set_sensitive (action_save, (!priv->save_disabled && eog_image_is_modified (job->image)));
	gtk_action_set_sensitive (action_undo, eog_image_is_modified (job->image));

	g_object_unref (job->image);
}

static void
eog_window_clear_transform_job (EogWindow *window)
{
	EogWindowPrivate *priv = window->priv;

	if (priv->transform_job != NULL) {
		if (!priv->transform_job->finished)
			eog_job_queue_remove_job (priv->transform_job);

		g_signal_handlers_disconnect_by_func (priv->transform_job,
						      eog_job_transform_cb,
						      window);
		g_object_unref (priv->transform_job);
		priv->transform_job = NULL;
	}
}

static void
eog_job_transform_cb (EogJobTransform *job, gpointer data)
{
	EogWindow *window;
	GtkAction *action_undo, *action_save;
	EogImage *image;

        g_return_if_fail (EOG_IS_WINDOW (data));

	window = EOG_WINDOW (data);

	eog_window_clear_transform_job (window);

	action_undo =
		gtk_action_group_get_action (window->priv->actions_image, "EditUndo");
	action_save =
		gtk_action_group_get_action (window->priv->actions_image, "ImageSave");

	image = eog_window_get_image (window);

	gtk_action_set_sensitive (action_undo, eog_image_is_modified (image));

	if (!window->priv->save_disabled)
	{
		gtk_action_set_sensitive (action_save, eog_image_is_modified (image));
	}
}

static void
apply_transformation (EogWindow *window, EogTransform *trans)
{
	EogWindowPrivate *priv;
	GList *images;

	g_return_if_fail (EOG_IS_WINDOW (window));

	priv = window->priv;

	images = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (priv->thumbview));

	eog_window_clear_transform_job (window);

	priv->transform_job = eog_job_transform_new (images, trans);

	g_signal_connect (priv->transform_job,
			  "finished",
			  G_CALLBACK (eog_job_transform_cb),
			  window);

	g_signal_connect (priv->transform_job,
			  "progress",
			  G_CALLBACK (eog_job_progress_cb),
			  window);

	eog_job_queue_add_job (priv->transform_job);
}

static void
handle_image_selection_changed_cb (EogThumbView *thumbview, EogWindow *window)
{
	EogWindowPrivate *priv;
	EogImage *image;
	gchar *status_message;
	gchar *str_image;

	priv = window->priv;

	if (eog_thumb_view_get_n_selected (EOG_THUMB_VIEW (priv->thumbview)) == 0)
		return;

	update_selection_ui_visibility (window);

	image = eog_thumb_view_get_first_selected_image (EOG_THUMB_VIEW (priv->thumbview));

	g_assert (EOG_IS_IMAGE (image));

	eog_window_clear_load_job (window);

	eog_window_set_message_area (window, NULL);

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	if (image == priv->image) {
		update_status_bar (window);
		return;
	}

	if (eog_image_has_data (image, EOG_IMAGE_DATA_ALL)) {
		eog_window_display_image (window, image);
		return;
	}

	if (priv->status == EOG_WINDOW_STATUS_INIT) {
		g_signal_connect (image,
				  "size-prepared",
				  G_CALLBACK (eog_window_obtain_desired_size),
				  window);
	}

	priv->load_job = eog_job_load_new (image, EOG_IMAGE_DATA_ALL);

	g_signal_connect (priv->load_job,
			  "finished",
			  G_CALLBACK (eog_job_load_cb),
			  window);

	g_signal_connect (priv->load_job,
			  "progress",
			  G_CALLBACK (eog_job_progress_cb),
			  window);

	eog_job_queue_add_job (priv->load_job);

	str_image = eog_image_get_uri_for_display (image);

	status_message = g_strdup_printf (_("Opening image \"%s\""),
				          str_image);

	g_free (str_image);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, status_message);

	g_free (status_message);
}

static void
view_zoom_changed_cb (GtkWidget *widget, double zoom, gpointer user_data)
{
	EogWindow *window;
	GtkAction *action_zoom_in;
	GtkAction *action_zoom_out;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

	update_status_bar (window);

	action_zoom_in =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomIn");

	action_zoom_out =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomOut");

	gtk_action_set_sensitive (action_zoom_in,
			!eog_scroll_view_get_zoom_is_max (EOG_SCROLL_VIEW (window->priv->view)));
	gtk_action_set_sensitive (action_zoom_out,
			!eog_scroll_view_get_zoom_is_min (EOG_SCROLL_VIEW (window->priv->view)));
}

static void
eog_window_open_recent_cb (GtkAction *action, EogWindow *window)
{
	GtkRecentInfo *info;
	const gchar *uri;
	GSList *list = NULL;

	info = g_object_get_data (G_OBJECT (action), "gtk-recent-info");
	g_return_if_fail (info != NULL);

	uri = gtk_recent_info_get_uri (info);
	list = g_slist_prepend (list, g_strdup (uri));

	eog_application_open_uri_list (EOG_APP,
				       list,
				       GDK_CURRENT_TIME,
				       0,
				       NULL);

	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}

static void
file_open_dialog_response_cb (GtkWidget *chooser,
			      gint       response_id,
			      EogWindow  *ev_window)
{
	if (response_id == GTK_RESPONSE_OK) {
		GSList *uris;

		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (chooser));

		eog_application_open_uri_list (EOG_APP,
					       uris,
					       GDK_CURRENT_TIME,
					       0,
					       NULL);

		g_slist_foreach (uris, (GFunc) g_free, NULL);
		g_slist_free (uris);
	}

	gtk_widget_destroy (chooser);
}

static void
eog_window_update_fullscreen_action (EogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ViewFullscreen");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eog_window_cmd_fullscreen), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == EOG_WINDOW_MODE_FULLSCREEN);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eog_window_cmd_fullscreen), window);
}

static void
eog_window_update_slideshow_action (EogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_collection,
					      "ViewSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eog_window_cmd_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == EOG_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eog_window_cmd_slideshow), window);
}

static void
eog_window_update_pause_slideshow_action (EogWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_image,
					      "PauseSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (eog_window_cmd_pause_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode != EOG_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (eog_window_cmd_pause_slideshow), window);
}

static void
eog_window_update_fullscreen_popup (EogWindow *window)
{
	GtkWidget *popup = window->priv->fullscreen_popup;
	GdkRectangle screen_rect;
	GdkScreen *screen;

	g_return_if_fail (popup != NULL);

	if (gtk_widget_get_window (GTK_WIDGET (window)) == NULL) return;

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	gdk_screen_get_monitor_geometry (screen,
			gdk_screen_get_monitor_at_window
                        (screen,
                         gtk_widget_get_window (GTK_WIDGET (window))),
                         &screen_rect);

	gtk_widget_set_size_request (popup,
				     screen_rect.width,
				     -1);

	gtk_window_move (GTK_WINDOW (popup), screen_rect.x, screen_rect.y);
}

static void
screen_size_changed_cb (GdkScreen *screen, EogWindow *window)
{
	eog_window_update_fullscreen_popup (window);
}

static void
fullscreen_popup_size_request_cb (GtkWidget      *popup,
				  GtkRequisition *req,
				  EogWindow      *window)
{
	eog_window_update_fullscreen_popup (window);
}

static gboolean
fullscreen_timeout_cb (gpointer data)
{
	EogWindow *window = EOG_WINDOW (data);

	gtk_widget_hide_all (window->priv->fullscreen_popup);

	eog_scroll_view_hide_cursor (EOG_SCROLL_VIEW (window->priv->view));

	fullscreen_clear_timeout (window);

	return FALSE;
}

static gboolean
slideshow_is_loop_end (EogWindow *window)
{
	EogWindowPrivate *priv = window->priv;
	EogImage *image = NULL;
	gint pos;

	image = eog_thumb_view_get_first_selected_image (EOG_THUMB_VIEW (priv->thumbview));

	pos = eog_list_store_get_pos_by_image (priv->store, image);

	return (pos == (eog_list_store_length (priv->store) - 1));
}

static gboolean
slideshow_switch_cb (gpointer data)
{
	EogWindow *window = EOG_WINDOW (data);
	EogWindowPrivate *priv = window->priv;

	eog_debug (DEBUG_WINDOW);

	if (!priv->slideshow_loop && slideshow_is_loop_end (window)) {
		eog_window_stop_fullscreen (window, TRUE);
		return FALSE;
	}

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_RIGHT);

	return TRUE;
}

static void
fullscreen_clear_timeout (EogWindow *window)
{
	eog_debug (DEBUG_WINDOW);

	if (window->priv->fullscreen_timeout_source != NULL) {
		g_source_unref (window->priv->fullscreen_timeout_source);
		g_source_destroy (window->priv->fullscreen_timeout_source);
	}

	window->priv->fullscreen_timeout_source = NULL;
}

static void
fullscreen_set_timeout (EogWindow *window)
{
	GSource *source;

	eog_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	source = g_timeout_source_new (EOG_WINDOW_FULLSCREEN_TIMEOUT);
	g_source_set_callback (source, fullscreen_timeout_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->fullscreen_timeout_source = source;

	eog_scroll_view_show_cursor (EOG_SCROLL_VIEW (window->priv->view));
}

static void
slideshow_clear_timeout (EogWindow *window)
{
	eog_debug (DEBUG_WINDOW);

	if (window->priv->slideshow_switch_source != NULL) {
		g_source_unref (window->priv->slideshow_switch_source);
		g_source_destroy (window->priv->slideshow_switch_source);
	}

	window->priv->slideshow_switch_source = NULL;
}

static void
slideshow_set_timeout (EogWindow *window)
{
	GSource *source;

	eog_debug (DEBUG_WINDOW);

	slideshow_clear_timeout (window);

	if (window->priv->slideshow_switch_timeout <= 0)
		return;

	source = g_timeout_source_new (window->priv->slideshow_switch_timeout * 1000);
	g_source_set_callback (source, slideshow_switch_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->slideshow_switch_source = source;
}

static void
show_fullscreen_popup (EogWindow *window)
{
	eog_debug (DEBUG_WINDOW);

	if (!gtk_widget_get_visible (window->priv->fullscreen_popup)) {
		gtk_widget_show_all (GTK_WIDGET (window->priv->fullscreen_popup));
	}

	fullscreen_set_timeout (window);
}

static gboolean
fullscreen_motion_notify_cb (GtkWidget      *widget,
			     GdkEventMotion *event,
			     gpointer       user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	eog_debug (DEBUG_WINDOW);

	if (event->y < EOG_WINDOW_FULLSCREEN_POPUP_THRESHOLD) {
		show_fullscreen_popup (window);
	} else {
		fullscreen_set_timeout (window);
	}

	return FALSE;
}

static gboolean
fullscreen_leave_notify_cb (GtkWidget *widget,
			    GdkEventCrossing *event,
			    gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	eog_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	return FALSE;
}

static void
exit_fullscreen_button_clicked_cb (GtkWidget *button, EogWindow *window)
{
	GtkAction *action;

	eog_debug (DEBUG_WINDOW);

	if (window->priv->mode == EOG_WINDOW_MODE_SLIDESHOW) {
		action = gtk_action_group_get_action (window->priv->actions_collection,
						      "ViewSlideshow");
	} else {
		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ViewFullscreen");
	}
	g_return_if_fail (action != NULL);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
}

static GtkWidget *
eog_window_get_exit_fullscreen_button (EogWindow *window)
{
	GtkWidget *button;

	button = gtk_button_new_from_stock (GTK_STOCK_LEAVE_FULLSCREEN);

	g_signal_connect (button, "clicked",
			  G_CALLBACK (exit_fullscreen_button_clicked_cb),
			  window);

	return button;
}

static GtkWidget *
eog_window_create_fullscreen_popup (EogWindow *window)
{
	GtkWidget *popup;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *toolbar;
	GdkScreen *screen;

	eog_debug (DEBUG_WINDOW);

	popup = gtk_window_new (GTK_WINDOW_POPUP);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (popup), hbox);

	toolbar = gtk_ui_manager_get_widget (window->priv->ui_mgr,
					     "/FullscreenToolbar");
	g_assert (GTK_IS_WIDGET (toolbar));
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 0);

	button = eog_window_get_exit_fullscreen_button (window);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	g_signal_connect_object (screen, "size-changed",
			         G_CALLBACK (screen_size_changed_cb),
				 window, 0);

	g_signal_connect_object (popup, "size_request",
			         G_CALLBACK (fullscreen_popup_size_request_cb),
				 window, 0);

	g_signal_connect (popup,
			  "enter-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	gtk_window_set_screen (GTK_WINDOW (popup), screen);

	return popup;
}

static void
update_ui_visibility (EogWindow *window)
{
	EogWindowPrivate *priv;

	GtkAction *action;
	GtkWidget *menubar;
	GError *error = NULL;

	gboolean fullscreen_mode, visible;

	g_return_if_fail (EOG_IS_WINDOW (window));

	eog_debug (DEBUG_WINDOW);

	priv = window->priv;

	fullscreen_mode = priv->mode == EOG_WINDOW_MODE_FULLSCREEN ||
			  priv->mode == EOG_WINDOW_MODE_SLIDESHOW;

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));

	visible = mateconf_client_get_bool (priv->client, EOG_CONF_UI_TOOLBAR, &error);
	visible = visible && !fullscreen_mode;
	if (error) {
		g_error_free (error);
		error = NULL;
		visible = !fullscreen_mode;
	}
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ToolbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

	visible = mateconf_client_get_bool (priv->client, EOG_CONF_UI_STATUSBAR, &error);
	visible = visible && !fullscreen_mode;
	if (error) {
		g_error_free (error);
		error = NULL;
		visible = !fullscreen_mode;
	}
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/StatusbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

	if (priv->status != EOG_WINDOW_STATUS_INIT) {
		visible = mateconf_client_get_bool (priv->client, EOG_CONF_UI_IMAGE_COLLECTION, NULL);
		visible = visible && priv->mode != EOG_WINDOW_MODE_SLIDESHOW;
		action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ImageCollectionToggle");
		g_assert (action != NULL);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
		if (visible) {
			gtk_widget_show (priv->nav);
		} else {
			gtk_widget_hide (priv->nav);
		}
	}

	visible = mateconf_client_get_bool (priv->client, EOG_CONF_UI_SIDEBAR, NULL);
	visible = visible && !fullscreen_mode;
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/SidebarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	if (visible) {
		gtk_widget_show (priv->sidebar);
	} else {
		gtk_widget_hide (priv->sidebar);
	}

	if (priv->fullscreen_popup != NULL) {
		gtk_widget_hide_all (priv->fullscreen_popup);
	}
}

static void
eog_window_run_fullscreen (EogWindow *window, gboolean slideshow)
{
	EogWindowPrivate *priv;
	GtkWidget *menubar;
	gboolean upscale;

	eog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (slideshow) {
		priv->mode = EOG_WINDOW_MODE_SLIDESHOW;
	} else {
		/* Stop the timer if we come from slideshowing */
		if (priv->mode == EOG_WINDOW_MODE_SLIDESHOW)
			slideshow_clear_timeout (window);

		priv->mode = EOG_WINDOW_MODE_FULLSCREEN;
	}

	if (window->priv->fullscreen_popup == NULL)
		priv->fullscreen_popup
			= eog_window_create_fullscreen_popup (window);

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_hide (menubar);

	g_signal_connect (priv->view,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->view,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	fullscreen_set_timeout (window);

	if (slideshow) {
		priv->slideshow_loop =
				mateconf_client_get_bool (priv->client,
						       EOG_CONF_FULLSCREEN_LOOP,
						       NULL);

		priv->slideshow_switch_timeout =
				mateconf_client_get_int (priv->client,
						      EOG_CONF_FULLSCREEN_SECONDS,
						      NULL);

		slideshow_set_timeout (window);
	}

	upscale = mateconf_client_get_bool (priv->client,
					 EOG_CONF_FULLSCREEN_UPSCALE,
					 NULL);

	eog_scroll_view_set_zoom_upscale (EOG_SCROLL_VIEW (priv->view),
					  upscale);

	gtk_widget_grab_focus (priv->view);

	eog_scroll_view_override_bg_color (EOG_SCROLL_VIEW (window->priv->view),
			  &(gtk_widget_get_style (GTK_WIDGET (window))->black));

	{
		GtkStyle *style;

		style = gtk_style_copy (gtk_widget_get_style (gtk_widget_get_parent (priv->view)));

		style->xthickness = 0;
		style->ythickness = 0;

		gtk_widget_set_style (gtk_widget_get_parent (priv->view),
				      style);

		g_object_unref (style);
	}

	gtk_window_fullscreen (GTK_WINDOW (window));
	eog_window_update_fullscreen_popup (window);

#ifdef HAVE_DBUS
	eog_application_screensaver_disable (EOG_APP);
#endif

	/* Update both actions as we could've already been in one those modes */
	eog_window_update_slideshow_action (window);
	eog_window_update_fullscreen_action (window);
	eog_window_update_pause_slideshow_action (window);
}

static void
eog_window_stop_fullscreen (EogWindow *window, gboolean slideshow)
{
	EogWindowPrivate *priv;
	GtkWidget *menubar;

	eog_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->mode != EOG_WINDOW_MODE_SLIDESHOW &&
	    priv->mode != EOG_WINDOW_MODE_FULLSCREEN) return;

	priv->mode = EOG_WINDOW_MODE_NORMAL;

	fullscreen_clear_timeout (window);

	if (slideshow) {
		slideshow_clear_timeout (window);
	}

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_show (menubar);

	eog_scroll_view_set_zoom_upscale (EOG_SCROLL_VIEW (priv->view), FALSE);

	eog_scroll_view_override_bg_color (EOG_SCROLL_VIEW (window->priv->view),
					   NULL);
	gtk_widget_set_style (gtk_widget_get_parent (window->priv->view), NULL);
	gtk_window_unfullscreen (GTK_WINDOW (window));

	if (slideshow) {
		eog_window_update_slideshow_action (window);
	} else {
		eog_window_update_fullscreen_action (window);
	}

	eog_scroll_view_show_cursor (EOG_SCROLL_VIEW (priv->view));

#ifdef HAVE_DBUS
	eog_application_screensaver_enable (EOG_APP);
#endif
}

static void
eog_window_print (EogWindow *window)
{
	GtkWidget *dialog;
	GError *error = NULL;
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	GtkPageSetup *page_setup;
	GtkPrintSettings *print_settings;

	eog_debug (DEBUG_PRINTING);

	print_settings = eog_print_get_print_settings ();
	page_setup = eog_print_get_page_setup ();

	/* Make sure the window stays valid while printing */
	g_object_ref (window);

	print = eog_print_operation_new (window->priv->image,
					 print_settings,
					 page_setup);

	res = gtk_print_operation_run (print,
				       GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       GTK_WINDOW (window), &error);

	if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Error printing file:\n%s"),
						 error->message);
		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
		g_error_free (error);
	} else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		eog_print_set_print_settings (gtk_print_operation_get_print_settings (print));
		eog_print_set_page_setup (gtk_print_operation_get_default_page_setup (print));
	}

	g_object_unref (page_setup);
	g_object_unref (print_settings);
	g_object_unref (window);
}

static void
eog_window_cmd_file_open (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
        EogImage *current;
	GtkWidget *dlg;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

        priv = window->priv;

	dlg = eog_file_chooser_new (GTK_FILE_CHOOSER_ACTION_OPEN);

	current = eog_thumb_view_get_first_selected_image (EOG_THUMB_VIEW (priv->thumbview));

	if (current != NULL) {
		gchar *dir_uri, *file_uri;

		file_uri = eog_image_get_uri_for_display (current);
		dir_uri = g_path_get_dirname (file_uri);

	        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg),
                                                         dir_uri);
		g_free (file_uri);
		g_free (dir_uri);
		g_object_unref (current);
	} else {
		/* If desired by the user,
		   fallback to the XDG_PICTURES_DIR (if available) */
		const gchar *pics_dir;
		gboolean use_fallback;

		use_fallback = mateconf_client_get_bool (priv->client,
					   EOG_CONF_UI_FILECHOOSER_XDG_FALLBACK,
					   NULL);
		pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
		if (use_fallback && pics_dir) {
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg),
							     pics_dir);
		}
	}

	g_signal_connect (dlg, "response",
			  G_CALLBACK (file_open_dialog_response_cb),
			  window);

	gtk_widget_show_all (dlg);
}

static void
eog_job_close_save_cb (EogJobSave *job, gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	g_signal_handlers_disconnect_by_func (job,
					      eog_job_close_save_cb,
					      window);

	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
close_confirmation_dialog_response_handler (EogCloseConfirmationDialog *dlg,
					    gint                        response_id,
					    EogWindow                  *window)
{
	GList *selected_images;
	EogWindowPrivate *priv;

	priv = window->priv;

	switch (response_id)
	{
		case GTK_RESPONSE_YES:
			/* save selected images */
			selected_images = eog_close_confirmation_dialog_get_selected_images (dlg);
			eog_close_confirmation_dialog_set_sensitive (dlg, FALSE);
			if (eog_window_save_images (window, selected_images)) {
				g_signal_connect (priv->save_job,
							  "finished",
							  G_CALLBACK (eog_job_close_save_cb),
							  window);

				eog_job_queue_add_job (priv->save_job);
			}

			break;

		case GTK_RESPONSE_NO:
			/* dont save */
			gtk_widget_destroy (GTK_WIDGET (window));
			break;

		default:
			/* Cancel */
			gtk_widget_destroy (GTK_WIDGET (dlg));
			break;
	}	
}

static gboolean
eog_window_unsaved_images_confirm (EogWindow *window)
{
	EogWindowPrivate *priv;
	GtkWidget *dialog;
	GList *list;
	EogImage *image;
	GtkTreeIter iter;

	priv = window->priv;

	if (window->priv->save_disabled) {
		return FALSE;
	}

	list = NULL;
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
					    EOG_LIST_STORE_EOG_IMAGE, &image,
					    -1);
			if (!image)
				continue;

			if (eog_image_is_modified (image)) {
				list = g_list_append (list, image);
			}
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));
	}		

	if (list) {	
		dialog = eog_close_confirmation_dialog_new (GTK_WINDOW (window),
							    list);
	
		g_signal_connect (dialog,
				  "response",
				  G_CALLBACK (close_confirmation_dialog_response_handler),
				  window);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

		gtk_widget_show (dialog);
		return TRUE;

	}
	return FALSE;
}

static void
eog_window_cmd_close_window (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = window->priv;

	if (priv->save_job != NULL) {
		eog_window_finish_saving (window);
	}

	if (!eog_window_unsaved_images_confirm (window)) {
		gtk_widget_destroy (GTK_WIDGET (user_data));
	}
}

static void
eog_window_cmd_preferences (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	GObject *pref_dlg;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

	pref_dlg = eog_preferences_dialog_get_instance (GTK_WINDOW (window),
							window->priv->client);

	eog_dialog_show (EOG_DIALOG (pref_dlg));
}

#define EOG_TB_EDITOR_DLG_RESET_RESPONSE 128

static void
eog_window_cmd_edit_toolbar_cb (GtkDialog *dialog, gint response, gpointer data)
{
	EogWindow *window = EOG_WINDOW (data);

	if (response == EOG_TB_EDITOR_DLG_RESET_RESPONSE) {
		EggToolbarsModel *model;
		EggToolbarEditor *editor;

		editor = g_object_get_data (G_OBJECT (dialog),
					    "EggToolbarEditor");

		g_return_if_fail (editor != NULL);

        	egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		eog_application_reset_toolbars_model (EOG_APP);
		model = eog_application_get_toolbars_model (EOG_APP);
		egg_editable_toolbar_set_model
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), model);
		egg_toolbar_editor_set_model (editor, model);

		/* Toolbar would be uneditable now otherwise */
		egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);
	} else if (response == GTK_RESPONSE_HELP) {
		eog_util_show_help ("eog-toolbareditor", NULL);
	} else {
        	egg_editable_toolbar_set_edit_mode
			(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), FALSE);

		eog_application_save_toolbars_model (EOG_APP);

        	gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

static void
eog_window_cmd_edit_toolbar (GtkAction *action, gpointer *user_data)
{
	EogWindow *window;
	GtkWidget *dialog;
	GtkWidget *editor;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

	dialog = gtk_dialog_new_with_buttons (_("Toolbar Editor"),
					      GTK_WINDOW (window),
				              GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("_Reset to Default"),
					      EOG_TB_EDITOR_DLG_RESET_RESPONSE,
 					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE,
					      GTK_STOCK_HELP,
					      GTK_RESPONSE_HELP,
					      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_CLOSE);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 400);

	editor = egg_toolbar_editor_new (window->priv->ui_mgr,
					 eog_application_get_toolbars_model (EOG_APP));

	gtk_container_set_border_width (GTK_CONTAINER (editor), 5);

	gtk_box_set_spacing (GTK_BOX (EGG_TOOLBAR_EDITOR (editor)), 5);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), editor);

	egg_editable_toolbar_set_edit_mode
		(EGG_EDITABLE_TOOLBAR (window->priv->toolbar), TRUE);

	g_object_set_data (G_OBJECT (dialog), "EggToolbarEditor", editor);

	g_signal_connect (dialog,
                          "response",
			  G_CALLBACK (eog_window_cmd_edit_toolbar_cb),
			  window);

	gtk_widget_show_all (dialog);
}

static void
eog_window_cmd_help (GtkAction *action, gpointer user_data)
{
	EogWindow *window;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);

	eog_util_show_help (NULL, GTK_WINDOW (window));
}

static void
eog_window_cmd_about (GtkAction *action, gpointer user_data)
{
	EogWindow *window;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	static const char *authors[] = {
		"Claudio Saavedra <csaavedra@igalia.com> (maintainer)",
		"Felix Riemann <friemann@mate.org> (maintainer)",
		"",
		"Lucas Rocha <lucasr@mate.org>",
		"Tim Gerla <tim+matebugs@gerla.net>",
		"Philip Van Hoof <pvanhoof@mate.org>",
                "Paolo Borelli <pborelli@katamail.com>",
		"Jens Finke <jens@triq.net>",
		"Martin Baulig <martin@home-of-linux.org>",
		"Arik Devens <arik@mate.org>",
		"Michael Meeks <mmeeks@gnu.org>",
		"Federico Mena-Quintero <federico@gnu.org>",
		"Lutz M\xc3\xbcller <urc8@rz.uni-karlsruhe.de>",
		NULL
	};

	static const char *documenters[] = {
		"Eliot Landrum <eliot@landrum.cx>",
		"Federico Mena-Quintero <federico@gnu.org>",
		"Sun MATE Documentation Team <gdocteam@sun.com>",
		NULL
	};

	const char *translators;

	translators = _("translator-credits");

	const char *license[] = {
		N_("This program is free software; you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation; either version 2 of the License, or "
		   "(at your option) any later version.\n"),
		N_("This program is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details.\n"),
		N_("You should have received a copy of the GNU General Public License "
		   "along with this program; if not, write to the Free Software "
		   "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.")
	};

	char *license_trans;

	license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n",
				     _(license[2]), "\n", NULL);

	window = EOG_WINDOW (user_data);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "program-name", _("Eye of MATE"),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2000-2010 Free Software Foundation, Inc.",
			       "comments",_("The MATE image viewer."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", translators,
			       "website", "http://projects.mate.org/eog/",
			       "logo-icon-name", "eog",
			       "wrap-license", TRUE,
			       "license", license_trans,
			       NULL);

	g_free (license_trans);
}

static void
eog_window_cmd_show_hide_bar (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
	gboolean visible;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = window->priv;

	if (priv->mode != EOG_WINDOW_MODE_NORMAL &&
            priv->mode != EOG_WINDOW_MODE_FULLSCREEN) return;

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewToolbar") == 0) {
		g_object_set (G_OBJECT (priv->toolbar), "visible", visible, NULL);

		if (priv->mode == EOG_WINDOW_MODE_NORMAL)
			mateconf_client_set_bool (priv->client, EOG_CONF_UI_TOOLBAR, visible, NULL);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewStatusbar") == 0) {
		g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

		if (priv->mode == EOG_WINDOW_MODE_NORMAL)
			mateconf_client_set_bool (priv->client, EOG_CONF_UI_STATUSBAR, visible, NULL);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewImageCollection") == 0) {
		if (visible) {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events */
#if GTK_CHECK_VERSION (2, 20, 0)
			if (!gtk_widget_get_realized (window->priv->thumbview))
#else
			if (!GTK_WIDGET_REALIZED (window->priv->thumbview))
#endif
				gtk_widget_realize (window->priv->thumbview);

			gtk_widget_show (priv->nav);
			gtk_widget_grab_focus (priv->thumbview);
		} else {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events.
			 * Don't do it during init phase or the view
			 * will get a bogus allocation. */
#if GTK_CHECK_VERSION (2, 20, 0)
			if (!gtk_widget_get_realized (priv->view)
#else
			if (!GTK_WIDGET_REALIZED (priv->view)
#endif
			    && priv->status == EOG_WINDOW_STATUS_NORMAL)
				gtk_widget_realize (priv->view);

			gtk_widget_hide (priv->nav);

#if GTK_CHECK_VERSION (2, 20, 0)
			if (gtk_widget_get_realized (priv->view))
#else
			if (GTK_WIDGET_REALIZED (priv->view))
#endif
				gtk_widget_grab_focus (priv->view);
		}
		mateconf_client_set_bool (priv->client, EOG_CONF_UI_IMAGE_COLLECTION, visible, NULL);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewSidebar") == 0) {
		if (visible) {
			gtk_widget_show (priv->sidebar);
		} else {
			gtk_widget_hide (priv->sidebar);
		}
		mateconf_client_set_bool (priv->client, EOG_CONF_UI_SIDEBAR, visible, NULL);
	}
}

static void
wallpaper_info_bar_response (GtkInfoBar *bar, gint response, EogWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		GdkScreen *screen;

		screen = gtk_widget_get_screen (GTK_WIDGET (window));
		gdk_spawn_command_line_on_screen (screen,
						  "mate-appearance-properties"
						  " --show-page=background",
						  NULL);
	}

	/* Close message area on every response */
	eog_window_set_message_area (window, NULL);
}

static void
eog_window_set_wallpaper (EogWindow *window, const gchar *filename, const gchar *visible_filename)
{
	EogWindowPrivate *priv = EOG_WINDOW_GET_PRIVATE (window);
	GtkWidget *info_bar;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	gchar *markup;
	gchar *text;
	gchar *basename;


	mateconf_client_set_string (priv->client,
				 EOG_CONF_DESKTOP_WALLPAPER,
				 filename,
				 NULL);

	/* I18N: When setting mnemonics for these strings, watch out to not
	   clash with mnemonics from eog's menubar */
	info_bar = gtk_info_bar_new_with_buttons (_("_Open Background Preferences"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea","Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
					  GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	if (!visible_filename)
		basename = g_path_get_basename (filename);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been set as Desktop Background."
				  "\nWould you like to modify its appearance?"),
				visible_filename ? visible_filename : basename);
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	g_free (text);
	if (!visible_filename)
		g_free (basename);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);


	eog_window_set_message_area (window, info_bar);
	gtk_info_bar_set_default_response (GTK_INFO_BAR (info_bar),
					   GTK_RESPONSE_YES);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (wallpaper_info_bar_response), window);
}

static void
eog_job_save_cb (EogJobSave *job, gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);
	GtkAction *action_save;

	g_signal_handlers_disconnect_by_func (job,
					      eog_job_save_cb,
					      window);

	g_signal_handlers_disconnect_by_func (job,
					      eog_job_save_progress_cb,
					      window);

	g_object_unref (window->priv->save_job);
	window->priv->save_job = NULL;

	update_status_bar (window);
	action_save = gtk_action_group_get_action (window->priv->actions_image,
						   "ImageSave");
	gtk_action_set_sensitive (action_save, FALSE);
}

static void
eog_job_copy_cb (EogJobCopy *job, gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);
	gchar *filepath, *basename, *filename, *extension;
	GtkAction *action;
	GFile *source_file, *dest_file;

	/* Create source GFile */
	basename = g_file_get_basename (job->images->data);
	filepath = g_build_filename (job->dest, basename, NULL);
	source_file = g_file_new_for_path (filepath);
	g_free (filepath);

	/* Create destination GFile */
	extension = eog_util_filename_get_extension (basename);
	filename = g_strdup_printf  ("%s.%s", EOG_WALLPAPER_FILENAME, extension);
	filepath = g_build_filename (job->dest, filename, NULL);
	dest_file = g_file_new_for_path (filepath);
	g_free (filename);
	g_free (extension);

	/* Move the file */
	g_file_move (source_file, dest_file, G_FILE_COPY_OVERWRITE,
		     NULL, NULL, NULL, NULL);

	/* Set the wallpaper */
	eog_window_set_wallpaper (window, filepath, basename);
	g_free (basename);
	g_free (filepath);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->copy_file_cid);
	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ImageSetAsWallpaper");
	gtk_action_set_sensitive (action, TRUE);

	window->priv->copy_job = NULL;

	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_object_unref (G_OBJECT (job->images->data));
	g_list_free (job->images);
	g_object_unref (job);
}

static gboolean
eog_window_save_images (EogWindow *window, GList *images)
{
	EogWindowPrivate *priv;

	priv = window->priv;

	if (window->priv->save_job != NULL)
		return FALSE;

	priv->save_job = eog_job_save_new (images);

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (eog_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (eog_job_save_progress_cb),
			  window);

	return TRUE;
}

static void
eog_window_cmd_save (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;
	EogWindow *window;
	GList *images;

	window = EOG_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (priv->thumbview));

	if (eog_window_save_images (window, images)) {
		eog_job_queue_add_job (priv->save_job);
	}
}

static GFile*
eog_window_retrieve_save_as_file (EogWindow *window, EogImage *image)
{
	GtkWidget *dialog;
	GFile *save_file = NULL;
	GFile *last_dest_folder;
	gint response;

	g_assert (image != NULL);

	dialog = eog_file_chooser_new (GTK_FILE_CHOOSER_ACTION_SAVE);

	last_dest_folder = window->priv->last_save_as_folder;

	if (last_dest_folder && g_file_query_exists (last_dest_folder, NULL)) {
		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog), last_dest_folder, NULL);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
						 eog_image_get_caption (image));
	} else {
		GFile *image_file;

		image_file = eog_image_get_file (image);
		/* Setting the file will also navigate to its parent folder */
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog),
					   image_file, NULL);
		g_object_unref (image_file);
	}

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		save_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
		if (window->priv->last_save_as_folder)
			g_object_unref (window->priv->last_save_as_folder);
		window->priv->last_save_as_folder = g_file_get_parent (save_file);
	}
	gtk_widget_destroy (dialog);

	return save_file;
}

static void
eog_window_cmd_save_as (GtkAction *action, gpointer user_data)
{
        EogWindowPrivate *priv;
        EogWindow *window;
	GList *images;
	guint n_images;

        window = EOG_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (priv->thumbview));
	n_images = g_list_length (images);

	if (n_images == 1) {
		GFile *file;

		file = eog_window_retrieve_save_as_file (window, images->data);

		if (!file) {
			g_list_free (images);
			return;
		}

		priv->save_job = eog_job_save_as_new (images, NULL, file);

		g_object_unref (file);
	} else if (n_images > 1) {
		GFile *base_file;
		GtkWidget *dialog;
		gchar *basedir;
		EogURIConverter *converter;

		basedir = g_get_current_dir ();
		base_file = g_file_new_for_path (basedir);
		g_free (basedir);

		dialog = eog_save_as_dialog_new (GTK_WINDOW (window),
						 images,
						 base_file);

		gtk_widget_show_all (dialog);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK) {
			g_object_unref (base_file);
			g_list_free (images);
			gtk_widget_destroy (dialog);

			return;
		}

		converter = eog_save_as_dialog_get_converter (dialog);

		g_assert (converter != NULL);

		priv->save_job = eog_job_save_as_new (images, converter, NULL);

		gtk_widget_destroy (dialog);

		g_object_unref (converter);
		g_object_unref (base_file);
	} else {
		/* n_images = 0 -- No Image selected */
		return;
	}

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (eog_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (eog_job_save_progress_cb),
			  window);

	eog_job_queue_add_job (priv->save_job);
}

static void
eog_window_cmd_print (GtkAction *action, gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	eog_window_print (window);
}

static void
eog_window_cmd_properties (GtkAction *action, gpointer user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);
	EogWindowPrivate *priv;
	GtkAction *next_image_action, *previous_image_action;

	priv = window->priv;

	next_image_action =
		gtk_action_group_get_action (priv->actions_collection,
					     "GoNext");

	previous_image_action =
		gtk_action_group_get_action (priv->actions_collection,
					     "GoPrevious");

	if (window->priv->properties_dlg == NULL) {
		gboolean netbook_mode;

		window->priv->properties_dlg =
			eog_properties_dialog_new (GTK_WINDOW (window),
						   EOG_THUMB_VIEW (priv->thumbview),
						   next_image_action,
						   previous_image_action);

		eog_properties_dialog_update (EOG_PROPERTIES_DIALOG (priv->properties_dlg),
					      priv->image);
		netbook_mode =
			mateconf_client_get_bool (priv->client,
					       EOG_CONF_UI_PROPSDIALOG_NETBOOK_MODE,
					       NULL);
		eog_properties_dialog_set_netbook_mode (EOG_PROPERTIES_DIALOG (priv->properties_dlg),
							netbook_mode);
	}

	eog_dialog_show (EOG_DIALOG (window->priv->properties_dlg));
}

static void
eog_window_cmd_undo (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (EOG_IS_WINDOW (user_data));

	apply_transformation (EOG_WINDOW (user_data), NULL);
}

static void
eog_window_cmd_flip_horizontal (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (EOG_IS_WINDOW (user_data));

	apply_transformation (EOG_WINDOW (user_data),
			      eog_transform_flip_new (EOG_TRANSFORM_FLIP_HORIZONTAL));
}

static void
eog_window_cmd_flip_vertical (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (EOG_IS_WINDOW (user_data));

	apply_transformation (EOG_WINDOW (user_data),
			      eog_transform_flip_new (EOG_TRANSFORM_FLIP_VERTICAL));
}

static void
eog_window_cmd_rotate_90 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (EOG_IS_WINDOW (user_data));

	apply_transformation (EOG_WINDOW (user_data),
			      eog_transform_rotate_new (90));
}

static void
eog_window_cmd_rotate_270 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (EOG_IS_WINDOW (user_data));

	apply_transformation (EOG_WINDOW (user_data),
			      eog_transform_rotate_new (270));
}

static void
eog_window_cmd_wallpaper (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
	EogImage *image;
	GFile *file;
	char *filename = NULL;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = window->priv;

	/* If currently copying an image to set it as wallpaper, return. */
	if (priv->copy_job != NULL)
		return;

	image = eog_thumb_view_get_first_selected_image (EOG_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (EOG_IS_IMAGE (image));

	file = eog_image_get_file (image);

	filename = g_file_get_path (file);

	/* Currently only local files can be set as wallpaper */
	if (filename == NULL || !eog_util_file_is_persistent (file))
	{
		GList *files = NULL;
		GtkAction *action;

		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ImageSetAsWallpaper");
		gtk_action_set_sensitive (action, FALSE);

		priv->copy_file_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
								    "copy_file_cid");
		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->copy_file_cid,
				    _("Saving image locally…"));

		files = g_list_append (files, eog_image_get_file (image));
		priv->copy_job = eog_job_copy_new (files, g_get_user_data_dir ());
		g_signal_connect (priv->copy_job,
				  "finished",
				  G_CALLBACK (eog_job_copy_cb),
				  window);
		g_signal_connect (priv->copy_job,
				  "progress",
				  G_CALLBACK (eog_job_progress_cb),
				  window);
		eog_job_queue_add_job (priv->copy_job);

		g_object_unref (file);
		g_free (filename);
		return;
	}

	g_object_unref (file);

	eog_window_set_wallpaper (window, filename, NULL);

	g_free (filename);
}

static gboolean
eog_window_all_images_trasheable (GList *images)
{
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	EogImage *image;
	gboolean can_trash = TRUE;

	for (iter = images; iter != NULL; iter = g_list_next (iter)) {
		image = (EogImage *) iter->data;
		file = eog_image_get_file (image);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
					       0, NULL, NULL);
		can_trash = g_file_info_get_attribute_boolean (file_info,
							       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);

		g_object_unref (file_info);
		g_object_unref (file);

		if (can_trash == FALSE)
			break;
	}

	return can_trash;
}

static int
show_move_to_trash_confirm_dialog (EogWindow *window, GList *images, gboolean can_trash)
{
	GtkWidget *dlg;
	char *prompt;
	int response;
	int n_images;
	EogImage *image;
	static gboolean dontaskagain = FALSE;
	gboolean neverask = FALSE;
	GtkWidget* dontask_cbutton = NULL;

	/* Check if the user never wants to be bugged. Ignore the error as
	 * it returns FALSE for safety anyway */
	neverask = mateconf_client_get_bool (window->priv->client,
					 EOG_CONF_UI_DISABLE_TRASH_CONFIRMATION,
					 NULL);

	/* Assume agreement, if the user doesn't want to be
	 * asked and the trash is available */
	if (can_trash && (dontaskagain || neverask))
		return GTK_RESPONSE_OK;
	
	n_images = g_list_length (images);

	if (n_images == 1) {
		image = EOG_IMAGE (images->data);
		if (can_trash) {
			prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"),
						  eog_image_get_caption (image));
		} else {
			prompt = g_strdup_printf (_("A trash for \"%s\" couldn't be found. Do you want to remove "
						    "this image permanently?"), eog_image_get_caption (image));
		}
	} else {
		if (can_trash) {
			prompt = g_strdup_printf (ngettext("Are you sure you want to move\n"
							   "the selected image to the trash?",
							   "Are you sure you want to move\n"
							   "the %d selected images to the trash?", n_images), n_images);
		} else {
			prompt = g_strdup (_("Some of the selected images can't be moved to the trash "
					     "and will be removed permanently. Are you sure you want "
					     "to proceed?"));
		}
	}

	dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_NONE,
						  "<span weight=\"bold\" size=\"larger\">%s</span>",
						  prompt);
	g_free (prompt);

	gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	if (can_trash) {
		gtk_dialog_add_button (GTK_DIALOG (dlg), _("Move to _Trash"), GTK_RESPONSE_OK);

		dontask_cbutton = gtk_check_button_new_with_mnemonic (_("_Do not ask again during this session"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dontask_cbutton), FALSE);

		gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), dontask_cbutton, TRUE, TRUE, 0);
	} else {
		if (n_images == 1) {
			gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_DELETE, GTK_RESPONSE_OK);
		} else {
			gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_YES, GTK_RESPONSE_OK);
		}
	}

	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
	gtk_window_set_title (GTK_WINDOW (dlg), "");
	gtk_widget_show_all (dlg);

	response = gtk_dialog_run (GTK_DIALOG (dlg));

	/* Only update the property if the user has accepted */
	if (can_trash && response == GTK_RESPONSE_OK)
		dontaskagain = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dontask_cbutton));

	/* The checkbutton is destroyed together with the dialog */
	gtk_widget_destroy (dlg);

	return response;
}

static gboolean
move_to_trash_real (EogImage *image, GError **error)
{
	GFile *file;
	GFileInfo *file_info;
	gboolean can_trash, result;

	g_return_val_if_fail (EOG_IS_IMAGE (image), FALSE);

	file = eog_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
				       0, NULL, NULL);
	if (file_info == NULL) {
		g_set_error (error,
			     EOG_WINDOW_ERROR,
			     EOG_WINDOW_ERROR_TRASH_NOT_FOUND,
			     _("Couldn't access trash."));
		return FALSE;
	}

	can_trash = g_file_info_get_attribute_boolean (file_info,
						       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);
	g_object_unref (file_info);
	if (can_trash)
	{
		result = g_file_trash (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     EOG_WINDOW_ERROR,
				     EOG_WINDOW_ERROR_TRASH_NOT_FOUND,
				     _("Couldn't access trash."));
		}
	} else {
		result = g_file_delete (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     EOG_WINDOW_ERROR,
				     EOG_WINDOW_ERROR_IO,
				     _("Couldn't delete file"));
		}
	}

        g_object_unref (file);

	return result;
}

static void
eog_window_cmd_move_to_trash (GtkAction *action, gpointer user_data)
{
	GList *images;
	GList *it;
	EogWindowPrivate *priv;
	EogListStore *list;
	int pos;
	EogImage *img;
	EogWindow *window;
	int response;
	int n_images;
	gboolean success;
	gboolean can_trash;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	window = EOG_WINDOW (user_data);
	priv = window->priv;
	list = priv->store;

	n_images = eog_thumb_view_get_n_selected (EOG_THUMB_VIEW (priv->thumbview));

	if (n_images < 1) return;

	/* save position of selected image after the deletion */
	images = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (priv->thumbview));

	g_assert (images != NULL);

	/* HACK: eog_list_store_get_n_selected return list in reverse order */
	images = g_list_reverse (images);

	can_trash = eog_window_all_images_trasheable (images);

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "Delete") == 0 ||
	    can_trash == FALSE) {
		response = show_move_to_trash_confirm_dialog (window, images, can_trash);

		if (response != GTK_RESPONSE_OK) return;
	}

	pos = eog_list_store_get_pos_by_image (list, EOG_IMAGE (images->data));

	/* FIXME: make a nice progress dialog */
	/* Do the work actually. First try to delete the image from the disk. If this
	 * is successfull, remove it from the screen. Otherwise show error dialog.
	 */
	for (it = images; it != NULL; it = it->next) {
		GError *error = NULL;
		EogImage *image;

		image = EOG_IMAGE (it->data);

		success = move_to_trash_real (image, &error);

		if (success) {
			eog_list_store_remove_image (list, image);
		} else {
			char *header;
			GtkWidget *dlg;

			header = g_strdup_printf (_("Error on deleting image %s"),
						  eog_image_get_caption (image));

			dlg = gtk_message_dialog_new (GTK_WINDOW (window),
						      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_ERROR,
						      GTK_BUTTONS_OK,
						      "%s", header);

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
								  "%s", error->message);

			gtk_dialog_run (GTK_DIALOG (dlg));

			gtk_widget_destroy (dlg);

			g_free (header);
		}
	}

	/* free list */
	g_list_foreach (images, (GFunc) g_object_unref, NULL);
	g_list_free (images);

	/* select image at previously saved position */
	pos = MIN (pos, eog_list_store_length (list) - 1);

	if (pos >= 0) {
		img = eog_list_store_get_image_by_pos (list, pos);

		eog_thumb_view_set_current_image (EOG_THUMB_VIEW (priv->thumbview),
						  img,
						  TRUE);

		if (img != NULL) {
			g_object_unref (img);
		}
	}
}

static void
eog_window_cmd_fullscreen (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	gboolean fullscreen;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (user_data);

	fullscreen = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (fullscreen) {
		eog_window_run_fullscreen (window, FALSE);
	} else {
		eog_window_stop_fullscreen (window, FALSE);
	}
}

static void
eog_window_cmd_slideshow (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	gboolean slideshow;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (user_data);

	slideshow = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (slideshow) {
		eog_window_run_fullscreen (window, TRUE);
	} else {
		eog_window_stop_fullscreen (window, TRUE);
	}
}

static void
eog_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data)
{
	EogWindow *window;
	gboolean slideshow;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (user_data);

	slideshow = window->priv->mode == EOG_WINDOW_MODE_SLIDESHOW;

	if (!slideshow && window->priv->mode != EOG_WINDOW_MODE_FULLSCREEN)
		return;

	eog_window_run_fullscreen (window, !slideshow);
}

static void
eog_window_cmd_zoom_in (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	if (priv->view) {
		eog_scroll_view_zoom_in (EOG_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
eog_window_cmd_zoom_out (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	if (priv->view) {
		eog_scroll_view_zoom_out (EOG_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
eog_window_cmd_zoom_normal (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	if (priv->view) {
		eog_scroll_view_set_zoom (EOG_SCROLL_VIEW (priv->view), 1.0);
	}
}

static void
eog_window_cmd_zoom_fit (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	if (priv->view) {
		eog_scroll_view_zoom_fit (EOG_SCROLL_VIEW (priv->view));
	}
}

static void
eog_window_cmd_go_prev (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_LEFT);
}

static void
eog_window_cmd_go_next (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_RIGHT);
}

static void
eog_window_cmd_go_first (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_FIRST);
}

static void
eog_window_cmd_go_last (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_LAST);
}

static void
eog_window_cmd_go_random (GtkAction *action, gpointer user_data)
{
	EogWindowPrivate *priv;

	g_return_if_fail (EOG_IS_WINDOW (user_data));

	eog_debug (DEBUG_WINDOW);

	priv = EOG_WINDOW (user_data)->priv;

	eog_thumb_view_select_single (EOG_THUMB_VIEW (priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_RANDOM);
}

static const GtkActionEntry action_entries_window[] = {
	{ "Image", NULL, N_("_Image") },
	{ "Edit",  NULL, N_("_Edit") },
	{ "View",  NULL, N_("_View") },
	{ "Go",    NULL, N_("_Go") },
	{ "Tools", NULL, N_("_Tools") },
	{ "Help",  NULL, N_("_Help") },

	{ "ImageOpen", GTK_STOCK_OPEN,  N_("_Open…"), "<control>O",
	  N_("Open a file"),
	  G_CALLBACK (eog_window_cmd_file_open) },
	{ "ImageClose", GTK_STOCK_CLOSE, N_("_Close"), "<control>W",
	  N_("Close window"),
	  G_CALLBACK (eog_window_cmd_close_window) },
	{ "EditToolbar", NULL, N_("T_oolbar"), NULL,
	  N_("Edit the application toolbar"),
	  G_CALLBACK (eog_window_cmd_edit_toolbar) },
	{ "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
	  N_("Preferences for Eye of MATE"),
	  G_CALLBACK (eog_window_cmd_preferences) },
	{ "HelpManual", GTK_STOCK_HELP, N_("_Contents"), "F1",
	  N_("Help on this application"),
	  G_CALLBACK (eog_window_cmd_help) },
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
	  N_("About this application"),
	  G_CALLBACK (eog_window_cmd_about) }
};

static const GtkToggleActionEntry toggle_entries_window[] = {
	{ "ViewToolbar", NULL, N_("_Toolbar"), NULL,
	  N_("Changes the visibility of the toolbar in the current window"),
	  G_CALLBACK (eog_window_cmd_show_hide_bar), TRUE },
	{ "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
	  N_("Changes the visibility of the statusbar in the current window"),
	  G_CALLBACK (eog_window_cmd_show_hide_bar), TRUE },
	{ "ViewImageCollection", "eog-image-collection", N_("_Image Collection"), "F9",
	  N_("Changes the visibility of the image collection pane in the current window"),
	  G_CALLBACK (eog_window_cmd_show_hide_bar), TRUE },
	{ "ViewSidebar", NULL, N_("Side _Pane"), "<control>F9",
	  N_("Changes the visibility of the side pane in the current window"),
	  G_CALLBACK (eog_window_cmd_show_hide_bar), TRUE },
};

static const GtkActionEntry action_entries_image[] = {
	{ "ImageSave", GTK_STOCK_SAVE, N_("_Save"), "<control>s",
	  N_("Save changes in currently selected images"),
	  G_CALLBACK (eog_window_cmd_save) },
	{ "ImageOpenWith", NULL, N_("Open _with"), NULL,
	  N_("Open the selected image with a different application"),
	  NULL},
	{ "ImageSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As…"), "<control><shift>s",
	  N_("Save the selected images with a different name"),
	  G_CALLBACK (eog_window_cmd_save_as) },
	{ "ImagePrint", GTK_STOCK_PRINT, N_("_Print…"), "<control>p",
	  N_("Print the selected image"),
	  G_CALLBACK (eog_window_cmd_print) },
	{ "ImageProperties", GTK_STOCK_PROPERTIES, N_("Prope_rties"), "<alt>Return",
	  N_("Show the properties and metadata of the selected image"),
	  G_CALLBACK (eog_window_cmd_properties) },
	{ "EditUndo", GTK_STOCK_UNDO, N_("_Undo"), "<control>z",
	  N_("Undo the last change in the image"),
	  G_CALLBACK (eog_window_cmd_undo) },
	{ "EditFlipHorizontal", "object-flip-horizontal", N_("Flip _Horizontal"), NULL,
	  N_("Mirror the image horizontally"),
	  G_CALLBACK (eog_window_cmd_flip_horizontal) },
	{ "EditFlipVertical", "object-flip-vertical", N_("Flip _Vertical"), NULL,
	  N_("Mirror the image vertically"),
	  G_CALLBACK (eog_window_cmd_flip_vertical) },
	{ "EditRotate90",  "object-rotate-right",  N_("_Rotate Clockwise"), "<control>r",
	  N_("Rotate the image 90 degrees to the right"),
	  G_CALLBACK (eog_window_cmd_rotate_90) },
	{ "EditRotate270", "object-rotate-left", N_("Rotate Counterc_lockwise"), "<ctrl><shift>r",
	  N_("Rotate the image 90 degrees to the left"),
	  G_CALLBACK (eog_window_cmd_rotate_270) },
	{ "ImageSetAsWallpaper", NULL, N_("Set as _Desktop Background"),
	  "<control>F8", N_("Set the selected image as the desktop background"),
	  G_CALLBACK (eog_window_cmd_wallpaper) },
	{ "EditMoveToTrash", "user-trash", N_("Move to _Trash"), NULL,
	  N_("Move the selected image to the trash folder"),
	  G_CALLBACK (eog_window_cmd_move_to_trash) },
	{ "ViewZoomIn", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>plus",
	  N_("Enlarge the image"),
	  G_CALLBACK (eog_window_cmd_zoom_in) },
	{ "ViewZoomOut", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>minus",
	  N_("Shrink the image"),
	  G_CALLBACK (eog_window_cmd_zoom_out) },
	{ "ViewZoomNormal", GTK_STOCK_ZOOM_100, N_("_Normal Size"), "<control>0",
	  N_("Show the image at its normal size"),
	  G_CALLBACK (eog_window_cmd_zoom_normal) },
	{ "ViewZoomFit", GTK_STOCK_ZOOM_FIT, N_("Best _Fit"), "F",
	  N_("Fit the image to the window"),
	  G_CALLBACK (eog_window_cmd_zoom_fit) },
	{ "ControlEqual", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>equal",
	  N_("Enlarge the image"),
	  G_CALLBACK (eog_window_cmd_zoom_in) },
	{ "ControlKpAdd", GTK_STOCK_ZOOM_IN, N_("_Zoom In"), "<control>KP_Add",
	  N_("Shrink the image"),
	  G_CALLBACK (eog_window_cmd_zoom_in) },
	{ "ControlKpSub", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>KP_Subtract",
	  N_("Shrink the image"),
	  G_CALLBACK (eog_window_cmd_zoom_out) },
	{ "Delete", NULL, N_("Move to _Trash"), "Delete",
	  NULL,
	  G_CALLBACK (eog_window_cmd_move_to_trash) },
};

static const GtkToggleActionEntry toggle_entries_image[] = {
	{ "ViewFullscreen", GTK_STOCK_FULLSCREEN, N_("_Fullscreen"), "F11",
	  N_("Show the current image in fullscreen mode"),
	  G_CALLBACK (eog_window_cmd_fullscreen), FALSE },
	{ "PauseSlideshow", "media-playback-pause", N_("Pause Slideshow"),
	  NULL, N_("Pause or resume the slideshow"),
	  G_CALLBACK (eog_window_cmd_pause_slideshow), FALSE },
};

static const GtkActionEntry action_entries_collection[] = {
	{ "GoPrevious", GTK_STOCK_GO_BACK, N_("_Previous Image"), "<Alt>Left",
	  N_("Go to the previous image of the collection"),
	  G_CALLBACK (eog_window_cmd_go_prev) },
	{ "GoNext", GTK_STOCK_GO_FORWARD, N_("_Next Image"), "<Alt>Right",
	  N_("Go to the next image of the collection"),
	  G_CALLBACK (eog_window_cmd_go_next) },
	{ "GoFirst", GTK_STOCK_GOTO_FIRST, N_("_First Image"), "<Alt>Home",
	  N_("Go to the first image of the collection"),
	  G_CALLBACK (eog_window_cmd_go_first) },
	{ "GoLast", GTK_STOCK_GOTO_LAST, N_("_Last Image"), "<Alt>End",
	  N_("Go to the last image of the collection"),
	  G_CALLBACK (eog_window_cmd_go_last) },
	{ "GoRandom", NULL, N_("_Random Image"), "<control>M",
	  N_("Go to a random image of the collection"),
	  G_CALLBACK (eog_window_cmd_go_random) },
	{ "BackSpace", NULL, N_("_Previous Image"), "BackSpace",
	  NULL,
	  G_CALLBACK (eog_window_cmd_go_prev) },
	{ "Home", NULL, N_("_First Image"), "Home",
	  NULL,
	  G_CALLBACK (eog_window_cmd_go_first) },
	{ "End", NULL, N_("_Last Image"), "End",
	  NULL,
	  G_CALLBACK (eog_window_cmd_go_last) },
};

static const GtkToggleActionEntry toggle_entries_collection[] = {
	{ "ViewSlideshow", "slideshow-play", N_("_Slideshow"), "F5",
	  N_("Start a slideshow view of the images"),
	  G_CALLBACK (eog_window_cmd_slideshow), FALSE },
};

static void
menu_item_select_cb (GtkMenuItem *proxy, EogWindow *window)
{
	GtkAction *action;
	char *message;

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));

	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy, EogWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  EogWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     EogWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
set_action_properties (GtkActionGroup *window_group,
		       GtkActionGroup *image_group,
		       GtkActionGroup *collection_group)
{
        GtkAction *action;

        action = gtk_action_group_get_action (collection_group, "GoPrevious");
        g_object_set (action, "short_label", _("Previous"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = gtk_action_group_get_action (collection_group, "GoNext");
        g_object_set (action, "short_label", _("Next"), NULL);
        g_object_set (action, "is-important", TRUE, NULL);

        action = gtk_action_group_get_action (image_group, "EditRotate90");
        g_object_set (action, "short_label", _("Right"), NULL);

        action = gtk_action_group_get_action (image_group, "EditRotate270");
        g_object_set (action, "short_label", _("Left"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomIn");
        g_object_set (action, "short_label", _("In"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomOut");
        g_object_set (action, "short_label", _("Out"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomNormal");
        g_object_set (action, "short_label", _("Normal"), NULL);

        action = gtk_action_group_get_action (image_group, "ViewZoomFit");
        g_object_set (action, "short_label", _("Fit"), NULL);

        action = gtk_action_group_get_action (window_group, "ViewImageCollection");
        g_object_set (action, "short_label", _("Collection"), NULL);

        action = gtk_action_group_get_action (image_group, "EditMoveToTrash");
        g_object_set (action, "short_label", C_("action (to trash)", "Trash"), NULL);
}

static gint
sort_recents_mru (GtkRecentInfo *a, GtkRecentInfo *b)
{
	gboolean has_eog_a, has_eog_b;

	/* We need to check this first as gtk_recent_info_get_application_info
	 * will treat it as a non-fatal error when the GtkRecentInfo doesn't
	 * have the application registered. */
	has_eog_a = gtk_recent_info_has_application (a,
						     EOG_RECENT_FILES_APP_NAME);
	has_eog_b = gtk_recent_info_has_application (b,
						     EOG_RECENT_FILES_APP_NAME);
	if (has_eog_a && has_eog_b) {
		time_t time_a, time_b;

		/* These should not fail as we already checked that
		 * the application is registered with the info objects */
		gtk_recent_info_get_application_info (a,
						      EOG_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_a);
		gtk_recent_info_get_application_info (b,
						      EOG_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_b);

		return (time_b - time_a);
	} else if (has_eog_a) {
		return -1;
	} else if (has_eog_b) {
		return 1;
	}

	return 0;
}

static void
eog_window_update_recent_files_menu (EogWindow *window)
{
	EogWindowPrivate *priv;
	GList *actions = NULL, *li = NULL, *items = NULL;
	guint count_recent = 0;

	priv = window->priv;

	if (priv->recent_menu_id != 0)
		gtk_ui_manager_remove_ui (priv->ui_mgr, priv->recent_menu_id);

	actions = gtk_action_group_list_actions (priv->actions_recent);

	for (li = actions; li != NULL; li = li->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (li->data),
						      G_CALLBACK(eog_window_open_recent_cb),
						      window);

		gtk_action_group_remove_action (priv->actions_recent,
						GTK_ACTION (li->data));
	}

	g_list_free (actions);

	priv->recent_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);
	items = gtk_recent_manager_get_items (gtk_recent_manager_get_default());
	items = g_list_sort (items, (GCompareFunc) sort_recents_mru);

	for (li = items; li != NULL && count_recent < EOG_RECENT_FILES_LIMIT; li = li->next) {
		gchar *action_name;
		gchar *label;
		gchar *tip;
		gchar **display_name;
		gchar *label_filename;
		GtkAction *action;
		GtkRecentInfo *info = li->data;

		/* Sorting moves non-EOG files to the end of the list.
		 * So no file of interest will follow if this test fails */
		if (!gtk_recent_info_has_application (info, EOG_RECENT_FILES_APP_NAME))
			break;

		count_recent++;

		action_name = g_strdup_printf ("recent-info-%d", count_recent);
		display_name = g_strsplit (gtk_recent_info_get_display_name (info), "_", -1);
		label_filename = g_strjoinv ("__", display_name);
		label = g_strdup_printf ("%s_%d. %s",
				(is_rtl ? "\xE2\x80\x8F" : ""), count_recent, label_filename);
		g_free (label_filename);
		g_strfreev (display_name);

		tip = gtk_recent_info_get_uri_display (info);

		/* This is a workaround for a bug (#351945) regarding
		 * gtk_recent_info_get_uri_display() and remote URIs.
		 * mate_vfs_format_uri_for_display is sufficient here
		 * since the password gets stripped when adding the
		 * file to the recently used list. */
		if (tip == NULL)
			tip = g_uri_unescape_string (gtk_recent_info_get_uri (info), NULL);

		action = gtk_action_new (action_name, label, tip, NULL);
		gtk_action_set_always_show_image (action, TRUE);

		g_object_set_data_full (G_OBJECT (action), "gtk-recent-info",
					gtk_recent_info_ref (info),
					(GDestroyNotify) gtk_recent_info_unref);

		g_object_set (G_OBJECT (action), "icon-name", "image-x-generic", NULL);

		g_signal_connect (action, "activate",
				  G_CALLBACK (eog_window_open_recent_cb),
				  window);

		gtk_action_group_add_action (priv->actions_recent, action);

		g_object_unref (action);

		gtk_ui_manager_add_ui (priv->ui_mgr, priv->recent_menu_id,
				       "/MainMenu/Image/RecentDocuments",
				       action_name, action_name,
				       GTK_UI_MANAGER_AUTO, FALSE);

		g_free (action_name);
		g_free (label);
		g_free (tip);
	}

	g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (items);
}

static void
eog_window_recent_manager_changed_cb (GtkRecentManager *manager, EogWindow *window)
{
	eog_window_update_recent_files_menu (window);
}

static void
eog_window_drag_data_received (GtkWidget *widget,
                               GdkDragContext *context,
                               gint x, gint y,
                               GtkSelectionData *selection_data,
                               guint info, guint time)
{
        GSList *file_list;
        EogWindow *window;
	GdkAtom target;

	target = gtk_selection_data_get_target (selection_data);

        if (!gtk_targets_include_uri (&target, 1))
                return;

        if (context->suggested_action == GDK_ACTION_COPY) {
                window = EOG_WINDOW (widget);

                file_list = eog_util_parse_uri_string_list_to_file_list ((const gchar *) gtk_selection_data_get_data (selection_data));

		eog_window_open_file_list (window, file_list);
        }
}

static void
eog_window_set_drag_dest (EogWindow *window)
{
        gtk_drag_dest_set (GTK_WIDGET (window),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           NULL, 0,
                           GDK_ACTION_COPY | GDK_ACTION_ASK);
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (window));
}

static void
eog_window_sidebar_visibility_changed (GtkWidget *widget, EogWindow *window)
{
	GtkAction *action;
	gboolean visible;

	visible = gtk_widget_get_visible (window->priv->sidebar);

	mateconf_client_set_bool (window->priv->client,
			       EOG_CONF_UI_SIDEBAR,
			       visible,
			       NULL);

	action = gtk_action_group_get_action (window->priv->actions_window,
					      "ViewSidebar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* Focus the image */
	if (!visible && window->priv->image != NULL)
		gtk_widget_grab_focus (window->priv->view);
}

static void
eog_window_sidebar_page_added (EogSidebar  *sidebar,
			       GtkWidget   *main_widget,
			       EogWindow   *window)
{
	if (eog_sidebar_get_n_pages (sidebar) == 1) {
		GtkAction *action;
		gboolean show;

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, TRUE);

		show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

		if (show)
			gtk_widget_show (GTK_WIDGET (sidebar));
	}
}
static void
eog_window_sidebar_page_removed (EogSidebar  *sidebar,
			         GtkWidget   *main_widget,
			         EogWindow   *window)
{
	if (eog_sidebar_is_empty (sidebar)) {
		GtkAction *action;

		gtk_widget_hide (GTK_WIDGET (sidebar));

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, FALSE);
	}
}

static void
eog_window_finish_saving (EogWindow *window)
{
	EogWindowPrivate *priv = window->priv;

	gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

	do {
		gtk_main_iteration ();
	} while (priv->save_job != NULL);
}

static void
eog_window_construct_ui (EogWindow *window)
{
	EogWindowPrivate *priv;

	GError *error = NULL;

	GtkWidget *menubar;
	GtkWidget *thumb_popup;
	GtkWidget *view_popup;
	GtkWidget *hpaned;
	GtkWidget *menuitem;

	MateConfEntry *entry;

	g_return_if_fail (EOG_IS_WINDOW (window));

	priv = window->priv;

	priv->box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), priv->box);
	gtk_widget_show (priv->box);

	priv->ui_mgr = gtk_ui_manager_new ();

	priv->actions_window = gtk_action_group_new ("MenuActionsWindow");

	gtk_action_group_set_translation_domain (priv->actions_window,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_window,
				      action_entries_window,
				      G_N_ELEMENTS (action_entries_window),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_window,
					     toggle_entries_window,
					     G_N_ELEMENTS (toggle_entries_window),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_window, 0);

	priv->actions_image = gtk_action_group_new ("MenuActionsImage");
	gtk_action_group_set_translation_domain (priv->actions_image,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_image,
				      action_entries_image,
				      G_N_ELEMENTS (action_entries_image),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_image,
					     toggle_entries_image,
					     G_N_ELEMENTS (toggle_entries_image),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_image, 0);

	priv->actions_collection = gtk_action_group_new ("MenuActionsCollection");
	gtk_action_group_set_translation_domain (priv->actions_collection,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_collection,
				      action_entries_collection,
				      G_N_ELEMENTS (action_entries_collection),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_collection,
					     toggle_entries_collection,
					     G_N_ELEMENTS (toggle_entries_collection),
					     window);

	set_action_properties (priv->actions_window,
			       priv->actions_image,
			       priv->actions_collection);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_collection, 0);

	if (!gtk_ui_manager_add_ui_from_file (priv->ui_mgr,
					      EOG_DATA_DIR"/eog-ui.xml",
					      &error)) {
                g_warning ("building menus failed: %s", error->message);
                g_error_free (error);
        }

	g_signal_connect (priv->ui_mgr, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_mgr, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_box_pack_start (GTK_BOX (priv->box), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipHorizontal");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipVertical");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate90");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate270");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	priv->toolbar = GTK_WIDGET
		(g_object_new (EGG_TYPE_EDITABLE_TOOLBAR,
			       "ui-manager", priv->ui_mgr,
			       "model", eog_application_get_toolbars_model (EOG_APP),
			       NULL));

	egg_editable_toolbar_show (EGG_EDITABLE_TOOLBAR (priv->toolbar),
				   "Toolbar");

	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->toolbar,
			    FALSE,
			    FALSE,
			    0);

	gtk_widget_show (priv->toolbar);

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_mgr));

	priv->actions_recent = gtk_action_group_new ("RecentFilesActions");
	gtk_action_group_set_translation_domain (priv->actions_recent,
						 GETTEXT_PACKAGE);

	g_signal_connect (gtk_recent_manager_get_default (), "changed",
			  G_CALLBACK (eog_window_recent_manager_changed_cb),
			  window);

	eog_window_update_recent_files_menu (window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_recent, 0);

	priv->cbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->cbox, TRUE, TRUE, 0);
	gtk_widget_show (priv->cbox);

	priv->statusbar = eog_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (priv->box),
			  GTK_WIDGET (priv->statusbar),
			  FALSE, FALSE, 0);
	gtk_widget_show (priv->statusbar);

	priv->image_info_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "image_info_message");
	priv->tip_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "tip_message");

	priv->layout = gtk_vbox_new (FALSE, 2);

	hpaned = gtk_hpaned_new ();

	priv->sidebar = eog_sidebar_new ();
	/* The sidebar shouldn't be shown automatically on show_all(),
	   but only when the user actually wants it. */
	gtk_widget_set_no_show_all (priv->sidebar, TRUE);

	gtk_widget_set_size_request (priv->sidebar, 210, -1);

	g_signal_connect_after (priv->sidebar,
				"show",
				G_CALLBACK (eog_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"hide",
				G_CALLBACK (eog_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-added",
				G_CALLBACK (eog_window_sidebar_page_added),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-removed",
				G_CALLBACK (eog_window_sidebar_page_removed),
				window);

 	priv->view = eog_scroll_view_new ();
	gtk_widget_set_size_request (GTK_WIDGET (priv->view), 100, 100);
	g_signal_connect (G_OBJECT (priv->view),
			  "zoom_changed",
			  G_CALLBACK (view_zoom_changed_cb),
			  window);

	view_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ViewPopup");
	eog_scroll_view_set_popup (EOG_SCROLL_VIEW (priv->view),
				   GTK_MENU (view_popup));

	gtk_paned_pack1 (GTK_PANED (hpaned),
			 priv->sidebar,
			 FALSE,
			 FALSE);

	gtk_paned_pack2 (GTK_PANED (hpaned),
			 priv->view,
			 TRUE,
			 FALSE);

	gtk_widget_show_all (hpaned);

	gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);

	priv->thumbview = eog_thumb_view_new ();

	/* giving shape to the view */
	gtk_icon_view_set_margin (GTK_ICON_VIEW (priv->thumbview), 4);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (priv->thumbview), 0);

	g_signal_connect (G_OBJECT (priv->thumbview), "selection_changed",
			  G_CALLBACK (handle_image_selection_changed_cb), window);

	priv->nav = eog_thumb_nav_new (priv->thumbview,
				       EOG_THUMB_NAV_MODE_ONE_ROW,
				       mateconf_client_get_bool (priv->client,
							      EOG_CONF_UI_SCROLL_BUTTONS,
							      NULL));

	thumb_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ThumbnailPopup");
	eog_thumb_view_set_thumbnail_popup (EOG_THUMB_VIEW (priv->thumbview),
					    GTK_MENU (thumb_popup));

	gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);


	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_EXTRAPOLATE,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_interp_in_type_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_INTERPOLATE,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_interp_out_type_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_SCROLL_WHEEL_ZOOM,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_scroll_wheel_zoom_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_ZOOM_MULTIPLIER,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_zoom_multiplier_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_BACKGROUND_COLOR,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_bg_color_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_USE_BG_COLOR,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_use_bg_color_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_TRANSPARENCY,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_transparency_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_VIEW_TRANS_COLOR,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_trans_color_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_UI_IMAGE_COLLECTION_POSITION,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_collection_mode_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	entry = mateconf_client_get_entry (priv->client,
					EOG_CONF_DESKTOP_CAN_SAVE,
					NULL, TRUE, NULL);
	if (entry != NULL) {
		eog_window_can_save_changed_cb (priv->client, 0, entry, window);
		mateconf_entry_unref (entry);
		entry = NULL;
	}

	if ((priv->flags & EOG_STARTUP_FULLSCREEN) ||
	    (priv->flags & EOG_STARTUP_SLIDE_SHOW)) {
		eog_window_run_fullscreen (window, (priv->flags & EOG_STARTUP_SLIDE_SHOW));
	} else {
		priv->mode = EOG_WINDOW_MODE_NORMAL;
		update_ui_visibility (window);
	}

	eog_window_set_drag_dest (window);
}

static void
eog_window_init (EogWindow *window)
{
	GdkGeometry hints;
	GdkScreen *screen;
	EogWindowPrivate *priv;

	eog_debug (DEBUG_WINDOW);

	hints.min_width  = EOG_WINDOW_MIN_WIDTH;
	hints.min_height = EOG_WINDOW_MIN_HEIGHT;

	screen = gtk_widget_get_screen (GTK_WIDGET (window));

	priv = window->priv = EOG_WINDOW_GET_PRIVATE (window);

	priv->client = mateconf_client_get_default ();

	mateconf_client_add_dir (window->priv->client, EOG_CONF_DIR,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_EXTRAPOLATE] =
		mateconf_client_notify_add (window->priv->client,
				 	EOG_CONF_VIEW_EXTRAPOLATE,
					 eog_window_interp_in_type_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_INTERPOLATE] =
		mateconf_client_notify_add (window->priv->client,
				 	 EOG_CONF_VIEW_INTERPOLATE,
					 eog_window_interp_out_type_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_SCROLLWHEEL_ZOOM] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_SCROLL_WHEEL_ZOOM,
					 eog_window_scroll_wheel_zoom_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_ZOOM_MULTIPLIER] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_ZOOM_MULTIPLIER,
					 eog_window_zoom_multiplier_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_BACKGROUND_COLOR] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_BACKGROUND_COLOR,
					 eog_window_bg_color_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_USE_BG_COLOR] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_USE_BG_COLOR,
					 eog_window_use_bg_color_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_TRANSPARENCY] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_TRANSPARENCY,
					 eog_window_transparency_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_TRANS_COLOR] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_VIEW_TRANS_COLOR,
					 eog_window_trans_color_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_SCROLL_BUTTONS] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_UI_SCROLL_BUTTONS,
					 eog_window_scroll_buttons_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_COLLECTION_POS] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_UI_IMAGE_COLLECTION_POSITION,
					 eog_window_collection_mode_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_COLLECTION_RESIZABLE] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_UI_IMAGE_COLLECTION_RESIZABLE,
					 eog_window_collection_mode_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_CAN_SAVE] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_DESKTOP_CAN_SAVE,
					 eog_window_can_save_changed_cb,
					 window, NULL, NULL);

	priv->client_notifications[EOG_WINDOW_NOTIFY_PROPSDIALOG_NETBOOK_MODE] =
		mateconf_client_notify_add (window->priv->client,
					 EOG_CONF_UI_PROPSDIALOG_NETBOOK_MODE,
					 eog_window_pd_nbmode_changed_cb,
					 window, NULL, NULL);

	window->priv->store = NULL;
	window->priv->image = NULL;

	window->priv->fullscreen_popup = NULL;
	window->priv->fullscreen_timeout_source = NULL;
	window->priv->slideshow_loop = FALSE;
	window->priv->slideshow_switch_timeout = 0;
	window->priv->slideshow_switch_source = NULL;

	gtk_window_set_geometry_hints (GTK_WINDOW (window),
				       GTK_WIDGET (window),
				       &hints,
				       GDK_HINT_MIN_SIZE);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     EOG_WINDOW_DEFAULT_WIDTH,
				     EOG_WINDOW_DEFAULT_HEIGHT);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	window->priv->mode = EOG_WINDOW_MODE_UNKNOWN;
	window->priv->status = EOG_WINDOW_STATUS_UNKNOWN;

#ifdef HAVE_LCMS
	window->priv->display_profile =
		eog_window_get_display_profile (screen);
#endif

	window->priv->recent_menu_id = 0;

	window->priv->collection_position = 0;
	window->priv->collection_resizable = FALSE;

	window->priv->save_disabled = FALSE;
}

static void
eog_window_dispose (GObject *object)
{
	EogWindow *window;
	EogWindowPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (EOG_IS_WINDOW (object));

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (object);
	priv = window->priv;

	eog_plugin_engine_garbage_collect ();

	if (priv->store != NULL) {
		g_signal_handlers_disconnect_by_func (priv->store,
					      eog_window_list_store_image_added,
					      window);
		g_signal_handlers_disconnect_by_func (priv->store,
					    eog_window_list_store_image_removed,
					    window);
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	if (priv->image != NULL) {
	  	g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);
		g_object_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->actions_window != NULL) {
		g_object_unref (priv->actions_window);
		priv->actions_window = NULL;
	}

	if (priv->actions_image != NULL) {
		g_object_unref (priv->actions_image);
		priv->actions_image = NULL;
	}

	if (priv->actions_collection != NULL) {
		g_object_unref (priv->actions_collection);
		priv->actions_collection = NULL;
	}

	if (priv->actions_recent != NULL) {
		g_object_unref (priv->actions_recent);
		priv->actions_recent = NULL;
	}

        if (priv->actions_open_with != NULL) {
                g_object_unref (priv->actions_open_with);
                priv->actions_open_with = NULL;
        }

	fullscreen_clear_timeout (window);

	if (window->priv->fullscreen_popup != NULL) {
		gtk_widget_destroy (priv->fullscreen_popup);
		priv->fullscreen_popup = NULL;
	}

	slideshow_clear_timeout (window);

	g_signal_handlers_disconnect_by_func (gtk_recent_manager_get_default (),
					      G_CALLBACK (eog_window_recent_manager_changed_cb),
					      window);

	priv->recent_menu_id = 0;

	eog_window_clear_load_job (window);

	eog_window_clear_transform_job (window);

	if (priv->client) {
		int i;

		for (i = 0; i < EOG_WINDOW_NOTIFY_LENGTH; ++i) {
			mateconf_client_notify_remove (priv->client,
						 priv->client_notifications[i]);
		}
		mateconf_client_remove_dir (priv->client, EOG_CONF_DIR, NULL);
		g_object_unref (priv->client);
		priv->client = NULL;
	}

	if (priv->file_list != NULL) {
		g_slist_foreach (priv->file_list, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->file_list);
		priv->file_list = NULL;
	}

#ifdef HAVE_LCMS
	if (priv->display_profile != NULL) {
		cmsCloseProfile (priv->display_profile);
		priv->display_profile = NULL;
	}
#endif

	if (priv->last_save_as_folder != NULL) {
		g_object_unref (priv->last_save_as_folder);
		priv->last_save_as_folder = NULL;
	}

	eog_plugin_engine_garbage_collect ();

	G_OBJECT_CLASS (eog_window_parent_class)->dispose (object);
}

static void
eog_window_finalize (GObject *object)
{
        GList *windows = eog_application_get_windows (EOG_APP);

	g_return_if_fail (EOG_IS_WINDOW (object));

	eog_debug (DEBUG_WINDOW);

        if (windows == NULL) {
                eog_application_shutdown (EOG_APP);
        } else {
                g_list_free (windows);
	}

        G_OBJECT_CLASS (eog_window_parent_class)->finalize (object);
}

static gint
eog_window_delete (GtkWidget *widget, GdkEventAny *event)
{
	EogWindow *window;
	EogWindowPrivate *priv;

	g_return_val_if_fail (EOG_IS_WINDOW (widget), FALSE);

	window = EOG_WINDOW (widget);
	priv = window->priv;

	if (priv->save_job != NULL) {
		eog_window_finish_saving (window);
	}

	if (eog_window_unsaved_images_confirm (window)) {
		return TRUE;
	}

	gtk_widget_destroy (widget);

	return TRUE;
}

static gint
eog_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	GtkContainer *tbcontainer = GTK_CONTAINER ((EOG_WINDOW (widget)->priv->toolbar));
	gint result = FALSE;
	gboolean handle_selection = FALSE;

	switch (event->keyval) {
	case GDK_space:
		if (event->state & GDK_CONTROL_MASK) {
			handle_selection = TRUE;
			break;
		}
	case GDK_Return:
		if (gtk_container_get_focus_child (tbcontainer) == NULL) {
			/* Image properties dialog case */
			if (event->state & GDK_MOD1_MASK) {
				result = FALSE;
				break;
			}

			if (event->state & GDK_SHIFT_MASK) {
				eog_window_cmd_go_prev (NULL, EOG_WINDOW (widget));
			} else {
				eog_window_cmd_go_next (NULL, EOG_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case GDK_p:
	case GDK_P:
		if (EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_FULLSCREEN || EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_SLIDESHOW) {
			gboolean slideshow;

			slideshow = EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_SLIDESHOW;
			eog_window_run_fullscreen (EOG_WINDOW (widget), !slideshow);
		}
		break;
	case GDK_Q:
	case GDK_q:
	case GDK_Escape:
		if (EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_FULLSCREEN) {
			eog_window_stop_fullscreen (EOG_WINDOW (widget), FALSE);
		} else if (EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_SLIDESHOW) {
			eog_window_stop_fullscreen (EOG_WINDOW (widget), TRUE);
		} else {
			eog_window_cmd_close_window (NULL, EOG_WINDOW (widget));
			return TRUE;
		}
		break;
	case GDK_Left:
		if (event->state & GDK_MOD1_MASK) {
			/* Alt+Left moves to previous image */
			if (is_rtl) { /* move to next in RTL mode */
				eog_window_cmd_go_next (NULL, EOG_WINDOW (widget));
			} else {
				eog_window_cmd_go_prev (NULL, EOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case GDK_Up:
		if (eog_scroll_view_scrollbars_visible (EOG_SCROLL_VIEW (EOG_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (gtk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!gtk_widget_get_visible (EOG_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == GDK_Left) {
				/* handle RTL fall-through,
				 * need to behave like GDK_Down then */
				eog_window_cmd_go_next (NULL,
							EOG_WINDOW (widget));
			} else {
				eog_window_cmd_go_prev (NULL,
							EOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case GDK_Right:
		if (event->state & GDK_MOD1_MASK) {
			/* Alt+Right moves to next image */
			if (is_rtl) { /* move to previous in RTL mode */
				eog_window_cmd_go_prev (NULL, EOG_WINDOW (widget));
			} else {
				eog_window_cmd_go_next (NULL, EOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		} /* else fall-trough is intended */
	case GDK_Down:
		if (eog_scroll_view_scrollbars_visible (EOG_SCROLL_VIEW (EOG_WINDOW (widget)->priv->view))) {
			/* break to let scrollview handle the key */
			break;
		}
		if (gtk_container_get_focus_child (tbcontainer) != NULL)
			break;
		if (!gtk_widget_get_visible (EOG_WINDOW (widget)->priv->nav)) {
			if (is_rtl && event->keyval == GDK_Right) {
				/* handle RTL fall-through,
				 * need to behave like GDK_Up then */
				eog_window_cmd_go_prev (NULL,
							EOG_WINDOW (widget));
			} else {
				eog_window_cmd_go_next (NULL,
							EOG_WINDOW (widget));
			}
			result = TRUE;
			break;
		}
	case GDK_Page_Up:
		if (!eog_scroll_view_scrollbars_visible (EOG_SCROLL_VIEW (EOG_WINDOW (widget)->priv->view))) {
			if (!gtk_widget_get_visible (EOG_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * previous image manually as it won't handle
				 * the keypress then. */
				eog_window_cmd_go_prev (NULL,
							EOG_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	case GDK_Page_Down:
		if (!eog_scroll_view_scrollbars_visible (EOG_SCROLL_VIEW (EOG_WINDOW (widget)->priv->view))) {
			if (!gtk_widget_get_visible (EOG_WINDOW (widget)->priv->nav)) {
				/* If the iconview is not visible skip to the
				 * next image manually as it won't handle
				 * the keypress then. */
				eog_window_cmd_go_next (NULL,
							EOG_WINDOW (widget));
				result = TRUE;
			} else
				handle_selection = TRUE;
		}
		break;
	}

	/* Update slideshow timeout */
	if (result && (EOG_WINDOW (widget)->priv->mode == EOG_WINDOW_MODE_SLIDESHOW)) {
		slideshow_set_timeout (EOG_WINDOW (widget));
	}

	if (handle_selection == TRUE && result == FALSE) {
		gtk_widget_grab_focus (GTK_WIDGET (EOG_WINDOW (widget)->priv->thumbview));

		result = gtk_widget_event (GTK_WIDGET (EOG_WINDOW (widget)->priv->thumbview),
					   (GdkEvent *) event);
	}

	/* If the focus is not in the toolbar and we still haven't handled the
	   event, give the scrollview a chance to do it.  */
	if (!gtk_container_get_focus_child (tbcontainer) && result == FALSE &&
#if GTK_CHECK_VERSION (2, 20, 0)
		gtk_widget_get_realized (GTK_WIDGET (EOG_WINDOW (widget)->priv->view))) {
#else
		GTK_WIDGET_REALIZED(GTK_WIDGET (EOG_WINDOW (widget)->priv->view))) {
#endif
			result = gtk_widget_event (GTK_WIDGET (EOG_WINDOW (widget)->priv->view),
						   (GdkEvent *) event);
	}

	if (result == FALSE && GTK_WIDGET_CLASS (eog_window_parent_class)->key_press_event) {
		result = (* GTK_WIDGET_CLASS (eog_window_parent_class)->key_press_event) (widget, event);
	}

	return result;
}

static gint
eog_window_button_press (GtkWidget *widget, GdkEventButton *event)
{
	EogWindow *window = EOG_WINDOW (widget);
	gint result = FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		switch (event->button) {
		case 6:
			eog_thumb_view_select_single (EOG_THUMB_VIEW (window->priv->thumbview),
						      EOG_THUMB_VIEW_SELECT_LEFT);
			result = TRUE;
		       	break;
		case 7:
			eog_thumb_view_select_single (EOG_THUMB_VIEW (window->priv->thumbview),
						      EOG_THUMB_VIEW_SELECT_RIGHT);
			result = TRUE;
		       	break;
		}
	}

	if (result == FALSE && GTK_WIDGET_CLASS (eog_window_parent_class)->button_press_event) {
		result = (* GTK_WIDGET_CLASS (eog_window_parent_class)->button_press_event) (widget, event);
	}

	return result;
}

static gboolean
eog_window_configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
	EogWindow *window;

	g_return_val_if_fail (EOG_IS_WINDOW (widget), TRUE);

	window = EOG_WINDOW (widget);

	GTK_WIDGET_CLASS (eog_window_parent_class)->configure_event (widget, event);

	return FALSE;
}

static gboolean
eog_window_window_state_event (GtkWidget *widget,
			       GdkEventWindowState *event)
{
	EogWindow *window;

	g_return_val_if_fail (EOG_IS_WINDOW (widget), TRUE);

	window = EOG_WINDOW (widget);

	if (event->changed_mask &
	    (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN))	{
		gboolean show;

		show = !(event->new_window_state &
		         (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN));

		eog_statusbar_set_has_resize_grip (EOG_STATUSBAR (window->priv->statusbar),
						   show);
	}

	return FALSE;
}

static void
eog_window_unrealize (GtkWidget *widget)
{
	EogWindow *window;

	g_return_if_fail (EOG_IS_WINDOW (widget));

	window = EOG_WINDOW (widget);

	GTK_WIDGET_CLASS (eog_window_parent_class)->unrealize (widget);
}


static gboolean
eog_window_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	EogWindow *window = EOG_WINDOW (widget);
	EogWindowPrivate *priv = window->priv;
	gboolean fullscreen;

	eog_debug (DEBUG_WINDOW);

	fullscreen = priv->mode == EOG_WINDOW_MODE_FULLSCREEN ||
		     priv->mode == EOG_WINDOW_MODE_SLIDESHOW;

	if (fullscreen) {
		gtk_widget_hide_all (priv->fullscreen_popup);
	}

	return GTK_WIDGET_CLASS (eog_window_parent_class)->focus_out_event (widget, event);
}

static void
eog_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	EogWindow *window;
	EogWindowPrivate *priv;

        g_return_if_fail (EOG_IS_WINDOW (object));

        window = EOG_WINDOW (object);
	priv = window->priv;

        switch (property_id) {
	case PROP_STARTUP_FLAGS:
		priv->flags = g_value_get_flags (value);
		break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
eog_window_get_property (GObject    *object,
			 guint       property_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	EogWindow *window;
	EogWindowPrivate *priv;

        g_return_if_fail (EOG_IS_WINDOW (object));

        window = EOG_WINDOW (object);
	priv = window->priv;

        switch (property_id) {
	case PROP_STARTUP_FLAGS:
		g_value_set_flags (value, priv->flags);
		break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static GObject *
eog_window_constructor (GType type,
			guint n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (eog_window_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	eog_window_construct_ui (EOG_WINDOW (object));

	eog_plugin_engine_update_plugins_ui (EOG_WINDOW (object), TRUE);

	return object;
}

static void
eog_window_class_init (EogWindowClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

	g_object_class->constructor = eog_window_constructor;
	g_object_class->dispose = eog_window_dispose;
	g_object_class->finalize = eog_window_finalize;
	g_object_class->set_property = eog_window_set_property;
	g_object_class->get_property = eog_window_get_property;

	widget_class->delete_event = eog_window_delete;
	widget_class->key_press_event = eog_window_key_press;
	widget_class->button_press_event = eog_window_button_press;
	widget_class->drag_data_received = eog_window_drag_data_received;
        widget_class->configure_event = eog_window_configure_event;
        widget_class->window_state_event = eog_window_window_state_event;
	widget_class->unrealize = eog_window_unrealize;
	widget_class->focus_out_event = eog_window_focus_out_event;

/**
 * EogWindow:startup-flags:
 *
 * A bitwise OR of #EogStartupFlags elements, indicating how the window
 * should behave upon creation.
 */
	g_object_class_install_property (g_object_class,
					 PROP_STARTUP_FLAGS,
					 g_param_spec_flags ("startup-flags",
							     NULL,
							     NULL,
							     EOG_TYPE_STARTUP_FLAGS,
					 		     0,
					 		     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));

/**
 * EogWindow::prepared:
 * @window: the object which received the signal.
 *
 * The #EogWindow::prepared signal is emitted when the @window is ready
 * to be shown.
 */
	signals [SIGNAL_PREPARED] =
		g_signal_new ("prepared",
			      EOG_TYPE_WINDOW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EogWindowClass, prepared),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (g_object_class, sizeof (EogWindowPrivate));
}

/**
 * eog_window_new:
 * @flags: the initialization parameters for the new window.
 *
 *
 * Creates a new and empty #EogWindow. Use @flags to indicate
 * if the window should be initialized fullscreen, in slideshow mode,
 * and/or without the thumbnails collection visible. See #EogStartupFlags.
 *
 * Returns: a newly created #EogWindow.
 **/
GtkWidget*
eog_window_new (EogStartupFlags flags)
{
	EogWindow *window;

	eog_debug (DEBUG_WINDOW);

	window = EOG_WINDOW (g_object_new (EOG_TYPE_WINDOW,
					   "type", GTK_WINDOW_TOPLEVEL,
					   "startup-flags", flags,
					   NULL));

	return GTK_WIDGET (window);
}

static void
eog_window_list_store_image_added (GtkTreeModel *tree_model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
eog_window_list_store_image_removed (GtkTreeModel *tree_model,
                                     GtkTreePath  *path,
                                     gpointer      user_data)
{
	EogWindow *window = EOG_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
eog_job_model_cb (EogJobModel *job, gpointer data)
{
	EogWindow *window;
	EogWindowPrivate *priv;
	gint n_images;

	eog_debug (DEBUG_WINDOW);

#ifdef HAVE_EXIF
        int i;
	EogImage *image;
#endif

        g_return_if_fail (EOG_IS_WINDOW (data));

	window = EOG_WINDOW (data);
	priv = window->priv;

	if (priv->store != NULL) {
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	priv->store = g_object_ref (job->store);

	n_images = eog_list_store_length (EOG_LIST_STORE (priv->store));

#ifdef HAVE_EXIF
	if (mateconf_client_get_bool (priv->client, EOG_CONF_VIEW_AUTOROTATE, NULL)) {
		for (i = 0; i < n_images; i++) {
			image = eog_list_store_get_image_by_pos (priv->store, i);
			eog_image_autorotate (image);
			g_object_unref (image);
		}
	}
#endif

	eog_thumb_view_set_model (EOG_THUMB_VIEW (priv->thumbview), priv->store);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-inserted",
			  G_CALLBACK (eog_window_list_store_image_added),
			  window);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-deleted",
			  G_CALLBACK (eog_window_list_store_image_removed),
			  window);

	if (n_images == 0) {
		gint n_files;

		priv->status = EOG_WINDOW_STATUS_NORMAL;
		update_action_groups_state (window);

		n_files = g_slist_length (priv->file_list);

		if (n_files > 0) {
			GtkWidget *message_area;
			GFile *file = NULL;

			if (n_files == 1) {
				file = (GFile *) priv->file_list->data;
			}

			message_area = eog_no_images_error_message_area_new (file);

			eog_window_set_message_area (window, message_area);

			gtk_widget_show (message_area);
		}

		g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
	}
}

/**
 * eog_window_open_file_list:
 * @window: An #EogWindow.
 * @file_list: A %NULL-terminated list of #GFile's.
 *
 * Opens a list of files, adding them to the collection in @window.
 * Files will be checked to be readable and later filtered according
 * with eog_list_store_add_files().
 **/
void
eog_window_open_file_list (EogWindow *window, GSList *file_list)
{
	EogJob *job;

	eog_debug (DEBUG_WINDOW);

	window->priv->status = EOG_WINDOW_STATUS_INIT;

	g_slist_foreach (file_list, (GFunc) g_object_ref, NULL);
	window->priv->file_list = file_list;

	job = eog_job_model_new (file_list);

	g_signal_connect (job,
			  "finished",
			  G_CALLBACK (eog_job_model_cb),
			  window);

	eog_job_queue_add_job (job);
	g_object_unref (job);
}

/**
 * eog_window_get_ui_manager:
 * @window: An #EogWindow.
 *
 * Gets the #GtkUIManager that describes the UI of @window.
 *
 * Returns: A #GtkUIManager.
 **/
GtkUIManager *
eog_window_get_ui_manager (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->ui_mgr;
}

/**
 * eog_window_get_mode:
 * @window: An #EogWindow.
 *
 * Gets the mode of @window. See #EogWindowMode for details.
 *
 * Returns: An #EogWindowMode.
 **/
EogWindowMode
eog_window_get_mode (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), EOG_WINDOW_MODE_UNKNOWN);

	return window->priv->mode;
}

/**
 * eog_window_set_mode:
 * @window: an #EogWindow.
 * @mode: an #EogWindowMode value.
 *
 * Changes the mode of @window to normal, fullscreen, or slideshow.
 * See #EogWindowMode for details.
 **/
void
eog_window_set_mode (EogWindow *window, EogWindowMode mode)
{
        g_return_if_fail (EOG_IS_WINDOW (window));

	if (window->priv->mode == mode)
		return;

	switch (mode) {
	case EOG_WINDOW_MODE_NORMAL:
		eog_window_stop_fullscreen (window,
					    window->priv->mode == EOG_WINDOW_MODE_SLIDESHOW);
		break;
	case EOG_WINDOW_MODE_FULLSCREEN:
		eog_window_run_fullscreen (window, FALSE);
		break;
	case EOG_WINDOW_MODE_SLIDESHOW:
		eog_window_run_fullscreen (window, TRUE);
		break;
	case EOG_WINDOW_MODE_UNKNOWN:
		break;
	}
}

/**
 * eog_window_get_store:
 * @window: An #EogWindow.
 *
 * Gets the #EogListStore that contains the images in the collection
 * of @window.
 *
 * Returns: an #EogListStore.
 **/
EogListStore *
eog_window_get_store (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return EOG_LIST_STORE (window->priv->store);
}

/**
 * eog_window_get_view:
 * @window: An #EogWindow.
 *
 * Gets the #EogScrollView in the window.
 *
 * Returns: the #EogScrollView.
 **/
GtkWidget *
eog_window_get_view (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

       return window->priv->view;
}

/**
 * eog_window_get_sidebar:
 * @window: An #EogWindow.
 *
 * Gets the sidebar widget of @window.
 *
 * Returns: the #EogSidebar.
 **/
GtkWidget *
eog_window_get_sidebar (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->sidebar;
}

/**
 * eog_window_get_thumb_view:
 * @window: an #EogWindow.
 *
 * Gets the thumbnails view in @window.
 *
 * Returns: an #EogThumbView.
 **/
GtkWidget *
eog_window_get_thumb_view (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->thumbview;
}

/**
 * eog_window_get_thumb_nav:
 * @window: an #EogWindow.
 *
 * Gets the thumbnails navigation pane in @window.
 *
 * Returns: an #EogThumbNav.
 **/
GtkWidget *
eog_window_get_thumb_nav (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->nav;
}

/**
 * eog_window_get_statusbar:
 * @window: an #EogWindow.
 *
 * Gets the statusbar in @window.
 *
 * Returns: a #EogStatusBar.
 **/
GtkWidget *
eog_window_get_statusbar (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->statusbar;
}

/**
 * eog_window_get_image:
 * @window: an #EogWindow.
 *
 * Gets the image currently displayed in @window or %NULL if
 * no image is being displayed.
 *
 * Returns: an #EogImage.
 **/
EogImage *
eog_window_get_image (EogWindow *window)
{
        g_return_val_if_fail (EOG_IS_WINDOW (window), NULL);

	return window->priv->image;
}

/**
 * eog_window_is_empty:
 * @window: an #EogWindow.
 *
 * Tells whether @window is currently empty or not.
 *
 * Returns: %TRUE if @window has no images, %FALSE otherwise.
 **/
gboolean
eog_window_is_empty (EogWindow *window)
{
        EogWindowPrivate *priv;
        gboolean empty = TRUE;

	eog_debug (DEBUG_WINDOW);

        g_return_val_if_fail (EOG_IS_WINDOW (window), FALSE);

        priv = window->priv;

        if (priv->store != NULL) {
                empty = (eog_list_store_length (EOG_LIST_STORE (priv->store)) == 0);
        }

        return empty;
}

void
eog_window_reload_image (EogWindow *window)
{
	GtkWidget *view;

	g_return_if_fail (EOG_IS_WINDOW (window));

	if (window->priv->image == NULL)
		return;

	g_object_unref (window->priv->image);
	window->priv->image = NULL;

	view = eog_window_get_view (window);
	eog_scroll_view_set_image (EOG_SCROLL_VIEW (view), NULL);

	eog_thumb_view_select_single (EOG_THUMB_VIEW (window->priv->thumbview),
				      EOG_THUMB_VIEW_SELECT_CURRENT);
}
