/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-engine-config.c: The MateComponent UI/XML Sync engine user config code
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-sync-menu.h>
#include <matecomponent/matecomponent-ui-config-widget.h>
#include <matecomponent/matecomponent-ui-engine-config.h>
#include <matecomponent/matecomponent-ui-engine-private.h>

#include <mateconf/mateconf-client.h>

#define PARENT_TYPE G_TYPE_OBJECT

static GObjectClass *parent_class = NULL;

struct _MateComponentUIEngineConfigPrivate {
	char           *path; 

	GtkWindow      *opt_parent;

	MateComponentUIEngine *engine;
	MateComponentUIXml    *tree;

	GSList         *clobbers;

	GtkWidget      *dialog;
};

typedef struct {
	char *path;
	char *attr;
	char *value;
} clobber_t;

static void
clobber_destroy (MateComponentUIXml *tree, clobber_t *cl)
{
	if (cl) {
		matecomponent_ui_xml_remove_watch_by_data (tree, cl);

		g_free (cl->path);
		cl->path = NULL;

		g_free (cl->attr);
		cl->attr = NULL;

		g_free (cl->value);
		cl->value = NULL;

		g_free (cl);
	}
}

static void
clobbers_free (MateComponentUIEngineConfig *config)
{
	GSList *l;

	for (l = config->priv->clobbers; l; l = l->next)
		clobber_destroy (config->priv->tree, l->data);

	g_slist_free (config->priv->clobbers);
	config->priv->clobbers = NULL;
}

void
matecomponent_ui_engine_config_serialize (MateComponentUIEngineConfig *config)
{
	GSList      *l;
	GSList      *values = NULL;
	MateConfClient *client;

	g_return_if_fail (config->priv->path != NULL);

	for (l = config->priv->clobbers; l; l = l->next) {
		clobber_t   *cl = l->data;
		char        *str;

		/* This sucks, but so does mateconf */
		str = g_strconcat (cl->path, ":",
				   cl->attr, ":",
				   cl->value, NULL);

		values = g_slist_prepend (values, str);
	}

	client = mateconf_client_get_default ();
	
	mateconf_client_set_list (
		client, config->priv->path,
		MATECONF_VALUE_STRING, values, NULL);

	g_slist_foreach (values, (GFunc) g_free, NULL);
	g_slist_free (values);

	mateconf_client_suggest_sync (client, NULL);

	g_object_unref (client);
}

static void
clobber_add (MateComponentUIEngineConfig *config,
	     const char           *path,
	     const char           *attr,
	     const char           *value)
{
	clobber_t *cl = g_new0 (clobber_t, 1);

	cl->path  = g_strdup (path);
	cl->attr  = g_strdup (attr);
	cl->value = g_strdup (value);

	config->priv->clobbers = g_slist_prepend (
		config->priv->clobbers, cl);

	matecomponent_ui_xml_add_watch (config->priv->tree, path, cl);
}

void
matecomponent_ui_engine_config_add (MateComponentUIEngineConfig *config,
			     const char           *path,
			     const char           *attr,
			     const char           *value)
{
	MateComponentUINode *node;

	matecomponent_ui_engine_config_remove (config, path, attr);

	clobber_add (config, path, attr, value);

	if ((node = matecomponent_ui_xml_get_path (config->priv->tree, path))) {
		const char *existing;
		gboolean    set = TRUE;

		if ((existing = matecomponent_ui_node_peek_attr (node, attr))) {
			if (!strcmp (existing, value))
				set = FALSE;
		}

		if (set) {
			matecomponent_ui_node_set_attr (node, attr, value);
			matecomponent_ui_xml_set_dirty (config->priv->tree, node);
			matecomponent_ui_engine_update (config->priv->engine);
		}
	}
}

void
matecomponent_ui_engine_config_remove (MateComponentUIEngineConfig *config,
				const char           *path,
				const char           *attr)
{
	GSList *l, *next;
	MateComponentUINode *node;

	for (l = config->priv->clobbers; l; l = next) {
		clobber_t *cl = l->data;

		next = l->next;

		if (!strcmp (cl->path, path) &&
		    !strcmp (cl->attr, attr)) {
			config->priv->clobbers = g_slist_remove (
				config->priv->clobbers, cl);
			clobber_destroy (config->priv->tree, cl);
		}
	}

	if ((node = matecomponent_ui_xml_get_path (config->priv->tree, path))) {

		if (matecomponent_ui_node_has_attr (node, attr)) {
			matecomponent_ui_node_remove_attr (node, attr);
			matecomponent_ui_xml_set_dirty (config->priv->tree, node);
			matecomponent_ui_engine_update (config->priv->engine);
		}
	}
}

