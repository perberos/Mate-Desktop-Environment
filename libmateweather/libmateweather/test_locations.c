
#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "location-entry.h"
#include "timezone-menu.h"

static void
deleted (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit ();
}

static void
location_changed (GObject *object, GParamSpec *param, gpointer tzmenu)
{
    MateWeatherLocationEntry *entry = MATEWEATHER_LOCATION_ENTRY (object);
    MateWeatherLocation *loc;
    MateWeatherTimezone *zone;

    loc = mateweather_location_entry_get_location (entry);
    g_return_if_fail (loc != NULL);
    zone = mateweather_location_get_timezone (loc);
    if (zone)
	mateweather_timezone_menu_set_tzid (tzmenu, mateweather_timezone_get_tzid (zone));
    else
	mateweather_timezone_menu_set_tzid (tzmenu, NULL);
    if (zone)
	mateweather_timezone_unref (zone);
    mateweather_location_unref (loc);
}

int
main (int argc, char **argv)
{
    MateWeatherLocation *loc;
    GtkWidget *window, *vbox, *entry;
    GtkWidget *combo;
    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "location");
    gtk_container_set_border_width (GTK_CONTAINER (window), 8);
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (deleted), NULL);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    loc = mateweather_location_new_world (FALSE);
    entry = mateweather_location_entry_new (loc);
    gtk_widget_set_size_request (entry, 400, -1);
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);

    combo = mateweather_timezone_menu_new (loc);
    mateweather_location_unref (loc);
    gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, TRUE, 0);

    g_signal_connect (entry, "notify::location",
		      G_CALLBACK (location_changed), combo);

    gtk_widget_show_all (window);

    gtk_main ();

    return 0;
}
