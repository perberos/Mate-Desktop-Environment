/* Eye Of Mate - Application Facade
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-application.h) by:
 * 	- Martin Kretzschmar <martink@mate.org>
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

#include "eog-image.h"
#include "eog-session.h"
#include "eog-window.h"
#include "eog-application.h"
#include "eog-util.h"

#ifdef HAVE_DBUS
#include "totem-scrsaver.h"
#endif

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#ifdef HAVE_DBUS
#include "eog-application-service.h"
#include <dbus/dbus-glib-bindings.h>

#define APPLICATION_SERVICE_NAME "org.mate.eog.ApplicationService"
#endif

static void eog_application_load_accelerators (void);
static void eog_application_save_accelerators (void);

#define EOG_APPLICATION_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_APPLICATION, EogApplicationPrivate))

G_DEFINE_TYPE (EogApplication, eog_application, G_TYPE_OBJECT);

#ifdef HAVE_DBUS

/**
 * eog_application_register_service:
 * @application: An #EogApplication.
 *
 * Registers #EogApplication<!-- -->'s DBus service, to allow
 * remote calls. If the DBus service is already registered,
 * or there is any other connection error, returns %FALSE.
 *
 * Returns: %TRUE if the service was registered succesfully. %FALSE
 * otherwise.
 **/
gboolean
eog_application_register_service (EogApplication *application)
{
	static DBusGConnection *connection = NULL;
	DBusGProxy *driver_proxy;
	GError *err = NULL;
	guint request_name_result;

	if (connection) {
		g_warning ("Service already registered.");
		return FALSE;
	}

	connection = dbus_g_bus_get (DBUS_BUS_STARTER, &err);

	if (connection == NULL) {
		g_warning ("Service registration failed.");
		g_error_free (err);

		return FALSE;
	}

	driver_proxy = dbus_g_proxy_new_for_name (connection,
						  DBUS_SERVICE_DBUS,
						  DBUS_PATH_DBUS,
						  DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (driver_proxy,
                                        	APPLICATION_SERVICE_NAME,
						DBUS_NAME_FLAG_DO_NOT_QUEUE,
						&request_name_result, &err)) {
		g_warning ("Service registration failed.");
		g_clear_error (&err);
	}

	g_object_unref (driver_proxy);

	if (request_name_result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		return FALSE;
	}

	dbus_g_object_type_install_info (EOG_TYPE_APPLICATION,
					 &dbus_glib_eog_application_object_info);

	dbus_g_connection_register_g_object (connection,
					     "/org/mate/eog/Eog",
                                             G_OBJECT (application));

        application->scr_saver = totem_scrsaver_new ();
        g_object_set (application->scr_saver,
		      "reason", _("Running in fullscreen mode"),
		      NULL);

	return TRUE;
}
#endif /* ENABLE_DBUS */

static void
eog_application_class_init (EogApplicationClass *eog_application_class)
{
}

static void
eog_application_init (EogApplication *eog_application)
{
	const gchar *dot_dir = eog_util_dot_dir ();

	eog_session_init (eog_application);

	eog_application->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (eog_application->toolbars_model,
				       EOG_DATA_DIR "/eog-toolbar.xml");

	if (G_LIKELY (dot_dir != NULL))
		eog_application->toolbars_file = g_build_filename
			(dot_dir, "eog_toolbar.xml", NULL);

	if (!dot_dir || !egg_toolbars_model_load_toolbars (eog_application->toolbars_model,
					       eog_application->toolbars_file)) {

		egg_toolbars_model_load_toolbars (eog_application->toolbars_model,
						  EOG_DATA_DIR "/eog-toolbar.xml");
	}

	egg_toolbars_model_set_flags (eog_application->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);

	eog_application_load_accelerators ();
}

/**
 * eog_application_get_instance:
 *
 * Returns a singleton instance of #EogApplication currently running.
 * If not running yet, it will create one.
 *
 * Returns: a running #EogApplication.
 **/
EogApplication *
eog_application_get_instance (void)
{
	static EogApplication *instance;

	if (!instance) {
		instance = EOG_APPLICATION (g_object_new (EOG_TYPE_APPLICATION, NULL));
	}

	return instance;
}

