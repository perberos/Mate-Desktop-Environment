/*
 * This file is part of libtile.
 *
 * Copyright (c) 2006, 2007 Novell, Inc.
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

#include "document-tile.h"
#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>
#include <gio/gio.h>

#include "slab-mate-util.h"
#include "mate-utils.h"
#include "libslab-utils.h"
#include "bookmark-agent.h"

#define MATECONF_SEND_TO_CMD_KEY       "/desktop/mate/applications/main-menu/file-area/file_send_to_cmd"
#define MATECONF_ENABLE_DELETE_KEY_DIR "/apps/caja/preferences"
#define MATECONF_ENABLE_DELETE_KEY     MATECONF_ENABLE_DELETE_KEY_DIR "/enable_delete"
#define MATECONF_CONFIRM_DELETE_KEY    MATECONF_ENABLE_DELETE_KEY_DIR "/confirm_trash"

G_DEFINE_TYPE (DocumentTile, document_tile, NAMEPLATE_TILE_TYPE)

static void document_tile_finalize (GObject *);
static void document_tile_style_set (GtkWidget *, GtkStyle *);

static void document_tile_private_setup (DocumentTile *);
static void load_image (DocumentTile *);

static GtkWidget *create_header (const gchar *);

static char      *create_subheader_string (time_t date);
static GtkWidget *create_subheader (const gchar *);
static void update_user_list_menu_item (DocumentTile *);

static void header_size_allocate_cb (GtkWidget *, GtkAllocation *, gpointer);

static void open_with_default_trigger    (Tile *, TileEvent *, TileAction *);
static void open_in_file_manager_trigger (Tile *, TileEvent *, TileAction *);
static void rename_trigger               (Tile *, TileEvent *, TileAction *);
static void move_to_trash_trigger        (Tile *, TileEvent *, TileAction *);
static void remove_recent_item           (Tile *, TileEvent *, TileAction *);
static void purge_recent_items           (Tile *, TileEvent *, TileAction *);
static void delete_trigger               (Tile *, TileEvent *, TileAction *);
static void user_docs_trigger            (Tile *, TileEvent *, TileAction *);
static void send_to_trigger              (Tile *, TileEvent *, TileAction *);

static void rename_entry_activate_cb (GtkEntry *, gpointer);
static gboolean rename_entry_key_release_cb (GtkWidget *, GdkEventKey *, gpointer);

static void mateconf_enable_delete_cb (MateConfClient *, guint, MateConfEntry *, gpointer);

static void agent_notify_cb (GObject *, GParamSpec *, gpointer);

typedef struct
{
	gchar *basename;
	gchar *mime_type;
	time_t modified;

	GAppInfo *default_app;

	GtkBin *header_bin;

	gboolean image_is_broken;
	gchar * force_icon_name;  //show an icon instead of a thumbnail

	gboolean delete_enabled;
	guint mateconf_conn_id;

	BookmarkAgent       *agent;
	BookmarkStoreStatus  store_status;
	gboolean             is_bookmarked;
	gulong               notify_signal_id;
} DocumentTilePrivate;

#define DOCUMENT_TILE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DOCUMENT_TILE_TYPE, DocumentTilePrivate))

static void document_tile_class_init (DocumentTileClass *this_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (this_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (this_class);

	g_obj_class->finalize = document_tile_finalize;

	widget_class->style_set = document_tile_style_set;

	g_type_class_add_private (this_class, sizeof (DocumentTilePrivate));
}

//Use a specific icon instead of a thumbnail.
GtkWidget *
document_tile_new_force_icon (const gchar *in_uri, const gchar *mime_type, time_t modified, const gchar *icon)
{
	DocumentTile *this;
	DocumentTilePrivate *priv;
	
	this = (DocumentTile *) document_tile_new (BOOKMARK_STORE_USER_DOCS, in_uri, mime_type, modified);
	priv = DOCUMENT_TILE_GET_PRIVATE (this);
	priv->force_icon_name = g_strdup (icon);
	return GTK_WIDGET (this);
}

GtkWidget *
document_tile_new (BookmarkStoreType bookmark_store_type, const gchar *in_uri, const gchar *mime_type, time_t modified)
{
	DocumentTile *this;
	DocumentTilePrivate *priv;

	gchar *uri;
	GtkWidget *image;
	GtkWidget *header;
	GtkWidget *subheader;
	GtkMenu *context_menu;

	GtkContainer *menu_ctnr;
	GtkWidget *menu_item;

	TileAction *action;

	gchar *basename;

	gchar *time_str;

	gchar *markup;
	gchar *str;

	AtkObject *accessible;

	GFile * file;
	gchar *tooltip_text;

	libslab_checkpoint ("document_tile_new(): start");
  
	uri = g_strdup (in_uri);

	image = gtk_image_new ();

	markup = g_path_get_basename (uri);
	basename = g_uri_unescape_string (markup, NULL);
	g_free (markup);

	header = create_header (basename);

	time_str = create_subheader_string (modified);
	subheader = create_subheader (time_str);
	
	file = g_file_new_for_uri (uri);
	tooltip_text = g_file_get_parse_name (file);
	g_object_unref (file);
	
	context_menu = GTK_MENU (gtk_menu_new ());

	this = g_object_new (DOCUMENT_TILE_TYPE, "tile-uri", uri, "nameplate-image", image,
		"nameplate-header", header, "nameplate-subheader", subheader, 
		"context-menu", context_menu, NULL);
	gtk_widget_set_tooltip_text (GTK_WIDGET (this), tooltip_text);

	g_free (uri);
	if (tooltip_text)
		g_free (tooltip_text);

	priv = DOCUMENT_TILE_GET_PRIVATE (this);
	priv->basename    = g_strdup (basename);
	priv->mime_type   = g_strdup (mime_type);
	priv->modified    = modified;
	priv->header_bin  = GTK_BIN (header);
	priv->agent = bookmark_agent_get_instance (bookmark_store_type);

	document_tile_private_setup (this);
	TILE (this)->actions = g_new0 (TileAction *, DOCUMENT_TILE_ACTION_NUM_OF_ACTIONS);
	TILE (this)->n_actions = DOCUMENT_TILE_ACTION_NUM_OF_ACTIONS;

	menu_ctnr = GTK_CONTAINER (TILE (this)->context_menu);

	/* make open with default action */

	if (priv->default_app) {
		str = g_strdup_printf (_("Open with \"%s\""),
				       g_app_info_get_name (priv->default_app));
		markup = g_markup_printf_escaped ("<b>%s</b>", str);
		action = tile_action_new (TILE (this), open_with_default_trigger, markup,
			TILE_ACTION_OPENS_NEW_WINDOW);
		g_free (markup);
		g_free (str);

		TILE (this)->default_action = action;

		menu_item = GTK_WIDGET (GTK_WIDGET (tile_action_get_menu_item (action)));
	}
	else {
		action = NULL;
		menu_item = gtk_menu_item_new_with_label (_("Open with Default Application"));
		gtk_widget_set_sensitive (menu_item, FALSE);
	}

	TILE (this)->actions[DOCUMENT_TILE_ACTION_OPEN_WITH_DEFAULT] = action;

	gtk_container_add (menu_ctnr, menu_item);

	/* make open in caja action */

	action = tile_action_new (TILE (this), open_in_file_manager_trigger,
		_("Open in File Manager"), TILE_ACTION_OPENS_NEW_WINDOW);
	TILE (this)->actions[DOCUMENT_TILE_ACTION_OPEN_IN_FILE_MANAGER] = action;

	if (!TILE (this)->default_action)
		TILE (this)->default_action = action;

	menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
	gtk_container_add (menu_ctnr, menu_item);

	/* insert separator */

	menu_item = gtk_separator_menu_item_new ();
	gtk_container_add (menu_ctnr, menu_item);

	/* make rename action */

	action = tile_action_new (TILE (this), rename_trigger, _("Rename..."), 0);
	TILE (this)->actions[DOCUMENT_TILE_ACTION_RENAME] = action;

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

	TILE (this)->actions[DOCUMENT_TILE_ACTION_SEND_TO] = action;

	gtk_container_add (menu_ctnr, menu_item);

	/* make "add/remove to favorites" action */

	action = tile_action_new (TILE (this), user_docs_trigger, NULL, 0);
	TILE (this)->actions [DOCUMENT_TILE_ACTION_UPDATE_MAIN_MENU] = action;

	update_user_list_menu_item (this);

	menu_item = GTK_WIDGET (tile_action_get_menu_item (action));

	gtk_container_add (menu_ctnr, menu_item);

	/* insert separator */

	menu_item = gtk_separator_menu_item_new ();
	gtk_container_add (menu_ctnr, menu_item);

	/* make move to trash action */

	action = tile_action_new (TILE (this), move_to_trash_trigger, _("Move to Trash"), 0);
	TILE (this)->actions[DOCUMENT_TILE_ACTION_MOVE_TO_TRASH] = action;

	menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
	gtk_container_add (menu_ctnr, menu_item);

	/* make delete action */

	if (priv->delete_enabled)
	{
		action = tile_action_new (TILE (this), delete_trigger, _("Delete"), 0);
		TILE (this)->actions[DOCUMENT_TILE_ACTION_DELETE] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_container_add (menu_ctnr, menu_item);
	}

	if (!priv->is_bookmarked) {
		/* clean item from menu */
		action = tile_action_new (TILE (this), remove_recent_item, _("Remove from recent menu"), 0);
		TILE (this)->actions[DOCUMENT_TILE_ACTION_CLEAN_ITEM] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_container_add (menu_ctnr, menu_item);

		/* clean all the items from menu */

		action = tile_action_new (TILE (this), purge_recent_items, _("Purge all the recent items"), 0);
		TILE (this)->actions[DOCUMENT_TILE_ACTION_CLEAN_ALL] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_container_add (menu_ctnr, menu_item);
	}

	gtk_widget_show_all (GTK_WIDGET (TILE (this)->context_menu));

	accessible = gtk_widget_get_accessible (GTK_WIDGET (this));
	if (basename)
	  atk_object_set_name (accessible, basename);
	if (time_str)
	  atk_object_set_description (accessible, time_str);

	g_free (basename);
	g_free (time_str);

	libslab_checkpoint ("document_tile_new(): end");

	return GTK_WIDGET (this);
}

