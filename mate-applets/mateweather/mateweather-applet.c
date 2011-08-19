/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main applet widget
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <mateconf/mateconf-client.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBMATENOTIFY
#include <libmatenotify/notify.h>
#include <libmatenotify/notification.h>
#endif

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#ifdef HAVE_NETWORKMANAGER
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <NetworkManager/NetworkManager.h>
#endif

#include "mateweather.h"
#include "mateweather-about.h"
#include "mateweather-pref.h"
#include "mateweather-dialog.h"
#include "mateweather-applet.h"

#define MAX_CONSECUTIVE_FAULTS (3)

static void about_cb (GtkAction      *action,
		      MateWeatherApplet *gw_applet)
{

    mateweather_about_run (gw_applet);
}

static void help_cb (GtkAction      *action,
		     MateWeatherApplet *gw_applet)
{
    GError *error = NULL;

    gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (gw_applet->applet)),
		"ghelp:mateweather",
		gtk_get_current_event_time (),
		&error);

    if (error) { 
	GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						    _("There was an error displaying help: %s"), error->message);
	g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (gw_applet->applet)));
	gtk_widget_show (dialog);
        g_error_free (error);
        error = NULL;
    }
}

static void pref_cb (GtkAction      *action,
		     MateWeatherApplet *gw_applet)
{
   if (gw_applet->pref_dialog) {
	gtk_window_present (GTK_WINDOW (gw_applet->pref_dialog));
   } else {
	gw_applet->pref_dialog = mateweather_pref_new(gw_applet);
	g_object_add_weak_pointer(G_OBJECT(gw_applet->pref_dialog),
				  (gpointer *)&(gw_applet->pref_dialog));
	gtk_widget_show_all (gw_applet->pref_dialog);
   }
}

static void details_cb (GtkAction      *action,
			MateWeatherApplet *gw_applet)
{
   if (gw_applet->details_dialog) {
	gtk_window_present (GTK_WINDOW (gw_applet->details_dialog));
   } else {
	gw_applet->details_dialog = mateweather_dialog_new(gw_applet);
	g_object_add_weak_pointer(G_OBJECT(gw_applet->details_dialog),
				  (gpointer *)&(gw_applet->details_dialog));
	mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
	gtk_widget_show (gw_applet->details_dialog);
   }
}

static void update_cb (GtkAction      *action,
		       MateWeatherApplet *gw_applet)
{
    mateweather_update (gw_applet);
}


