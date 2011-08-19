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

#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <string.h>

#include "cpufreq-popup.h"
#include "cpufreq-selector.h"
#include "cpufreq-utils.h"

struct _CPUFreqPopupPrivate {
	GtkUIManager        *ui_manager;
	GSList              *radio_group;
	
	GtkActionGroup      *freqs_group;
	GSList              *freqs_actions;
	
	GtkActionGroup      *govs_group;
	GSList              *govs_actions;

	guint                merge_id;
	gboolean             need_build;
	gboolean             show_freqs;

	CPUFreqMonitor      *monitor;
	GtkWidget           *parent;
};

#define CPUFREQ_POPUP_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), CPUFREQ_TYPE_POPUP, CPUFreqPopupPrivate))

static void cpufreq_popup_init       (CPUFreqPopup      *popup);
static void cpufreq_popup_class_init (CPUFreqPopupClass *klass);
static void cpufreq_popup_finalize   (GObject           *object);

G_DEFINE_TYPE (CPUFreqPopup, cpufreq_popup, G_TYPE_OBJECT)

static const gchar *ui_popup =
"<ui>"
"    <popup name=\"CPUFreqSelectorPopup\" action=\"PopupAction\">"
"        <placeholder name=\"FreqsItemsGroup\">"
"        </placeholder>"
"        <separator />"
"        <placeholder name=\"GovsItemsGroup\">"
"        </placeholder>"
"    </popup>"
"</ui>";

#define FREQS_PLACEHOLDER_PATH "/CPUFreqSelectorPopup/FreqsItemsGroup"
#define GOVS_PLACEHOLDER_PATH "/CPUFreqSelectorPopup/GovsItemsGroup"

static void
cpufreq_popup_init (CPUFreqPopup *popup)
{
	popup->priv = CPUFREQ_POPUP_GET_PRIVATE (popup);

	popup->priv->ui_manager = gtk_ui_manager_new ();
	popup->priv->radio_group = NULL;

	popup->priv->freqs_group = NULL;
	popup->priv->freqs_actions = NULL;

	popup->priv->govs_group = NULL;
	popup->priv->govs_actions = NULL;

	popup->priv->merge_id = 0;
	popup->priv->need_build = TRUE;
	popup->priv->show_freqs = FALSE;

	gtk_ui_manager_add_ui_from_string (popup->priv->ui_manager,
					   ui_popup, -1, NULL);
	
	popup->priv->monitor = NULL;
}

static void
cpufreq_popup_class_init (CPUFreqPopupClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (g_object_class, sizeof (CPUFreqPopupPrivate));

	g_object_class->finalize = cpufreq_popup_finalize;
}

static void
cpufreq_popup_finalize (GObject *object)
{
	CPUFreqPopup *popup = CPUFREQ_POPUP (object);

	if (popup->priv->ui_manager) {
		g_object_unref (popup->priv->ui_manager);
		popup->priv->ui_manager = NULL;
	}

	if (popup->priv->freqs_group) {
		g_object_unref (popup->priv->freqs_group);
		popup->priv->freqs_group = NULL;
	}

	if (popup->priv->freqs_actions) {
		g_slist_free (popup->priv->freqs_actions);
		popup->priv->freqs_actions = NULL;
	}

	if (popup->priv->govs_group) {
		g_object_unref (popup->priv->govs_group);
		popup->priv->govs_group = NULL;
	}

	if (popup->priv->govs_actions) {
		g_slist_free (popup->priv->govs_actions);
		popup->priv->govs_actions = NULL;
	}
	
	if (popup->priv->monitor) {
		g_object_unref (popup->priv->monitor);
		popup->priv->monitor = NULL;
	}
	
	G_OBJECT_CLASS (cpufreq_popup_parent_class)->finalize (object);
}

CPUFreqPopup *
cpufreq_popup_new (void)
{
	CPUFreqPopup *popup;

	popup = CPUFREQ_POPUP (g_object_new (CPUFREQ_TYPE_POPUP,
					     NULL));

	return popup;
}

/* Public methods */
void
cpufreq_popup_set_monitor (CPUFreqPopup   *popup,
			   CPUFreqMonitor *monitor)
{
	g_return_if_fail (CPUFREQ_IS_POPUP (popup));
	g_return_if_fail (CPUFREQ_IS_MONITOR (monitor));

	if (popup->priv->monitor == monitor)
		return;
	
	if (popup->priv->monitor)
		g_object_unref (popup->priv->monitor);
	popup->priv->monitor = g_object_ref (monitor);
}

void
cpufreq_popup_set_parent (CPUFreqPopup *popup,
			  GtkWidget    *parent)
{
	g_return_if_fail (CPUFREQ_IS_POPUP (popup));
	g_return_if_fail (GTK_IS_WIDGET (parent));

	popup->priv->parent = parent;
}

