#ifndef __EOG_FULLSCREEN_PLUGIN_H__
#define __EOG_FULLSCREEN_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <eog-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define EOG_TYPE_FULLSCREEN_PLUGIN		(eog_fullscreen_plugin_get_type ())
#define EOG_FULLSCREEN_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_FULLSCREEN_PLUGIN, EogFullscreenPlugin))
#define EOG_FULLSCREEN_PLUGIN_CLASS(k)		G_TYPE_CHECK_CLASS_CAST((k),      EOG_TYPE_FULLSCREEN_PLUGIN, EogFullscreenPluginClass))
#define EOG_IS_FULLSCREEN_PLUGIN(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_FULLSCREEN_PLUGIN))
#define EOG_IS_FULLSCREEN_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k),    EOG_TYPE_FULLSCREEN_PLUGIN))
#define EOG_FULLSCREEN_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o),  EOG_TYPE_FULLSCREEN_PLUGIN, EogFullscreenPluginClass))

/* Private structure type */
typedef struct _EogFullscreenPluginPrivate	EogFullscreenPluginPrivate;

/*
 * Main object structure
 */
typedef struct _EogFullscreenPlugin		EogFullscreenPlugin;

struct _EogFullscreenPlugin
{
	EogPlugin parent_instance;
};

/*
 * Class definition
 */
typedef struct _EogFullscreenPluginClass	EogFullscreenPluginClass;

struct _EogFullscreenPluginClass
{
	EogPluginClass parent_class;
};

/*
 * Public methods
 */
GType	eog_fullscreen_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_eog_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __EOG_FULLSCREEN_PLUGIN_H__ */
