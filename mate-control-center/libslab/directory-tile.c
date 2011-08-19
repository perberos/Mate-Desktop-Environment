/*
 * This file is part of libtile.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libtile is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libtile is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "directory-tile.h"
#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <unistd.h>

#include "slab-mate-util.h"
#include "mate-utils.h"
#include "libslab-utils.h"

#define MATECONF_SEND_TO_CMD_KEY       "/desktop/mate/applications/main-menu/file-area/file_send_to_cmd"
#define MATECONF_ENABLE_DELETE_KEY_DIR "/apps/caja/preferences"
#define MATECONF_ENABLE_DELETE_KEY     MATECONF_ENABLE_DELETE_KEY_DIR "/enable_delete"
#define MATECONF_CONFIRM_DELETE_KEY    MATECONF_ENABLE_DELETE_KEY_DIR "/confirm_trash"

G_DEFINE_TYPE (DirectoryTile, directory_tile, NAMEPLATE_TILE_TYPE)

static void directory_tile_finalize (GObject *);
static void directory_tile_style_set (GtkWidget *, GtkStyle *);

static void directory_tile_private_setup (DirectoryTile *);
static void load_image (DirectoryTile *);

static GtkWidget *create_header (const gchar *);

static void header_size_allocate_cb (GtkWidget *, GtkAllocation *, gpointer);

static void open_with_default_trigger (Tile *, TileEvent *, TileAction *);
static void rename_trigger (Tile *, TileEvent *, TileAction *);
static void move_to_trash_trigger (Tile *, TileEvent *, TileAction *);
static void delete_trigger (Tile *, TileEvent *, TileAction *);
static void send_to_trigger (Tile *, TileEvent *, TileAction *);

static void rename_entry_activate_cb (GtkEntry *, gpointer);
static gboolean rename_entry_key_release_cb (GtkWidget *, GdkEventKey *, gpointer);
static void mateconf_enable_delete_cb (MateConfClient *, guint, MateConfEntry *, gpointer);

static void disown_spawned_child (gpointer);

typedef struct
{
	gchar *basename;
	gchar *mime_type;
	gchar *icon_name;
	
	GtkBin *header_bin;
	GAppInfo *default_app;
	
	gboolean image_is_broken;
	
	gboolean delete_enabled;
	guint mateconf_conn_id;
} DirectoryTilePrivate;

#define DIRECTORY_TILE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DIRECTORY_TILE_TYPE, DirectoryTilePrivate))

static void directory_tile_class_init (DirectoryTileClass *this_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (this_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (this_class);

	g_obj_class->finalize = directory_tile_finalize;

	widget_class->style_set = directory_tile_style_set;

	g_type_class_add_private (this_class, sizeof (DirectoryTilePrivate));
}

GtkWidget *
directory_tile_new (const gchar *in_uri, const gchar *title, const gchar *icon_name, const gchar *mime_type)
{
	DirectoryTile *this;
	DirectoryTilePrivate *priv;

	gchar *uri;
	GtkWidget *image;
	GtkWidget *header;
	GtkMenu *context_menu;

	GtkContainer *menu_ctnr;
	GtkWidget *menu_item;

	TileAction *action;

	gchar *basename;

	gchar *markup;

	AtkObject *accessible;
	
	gchar *filename;
	gchar *tooltip_text;

  
	uri = g_strdup (in_uri);

	image = gtk_image_new ();

	if (! title) {
		markup = g_path_get_basename (uri);
		basename = g_uri_unescape_string (markup, NULL);
		g_free (markup);
	}
	else
		basename = g_strdup (title);

	header = create_header (basename);

	filename = g_filename_from_uri (uri, NULL, NULL);

  	if (filename)
		tooltip_text = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
	else
		tooltip_text = NULL;

	g_free (filename);
	
	context_menu = GTK_MENU (gtk_menu_new ());

	this = g_object_new (
		DIRECTORY_TILE_TYPE,
		"tile-uri",          uri,
		"nameplate-image",   image,
		"nameplate-header",  header,
		"context-menu",      context_menu,
		NULL);
	gtk_widget_set_tooltip_text (GTK_WIDGET (this), tooltip_text);

	g_free (uri);
	if (tooltip_text)
		g_free (tooltip_text);

	priv = DIRECTORY_TILE_GET_PRIVATE (this);
	priv->basename    = g_strdup (basename);
	priv->header_bin  = GTK_BIN (header);
	priv->icon_name   = g_strdup (icon_name);
	priv->mime_type   = g_strdup (mime_type);

	directory_tile_private_setup (this);

	TILE (this)->actions = g_new0 (TileAction *, 6);
	TILE (this)->n_actions = 6;

	menu_ctnr = GTK_CONTAINER (TILE (this)->context_menu);

	/* make open with default action */

	markup = g_markup_printf_escaped (_("<b>Open</b>"));
	action = tile_action_new (TILE (this), open_with_default_trigger, markup, TILE_ACTION_OPENS_NEW_WINDOW);
	g_free (markup);

	TILE (this)->default_action = action;

	menu_item = GTK_WIDGET (GTK_WIDGET (tile_action_get_menu_item (action)));

	TILE (this)->actions [DIRECTORY_TILE_ACTION_OPEN] = action;

	gtk_container_add (menu_ctnr, menu_item);

	/* insert separator */

	menu_item = gtk_separator_menu_item_new ();
	gtk_container_add (menu_ctnr, menu_item);

	/* make rename action */

	action = tile_action_new (TILE (this), rename_trigger, _("Rename..."), 0);
	TILE (this)->actions[DIRECTORY_TILE_ACTION_RENAME] = action;

	menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
	gtk_container_add (menu_ctnr, menu_item);

	/* make send to action */

	/* Only allow Send To for local files, ideally this would use something
	 * equivalent to mate_vfs_uri_is_local, but that method will stat the file and
	 * that can hang in some conditions. */

	if (!strncmp (TILE (this)->uri, "file://", 7))
	{
		action = tile_action_new (TILE (this), send_to_trigger, _("Send To..."),
			TILE_ACTION_OPENS_NEW_WINDOW);

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
	}
	else
	{
		action = NULL;

		menu_item = gtk_menu_item_new_with_label (_("Send To..."));
		gtk_widget_set_sensitive (menu_item, FALSE);
	}

	TILE (this)->actions[DIRECTORY_TILE_ACTION_SEND_TO] = action;

	gtk_container_add (menu_ctnr, menu_item);

	/* insert separator */

	menu_item = gtk_separator_menu_item_new ();
	gtk_container_add (menu_ctnr, menu_item);

	/* make move to trash action */

	action = tile_action_new (TILE (this), move_to_trash_trigger, _("Move to Trash"), 0);
	TILE (this)->actions[DIRECTORY_TILE_ACTION_MOVE_TO_TRASH] = action;

	menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
	gtk_container_add (menu_ctnr, menu_item);

	/* make delete action */

	if (priv->delete_enabled)
	{
		action = tile_action_new (TILE (this), delete_trigger, _("Delete"), 0);
		TILE (this)->actions[DIRECTORY_TILE_ACTION_DELETE] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_container_add (menu_ctnr, menu_item);
	}

	gtk_widget_show_all (GTK_WIDGET (TILE (this)->context_menu));

	load_image (this);

	accessible = gtk_widget_get_accessible (GTK_WIDGET (this));
	if (basename)
	  atk_object_set_name (accessible, basename);

	g_free (basename);

	return GTK_WIDGET (this);
}

