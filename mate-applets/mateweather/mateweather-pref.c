/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Preferences dialog
 *
 */

/* Radar map on by default. */
#define RADARMAP

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <locale.h>

#include <mate-panel-applet.h>
#include <mateconf/mateconf-client.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include <libmateweather/mateweather-xml.h>
#include "mateweather.h"
#include "mateweather-pref.h"
#include "mateweather-applet.h"
#include "mateweather-dialog.h"

#define NEVER_SENSITIVE		"never_sensitive"

struct _MateWeatherPrefPrivate {
	GtkWidget *basic_detailed_btn;
	GtkWidget *basic_temp_combo;
	GtkWidget *basic_speed_combo;
	GtkWidget *basic_dist_combo;
	GtkWidget *basic_pres_combo;
	GtkWidget *find_entry;
	GtkWidget *find_next_btn;
	
#ifdef RADARMAP
	GtkWidget *basic_radar_btn;
	GtkWidget *basic_radar_url_btn;
	GtkWidget *basic_radar_url_hbox;
	GtkWidget *basic_radar_url_entry;
#endif /* RADARMAP */
	GtkWidget *basic_update_spin;
	GtkWidget *basic_update_btn;
	GtkWidget *tree;

	GtkTreeModel *model;

	MateWeatherApplet *applet;
};

enum
{
	PROP_0,
	PROP_MATEWEATHER_APPLET,
};

G_DEFINE_TYPE (MateWeatherPref, mateweather_pref, GTK_TYPE_DIALOG);
#define MATEWEATHER_PREF_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATEWEATHER_TYPE_PREF, MateWeatherPrefPrivate))


/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	gtk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		gtk_widget_set_sensitive (w, FALSE);
	else
		gtk_widget_set_sensitive (w, sensitivity);
}


static gboolean
key_writable (MateWeatherPref *pref, const char *key)
{
        MateWeatherApplet *applet = pref->priv->applet;
	gboolean writable;
	char *fullkey;
	static MateConfClient *client = NULL;
	if (client == NULL)
		client = mateconf_client_get_default ();

	fullkey = mateweather_mateconf_get_full_key (applet->mateconf, key);

	writable = mateconf_client_key_is_writable (client, fullkey, NULL);

	g_free (fullkey);

	return writable;
}

/* sets up ATK Relation between the widgets */

static void
add_atk_relation (GtkWidget *widget1, GtkWidget *widget2, AtkRelationType type)
{
    AtkObject *atk_obj1, *atk_obj2;
    AtkRelationSet *relation_set;
    AtkRelation *relation;
   
    atk_obj1 = gtk_widget_get_accessible (widget1);
    if (! GTK_IS_ACCESSIBLE (atk_obj1))
       return;
    atk_obj2 = gtk_widget_get_accessible (widget2);

    relation_set = atk_object_ref_relation_set (atk_obj1);
    relation = atk_relation_new (&atk_obj2, 1, type);
    atk_relation_set_add (relation_set, relation);
    g_object_unref (G_OBJECT (relation));
}

/* sets accessible name and description */

void
set_access_namedesc (GtkWidget *widget, const gchar *name, const gchar *desc)
{
    AtkObject *obj;

    obj = gtk_widget_get_accessible (widget);
    if (! GTK_IS_ACCESSIBLE (obj))
       return;

    if ( desc )
       atk_object_set_description (obj, desc);
    if ( name )
       atk_object_set_name (obj, name);
}

/* sets accessible name, description, CONTROLLED_BY 
 * and CONTROLLER_FOR relations for the components
 * in mateweather preference dialog.
 */

static void mateweather_pref_set_accessibility (MateWeatherPref *pref)
{

    /* Relation between components in General page */

    add_atk_relation (pref->priv->basic_update_btn, pref->priv->basic_update_spin, ATK_RELATION_CONTROLLER_FOR);
    add_atk_relation (pref->priv->basic_radar_btn, pref->priv->basic_radar_url_btn, ATK_RELATION_CONTROLLER_FOR);
    add_atk_relation (pref->priv->basic_radar_btn, pref->priv->basic_radar_url_entry, ATK_RELATION_CONTROLLER_FOR);

    add_atk_relation (pref->priv->basic_update_spin, pref->priv->basic_update_btn, ATK_RELATION_CONTROLLED_BY);
    add_atk_relation (pref->priv->basic_radar_url_btn, pref->priv->basic_radar_btn, ATK_RELATION_CONTROLLED_BY);
    add_atk_relation (pref->priv->basic_radar_url_entry, pref->priv->basic_radar_btn, ATK_RELATION_CONTROLLED_BY);

    /* Accessible Name and Description for the components in Preference Dialog */
   
    set_access_namedesc (pref->priv->tree, _("Location view"), _("Select Location from the list"));
    set_access_namedesc (pref->priv->basic_update_spin, _("Update spin button"), _("Spinbutton for updating"));
    set_access_namedesc (pref->priv->basic_radar_url_entry, _("Address Entry"), _("Enter the URL"));

}