static void
document_tile_private_setup (DocumentTile *this)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (this);

	GFile *file;
	GAppInfo *app;

	MateConfClient *client;
	GError *error = NULL;

	file = g_file_new_for_uri (TILE (this)->uri);
	app = g_file_query_default_handler (file, NULL, &error);
	priv->default_app = app;

	if (error)
		g_error_free (error);
	g_object_unref (file);

	priv->delete_enabled =
		(gboolean) GPOINTER_TO_INT (get_mateconf_value (MATECONF_ENABLE_DELETE_KEY));

	client = mateconf_client_get_default ();

	mateconf_client_add_dir (client, MATECONF_ENABLE_DELETE_KEY_DIR, MATECONF_CLIENT_PRELOAD_NONE, NULL);
	priv->mateconf_conn_id =
		connect_mateconf_notify (MATECONF_ENABLE_DELETE_KEY, mateconf_enable_delete_cb, this);

	g_object_unref (client);

	priv->notify_signal_id = g_signal_connect (
		G_OBJECT (priv->agent), "notify", G_CALLBACK (agent_notify_cb), this);
}

static void
document_tile_init (DocumentTile *tile)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

	priv->basename         = NULL;
	priv->mime_type        = NULL;
	priv->modified         = 0;

	priv->default_app      = NULL;

	priv->header_bin       = NULL;

	priv->image_is_broken  = TRUE;
	priv->force_icon_name  = NULL;

	priv->delete_enabled   = FALSE;
	priv->mateconf_conn_id    = 0;

	priv->agent            = NULL;
	priv->store_status     = BOOKMARK_STORE_DEFAULT;
	priv->is_bookmarked    = FALSE;
	priv->notify_signal_id = 0;
}