static void
directory_tile_private_setup (DirectoryTile *tile)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);
	MateConfClient *client;

	if (priv->mime_type)
		priv->default_app = g_app_info_get_default_for_type (priv->mime_type, TRUE);
	else
		priv->default_app = NULL;

	priv->delete_enabled =
		(gboolean) GPOINTER_TO_INT (get_mateconf_value (MATECONF_ENABLE_DELETE_KEY));

	client = mateconf_client_get_default ();

	mateconf_client_add_dir (client, MATECONF_ENABLE_DELETE_KEY_DIR, MATECONF_CLIENT_PRELOAD_NONE, NULL);
	priv->mateconf_conn_id =
		connect_mateconf_notify (MATECONF_ENABLE_DELETE_KEY, mateconf_enable_delete_cb, tile);

	g_object_unref (client);
}

static void
directory_tile_init (DirectoryTile *tile)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);

	priv->default_app = NULL;
	priv->basename = NULL;
	priv->header_bin = NULL;
	priv->icon_name = NULL;
	priv->mime_type = NULL;
	priv->image_is_broken = TRUE;
	priv->delete_enabled = FALSE;
	priv->mateconf_conn_id = 0;
}

static void
directory_tile_finalize (GObject *g_object)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (g_object);

	MateConfClient *client;

	g_free (priv->basename);
	g_free (priv->icon_name);
	g_free (priv->mime_type);

	if (priv->default_app)
		g_object_unref (priv->default_app);
    
	client = mateconf_client_get_default ();

	mateconf_client_notify_remove (client, priv->mateconf_conn_id);
	mateconf_client_remove_dir (client, MATECONF_ENABLE_DELETE_KEY_DIR, NULL);

	g_object_unref (client);

	(* G_OBJECT_CLASS (directory_tile_parent_class)->finalize) (g_object);
}