/* Update pref dialog from mateweather_pref */
static gboolean update_dialog (MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;

    g_return_val_if_fail(gw_applet->mateweather_pref.location != NULL, FALSE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pref->priv->basic_update_spin), 
    			      gw_applet->mateweather_pref.update_interval / 60);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref->priv->basic_update_btn), 
    				 gw_applet->mateweather_pref.update_enabled);
    soft_set_sensitive(pref->priv->basic_update_spin, 
    			     gw_applet->mateweather_pref.update_enabled);

    if ( gw_applet->mateweather_pref.use_temperature_default) {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_temp_combo), 0);
    } else {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_temp_combo), 
				  gw_applet->mateweather_pref.temperature_unit -1);
    }
	
    if ( gw_applet->mateweather_pref.use_speed_default) {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_speed_combo), 0);
    } else {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_speed_combo), 
				  gw_applet->mateweather_pref.speed_unit -1);
    }
	
    if ( gw_applet->mateweather_pref.use_pressure_default) {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_pres_combo), 0);
    } else {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_pres_combo), 
				  gw_applet->mateweather_pref.pressure_unit -1);
    }
    if ( gw_applet->mateweather_pref.use_distance_default) {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_dist_combo), 0);
    } else {
        gtk_combo_box_set_active (GTK_COMBO_BOX(pref->priv->basic_dist_combo), 
				  gw_applet->mateweather_pref.distance_unit -1);
    }

#if 0
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref->priv->basic_detailed_btn), 
    				 gw_applet->mateweather_pref.detailed);
#endif
#ifdef RADARMAP
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref->priv->basic_radar_btn),
    				 gw_applet->mateweather_pref.radar_enabled);
    				 
    soft_set_sensitive (pref->priv->basic_radar_url_btn, 
    			      gw_applet->mateweather_pref.radar_enabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref->priv->basic_radar_url_btn),
    				 gw_applet->mateweather_pref.use_custom_radar_url);
    soft_set_sensitive (pref->priv->basic_radar_url_hbox, 
    			      gw_applet->mateweather_pref.radar_enabled);
    if (gw_applet->mateweather_pref.radar)
    	gtk_entry_set_text (GTK_ENTRY (pref->priv->basic_radar_url_entry),
    			    gw_applet->mateweather_pref.radar);
#endif /* RADARMAP */
    
    
    return TRUE;
}

static void row_selected_cb (GtkTreeSelection *selection, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    GtkTreeModel *model;
    WeatherLocation *loc = NULL;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    	return;
    	
    gtk_tree_model_get (model, &iter, MATEWEATHER_XML_COL_POINTER, &loc, -1);
    
    if (!loc)
    	return;

    mateweather_mateconf_set_string(gw_applet->mateconf, "location1", loc->code, NULL);
    mateweather_mateconf_set_string(gw_applet->mateconf, "location2", loc->zone, NULL);
    mateweather_mateconf_set_string(gw_applet->mateconf, "location3", loc->radar, NULL);
    mateweather_mateconf_set_string(gw_applet->mateconf, "location4", loc->name, NULL);
    mateweather_mateconf_set_string(gw_applet->mateconf, "coordinates", loc->coordinates, NULL);
    
    if (gw_applet->mateweather_pref.location) {
       weather_location_free (gw_applet->mateweather_pref.location);
    }
    gw_applet->mateweather_pref.location = mateweather_mateconf_get_location (gw_applet->mateconf);
    
    mateweather_update (gw_applet);
} 

static gboolean
compare_location (GtkTreeModel *model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      user_data)
{
    MateWeatherPref *pref = user_data;
    WeatherLocation *loc;
    GtkTreeView *view;

    gtk_tree_model_get (model, iter, MATEWEATHER_XML_COL_POINTER, &loc, -1);
    if (!loc)
	return FALSE;

    if (!weather_location_equal (loc, pref->priv->applet->mateweather_pref.location))
	return FALSE;

    view = GTK_TREE_VIEW (pref->priv->tree);
    gtk_tree_view_expand_to_path (view, path);
    gtk_tree_view_set_cursor (view, path, NULL, FALSE);
    gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 0.5, 0.5);
    return TRUE;
}

static void load_locations (MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    GtkTreeView *tree = GTK_TREE_VIEW(pref->priv->tree);
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell_renderer;
    WeatherLocation *current_location;
        
    /* Add a colum for the locations */
    cell_renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("not used", cell_renderer,
						       "text", MATEWEATHER_XML_COL_LOC, NULL);
    gtk_tree_view_append_column (tree, column);
    gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree), column);
    
    /* load locations from xml file */
    pref->priv->model = mateweather_xml_load_locations ();
    if (!pref->priv->model)
    {
        GtkWidget *d;

        d = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                    _("Failed to load the Locations XML "
                                      "database.  Please report this as "
                                      "a bug."));
        gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
    }
    gtk_tree_view_set_model (tree, pref->priv->model);

    if (pref->priv->applet->mateweather_pref.location) {
	/* Select the current (default) location */
	gtk_tree_model_foreach (GTK_TREE_MODEL (pref->priv->model), compare_location, pref);
    }
}

static void
auto_update_toggled (GtkToggleButton *button, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    gboolean toggled;
    gint nxtSunEvent;

    toggled = gtk_toggle_button_get_active(button);
    gw_applet->mateweather_pref.update_enabled = toggled;
    soft_set_sensitive (pref->priv->basic_update_spin, toggled);
    mateweather_mateconf_set_bool(gw_applet->mateconf, "auto_update", toggled, NULL);
    if (gw_applet->timeout_tag > 0)
        g_source_remove(gw_applet->timeout_tag);
    if (gw_applet->suncalc_timeout_tag > 0)
        g_source_remove(gw_applet->suncalc_timeout_tag);
    if (gw_applet->mateweather_pref.update_enabled) {
        gw_applet->timeout_tag = g_timeout_add_seconds (
				gw_applet->mateweather_pref.update_interval,
				timeout_cb, gw_applet);
	nxtSunEvent = weather_info_next_sun_event(gw_applet->mateweather_info);
	if (nxtSunEvent >= 0)
	    gw_applet->suncalc_timeout_tag = g_timeout_add_seconds (
	    						nxtSunEvent,
							suncalc_timeout_cb,
							gw_applet);
    }
}

