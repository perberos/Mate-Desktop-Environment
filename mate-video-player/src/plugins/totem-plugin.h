/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * heavily based on code from Rhythmbox and Gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __TOTEM_PLUGIN_H__
#define __TOTEM_PLUGIN_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "totem.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define TOTEM_TYPE_PLUGIN              (totem_plugin_get_type())
#define TOTEM_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), TOTEM_TYPE_PLUGIN, TotemPlugin))
#define TOTEM_PLUGIN_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), TOTEM_TYPE_PLUGIN, TotemPlugin const))
#define TOTEM_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), TOTEM_TYPE_PLUGIN, TotemPluginClass))
#define TOTEM_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), TOTEM_TYPE_PLUGIN))
#define TOTEM_IS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_PLUGIN))
#define TOTEM_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), TOTEM_TYPE_PLUGIN, TotemPluginClass))

/**
 * TotemPlugin:
 *
 * All the fields in the #TotemPlugin structure are private and should never be accessed directly.
 **/
typedef struct {
	GObject parent;
} TotemPlugin;

/**
 * TotemPluginActivationFunc:
 * @plugin: the #TotemPlugin
 * @totem: a #TotemObject
 * @error: a #GError
 *
 * Called when the user has requested @plugin be activated, this function should be used to initialise
 * any resources the plugin needs, and attach itself to the Totem UI.
 *
 * If an error is encountered while setting up the plugin, @error should be set, and the function
 * should return %FALSE. Totem will then not mark the plugin as activated, and will ensure it's not loaded
 * again unless explicitly asked for by the user.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
typedef gboolean	(*TotemPluginActivationFunc)		(TotemPlugin *plugin, TotemObject *totem,
								 GError **error);

/**
 * TotemPluginDeactivationFunc:
 * @plugin: the #TotemPlugin
 * @totem: a #TotemObject
 *
 * Called when the user has requested @plugin be deactivated, this function should destroy all resources
 * created during the plugin's lifetime, especially those created in the activation function.
 *
 * It should be possible to activate and deactivate the plugin multiple times sequentially in a single Totem
 * session without memory or resource leaks, or errors.
 **/
typedef void		(*TotemPluginDeactivationFunc)		(TotemPlugin *plugin, TotemObject *totem);

/**
 * TotemPluginWidgetFunc:
 * @plugin: the #TotemPlugin
 *
 * Called when the configuration dialogue for the plugin needs to be built, this function should return
 * a complete window which will be shown by the Totem code. The widget needs to be capable of hiding itself
 * when configuration is complete.
 *
 * If your plugin is not configurable, do not define this function.
 *
 * Return value: a #GtkWidget
 **/
typedef GtkWidget *	(*TotemPluginWidgetFunc)		(TotemPlugin *plugin);
typedef gboolean	(*TotemPluginBooleanFunc)		(TotemPlugin *plugin);

/**
 * TotemPluginClass:
 * @parent_class: the parent class
 * @activate: function called when activating a plugin using totem_plugin_activate().
 * It must be set by inheriting classes, and should return %TRUE if it successfully created/got handles to
 * the resources needed by the plugin. If it returns %FALSE, loading the plugin is abandoned.
 * @deactivate: function called when deactivating a plugin using totem_plugin_deactivate();
 * It must be set by inheriting classes, and should free/unref any resources the plugin used.
 * @create_configure_dialog: function called when configuring a plugin using totem_plugin_create_configure_dialog().
 * If non-%NULL, it should create and return the plugin's configuration dialog. If %NULL, the plugin is not
 * configurable.
 *
 * The class structure for the #TotemPlParser type.
 **/
typedef struct {
	GObjectClass parent_class;

	/* Virtual public methods */

	TotemPluginActivationFunc	activate;
	TotemPluginDeactivationFunc	deactivate;
	TotemPluginWidgetFunc		create_configure_dialog;

	/*< private >*/
	/* Plugins should not override this, it's handled automatically by
	   the TotemPluginClass */
	TotemPluginBooleanFunc		is_configurable;
} TotemPluginClass;

/**
 * TotemPluginError:
 * @TOTEM_PLUGIN_ERROR_ACTIVATION: there was an error activating the plugin
 *
 * Error codes returned by #TotemPlugin operations.
 **/
typedef enum {
	TOTEM_PLUGIN_ERROR_ACTIVATION
} TotemPluginError;

typedef struct TotemPluginPrivate	TotemPluginPrivate;

GType totem_plugin_error_get_type	(void);
GQuark totem_plugin_error_quark 	(void);
#define TOTEM_TYPE_PLUGIN_ERROR		(totem_remote_command_get_type())
#define TOTEM_PLUGIN_ERROR		(totem_plugin_error_quark ())

/*
 * Public methods
 */
GType 		 totem_plugin_get_type 		(void) G_GNUC_CONST;

gboolean	 totem_plugin_activate		(TotemPlugin *plugin,
						 TotemObject *totem,
						 GError **error);
void 		 totem_plugin_deactivate	(TotemPlugin *plugin,
						 TotemObject *totem);

gboolean	 totem_plugin_is_configurable	(TotemPlugin *plugin);
GtkWidget	*totem_plugin_create_configure_dialog
						(TotemPlugin *plugin);

char *		 totem_plugin_find_file		(TotemPlugin *plugin,
						 const char *file);

GtkBuilder *     totem_plugin_load_interface    (TotemPlugin *plugin,
						 const char *name,
						 gboolean fatal,
						 GtkWindow *parent,
						 gpointer user_data);

GList *          totem_get_plugin_paths            (void);