static void
directory_tile_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
	load_image (DIRECTORY_TILE (widget));
}

static void
load_image (DirectoryTile *tile)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);
	gchar *icon_name;

	if (priv->icon_name)
		icon_name = priv->icon_name;
	else
		icon_name = "folder";

	priv->image_is_broken = slab_load_image (
		GTK_IMAGE (NAMEPLATE_TILE (tile)->image), GTK_ICON_SIZE_DND, icon_name);
}

static GtkWidget *
create_header (const gchar *name)
{
	GtkWidget *header_bin;
	GtkWidget *header;

	header = gtk_label_new (name);
	gtk_label_set_ellipsize (GTK_LABEL (header), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (header), 0.0, 0.5);

	header_bin = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (header_bin), header);

	g_signal_connect (G_OBJECT (header), "size-allocate", G_CALLBACK (header_size_allocate_cb),
		NULL);

	return header_bin;
}

static void
header_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer user_data)
{
	gtk_widget_set_size_request (widget, alloc->width, -1);
}

static void
rename_entry_activate_cb (GtkEntry *entry, gpointer user_data)
{
	DirectoryTile *tile = DIRECTORY_TILE (user_data);
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);

	GFile *src_file;
	GFile *dst_file;

	gchar *dirname;
	gchar *dst_uri;
	gchar *src_path;

	GtkWidget *child;
	GtkWidget *header;

	gboolean res;
	GError *error = NULL;

	if (strlen (gtk_entry_get_text (entry)) < 1)
		return;

	src_file = g_file_new_for_uri (TILE (tile)->uri);

	src_path = g_filename_from_uri (TILE (tile)->uri, NULL, NULL);
	dirname = g_path_get_dirname (src_path);
	dst_uri = g_build_filename (dirname, gtk_entry_get_text (entry), NULL);
	dst_file = g_file_new_for_uri (dst_uri);

	g_free (dirname);
	g_free (src_path);

	res = g_file_move (src_file, dst_file,
			   G_FILE_COPY_NONE, NULL, NULL, NULL, &error);

	if (res) {
		g_free (priv->basename);
		priv->basename = g_strdup (gtk_entry_get_text (entry));
	}
	else {
		g_warning ("unable to move [%s] to [%s]: %s\n", TILE (tile)->uri, dst_uri,
			   error->message);
		g_error_free (error);
	}

	g_free (dst_uri);
	g_object_unref (src_file);
	g_object_unref (dst_file);

	header = gtk_label_new (priv->basename);
	gtk_misc_set_alignment (GTK_MISC (header), 0.0, 0.5);

	child = gtk_bin_get_child (priv->header_bin);

	if (child)
		gtk_widget_destroy (child);

	gtk_container_add (GTK_CONTAINER (priv->header_bin), header);

	gtk_widget_show (header);
}

static gboolean
rename_entry_key_release_cb (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	return TRUE;
}

static void
mateconf_enable_delete_cb (MateConfClient *client, guint conn_id, MateConfEntry *entry, gpointer user_data)
{
	Tile *tile = TILE (user_data);
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (user_data);

	GtkMenuShell *menu;
	gboolean delete_enabled;

	TileAction *action;
	GtkWidget *menu_item;

	menu = GTK_MENU_SHELL (tile->context_menu);

	delete_enabled = mateconf_value_get_bool (entry->value);

	if (delete_enabled == priv->delete_enabled)
		return;

	priv->delete_enabled = delete_enabled;

	if (priv->delete_enabled)
	{
		action = tile_action_new (tile, delete_trigger, _("Delete"), 0);
		tile->actions[DIRECTORY_TILE_ACTION_DELETE] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_menu_shell_insert (menu, menu_item, 7);

		gtk_widget_show_all (menu_item);
	}
	else
	{
		g_object_unref (tile->actions[DIRECTORY_TILE_ACTION_DELETE]);

		tile->actions[DIRECTORY_TILE_ACTION_DELETE] = NULL;
	}
}

static void
rename_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);

	GtkWidget *child;
	GtkWidget *entry;


	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), priv->basename);
	gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);

	child = gtk_bin_get_child (priv->header_bin);

	if (child)
		gtk_widget_destroy (child);

	gtk_container_add (GTK_CONTAINER (priv->header_bin), entry);

	g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (rename_entry_activate_cb), tile);

	g_signal_connect (G_OBJECT (entry), "key_release_event",
		G_CALLBACK (rename_entry_key_release_cb), NULL);

	gtk_widget_show (entry);
	gtk_widget_grab_focus (entry);
}