#if 0
static void
detailed_toggled (GtkToggleButton *button, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    gboolean toggled;
    
    toggled = gtk_toggle_button_get_active(button);
    gw_applet->mateweather_pref.detailed = toggled;
    mateweather_mateconf_set_bool(gw_applet->mateconf, "enable_detailed_forecast", 
    				toggled, NULL);    
}
#endif

static void temp_combo_changed_cb (GtkComboBox *combo, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    TempUnit       new_unit, old_unit;

    g_return_if_fail(gw_applet != NULL);

    new_unit = gtk_combo_box_get_active(combo) + 1;

    if (gw_applet->mateweather_pref.use_temperature_default)
        old_unit = TEMP_UNIT_DEFAULT;
    else
        old_unit = gw_applet->mateweather_pref.temperature_unit;	

    if (new_unit == old_unit)
        return;

    gw_applet->mateweather_pref.use_temperature_default = new_unit == TEMP_UNIT_DEFAULT;
    gw_applet->mateweather_pref.temperature_unit = new_unit;

    mateweather_mateconf_set_string(gw_applet->mateconf, MATECONF_TEMP_UNIT,
                              mateweather_prefs_temp_enum_to_string (new_unit),
                                  NULL);

    gtk_label_set_text(GTK_LABEL(gw_applet->label), 
                       weather_info_get_temp_summary(gw_applet->mateweather_info));

    if (gw_applet->details_dialog)
        mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
}

static void speed_combo_changed_cb (GtkComboBox *combo, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    SpeedUnit      new_unit, old_unit;

	g_return_if_fail(gw_applet != NULL);

    new_unit = gtk_combo_box_get_active(combo) + 1;

    if (gw_applet->mateweather_pref.use_speed_default)
        old_unit = SPEED_UNIT_DEFAULT;
    else
        old_unit = gw_applet->mateweather_pref.speed_unit;	

    if (new_unit == old_unit)
        return;

    gw_applet->mateweather_pref.use_speed_default = new_unit == SPEED_UNIT_DEFAULT;
    gw_applet->mateweather_pref.speed_unit = new_unit;

    mateweather_mateconf_set_string(gw_applet->mateconf, MATECONF_SPEED_UNIT,
                              mateweather_prefs_speed_enum_to_string (new_unit),
                                  NULL);

    if (gw_applet->details_dialog)
        mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
}

static void pres_combo_changed_cb (GtkComboBox *combo, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    PressureUnit   new_unit, old_unit;

    g_return_if_fail(gw_applet != NULL);

    new_unit = gtk_combo_box_get_active(combo) + 1;

    if (gw_applet->mateweather_pref.use_pressure_default)
        old_unit = PRESSURE_UNIT_DEFAULT;
    else
        old_unit = gw_applet->mateweather_pref.pressure_unit;	

    if (new_unit == old_unit)
        return;

    gw_applet->mateweather_pref.use_pressure_default = new_unit == PRESSURE_UNIT_DEFAULT;
    gw_applet->mateweather_pref.pressure_unit = new_unit;

    mateweather_mateconf_set_string(gw_applet->mateconf, MATECONF_PRESSURE_UNIT,
                              mateweather_prefs_pressure_enum_to_string (new_unit),
                                  NULL);

    if (gw_applet->details_dialog)
        mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
}

static void dist_combo_changed_cb (GtkComboBox *combo, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    DistanceUnit   new_unit, old_unit;

    g_return_if_fail(gw_applet != NULL);

    new_unit = gtk_combo_box_get_active(combo) + 1;

    if (gw_applet->mateweather_pref.use_distance_default)
        old_unit = DISTANCE_UNIT_DEFAULT;
    else
        old_unit = gw_applet->mateweather_pref.distance_unit;	

    if (new_unit == old_unit)
        return;

    gw_applet->mateweather_pref.use_distance_default = new_unit == DISTANCE_UNIT_DEFAULT;
    gw_applet->mateweather_pref.distance_unit = new_unit;

    mateweather_mateconf_set_string(gw_applet->mateconf, MATECONF_DISTANCE_UNIT,
                              mateweather_prefs_distance_enum_to_string (new_unit),
                                  NULL);

    if (gw_applet->details_dialog)
        mateweather_dialog_update (MATEWEATHER_DIALOG (gw_applet->details_dialog));
}

static void
radar_toggled (GtkToggleButton *button, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    gboolean toggled;
    
    toggled = gtk_toggle_button_get_active(button);
    gw_applet->mateweather_pref.radar_enabled = toggled;
    mateweather_mateconf_set_bool(gw_applet->mateconf, "enable_radar_map", toggled, NULL);
    soft_set_sensitive (pref->priv->basic_radar_url_btn, toggled);
    if (toggled == FALSE || gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (pref->priv->basic_radar_url_btn)) == TRUE)
            soft_set_sensitive (pref->priv->basic_radar_url_hbox, toggled);
}