void
matecomponent_ui_engine_config_hydrate (MateComponentUIEngineConfig *config)
{
	GSList *l, *values;
	MateConfClient *client;

	g_return_if_fail (config->priv->path != NULL);

	matecomponent_ui_engine_freeze (config->priv->engine);

	clobbers_free (config);

	client = mateconf_client_get_default ();

	values = mateconf_client_get_list (
		client, config->priv->path, MATECONF_VALUE_STRING, NULL);

	for (l = values; l; l = l->next) {
		char **strs = g_strsplit (l->data, ":", -1);

		if (!strs || !strs [0] || !strs [1] || !strs [2] || strs [3])
			g_warning ("Syntax error in '%s'", (char *) l->data);
		else
			matecomponent_ui_engine_config_add (
				config, strs [0], strs [1], strs [2]);

		g_strfreev (strs);
		g_free (l->data);
	}

	g_slist_free (values);

	matecomponent_ui_engine_thaw (config->priv->engine);

	g_object_unref (client);
}

typedef struct {
	MateComponentUIEngine *engine;
	char           *path;
	MateComponentUIEngineConfigFn     config_fn;
	MateComponentUIEngineConfigVerbFn verb_fn;
} closure_t;

static void
closure_destroy (closure_t *c)
{
	g_free (c->path);
	g_free (c);
}

static void
emit_verb_on_cb (MateComponentUIEngine *engine,
		 MateComponentUINode   *popup_node,
		 closure_t      *c)
{
	if (c->verb_fn)
		c->verb_fn (matecomponent_ui_engine_get_config (c->engine),
			    c->path, NULL, engine, popup_node);
}


static void
emit_event_on_cb (MateComponentUIEngine *engine,
		  MateComponentUINode   *popup_node,
		  const char     *state,
		  closure_t      *c)
{
	if (c->verb_fn)
		c->verb_fn (matecomponent_ui_engine_get_config (c->engine),
			    c->path, state, engine, popup_node);
}

static MateComponentUIEngine *
create_popup_engine (closure_t *c,
		     GtkMenu   *menu)
{
	MateComponentUIEngine *engine;
	MateComponentUISync   *smenu;
	MateComponentUINode   *node;
	char           *str;

	engine = matecomponent_ui_engine_new (NULL);
	smenu  = matecomponent_ui_sync_menu_new (engine, NULL, NULL, NULL);

	matecomponent_ui_engine_add_sync (engine, smenu);

	node = matecomponent_ui_engine_get_path (c->engine, c->path);
	if (c->config_fn)
		str = c->config_fn (
			matecomponent_ui_engine_get_config (c->engine),
			node, engine);
	else
		str = NULL;

	g_return_val_if_fail (str != NULL, NULL);

	node = matecomponent_ui_node_from_string (str);
	matecomponent_ui_util_translate_ui (node);
	matecomponent_ui_engine_xml_merge_tree (
		engine, "/", node, "popup");

	matecomponent_ui_sync_menu_add_popup (
		MATECOMPONENT_UI_SYNC_MENU (smenu),
		menu, "/popups/popup");

	g_signal_connect (G_OBJECT (engine),
			  "emit_verb_on",
			  (GCallback) emit_verb_on_cb, c);

	g_signal_connect (G_OBJECT (engine),
			  "emit_event_on",
			  (GCallback) emit_event_on_cb, c);

	matecomponent_ui_engine_update (engine);

	return engine;
}

static int
config_button_pressed (GtkWidget      *widget,
		       GdkEventButton *event,
		       closure_t      *c)
{
	if (event->button == 3) {
		GtkWidget *menu;

		menu = gtk_menu_new ();

		create_popup_engine (c, GTK_MENU (menu));

		gtk_widget_show (GTK_WIDGET (menu));

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				NULL, NULL, 3, 0);

		return TRUE;
	} else
		return FALSE;
}

void
matecomponent_ui_engine_config_connect (GtkWidget      *widget,
				 MateComponentUIEngine *engine,
				 const char     *path,
				 MateComponentUIEngineConfigFn     config_fn,
				 MateComponentUIEngineConfigVerbFn verb_fn)
{
	MateComponentUIEngineConfig *config;
	closure_t *c;

	config = matecomponent_ui_engine_get_config (engine);
	if (!config || !config->priv->path)
		return;

	c = g_new0 (closure_t, 1);
	c->engine    = engine;
	c->path      = g_strdup (path);
	c->config_fn = config_fn;
	c->verb_fn   = verb_fn;

	g_signal_connect_data (
		widget, "button_press_event",
		G_CALLBACK (config_button_pressed),
		c,
		(GClosureNotify) closure_destroy, 0);
}

