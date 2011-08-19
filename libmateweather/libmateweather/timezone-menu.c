/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* timezone-menu.c - Timezone-selecting menu
 *
 * Copyright 2008, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "timezone-menu.h"
#include "weather-priv.h"

#include <string.h>

/**
 * MateWeatherTimezoneMenu:
 *
 * A #GtkComboBox subclass for choosing a #MateWeatherTimezone
 **/

G_DEFINE_TYPE (MateWeatherTimezoneMenu, mateweather_timezone_menu, GTK_TYPE_COMBO_BOX)

enum {
    PROP_0,

    PROP_TOP,
    PROP_TZID,

    LAST_PROP
};

static void set_property (GObject *object, guint prop_id,
			  const GValue *value, GParamSpec *pspec);
static void get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec);

static void changed      (GtkComboBox *combo);

static GtkTreeModel *mateweather_timezone_model_new (MateWeatherLocation *top);
static gboolean row_separator_func (GtkTreeModel *model, GtkTreeIter *iter,
				    gpointer data);
static void is_sensitive (GtkCellLayout *cell_layout, GtkCellRenderer *cell,
			  GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);

static void
mateweather_timezone_menu_init (MateWeatherTimezoneMenu *menu)
{
    GtkCellRenderer *renderer;

    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (menu),
					  row_separator_func, NULL, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (menu), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (menu), renderer,
				    "markup", 0,
				    NULL);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (menu),
					renderer, is_sensitive, NULL, NULL);
}

static void
finalize (GObject *object)
{
    MateWeatherTimezoneMenu *menu = MATEWEATHER_TIMEZONE_MENU (object);

    if (menu->zone)
	mateweather_timezone_unref (menu->zone);

    G_OBJECT_CLASS (mateweather_timezone_menu_parent_class)->finalize (object);
}