static void
document_tile_finalize (GObject *g_object)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (g_object);

	MateConfClient *client;

	g_free (priv->basename);
	g_free (priv->mime_type);
	g_free (priv->force_icon_name);

	if (priv->default_app)
		g_object_unref (priv->default_app);

	if (priv->notify_signal_id)
		g_signal_handler_disconnect (priv->agent, priv->notify_signal_id);

	g_object_unref (G_OBJECT (priv->agent));

	client = mateconf_client_get_default ();

	mateconf_client_notify_remove (client, priv->mateconf_conn_id);
	mateconf_client_remove_dir (client, MATECONF_ENABLE_DELETE_KEY_DIR, NULL);

	g_object_unref (client);

	G_OBJECT_CLASS (document_tile_parent_class)->finalize (g_object);
}

static void
document_tile_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
	load_image (DOCUMENT_TILE (widget));
}

static void
load_image (DocumentTile *tile)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

	gchar *icon_id = NULL;
	gboolean free_icon_id = TRUE;
	MateDesktopThumbnailFactory *thumbnail_factory;
	GIcon *icon;

	libslab_checkpoint ("document-tile.c: load_image(): start for %s", TILE (tile)->uri);

	if (priv->force_icon_name || ! priv->mime_type) {
		if (priv->force_icon_name)
			icon_id = priv->force_icon_name;
		else
			icon_id = "text-x-preview";
		free_icon_id = FALSE;

		goto exit;
	}

	thumbnail_factory = libslab_thumbnail_factory_get ();

	icon_id = mate_desktop_thumbnail_factory_lookup (thumbnail_factory, TILE (tile)->uri, priv->modified);

	if (! icon_id) {
		icon = g_content_type_get_icon (priv->mime_type);
		g_object_get (icon, "name", &icon_id, NULL);

		g_object_unref (icon);
	}	

