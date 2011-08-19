#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-storage.h>
#include "config.h"

#include "document.h"

struct _SampleDocPrivate {
	GList 		*components;
	gchar 		*filename;
	MateComponentStorage 	*storage;
	gboolean 	 is_dirty;
};

static GObjectClass *doc_parent_class;

enum SIGNALS {
	COMPONENT_ADDED,
	COMPONENT_REMOVED,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };

void
sample_doc_finalize (GObject *obj)
{
	SampleDoc *doc = SAMPLE_DOC (obj);
	GList *l;

	for (l = doc->priv->components; l; l = l->next) {
		g_object_unref (G_OBJECT (l->data));
	}

	g_list_free (doc->priv->components);

	if (doc->priv->storage)
		matecomponent_object_unref (doc->priv->storage);

	g_free (doc->priv->filename);
	g_free (doc->priv);

	doc_parent_class->finalize (obj);
}

static void
sample_doc_class_init (GObjectClass *klass)
{
/*	SampleDocClass *doc_class = SAMPLE_DOC_CLASS (klass); */

	doc_parent_class = g_type_class_peek_parent (klass);

	signals [COMPONENT_ADDED] = g_signal_new (
		"component_added", G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST, 
		G_STRUCT_OFFSET (SampleDocClass, component_added),
		NULL, NULL, g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE, 1, G_TYPE_POINTER);

	signals [COMPONENT_REMOVED] = g_signal_new (
		"component_removed", G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST, 
		G_STRUCT_OFFSET (SampleDocClass, component_removed),
		NULL, NULL, g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE, 1, G_TYPE_POINTER);

	klass->finalize = sample_doc_finalize;
}

static void
sample_doc_instance_init (GObject *obj)
{
	SampleDoc *doc = SAMPLE_DOC (obj);

	doc->priv = g_new0 (SampleDocPrivate, 1);
}

GType
sample_doc_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (SampleDocClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sample_doc_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (SampleDoc),
			0, /* n_preallocs */
			(GInstanceInitFunc) sample_doc_instance_init
		};
		
		type = g_type_register_static (G_TYPE_OBJECT, "SampleDoc",
					       &info, 0);
	}

	return type;
}

SampleDoc *
sample_doc_new (void)
{
	SampleDoc *doc = g_object_new (SAMPLE_DOC_TYPE, NULL);

	return doc;
}

gchar *
sample_doc_get_filename (SampleDoc *doc)
{
	g_return_val_if_fail (SAMPLE_IS_DOC (doc), NULL);

	return g_strdup (doc->priv->filename);
}

gboolean
sample_doc_is_dirty (SampleDoc *doc)
{
	GList *l;

	g_return_val_if_fail (SAMPLE_IS_DOC (doc), FALSE);

	if (doc->priv->is_dirty)
		return TRUE;

	for (l = doc->priv->components; l; l = l->next) {
		SampleComponent *comp = (SampleComponent *) l->data;

		if (!comp)
			continue;

		if (sample_component_is_dirty (comp))
			return TRUE;
	}

	return FALSE;
}

void
sample_doc_add_component (SampleDoc *doc, gchar *iid)
{
	SampleComponent *comp;

	g_return_if_fail (SAMPLE_IS_DOC (doc));

	doc->priv->is_dirty = TRUE;

	comp = sample_component_new (iid);

	doc->priv->components = g_list_prepend (doc->priv->components, comp);

	g_signal_emit (G_OBJECT (doc), signals [COMPONENT_ADDED], 0, comp);
}

GList *
sample_doc_get_components (SampleDoc *doc)
{
	return g_list_copy (doc->priv->components);
}

void
sample_doc_load (SampleDoc *doc, const gchar *filename)
{
     g_warning ("No file load yet.");
}

void
sample_doc_save (SampleDoc *doc, const gchar *filename)
{
     g_warning ("No file save yet.");
}