static void
use_radar_url_toggled (GtkToggleButton *button, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    gboolean toggled;
    
    toggled = gtk_toggle_button_get_active(button);
    gw_applet->mateweather_pref.use_custom_radar_url = toggled;
    mateweather_mateconf_set_bool(gw_applet->mateconf, "use_custom_radar_url", toggled, NULL);
    soft_set_sensitive (pref->priv->basic_radar_url_hbox, toggled);
}

static gboolean
radar_url_changed (GtkWidget *widget, GdkEventFocus *event, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    gchar *text;
    
    text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
    
    if (gw_applet->mateweather_pref.radar)
        g_free (gw_applet->mateweather_pref.radar);
        
    if (text) {
    	gw_applet->mateweather_pref.radar = g_strdup (text);
    	g_free (text);
    }
    else
    	gw_applet->mateweather_pref.radar = NULL;
    	
    mateweather_mateconf_set_string (gw_applet->mateconf, "radar", 
    				   gw_applet->mateweather_pref.radar, NULL);
    				   
    return FALSE;
    				   
}

static void
update_interval_changed (GtkSpinButton *button, MateWeatherPref *pref)
{
    MateWeatherApplet *gw_applet = pref->priv->applet;
    
    gw_applet->mateweather_pref.update_interval = gtk_spin_button_get_value_as_int(button)*60;
    mateweather_mateconf_set_int(gw_applet->mateconf, "auto_update_interval", 
    		               gw_applet->mateweather_pref.update_interval, NULL);
    if (gw_applet->timeout_tag > 0)
        g_source_remove(gw_applet->timeout_tag);
    if (gw_applet->mateweather_pref.update_enabled)
        gw_applet->timeout_tag =  
        	g_timeout_add_seconds (gw_applet->mateweather_pref.update_interval,
                                 timeout_cb, gw_applet);
}

static gboolean
free_data (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
   WeatherLocation *location;
   
   gtk_tree_model_get (model, iter, MATEWEATHER_XML_COL_POINTER, &location, -1);
   if (!location)
   	return FALSE;

   weather_location_free (location);
   
   return FALSE;
}


static GtkWidget *
create_hig_catagory (GtkWidget *main_box, gchar *title)
{
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *label;
	gchar *tmp;
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);

	tmp = g_strdup_printf ("<b>%s</b>", title);
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

	return vbox2;		
}

static gboolean
find_location (GtkTreeModel *model, GtkTreeIter *iter, const gchar *location, gboolean go_parent)
{
	GtkTreeIter iter_child;
	GtkTreeIter iter_parent;
	gchar *aux_loc;
	gboolean valid;
	int len;

	len = strlen (location);

	if (len <= 0) {
		return FALSE;
	}
	
	do {
		
		gtk_tree_model_get (model, iter, MATEWEATHER_XML_COL_LOC, &aux_loc, -1);

		if (g_ascii_strncasecmp (aux_loc, location, len) == 0) {
			g_free (aux_loc);
			return TRUE;
		}

		if (gtk_tree_model_iter_has_child (model, iter)) {
			gtk_tree_model_iter_nth_child (model, &iter_child, iter, 0);

			if (find_location (model, &iter_child, location, FALSE)) {
				/* Manual copying of the iter */
				iter->stamp = iter_child.stamp;
				iter->user_data = iter_child.user_data;
				iter->user_data2 = iter_child.user_data2;
				iter->user_data3 = iter_child.user_data3;

				g_free (aux_loc);
				
				return TRUE;
			}
		}
		g_free (aux_loc);

		valid = gtk_tree_model_iter_next (model, iter);		
	} while (valid);

	if (go_parent) {
		iter_parent = *iter;
                while (gtk_tree_model_iter_parent (model, iter, &iter_parent)) {
			if (gtk_tree_model_iter_next (model, iter))
				return find_location (model, iter, location, TRUE);
			iter_parent = *iter;
		}
	}

	return FALSE;
}

static void
find_next_clicked (GtkButton *button, MateWeatherPref *pref)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkEntry *entry;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeIter iter_parent;
	GtkTreePath *path;
	const gchar *location;

	tree = GTK_TREE_VIEW (pref->priv->tree);
	model = gtk_tree_view_get_model (tree);
	entry = GTK_ENTRY (pref->priv->find_entry);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	if (gtk_tree_selection_count_selected_rows (selection) >= 1) {
		gtk_tree_selection_get_selected (selection, &model, &iter);
		/* Select next or select parent */
		if (!gtk_tree_model_iter_next (model, &iter)) {
			iter_parent = iter;
			if (!gtk_tree_model_iter_parent (model, &iter, &iter_parent) || !gtk_tree_model_iter_next (model, &iter))
				gtk_tree_model_get_iter_first (model, &iter);
		}

	} else {
		gtk_tree_model_get_iter_first (model, &iter);
	}

	location = gtk_entry_get_text (entry);

	if (find_location (model, &iter, location, TRUE)) {
		gtk_widget_set_sensitive (pref->priv->find_next_btn, TRUE);

		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_expand_to_path (tree, path);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0);

		gtk_tree_path_free (path);
	} else {
		gtk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);
	}
}

static void
find_entry_changed (GtkEditable *entry, MateWeatherPref *pref)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	const gchar *location;

	tree = GTK_TREE_VIEW (pref->priv->tree);
	model = gtk_tree_view_get_model (tree);

	g_return_if_fail (model != NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_model_get_iter_first (model, &iter);

	location = gtk_entry_get_text (GTK_ENTRY (entry));

	if (find_location (model, &iter, location, TRUE)) {
		gtk_widget_set_sensitive (pref->priv->find_next_btn, TRUE);

		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_expand_to_path (tree, path);
		gtk_tree_selection_select_iter (selection, &iter);
		gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0);

		gtk_tree_path_free (path);
	} else {
		gtk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);
	}
}