exit:

	priv->image_is_broken = slab_load_image (
		GTK_IMAGE (NAMEPLATE_TILE (tile)->image), GTK_ICON_SIZE_DND, icon_id);

	if (free_icon_id && icon_id)
		g_free (icon_id);

	libslab_checkpoint ("document-tile.c: load_image(): end");
}

/* Next function taken from e-data-server-util.c in evolution-data-server */
/** 
 * e_strftime:
 * @s: The string array to store the result in.
 * @max: The size of array @s.
 * @fmt: The formatting to use on @tm.
 * @tm: The time value to format.
 *
 * This function is a wrapper around the strftime(3) function, which
 * converts the &percnt;l and &percnt;k (12h and 24h) format variables if necessary.
 *
 * Returns: The number of characters placed in @s.
 **/
static size_t
e_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
#ifdef HAVE_LKSTRFTIME
	return strftime(s, max, fmt, tm);
#else
	char *c, *ffmt, *ff;
	size_t ret;

	ffmt = g_strdup(fmt);
	ff = ffmt;
	while ((c = strstr(ff, "%l")) != NULL) {
		c[1] = 'I';
		ff = c;
	}

	ff = ffmt;
	while ((c = strstr(ff, "%k")) != NULL) {
		c[1] = 'H';
		ff = c;
	}

#ifdef G_OS_WIN32
	/* The Microsoft strftime() doesn't have %e either */
	ff = ffmt;
	while ((c = strstr(ff, "%e")) != NULL) {
		c[1] = 'd';
		ff = c;
	}
#endif

	ret = strftime(s, max, ffmt, tm);
	g_free(ffmt);
	return ret;
#endif
}

/* Next two functions taken from e-util.c in evolution */
/**
 * Function to do a last minute fixup of the AM/PM stuff if the locale
 * and gettext haven't done it right. Most English speaking countries
 * except the USA use the 24 hour clock (UK, Australia etc). However
 * since they are English nobody bothers to write a language
 * translation (gettext) file. So the locale turns off the AM/PM, but
 * gettext does not turn on the 24 hour clock. Leaving a mess.
 *
 * This routine checks if AM/PM are defined in the locale, if not it
 * forces the use of the 24 hour clock.
 *
 * The function itself is a front end on strftime and takes exactly
 * the same arguments.
 *
 * TODO: Actually remove the '%p' from the fixed up string so that
 * there isn't a stray space.
 **/

static size_t
e_strftime_fix_am_pm(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	char buf[10];
	char *sp;
	char *ffmt;
	size_t ret;

	if (strstr(fmt, "%p")==NULL && strstr(fmt, "%P")==NULL) {
		/* No AM/PM involved - can use the fmt string directly */
		ret=e_strftime(s, max, fmt, tm);
	} else {
		/* Get the AM/PM symbol from the locale */
		e_strftime (buf, 10, "%p", tm);

		if (buf[0]) {
			/**
			 * AM/PM have been defined in the locale
			 * so we can use the fmt string directly
			 **/
			ret=e_strftime(s, max, fmt, tm);
		} else {
			/**
			 * No AM/PM defined by locale
			 * must change to 24 hour clock
			 **/
			ffmt=g_strdup(fmt);
			for (sp=ffmt; (sp=strstr(sp, "%l")); sp++) {
				/**
				 * Maybe this should be 'k', but I have never
				 * seen a 24 clock actually use that format
				 **/
				sp[1]='H';
			}
			for (sp=ffmt; (sp=strstr(sp, "%I")); sp++) {
				sp[1]='H';
			}
			ret=e_strftime(s, max, ffmt, tm);
			g_free(ffmt);
		}
	}

	return(ret);
}