static void
cpufreq_popup_frequencies_menu_activate (GtkAction    *action,
					 CPUFreqPopup *popup)
{
	CPUFreqSelector *selector;
	const gchar     *name;
	guint            cpu;
	guint            freq;
	guint32          parent;

	if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		return;

	selector = cpufreq_selector_get_default ();

	cpu = cpufreq_monitor_get_cpu (popup->priv->monitor);
	name = gtk_action_get_name (action);
	freq = (guint) atoi (name + strlen ("Frequency"));
	parent = GDK_WINDOW_XID (gtk_widget_get_window (popup->priv->parent));
	

	cpufreq_selector_set_frequency_async (selector, cpu, freq, parent);
}

static void
cpufreq_popup_governors_menu_activate (GtkAction    *action,
				       CPUFreqPopup *popup)
{
	CPUFreqSelector *selector;
	const gchar     *name;
	guint            cpu;
	const gchar     *governor;
	guint32          parent;

	if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		return;

	selector = cpufreq_selector_get_default ();
	
	cpu = cpufreq_monitor_get_cpu (popup->priv->monitor);
	name = gtk_action_get_name (action);
	governor = name + strlen ("Governor");
	parent = GDK_WINDOW_XID (gtk_widget_get_window (popup->priv->parent));

	cpufreq_selector_set_governor_async (selector, cpu, governor, parent);
}

static void
cpufreq_popup_menu_add_action (CPUFreqPopup   *popup,
			       const gchar    *menu,
			       GtkActionGroup *action_group,
			       const gchar    *action_name,
			       const gchar    *label,
			       gboolean        sensitive)
{
	GtkToggleAction *action;
	gchar           *name;

	name = g_strdup_printf ("%s%s", menu, action_name);
	
	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", name,
			       "label", label,
			       NULL);

	gtk_action_set_sensitive (GTK_ACTION (action), sensitive);
	
	gtk_radio_action_set_group (GTK_RADIO_ACTION (action), popup->priv->radio_group);
	popup->priv->radio_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (action));
	
	if (g_ascii_strcasecmp (menu, "Frequency") == 0) {
		popup->priv->freqs_actions = g_slist_prepend (popup->priv->freqs_actions,
							      (gpointer) action);

		g_signal_connect (action, "activate",
				  G_CALLBACK (cpufreq_popup_frequencies_menu_activate),
				  (gpointer) popup);
	} else if (g_ascii_strcasecmp (menu, "Governor") == 0) {
		popup->priv->govs_actions = g_slist_prepend (popup->priv->govs_actions,
							     (gpointer) action);

		g_signal_connect (action, "activate",
				  G_CALLBACK (cpufreq_popup_governors_menu_activate),
				  (gpointer) popup);
	}

	gtk_action_group_add_action (action_group, GTK_ACTION (action));
	g_object_unref (action);
	
	g_free (name);
}

static void
frequencies_menu_create_actions (CPUFreqPopup *popup)
{
	GList *available_freqs;

	available_freqs = cpufreq_monitor_get_available_frequencies (popup->priv->monitor);

	while (available_freqs) {
		const gchar *text;
		gchar       *freq_text;
		gchar       *label;
		gchar       *unit;
		gint         freq;
		
		text = (const gchar *) available_freqs->data;
		freq = atoi (text);

		freq_text = cpufreq_utils_get_frequency_label (freq);
		unit = cpufreq_utils_get_frequency_unit (freq);

		label = g_strdup_printf ("%s %s", freq_text, unit);
		g_free (freq_text);
		g_free (unit);

		cpufreq_popup_menu_add_action (popup,
					       "Frequency", 
					       popup->priv->freqs_group,
					       text, label, TRUE);
		g_free (label);

		available_freqs = g_list_next (available_freqs);
	}
}

static void
governors_menu_create_actions (CPUFreqPopup *popup)
{
	GList *available_govs;

	available_govs = cpufreq_monitor_get_available_governors (popup->priv->monitor);
	available_govs = g_list_sort (available_govs, (GCompareFunc)g_ascii_strcasecmp);

	while (available_govs) {
		const gchar *governor;
		gchar       *label;

		governor = (const gchar *) available_govs->data;
		if (g_ascii_strcasecmp (governor, "userspace") == 0) {
			popup->priv->show_freqs = TRUE;
			available_govs = g_list_next (available_govs);
			continue;
		}
		
		label = g_strdup (governor);
		label[0] = g_ascii_toupper (label[0]);
		
		cpufreq_popup_menu_add_action (popup,
					       "Governor",
					       popup->priv->govs_group,
					       governor, label, TRUE);
		g_free (label);

		available_govs = g_list_next (available_govs);
	}
}

