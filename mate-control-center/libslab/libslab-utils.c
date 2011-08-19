#include "libslab-utils.h"

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <mateconf/mateconf-value.h>
#include <gtk/gtk.h>

#define DESKTOP_ITEM_TERMINAL_EMULATOR_FLAG "TerminalEmulator"
#define ALTERNATE_DOCPATH_KEY               "DocPath"

static FILE *checkpoint_file;

gboolean
libslab_gtk_image_set_by_id (GtkImage *image, const gchar *id)
{
	GdkPixbuf *pixbuf;

	gint size;
	gint width;
	gint height;

	GtkIconTheme *icon_theme;

	gboolean found;

	gchar *tmp;


	if (! id)
		return FALSE;

	g_object_get (G_OBJECT (image), "icon-size", & size, NULL);

	if (size == GTK_ICON_SIZE_INVALID)
		size = GTK_ICON_SIZE_DND;

	gtk_icon_size_lookup (size, & width, & height);

	if (g_path_is_absolute (id)) {
		pixbuf = gdk_pixbuf_new_from_file_at_size (id, width, height, NULL);

		found = (pixbuf != NULL);

		if (found) {
			gtk_image_set_from_pixbuf (image, pixbuf);

			g_object_unref (pixbuf);
		}
		else
			gtk_image_set_from_stock (image, "gtk-missing-image", size);
	}
	else {
		tmp = g_strdup (id);

		if ( /* file extensions are not copesetic with loading by "name" */
			g_str_has_suffix (tmp, ".png") ||
			g_str_has_suffix (tmp, ".svg") ||
			g_str_has_suffix (tmp, ".xpm")
		)

			tmp [strlen (tmp) - 4] = '\0';

		if (gtk_widget_has_screen (GTK_WIDGET (image)))
			icon_theme = gtk_icon_theme_get_for_screen (
				gtk_widget_get_screen (GTK_WIDGET (image)));
		else
			icon_theme = gtk_icon_theme_get_default ();

		found = gtk_icon_theme_has_icon (icon_theme, tmp);

		if (found)
			gtk_image_set_from_icon_name (image, tmp, size);
		else
			gtk_image_set_from_stock (image, "gtk-missing-image", size);

		g_free (tmp);
	}

	return found;
}