static EogWindow *
eog_application_get_empty_window (EogApplication *application)
{
	EogWindow *empty_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	windows = eog_application_get_windows (application);

	for (l = windows; l != NULL; l = l->next) {
		EogWindow *window = EOG_WINDOW (l->data);

		if (eog_window_is_empty (window)) {
			empty_window = window;
			break;
		}
	}

	g_list_free (windows);

	return empty_window;
}

/**
 * eog_application_open_window:
 * @application: An #EogApplication.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens and presents an empty #EogWindow to the user. If there is
 * an empty window already open, this will be used. Otherwise, a
 * new one will be instantiated.
 *
 * Returns: %FALSE if @application is invalid, %TRUE otherwise
 **/
gboolean
eog_application_open_window (EogApplication  *application,
			     guint32         timestamp,
			     EogStartupFlags flags,
			     GError        **error)
{
	GtkWidget *new_window = NULL;

	new_window = GTK_WIDGET (eog_application_get_empty_window (application));

	if (new_window == NULL) {
		new_window = eog_window_new (flags);
	}

	g_return_val_if_fail (EOG_IS_APPLICATION (application), FALSE);

	gtk_window_present_with_time (GTK_WINDOW (new_window),
				      timestamp);

	return TRUE;
}

static EogWindow *
eog_application_get_file_window (EogApplication *application, GFile *file)
{
	EogWindow *file_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	windows = gtk_window_list_toplevels ();

	for (l = windows; l != NULL; l = l->next) {
		if (EOG_IS_WINDOW (l->data)) {
			EogWindow *window = EOG_WINDOW (l->data);

			if (!eog_window_is_empty (window)) {
				EogImage *image = eog_window_get_image (window);
				GFile *window_file;

				window_file = eog_image_get_file (image);
				if (g_file_equal (window_file, file)) {
					file_window = window;
					break;
				}
			}
		}
	}

	g_list_free (windows);

	return file_window;
}

static void
eog_application_show_window (EogWindow *window, gpointer user_data)
{
	gtk_window_present_with_time (GTK_WINDOW (window),
				      GPOINTER_TO_UINT (user_data));
}

/**
 * eog_application_open_file_list:
 * @application: An #EogApplication.
 * @file_list: A list of #GFile<!-- -->s.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of files in a #EogWindow. If an #EogWindow displaying the first
 * image in the list is already open, this will be used. Otherwise, an empty
 * #EogWindow is used, either already existing or newly created.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_file_list (EogApplication  *application,
				GSList          *file_list,
				guint           timestamp,
				EogStartupFlags flags,
				GError         **error)
{
	EogWindow *new_window = NULL;

	if (file_list != NULL)
		new_window = eog_application_get_file_window (application,
							      (GFile *) file_list->data);

	if (new_window != NULL) {
		gtk_window_present_with_time (GTK_WINDOW (new_window),
					      timestamp);
		return TRUE;
	}

	new_window = eog_application_get_empty_window (application);

	if (new_window == NULL) {
		new_window = EOG_WINDOW (eog_window_new (flags));
	}

	g_signal_connect (new_window,
			  "prepared",
			  G_CALLBACK (eog_application_show_window),
			  GUINT_TO_POINTER (timestamp));

	eog_window_open_file_list (new_window, file_list);

	return TRUE;
}

/**
 * eog_application_open_uri_list:
 * @application: An #EogApplication.
 * @uri_list: A list of URIs.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URIs. See
 * eog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_uri_list (EogApplication  *application,
 			       GSList          *uri_list,
 			       guint           timestamp,
 			       EogStartupFlags flags,
 			       GError         **error)
{
 	GSList *file_list = NULL;

 	g_return_val_if_fail (EOG_IS_APPLICATION (application), FALSE);

 	file_list = eog_util_string_list_to_file_list (uri_list);

 	return eog_application_open_file_list (application,
					       file_list,
					       timestamp,
					       flags,
					       error);
}

#ifdef HAVE_DBUS
/**
 * eog_application_open_uris:
 * @application: an #EogApplication
 * @uris:  A #GList of URI strings.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URI strings. See
 * eog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_uris (EogApplication  *application,
 			   gchar          **uris,
 			   guint           timestamp,
 			   EogStartupFlags flags,
 			   GError        **error)
{
 	GSList *file_list = NULL;

 	file_list = eog_util_strings_to_file_list (uris);

 	return eog_application_open_file_list (application, file_list, timestamp,
						    flags, error);
}
#endif

/**
 * eog_application_shutdown:
 * @application: An #EogApplication.
 *
 * Takes care of shutting down the Eye of MATE, and quits.
 **/
