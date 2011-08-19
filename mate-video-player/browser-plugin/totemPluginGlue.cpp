/* Totem Mozilla plugin
 * 
 * Copyright © 2004-2006 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2008 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#include <config.h>

#include <string.h>

#include <gio/gio.h>
#include <dlfcn.h>

#include "npapi.h"
#include "npupp.h"

#include "totemPlugin.h"

NPNetscapeFuncs NPNFuncs; /* used in npn_gate.cpp */

static char *mime_list = NULL;

static NPError
totem_plugin_new_instance (NPMIMEType mimetype,
			   NPP instance,
			   uint16_t mode,
			   int16_t argc,
			   char *argn[],
			   char *argv[],
			   NPSavedData *savedData)
{
	if (!instance)
		return NPERR_INVALID_INSTANCE_ERROR;

	totemPlugin *plugin = new totemPlugin (instance);
	if (!plugin)
		return NPERR_OUT_OF_MEMORY_ERROR;

	instance->pdata = reinterpret_cast<void*> (plugin);

	NPError rv = plugin->Init (mimetype, mode, argc, argn, argv, savedData);
	if (rv != NPERR_NO_ERROR) {
		delete plugin;
		instance->pdata = 0;
	}

	return rv;
}

static NPError
totem_plugin_destroy_instance (NPP instance,
			       NPSavedData **save)
{
	if (!instance)
		return NPERR_INVALID_INSTANCE_ERROR;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return NPERR_NO_ERROR;

	delete plugin;

	instance->pdata = 0;

	return NPERR_NO_ERROR;
}

static NPError
totem_plugin_set_window (NPP instance,
			 NPWindow* window)
{
	if (!instance)
		return NPERR_INVALID_INSTANCE_ERROR;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return NPERR_INVALID_INSTANCE_ERROR;

	return plugin->SetWindow (window);
}

static NPError
totem_plugin_new_stream (NPP instance,
			 NPMIMEType type,
			 NPStream* stream_ptr,
			 NPBool seekable,
			 uint16* stype)
{
	if (!instance)
		return NPERR_INVALID_INSTANCE_ERROR;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return NPERR_INVALID_INSTANCE_ERROR;

	return plugin->NewStream (type, stream_ptr, seekable, stype);
}

static NPError
totem_plugin_destroy_stream (NPP instance,
			     NPStream* stream,
			     NPError reason)
{
	if (!instance) {
		g_debug ("totem_plugin_destroy_stream instance is NULL");
		/* FIXME? */
		return NPERR_NO_ERROR;
	}

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return NPERR_INVALID_INSTANCE_ERROR;

	return plugin->DestroyStream (stream, reason);
}

static int32_t
totem_plugin_write_ready (NPP instance,
			  NPStream *stream)
{
	if (!instance)
		return -1;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return -1;

	return plugin->WriteReady (stream);
}

static int32_t
totem_plugin_write (NPP instance,
		    NPStream *stream,
		    int32_t offset,
		    int32_t len,
		    void *buffer)
{
	if (!instance)
		return -1;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return -1;

	return plugin->Write (stream, offset, len, buffer);
}

static void
totem_plugin_stream_as_file (NPP instance,
			     NPStream *stream,
			     const char* fname)
{
	if (!instance)
		return;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return;

	plugin->StreamAsFile (stream, fname);
}

static void
totem_plugin_url_notify (NPP instance,
			 const char* url,
			 NPReason reason,
			 void* notifyData)
{
	if (!instance)
		return;

	totemPlugin *plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	if (!plugin)
		return;

	plugin->URLNotify (url, reason, notifyData);
}

static void
totem_plugin_print (NPP instance,
                    NPPrint* platformPrint)
{
	g_debug ("Print");
}

static int16_t
totem_plugin_handle_event (NPP instance,
                           void* event)
{
	g_debug ("Handle event");
        return FALSE;
}

