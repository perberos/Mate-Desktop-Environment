/* Eye Of Mate - EOG Preferences Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@mate.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eog-preferences-dialog.h"
#include "eog-plugin-manager.h"
#include "eog-util.h"
#include "eog-config-keys.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#define EOG_PREFERENCES_DIALOG_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_PREFERENCES_DIALOG, EogPreferencesDialogPrivate))

G_DEFINE_TYPE (EogPreferencesDialog, eog_preferences_dialog, EOG_TYPE_DIALOG);

enum {
        PROP_0,
        PROP_MATECONF_CLIENT,
};

#define MATECONF_OBJECT_KEY	"MATECONF_KEY"
#define MATECONF_OBJECT_VALUE	"MATECONF_VALUE"
#define TOGGLE_INVERT_VALUE	"TOGGLE_INVERT_VALUE"

struct _EogPreferencesDialogPrivate {
	MateConfClient   *client;
};

static GObject *instance = NULL;

static void
pd_check_toggle_cb (GtkWidget *widget, gpointer data)
{
	char *key = NULL;
	gboolean invert = FALSE;
	gboolean value;

	key = g_object_get_data (G_OBJECT (widget), MATECONF_OBJECT_KEY);
	invert = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), TOGGLE_INVERT_VALUE));

	value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	if (key == NULL) return;

	mateconf_client_set_bool (MATECONF_CLIENT (data),
			       key,
			       (invert) ? !value : value,
			       NULL);
}

static void
pd_spin_button_changed_cb (GtkWidget *widget, gpointer data)
{
	char *key = NULL;

	key = g_object_get_data (G_OBJECT (widget), MATECONF_OBJECT_KEY);

	if (key == NULL) return;

	mateconf_client_set_int (MATECONF_CLIENT (data),
			      key,
			      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget)),
			      NULL);
}

static void
pd_color_change_cb (GtkColorButton *button, gpointer data)
{
	GdkColor color;
	char *key = NULL;
	char *value = NULL;

	gtk_color_button_get_color (button, &color);

	value = g_strdup_printf ("#%02X%02X%02X",
				 color.red / 256,
				 color.green / 256,
				 color.blue / 256);

	key = g_object_get_data (G_OBJECT (button), MATECONF_OBJECT_KEY);

	if (key == NULL || value == NULL)
		return;

	mateconf_client_set_string (MATECONF_CLIENT (data),
				 key,
				 value,
				 NULL);
	g_free (value);
}

static void
pd_radio_toggle_cb (GtkWidget *widget, gpointer data)
{
	char *key = NULL;
	char *value = NULL;

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
	    return;

	key = g_object_get_data (G_OBJECT (widget), MATECONF_OBJECT_KEY);
	value = g_object_get_data (G_OBJECT (widget), MATECONF_OBJECT_VALUE);

	if (key == NULL || value == NULL)
		return;

	mateconf_client_set_string (MATECONF_CLIENT (data),
				 key,
				 value,
				 NULL);
}

static void
eog_preferences_response_cb (GtkDialog *dlg, gint res_id, gpointer data)
{
	switch (res_id) {
		case GTK_RESPONSE_HELP:
			eog_util_show_help ("eog-prefs", NULL);
			break;
		default:
			gtk_widget_destroy (GTK_WIDGET (dlg));
			instance = NULL;
	}
}

static void
eog_preferences_dialog_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
	EogPreferencesDialog *pref_dlg = EOG_PREFERENCES_DIALOG (object);

	switch (prop_id) {
		case PROP_MATECONF_CLIENT:
			pref_dlg->priv->client = g_value_get_object (value);
			break;
	}
}

static void
eog_preferences_dialog_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	EogPreferencesDialog *pref_dlg = EOG_PREFERENCES_DIALOG (object);

	switch (prop_id) {
		case PROP_MATECONF_CLIENT:
			g_value_set_object (value, pref_dlg->priv->client);
			break;
	}
}

static GObject *
eog_preferences_dialog_constructor (GType type,
				    guint n_construct_properties,
				    GObjectConstructParam *construct_params)

{
	EogPreferencesDialogPrivate *priv;
	GtkWidget *dlg;
	GtkWidget *interpolate_check;
	GtkWidget *extrapolate_check;
	GtkWidget *autorotate_check;
	GtkWidget *bg_color_check;
	GtkWidget *bg_color_button;
	GtkWidget *color_radio;
	GtkWidget *checkpattern_radio;
	GtkWidget *background_radio;
	GtkWidget *color_button;
	GtkWidget *upscale_check;
	GtkWidget *loop_check;
	GtkWidget *seconds_spin;
	GtkWidget *plugin_manager;
	GtkWidget *plugin_manager_container;
	GObject *object;
	GdkColor color;
	gchar *value;

	object = G_OBJECT_CLASS (eog_preferences_dialog_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	priv = EOG_PREFERENCES_DIALOG (object)->priv;

	eog_dialog_construct (EOG_DIALOG (object),
			      "eog-preferences-dialog.ui",
			      "eog_preferences_dialog");

	eog_dialog_get_controls (EOG_DIALOG (object),
			         "eog_preferences_dialog", &dlg,
			         "interpolate_check", &interpolate_check,
			         "extrapolate_check", &extrapolate_check,
			         "autorotate_check", &autorotate_check,
				 "bg_color_check", &bg_color_check,
				 "bg_color_button", &bg_color_button,
			         "color_radio", &color_radio,
			         "checkpattern_radio", &checkpattern_radio,
			         "background_radio", &background_radio,
			         "color_button", &color_button,
			         "upscale_check", &upscale_check,
			         "loop_check", &loop_check,
			         "seconds_spin", &seconds_spin,
			         "plugin_manager_container", &plugin_manager_container,
			         NULL);

	g_signal_connect (G_OBJECT (dlg),
			  "response",
			  G_CALLBACK (eog_preferences_response_cb),
			  dlg);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interpolate_check),
				      mateconf_client_get_bool (priv->client,
							     EOG_CONF_VIEW_INTERPOLATE,
							     NULL));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (extrapolate_check),
				      mateconf_client_get_bool (priv->client,
							     EOG_CONF_VIEW_EXTRAPOLATE,
							     NULL));

	g_object_set_data (G_OBJECT (interpolate_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_INTERPOLATE);

	g_object_set_data (G_OBJECT (extrapolate_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_EXTRAPOLATE);

	g_signal_connect (G_OBJECT (interpolate_check),
			  "toggled",
			  G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	g_signal_connect (G_OBJECT (extrapolate_check),
			  "toggled",
			  G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (autorotate_check),
				      mateconf_client_get_bool (priv->client,
							     EOG_CONF_VIEW_AUTOROTATE,
							     NULL));

	g_object_set_data (G_OBJECT (autorotate_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_AUTOROTATE);

	g_signal_connect (G_OBJECT (autorotate_check),
			  "toggled",
			  G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bg_color_check),
				      mateconf_client_get_bool (priv->client,
				      			     EOG_CONF_VIEW_USE_BG_COLOR, NULL));
	g_object_set_data (G_OBJECT (bg_color_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_USE_BG_COLOR);
	g_signal_connect (G_OBJECT (bg_color_check),
			  "toggled", G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	value = mateconf_client_get_string (priv->client,
					 EOG_CONF_VIEW_BACKGROUND_COLOR,
					 NULL);
	if (gdk_color_parse (value, &color)){
		gtk_color_button_set_color (GTK_COLOR_BUTTON (bg_color_button),
					    &color);
	}
	g_free (value);

	g_object_set_data (G_OBJECT (bg_color_button),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_BACKGROUND_COLOR);

	g_signal_connect (G_OBJECT (bg_color_button),
			  "color-set",
			  G_CALLBACK (pd_color_change_cb),
			  priv->client);

	

	g_object_set_data (G_OBJECT (color_radio),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_TRANSPARENCY);

	g_object_set_data (G_OBJECT (color_radio),
			   MATECONF_OBJECT_VALUE,
			   "COLOR");

	g_signal_connect (G_OBJECT (color_radio),
			  "toggled",
			  G_CALLBACK (pd_radio_toggle_cb),
			  priv->client);

	g_object_set_data (G_OBJECT (checkpattern_radio),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_TRANSPARENCY);

	g_object_set_data (G_OBJECT (checkpattern_radio),
			   MATECONF_OBJECT_VALUE,
			   "CHECK_PATTERN");

	g_signal_connect (G_OBJECT (checkpattern_radio),
			  "toggled",
			  G_CALLBACK (pd_radio_toggle_cb),
			  priv->client);

	g_object_set_data (G_OBJECT (background_radio),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_TRANSPARENCY);

	g_object_set_data (G_OBJECT (background_radio),
			   MATECONF_OBJECT_VALUE,
			   "NONE");

	g_signal_connect (G_OBJECT (background_radio),
			  "toggled",
			  G_CALLBACK (pd_radio_toggle_cb),
			  priv->client);

	value = mateconf_client_get_string (priv->client,
					 EOG_CONF_VIEW_TRANSPARENCY,
					 NULL);

	if (g_ascii_strcasecmp (value, "COLOR") == 0) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (color_radio), TRUE);
	}
	else if (g_ascii_strcasecmp (value, "CHECK_PATTERN") == 0) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkpattern_radio), TRUE);
	}
	else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (background_radio), TRUE);
	}

	g_free (value);

	value = mateconf_client_get_string (priv->client,
					 EOG_CONF_VIEW_TRANS_COLOR,
					 NULL);

	if (gdk_color_parse (value, &color)) {
		gtk_color_button_set_color (GTK_COLOR_BUTTON (color_button),
					    &color);
	}

	g_object_set_data (G_OBJECT (color_button),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_VIEW_TRANS_COLOR);

	g_signal_connect (G_OBJECT (color_button),
			  "color-set",
			  G_CALLBACK (pd_color_change_cb),
			  priv->client);

	g_free (value);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (upscale_check),
				      mateconf_client_get_bool (priv->client,
							     EOG_CONF_FULLSCREEN_UPSCALE,
							     NULL));

	g_object_set_data (G_OBJECT (upscale_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_FULLSCREEN_UPSCALE);

	g_signal_connect (G_OBJECT (upscale_check),
			  "toggled",
			  G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (loop_check),
				      mateconf_client_get_bool (priv->client,
							     EOG_CONF_FULLSCREEN_LOOP,
							     NULL));

	g_object_set_data (G_OBJECT (loop_check),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_FULLSCREEN_LOOP);

	g_signal_connect (G_OBJECT (loop_check),
			  "toggled",
			  G_CALLBACK (pd_check_toggle_cb),
			  priv->client);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (seconds_spin),
				   mateconf_client_get_int (priv->client,
							 EOG_CONF_FULLSCREEN_SECONDS,
							 NULL));

	g_object_set_data (G_OBJECT (seconds_spin),
			   MATECONF_OBJECT_KEY,
			   EOG_CONF_FULLSCREEN_SECONDS);

	g_signal_connect (G_OBJECT (seconds_spin),
			  "value-changed",
			  G_CALLBACK (pd_spin_button_changed_cb),
			  priv->client);

        plugin_manager = eog_plugin_manager_new ();

        g_assert (plugin_manager != NULL);

        gtk_box_pack_start (GTK_BOX (plugin_manager_container),
                            plugin_manager,
                            TRUE,
                            TRUE,
                            0);

        gtk_widget_show_all (plugin_manager);

	return object;
}

static void
eog_preferences_dialog_class_init (EogPreferencesDialogClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->constructor = eog_preferences_dialog_constructor;
	g_object_class->set_property = eog_preferences_dialog_set_property;
	g_object_class->get_property = eog_preferences_dialog_get_property;

	g_object_class_install_property (g_object_class,
					 PROP_MATECONF_CLIENT,
					 g_param_spec_object ("mateconf-client",
							      "MateConf Client",
							      "MateConf Client",
							      MATECONF_TYPE_CLIENT,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_NAME |
							      G_PARAM_STATIC_NICK |
							      G_PARAM_STATIC_BLURB));

	g_type_class_add_private (g_object_class, sizeof (EogPreferencesDialogPrivate));
}

static void
eog_preferences_dialog_init (EogPreferencesDialog *pref_dlg)
{
	pref_dlg->priv = EOG_PREFERENCES_DIALOG_GET_PRIVATE (pref_dlg);

	pref_dlg->priv->client = NULL;
}

GObject *
eog_preferences_dialog_get_instance (GtkWindow *parent, MateConfClient *client)
{
	if (instance == NULL) {
		instance = g_object_new (EOG_TYPE_PREFERENCES_DIALOG,
				 	 "parent-window", parent,
				 	 "mateconf-client", client,
				 	 NULL);
	}

	return instance;
}