static void
cpufreq_popup_build_ui (CPUFreqPopup *popup,
			GSList       *actions,
			const gchar  *menu_path)
{
	GSList *l = NULL;
	
	for (l = actions; l && l->data; l = g_slist_next (l)) { 		
		GtkAction *action;
		gchar     *name = NULL;
		gchar     *label = NULL;

		action = (GtkAction *) l->data;

		g_object_get (G_OBJECT (action),
			      "name", &name,
			      "label", &label,
			      NULL);

		gtk_ui_manager_add_ui (popup->priv->ui_manager,
				       popup->priv->merge_id,
				       menu_path,
				       label, name,
				       GTK_UI_MANAGER_MENUITEM,
				       FALSE);
		
		g_free (name);
		g_free (label);
	}
}

static void
cpufreq_popup_build_frequencies_menu (CPUFreqPopup *popup,
				      const gchar  *path)
{
	if (!popup->priv->freqs_group) {
		GtkActionGroup *action_group;

		action_group = gtk_action_group_new ("FreqsActions");
		popup->priv->freqs_group = action_group;
		gtk_action_group_set_translation_domain (action_group, NULL);

		frequencies_menu_create_actions (popup);
		popup->priv->freqs_actions = g_slist_reverse (popup->priv->freqs_actions);
		gtk_ui_manager_insert_action_group (popup->priv->ui_manager,
						    action_group, 0);
	}

	cpufreq_popup_build_ui (popup,
				popup->priv->freqs_actions,
				path);
}

static void
cpufreq_popup_build_governors_menu (CPUFreqPopup *popup,
				    const gchar  *path)
{
	if (!popup->priv->govs_group) {
		GtkActionGroup *action_group;

		action_group = gtk_action_group_new ("GovsActions");
		popup->priv->govs_group = action_group;
		gtk_action_group_set_translation_domain (action_group, NULL);

		governors_menu_create_actions (popup);
		popup->priv->govs_actions = g_slist_reverse (popup->priv->govs_actions);
		gtk_ui_manager_insert_action_group (popup->priv->ui_manager,
						    action_group, 1);
	}

	cpufreq_popup_build_ui (popup,
				popup->priv->govs_actions,
				path);
}

static void
cpufreq_popup_build_menu (CPUFreqPopup *popup)
{
	if (popup->priv->merge_id > 0) {
		gtk_ui_manager_remove_ui (popup->priv->ui_manager,
					  popup->priv->merge_id);
		gtk_ui_manager_ensure_update (popup->priv->ui_manager);
	}
	
	popup->priv->merge_id = gtk_ui_manager_new_merge_id (popup->priv->ui_manager);
		
	cpufreq_popup_build_frequencies_menu (popup, FREQS_PLACEHOLDER_PATH);
	cpufreq_popup_build_governors_menu (popup, GOVS_PLACEHOLDER_PATH);

	gtk_action_group_set_visible (popup->priv->freqs_group,
				      popup->priv->show_freqs);
}

static void
cpufreq_popup_menu_set_active_action (CPUFreqPopup   *popup,
				      GtkActionGroup *action_group,
				      const gchar    *prefix,
				      const gchar    *item)
{
	gchar      name[128];
	GtkAction *action;

	g_snprintf (name, sizeof (name), "%s%s", prefix, item);
	action = gtk_action_group_get_action (action_group, name);
	
	g_signal_handlers_block_by_func (action,
					 cpufreq_popup_frequencies_menu_activate,
					 popup);
	g_signal_handlers_block_by_func (action,
					 cpufreq_popup_governors_menu_activate,
					 popup);
	
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

	g_signal_handlers_unblock_by_func (action,
					   cpufreq_popup_frequencies_menu_activate,
					   popup);
	g_signal_handlers_unblock_by_func (action,
					   cpufreq_popup_governors_menu_activate,
					   popup);
}

static void
cpufreq_popup_menu_set_active (CPUFreqPopup *popup)
{
	const gchar *governor;

	governor = cpufreq_monitor_get_governor (popup->priv->monitor);
	
	if (g_ascii_strcasecmp (governor, "userspace") == 0) {
		gchar *active;
		guint  freq;

		freq = cpufreq_monitor_get_frequency (popup->priv->monitor);
		active = g_strdup_printf ("%d", freq);
		cpufreq_popup_menu_set_active_action (popup,
						      popup->priv->freqs_group,
						      "Frequency", active);
		g_free (active);
	} else {
		cpufreq_popup_menu_set_active_action (popup,
						      popup->priv->govs_group,
						      "Governor", governor);
	}
}

GtkWidget *
cpufreq_popup_get_menu (CPUFreqPopup *popup)
{
	GtkWidget *menu;
	
	g_return_val_if_fail (CPUFREQ_IS_POPUP (popup), NULL);
	g_return_val_if_fail (CPUFREQ_IS_MONITOR (popup->priv->monitor), NULL);
	
	if (!cpufreq_utils_selector_is_available ())
		return NULL;

	if (popup->priv->need_build) {
		cpufreq_popup_build_menu (popup);
		popup->priv->need_build = FALSE;
	}

	cpufreq_popup_menu_set_active (popup);
	
	menu = gtk_ui_manager_get_widget (popup->priv->ui_manager,
					  "/CPUFreqSelectorPopup");
	
	return menu;
}
