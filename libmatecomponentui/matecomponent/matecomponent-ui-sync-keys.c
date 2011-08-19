/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-sync-keys.h: The MateComponent UI/XML sync engine for keys bits.
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>

#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-sync.h>
#include <matecomponent/matecomponent-ui-sync-keys.h>
#include <matecomponent/matecomponent-ui-private.h>

static GQuark accel_id = 0;
static GQuark keybindings_id = 0;

static MateComponentUISyncClass *parent_class = NULL;

#define PARENT_TYPE matecomponent_ui_sync_get_type ()

#define	BINDING_MOD_MASK()				\
	(gtk_accelerator_get_default_mod_mask () | GDK_RELEASE_MASK)

typedef struct {
	guint           key;
	GdkModifierType mods;
	MateComponentUINode        *node;
} Binding;

static gboolean
keybindings_free (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	g_free (key);

	return TRUE;
}

/* Shamelessly stolen from gtkbindings.c */
static guint
keybinding_hash_fn (gconstpointer  key)
{
	register const Binding *e = key;
	register guint h;

	h = e->key;
	h ^= e->mods;

	return h;
}

static gint
keybinding_compare_fn (gconstpointer a,
		       gconstpointer b)
{
	register const Binding *ba = a;
	register const Binding *bb = b;

	return (ba->key == bb->key && ba->mods == bb->mods);
}

gint
matecomponent_ui_sync_keys_binding_handle (GtkWidget        *widget,
				    GdkEventKey      *event,
				    MateComponentUISyncKeys *msync)
{
	Binding lookup, *binding;

	lookup.key  = gdk_keyval_to_lower (event->keyval);
	lookup.mods = event->state & BINDING_MOD_MASK ();

	if (!(binding = g_hash_table_lookup (
		msync->keybindings, &lookup)))

		return FALSE;
	else {
		matecomponent_ui_engine_emit_verb_on (
			msync->parent.engine, binding->node);

		return TRUE;
	}
}

static void
impl_finalize (GObject *object)
{
	MateComponentUISyncKeys *sync;

	sync = MATECOMPONENT_UI_SYNC_KEYS (object);

	g_hash_table_foreach_remove (sync->keybindings,
				     keybindings_free, NULL);
	g_hash_table_destroy (sync->keybindings);
	sync->keybindings = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
update_keybindings (MateComponentUISyncKeys *msync,
		    MateComponentUINode     *node)
{
	MateComponentUINode    *l;
	MateComponentUIXmlData *data;

	if (!matecomponent_ui_engine_node_is_dirty (msync->parent.engine, node))
		return;

	data = matecomponent_ui_xml_get_data (NULL, node);
	g_return_if_fail (data != NULL);

	if (!data->dirty)
		return;


	g_hash_table_foreach_remove (
		msync->keybindings, keybindings_free, NULL);

	for (l = matecomponent_ui_node_children (node); l;
	     l = matecomponent_ui_node_next (l)) {
		guint           key;
		GdkModifierType mods;
		const char     *name;
		Binding        *binding;
		
		name = matecomponent_ui_node_peek_attr (l, "name");
		if (!name)
			continue;
		
		matecomponent_ui_util_accel_parse (name, &key, &mods);

		binding       = g_new0 (Binding, 1);
		binding->mods = mods & BINDING_MOD_MASK ();
		binding->key  = gdk_keyval_to_lower (key);
		binding->node = l;

		g_hash_table_insert (msync->keybindings, binding, binding);
	}
}

static void
impl_matecomponent_ui_sync_keys_update_root (MateComponentUISync *sync,
				      MateComponentUINode *root)
{
	if (matecomponent_ui_node_has_name (root, "keybindings"))
		update_keybindings (MATECOMPONENT_UI_SYNC_KEYS (sync), root);
}

static void
impl_matecomponent_ui_sync_keys_stamp_root (MateComponentUISync *sync)
{
	MateComponentUINode *node;

	node = matecomponent_ui_engine_get_path (sync->engine, "/keybindings");

	if (node)
		matecomponent_ui_engine_node_set_dirty (sync->engine, node, TRUE);
}

static gboolean
impl_matecomponent_ui_sync_keys_can_handle (MateComponentUISync *sync,
				     MateComponentUINode *node)
{
	if (!accel_id) {
		accel_id = g_quark_from_static_string ("accel");
		keybindings_id = g_quark_from_static_string ("keybindings");
	}

	return (node->name_id == accel_id ||
		node->name_id == keybindings_id);
}

/* We need to map the shell to the item */

static void
class_init (MateComponentUISyncClass *sync_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (sync_class);

	object_class = G_OBJECT_CLASS (sync_class);
	object_class->finalize    = impl_finalize;

	sync_class->update_root   = impl_matecomponent_ui_sync_keys_update_root;
	sync_class->can_handle    = impl_matecomponent_ui_sync_keys_can_handle;
	sync_class->stamp_root    = impl_matecomponent_ui_sync_keys_stamp_root;
}

static void
init (MateComponentUISyncKeys *msync)
{
	msync->keybindings = g_hash_table_new (
		keybinding_hash_fn, keybinding_compare_fn);	
}

GType
matecomponent_ui_sync_keys_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUISyncKeysClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUISyncKeys),
			0, /* n_preallocs */
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUISyncKeys",
					       &info, 0);
	}

	return type;
}

MateComponentUISync *
matecomponent_ui_sync_keys_new (MateComponentUIEngine *engine)
{
	MateComponentUISyncKeys *sync;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	sync = g_object_new (MATECOMPONENT_TYPE_UI_SYNC_KEYS, NULL);

	return matecomponent_ui_sync_construct (
		MATECOMPONENT_UI_SYNC (sync), engine, FALSE, FALSE);
}
