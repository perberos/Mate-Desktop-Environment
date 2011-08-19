#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-reload-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <eog-debug.h>
#include <eog-scroll-view.h>
#include <eog-thumb-view.h>
#include <eog-image.h>

#define WINDOW_DATA_KEY "EogReloadWindowData"

EOG_PLUGIN_REGISTER_TYPE(EogReloadPlugin, eog_reload_plugin)

typedef struct
{
	GtkActionGroup *ui_action_group;
	guint ui_id;
} WindowData;

static void
reload_cb (GtkAction	*action,
	  EogWindow *window)
{
        eog_window_reload_image (window);
}

static const gchar * const ui_definition =
	"<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\"><separator/>"
	"<menuitem name=\"EogPluginReload\" action=\"EogPluginRunReload\"/>"
	"<separator/></menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
	"<menuitem action=\"EogPluginRunReload\"/><separator/>"
	"</popup></ui>";

static const GtkActionEntry action_entries[] =
{
	{ "EogPluginRunReload",
	  GTK_STOCK_REFRESH,
	  N_("Reload Image"),
	  "R",
	  N_("Reload current image"),
	  G_CALLBACK (reload_cb) }
};

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	eog_debug (DEBUG_PLUGINS);

	g_object_unref (data->ui_action_group);

	g_free (data);
}

static void
eog_reload_plugin_init (EogReloadPlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogReloadPlugin initializing");
}

static void
eog_reload_plugin_finalize (GObject *object)
{
	eog_debug_message (DEBUG_PLUGINS, "EogReloadPlugin finalizing");

	G_OBJECT_CLASS (eog_reload_plugin_parent_class)->finalize (object);
}

static void
impl_activate (EogPlugin *plugin,
	       EogWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	eog_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);

	manager = eog_window_get_ui_manager (window);

	data->ui_action_group = gtk_action_group_new ("EogReloadPluginActions");

	gtk_action_group_set_translation_domain (data->ui_action_group,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (data->ui_action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);

	gtk_ui_manager_insert_action_group (manager,
					    data->ui_action_group,
					    -1);

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);

	data->ui_id = gtk_ui_manager_add_ui_from_string (manager,
							 ui_definition,
							 -1, NULL);
	g_warn_if_fail (data->ui_id != 0);
}

static void
impl_deactivate	(EogPlugin *plugin,
		 EogWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	eog_debug (DEBUG_PLUGINS);

	manager = eog_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager,
				  data->ui_id);

	gtk_ui_manager_remove_action_group (manager,
					    data->ui_action_group);

	g_object_set_data (G_OBJECT (window),
			   WINDOW_DATA_KEY,
			   NULL);
}

static void
eog_reload_plugin_class_init (EogReloadPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	EogPluginClass *plugin_class = EOG_PLUGIN_CLASS (klass);

	object_class->finalize = eog_reload_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}
