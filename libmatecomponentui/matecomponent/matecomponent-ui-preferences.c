/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-preferences.c: private wrappers for global UI preferences.
 *
 * Authors:
 *     Michael Meeks (michael@ximian.com)
 *     Martin Baulig (martin@home-of-linux.org)
 *
 * Copyright 2001 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>
#include <libmate/mate-mateconf.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-preferences.h>

#define GLOBAL_INTERFACE_KEY "/desktop/mate/interface"

static MateConfEnumStringPair toolbar_styles[] = {
        { GTK_TOOLBAR_TEXT,        "text" },
        { GTK_TOOLBAR_ICONS,       "icons" },
        { GTK_TOOLBAR_BOTH,        "both" },
        { GTK_TOOLBAR_BOTH_HORIZ,  "both_horiz" },
	{ GTK_TOOLBAR_BOTH_HORIZ,  "both-horiz" },
	{ -1, NULL }
};

static MateConfClient *client;
static GSList *engine_list;
static guint desktop_notify_id;
static guint update_engines_idle_id;

static void keys_changed_fn (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, gpointer user_data);

static gboolean
update_engines_idle_callback (gpointer data)
{
	MateComponentUINode *root_node;
	MateComponentUIEngine *engine;
	GSList *node;

	if (update_engines_idle_id == 0)
		return FALSE;
	
	for (node = engine_list; node; node = node->next) {
		engine = node->data;
		
		root_node = matecomponent_ui_engine_get_path (engine, "/");

		matecomponent_ui_engine_dirty_tree (engine, root_node);
	}

	update_engines_idle_id = 0;

	return FALSE;
}

void
matecomponent_ui_preferences_add_engine (MateComponentUIEngine *engine)
{
	if (!client)
		client = mateconf_client_get_default ();
	
	if (engine_list == NULL) {
		/* We need to intialize the notifiers */
		mateconf_client_add_dir (client, GLOBAL_INTERFACE_KEY, MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
		
		desktop_notify_id = mateconf_client_notify_add (client, GLOBAL_INTERFACE_KEY,
							     keys_changed_fn,
							     NULL, NULL, NULL);
	}

	engine_list = g_slist_prepend (engine_list, engine);
}

void
matecomponent_ui_preferences_remove_engine (MateComponentUIEngine *engine)
{
	if (!g_slist_find (engine_list, engine))
		return;

	engine_list = g_slist_remove (engine_list, engine);

	if (engine_list == NULL) {
		/* Remove notification */
		mateconf_client_remove_dir (client, GLOBAL_INTERFACE_KEY, NULL);
		mateconf_client_notify_remove (client, desktop_notify_id);
		desktop_notify_id = 0;
	}
}


/*
 *   Yes Gconf's C API sucks, yes matecomponent-config is a far better
 * way to access configuration, yes I hate this code; Michael.
 */
static gboolean
get (const char *key, gboolean def)
{
	gboolean ret;
	GError  *err = NULL;

	if (!client)					
		client = mateconf_client_get_default ();	

	ret = mateconf_client_get_bool (client, key, &err);

	if (err) {
		static int warned = 0;
		if (!warned++)
			g_warning ("Failed to get '%s': '%s'", key, err->message);
		g_error_free (err);
		ret = def;
	}

	return ret;
}

int
matecomponent_ui_preferences_shutdown (void)
{
	int ret = 0;

	if (client) {
		g_object_unref (client);
		client = NULL;
	}

	ret = mateconf_debug_shutdown ();
	if (ret)
		g_warning ("MateConf's dirty shutdown");

	return ret;
}

#define DEFINE_MATECOMPONENT_UI_PREFERENCE(c_name, prop_name, def)      \
static gboolean cached_## c_name;                                \
\
gboolean                                                         \
matecomponent_ui_preferences_get_ ## c_name (void)                      \
{                                                                \
	static gboolean value;					 \
                                                                 \
        if (!cached_##c_name) {                                  \
		value = get ("/desktop/mate/interface/" prop_name, def); \
                cached_## c_name = TRUE;                                  \
	}                                                                 \
	return value;							  \
}


MateComponentUIToolbarStyle
matecomponent_ui_preferences_get_toolbar_style (void)
{
	MateComponentUIToolbarStyle style;
	char *str;

	if (!client)
		client = mateconf_client_get_default ();

	style = GTK_TOOLBAR_BOTH;

	str = mateconf_client_get_string (client,
				       "/desktop/mate/interface/toolbar_style",
				       NULL);
	
	if (str != NULL) {
		gint intstyle;

		mateconf_string_to_enum (toolbar_styles,
				      str, &intstyle);

		g_free (str);
		style = intstyle;
	}

	return style;
}

DEFINE_MATECOMPONENT_UI_PREFERENCE (toolbar_detachable, "toolbar_detachable", TRUE)
DEFINE_MATECOMPONENT_UI_PREFERENCE (menus_have_icons,   "menus_have_icons",   TRUE)
DEFINE_MATECOMPONENT_UI_PREFERENCE (menus_have_tearoff, "menus_have_tearoff", FALSE)
DEFINE_MATECOMPONENT_UI_PREFERENCE (menubar_detachable, "menubar_detachable", TRUE)


static void
keys_changed_fn (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, gpointer user_data)
{
	const char *key_name;

	key_name = mateconf_entry_get_key (entry);
	g_return_if_fail (key_name != NULL);

	/* FIXME: update the values instead */
	if (!strcmp (key_name, GLOBAL_INTERFACE_KEY "/toolbar_detachable"))
		cached_toolbar_detachable = FALSE;
	else if (!strcmp (key_name, GLOBAL_INTERFACE_KEY "/menus_have_icons"))
		cached_menus_have_icons = FALSE;
	else if (!strcmp (key_name, GLOBAL_INTERFACE_KEY "/menus_have_tearoff"))
		cached_menus_have_tearoff = FALSE;
	else if (!strcmp (key_name, GLOBAL_INTERFACE_KEY "/menubar_detachable"))
		cached_menubar_detachable = FALSE;

	if (update_engines_idle_id != 0)
		return;

	update_engines_idle_id = g_idle_add (update_engines_idle_callback, NULL);
}