MateDesktopItem *
libslab_mate_desktop_item_new_from_unknown_id (const gchar *id)
{
	MateDesktopItem *item;
	gchar            *basename;

	GError *error = NULL;


	if (! id)
		return NULL;

	item = mate_desktop_item_new_from_uri (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_file (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_basename (id, 0, & error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	basename = g_strrstr (id, "/");

	if (basename) {
		basename++;

		item = mate_desktop_item_new_from_basename (basename, 0, &error);

		if (! error)
			return item;
		else {
			g_error_free (error);
			error = NULL;
		}
	}

	return NULL;
}

gboolean
libslab_mate_desktop_item_launch_default (MateDesktopItem *item)
{
	GError *error = NULL;

	if (! item)
		return FALSE;

	mate_desktop_item_launch (item, NULL, MATE_DESKTOP_ITEM_LAUNCH_ONLY_ONE, & error);

	if (error) {
		g_warning ("error launching %s [%s]\n",
			mate_desktop_item_get_location (item), error->message);

		g_error_free (error);

		return FALSE;
	}

	return TRUE;
}

gchar *
libslab_mate_desktop_item_get_docpath (MateDesktopItem *item)
{
	gchar *path;

	path = g_strdup (mate_desktop_item_get_localestring (item, MATE_DESKTOP_ITEM_DOC_PATH));

	if (! path)
		path = g_strdup (mate_desktop_item_get_localestring (item, ALTERNATE_DOCPATH_KEY));

	return path;
}

/* Ugh, here we don't have knowledge of the screen that is being used.  So, do
 * what we can to find it.
 */
GdkScreen *
libslab_get_current_screen (void)
{
	GdkEvent *event;
	GdkScreen *screen = NULL;

	event = gtk_get_current_event ();
	if (event) {
		if (event->any.window)
			screen = gdk_drawable_get_screen (GDK_DRAWABLE (event->any.window));

		gdk_event_free (event);
	}

	if (!screen)
		screen = gdk_screen_get_default ();

	return screen;
}

gboolean
libslab_mate_desktop_item_open_help (MateDesktopItem *item)
{
	gchar *doc_path;
	gchar *help_uri;

	GError *error = NULL;

	gboolean retval = FALSE;


	if (! item)
		return retval;

	doc_path = libslab_mate_desktop_item_get_docpath (item);

	if (doc_path) {
		help_uri = g_strdup_printf ("ghelp:%s", doc_path);

		if (!gtk_show_uri (libslab_get_current_screen (), help_uri, gtk_get_current_event_time (), &error)) {
			g_warning ("error opening %s [%s]\n", help_uri, error->message);

			g_error_free (error);

			retval = FALSE;
		}
		else
			retval = TRUE;

		g_free (help_uri);
		g_free (doc_path);
	}

	return retval;
}

guint32
libslab_get_current_time_millis ()
{
	GTimeVal t_curr;

	g_get_current_time (& t_curr);

	return 1000L * t_curr.tv_sec + t_curr.tv_usec / 1000L;
}

gint
libslab_strcmp (const gchar *a, const gchar *b)
{
	if (! a && ! b)
		return 0;

	if (! a)
		return strcmp ("", b);

	if (! b)
		return strcmp (a, "");

	return strcmp (a, b);
}

gint
libslab_strlen (const gchar *a)
{
	if (! a)
		return 0;

	return strlen (a);
}

gpointer
libslab_get_mateconf_value (const gchar *key)
{
	MateConfClient *client;
	MateConfValue  *value;
	GError      *error = NULL;

	gpointer retval = NULL;

	GList  *list;
	GSList *slist;

	MateConfValue *value_i;
	GSList     *node;


	client = mateconf_client_get_default ();
	value  = mateconf_client_get (client, key, & error);

	if (error || ! value)
		libslab_handle_g_error (& error, "%s: error getting %s", G_STRFUNC, key);
	else {
		switch (value->type) {
			case MATECONF_VALUE_STRING:
				retval = (gpointer) g_strdup (mateconf_value_get_string (value));
				break;

			case MATECONF_VALUE_INT:
				retval = GINT_TO_POINTER (mateconf_value_get_int (value));
				break;

			case MATECONF_VALUE_BOOL:
				retval = GINT_TO_POINTER (mateconf_value_get_bool (value));
				break;

			case MATECONF_VALUE_LIST:
				list = NULL;
				slist = mateconf_value_get_list (value);

				for (node = slist; node; node = node->next) {
					value_i = (MateConfValue *) node->data;

					if (value_i->type == MATECONF_VALUE_STRING)
						list = g_list_append (
							list, g_strdup (
								mateconf_value_get_string (value_i)));
					else if (value_i->type == MATECONF_VALUE_INT)
						list = g_list_append (
							list, GINT_TO_POINTER (
								mateconf_value_get_int (value_i)));
					else
						;
				}

				retval = (gpointer) list;

				break;

			default:
				break;
		}
	}

	g_object_unref (client);

	if (value)
		mateconf_value_free (value);

	return retval;
}

void
libslab_set_mateconf_value (const gchar *key, gconstpointer data)
{
	MateConfClient *client;
	MateConfValue  *value;

	MateConfValueType type;
	MateConfValueType list_type;

	GSList *slist = NULL;

	GError *error = NULL;

	MateConfValue *value_i;
	GList      *node;


	client = mateconf_client_get_default ();
	value  = mateconf_client_get (client, key, & error);

	if (error) {
		libslab_handle_g_error (&error, "%s: error getting %s", G_STRFUNC, key);

		goto exit;
	}

	type = value->type;
	list_type = ((type == MATECONF_VALUE_LIST) ?
		mateconf_value_get_list_type (value) : MATECONF_VALUE_INVALID);

	mateconf_value_free (value);
	value = mateconf_value_new (type);

	if (type == MATECONF_VALUE_LIST)
		mateconf_value_set_list_type (value, list_type);

	switch (type) {
		case MATECONF_VALUE_STRING:
			mateconf_value_set_string (value, g_strdup ((gchar *) data));
			break;

		case MATECONF_VALUE_INT:
			mateconf_value_set_int (value, GPOINTER_TO_INT (data));
			break;

		case MATECONF_VALUE_BOOL:
			mateconf_value_set_bool (value, GPOINTER_TO_INT (data));
			break;

		case MATECONF_VALUE_LIST:
			for (node = (GList *) data; node; node = node->next) {
				value_i = mateconf_value_new (list_type);

				if (list_type == MATECONF_VALUE_STRING)
					mateconf_value_set_string (value_i, (const gchar *) node->data);
				else if (list_type == MATECONF_VALUE_INT)
					mateconf_value_set_int (value_i, GPOINTER_TO_INT (node->data));
				else
					g_assert_not_reached ();

				slist = g_slist_append (slist, value_i);
			}

			mateconf_value_set_list_nocopy (value, slist);

			break;

		default:
			break;
	}

	mateconf_client_set (client, key, value, & error);

	if (error)
		libslab_handle_g_error (&error, "%s: error setting %s", G_STRFUNC, key);

exit:

	mateconf_value_free (value);
	g_object_unref (client);
}

guint
libslab_mateconf_notify_add (const gchar *key, MateConfClientNotifyFunc callback, gpointer user_data)
{
	MateConfClient *client;
	guint        conn_id;

	GError *error = NULL;


	client  = mateconf_client_get_default ();
	conn_id = mateconf_client_notify_add (client, key, callback, user_data, NULL, & error);

	if (error)
		libslab_handle_g_error (
			& error, "%s: error adding mateconf notify for (%s)", G_STRFUNC, key);

	g_object_unref (client);

	return conn_id;
}

void
libslab_mateconf_notify_remove (guint conn_id)
{
	MateConfClient *client;

	GError *error = NULL;


	if (conn_id == 0)
		return;

	client = mateconf_client_get_default ();
	mateconf_client_notify_remove (client, conn_id);

	if (error)
		libslab_handle_g_error (
			& error, "%s: error removing mateconf notify", G_STRFUNC);

	g_object_unref (client);
}

void
libslab_handle_g_error (GError **error, const gchar *msg_format, ...)
{
	gchar   *msg;
	va_list  args;


	va_start (args, msg_format);
	msg = g_strdup_vprintf (msg_format, args);
	va_end (args);

	if (*error) {
		g_log (
			G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"\nGError raised: [%s]\nuser_message: [%s]\n", (*error)->message, msg);

		g_error_free (*error);

		*error = NULL;
	}
	else
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "\nerror raised: [%s]\n", msg);

	g_free (msg);
}

gboolean
libslab_desktop_item_is_a_terminal (const gchar *uri)
{
	MateDesktopItem *d_item;
	const gchar      *categories;

	gboolean is_terminal = FALSE;


	d_item = libslab_mate_desktop_item_new_from_unknown_id (uri);

	if (! d_item)
		return FALSE;

	categories = mate_desktop_item_get_string (d_item, MATE_DESKTOP_ITEM_CATEGORIES);

	is_terminal = (categories && strstr (categories, DESKTOP_ITEM_TERMINAL_EMULATOR_FLAG));

	mate_desktop_item_unref (d_item);

	return is_terminal;
}

gboolean
libslab_desktop_item_is_logout (const gchar *uri)
{
	MateDesktopItem *d_item;
	gboolean is_logout = FALSE;


	d_item = libslab_mate_desktop_item_new_from_unknown_id (uri);

	if (! d_item)
		return FALSE;

	is_logout = strstr ("Logout", mate_desktop_item_get_string (d_item, MATE_DESKTOP_ITEM_NAME)) != NULL;

	mate_desktop_item_unref (d_item);

	return is_logout;
}

gboolean
libslab_desktop_item_is_lockscreen (const gchar *uri)
{
	MateDesktopItem *d_item;
	gboolean is_logout = FALSE;


	d_item = libslab_mate_desktop_item_new_from_unknown_id (uri);

	if (! d_item)
		return FALSE;

	is_logout = strstr ("Lock Screen", mate_desktop_item_get_string (d_item, MATE_DESKTOP_ITEM_NAME)) != NULL;

	mate_desktop_item_unref (d_item);

	return is_logout;
}

gchar *
libslab_string_replace_once (const gchar *string, const gchar *key, const gchar *value)
{
	GString *str_built;
	gint pivot;


	pivot = strstr (string, key) - string;

	str_built = g_string_new_len (string, pivot);
	g_string_append (str_built, value);
	g_string_append (str_built, & string [pivot + strlen (key)]);

	return g_string_free (str_built, FALSE);
}

void
libslab_spawn_command (const gchar *cmd)
{
	gchar **argv;

	GError *error = NULL;


	if (! cmd || strlen (cmd) < 1)
		return;

	argv = g_strsplit (cmd, " ", -1);

	g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, & error);

	if (error)
		libslab_handle_g_error (& error, "%s: error spawning [%s]", G_STRFUNC, cmd);

	g_strfreev (argv);
}

static guint thumbnail_factory_idle_id;
static MateDesktopThumbnailFactory *thumbnail_factory;

static void
create_thumbnail_factory (void)
{
	/* The thumbnail_factory may already have been created by an applet
	 * instance that was launched before the current one.
	 */
	if (thumbnail_factory != NULL)
		return;

	libslab_checkpoint ("create_thumbnail_factory(): start");

	thumbnail_factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

	libslab_checkpoint ("create_thumbnail_factory(): end");
}

static gboolean
init_thumbnail_factory_idle_cb (gpointer data)
{
	create_thumbnail_factory ();
	thumbnail_factory_idle_id = 0;
	return FALSE;
}

void
libslab_thumbnail_factory_preinit (void)
{
	thumbnail_factory_idle_id = g_idle_add (init_thumbnail_factory_idle_cb, NULL);
}

MateDesktopThumbnailFactory *
libslab_thumbnail_factory_get (void)
{
	if (thumbnail_factory_idle_id != 0) {
		g_source_remove (thumbnail_factory_idle_id);
		thumbnail_factory_idle_id = 0;

		create_thumbnail_factory ();
	}

	g_assert (thumbnail_factory != NULL);
	return thumbnail_factory;
}

void
libslab_checkpoint_init (const char *checkpoint_config_file_basename,
			 const char *checkpoint_file_basename)
{
	char *filename;
	struct stat st;
	int result;
	time_t t;
	struct tm tm;
	char *checkpoint_full_basename;

	g_return_if_fail (checkpoint_config_file_basename != NULL);
	g_return_if_fail (checkpoint_file_basename != NULL);

	filename = g_build_filename (g_get_home_dir (), checkpoint_config_file_basename, NULL);

	result = stat (filename, &st);
	g_free (filename);

	if (result != 0)
		return;

	t = time (NULL);
	tm = *localtime (&t);

	checkpoint_full_basename = g_strdup_printf ("%s-%04d-%02d-%02d-%02d-%02d-%02d.checkpoint",
						    checkpoint_file_basename,
						    tm.tm_year + 1900,
						    tm.tm_mon + 1,
						    tm.tm_mday,
						    tm.tm_hour,
						    tm.tm_min,
						    tm.tm_sec);

	filename = g_build_filename (g_get_home_dir (), checkpoint_full_basename, NULL);
	g_free (checkpoint_full_basename);

	checkpoint_file = fopen (filename, "w");
	g_free (filename);
}

void
libslab_checkpoint (const char *format, ...)
{
	va_list args;
	struct timeval tv;
	struct tm tm;
	struct rusage rusage;

	if (!checkpoint_file)
		return;

	gettimeofday (&tv, NULL);
	tm = *localtime (&tv.tv_sec);

	getrusage (RUSAGE_SELF, &rusage);

	fprintf (checkpoint_file,
		 "%02d:%02d:%02d.%04d (user:%d.%04d, sys:%d.%04d) - ",
		 (int) tm.tm_hour,
		 (int) tm.tm_min,
		 (int) tm.tm_sec,
		 (int) (tv.tv_usec / 100),
		 (int) rusage.ru_utime.tv_sec,
		 (int) (rusage.ru_utime.tv_usec / 100),
		 (int) rusage.ru_stime.tv_sec,
		 (int) (rusage.ru_stime.tv_usec / 100));

	va_start (args, format);
	vfprintf (checkpoint_file, format, args);
	va_end (args);

	fputs ("\n", checkpoint_file);
	fflush (checkpoint_file);
}
