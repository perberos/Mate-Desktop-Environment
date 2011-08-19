#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-fullscreen-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <eog-debug.h>
#include <eog-scroll-view.h>

#define WINDOW_DATA_KEY "EogFullscreenWindowData"

EOG_PLUGIN_REGISTER_TYPE(EogFullscreenPlugin, eog_fullscreen_plugin)

typedef struct
{
	gulong signal_id;
} WindowData;

static gboolean
on_button_press (GtkWidget *button, GdkEventButton *event, EogWindow *window)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		EogWindowMode mode = eog_window_get_mode (window);

		if (mode == EOG_WINDOW_MODE_SLIDESHOW ||
		    mode == EOG_WINDOW_MODE_FULLSCREEN)
			eog_window_set_mode (window, EOG_WINDOW_MODE_NORMAL);
		else if (mode == EOG_WINDOW_MODE_NORMAL)
			eog_window_set_mode (window, EOG_WINDOW_MODE_FULLSCREEN);

		return TRUE;
	}

	return FALSE;
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	eog_debug (DEBUG_PLUGINS);

	g_free (data);
}

static void
eog_fullscreen_plugin_init (EogFullscreenPlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogFullscreenPlugin initializing");
}

static void
eog_fullscreen_plugin_finalize (GObject *object)
{
	eog_debug_message (DEBUG_PLUGINS, "EogFullscreenPlugin finalizing");

	G_OBJECT_CLASS (eog_fullscreen_plugin_parent_class)->finalize (object);
}

static void
impl_activate (EogPlugin *plugin,
	       EogWindow *window)
{
	GtkWidget *view = eog_window_get_view (window);
	WindowData *data;

	eog_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);

	data->signal_id = g_signal_connect (G_OBJECT (view),
			   		    "button-press-event",
			  		    G_CALLBACK (on_button_press),
			  		    window);

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
}

static void
impl_deactivate	(EogPlugin *plugin,
		 EogWindow *window)
{
	GtkWidget *view = eog_window_get_view (window);
	WindowData *data;

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);

	g_signal_handler_disconnect (view, data->signal_id);

	g_object_set_data (G_OBJECT (window),
			   WINDOW_DATA_KEY,
			   NULL);
}

static void
impl_update_ui (EogPlugin *plugin,
		EogWindow *window)
{
}

static void
eog_fullscreen_plugin_class_init (EogFullscreenPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	EogPluginClass *plugin_class = EOG_PLUGIN_CLASS (klass);

	object_class->finalize = eog_fullscreen_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