static void
mateweather_timezone_menu_class_init (MateWeatherTimezoneMenuClass *timezone_menu_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (timezone_menu_class);
    GtkComboBoxClass *combo_class = GTK_COMBO_BOX_CLASS (timezone_menu_class);

    object_class->finalize = finalize;
    object_class->set_property = set_property;
    object_class->get_property = get_property;

    combo_class->changed = changed;

    /* properties */
    g_object_class_install_property (
	object_class, PROP_TOP,
	g_param_spec_pointer ("top",
			      "Top Location",
			      "The MateWeatherLocation whose children will be used to fill in the menu",
			      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (
	object_class, PROP_TZID,
	g_param_spec_string ("tzid",
			     "TZID",
			     "The selected TZID",
			     NULL,
			     G_PARAM_READWRITE));
}

static void
set_property (GObject *object, guint prop_id,
	      const GValue *value, GParamSpec *pspec)
{
    GtkTreeModel *model;

    switch (prop_id) {
    case PROP_TOP:
	model = mateweather_timezone_model_new (g_value_get_pointer (value));
	gtk_combo_box_set_model (GTK_COMBO_BOX (object), model);
	g_object_unref (model);
	gtk_combo_box_set_active (GTK_COMBO_BOX (object), 0);
	break;

    case PROP_TZID:
	mateweather_timezone_menu_set_tzid (MATEWEATHER_TIMEZONE_MENU (object),
					 g_value_get_string (value));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

static void
get_property (GObject *object, guint prop_id,
	      GValue *value, GParamSpec *pspec)
{
    MateWeatherTimezoneMenu *menu = MATEWEATHER_TIMEZONE_MENU (object);

    switch (prop_id) {
    case PROP_TZID:
	g_value_set_string (value, mateweather_timezone_menu_get_tzid (menu));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
    }
}

enum {
    MATEWEATHER_TIMEZONE_MENU_NAME,
    MATEWEATHER_TIMEZONE_MENU_ZONE
};

static void
changed (GtkComboBox *combo)
{
    MateWeatherTimezoneMenu *menu = MATEWEATHER_TIMEZONE_MENU (combo);
    GtkTreeIter iter;

    if (menu->zone)
	mateweather_timezone_unref (menu->zone);

    gtk_combo_box_get_active_iter (combo, &iter);
    gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter,
			MATEWEATHER_TIMEZONE_MENU_ZONE, &menu->zone,
			-1);

    if (menu->zone)
	mateweather_timezone_ref (menu->zone);

    g_object_notify (G_OBJECT (combo), "tzid");
}

static void
append_offset (GString *desc, int offset)
{
    int hours, minutes;

    hours = offset / 60;
    minutes = (offset > 0) ? offset % 60 : -offset % 60;

    if (minutes)
	g_string_append_printf (desc, "GMT%+d:%02d", hours, minutes);
    else if (hours)
	g_string_append_printf (desc, "GMT%+d", hours);
    else
	g_string_append (desc, "GMT");
}

static char *
get_offset (MateWeatherTimezone *zone)
{
    GString *desc;

    desc = g_string_new (NULL);
    append_offset (desc, mateweather_timezone_get_offset (zone));
    if (mateweather_timezone_has_dst (zone)) {
	g_string_append (desc, " / ");
	append_offset (desc, mateweather_timezone_get_dst_offset (zone));
    }
    return g_string_free (desc, FALSE);
}

static void
insert_location (GtkTreeStore *store, MateWeatherTimezone *zone, const char *loc_name, GtkTreeIter *parent)
{
    GtkTreeIter iter;
    char *name, *offset;

    offset = get_offset (zone);
    name = g_strdup_printf ("%s <small>(%s)</small>",
                            loc_name ? loc_name : mateweather_timezone_get_name (zone),
                            offset);
    gtk_tree_store_append (store, &iter, parent);
    gtk_tree_store_set (store, &iter,
                        MATEWEATHER_TIMEZONE_MENU_NAME, name,
                        MATEWEATHER_TIMEZONE_MENU_ZONE, mateweather_timezone_ref (zone),
                        -1);
    g_free (name);
    g_free (offset);
}

static void
insert_locations (GtkTreeStore *store, MateWeatherLocation *loc)
{
    int i;

    if (mateweather_location_get_level (loc) < MATEWEATHER_LOCATION_COUNTRY) {
	MateWeatherLocation **children;

	children = mateweather_location_get_children (loc);
	for (i = 0; children[i]; i++)
	    insert_locations (store, children[i]);
	mateweather_location_free_children (loc, children);
    } else {
	MateWeatherTimezone **zones;
	GtkTreeIter iter;

	zones = mateweather_location_get_timezones (loc);
	if (zones[1]) {
	    gtk_tree_store_append (store, &iter, NULL);
	    gtk_tree_store_set (store, &iter,
				MATEWEATHER_TIMEZONE_MENU_NAME, mateweather_location_get_name (loc),
				-1);

	    for (i = 0; zones[i]; i++) {
                insert_location (store, zones[i], NULL, &iter);
	    }
	} else if (zones[0]) {
            insert_location (store, zones[0], mateweather_location_get_name (loc), NULL);
	}

	mateweather_location_free_timezones (loc, zones);
    }
}

static GtkTreeModel *
mateweather_timezone_model_new (MateWeatherLocation *top)
{
    GtkTreeStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *unknown;
    MateWeatherTimezone *utc;

    store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    unknown = g_markup_printf_escaped ("<i>%s</i>", C_("timezone", "Unknown"));

    gtk_tree_store_append (store, &iter, NULL);
    gtk_tree_store_set (store, &iter,
			MATEWEATHER_TIMEZONE_MENU_NAME, unknown,
			MATEWEATHER_TIMEZONE_MENU_ZONE, NULL,
			-1);

    utc = mateweather_timezone_get_utc ();
    if (utc) {
        insert_location (store, utc, NULL, NULL);
        mateweather_timezone_unref (utc);
    }

    gtk_tree_store_append (store, &iter, NULL);

    g_free (unknown);

    insert_locations (store, top);

    return model;
}

static gboolean
row_separator_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    char *name;

    gtk_tree_model_get (model, iter,
			MATEWEATHER_TIMEZONE_MENU_NAME, &name,
			-1);
    if (name) {
	g_free (name);
	return FALSE;
    } else
	return TRUE;
}

static void
is_sensitive (GtkCellLayout *cell_layout, GtkCellRenderer *cell,
	      GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    gboolean sensitive;

    sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);
    g_object_set (cell, "sensitive", sensitive, NULL);
}

/**
 * mateweather_timezone_menu_new:
 * @top: the top-level location for the menu.
 *
 * Creates a new #MateWeatherTimezoneMenu.
 *
 * @top will normally be a location returned from
 * mateweather_location_new_world(), but you can create a menu that
 * contains the timezones from a smaller set of locations if you want.
 *
 * Return value: the new #MateWeatherTimezoneMenu
 **/
GtkWidget *
mateweather_timezone_menu_new (MateWeatherLocation *top)
{
    return g_object_new (MATEWEATHER_TYPE_TIMEZONE_MENU,
			 "top", top,
			 NULL);
}

typedef struct {
    GtkComboBox *combo;
    const char  *tzid;
} SetTimezoneData;

static gboolean
check_tzid (GtkTreeModel *model, GtkTreePath *path,
	    GtkTreeIter *iter, gpointer data)
{
    SetTimezoneData *tzd = data;
    MateWeatherTimezone *zone;

    gtk_tree_model_get (model, iter,
			MATEWEATHER_TIMEZONE_MENU_ZONE, &zone,
			-1);
    if (!zone)
	return FALSE;

    if (!strcmp (mateweather_timezone_get_tzid (zone), tzd->tzid)) {
	gtk_combo_box_set_active_iter (tzd->combo, iter);
	return TRUE;
    } else
	return FALSE;
}

/**
 * mateweather_timezone_menu_set_tzid:
 * @menu: a #MateWeatherTimezoneMenu
 * @tzid: (allow-none): a tzdata id (eg, "America/New_York")
 *
 * Sets @menu to the given @tzid. If @tzid is %NULL, sets @menu to
 * "Unknown".
 **/
void
mateweather_timezone_menu_set_tzid (MateWeatherTimezoneMenu *menu,
				 const char           *tzid)
{
    SetTimezoneData tzd;

    g_return_if_fail (MATEWEATHER_IS_TIMEZONE_MENU (menu));

    if (!tzid) {
	gtk_combo_box_set_active (GTK_COMBO_BOX (menu), 0);
	return;
    }

    tzd.combo = GTK_COMBO_BOX (menu);
    tzd.tzid = tzid;
    gtk_tree_model_foreach (gtk_combo_box_get_model (tzd.combo),
			    check_tzid, &tzd);
}

/**
 * mateweather_timezone_menu_get_tzid:
 * @menu: a #MateWeatherTimezoneMenu
 *
 * Gets @menu's timezone id.
 *
 * Return value: (allow-none): @menu's tzid, or %NULL if no timezone
 * is selected.
 **/
const char *
mateweather_timezone_menu_get_tzid (MateWeatherTimezoneMenu *menu)
{
    g_return_val_if_fail (MATEWEATHER_IS_TIMEZONE_MENU (menu), NULL);

    if (!menu->zone)
	return NULL;
    return mateweather_timezone_get_tzid (menu->zone);
}

