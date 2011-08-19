/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "gkbd-indicator-plugins-capplet.h"

#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>

static GkbdKeyboardConfig initialSysKbdConfig;
static GMainLoop *loop;

extern void
CappletFillActivePluginList (GkbdIndicatorPluginsCapplet * gipc)
{
	GtkWidget *activePlugins =
	    CappletGetUiWidget (gipc, "activePlugins");
	GtkListStore *activePluginsModel =
	    GTK_LIST_STORE (gtk_tree_view_get_model
			    (GTK_TREE_VIEW (activePlugins)));
	GSList *pluginPathNode = gipc->applet_cfg.enabled_plugins;
	GHashTable *allPluginRecs = gipc->plugin_manager.all_plugin_recs;

	gtk_list_store_clear (activePluginsModel);
	if (allPluginRecs == NULL)
		return;

	while (pluginPathNode != NULL) {
		GtkTreeIter iter;
		const char *fullPath = (const char *) pluginPathNode->data;
		const GkbdIndicatorPlugin *plugin =
		    gkbd_indicator_plugin_manager_get_plugin (&gipc->
							      plugin_manager,
							      fullPath);
		if (plugin != NULL) {
			gtk_list_store_append (activePluginsModel, &iter);
			gtk_list_store_set (activePluginsModel, &iter,
					    NAME_COLUMN, plugin->name,
					    FULLPATH_COLUMN, fullPath, -1);
		}

		pluginPathNode = g_slist_next (pluginPathNode);
	}
}

static char *
CappletGetSelectedActivePluginPath (GkbdIndicatorPluginsCapplet * gipc)
{
	GtkTreeView *pluginsList =
	    GTK_TREE_VIEW (CappletGetUiWidget (gipc, "activePlugins"));
	return CappletGetSelectedPluginPath (pluginsList, gipc);
}

char *
CappletGetSelectedPluginPath (GtkTreeView * pluginsList,
			      GkbdIndicatorPluginsCapplet * gipc)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (pluginsList));
	GtkTreeSelection *selection =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW (pluginsList));
	GtkTreeIter selectedIter;

	if (gtk_tree_selection_get_selected
	    (selection, NULL, &selectedIter)) {
		char *fullPath = NULL;

		gtk_tree_model_get (model, &selectedIter,
				    FULLPATH_COLUMN, &fullPath, -1);
		return fullPath;
	}
	return NULL;
}

static void
CappletActivePluginsSelectionChanged (GtkTreeSelection *
				      selection,
				      GkbdIndicatorPluginsCapplet * gipc)
{
	GtkWidget *activePlugins =
	    CappletGetUiWidget (gipc, "activePlugins");
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (activePlugins));
	GtkTreeIter selectedIter;
	gboolean isAnythingSelected = FALSE;
	gboolean isFirstSelected = FALSE;
	gboolean isLastSelected = FALSE;
	gboolean hasConfigurationUi = FALSE;
	GtkWidget *btnRemove = CappletGetUiWidget (gipc, "btnRemove");
	GtkWidget *btnUp = CappletGetUiWidget (gipc, "btnUp");
	GtkWidget *btnDown = CappletGetUiWidget (gipc, "btnDown");
	GtkWidget *btnProperties = CappletGetUiWidget (gipc, "btnProperties");
	GtkWidget *lblDescription = CappletGetUiWidget (gipc, "lblDescription");

	gtk_label_set_text (GTK_LABEL (lblDescription),
			    g_strconcat ("<small><i>",
					 _("No description."),
					 "</i></small>", NULL));
	gtk_label_set_use_markup (GTK_LABEL (lblDescription), TRUE);

	if (gtk_tree_selection_get_selected
	    (selection, NULL, &selectedIter)) {
		int counter = gtk_tree_model_iter_n_children (model, NULL);
		GtkTreePath *treePath =
		    gtk_tree_model_get_path (model, &selectedIter);
		gint *indices = gtk_tree_path_get_indices (treePath);
		char *fullPath = CappletGetSelectedActivePluginPath (gipc);
		const GkbdIndicatorPlugin *plugin =
		    gkbd_indicator_plugin_manager_get_plugin (&gipc->
							      plugin_manager,
							      fullPath);

		isAnythingSelected = TRUE;

		isFirstSelected = indices[0] == 0;
		isLastSelected = indices[0] == counter - 1;

		if (plugin != NULL) {
			hasConfigurationUi =
			    (plugin->configure_properties_callback !=
			     NULL);
			gtk_label_set_text (GTK_LABEL (lblDescription),
					    g_strconcat ("<small><i>",
							 plugin->
							 description,
							 "</i></small>",
							 NULL));
			gtk_label_set_use_markup (GTK_LABEL
						  (lblDescription), TRUE);
		}
		g_free (fullPath);

		gtk_tree_path_free (treePath);
	}
	gtk_widget_set_sensitive (btnRemove, isAnythingSelected);
	gtk_widget_set_sensitive (btnUp, isAnythingSelected
				  && !isFirstSelected);
	gtk_widget_set_sensitive (btnDown, isAnythingSelected
				  && !isLastSelected);
	gtk_widget_set_sensitive (btnProperties, isAnythingSelected
				  && hasConfigurationUi);
}

