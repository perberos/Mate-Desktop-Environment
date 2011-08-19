/*
 * MATE CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@mate.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Authors : Carlos García Campos <carlosgc@mate.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>

#include "cpufreq-prefs.h"
#include "cpufreq-utils.h"

enum {
	PROP_0,
	PROP_MATECONF_KEY,
	PROP_CPU,
	PROP_SHOW_MODE,
	PROP_SHOW_TEXT_MODE,
};

struct _CPUFreqPrefsPrivate {
	MateConfClient        *mateconf_client;
	gchar              *mateconf_key;
	
	guint               cpu;
	CPUFreqShowMode     show_mode;
	CPUFreqShowTextMode show_text_mode;

	/* Preferences dialog */
	GtkWidget *dialog;
	GtkWidget *show_freq;
	GtkWidget *show_unit;
	GtkWidget *show_perc;
	GtkWidget *cpu_combo;
	GtkWidget *monitor_settings_box;
	GtkWidget *show_mode_combo;
};

#define CPUFREQ_PREFS_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), CPUFREQ_TYPE_PREFS, CPUFreqPrefsPrivate))

static void cpufreq_prefs_init                      (CPUFreqPrefs      *prefs);
static void cpufreq_prefs_class_init                (CPUFreqPrefsClass *klass);
static void cpufreq_prefs_finalize                  (GObject           *object);

static void cpufreq_prefs_set_property              (GObject           *object,
						     guint              prop_id,
						     const GValue      *value,
						     GParamSpec        *pspec);
static void cpufreq_prefs_get_property              (GObject           *object,
						     guint              prop_id,
						     GValue            *value,
						     GParamSpec        *pspec);

static void cpufreq_prefs_dialog_update_sensitivity (CPUFreqPrefs      *prefs);


G_DEFINE_TYPE (CPUFreqPrefs, cpufreq_prefs, G_TYPE_OBJECT)

static void
cpufreq_prefs_init (CPUFreqPrefs *prefs)
{
	prefs->priv = CPUFREQ_PREFS_GET_PRIVATE (prefs);

	prefs->priv->mateconf_client = mateconf_client_get_default ();
	prefs->priv->mateconf_key = NULL;

	prefs->priv->cpu = 0;
}

static void
cpufreq_prefs_class_init (CPUFreqPrefsClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = cpufreq_prefs_set_property;
	g_object_class->get_property = cpufreq_prefs_get_property;
	
	g_type_class_add_private (g_object_class, sizeof (CPUFreqPrefsPrivate));

	/* Properties */
	g_object_class_install_property (g_object_class,
					 PROP_MATECONF_KEY,
					 g_param_spec_string ("mateconf-key",
							      "MateConfKey",
							      "The applet mateconf key",
							      NULL,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (g_object_class,
					 PROP_CPU,
					 g_param_spec_uint ("cpu",
							    "CPU",
							    "The monitored cpu",
							    0,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));
	g_object_class_install_property (g_object_class,
					 PROP_SHOW_MODE,
					 g_param_spec_enum ("show-mode",
							    "ShowMode",
							    "The applet show mode",
							    CPUFREQ_TYPE_SHOW_MODE,
							    CPUFREQ_MODE_BOTH,
							    G_PARAM_READWRITE));
	g_object_class_install_property (g_object_class,
					 PROP_SHOW_TEXT_MODE,
					 g_param_spec_enum ("show-text-mode",
							    "ShowTextMode",
							    "The applet show text mode",
							    CPUFREQ_TYPE_SHOW_TEXT_MODE,
							    CPUFREQ_MODE_TEXT_FREQUENCY_UNIT,
							    G_PARAM_READWRITE));

	g_object_class->finalize = cpufreq_prefs_finalize;
}

static void
cpufreq_prefs_finalize (GObject *object)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);

	if (prefs->priv->mateconf_client) {
		g_object_unref (prefs->priv->mateconf_client);
		prefs->priv->mateconf_client = NULL;
	}

	if (prefs->priv->mateconf_key) {
		g_free (prefs->priv->mateconf_key);
		prefs->priv->mateconf_key = NULL;
	}

	if (prefs->priv->dialog) {
		gtk_widget_destroy (prefs->priv->dialog);
		prefs->priv->dialog = NULL;
	}

	G_OBJECT_CLASS (cpufreq_prefs_parent_class)->finalize (object);
}