static NPError
totem_plugin_get_value (NPP instance,
			NPPVariable variable,
		        void *value)
{
	totemPlugin *plugin = 0;
	NPError err = NPERR_NO_ERROR;

	if (instance) {
                plugin = reinterpret_cast<totemPlugin*> (instance->pdata);
	}

	/* See NPPVariable in npapi.h */
	switch (variable) {
	case NPPVpluginNameString:
		*((char **)value) = totemPlugin::PluginDescription ();
		break;
	case NPPVpluginDescriptionString:
		*((char **)value) = totemPlugin::PluginLongDescription ();
		break;
	case NPPVpluginNeedsXEmbed:
                // FIXMEchpe fix webkit which passes a (unsigned int*) here...
		*((NPBool *)value) = TRUE;
		break;
	case NPPVpluginScriptableIID:
	case NPPVpluginScriptableInstance:
                /* XPCOM scripting, obsolete */
                err = NPERR_GENERIC_ERROR;
		break;
	case NPPVpluginScriptableNPObject:
                if (plugin) {
                        err = plugin->GetScriptableNPObject (value);
                } else {
			err = NPERR_INVALID_PLUGIN_ERROR;
                }
		break;
	default:
		g_debug ("Unhandled variable %d instance %p", variable, plugin);
		err = NPERR_INVALID_PARAM;
		break;
	}

	return err;
}

static NPError
totem_plugin_set_value (NPP instance,
			NPNVariable variable,
			void *value)
{
	g_debug ("SetValue variable %d (%x)", variable, variable);

	return NPERR_NO_ERROR;
}

NPError
NP_GetValue (void *future,
	     NPPVariable variable,
	     void *value)
{
	return totem_plugin_get_value (NULL, variable, value);
}

static gboolean
totem_plugin_mimetype_is_disabled (const char *mimetype,
				   GKeyFile *system,
				   GKeyFile *user)
{
	GError *error = NULL;
	gboolean retval;
	char *key;

	retval = FALSE;
	key = g_strdup_printf ("%s.disabled", mimetype);

	/* If the plugin is listed as enabled or disabled explicitely,
	 * in the system-wide config file, then that's it */
	if (system != NULL) {
		retval = g_key_file_get_boolean (system, "Plugins", key, &error);
		if (error == NULL) {
			g_free (key);
			return retval;
		}
		g_error_free (error);
		error = NULL;
	}

	if (user != NULL) {
		retval = g_key_file_get_boolean (user, "Plugins", key, &error);
		if (error != NULL) {
			g_error_free (error);
			g_free (key);
			return FALSE;
		}
	}

        // FIXME g_free (key);
	return retval;
}

char *
NP_GetMIMEDescription (void)
{
	GString *list;

	if (mime_list != NULL)
		return mime_list;

	g_type_init ();

	list = g_string_new (NULL);

	/* Load the configuration files for the enabled plugins */
	GKeyFile *system, *user;

	system = g_key_file_new ();
	user = g_key_file_new ();

	if (g_key_file_load_from_file (system,
				       SYSCONFDIR"/totem/browser-plugins.ini",
				       G_KEY_FILE_NONE,
				       NULL) == FALSE) {
		g_key_file_free (system);
		system = NULL;
	}

	char *user_ini_file;
	user_ini_file = g_build_filename (g_get_user_config_dir (),
					  "totem",
					  "browser-plugins.ini",
					  NULL);
	if (g_key_file_load_from_file (user,
				       user_ini_file,
				       G_KEY_FILE_NONE,
				       NULL) == FALSE) {
		g_key_file_free (user);
		user = NULL;
	}
	g_free (user_ini_file);

	const totemPluginMimeEntry *mimetypes;
	uint32_t count;
	totemPlugin::PluginMimeTypes (&mimetypes, &count);
	for (uint32_t i = 0; i < count; ++i) {
		char *desc;

		if (totem_plugin_mimetype_is_disabled (mimetypes[i].mimetype, system, user))
			continue;

		desc = NULL;
		if (mimetypes[i].mime_alias != NULL) {
			if (strstr (mimetypes[i].mime_alias, "/") != NULL) {
				desc = g_content_type_get_description
					(mimetypes[i].mime_alias);
			} else {
				desc = g_strdup (mimetypes[i].mime_alias);
			}
		}

		if (desc == NULL)
			desc = g_content_type_get_description (mimetypes[i].mimetype);

		g_string_append_printf (list,"%s:%s:%s;",
					mimetypes[i].mimetype,
					mimetypes[i].extensions,
					desc);
		g_free (desc);
	}

	mime_list = g_string_free (list, FALSE);

	if (user != NULL)
		g_key_file_free (user);
	if (system != NULL)
		g_key_file_free (system);

	return mime_list;
}

// FIXMEchpe!!!!
typedef enum {
  NPNVGtk12 = 1,
  NPNVGtk2
} NPNToolkitType;