static void
CappletPromotePlugin (GtkWidget * btnUp,
		      GkbdIndicatorPluginsCapplet * gipc)
{
	char *fullPath = CappletGetSelectedActivePluginPath (gipc);
	if (fullPath != NULL) {
		gkbd_indicator_plugin_manager_promote_plugin (&gipc->
							      plugin_manager,
							      gipc->
							      applet_cfg.
							      enabled_plugins,
							      fullPath);
		g_free (fullPath);
		CappletFillActivePluginList (gipc);
		gkbd_indicator_config_save_to_mateconf (&gipc->applet_cfg);
	}
}

static void
CappletDemotePlugin (GtkWidget * btnUp, GkbdIndicatorPluginsCapplet * gipc)
{
	char *fullPath = CappletGetSelectedActivePluginPath (gipc);
	if (fullPath != NULL) {
		gkbd_indicator_plugin_manager_demote_plugin (&gipc->
							     plugin_manager,
							     gipc->
							     applet_cfg.
							     enabled_plugins,
							     fullPath);
		g_free (fullPath);
		CappletFillActivePluginList (gipc);
		gkbd_indicator_config_save_to_mateconf (&gipc->applet_cfg);
	}
}

static void
CappletDisablePlugin (GtkWidget * btnRemove,
		      GkbdIndicatorPluginsCapplet * gipc)
{
	char *fullPath = CappletGetSelectedActivePluginPath (gipc);
	if (fullPath != NULL) {
		gkbd_indicator_plugin_manager_disable_plugin (&gipc->
							      plugin_manager,
							      &gipc->
							      applet_cfg.
							      enabled_plugins,
							      fullPath);
		g_free (fullPath);
		CappletFillActivePluginList (gipc);
		gkbd_indicator_config_save_to_mateconf (&gipc->applet_cfg);
	}
}

static void
CappletConfigurePlugin (GtkWidget * btnRemove,
			GkbdIndicatorPluginsCapplet * gipc)
{
	char *fullPath = CappletGetSelectedActivePluginPath (gipc);
	if (fullPath != NULL) {
		gkbd_indicator_plugin_manager_configure_plugin (&gipc->
								plugin_manager,
								&gipc->
								plugin_container,
								fullPath,
								GTK_WINDOW
								(gipc->
								 capplet));
		g_free (fullPath);
	}
}

static void
CappletResponse (GtkDialog * dialog, gint response)
{
	if (response == GTK_RESPONSE_HELP) {
		GError *error = NULL;
		GdkAppLaunchContext *ctx = gdk_app_launch_context_new ();

		g_app_info_launch_default_for_uri
		    ("ghelp:gkbd?gkb-indicator-applet-plugins",
		     G_APP_LAUNCH_CONTEXT (ctx), &error);

		if (error) {
			GtkWidget *d = NULL;

			d = gtk_message_dialog_new (GTK_WINDOW (dialog),
						    GTK_DIALOG_MODAL |
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR,
						    GTK_BUTTONS_CLOSE,
						    "%s", _
						    ("Unable to open help file"));
			gtk_message_dialog_format_secondary_text
			    (GTK_MESSAGE_DIALOG (d), "%s", error->message);
			g_signal_connect (d, "response",
					  G_CALLBACK (gtk_widget_destroy),
					  NULL);
			gtk_window_present (GTK_WINDOW (d));

			g_error_free (error);
		}

		return;
	}

	g_main_loop_quit (loop);
}