static void
cpufreq_prefs_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);
	gboolean      update_sensitivity = FALSE;

	switch (prop_id) {
	case PROP_MATECONF_KEY:
		prefs->priv->mateconf_key = g_value_dup_string (value);
		break;
	case PROP_CPU: {
		guint cpu;

		cpu = g_value_get_uint (value);
		if (prefs->priv->cpu != cpu) {
			gchar *key;
			
			prefs->priv->cpu = cpu;
			key = g_strjoin ("/",
					 prefs->priv->mateconf_key,
					 "cpu",
					 NULL);
			mateconf_client_set_int (prefs->priv->mateconf_client,
					      key, prefs->priv->cpu,
					      NULL);
			g_free (key);
		}
	}
		break;
	case PROP_SHOW_MODE: {
		CPUFreqShowMode mode;

		mode = g_value_get_enum (value);
		if (prefs->priv->show_mode != mode) {
			gchar *key;

			update_sensitivity = TRUE;
			prefs->priv->show_mode = mode;
			key = g_strjoin ("/",
					 prefs->priv->mateconf_key,
					 "show_mode",
					 NULL);
			mateconf_client_set_int (prefs->priv->mateconf_client,
					      key, prefs->priv->show_mode,
					      NULL);
			g_free (key);
		}
	}
		break;
	case PROP_SHOW_TEXT_MODE: {
		CPUFreqShowTextMode mode;

		mode = g_value_get_enum (value);
		if (prefs->priv->show_text_mode != mode) {
			gchar *key;

			update_sensitivity = TRUE;
			prefs->priv->show_text_mode = mode;
			key = g_strjoin ("/",
					 prefs->priv->mateconf_key,
					 "show_text_mode",
					 NULL);
			mateconf_client_set_int (prefs->priv->mateconf_client,
					      key, prefs->priv->show_text_mode,
					      NULL);
			g_free (key);
		}
	}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}

	if (prefs->priv->dialog && update_sensitivity)
		cpufreq_prefs_dialog_update_sensitivity (prefs);
}