static void help_cb (GtkDialog *dialog)
{
    GError *error = NULL;

    gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
		"ghelp:mateweather?mateweather-settings",
		gtk_get_current_event_time (),
		&error);

    if (error) { 
	GtkWidget *error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							  _("There was an error displaying help: %s"), error->message);
	g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
	gtk_window_set_screen (GTK_WINDOW (error_dialog), gtk_widget_get_screen (GTK_WIDGET (dialog)));
	gtk_widget_show (error_dialog);
        g_error_free (error);
        error = NULL;
    }
}


static void
response_cb (GtkDialog *dialog, gint id, MateWeatherPref *pref)
{
    if (id == GTK_RESPONSE_HELP) {
        help_cb (dialog);
    } else {
        gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}


static void
mateweather_pref_create (MateWeatherPref *pref)
{
    GtkWidget *pref_vbox;
    GtkWidget *pref_notebook;
#ifdef RADARMAP
    GtkWidget *radar_toggle_hbox;
#endif /* RADARMAP */
    GtkWidget *pref_basic_update_alignment;
    GtkWidget *pref_basic_update_lbl;
    GtkWidget *pref_basic_update_hbox;
    GtkObject *pref_basic_update_spin_adj;
    GtkWidget *pref_basic_update_sec_lbl;
    GtkWidget *pref_basic_note_lbl;
    GtkWidget *pref_loc_hbox;
    GtkWidget *pref_loc_note_lbl;
    GtkWidget *scrolled_window;
    GtkWidget *label, *value_hbox, *tree_label;
    GtkTreeSelection *selection;
    GtkWidget *pref_basic_vbox;
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *temp_label;
    GtkWidget *temp_combo;
    GtkWidget *speed_label;
    GtkWidget *speed_combo;	
    GtkWidget *pres_label;
    GtkWidget *pres_combo;
    GtkWidget *dist_label;
    GtkWidget *dist_combo;
    GtkWidget *unit_table;	
    GtkWidget *pref_find_label;
    GtkWidget *pref_find_hbox;
    GtkWidget *image;


    g_object_set (pref, "destroy-with-parent", TRUE, NULL);
    gtk_window_set_title (GTK_WINDOW (pref), _("Weather Preferences"));
    gtk_dialog_add_buttons (GTK_DIALOG (pref),
			    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			    GTK_STOCK_HELP, GTK_RESPONSE_HELP,
			    NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (pref), GTK_RESPONSE_CLOSE);
    gtk_dialog_set_has_separator (GTK_DIALOG (pref), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (pref), 5);
    gtk_window_set_resizable (GTK_WINDOW (pref), TRUE);
    gtk_window_set_screen (GTK_WINDOW (pref),
			   gtk_widget_get_screen (GTK_WIDGET (pref->priv->applet->applet)));

    pref_vbox = gtk_dialog_get_content_area (GTK_DIALOG (pref));
    gtk_box_set_spacing (GTK_BOX (pref_vbox), 2);
    gtk_widget_show (pref_vbox);

    pref_notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (pref_notebook), 5);
    gtk_widget_show (pref_notebook);
    gtk_box_pack_start (GTK_BOX (pref_vbox), pref_notebook, TRUE, TRUE, 0);

  /*
   * General settings page.
   */

    pref_basic_vbox = gtk_vbox_new (FALSE, 18);
    gtk_container_set_border_width (GTK_CONTAINER (pref_basic_vbox), 12);
    gtk_container_add (GTK_CONTAINER (pref_notebook), pref_basic_vbox);

    pref_basic_update_alignment = gtk_alignment_new (0, 0.5, 0, 1);
    gtk_widget_show (pref_basic_update_alignment);

    pref->priv->basic_update_btn = gtk_check_button_new_with_mnemonic (_("_Automatically update every:"));
    gtk_widget_show (pref->priv->basic_update_btn);
    gtk_container_add (GTK_CONTAINER (pref_basic_update_alignment), pref->priv->basic_update_btn);
    g_signal_connect (G_OBJECT (pref->priv->basic_update_btn), "toggled",
    		      G_CALLBACK (auto_update_toggled), pref);
    if ( ! key_writable (pref, "auto_update"))
	    hard_set_sensitive (pref->priv->basic_update_btn, FALSE);

  /*
   * Units settings page.
   */

    /* Temperature Unit */
    temp_label = gtk_label_new_with_mnemonic (_("_Temperature unit:"));
    gtk_label_set_use_markup (GTK_LABEL (temp_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (temp_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (temp_label), 0, 0.5);
    gtk_widget_show (temp_label);

    temp_combo = gtk_combo_box_new_text ();
	pref->priv->basic_temp_combo = temp_combo;
    gtk_label_set_mnemonic_widget (GTK_LABEL (temp_label), temp_combo);
    gtk_combo_box_append_text (GTK_COMBO_BOX (temp_combo), _("Default"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (temp_combo), _("Kelvin"));
    /* TRANSLATORS: Celsius is sometimes referred Centigrade */
    gtk_combo_box_append_text (GTK_COMBO_BOX (temp_combo), _("Celsius"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (temp_combo), _("Fahrenheit"));
	gtk_widget_show (temp_combo);
		
    if ( ! key_writable (pref, MATECONF_TEMP_UNIT))
        hard_set_sensitive (pref->priv->basic_temp_combo, FALSE);
	
    /* Speed Unit */
    speed_label = gtk_label_new_with_mnemonic (_("_Wind speed unit:"));
    gtk_label_set_use_markup (GTK_LABEL (speed_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (speed_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (speed_label), 0, 0.5);
    gtk_widget_show (speed_label);

    speed_combo = gtk_combo_box_new_text ();
    pref->priv->basic_speed_combo = speed_combo;
    gtk_label_set_mnemonic_widget (GTK_LABEL (speed_label), speed_combo);
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo), _("Default"));
    /* TRANSLATOR: The wind speed unit "meters per second" */    
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo), _("m/s"));
    /* TRANSLATOR: The wind speed unit "kilometers per hour" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo), _("km/h"));
    /* TRANSLATOR: The wind speed unit "miles per hour" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo), _("mph"));
    /* TRANSLATOR: The wind speed unit "knots" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo), _("knots"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (speed_combo),
		    _("Beaufort scale"));
	gtk_widget_show (speed_combo);

    if ( ! key_writable (pref, MATECONF_SPEED_UNIT))
        hard_set_sensitive (pref->priv->basic_speed_combo, FALSE);

    /* Pressure Unit */
    pres_label = gtk_label_new_with_mnemonic (_("_Pressure unit:"));
    gtk_label_set_use_markup (GTK_LABEL (pres_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (pres_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (pres_label), 0, 0.5);
    gtk_widget_show (pres_label);

    pres_combo = gtk_combo_box_new_text ();
	pref->priv->basic_pres_combo = pres_combo;
    gtk_label_set_mnemonic_widget (GTK_LABEL (pres_label), pres_combo);
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("Default"));
    /* TRANSLATOR: The pressure unit "kiloPascals" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("kPa"));
    /* TRANSLATOR: The pressure unit "hectoPascals" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("hPa"));
    /* TRANSLATOR: The pressure unit "millibars" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("mb"));
    /* TRANSLATOR: The pressure unit "millibars of mercury" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("mmHg"));
    /* TRANSLATOR: The pressure unit "inches of mercury" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("inHg"));
    /* TRANSLATOR: The pressure unit "atmospheres" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (pres_combo), _("atm"));
    gtk_widget_show (pres_combo);

    if ( ! key_writable (pref, MATECONF_PRESSURE_UNIT))
        hard_set_sensitive (pref->priv->basic_pres_combo, FALSE);

    /* Distance Unit */
    dist_label = gtk_label_new_with_mnemonic (_("_Visibility unit:"));
    gtk_label_set_use_markup (GTK_LABEL (dist_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (dist_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (dist_label), 0, 0.5);
    gtk_widget_show (dist_label);

    dist_combo = gtk_combo_box_new_text ();
	pref->priv->basic_dist_combo = dist_combo;
    gtk_label_set_mnemonic_widget (GTK_LABEL (dist_label), dist_combo);
    gtk_combo_box_append_text (GTK_COMBO_BOX (dist_combo), _("Default"));
    /* TRANSLATOR: The distance unit "meters" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (dist_combo), _("meters"));
    /* TRANSLATOR: The distance unit "kilometers" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (dist_combo), _("km"));
    /* TRANSLATOR: The distance unit "miles" */
    gtk_combo_box_append_text (GTK_COMBO_BOX (dist_combo), _("miles"));
	gtk_widget_show (dist_combo);

    if ( ! key_writable (pref, MATECONF_DISTANCE_UNIT))
        hard_set_sensitive (pref->priv->basic_dist_combo, FALSE);
	
	unit_table = gtk_table_new(5, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(unit_table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(unit_table), 12);
	gtk_table_attach(GTK_TABLE(unit_table), temp_label, 0, 1, 0, 1,
	                 (GtkAttachOptions) (GTK_FILL),
	                 (GtkAttachOptions) (0), 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(unit_table), temp_combo,  1, 2, 0, 1);
	gtk_table_attach(GTK_TABLE(unit_table), speed_label, 0, 1, 1, 2,
	                 (GtkAttachOptions) (GTK_FILL),
	                 (GtkAttachOptions) (0), 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(unit_table), speed_combo, 1, 2, 1, 2);
	gtk_table_attach(GTK_TABLE(unit_table), pres_label, 0, 1, 2, 3,
	                 (GtkAttachOptions) (GTK_FILL),
	                 (GtkAttachOptions) (0), 0, 0);	
	gtk_table_attach_defaults(GTK_TABLE(unit_table), pres_combo,  1, 2, 2, 3);
	gtk_table_attach(GTK_TABLE(unit_table), dist_label, 0, 1, 3, 4,
	                 (GtkAttachOptions) (GTK_FILL),
	                 (GtkAttachOptions) (0), 0, 0);	
	gtk_table_attach_defaults(GTK_TABLE(unit_table), dist_combo,  1, 2, 3, 4);
	gtk_widget_show(unit_table);
	
	g_signal_connect (temp_combo, "changed", G_CALLBACK (temp_combo_changed_cb), pref);
	g_signal_connect (speed_combo, "changed", G_CALLBACK (speed_combo_changed_cb), pref);
	g_signal_connect (dist_combo, "changed", G_CALLBACK (dist_combo_changed_cb), pref);
	g_signal_connect (pres_combo, "changed", G_CALLBACK (pres_combo_changed_cb), pref);

	
#ifdef RADARMAP
    pref->priv->basic_radar_btn = gtk_check_button_new_with_mnemonic (_("Enable _radar map"));
    gtk_widget_show (pref->priv->basic_radar_btn);
    g_signal_connect (G_OBJECT (pref->priv->basic_radar_btn), "toggled",
    		      G_CALLBACK (radar_toggled), pref);
    if ( ! key_writable (pref, "enable_radar_map"))
	    hard_set_sensitive (pref->priv->basic_radar_btn, FALSE);
    
    radar_toggle_hbox = gtk_hbox_new (FALSE, 12);
    gtk_widget_show (radar_toggle_hbox);
    
    label = gtk_label_new ("    ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (radar_toggle_hbox), label, FALSE, FALSE, 0); 
					      
    pref->priv->basic_radar_url_btn = gtk_check_button_new_with_mnemonic (_("Use _custom address for radar map"));
    gtk_widget_show (pref->priv->basic_radar_url_btn);
    gtk_box_pack_start (GTK_BOX (radar_toggle_hbox), pref->priv->basic_radar_url_btn, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (pref->priv->basic_radar_url_btn), "toggled",
    		      G_CALLBACK (use_radar_url_toggled), pref);
    if ( ! key_writable (pref, "use_custom_radar_url"))
	    hard_set_sensitive (pref->priv->basic_radar_url_btn, FALSE);
    		      
    pref->priv->basic_radar_url_hbox = gtk_hbox_new (FALSE, 12);
    gtk_widget_show (pref->priv->basic_radar_url_hbox);

    label = gtk_label_new ("    ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (pref->priv->basic_radar_url_hbox),
    			label, FALSE, FALSE, 0); 
			
    label = gtk_label_new_with_mnemonic (_("A_ddress:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (pref->priv->basic_radar_url_hbox),
    			label, FALSE, FALSE, 0); 
    pref->priv->basic_radar_url_entry = gtk_entry_new ();
    gtk_widget_show (pref->priv->basic_radar_url_entry);
    gtk_box_pack_start (GTK_BOX (pref->priv->basic_radar_url_hbox),
    			pref->priv->basic_radar_url_entry, TRUE, TRUE, 0);    
    g_signal_connect (G_OBJECT (pref->priv->basic_radar_url_entry), "focus_out_event",
    		      G_CALLBACK (radar_url_changed), pref);
    if ( ! key_writable (pref, "radar"))
	    hard_set_sensitive (pref->priv->basic_radar_url_entry, FALSE);
#endif /* RADARMAP */

    frame = create_hig_catagory (pref_basic_vbox, _("Update"));

    pref_basic_update_hbox = gtk_hbox_new (FALSE, 12);

    pref_basic_update_lbl = gtk_label_new_with_mnemonic (_("_Automatically update every:"));
    gtk_widget_show (pref_basic_update_lbl);
    

/*
    gtk_label_set_justify (GTK_LABEL (pref_basic_update_lbl), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment (GTK_MISC (pref_basic_update_lbl), 1, 0.5);
*/

    gtk_widget_show (pref_basic_update_hbox);

    pref_basic_update_spin_adj = gtk_adjustment_new (30, 1, 3600, 5, 25, 1);
    pref->priv->basic_update_spin = gtk_spin_button_new (GTK_ADJUSTMENT (pref_basic_update_spin_adj), 1, 0);
    gtk_widget_show (pref->priv->basic_update_spin);

    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (pref->priv->basic_update_spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (pref->priv->basic_update_spin), GTK_UPDATE_IF_VALID);
    g_signal_connect (G_OBJECT (pref->priv->basic_update_spin), "value_changed",
    		      G_CALLBACK (update_interval_changed), pref);
    
    pref_basic_update_sec_lbl = gtk_label_new (_("minutes"));
    gtk_widget_show (pref_basic_update_sec_lbl);
    if ( ! key_writable (pref, "auto_update_interval")) {
	    hard_set_sensitive (pref->priv->basic_update_spin, FALSE);
	    hard_set_sensitive (pref_basic_update_sec_lbl, FALSE);
    }

    value_hbox = gtk_hbox_new (FALSE, 6);

    gtk_box_pack_start (GTK_BOX (pref_basic_update_hbox), pref_basic_update_alignment, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (pref_basic_update_hbox), value_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (value_hbox), pref->priv->basic_update_spin, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (value_hbox), pref_basic_update_sec_lbl, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frame), pref_basic_update_hbox);

    frame = create_hig_catagory (pref_basic_vbox, _("Display"));

    vbox = gtk_vbox_new (FALSE, 6);

	gtk_box_pack_start (GTK_BOX (vbox), unit_table, TRUE, TRUE, 0);

#ifdef RADARMAP
    gtk_box_pack_start (GTK_BOX (vbox), pref->priv->basic_radar_btn, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radar_toggle_hbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), pref->priv->basic_radar_url_hbox, TRUE, 
    			TRUE, 0);
#endif /* RADARMAP */

    gtk_container_add (GTK_CONTAINER (frame), vbox);

    pref_basic_note_lbl = gtk_label_new (_("General"));
    gtk_widget_show (pref_basic_note_lbl);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (pref_notebook), 
    				gtk_notebook_get_nth_page (GTK_NOTEBOOK (pref_notebook), 0),
    				pref_basic_note_lbl);

  /*
   * Location page.
   */
    pref_loc_hbox = gtk_vbox_new (FALSE, 6);
    gtk_container_set_border_width (GTK_CONTAINER (pref_loc_hbox), 12);
    gtk_container_add (GTK_CONTAINER (pref_notebook), pref_loc_hbox);

    tree_label = gtk_label_new_with_mnemonic (_("_Select a location:"));
    gtk_misc_set_alignment (GTK_MISC (tree_label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (pref_loc_hbox), tree_label, FALSE, FALSE, 0);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);

    pref->priv->tree = gtk_tree_view_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (tree_label), GTK_WIDGET (pref->priv->tree));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (pref->priv->tree), FALSE);
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pref->priv->tree));
    g_signal_connect (G_OBJECT (selection), "changed",
    		      G_CALLBACK (row_selected_cb), pref);
    
    gtk_container_add (GTK_CONTAINER (scrolled_window), pref->priv->tree);
    gtk_widget_show (pref->priv->tree);
    gtk_widget_show (scrolled_window);
    gtk_box_pack_start (GTK_BOX (pref_loc_hbox), scrolled_window, TRUE, TRUE, 0);
    load_locations(pref);

    pref_find_hbox = gtk_hbox_new (FALSE, 6);
    pref_find_label = gtk_label_new (_("_Find:"));
    gtk_label_set_use_underline (GTK_LABEL (pref_find_label), TRUE);

    pref->priv->find_entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (pref_find_label),
		    pref->priv->find_entry);
    
    pref->priv->find_next_btn = gtk_button_new_with_mnemonic (_("Find _Next"));
    gtk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);
    
    image = gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON); 
    gtk_button_set_image (GTK_BUTTON (pref->priv->find_next_btn), image);

    g_signal_connect (G_OBJECT (pref->priv->find_next_btn), "clicked",
		      G_CALLBACK (find_next_clicked), pref);
    g_signal_connect (G_OBJECT (pref->priv->find_entry), "changed",
		      G_CALLBACK (find_entry_changed), pref);

    gtk_container_set_border_width (GTK_CONTAINER (pref_find_hbox), 0);
    gtk_box_pack_start (GTK_BOX (pref_find_hbox), pref_find_label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (pref_find_hbox), pref->priv->find_entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (pref_find_hbox), pref->priv->find_next_btn, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (pref_loc_hbox), pref_find_hbox, FALSE, FALSE, 0);
    
    if ( ! key_writable (pref, "location0")) {
	    hard_set_sensitive (scrolled_window, FALSE);
    }

    pref_loc_note_lbl = gtk_label_new (_("Location"));
    gtk_widget_show (pref_loc_note_lbl);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (pref_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (pref_notebook), 1), pref_loc_note_lbl);


    g_signal_connect (G_OBJECT (pref), "response",
    		      G_CALLBACK (response_cb), pref);
   
    mateweather_pref_set_accessibility (pref); 
    gtk_label_set_mnemonic_widget (GTK_LABEL (pref_basic_update_sec_lbl), pref->priv->basic_update_spin);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), pref->priv->basic_radar_url_entry);
}