static size_t 
e_utf8_strftime_fix_am_pm(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	size_t sz, ret;
	char *locale_fmt, *buf;

	locale_fmt = g_locale_from_utf8(fmt, -1, NULL, &sz, NULL);
	if (!locale_fmt)
		return 0;

	ret = e_strftime_fix_am_pm(s, max, locale_fmt, tm);
	if (!ret) {
		g_free (locale_fmt);
		return 0;
	}

	buf = g_locale_to_utf8(s, ret, NULL, &sz, NULL);
	if (!buf) {
		g_free (locale_fmt);
		return 0;
	}

	if (sz >= max) {
		char *tmp = buf + max - 1;
		tmp = g_utf8_find_prev_char(buf, tmp);
		if (tmp)
			sz = tmp - buf;
		else
			sz = 0;
	}
	memcpy(s, buf, sz);
	s[sz] = '\0';
	g_free(locale_fmt);
	g_free(buf);
	return sz;
}

static char *
create_subheader_string (time_t date)
{
	time_t nowdate = time(NULL);
	time_t yesdate;
	struct tm then, now, yesterday;
	char buf[100];
	gboolean done = FALSE;

	if (date == 0) {
		return g_strdup (_("?"));
	}

	localtime_r (&date, &then);
	localtime_r (&nowdate, &now);

	if (nowdate - date < 60 * 60 * 8 && nowdate > date) {
		e_utf8_strftime_fix_am_pm (buf, 100, _("%l:%M %p"), &then);
		done = TRUE;
	}

	if (!done) {
		if (then.tm_mday == now.tm_mday &&
		    then.tm_mon == now.tm_mon &&
		    then.tm_year == now.tm_year) {
			e_utf8_strftime_fix_am_pm (buf, 100, _("Today %l:%M %p"), &then);
			done = TRUE;
		}
	}
	if (!done) {
		yesdate = nowdate - 60 * 60 * 24;
		localtime_r (&yesdate, &yesterday);
		if (then.tm_mday == yesterday.tm_mday &&
		    then.tm_mon == yesterday.tm_mon &&
		    then.tm_year == yesterday.tm_year) {
			e_utf8_strftime_fix_am_pm (buf, 100, _("Yesterday %l:%M %p"), &then);
			done = TRUE;
		}
	}
	if (!done) {
		int i;
		for (i = 2; i < 7; i++) {
			yesdate = nowdate - 60 * 60 * 24 * i;
			localtime_r (&yesdate, &yesterday);
			if (then.tm_mday == yesterday.tm_mday &&
			    then.tm_mon == yesterday.tm_mon &&
			    then.tm_year == yesterday.tm_year) {
				e_utf8_strftime_fix_am_pm (buf, 100, _("%a %l:%M %p"), &then);
				done = TRUE;
				break;
			}
		}
	}
	if (!done) {
		if (then.tm_year == now.tm_year) {
			e_utf8_strftime_fix_am_pm (buf, 100, _("%b %d %l:%M %p"), &then);
		} else {
			e_utf8_strftime_fix_am_pm (buf, 100, _("%b %d %Y"), &then);
		}
	}

	return g_strdup (g_strstrip (buf));
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

static GtkWidget *
create_subheader (const gchar *desc)
{
	GtkWidget *subheader;

	subheader = gtk_label_new (desc);
	gtk_label_set_ellipsize (GTK_LABEL (subheader), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (subheader), 0.0, 0.5);
	gtk_widget_modify_fg (subheader, GTK_STATE_NORMAL,
		&subheader->style->fg[GTK_STATE_INSENSITIVE]);

	return subheader;
}

static void
update_user_list_menu_item (DocumentTile *this)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (this);

	TileAction  *action;
	GtkMenuItem *item;


	action = TILE (this)->actions [DOCUMENT_TILE_ACTION_UPDATE_MAIN_MENU];

	if (! action)
		return;

	priv->is_bookmarked = bookmark_agent_has_item (bookmark_agent_get_instance (BOOKMARK_STORE_USER_DOCS), TILE (this)->uri);

	if (priv->is_bookmarked)
		tile_action_set_menu_item_label (action, _("Remove from Favorites"));
	else
		tile_action_set_menu_item_label (action, _("Add to Favorites"));

	item = tile_action_get_menu_item (action);

	if (! GTK_IS_MENU_ITEM (item))
		return;

	g_object_get (G_OBJECT (priv->agent), BOOKMARK_AGENT_STORE_STATUS_PROP, & priv->store_status, NULL);

	gtk_widget_set_sensitive (GTK_WIDGET (item), (priv->store_status != BOOKMARK_STORE_DEFAULT_ONLY));
}