static const GtkActionEntry weather_applet_menu_actions [] = {
	{ "Details", NULL, N_("_Details"),
	  NULL, NULL,
	  G_CALLBACK (details_cb) },
	{ "Update", GTK_STOCK_REFRESH, N_("_Update"),
	  NULL, NULL,
	  G_CALLBACK (update_cb) },
	{ "Props", GTK_STOCK_PROPERTIES, N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (pref_cb) },
	{ "Help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

static void place_widgets (MateWeatherApplet *gw_applet)
{
    GtkRequisition req;
    int total_size = 0;
    gboolean horizontal = FALSE;
    int panel_size = gw_applet->size;
    const gchar *temp;   
    const gchar *icon_name;
	
    switch (gw_applet->orient) {
	case MATE_PANEL_APPLET_ORIENT_LEFT:
	case MATE_PANEL_APPLET_ORIENT_RIGHT:
	    horizontal = FALSE;
	    break;
	case MATE_PANEL_APPLET_ORIENT_UP:
	case MATE_PANEL_APPLET_ORIENT_DOWN:
	    horizontal = TRUE;
	    break;
    }

    /* Create the weather icon */
    icon_name = weather_info_get_icon_name (gw_applet->mateweather_info);
    gw_applet->image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON); 

    if (icon_name != NULL) {
        gtk_widget_size_request(gw_applet->image, &req);
        if (horizontal)
            total_size += req.height;
        else
            total_size += req.width;
    }

    /* Create the temperature label */
    gw_applet->label = gtk_label_new("0\302\260F");
    
    /* Update temperature text */
    temp = weather_info_get_temp_summary(gw_applet->mateweather_info);
    if (temp) 
        gtk_label_set_text(GTK_LABEL(gw_applet->label), temp);

    /* Check the label size to determine box layout */
    gtk_widget_size_request(gw_applet->label, &req);
    if (horizontal)
        total_size += req.height;
    else
        total_size += req.width;

    /* Pack the box */
    if (gw_applet->box)
        gtk_widget_destroy (gw_applet->box);
    
    if (horizontal && (total_size <= panel_size))
        gw_applet->box = gtk_vbox_new(FALSE, 0);
    else if (horizontal && (total_size > panel_size))
        gw_applet->box = gtk_hbox_new(FALSE, 2);
    else if (!horizontal && (total_size <= panel_size))
        gw_applet->box = gtk_hbox_new(FALSE, 2);
    else 
        gw_applet->box = gtk_vbox_new(FALSE, 0);

    /* Rebuild the applet it's visual area */
    gtk_container_add (GTK_CONTAINER (gw_applet->container), gw_applet->box);
    gtk_box_pack_start (GTK_BOX (gw_applet->box), gw_applet->image, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (gw_applet->box), gw_applet->label, TRUE, TRUE, 0);

    gtk_widget_show_all (GTK_WIDGET (gw_applet->applet));
}

static void change_orient_cb (MatePanelApplet *w, MatePanelAppletOrient o, gpointer data)
{
    MateWeatherApplet *gw_applet = (MateWeatherApplet *)data;
	
    gw_applet->orient = o;
    place_widgets(gw_applet);
    return;
}

static void size_allocate_cb(MatePanelApplet *w, GtkAllocation *allocation, gpointer data)
{
    MateWeatherApplet *gw_applet = (MateWeatherApplet *)data;
	
    if ((gw_applet->orient == MATE_PANEL_APPLET_ORIENT_LEFT) || (gw_applet->orient == MATE_PANEL_APPLET_ORIENT_RIGHT)) {
      if (gw_applet->size == allocation->width)
	return;
      gw_applet->size = allocation->width;
    } else {
      if (gw_applet->size == allocation->height)
	return;
      gw_applet->size = allocation->height;
    }
	
    place_widgets(gw_applet);
    return;
}

static gboolean clicked_cb (GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
    MateWeatherApplet *gw_applet = data;

    if ((ev == NULL) || (ev->button != 1))
        return FALSE;

    if (ev->type == GDK_BUTTON_PRESS) {
	if (!gw_applet->details_dialog)
		details_cb (NULL, gw_applet);
	else
		gtk_widget_destroy (GTK_WIDGET (gw_applet->details_dialog));
	
	return TRUE;
    }
    
    return FALSE;
}

static gboolean 
key_press_cb (GtkWidget *widget, GdkEventKey *event, MateWeatherApplet *gw_applet)
{
	switch (event->keyval) {	
	case GDK_u:
		if (event->state == GDK_CONTROL_MASK) {
			mateweather_update (gw_applet);
			return TRUE;
		}
		break;
	case GDK_d:
		if (event->state == GDK_CONTROL_MASK) {
			details_cb (NULL, gw_applet);
			return TRUE;
		}
		break;		
	case GDK_KP_Enter:
	case GDK_ISO_Enter:
	case GDK_3270_Enter:
	case GDK_Return:
	case GDK_space:
	case GDK_KP_Space:
		details_cb (NULL, gw_applet);
		return TRUE;
	default:
		break;
	}

	return FALSE;

}

static void
applet_destroy (GtkWidget *widget, MateWeatherApplet *gw_applet)
{
    if (gw_applet->pref_dialog)
       gtk_widget_destroy (gw_applet->pref_dialog);

    if (gw_applet->details_dialog)
       gtk_widget_destroy (gw_applet->details_dialog);

    if (gw_applet->timeout_tag > 0) {
       g_source_remove(gw_applet->timeout_tag);
       gw_applet->timeout_tag = 0;
    }
	
    if (gw_applet->suncalc_timeout_tag > 0) {
       g_source_remove(gw_applet->suncalc_timeout_tag);
       gw_applet->suncalc_timeout_tag = 0;
    }
	
    if (gw_applet->mateconf) {
       mateweather_mateconf_free (gw_applet->mateconf);
    }

    weather_info_abort (gw_applet->mateweather_info);
}

#ifdef HAVE_NETWORKMANAGER
static void setup_network_monitor (MateWeatherApplet *gw_applet);
#endif

void mateweather_applet_create (MateWeatherApplet *gw_applet)
{
    GtkActionGroup *action_group;
    gchar          *ui_path;
    AtkObject      *atk_obj;

    gw_applet->mateweather_pref.location = NULL;
    gw_applet->mateweather_pref.update_interval = 1800;
    gw_applet->mateweather_pref.update_enabled = TRUE;
    gw_applet->mateweather_pref.detailed = FALSE;
    gw_applet->mateweather_pref.radar_enabled = TRUE;
    gw_applet->mateweather_pref.temperature_unit = TEMP_UNIT_INVALID;
    gw_applet->mateweather_pref.speed_unit = SPEED_UNIT_INVALID;
    gw_applet->mateweather_pref.pressure_unit = PRESSURE_UNIT_INVALID;
    gw_applet->mateweather_pref.distance_unit = DISTANCE_UNIT_INVALID;
    
    mate_panel_applet_set_flags (gw_applet->applet, MATE_PANEL_APPLET_EXPAND_MINOR);

    mate_panel_applet_set_background_widget(gw_applet->applet,
                                       GTK_WIDGET(gw_applet->applet));

    g_set_application_name (_("Weather Report"));

    gtk_window_set_default_icon_name ("weather-storm");

    gw_applet->container = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (gw_applet->applet), gw_applet->container);

    g_signal_connect (G_OBJECT(gw_applet->applet), "change_orient",
                       G_CALLBACK(change_orient_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "size_allocate",
                       G_CALLBACK(size_allocate_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "destroy", 
                       G_CALLBACK (applet_destroy), gw_applet);
    g_signal_connect (GTK_OBJECT(gw_applet->applet), "button_press_event",
                       G_CALLBACK(clicked_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "key_press_event",           
			G_CALLBACK(key_press_cb), gw_applet);
                     
    gtk_widget_set_tooltip_text (GTK_WIDGET(gw_applet->applet), _("MATE Weather"));

    atk_obj = gtk_widget_get_accessible (GTK_WIDGET (gw_applet->applet));
    if (GTK_IS_ACCESSIBLE (atk_obj))
	   atk_object_set_name (atk_obj, _("MATE Weather"));

    gw_applet->size = mate_panel_applet_get_size (gw_applet->applet);

    gw_applet->orient = mate_panel_applet_get_orient (gw_applet->applet);

    action_group = gtk_action_group_new ("MateWeather Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
				  weather_applet_menu_actions,
				  G_N_ELEMENTS (weather_applet_menu_actions),
				  gw_applet);
    ui_path = g_build_filename (MATEWEATHER_MENU_UI_DIR, "mateweather-applet-menu.xml", NULL);
    mate_panel_applet_setup_menu_from_file (gw_applet->applet,
				       ui_path, action_group);
    g_free (ui_path);

    if (mate_panel_applet_get_locked_down (gw_applet->applet)) {
	    GtkAction *action;

	    action = gtk_action_group_get_action (action_group, "Props");
	    gtk_action_set_visible (action, FALSE);
    }
    g_object_unref (action_group);
	
    place_widgets(gw_applet);        

#ifdef HAVE_NETWORKMANAGER
    setup_network_monitor (gw_applet);     
#endif
}

gint timeout_cb (gpointer data)
{
    MateWeatherApplet *gw_applet = (MateWeatherApplet *)data;
	
    mateweather_update(gw_applet);
    return 0;  /* Do not repeat timeout (will be re-set by mateweather_update) */
}

static void
update_finish (WeatherInfo *info, gpointer data)
{
    static int gw_fault_counter = 0;
#ifdef HAVE_LIBMATENOTIFY
    char *message, *detail;
    MateConfClient *conf;
#endif
    char *s;
    MateWeatherApplet *gw_applet = (MateWeatherApplet *)data;
    gint nxtSunEvent;
    const gchar *icon_name;

    /* Update timer */
    if (gw_applet->timeout_tag > 0)
        g_source_remove(gw_applet->timeout_tag);
    if (gw_applet->mateweather_pref.update_enabled)
    {
	gw_applet->timeout_tag =
		g_timeout_add_seconds (
                       gw_applet->mateweather_pref.update_interval,
                        timeout_cb, gw_applet);

        nxtSunEvent = weather_info_next_sun_event(gw_applet->mateweather_info);
        if (nxtSunEvent >= 0)
            gw_applet->suncalc_timeout_tag =
                        g_timeout_add_seconds (nxtSunEvent,
                                suncalc_timeout_cb, gw_applet);
    }

    if ((TRUE == weather_info_is_valid (info)) ||
	     (gw_fault_counter >= MAX_CONSECUTIVE_FAULTS))
    {
	    gw_fault_counter = 0;
            icon_name = weather_info_get_icon_name (gw_applet->mateweather_info);
            gtk_image_set_from_icon_name (GTK_IMAGE(gw_applet->image), 
                                          icon_name, GTK_ICON_SIZE_BUTTON);
	      
	    gtk_label_set_text (GTK_LABEL (gw_applet->label), 
	        		weather_info_get_temp_summary(
					gw_applet->mateweather_info));
	    
	    s = weather_info_get_weather_summary (gw_applet->mateweather_info);
	    gtk_widget_set_tooltip_text (GTK_WIDGET (gw_applet->applet), s);
	    g_free (s);

	    /* Update dialog -- if one is present */
	    if (gw_applet->details_dialog) {
	    	mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
	    }

	    /* update applet */
	    place_widgets(gw_applet);

#ifdef HAVE_LIBMATENOTIFY
	    if (mate_panel_applet_mateconf_get_bool (gw_applet->applet,
				    "show_notifications", NULL))
	    {
		    NotifyNotification *n;
	            
		    /* Show notifications if possible */
	            if (!notify_is_initted ())
	                notify_init (_("Weather Forecast"));

		    if (notify_is_initted ())
		    {
			 GError *error = NULL;
                         const char *icon;
			 
	           	 /* Show notification */
	           	 message = g_strdup_printf ("%s: %s",
					 weather_info_get_location_name (info),
					 weather_info_get_sky (info));
	           	 detail = g_strdup_printf (
					 _("City: %s\nSky: %s\nTemperature: %s"),
					 weather_info_get_location_name (info),
					 weather_info_get_sky (info),
					 weather_info_get_temp_summary (info));

			 icon = weather_info_get_icon_name (gw_applet->mateweather_info);
			 if (icon == NULL)
				 icon = "stock-unknown";
	           	 
			 n = notify_notification_new (message, detail, icon,
					 gw_applet->container);
	
		   	 notify_notification_show (n, &error);
			 if (error)
			 {
				 g_warning ("%s", error->message);
				 g_error_free (error);
			 }
		   	     
		   	 g_free (message);
		   	 g_free (detail);
		    }
	    }
#endif
    }
    else
    {
	    /* there has been an error during retrival
	     * just update the fault counter
	     */
	    gw_fault_counter++;
    }
}

gint suncalc_timeout_cb (gpointer data)
{
    WeatherInfo *info = ((MateWeatherApplet *)data)->mateweather_info;
    update_finish(info, data);
    return 0;  /* Do not repeat timeout (will be re-set by update_finish) */
}


void mateweather_update (MateWeatherApplet *gw_applet)
{
    WeatherPrefs prefs;
    const gchar *icon_name;

    icon_name = weather_info_get_icon_name(gw_applet->mateweather_info);
    gtk_image_set_from_icon_name (GTK_IMAGE (gw_applet->image), 
    			          icon_name, GTK_ICON_SIZE_BUTTON); 
    gtk_widget_set_tooltip_text (GTK_WIDGET(gw_applet->applet),  _("Updating..."));

    /* Set preferred forecast type */
    prefs.type = gw_applet->mateweather_pref.detailed ? FORECAST_ZONE : FORECAST_STATE;

    /* Set radar map retrieval option */
    prefs.radar = gw_applet->mateweather_pref.radar_enabled;
    prefs.radar_custom_url = (gw_applet->mateweather_pref.use_custom_radar_url &&
    				gw_applet->mateweather_pref.radar) ?
				gw_applet->mateweather_pref.radar : NULL;

    /* Set the units */
    prefs.temperature_unit = gw_applet->mateweather_pref.temperature_unit;
    prefs.speed_unit = gw_applet->mateweather_pref.speed_unit;
    prefs.pressure_unit = gw_applet->mateweather_pref.pressure_unit;
    prefs.distance_unit = gw_applet->mateweather_pref.distance_unit;

    /* Update current conditions */
    if (gw_applet->mateweather_info && 
    	weather_location_equal(weather_info_get_location(gw_applet->mateweather_info),
    			       gw_applet->mateweather_pref.location)) {
	weather_info_update(gw_applet->mateweather_info, &prefs,
			    update_finish, gw_applet);
    } else {
        weather_info_free(gw_applet->mateweather_info);
        gw_applet->mateweather_info = weather_info_new(gw_applet->mateweather_pref.location,
						    &prefs,
						    update_finish, gw_applet);
    }
}

#ifdef HAVE_NETWORKMANAGER
static void
state_notify (DBusPendingCall *pending, gpointer data)
{
	MateWeatherApplet *gw_applet = data;

        DBusMessage *msg = dbus_pending_call_steal_reply (pending);

        if (!msg)
                return;

        if (dbus_message_get_type (msg) == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
                dbus_uint32_t result;

                if (dbus_message_get_args (msg, NULL,
                                           DBUS_TYPE_UINT32, &result,
                                           DBUS_TYPE_INVALID)) {
                        if (result == NM_STATE_CONNECTED) {
				/* thank you, glibc */
				res_init ();
				mateweather_update (gw_applet);
                        }
                }
        }

        dbus_message_unref (msg);
}

static void
check_network (DBusConnection *connection, gpointer user_data)
{
        DBusMessage *message;
        DBusPendingCall *reply;

        message = dbus_message_new_method_call (NM_DBUS_SERVICE,
                                                NM_DBUS_PATH,
                                                NM_DBUS_INTERFACE,
                                                "state");
        if (dbus_connection_send_with_reply (connection, message, &reply, -1)) {
                dbus_pending_call_set_notify (reply, state_notify, user_data, NULL);
                dbus_pending_call_unref (reply);
        }

        dbus_message_unref (message);
}

static DBusHandlerResult
filter_func (DBusConnection *connection, DBusMessage *message, void *user_data)
{
    if (dbus_message_is_signal (message, 
				NM_DBUS_INTERFACE, 
				"StateChanged")) {
	check_network (connection, user_data);

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
setup_network_monitor (MateWeatherApplet *gw_applet)
{
    GError *error;
    static DBusGConnection *bus = NULL;
    DBusConnection *dbus;

    if (bus == NULL) {
        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (bus == NULL) {
            g_warning ("Couldn't connect to system bus: %s",
                       error->message);
            g_error_free (error);

            return;
        }

        dbus = dbus_g_connection_get_connection (bus);	
        dbus_connection_add_filter (dbus, filter_func, gw_applet, NULL);
        dbus_bus_add_match (dbus,
                            "type='signal',"
                            "interface='" NM_DBUS_INTERFACE "'",
                            NULL);
    }
}	
#endif /* HAVE_NETWORKMANAGER */
