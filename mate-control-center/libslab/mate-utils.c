#include "mate-utils.h"

#include <string.h>

static void section_header_style_set (GtkWidget *, GtkStyle *, gpointer);

gboolean
load_image_by_id (GtkImage * image, GtkIconSize size, const gchar * image_id)
{
	GdkPixbuf *pixbuf;
	gint width;
	gint height;

	GtkIconTheme *icon_theme;

	gchar *id;

	gboolean icon_exists;

	if (!image_id)
		return FALSE;

	id = g_strdup (image_id);

	gtk_icon_size_lookup (size, &width, &height);

	if (g_path_is_absolute (id))
	{
		pixbuf = gdk_pixbuf_new_from_file_at_size (id, width, height, NULL);

		icon_exists = (pixbuf != NULL);

		if (icon_exists)
		{
			gtk_image_set_from_pixbuf (image, pixbuf);

			g_object_unref (pixbuf);
		}
		else
			gtk_image_set_from_stock (image, "gtk-missing-image", size);
	}
	else
	{
		if (		/* file extensions are not copesetic with loading by "name" */
			g_str_has_suffix (id, ".png") ||
			g_str_has_suffix (id, ".svg") ||
			g_str_has_suffix (id, ".xpm")
		   )

			id[strlen (id) - 4] = '\0';

		if (gtk_widget_has_screen (GTK_WIDGET (image)))
			icon_theme =
				gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET
					(image)));
		else
			icon_theme = gtk_icon_theme_get_default ();

		icon_exists = gtk_icon_theme_has_icon (icon_theme, id);

		if (icon_exists)
			gtk_image_set_from_icon_name (image, id, size);
		else
			gtk_image_set_from_stock (image, "gtk-missing-image", size);

	}

	g_free (id);

	return icon_exists;
}

MateDesktopItem *
load_desktop_item_by_unknown_id (const gchar * id)
{
	MateDesktopItem *item;
	GError *error = NULL;

	item = mate_desktop_item_new_from_uri (id, 0, &error);

	if (!error)
		return item;
	else
	{
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_file (id, 0, &error);

	if (!error)
		return item;
	else
	{
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_basename (id, 0, &error);

	if (!error)
		return item;
	else
	{
		g_error_free (error);
		error = NULL;
	}

	return NULL;
}

gpointer
get_mateconf_value (const gchar * key)
{
	MateConfClient *client;
	MateConfValue *value;
	GError *error = NULL;

	gpointer retval = NULL;

	GList *list;
	GSList *slist;

	MateConfValue *value_i;
	GSList *node;

	client = mateconf_client_get_default ();
	value = mateconf_client_get (client, key, &error);

	if (error || ! value)
	{
		handle_g_error (&error, "%s: error getting %s", G_STRFUNC, key);

		goto exit;
	}

	switch (value->type)
	{
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

		for (node = slist; node; node = node->next)
		{
			value_i = (MateConfValue *) node->data;

			if (value_i->type == MATECONF_VALUE_STRING)
				list = g_list_append (list,
					g_strdup (mateconf_value_get_string (value_i)));
			else if (value_i->type == MATECONF_VALUE_INT)
				list = g_list_append (list,
					GINT_TO_POINTER (mateconf_value_get_int (value_i)));
			else
				g_assert_not_reached ();
		}

		retval = (gpointer) list;

		break;

	default:
		g_assert_not_reached ();
		break;
	}

	exit:

	g_object_unref (client);
	if(value)
		mateconf_value_free (value);

	return retval;
}

void
set_mateconf_value (const gchar * key, gconstpointer data)
{
	MateConfClient *client;
	MateConfValue *value;

	MateConfValueType type;
	MateConfValueType list_type;

	GSList *slist = NULL;

	GError *error = NULL;

	MateConfValue *value_i;
	GList *node;

	client = mateconf_client_get_default ();
	value = mateconf_client_get (client, key, &error);

	if (error)
	{
		handle_g_error (&error, "%s: error getting %s", G_STRFUNC, key);

		goto exit;
	}

	type = value->type;
	list_type =
		(type ==
		MATECONF_VALUE_LIST ? mateconf_value_get_list_type (value) : MATECONF_VALUE_INVALID);

	mateconf_value_free (value);
	value = mateconf_value_new (type);

	if (type == MATECONF_VALUE_LIST)
		mateconf_value_set_list_type (value, list_type);

	switch (type)
	{
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
		for (node = (GList *) data; node; node = node->next)
		{
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
		g_assert_not_reached ();
		break;
	}

	mateconf_client_set (client, key, value, &error);

	if (error)
		handle_g_error (&error, "%s: error setting %s", G_STRFUNC, key);

      exit:

	mateconf_value_free (value);
	g_object_unref (client);
}

guint
connect_mateconf_notify (const gchar * key, MateConfClientNotifyFunc cb, gpointer user_data)
{
	MateConfClient *client;
	guint conn_id;

	GError *error = NULL;

	client = mateconf_client_get_default ();
	conn_id = mateconf_client_notify_add (client, key, cb, user_data, NULL, &error);

	if (error)
		handle_g_error (&error, "%s: error adding notify for (%s)", G_STRFUNC, key);

	g_object_unref (client);

	return conn_id;
}

void
handle_g_error (GError ** error, const gchar * msg_format, ...)
{
	gchar *msg;
	va_list args;

	va_start (args, msg_format);
	msg = g_strdup_vprintf (msg_format, args);
	va_end (args);

	if (*error)
	{
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"\nGError raised: [%s]\nuser_message: [%s]\n", (*error)->message, msg);

		g_error_free (*error);

		*error = NULL;
	}
	else
		g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "\nerror raised: [%s]\n", msg);

	g_free (msg);
}

GtkWidget *
get_main_menu_section_header (const gchar * markup)
{
	GtkWidget *label;
	gchar *text;

	text = g_strdup_printf ("<span size=\"large\">%s</span>", markup);

	label = gtk_label_new (text);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_set_name (label, "mate-main-menu-section-header");

	g_signal_connect (G_OBJECT (label), "style-set", G_CALLBACK (section_header_style_set),
		NULL);

	g_free (text);

	return label;
}

static void
section_header_style_set (GtkWidget * widget, GtkStyle * prev_style, gpointer user_data)
{
	if (prev_style
		&& widget->style->fg[GTK_STATE_SELECTED].green ==
		prev_style->fg[GTK_STATE_SELECTED].green)
		return;

	gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &widget->style->bg[GTK_STATE_SELECTED]);
}