/**
 * TOTEM_PLUGIN_REGISTER:
 * @PluginName: the plugin's name in camelcase
 * @plugin_name: the plugin's name in lowercase, with underscores
 *
 * Registers a new Totem plugin type. A plugin is, at its core, just a class which is
 * instantiated and activated on the user's request. This macro registers that class.
 **/
#define TOTEM_PLUGIN_REGISTER(PluginName, plugin_name)				\
	TOTEM_PLUGIN_REGISTER_EXTENDED(PluginName, plugin_name, {})

/**
 * TOTEM_PLUGIN_REGISTER_EXTENDED:
 * @PluginName: the plugin's name in camelcase
 * @plugin_name: the plugin's name in lowercase, with underscores
 * @_C_: extra code to call in the module type registration function
 *
 * Registers a new Totem plugin type with custom code in the module type registration
 * function. See TOTEM_PLUGIN_REGISTER() for more information about the registration
 * process.
 *
 * A variable named @our_info is available with the module's #GTypeInfo information.
 * @plugin_module_type is the plugin's #GTypeModule.
 * @<replaceable>plugin_name</replaceable>_type is the plugin's newly-registered #GType
 * (where <replaceable>plugin_name</replaceable> is the plugin name passed to the
 * TOTEM_PLUGIN_REGISTER_EXTENDED() macro).
 **/
#define TOTEM_PLUGIN_REGISTER_EXTENDED(PluginName, plugin_name, _C_)		\
	_TOTEM_PLUGIN_REGISTER_EXTENDED_BEGIN (PluginName, plugin_name) {_C_;} _TOTEM_PLUGIN_REGISTER_EXTENDED_END(plugin_name)

#define _TOTEM_PLUGIN_REGISTER_EXTENDED_BEGIN(PluginName, plugin_name)		\
										\
static GType plugin_name##_type = 0;						\
static GTypeModule *plugin_module_type = NULL;					\
										\
GType										\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void     plugin_name##_init              (PluginName        *self);	\
static void     plugin_name##_class_init        (PluginName##Class *klass);	\
static gpointer plugin_name##_parent_class = NULL;				\
static void     plugin_name##_class_intern_init (gpointer klass)		\
{										\
	plugin_name##_parent_class = g_type_class_peek_parent (klass);		\
	plugin_name##_class_init ((PluginName##Class *) klass);			\
}										\
										\
G_MODULE_EXPORT GType								\
register_totem_plugin (GTypeModule *module)					\
{										\
	const GTypeInfo our_info =						\
	{									\
		sizeof (PluginName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) plugin_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (PluginName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) plugin_name##_init,				\
		NULL								\
	};									\
										\
	/* Initialise the i18n stuff */						\
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);			\
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");			\
										\
	plugin_module_type = module;						\
	plugin_name##_type = g_type_module_register_type (module,		\
					    TOTEM_TYPE_PLUGIN,			\
					    #PluginName,			\
					    &our_info,				\
					    0);					\
	{ /* custom code follows */

#define _TOTEM_PLUGIN_REGISTER_EXTENDED_END(plugin_name)			\
		/* following custom code */					\
	}									\
	return plugin_name##_type;						\
}

/**
 * TOTEM_PLUGIN_REGISTER_TYPE:
 * @type_name: the type's name in lowercase, with underscores
 *
 * Calls the type registration function for a type inside a plugin module previously
 * defined with TOTEM_PLUGIN_DEFINE_TYPE().
 **/
#define TOTEM_PLUGIN_REGISTER_TYPE(type_name)					\
	type_name##_register_type (plugin_module_type)

/**
 * TOTEM_PLUGIN_DEFINE_TYPE:
 * @TypeName: the type name in camelcase
 * @type_name: the type name in lowercase, with underscores
 * @TYPE_PARENT: the type's parent name in uppercase, with underscores
 *
 * Registers a type to be used inside a Totem plugin, but not the plugin's itself;
 * use TOTEM_PLUGIN_REGISTER() for that.
 **/
#define TOTEM_PLUGIN_DEFINE_TYPE(TypeName, type_name, TYPE_PARENT)		\
static void type_name##_init (TypeName *self); 					\
static void type_name##_class_init (TypeName##Class *klass); 			\
static gpointer type_name##_parent_class = ((void *)0); 			\
static GType type_name##_type_id = 0;						\
										\
static void 									\
type_name##_class_intern_init (gpointer klass) 					\
{ 										\
	type_name##_parent_class = g_type_class_peek_parent (klass);		\
	type_name##_class_init ((TypeName##Class*) klass); 			\
}										\
										\
										\
GType 										\
type_name##_get_type (void)							\
{										\
	g_assert (type_name##_type_id != 0);					\
										\
	return type_name##_type_id;						\
}										\
										\
GType 										\
type_name##_register_type (GTypeModule *module) 				\
{ 										\
										\
	const GTypeInfo g_define_type_info = { 					\
		sizeof (TypeName##Class), 					\
		(GBaseInitFunc) ((void *)0), 					\
		(GBaseFinalizeFunc) ((void *)0), 				\
		(GClassInitFunc) type_name##_class_intern_init, 		\
		(GClassFinalizeFunc) ((void *)0), 				\
		((void *)0), 							\
		sizeof (TypeName), 						\
		0, 								\
		(GInstanceInitFunc) type_name##_init,				\
		((void *)0) 							\
	}; 									\
	type_name##_type_id = 							\
		g_type_module_register_type (module, 				\
					     TYPE_PARENT, 			\
					     #TypeName,				\
					     &g_define_type_info, 		\
					     (GTypeFlags) 0); 			\
										\
	return type_name##_type_id;						\
}

G_END_DECLS

#endif  /* __TOTEM_PLUGIN_H__ */
