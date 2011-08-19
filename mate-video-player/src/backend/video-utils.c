
#include "config.h"

#include "video-utils.h"

#include <glib/gi18n.h>
#include <libintl.h>

#include <gdk/gdk.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void
totem_gdk_window_set_invisible_cursor (GdkWindow *window)
{
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_BLANK_CURSOR);
	gdk_window_set_cursor (window, cursor);
	gdk_cursor_unref (cursor);
}

void
totem_gdk_window_set_waiting_cursor (GdkWindow *window)
{
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (window, cursor);
	gdk_cursor_unref (cursor);

	gdk_flush ();
}

gboolean
totem_display_is_local (void)
{
	const char *name, *work;
	int display, screen;
	gboolean has_hostname;

	name = gdk_display_get_name (gdk_display_get_default ());
	if (name == NULL)
		return TRUE;

	work = strstr (name, ":");
	if (work == NULL)
		return TRUE;

	has_hostname = (work - name) > 0;

	/* Get to the character after the colon */
	work++;
	if (*work == '\0')
		return TRUE;

	if (sscanf (work, "%d.%d", &display, &screen) != 2)
		return TRUE;

	if (has_hostname == FALSE)
		return TRUE;

	if (display < 10)
		return TRUE;

	return FALSE;
}

char *
totem_time_to_string (gint64 msecs)
{
	int sec, min, hour, _time;

	_time = (int) (msecs / 1000);
	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	if (hour > 0)
	{
		/* hour:minutes:seconds */
		/* Translators: This is a time format, like "9:05:02" for 9
		 * hours, 5 minutes, and 2 seconds. You may change ":" to
		 * the separator that your locale uses or use "%Id" instead
		 * of "%d" if your locale uses localized digits.
		 */
		return g_strdup_printf (C_("long time format", "%d:%02d:%02d"), hour, min, sec);
	}

	/* minutes:seconds */
	/* Translators: This is a time format, like "5:02" for 5
	 * minutes and 2 seconds. You may change ":" to the
	 * separator that your locale uses or use "%Id" instead of
	 * "%d" if your locale uses localized digits.
	 */
	return g_strdup_printf (C_("short time format", "%d:%02d"), min, sec);
}

gint64
totem_string_to_time (const char *time_string)
{
	int sec, min, hour, args;

	args = sscanf (time_string, C_("long time format", "%d:%02d:%02d"), &hour, &min, &sec);

	if (args == 3) {
		/* Parsed all three arguments successfully */
		return (hour * (60 * 60) + min * 60 + sec) * 1000;
	} else if (args == 2) {
		/* Only parsed the first two arguments; treat hour and min as min and sec, respectively */
		return (hour * 60 + min) * 1000;
	} else if (args == 1) {
		/* Only parsed the first argument; treat hour as sec */
		return hour * 1000;
	} else {
		/* Error! */
		return -1;
	}
}

char *
totem_time_to_string_text (gint64 msecs)
{
	char *secs, *mins, *hours, *string;
	int sec, min, hour, _time;

	_time = (int) (msecs / 1000);
	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	hours = g_strdup_printf (ngettext ("%d hour", "%d hours", hour), hour);

	mins = g_strdup_printf (ngettext ("%d minute",
					  "%d minutes", min), min);

	secs = g_strdup_printf (ngettext ("%d second",
					  "%d seconds", sec), sec);

	if (hour > 0)
	{
		/* hour:minutes:seconds */
		string = g_strdup_printf (_("%s %s %s"), hours, mins, secs);
	} else if (min > 0) {
		/* minutes:seconds */
		string = g_strdup_printf (_("%s %s"), mins, secs);
	} else if (sec > 0) {
		/* seconds */
		string = g_strdup_printf (_("%s"), secs);
	} else {
		/* 0 seconds */
		string = g_strdup (_("0 seconds"));
	}

	g_free (hours);
	g_free (mins);
	g_free (secs);

	return string;
}

typedef struct _TotemPrefSize {
  gint width, height;
  gulong sig_id;
} TotemPrefSize;

static gboolean
cb_unset_size (gpointer data)
{
  GtkWidget *widget = data;

  gtk_widget_queue_resize_no_redraw (widget);

  return FALSE;
}

static void
cb_set_preferred_size (GtkWidget *widget, GtkRequisition *req,
		       gpointer data)
{
  TotemPrefSize *size = data;

  req->width = size->width;
  req->height = size->height;

  g_signal_handler_disconnect (widget, size->sig_id);
  g_free (size);
  g_idle_add (cb_unset_size, widget);
}

void
totem_widget_set_preferred_size (GtkWidget *widget, gint width,
				 gint height)
{
  TotemPrefSize *size = g_new (TotemPrefSize, 1);

  size->width = width;
  size->height = height;
  size->sig_id = g_signal_connect (widget, "size-request",
				   G_CALLBACK (cb_set_preferred_size),
				   size);

  gtk_widget_queue_resize (widget);
}

gboolean
totem_ratio_fits_screen (GdkWindow *video_window, int video_width,
			 int video_height, gfloat ratio)
{
	GdkRectangle fullscreen_rect;
	int new_w, new_h;
	GdkScreen *screen;

	if (video_width <= 0 || video_height <= 0)
		return TRUE;

	new_w = video_width * ratio;
	new_h = video_height * ratio;

	screen = gdk_drawable_get_screen (GDK_DRAWABLE (video_window));
	gdk_screen_get_monitor_geometry (screen,
			gdk_screen_get_monitor_at_window
			(screen, video_window),
			&fullscreen_rect);

	if (new_w > (fullscreen_rect.width - 128) ||
			new_h > (fullscreen_rect.height - 128))
	{
		return FALSE;
	}

	return TRUE;
}