static void
matecomponent_ui_engine_config_watch (MateComponentUIXml    *xml,
			       const char     *path,
			       MateComponentUINode   *opt_node,
			       gpointer        user_data)
{
	clobber_t *cl = user_data;

	if (opt_node) {
/*		g_warning ("Setting attr '%s' to '%s' on '%s",
		cl->attr, cl->value, path);*/
		matecomponent_ui_node_set_attr (opt_node, cl->attr, cl->value);
	} else
		g_warning ("Stamp new config data onto NULL @ '%s'", path);
}

static void
impl_finalize (GObject *object)
{
	MateComponentUIEngineConfig *config;
	MateComponentUIEngineConfigPrivate *priv;

	config = MATECOMPONENT_UI_ENGINE_CONFIG (object);
	priv = config->priv;

	if (priv->dialog)
		gtk_widget_destroy (priv->dialog);

	g_free (priv->path);

	clobbers_free (config);

	g_free (priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (MateComponentUIEngineClass *engine_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (engine_class);

	object_class = G_OBJECT_CLASS (engine_class);

	object_class->finalize = impl_finalize;
}

static void
init (MateComponentUIEngineConfig *config)
{
	MateComponentUIEngineConfigPrivate *priv;

	priv = g_new0 (MateComponentUIEngineConfigPrivate, 1);

	config->priv = priv;
}

GType
matecomponent_ui_engine_config_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (MateComponentUIEngineConfigClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUIEngineConfig),
			0, /* n_preallocs */
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUIEngineConfig",
					       &info, 0);
	}

	return type;
}

MateComponentUIEngineConfig *
matecomponent_ui_engine_config_construct (MateComponentUIEngineConfig *config,
				   MateComponentUIEngine       *engine,
				   GtkWindow            *opt_parent)
{
	config->priv->engine = engine;
	config->priv->tree   = matecomponent_ui_engine_get_xml (engine);
	config->priv->opt_parent = opt_parent;

	matecomponent_ui_xml_set_watch_fn (
		matecomponent_ui_engine_get_xml (engine),
		matecomponent_ui_engine_config_watch);

	return config;
}

MateComponentUIEngineConfig *
matecomponent_ui_engine_config_new (MateComponentUIEngine *engine,
			     GtkWindow      *opt_parent)
{
	MateComponentUIEngineConfig *config;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	config = g_object_new (matecomponent_ui_engine_config_get_type (), NULL);

	return matecomponent_ui_engine_config_construct (config, engine, opt_parent);
}

void
matecomponent_ui_engine_config_set_path (MateComponentUIEngine *engine,
				  const char     *path)
{
	MateComponentUIEngineConfig *config;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	config = matecomponent_ui_engine_get_config (engine);

	g_free (config->priv->path);
	config->priv->path = g_strdup (path);

	matecomponent_ui_engine_config_hydrate (config);
}

const char *
matecomponent_ui_engine_config_get_path (MateComponentUIEngine *engine)
{
	MateComponentUIEngineConfig *config;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	config = matecomponent_ui_engine_get_config (engine);
	
	return config->priv->path;
}

static void
response_fn (GtkDialog            *dialog,
	     gint                  response_id,
	     MateComponentUIEngineConfig *config)
{
	matecomponent_ui_engine_config_serialize (config);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
dialog_new (MateComponentUIEngineConfig *config)
{
	GtkAccelGroup *accel_group;
	GtkWidget     *window, *cwidget;

	accel_group = gtk_accel_group_new ();

	window = gtk_dialog_new_with_buttons (_("Configure UI"), 
					      config->priv->opt_parent, 0,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);

	g_signal_connect (window, "response",
			  G_CALLBACK (response_fn), config);

	cwidget = matecomponent_ui_config_widget_new (config->priv->engine, accel_group);
	gtk_widget_show (cwidget);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), cwidget);

	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
	
	return window;
}

static void
null_dialog (GtkObject *object, 
	     MateComponentUIEngineConfig *config)
{
	config->priv->dialog = NULL;
}

void
matecomponent_ui_engine_config_configure (MateComponentUIEngineConfig *config)
{
	if (!config->priv->path)
		return;

	/* Fire up a single non-modal dialog */
	if (config->priv->dialog) {
		gtk_window_activate_focus (
			GTK_WINDOW (config->priv->dialog));
		return;
	}

	config->priv->dialog = dialog_new (config);
	gtk_window_set_default_size (
		GTK_WINDOW (config->priv->dialog), 300, 300);
	gtk_widget_show (config->priv->dialog);
	g_signal_connect (config->priv->dialog,
			  "destroy", G_CALLBACK (null_dialog), config);
}

MateComponentUIEngine *
matecomponent_ui_engine_config_get_engine (MateComponentUIEngineConfig *config)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE_CONFIG (config), NULL);

	return config->priv->engine;
}