static void
CappletSetup (GkbdIndicatorPluginsCapplet * gipc)
{
	GtkBuilder *builder;
	GError *error = NULL;
	GtkWidget *button;
	GtkWidget *capplet;
	GtkWidget *activePlugins;
	GtkTreeModel *activePluginsModel;
	GtkCellRenderer *renderer =
	    GTK_CELL_RENDERER (gtk_cell_renderer_text_new ());
	GtkTreeViewColumn *column =
	    gtk_tree_view_column_new_with_attributes (NULL, renderer,
						      "text", 0,
						      NULL);
	GtkTreeSelection *selection;
	builder = gtk_builder_new ();

	gtk_window_set_default_icon_name ("input-keyboard");

	/* default domain! */
	if (!gtk_builder_add_from_file (builder,
	                                UIDIR "/gkbd-indicator-plugins.ui",
	                                &error)) {
		g_warning ("Could not load builder file: %s", error->message);
		g_error_free(error);
		return;
	}

	gipc->capplet = capplet =
	    GTK_WIDGET (gtk_builder_get_object (builder, "gkbd_indicator_plugins"));

	gtk_builder_connect_signals (builder, NULL);

	g_object_set_data (G_OBJECT (capplet), "uiData", builder);
	g_signal_connect_swapped (GTK_OBJECT (capplet),
				  "destroy", G_CALLBACK (g_object_unref),
				  builder);
	g_signal_connect_swapped (G_OBJECT (capplet), "unrealize",
				  G_CALLBACK (g_main_loop_quit), loop);

	g_signal_connect (GTK_OBJECT (capplet),
			  "response", G_CALLBACK (CappletResponse), NULL);

	button = GTK_WIDGET (gtk_builder_get_object (builder, "btnUp"));
	g_signal_connect (button,  "clicked",
				       G_CALLBACK
				       (CappletPromotePlugin), gipc);
	button = GTK_WIDGET (gtk_builder_get_object (builder, "btnDown"));
	g_signal_connect (button, 
				       "clicked",
				       G_CALLBACK
				       (CappletDemotePlugin), gipc);
	button = GTK_WIDGET (gtk_builder_get_object (builder, "btnAdd"));
	g_signal_connect (button,  "clicked",
				       G_CALLBACK
				       (CappletEnablePlugin), gipc);
	button = GTK_WIDGET (gtk_builder_get_object (builder, "btnRemove"));
	g_signal_connect (button,  "clicked",
				       G_CALLBACK
				       (CappletDisablePlugin), gipc);
	button = GTK_WIDGET (gtk_builder_get_object (builder, "btnProperties"));
	g_signal_connect (button,  "clicked",
				       G_CALLBACK
				       (CappletConfigurePlugin), gipc);

	activePlugins = CappletGetUiWidget (gipc, "activePlugins");
	activePluginsModel =
	    GTK_TREE_MODEL (gtk_list_store_new
			    (2, G_TYPE_STRING, G_TYPE_STRING));
	gtk_tree_view_set_model (GTK_TREE_VIEW (activePlugins),
				 activePluginsModel);
	gtk_tree_view_append_column (GTK_TREE_VIEW (activePlugins),
				     column);
	selection =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW (activePlugins));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK
			  (CappletActivePluginsSelectionChanged), gipc);
	CappletFillActivePluginList (gipc);
	CappletActivePluginsSelectionChanged (selection, gipc);
	gtk_widget_show_all (capplet);
}