static void
header_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer user_data)
{
	gtk_widget_set_size_request (widget, alloc->width, -1);
}

static void
rename_entry_activate_cb (GtkEntry *entry, gpointer user_data)
{
	DocumentTile *tile = DOCUMENT_TILE (user_data);
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

	GFile *src_file;
	GFile *dst_file;

	char *src_path;
	char *dirname;
	char *dst_path;

	gboolean res;
	GError *error = NULL;

	GtkWidget *child;
	GtkWidget *header;

	if (strlen (gtk_entry_get_text (entry)) < 1)
		return;

	src_file = g_file_new_for_uri (TILE (tile)->uri);

	src_path = g_filename_from_uri (TILE (tile)->uri, NULL, NULL);
	dirname = g_path_get_dirname (src_path);
	dst_path = g_build_filename (dirname, gtk_entry_get_text (entry), NULL);
	dst_file = g_file_new_for_path (dst_path);

	res = g_file_move (src_file, dst_file, 0, NULL, NULL, NULL, &error);
	
	if (res) {
		char *dst_uri;

		dst_uri = g_file_get_uri (dst_file);
		bookmark_agent_move_item (priv->agent, TILE (tile)->uri, dst_uri);
		g_free (dst_uri);

		g_free (priv->basename);
		priv->basename = g_strdup (gtk_entry_get_text (entry));
	}
	else {
		g_warning ("unable to move [%s] to [%s]: %s\n", TILE (tile)->uri,
			   dst_path, error->message);
		g_error_free (error);
	}

	header = gtk_label_new (priv->basename);
	gtk_misc_set_alignment (GTK_MISC (header), 0.0, 0.5);

	child = gtk_bin_get_child (priv->header_bin);

	if (child)
		gtk_widget_destroy (child);

	gtk_container_add (GTK_CONTAINER (priv->header_bin), header);

	gtk_widget_show (header);

	g_object_unref (src_file);
	g_object_unref (dst_file);

	g_free (dirname);
	g_free (dst_path);
	g_free (src_path);
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
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (user_data);

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
		tile->actions[DOCUMENT_TILE_ACTION_DELETE] = action;

		menu_item = GTK_WIDGET (tile_action_get_menu_item (action));
		gtk_menu_shell_insert (menu, menu_item, 7);

		gtk_widget_show_all (menu_item);
	}
	else
	{
		g_object_unref (tile->actions[DOCUMENT_TILE_ACTION_DELETE]);

		tile->actions[DOCUMENT_TILE_ACTION_DELETE] = NULL;
	}
}

static void
open_with_default_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

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
					      G_APP_LAUNCH_CONTEXT (launch_context), &error);

		if (!res) {
			g_warning
				("error: could not launch application with [%s]: %s\n",
				TILE (tile)->uri, error->message);
			g_error_free (error);
		}

		g_list_free (uris);
		g_object_unref (launch_context);
	}
}

static void
open_in_file_manager_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	GFile *filename;
	GFile *dirname;
	gchar *uri;

	gchar *cmd;

	filename = g_file_new_for_uri (TILE (tile)->uri);
	dirname = g_file_get_parent (filename);
	uri = g_file_get_uri (dirname);
	
	if (!uri)
		g_warning ("error getting dirname for [%s]\n", TILE (tile)->uri);
	else
	{
		cmd = string_replace_once (get_slab_mateconf_string (SLAB_FILE_MANAGER_OPEN_CMD),
			"FILE_URI", uri);
		spawn_process (cmd);

		g_free (cmd);
	}

	g_object_unref (filename);
	g_object_unref (dirname);
	g_free (uri);
}