static void
cpufreq_prefs_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);

	switch (prop_id) {
	case PROP_MATECONF_KEY:
		/* Is not readable */
		break;
	case PROP_CPU:
		g_value_set_uint (value, prefs->priv->cpu);
		break;
	case PROP_SHOW_MODE:
		g_value_set_enum (value, prefs->priv->show_mode);
		break;
	case PROP_SHOW_TEXT_MODE:
		g_value_set_enum (value, prefs->priv->show_text_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
cpufreq_prefs_setup (CPUFreqPrefs *prefs)
{
	guint                cpu;
	CPUFreqShowMode      show_mode;
	CPUFreqShowTextMode  show_text_mode;
	gchar               *key;
	GError              *error = NULL;

	g_assert (MATECONF_IS_CLIENT (prefs->priv->mateconf_client));
	g_assert (prefs->priv->mateconf_key != NULL);

	key = g_strjoin ("/", prefs->priv->mateconf_key, "cpu", NULL);
	cpu = mateconf_client_get_int (prefs->priv->mateconf_client,
				    key, &error);
	g_free (key);
	/* In case anything went wrong with mateconf, get back to the default */
	if (error) {
		g_warning ("%s", error->message);
		cpu = 0;
		g_error_free (error);
		error = NULL;
	}
	prefs->priv->cpu = cpu;

	key = g_strjoin ("/", prefs->priv->mateconf_key, "show_mode", NULL);
	show_mode = mateconf_client_get_int (prefs->priv->mateconf_client,
					  key, &error);
	g_free (key);
	/* In case anything went wrong with mateconf, get back to the default */
	if (error ||
	    show_mode < CPUFREQ_MODE_GRAPHIC ||
	    show_mode > CPUFREQ_MODE_BOTH) {
		show_mode = CPUFREQ_MODE_BOTH;
		if (error) {
			g_warning ("%s", error->message);
			g_error_free (error);
			error = NULL;
		}
	}
	prefs->priv->show_mode = show_mode;

	key = g_strjoin ("/", prefs->priv->mateconf_key, "show_text_mode", NULL);
	show_text_mode = mateconf_client_get_int (prefs->priv->mateconf_client,
					       key, &error);
	g_free (key);
	/* In case anything went wrong with mateconf, get back to the default */
	if (error ||
	    show_text_mode < CPUFREQ_MODE_TEXT_FREQUENCY ||
	    show_text_mode > CPUFREQ_MODE_TEXT_PERCENTAGE) {
		show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;
		if (error) {
			g_warning ("%s", error->message);
			g_error_free (error);
			error = NULL;
		}
	}
	prefs->priv->show_text_mode = show_text_mode;
}

CPUFreqPrefs *
cpufreq_prefs_new (const gchar *mateconf_key)
{
	CPUFreqPrefs *prefs;

	g_return_val_if_fail (mateconf_key != NULL, NULL);

	prefs = CPUFREQ_PREFS (g_object_new (CPUFREQ_TYPE_PREFS,
					     "mateconf-key", mateconf_key,
					     NULL));
	
	cpufreq_prefs_setup (prefs);

	return prefs;
}

/* Public Methods */
guint
cpufreq_prefs_get_cpu (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs), 0);
	
	return MIN (prefs->priv->cpu, cpufreq_utils_get_n_cpus () - 1);
}

CPUFreqShowMode
cpufreq_prefs_get_show_mode (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs),
			      CPUFREQ_MODE_BOTH);

	return prefs->priv->show_mode;
}

CPUFreqShowTextMode
cpufreq_prefs_get_show_text_mode (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs),
			      CPUFREQ_MODE_TEXT_FREQUENCY_UNIT);

	return prefs->priv->show_text_mode;
}

/* Preferences Dialog */
static gboolean
cpufreq_prefs_key_is_writable (CPUFreqPrefs *prefs, const gchar *key)
{
        gboolean  writable;
        gchar    *fullkey;

	g_assert (prefs->priv->mateconf_client != NULL);

	fullkey = g_strjoin ("/", prefs->priv->mateconf_key, key, NULL);
        writable = mateconf_client_key_is_writable (prefs->priv->mateconf_client,
						 fullkey, NULL);
        g_free (fullkey);

        return writable;
}

static void
cpufreq_prefs_dialog_show_freq_toggled (GtkWidget *show_freq, CPUFreqPrefs *prefs)
{
	CPUFreqShowTextMode show_text_mode;
           
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_freq))) {
		GtkWidget *show_unit = prefs->priv->show_unit;

                if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_unit)))
                        show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;
                else
                        show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY;

		g_object_set (G_OBJECT (prefs),
			      "show-text-mode", show_text_mode,
			      NULL);
	}
}	

static void
cpufreq_prefs_dialog_show_unit_toggled (GtkWidget *show_unit, CPUFreqPrefs *prefs)
{
	CPUFreqShowTextMode show_text_mode;
           
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_unit))) {
                show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;
        } else {
                show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY;
        }

	g_object_set (G_OBJECT (prefs),
		      "show-text-mode", show_text_mode,
		      NULL);
}

static void
cpufreq_prefs_dialog_show_perc_toggled (GtkWidget *show_perc, CPUFreqPrefs *prefs)
{

	CPUFreqShowTextMode show_text_mode;
	
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_perc))) {
                /* Show cpu usage in percentage */
                show_text_mode = CPUFREQ_MODE_TEXT_PERCENTAGE;

		g_object_set (G_OBJECT (prefs),
			      "show-text-mode", show_text_mode,
			      NULL);
        }
}

