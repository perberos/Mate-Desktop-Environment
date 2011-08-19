
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define TOTEM_OBJECT_HAS_SIGNAL(obj, name) (g_signal_lookup (name, g_type_from_name (G_OBJECT_TYPE_NAME (obj))) != 0)

void totem_gdk_window_set_invisible_cursor (GdkWindow *window);
void totem_gdk_window_set_waiting_cursor (GdkWindow *window);

gboolean totem_display_is_local (void);

char *totem_time_to_string (gint64 msecs);
gint64 totem_string_to_time (const char *time_string);
char *totem_time_to_string_text (gint64 msecs);

void totem_widget_set_preferred_size (GtkWidget *widget, gint width,
				      gint height);
gboolean totem_ratio_fits_screen (GdkWindow *window, int video_width,
				  int video_height, gfloat ratio);

