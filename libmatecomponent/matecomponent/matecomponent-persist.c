/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-persist.c: a persistance interface
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Ximian, Inc.
 */
#include <config.h>
#include <string.h>
#include <glib-object.h>
#include <gobject/gmarshal.h>
#include <matecomponent/matecomponent-persist.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

/* Parent object class */
static GObjectClass *matecomponent_persist_parent_class;

#define CLASS(o) MATECOMPONENT_PERSIST_CLASS(G_OBJECT_GET_CLASS (o))

struct _MateComponentPersistPrivate
{
	gchar *iid;
	gboolean dirty;
};

static inline MateComponentPersist *
matecomponent_persist_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_PERSIST (matecomponent_object_from_servant (servant));
}

static MateComponent_Persist_ContentTypeList *
impl_MateComponent_Persist_getContentTypes (PortableServer_Servant servant,
				     CORBA_Environment     *ev)
{
	MateComponentPersist *persist = matecomponent_persist_from_servant (servant);

	return CLASS (persist)->get_content_types (persist, ev);
}

static CORBA_char*
impl_MateComponent_Persist_getIId (PortableServer_Servant   servant,
			    CORBA_Environment       *ev)
{
	MateComponentPersist *persist = matecomponent_persist_from_servant (servant);

	return CORBA_string_dup (persist->priv->iid);
}

static CORBA_boolean
impl_MateComponent_Persist_isDirty (PortableServer_Servant   servant,
			     CORBA_Environment       *ev)
{
	MateComponentPersist *persist = matecomponent_persist_from_servant (servant);

	return persist->priv->dirty;
}

static void
matecomponent_persist_finalize (GObject *object)
{
	MateComponentPersist *persist = MATECOMPONENT_PERSIST (object);

	if (persist->priv)
	{
		g_free (persist->priv->iid);
		g_free (persist->priv);
		persist->priv = NULL;
	}
	
	matecomponent_persist_parent_class->finalize (object);
}

static void
matecomponent_persist_class_init (MateComponentPersistClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_Persist__epv *epv = &klass->epv;

	matecomponent_persist_parent_class = g_type_class_peek_parent (klass);

	/* Override and initialize methods */
	object_class->finalize = matecomponent_persist_finalize;

	epv->getContentTypes = impl_MateComponent_Persist_getContentTypes;
	epv->getIId = impl_MateComponent_Persist_getIId;
	epv->isDirty = impl_MateComponent_Persist_isDirty;
}

static void
matecomponent_persist_init (GObject *object)
{
	MateComponentPersist *persist = MATECOMPONENT_PERSIST (object);
	persist->priv = g_new0 (MateComponentPersistPrivate, 1);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentPersist, 
		       MateComponent_Persist,
		       PARENT_TYPE,
		       matecomponent_persist)

/**
 * matecomponent_persist_generate_content_types:
 * @num: the number of content types specified
 * @...: the content types (as strings)
 *
 * Returns: a ContentTypeList containing the given ContentTypes
 **/
MateComponent_Persist_ContentTypeList *
matecomponent_persist_generate_content_types (int num, ...)
{
	MateComponent_Persist_ContentTypeList *types;
	va_list ap;
	char *type;
	int i;

	types = MateComponent_Persist_ContentTypeList__alloc ();
	CORBA_sequence_set_release (types, TRUE);
	types->_length = types->_maximum = num;
	types->_buffer = CORBA_sequence_MateComponent_Persist_ContentType_allocbuf (num);

	va_start (ap, num);
	for (i = 0; i < num; i++) {
		type = va_arg (ap, char *);
		types->_buffer[i] = CORBA_string_alloc (strlen (type) + 1);
		strcpy (types->_buffer[i], type);
	}
	va_end (ap);

	return types;
}

/**
 * matecomponent_persist_construct:
 * @persist: A MateComponentPersist
 * @iid: OAF IID of the object this interface is aggregated to
 *
 * Initializes the MateComponentPersist object. You should only use this
 * method in derived implementations, because a MateComponentPersist instance
 * doesn't make a lot of sense, but the iid private field has to be
 * set at construction time.
 *
 * Returns: the #MateComponentPersist.
 */
MateComponentPersist *
matecomponent_persist_construct (MateComponentPersist *persist,
			  const gchar   *iid)
{
	g_return_val_if_fail (persist != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_PERSIST (persist), NULL);

	g_return_val_if_fail (iid != NULL, NULL);

	persist->priv->iid = g_strdup (iid);

	return persist;
}

/**
 * matecomponent_persist_set_dirty:
 * @persist: A MateComponentPersist
 * @dirty: A flag indicating the dirty status of this object.
 *
 * Sets the dirty status of the interface which is reported via
 * the isDirty method.
 */
void
matecomponent_persist_set_dirty (MateComponentPersist *persist, gboolean dirty)
{
	persist->priv->dirty = dirty;
}