static void
cpufreq_prefs_dialog_cpu_number_changed (GtkWidget *cpu_combo, CPUFreqPrefs *prefs)
{
        gint cpu;
	
        cpu = gtk_combo_box_get_active (GTK_COMBO_BOX (prefs->priv->cpu_combo));

        if (cpu >= 0) {
		g_object_set (G_OBJECT (prefs),
			      "cpu", cpu,
			      NULL);
        }
}

static void
cpufreq_prefs_dialog_show_mode_changed (GtkWidget *show_mode_combo, CPUFreqPrefs *prefs)
{
	CPUFreqShowMode show_mode;
           
        show_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (show_mode_combo));
	g_object_set (G_OBJECT (prefs),
		      "show-mode", show_mode,
		      NULL);
}

static void
cpufreq_prefs_dialog_response_cb (CPUFreqPrefs *prefs,
				  gint          response,
				  GtkDialog    *dialog)
{
        GError *error = NULL;

        if (response == GTK_RESPONSE_HELP) {
		gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (prefs->priv->dialog)),
			"ghelp:cpufreq-applet?cpufreq-applet-prefs",
			gtk_get_current_event_time (),
			&error);

                if (error) {
                        cpufreq_utils_display_error (_("Could not open help document"),
						     error->message);
                        g_error_free (error);
		}
        } else {
                gtk_widget_destroy (prefs->priv->dialog);
                prefs->priv->dialog = NULL;
        }
}

static void
cpufreq_prefs_dialog_update_visibility (CPUFreqPrefs *prefs)
{
	if (cpufreq_utils_get_n_cpus () > 1)
		gtk_widget_show (prefs->priv->monitor_settings_box);
	else
		gtk_widget_hide (prefs->priv->monitor_settings_box);
}

static void
cpufreq_prefs_dialog_update_sensitivity (CPUFreqPrefs *prefs)
{
	gtk_widget_set_sensitive (prefs->priv->show_mode_combo,
				  cpufreq_prefs_key_is_writable (prefs, "show_mode"));
	
	if (prefs->priv->show_mode != CPUFREQ_MODE_GRAPHIC) {
		gboolean key_writable;
		
		key_writable = cpufreq_prefs_key_is_writable (prefs, "show_text_mode");
		
		gtk_widget_set_sensitive (prefs->priv->show_freq,
					  (TRUE && key_writable));
		gtk_widget_set_sensitive (prefs->priv->show_perc,
					  (TRUE && key_writable));
		
		if (prefs->priv->show_text_mode == CPUFREQ_MODE_TEXT_PERCENTAGE)
			gtk_widget_set_sensitive (prefs->priv->show_unit,
						  FALSE);
		else
			gtk_widget_set_sensitive (prefs->priv->show_unit,
						  (TRUE && key_writable));
	} else {
		gtk_widget_set_sensitive (prefs->priv->show_freq, FALSE);
		gtk_widget_set_sensitive (prefs->priv->show_unit, FALSE);
		gtk_widget_set_sensitive (prefs->priv->show_perc, FALSE);
	}
}

static void
cpufreq_prefs_dialog_update (CPUFreqPrefs *prefs)
{
	if (cpufreq_utils_get_n_cpus () > 1) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (prefs->priv->cpu_combo),
					  MIN (prefs->priv->cpu, cpufreq_utils_get_n_cpus () - 1));
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (prefs->priv->show_mode_combo),
				  prefs->priv->show_mode);
	
	switch (prefs->priv->show_text_mode) {
	case CPUFREQ_MODE_TEXT_FREQUENCY:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->priv->show_freq),
					      TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->priv->show_unit),
					      FALSE);

		break;
	case CPUFREQ_MODE_TEXT_FREQUENCY_UNIT:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->priv->show_freq),
					      TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->priv->show_unit),
					      TRUE);

		break;
	case CPUFREQ_MODE_TEXT_PERCENTAGE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->priv->show_perc),
					      TRUE);

		break;
	}
}