int
main (int argc, char **argv)
{
	GkbdIndicatorPluginsCapplet gipc;

	GError *mateconf_error = NULL;
	MateConfClient *confClient;

	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	memset (&gipc, 0, sizeof (gipc));
	gtk_init_with_args (&argc, &argv, "gkbd", NULL, NULL, NULL);
	if (!mateconf_init (argc, argv, &mateconf_error)) {
		g_warning (_("Failed to init MateConf: %s\n"),
			   mateconf_error->message);
		g_error_free (mateconf_error);
		return 1;
	}
	mateconf_error = NULL;
	/*GkbdIndicatorInstallGlibLogAppender(  ); */
	gipc.engine = xkl_engine_get_instance (GDK_DISPLAY ());
	gipc.config_registry =
	    xkl_config_registry_get_instance (gipc.engine);

	confClient = mateconf_client_get_default ();
	gkbd_indicator_plugin_container_init (&gipc.plugin_container,
					      confClient);
	g_object_unref (confClient);

	gkbd_keyboard_config_init (&gipc.kbd_cfg, confClient, gipc.engine);
	gkbd_keyboard_config_init (&initialSysKbdConfig, confClient,
				   gipc.engine);

	gkbd_indicator_config_init (&gipc.applet_cfg, confClient,
				    gipc.engine);

	gkbd_indicator_plugin_manager_init (&gipc.plugin_manager);

	gkbd_keyboard_config_load_from_x_initial (&initialSysKbdConfig,
						  NULL);
	gkbd_keyboard_config_load_from_mateconf (&gipc.kbd_cfg,
					      &initialSysKbdConfig);

	gkbd_indicator_config_load_from_mateconf (&gipc.applet_cfg);

	loop = g_main_loop_new (NULL, TRUE);

	CappletSetup (&gipc);

	g_main_loop_run (loop);

	gkbd_indicator_plugin_manager_term (&gipc.plugin_manager);

	gkbd_indicator_config_term (&gipc.applet_cfg);

	gkbd_keyboard_config_term (&gipc.kbd_cfg);
	gkbd_keyboard_config_term (&initialSysKbdConfig);

	gkbd_indicator_plugin_container_term (&gipc.plugin_container);
	g_object_unref (G_OBJECT (gipc.config_registry));
	g_object_unref (G_OBJECT (gipc.engine));
	return 0;
}

/* functions just for plugins - otherwise ldd is not happy */
void
gkbd_indicator_plugin_container_reinit_ui (GkbdIndicatorPluginContainer *
					   pc)
{
}

gchar **gkbd_indicator_plugin_load_localized_group_names
    (GkbdIndicatorPluginContainer * pc) {
	GkbdDesktopConfig *config =
	    &((GkbdIndicatorPluginsCapplet *) pc)->cfg;
	XklConfigRegistry *config_registry =
	    ((GkbdIndicatorPluginsCapplet *) pc)->config_registry;

	int i;
	const gchar **native_names =
	    xkl_engine_get_groups_names (config->engine);
	guint total_groups = xkl_engine_get_num_groups (config->engine);
	guint total_layouts;
	gchar **rv = g_new0 (char *, total_groups + 1);
	gchar **current_descr = rv;

	if ((xkl_engine_get_features (config->engine) &
	     XKLF_MULTIPLE_LAYOUTS_SUPPORTED)
	    && config->layout_names_as_group_names) {
		XklConfigRec *xkl_config = xkl_config_rec_new ();
		if (xkl_config_rec_get_from_server
		    (xkl_config, config->engine)) {
			gchar **sgn = NULL;
			gchar **fgn = NULL;
			gkbd_desktop_config_load_group_descriptions
			    (config, config_registry,
			     (const gchar **) xkl_config->layouts,
			     (const gchar **) xkl_config->variants, &sgn,
			     &fgn);
			g_strfreev (sgn);
			rv = fgn;
		}
		g_object_unref (G_OBJECT (xkl_config));
		/* Worst case - multiple layous - but SOME of them are multigrouped :(((
		 * We cannot do much - just add empty descriptions.
		 * The UI is going to be messy.
		 * Canadian layouts are famous for this sh.t. */
		total_layouts = g_strv_length (rv);
		if (total_layouts != total_groups) {
			xkl_debug (0,
				   "The mismatch between "
				   "the number of groups: %d and number of layouts: %d\n",
				   total_groups, total_layouts);
			current_descr = rv + total_layouts;
			for (i = total_groups - total_layouts; --i >= 0;)
				*current_descr++ = g_strdup ("");
		}
	}
	total_layouts = g_strv_length (rv);
	if (!total_layouts)
		for (i = total_groups; --i >= 0;)
			*current_descr++ = g_strdup (*native_names++);

	return rv;
}