void
eog_application_shutdown (EogApplication *application)
{
	g_return_if_fail (EOG_IS_APPLICATION (application));

	if (application->toolbars_model) {
		g_object_unref (application->toolbars_model);
		application->toolbars_model = NULL;

		g_free (application->toolbars_file);
		application->toolbars_file = NULL;
	}

	eog_application_save_accelerators ();

	g_object_unref (application);

	gtk_main_quit ();
}

/**
 * eog_application_get_windows:
 * @application: An #EogApplication.
 *
 * Gets the list of existing #EogApplication<!-- -->s. The windows
 * in this list are not individually referenced, you need to keep
 * your own references if you want to perform actions that may destroy
 * them.
 *
 * Returns: A new list of #EogWindow<!-- -->s.
 **/
GList *
eog_application_get_windows (EogApplication *application)
{
	GList *l, *toplevels;
	GList *windows = NULL;

	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	toplevels = gtk_window_list_toplevels ();

	for (l = toplevels; l != NULL; l = l->next) {
		if (EOG_IS_WINDOW (l->data)) {
			windows = g_list_append (windows, l->data);
		}
	}

	g_list_free (toplevels);

	return windows;
}

/**
 * eog_application_get_toolbars_model:
 * @application: An #EogApplication.
 *
 * Retrieves the #EggToolbarsModel for the toolbar in #EogApplication.
 *
 * Returns: An #EggToolbarsModel.
 **/
EggToolbarsModel *
eog_application_get_toolbars_model (EogApplication *application)
{
	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	return application->toolbars_model;
}

/**
 * eog_application_save_toolbars_model:
 * @application: An #EogApplication.
 *
 * Causes the saving of the model of the toolbar in #EogApplication to a file.
 **/
void
eog_application_save_toolbars_model (EogApplication *application)
{
	if (G_LIKELY(application->toolbars_file != NULL))
        	egg_toolbars_model_save_toolbars (application->toolbars_model,
				 	          application->toolbars_file,
						  "1.0");
}

/**
 * eog_application_reset_toolbars_model:
 * @app: an #EogApplication
 *
 * Restores the toolbars model to the defaults.
 **/
void
eog_application_reset_toolbars_model (EogApplication *app)
{
	g_return_if_fail (EOG_IS_APPLICATION (app));

	g_object_unref (app->toolbars_model);

	app->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (app->toolbars_model,
				       EOG_DATA_DIR "/eog-toolbar.xml");
	egg_toolbars_model_load_toolbars (app->toolbars_model,
					  EOG_DATA_DIR "/eog-toolbar.xml");
	egg_toolbars_model_set_flags (app->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);
}

#ifdef HAVE_DBUS
/**
 * eog_application_screensaver_enable:
 * @application: an #EogApplication.
 *
 * Enables the screensaver. Usually necessary after a call to
 * eog_application_screensaver_disable().
 **/
void
eog_application_screensaver_enable (EogApplication *application)
{
        if (application->scr_saver)
                totem_scrsaver_enable (application->scr_saver);
}

/**
 * eog_application_screensaver_disable:
 * @application: an #EogApplication.
 *
 * Disables the screensaver. Useful when the application is in fullscreen or
 * similar mode.
 **/
void
eog_application_screensaver_disable (EogApplication *application)
{
        if (application->scr_saver)
                totem_scrsaver_disable (application->scr_saver);
}
#endif

static void
eog_application_load_accelerators (void)
{
	gchar *accelfile = g_build_filename (g_get_home_dir (),
					     ".mate2",
					     "accels",
					     "eog", NULL);

	/* gtk_accel_map_load does nothing if the file does not exist */
	gtk_accel_map_load (accelfile);
	g_free (accelfile);
}

static void
eog_application_save_accelerators (void)
{
	gchar *accelfile = g_build_filename (g_get_home_dir (),
					     ".mate2",
					     "accels",
					     "eog", NULL);

	gtk_accel_map_save (accelfile);
	g_free (accelfile);
}