static void
mateweather_pref_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    MateWeatherPref *pref = MATEWEATHER_PREF (object);

    switch (prop_id) {
	case PROP_MATEWEATHER_APPLET:
	    pref->priv->applet = g_value_get_pointer (value);
	    break;
    }
}


static void
mateweather_pref_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    MateWeatherPref *pref = MATEWEATHER_PREF (object);

    switch (prop_id) {
	case PROP_MATEWEATHER_APPLET:
	    g_value_set_pointer (value, pref->priv->applet);
	    break;
    }
}


static void
mateweather_pref_init (MateWeatherPref *self)
{
    self->priv = MATEWEATHER_PREF_GET_PRIVATE (self);
}


static GObject *
mateweather_pref_constructor (GType type,
			   guint n_construct_params,
			   GObjectConstructParam *construct_params)
{
    GObject *object;
    MateWeatherPref *self;

    object = G_OBJECT_CLASS (mateweather_pref_parent_class)->
        constructor (type, n_construct_params, construct_params);
    self = MATEWEATHER_PREF (object);

    mateweather_pref_create (self);
    update_dialog (self);

    return object;
}


GtkWidget *
mateweather_pref_new (MateWeatherApplet *applet)
{
    return g_object_new (MATEWEATHER_TYPE_PREF,
			 "mateweather-applet", applet,
			 NULL);
}


static void
mateweather_pref_finalize (GObject *object)
{
   MateWeatherPref *self = MATEWEATHER_PREF (object);

   gtk_tree_model_foreach (self->priv->model, free_data, NULL);
   g_object_unref (G_OBJECT (self->priv->model));

   G_OBJECT_CLASS (mateweather_pref_parent_class)->finalize(object);
}


static void
mateweather_pref_class_init (MateWeatherPrefClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    mateweather_pref_parent_class = g_type_class_peek_parent (klass);

    object_class->set_property = mateweather_pref_set_property;
    object_class->get_property = mateweather_pref_get_property;
    object_class->constructor = mateweather_pref_constructor;
    object_class->finalize = mateweather_pref_finalize;

    /* This becomes an OBJECT property when MateWeatherApplet is redone */
    g_object_class_install_property (object_class,
				     PROP_MATEWEATHER_APPLET,
				     g_param_spec_pointer ("mateweather-applet",
							   "MateWeather Applet",
							   "The MateWeather Applet",
							   G_PARAM_READWRITE |
							   G_PARAM_CONSTRUCT_ONLY));

    g_type_class_add_private (klass, sizeof (MateWeatherPrefPrivate));
}