NPError
NP_Initialize (NPNetscapeFuncs *aMozillaVTable,
	       NPPluginFuncs *aPluginVTable)
{
	g_debug ("NP_Initialize");

	g_type_init ();

	if (aMozillaVTable == NULL || aPluginVTable == NULL)
		return NPERR_INVALID_FUNCTABLE_ERROR;

	if ((aMozillaVTable->version >> 8) > NP_VERSION_MAJOR)
		return NPERR_INCOMPATIBLE_VERSION_ERROR;

	if (aMozillaVTable->size < sizeof (NPNetscapeFuncs))
		return NPERR_INVALID_FUNCTABLE_ERROR;
	if (aPluginVTable->size < sizeof (NPPluginFuncs))
		return NPERR_INVALID_FUNCTABLE_ERROR;

        /* Copy the function table. We can use memcpy here since we've already
         * established that the aMozillaVTable is at least as big as the compile-
         * time NPNetscapeFuncs.
         */
        memcpy (&NPNFuncs, aMozillaVTable, sizeof (NPNetscapeFuncs));
        NPNFuncs.size = sizeof (NPNetscapeFuncs);
#if 0 // FIXMEchpe
	/* Do we support XEMBED? */
	NPError err;
	NPBool supportsXEmbed = 0;
	err = NPN_GetValue (NULL, NPNVSupportsXEmbedBool, (void *) &supportsXEmbed);
	if (err != NPERR_NO_ERROR || !supportsXEmbed)
		return NPERR_INCOMPATIBLE_VERSION_ERROR;

        /* Are we using a GTK+ 2.x Moz? */
	NPNToolkitType toolkit = (NPNToolkitType) 0;
	err = NPN_GetValue (NULL, NPNVToolkit, (void *) &toolkit);
	if (err != NPERR_NO_ERROR || toolkit != NPNVGtk2)
		return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
	/* we want to open libdbus-glib-1.so.2 in such a way
	 * in such a way that it becomes permanentely resident */
	void *handle;
	handle = dlopen ("libdbus-glib-1.so.2", RTLD_NOW | RTLD_NODELETE);
	if (!handle) {
		fprintf (stderr, "%s\n", dlerror()); 
		return NPERR_MODULE_LOAD_FAILED_ERROR;
	}
	/* RTLD_NODELETE allows us to close right away ... */
	dlclose(handle);

	/*
	 * Set up a plugin function table that Mozilla will use to call
	 * into us.  Mozilla needs to know about our version and size and
	 * have a UniversalProcPointer for every function we implement.
	 */

	aPluginVTable->size           = sizeof (NPPluginFuncs);
	aPluginVTable->version        = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
	aPluginVTable->newp           = NewNPP_NewProc (totem_plugin_new_instance);
	aPluginVTable->destroy        = NewNPP_DestroyProc (totem_plugin_destroy_instance);
	aPluginVTable->setwindow      = NewNPP_SetWindowProc (totem_plugin_set_window);
	aPluginVTable->newstream      = NewNPP_NewStreamProc (totem_plugin_new_stream);
	aPluginVTable->destroystream  = NewNPP_DestroyStreamProc (totem_plugin_destroy_stream);
	aPluginVTable->asfile         = NewNPP_StreamAsFileProc (totem_plugin_stream_as_file);
	aPluginVTable->writeready     = NewNPP_WriteReadyProc (totem_plugin_write_ready);
	aPluginVTable->write          = NewNPP_WriteProc (totem_plugin_write);
	aPluginVTable->print          = NewNPP_PrintProc (totem_plugin_print);
	aPluginVTable->event          = NewNPP_HandleEventProc (totem_plugin_handle_event);
	aPluginVTable->urlnotify      = NewNPP_URLNotifyProc (totem_plugin_url_notify);
	aPluginVTable->javaClass      = NULL; 
	aPluginVTable->getvalue       = NewNPP_GetValueProc (totem_plugin_get_value);
	aPluginVTable->setvalue       = NewNPP_SetValueProc (totem_plugin_set_value);

	g_debug ("NP_Initialize succeeded");

	return totemPlugin::Initialise ();
}

NPError
NP_Shutdown(void)
{
	g_debug ("NP_Shutdown");

	g_free (mime_list);
	mime_list = NULL;

	return totemPlugin::Shutdown ();
}