static void
cpufreq_prefs_dialog_cpu_combo_setup (CPUFreqPrefs *prefs)
{
	GtkListStore    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	guint            i;
	guint            n_cpus;

	model = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (prefs->priv->cpu_combo),
				 GTK_TREE_MODEL (model));

	n_cpus = cpufreq_utils_get_n_cpus ();
	
	for (i = 0; i < n_cpus; i++) {
		gchar *text_label;
		
		text_label = g_strdup_printf ("CPU %u", i);

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    0, text_label,
				    -1);

		g_free (text_label);
	}

	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (prefs->priv->cpu_combo));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (prefs->priv->cpu_combo),
				    renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (prefs->priv->cpu_combo),
					renderer,
					"text", 0,
					NULL);
}

static void
cpufreq_prefs_dialog_show_mode_combo_setup (CPUFreqPrefs *prefs)
{
	GtkListStore    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;

	model = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (prefs->priv->show_mode_combo),
				 GTK_TREE_MODEL (model));

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    0, _("Graphic"),
			    -1);

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    0, _("Text"),
			    -1);

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    0, _("Graphic and Text"),
			    -1);

	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (prefs->priv->show_mode_combo));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (prefs->priv->show_mode_combo),
				    renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (prefs->priv->show_mode_combo),
					renderer,
					"text", 0,
					NULL);
}

static void
cpufreq_prefs_dialog_create (CPUFreqPrefs *prefs)
{
	GtkBuilder *builder;

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, GTK_BUILDERDIR "/cpufreq-preferences.ui", NULL);

	prefs->priv->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_dialog"));

	prefs->priv->cpu_combo = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_cpu_number"));
	
	prefs->priv->show_mode_combo = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_show_mode"));
	
	prefs->priv->show_freq = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_show_freq"));
	prefs->priv->show_unit = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_show_unit"));
	prefs->priv->show_perc = GTK_WIDGET (gtk_builder_get_object (builder, "prefs_show_perc"));

	prefs->priv->monitor_settings_box = GTK_WIDGET (gtk_builder_get_object (builder, "monitor_settings_box"));

	g_object_unref (builder);

	cpufreq_prefs_dialog_show_mode_combo_setup (prefs);
	
	if (cpufreq_utils_get_n_cpus () > 1)
		cpufreq_prefs_dialog_cpu_combo_setup (prefs);
		
	g_signal_connect_swapped (G_OBJECT (prefs->priv->dialog), "response",
				  G_CALLBACK (cpufreq_prefs_dialog_response_cb),
				  (gpointer) prefs);
	
	g_signal_connect (G_OBJECT (prefs->priv->show_freq), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_freq_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_unit), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_unit_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_perc), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_perc_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->cpu_combo), "changed",
			  G_CALLBACK (cpufreq_prefs_dialog_cpu_number_changed),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_mode_combo), "changed",
			  G_CALLBACK (cpufreq_prefs_dialog_show_mode_changed),
			  (gpointer) prefs);
}

void 
cpufreq_preferences_dialog_run (CPUFreqPrefs *prefs, GdkScreen *screen)
{
        g_return_if_fail (CPUFREQ_IS_PREFS (prefs));

        if (prefs->priv->dialog) {
                /* Dialog already exist, only show it */
                gtk_window_present (GTK_WINDOW (prefs->priv->dialog));
                return;
        }

	cpufreq_prefs_dialog_create (prefs);
        gtk_window_set_screen (GTK_WINDOW (prefs->priv->dialog), screen);

	cpufreq_prefs_dialog_update_sensitivity (prefs);
	cpufreq_prefs_dialog_update_visibility (prefs);
	cpufreq_prefs_dialog_update (prefs);

	gtk_widget_show (prefs->priv->dialog);
}