static void
move_to_trash_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	GFile *src_file;
	gboolean res;
	GError *error = NULL;

	src_file = g_file_new_for_uri (TILE (tile)->uri);

	res = g_file_trash (src_file, NULL, &error);
	if (!res) {
		g_warning ("unable to move [%s] to the trash: %s\n", TILE (tile)->uri,
			   error->message);
		g_error_free (error);
	}

	g_object_unref (src_file);
}

static void
delete_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	GtkDialog *confirm_dialog;
	gint       result;

	GFile *src_file;
	gboolean res;
	GError *error = NULL;

	if (GPOINTER_TO_INT (libslab_get_mateconf_value (MATECONF_CONFIRM_DELETE_KEY))) {
		confirm_dialog = GTK_DIALOG(gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, 
				GTK_BUTTONS_NONE, _("Are you sure you want to permanently delete \"%s\"?"), DIRECTORY_TILE_GET_PRIVATE (tile)->basename));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(confirm_dialog), _("If you delete an item, it is permanently lost."));
							
		gtk_dialog_add_button (confirm_dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (confirm_dialog, GTK_STOCK_DELETE, GTK_RESPONSE_YES);
		gtk_dialog_set_default_response (GTK_DIALOG (confirm_dialog), GTK_RESPONSE_YES);

		result = gtk_dialog_run (confirm_dialog);

		gtk_widget_destroy (GTK_WIDGET (confirm_dialog));

		if (result != GTK_RESPONSE_YES)
			return;
	}

	src_file = g_file_new_for_uri (TILE (tile)->uri);

	res = g_file_delete (src_file, NULL, &error);

	if (!res) {
		g_warning ("unable to delete [%s]: %s\n", TILE (tile)->uri,
			   error->message);
		g_error_free (error);
	}

	g_object_unref (src_file);
}

static void
send_to_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	gchar  *cmd;
	gint    argc;
	gchar **argv_parsed = NULL;
	gchar **argv        = NULL;

	gchar *path;
	gchar *dirname;
	gchar *basename;

	GError *error = NULL;

	gint i;


	cmd = (gchar *) get_mateconf_value (MATECONF_SEND_TO_CMD_KEY);

	if (! g_shell_parse_argv (cmd, & argc, & argv_parsed, NULL))
		goto exit;

	argv = g_new0 (gchar *, argc + 1);

	path     = g_filename_from_uri (tile->uri, NULL, NULL);
	dirname  = g_path_get_dirname  (path);
	basename = g_path_get_basename (path);

	for (i = 0; i < argc; ++i) {
		if (strstr (argv_parsed [i], "DIRNAME"))
			argv [i] = string_replace_once (argv_parsed [i], "DIRNAME", dirname);
		else if (strstr (argv_parsed [i], "BASENAME"))
			argv [i] = string_replace_once (argv_parsed [i], "BASENAME", basename);
		else
			argv [i] = g_strdup (argv_parsed [i]);
	}

	argv [argc] = NULL;

	g_free (path);
	g_free (dirname);
	g_free (basename);

	g_spawn_async (
		NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
		disown_spawned_child, NULL, NULL, & error);

	if (error) {
		cmd = g_strjoinv (" ", argv);
		libslab_handle_g_error (
			& error, "%s: can't execute search [%s]\n", G_STRFUNC, cmd);
		g_free (cmd);
	}

	g_strfreev (argv);

exit:

	g_free (cmd);
	g_strfreev (argv_parsed);
}

static void
disown_spawned_child (gpointer user_data)
{
	setsid  ();
	setpgid (0, 0);
}

static void
open_with_default_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DirectoryTilePrivate *priv = DIRECTORY_TILE_GET_PRIVATE (tile);
	GList *uris = NULL;
	gboolean res;
	GdkAppLaunchContext *launch_context;
	GError *error = NULL;

	if (priv->default_app)
	{
		uris = g_list_append (uris, TILE (tile)->uri);
		
		launch_context = gdk_app_launch_context_new ();
		gdk_app_launch_context_set_screen (launch_context,
						   gtk_widget_get_screen (GTK_WIDGET (tile)));
		gdk_app_launch_context_set_timestamp (launch_context,
						      event->time);

		res = g_app_info_launch_uris (priv->default_app, uris,
					      G_APP_LAUNCH_CONTEXT (launch_context),
					      &error);

		if (!res) {
			g_warning
				("error: could not launch application with [%s]: %s\n",
				 TILE (tile)->uri, error->message);
			g_error_free (error);
		}

		g_list_free (uris);
		g_object_unref (launch_context);
	} else {
		gchar *cmd;
		cmd = string_replace_once (
			get_slab_mateconf_string (SLAB_FILE_MANAGER_OPEN_CMD), "FILE_URI", tile->uri);
		spawn_process (cmd);
		g_free (cmd);
	}
}