static void
rename_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

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
remove_recent_item (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);
	bookmark_agent_remove_item (priv->agent, TILE (tile)->uri);
}

static void
purge_recent_items (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);
	bookmark_agent_purge_items (priv->agent);
}

static void
move_to_trash_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

	GFile *src_file;
	gboolean res;
	GError *error = NULL;

	src_file = g_file_new_for_uri (TILE (tile)->uri);

	res = g_file_trash (src_file, NULL, &error);

	if (res)
		bookmark_agent_remove_item (priv->agent, TILE (tile)->uri);
	else {
		g_warning ("unable to move [%s] to the trash: %s\n", TILE (tile)->uri,
			   error->message);

		g_error_free (error);
	}

	g_object_unref (src_file);
}

static void
delete_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (tile);

	GtkDialog *confirm_dialog;
	gint       result;

	GFile *src_file;
	gboolean res;
	GError *error = NULL;


	if (GPOINTER_TO_INT (libslab_get_mateconf_value (MATECONF_CONFIRM_DELETE_KEY))) {
		confirm_dialog = GTK_DIALOG(gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, 
				GTK_BUTTONS_NONE, _("Are you sure you want to permanently delete \"%s\"?"), priv->basename));
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

	if (res)
		bookmark_agent_remove_item (priv->agent, TILE (tile)->uri);
	else {
		g_warning ("unable to delete [%s]: %s\n", TILE (tile)->uri,
			   error->message);
		g_error_free (error);
	}

	g_object_unref (src_file);
}

static void
user_docs_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	DocumentTile *this        = DOCUMENT_TILE             (tile);
	DocumentTilePrivate *priv = DOCUMENT_TILE_GET_PRIVATE (this);

	BookmarkItem *item;


	if (priv->is_bookmarked)
		bookmark_agent_remove_item (priv->agent, tile->uri);
	else {
		item = g_new0 (BookmarkItem, 1);
		item->uri       = tile->uri;
		item->mime_type = priv->mime_type;
		item->mtime     = priv->modified;
		if (priv->default_app) {
			item->app_name  = (gchar *) g_app_info_get_name (priv->default_app);
			item->app_exec  = (gchar *) g_app_info_get_executable (priv->default_app);
		}

		bookmark_agent_add_item (priv->agent, item);
		g_free (item);
	}

	update_user_list_menu_item (this);
}

static void
send_to_trigger (Tile *tile, TileEvent *event, TileAction *action)
{
	gchar *cmd;
	gchar **argv;

	gchar *filename;
	gchar *dirname;
	gchar *basename;

	GError *error = NULL;

	gchar *tmp;
	gint i;

	cmd = (gchar *) get_mateconf_value (MATECONF_SEND_TO_CMD_KEY);
	argv = g_strsplit (cmd, " ", 0);

	filename = g_filename_from_uri (TILE (tile)->uri, NULL, NULL);
	dirname = g_path_get_dirname (filename);
	basename = g_path_get_basename (filename);

	for (i = 0; argv[i]; ++i)
	{
		if (strstr (argv[i], "DIRNAME"))
		{
			tmp = string_replace_once (argv[i], "DIRNAME", dirname);
			g_free (argv[i]);
			argv[i] = tmp;
		}

		if (strstr (argv[i], "BASENAME"))
		{
			tmp = string_replace_once (argv[i], "BASENAME", basename);
			g_free (argv[i]);
			argv[i] = tmp;
		}
	}

	gdk_spawn_on_screen (gtk_widget_get_screen (GTK_WIDGET (tile)), NULL, argv, NULL,
		G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

	if (error)
		handle_g_error (&error, "error in %s", G_STRFUNC);

	g_free (cmd);
	g_free (filename);
	g_free (dirname);
	g_free (basename);
	g_strfreev (argv);
}

static void
agent_notify_cb (GObject *g_obj, GParamSpec *pspec, gpointer user_data)
{
	update_user_list_menu_item (DOCUMENT_TILE (user_data));
}
